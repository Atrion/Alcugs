/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
*    See the file AUTHORS for more info about the team                         *
*                                                                              *
*    This program is free software; you can redistribute it and/or modify      *
*    it under the terms of the GNU General Public License as published by      *
*    the Free Software Foundation; either version 2 of the License, or         *
*    (at your option) any later version.                                       *
*                                                                              *
*    This program is distributed in the hope that it will be useful,           *
*    but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
*    GNU General Public License for more details.                              *
*                                                                              *
*    You should have received a copy of the GNU General Public License         *
*    along with this program; if not, write to the Free Software               *
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
*                                                                              *
*    Please see the file COPYING for the full license.                         *
*    Please see the file DISCLAIMER for more details, before doing nothing.    *
*                                                                              *
*                                                                              *
*******************************************************************************/

#ifndef __U_NETSESSION_H
#define __U_NETSESSION_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSION_H_ID "$Id$"

#include "protocol/protocol.h"
#include "netmsgq.h"

#include <netinet/in.h>

namespace alc {
	
	class tNetSessionIte;
	class tUnet;

#define UNetUpgraded 0x01
//#define UNetNoConn   0x02

/** base class for data associated to a session */
class tNetSessionData {
public:
	tNetSessionData() {}
	virtual ~tNetSessionData() {}
};

class tNetSession {
	friend class tUnet; // these two classes work together closely

// methods
public:
	tNetSession(tUnet * net,uint32_t ip,uint16_t port,uint32_t sid); //ip, port in network order
	~tNetSession();
	tString str(bool detail = true);
	size_t getMaxFragmentSize() { return(static_cast<size_t>(maxPacketSz)-getHeaderSize()); }
	size_t getMaxDataSize() { return(getMaxFragmentSize() * 256); }
	size_t getHeaderSize();
	tNetSessionIte getIte();
	time_t onlineTime(void) { return alcGetTime()-nego_stamp.seconds; }
	void send(tmBase &msg, tNetTimeDiff delay = 0); //!< delay is in msecs - may be called in worker thread
	void terminate(int tout);
	void setAuthData(uint8_t accessLevel, const tString &passwd);
	
	void setTimeout(unsigned int tout) { conn_timeout=tout*1000*1000; } //!< set timeout (in seconds)
	void challengeSent(void) { if (authenticated == 0) authenticated = 10; }
	void setRejectMessages(bool reject) { rejectMessages = reject; }
	
	int getSid(void) { return sid; }
	uint8_t getPeerType() { return whoami; }
	tNetTimeDiff getRTT() { return rtt; }
	bool isConnected() { return cabal!=0; }
	uint32_t getIp(void) { return ip; }
	uint16_t getPort(void) { return port; }
	uint8_t getAccessLevel(void) { return accessLevel; }
	uint8_t getAuthenticated(void) { return authenticated; }
	size_t getMaxPacketSz(void) { return maxPacketSz; }
	bool isClient() { return client; }
	bool isTerminated() { return terminated; }
	void setTypeToGame() { whoami = KGame; }
	bool isAlcugsServer()
		{ return whoami == KLobby || whoami == KGame || whoami == KVault || whoami == KAuth || whoami == KTracking; }
	bool anythingToSend() { return !ackq.empty() || !sndq.empty(); }

private:
	void init();
	void resetMsgCounters(void);
	void processIncomingMsg(void * buf,size_t size); //!< we received a message
	tNetTimeDiff processSendQueues(); //!< send what is in our queues

	void createAckReply(tUnetUruMsg &msg);
	tNetTimeDiff ackSend(); //!< return the maximum wait time before we have to check again
	void ackCheck(tUnetUruMsg &msg);
	
	void acceptMessage(tUnetUruMsg *msg);
	void queueReceivedMessage(tUnetUruMsg *msg);
	void checkQueuedMessages(void);

	static int8_t compareMsgNumbers(uint32_t sn1, uint8_t fr1, uint32_t sn2, uint8_t fr2);
	void updateRTT(tNetTimeDiff newread);
	void increaseCabal();
	void decreaseCabal(bool emergency);
	tNetTimeDiff timeToSend(size_t psize);
	
	void negotiate();

// properties
public:
	// The public properties are not used by tNetSession internally, they server for others to be able to save data about the session
	uint8_t max_version; //!< peer major version
	uint8_t min_version; //!< peer minor version
	
	// used only by some servers
	tString name; //!< peer age name in tracking server, peer account name in lobby and game
	uint8_t serverGuid[8]; //!< hex; server guid in tracking server
	tNetSessionData *data; //!< save additional data (e.g. tracking information)
	
	// used by lobbybase
	uint8_t tpots; //!< tpots version 0=undefined, 1=tpots client, 2=non-tpots client
	uint32_t ki; //!< player set and valid id, otherwise 0
	uint8_t uid[16]; //!< hex; player uid
	tString avatar; //!< peer avatar name if set
	uint8_t challenge[16]; //!< peer challenge (hex)
	uint8_t buildType; //!< type of client (internal/external)
	
	// used by game
	bool joined;
private:
	tUnet * net;
	uint32_t ip; //network order
	uint16_t port; //network order
	uint32_t sid;
	char sockaddr[sizeof(struct sockaddr_in)]; // saves the address information of this peer
	struct { //server message counters
		uint32_t pn; //!< the overall packet number
		uint32_t sn; //!< the message number
		uint8_t pfr; //!< the fragment number of the last packet I sent which required an ack
		uint32_t ps; //!< the msg number of the last packet I sent which required an ack
	} serverMsg;
	struct { // client message counters
		uint8_t pfr;
		uint32_t ps;
	} clientMsg;
	uint8_t validation; //!< store the validation level (0,1,2)
	uint8_t authenticated; //!< is the peer authed? 0 = no, 1 = yes, 2 = it just got authed, 10 = the client got an auth challenge
	uint8_t cflags; //!< session flags
	const uint16_t maxPacketSz; //!< maxium size of the packets. Must be 1024 (always)
	tUnetMsg *rcv; //!< The place to assemble a fragmented message

	//flood control
	tNetTime flood_last_check;
	unsigned int flood_npkts;
	
	tNetTime receive_stamp; //!< last time we got something from this client
	tNetTime send_stamp; //!< last time we sent something to this client
	tTime nego_stamp; //!< initial negotiation stamp
	tTime renego_stamp; //!< remote/received nego stamp (stamp of last nego we got)
	tNetTimeDiff conn_timeout; //!< time after which the session will timeout (in microseconds)
	bool negotiating; //!< set to true when we are waiting for the answer of a negotiate we sent
	
	tString passwd; //!< peer passwd hash (used in V2) (string)
	uint8_t accessLevel; //!< peer access level

	//flux control (bandwidth and latency)
	unsigned int cabal; //!< cur avg bandwidth (in bytes per second), can't be > maxBandwidth, will grow slower when > minBandwith
	unsigned int consecutiveCabalIncreases; //!< count how often in a row we increased the cabal - reset to 0 when decreasing
	tNetTime next_msg_time; //!< time to send next msg in usecs (referring to tUnet::net_time)
	tNetTimeDiff rtt; //!< round trip time, used to caluclate timeout
	int deviation; //!< used to calculate timeout
	tNetTimeDiff msg_timeout; //!< time after which a message is re-sent
	
	// queues
	tPointerList<tUnetAck> ackq; //!< ack queue, saves acks to be packed - acks do not intersect here, and are sorted by the SNs they ack
	tPointerList<tUnetUruMsg> sndq; //!<outcomming message queue
	tPointerList<tUnetUruMsg> rcvq; //!< received, but not yet accepted messages
	
	// other status variables
	bool rejectMessages; //!< when set to true, messages are rejected (the other side has to send them again)
	
	bool terminated; //!< false: connection is established; true: a NetMsgTerminated was sent (and we expect a NetMsgLeave), or a NetMsgLeave was sent
	
	uint8_t whoami; //!< peer type
	bool client; //!< it's a client or a server?

	FORBID_CLASS_COPY(tNetSession)
};


}

#endif
