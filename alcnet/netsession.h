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

#include "netqueue.h"
#include "protocol/protocol.h"
#include <alctypes.h>

#include <netinet/in.h>

namespace alc {
	
	
	class tUnet;
	class tmBase;
	class tUnetUruMsg;
	class tLog;
	class tUnetMsg;
	class tUnetAck;

/**
 * This class manages a session.
 * It is responsible for the startup (negotation) and shutdown of a connection, for receiving and (re-)sending messages.
 * Events for the upper layers are added to the unet event queue. */
class tNetSession {
// Types
public:
	typedef enum { Unknown, Negotiating, Connected, Terminating, Leaving } tConnectionState;
	typedef enum { UnknownGame, POTSGame, UUGame } tGameType;
// methods
public:
	tNetSession(tUnet * net,uint32_t ip,uint16_t port,uint32_t sid,bool client,uint8_t validation); //ip, port in network order
	~tNetSession();
	tString str();
	void send(const tmBase &msg, tNetTime delay = 0); //!< delay is in msecs - may be called in worker thread
	void terminate(uint8_t reason = 0);
	void setAuthData(uint8_t accessLevel, const tString &passwd);
	
	void setTimeout(tNetTime tout) { tWriteLock lock(prvDataMutex); conn_timeout=tout; } //!< set timeout (in seconds)
	void setRejectMessages(bool reject) { tWriteLock lock(prvDataMutex); rejectMessages = reject; }
	
	uint32_t getSid(void) { return sid; } //!< sid will never change, so this is thread-safe
	uint32_t getIp(void) const { return ip; }
	uint16_t getPort(void) const { return port; }
	size_t getMaxPacketSz(void) const;
	uint8_t getAccessLevel(void) { tReadLock lock(prvDataMutex); return accessLevel; }
	bool isUruClient(void) { tReadLock lock(prvDataMutex); return authenticated; } //!< thread-safe
	bool isClient(void) { tReadLock lock(prvDataMutex); return client; } //!< thread-safe
	bool isTerminated(void) { tReadLock lock(prvDataMutex); return state >= Terminating; } //!< thread-safe
	bool anythingToSend(void) { tMutexLock lock(sendMutex); return acksToSend() || !sndq.empty(); } //!< thread-safe
	bool useUpdatedProtocol(void) { tReadLock lock(prvDataMutex); return upgradedProtocol; } //!< thread-safe
	tLog *getLog(void);
	
	void incRefs() { __sync_add_and_fetch(&refs, 1); }
	void decRefs();
	
	void processIncomingMsg(void * buf,size_t size); //!< we received a message
	tNetTimeBoolPair processSendQueues(); //!< send what is in our queues, returning the sleep timeout and a boolean which is true if we want to be deleted

private:
	void resetMsgCounters(void);
	void rawsend(tUnetUruMsg *msg);

	void createAckReply(const tUnetUruMsg* msg);
#ifdef ENABLE_ACKCACHE
	tNetTime ackSend(); //!< return the maximum wait time before we have to check again
	bool acksToSend() { return !ackq.empty(); }
#else
	bool acksToSend() { return false; }
#endif
	void ackCheck(tUnetUruMsg &msg);
	
	void acceptMessage(tUnetUruMsg *msg);
	void queueReceivedMessage(tUnetUruMsg *msg);
	void checkQueuedMessages(void);
	bool parseBasicMsg(tUnetMsg * msg);

	tConnectionState getState() { tReadLock lock(prvDataMutex); return state; }
	void updateRTT(tNetTime newread);
	void increaseCabal();
	void decreaseCabal(bool emergency);
	tNetTime timeToSend(size_t size) { return static_cast<double>(size)/cabal; }
	
	void negotiate();

// properties
public:
	// The public properties are not used by tNetSession internally, they serve for others to be able to save data about the session
	uint8_t max_version; //!< peer major version
	uint8_t min_version; //!< peer minor version
	
	// used only by some servers
	tString name; //!< peer age name in tracking server, peer account name in lobby and game
	uint8_t serverGuid[8]; //!< hex; server guid in tracking server
	tBaseType *data; //!< save additional data (e.g. tracking information)
	
	// used by lobbybase (lobby and game)
	tGameType gameType;
	uint32_t ki; //!< player set and valid id, otherwise 0
	uint8_t uid[16]; //!< hex; player uid - only set for whoami == KClient
	tString avatar; //!< peer avatar name if set
	uint8_t challenge[16]; //!< peer challenge (hex)
	uint8_t buildType; //!< type of client (internal/external)
	
	// used by game
	bool joined;
	time_t joinedTime;
	
	// synchronization
	tReadWriteEx pubDataMutex; //!< protecting above public variables - take it when accessing from main thread, or writing from worker thread (reading from worker is safe always)
private:
	tUnet *net;
	const uint32_t ip; //!< network order
	const uint16_t port; //!< network order
	const uint32_t sid;
	struct sockaddr_in sockaddr; //!< saves the address information of this peer
	struct { //server message counters, protected by send mutex
		uint32_t pn; //!< the overall packet number
		uint32_t sn; //!< the message number
		uint32_t cps; //!< the msg and fragment number of the last packet I sent which required an ack
	} serverMsg;
	struct { // client message counters
		uint32_t cps; //!< msg and fragment number of last acked message I got
		uint32_t psn(void) const { return cps >> 8; }
		uint8_t pfr(void) const { return cps & 0xFF; }
	} clientMsg;
	uint8_t validation; //!< store the validation level (0,1,2)
	bool upgradedProtocol; //!< use the alcugs upgraded protocol; protected by prvDataMutex
	tUnetMsg *rcv; //!< The place to assemble a fragmented message

	//flood control
	tNetTime flood_last_check;
	unsigned int flood_npkts;
	
	tNetTime receive_stamp; //!< last time we got something from this client
	tNetTime send_stamp; //!< last time we sent something to this client
	tNetTime conn_timeout; //!< time after which the session will timeout; protected by prvDataMutex
	tTime renego_stamp; //!< remote/received nego stamp (stamp of last nego we got)
	
	tString passwd; //!< peer passwd hash (used in V2) (string); protected by prvDataMutex
	bool authenticated; //!< set to true when the peer acked the "you got authed" message (obviously we can't use the passwd for that one yet); protected by prvDataMutex
	uint8_t accessLevel; //!< peer access level; protected by prvDataMutex

	//flux control (bandwidth and latency)
	unsigned int cabal; //!< cur avg bandwidth (in bytes per second), can't be > maxBandwidth, will grow slower when > minBandwith
	unsigned int consecutiveCabalIncreases; //!< count how often in a row we increased the cabal - reset to 0 when decreasing
	tNetTime next_msg_time; //!< time to send next msg in usecs (referring to tUnet::net_time)
	tNetTime rtt; //!< round trip time, used to caluclate timeout
	tNetTime deviation; //!< used to calculate timeout
	tNetTime msg_timeout; //!< time after which a message is re-sent
	
	// queues
#ifdef ENABLE_ACKCACHE
	typedef std::list<tUnetAck> tAckList;
	tAckList ackq; //!< ack queue, saves acks to be packed - acks do not intersect here, and are sorted by the SNs they ack
#endif
	tNetQeue<tUnetUruMsg> sndq; //!< outgoing message queue, protected by send mutex
	tNetQeue<tUnetUruMsg> rcvq; //!< received, but not yet accepted messages
	
	// other status variables
	tConnectionState state; //!< current connection state, protected by prvDataMutex
	bool rejectMessages; //!< when set to true, messages are rejected (the other side has to send them again); protected by prvDataMutex
	bool client; //!< it's a client or a server?
	
	// mutexes
	tMutex sendMutex; //!< protecting serverMsg and sndq
	tReadWriteEx prvDataMutex; //!< protecting terminated, conn_timeout, rejectMessages, client, authenticated, accessLevel, passwd - inside of pubDataMutex
	
	// reference counting
	int refs;

	FORBID_CLASS_COPY(tNetSession)
};


}

#endif
