/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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

/**
	URUNET 3+
*/

#ifndef __U_NETSESSION_H
#define __U_NETSESSION_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSION_H_ID "$Id$"

namespace alc {

class tNetSessionIte;
class tNetSessionMgr;
class tUnet;
class tUnetLobbyServerBase;

typedef Byte tNetSessionFlags;

#define UNetUpgraded 0x01
#define UNetNoConn   0x02

/** base class for data associated to a session */
class tNetSessionData {
public:
	tNetSessionData() {}
	virtual ~tNetSessionData() {}
};

class tNetSession {
	friend class tUnet;
	friend class tUnetBase;
	friend class tUnetLobbyServerBase; // it has to do the authenticate stuff

// methods
public:
	tNetSession(tUnet * net,U32 ip,U16 port,int sid); //ip, port in network order
	~tNetSession();
	const char * str(bool detail = true);
	U32 getMaxFragmentSize();
	U32 getMaxDataSize();
	U32 getHeaderSize();
	inline void setTimeout(U32 tout) { conn_timeout=tout; }
	inline int getSid(void) { return sid; }
	Byte getPeerType() { return whoami; }
	tNetSessionIte getIte();
	U32 getRTT() { return rtt; }
	inline bool isConnected() { return cabal!=0; }
	void checkAlive(void);
	inline bool isAuthed(void) { return authenticated == 1; }
	inline U32 getIp(void) { return ip; }
	inline U16 getPort(void) { return port; }
	inline Byte getAccessLevel(void) { return accessLevel; }
	inline Byte getTpots(void) { return tpots; }
	U32 onlineTime(void);
	void send(tmBase &msg);

private:
	void init();
	void processMsg(Byte * buf,int size);
	void doWork();

	Byte checkDuplicate(tUnetUruMsg &msg);

	void createAckReply(tUnetUruMsg &msg);
	void ackUpdate();
	void ackCheck(tUnetUruMsg &msg);
	
	void assembleMessage(tUnetUruMsg &msg);

	void updateRTT(U32 newread);
	void increaseCabal();
	void decreaseCabal(bool partial);
	U32 timeToSend(U32 psize);
	
	void negotiate();

// properties
public:
	Byte max_version; //!< peer major version
	Byte min_version; //!< peer minor version
	U32 proto; //!< peer unet protocol version
	
	U32 ki; //!< player set and valid id, otherwise 0 (lobby/game)
	U32 x; //!< x value (for lobby/game)
	Byte uid[16]; //!< hex; player uid in lobby/game server
	Byte name[200]; //!< peer age name in tracking server, peer account name in lobby and game
	Byte serverGuid[8]; //!< hex; server guid in tracking server
	tNetSessionData *data; //!< save additional data (e.g. tracking information)

private:
	tUnet * net;
	U32 ip; //network order
	U16 port; //network order
	int sid;
	char sock_array[sizeof(struct sockaddr_in)]; // used by tUnet
	socklen_t a_client_size; // used by tUnet
	//server Message counters
	struct {
		//Byte val;
		U32 pn; //!< the overall packet number
		//Byte tf;
		//Byte frn;
		U32 sn; //!< the message number
		Byte pfr; //!< the fragment number of the last packet I sent which required an ack
		U32 ps; //!< the msg number of the last packet I sent which required an ack
	} serverMsg;
#ifdef ENABLE_NEWDROP
	// client Message counter
	U32 clientPs; //!< sn of last acked packet we got
	bool lastMsgComplete; //!< true when the last msg we got from that client is complete (i.e. all fragments are revieved)
	#ifdef ENABLE_UNET2
	U32 lastAckSn; //!< sn of the last ack packet we got (work-around for unet2 bug)
	#endif
#endif
	Byte validation; //!< store the validation level (0,1,2)
	Byte authenticated; //!< is the peer authed? 0 = no, 1 = yes, 2 = it just got authed, 10 = the client got an auth challenge
	tNetSessionFlags cflags; //!< ession flags
	U16 maxPacketSz; //!< maxium size of the packets. Must be 1024 (always)

#ifndef ENABLE_NEWDROP
	char * w; //rcv window
	U32 wite;
	U32 rcv_win;
#endif
	//flood control
	U32 flood_last_check;
	U32 flood_npkts;
	
	tTime timestamp; //!< last time we got something from this client
	tTime nego_stamp; //!< initial negotiation stamp
	tTime renego_stamp; //!< remote nego stamp (stamp of last nego we got)
	U32 conn_timeout; //!< time after which the session will timeout (in secs)
	bool negotiating; //!< set to true when we are waiting for the answer of a negotiate we sent
	
	Byte passwd[33]; //!< peer passwd hash (used in V2) (string)
	Byte accessLevel; //!< peer access level

	//flux control
	U32 bandwidth; //!< client reported bandwidth (in byte/s)
	U32 cabal; //!< cur avg bandwidth (in bytes per second), can't be > max_cabal
	U32 max_cabal; //!< min(client's upstream, our downstream), cabal will grow slower when it gets to this mark, will never be > bandwidth
	
	U32 next_msg_time; //!< time to send next msg in usecs (referring to tUnet::net_time)
	U32 rtt; //!< round trip time, used to caluclate timeout
	S32 deviation; //!< used to calculate timeout
	U32 timeout; //!< time after which a message is re-sent
	
	tUnetMsgQ<tUnetAck> * ackq; //!< ack queue, saves acks to be packed
	tUnetMsgQ<tUnetUruMsg> * sndq; //!<outcomming message queue
	tUnetMsgQ<tUnetMsg> * rcvq; //!< incomming message queue
	
	bool idle; //!< true when the session has nothing to do (all queue emtpy)
	bool delayMessages; //!< when set to true, messages are kept in the recieve buffer
	
	bool terminated; //!< false: connection is established; true: a NetMsgTerminated was sent (and we expect a NetMsgLeave), or a NetMsgLeave was sent
	
	Byte whoami; //!< peer type
	bool client; //!< it's a client or a server?
	Byte tpots; //!< tpots version 0=undefined, 1=tpots client, 2=non-tpots client
	
	// used by lobbybyse
	Byte challenge[16]; //!< peer challenge (hex)
	Byte release; //!< type of client (internal/external)
	bool inRoute; //!< saves if the player is on his way to another server
};

#if 0
	Byte reason; //reason code
	Byte status; //the player status, defined inside a states machine (see the states machine doc)
	Byte paged; //0x00 non-paged player, 0x01 player is paged
#endif


}

#endif
