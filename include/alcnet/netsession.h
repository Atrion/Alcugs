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

class tNetSession {
public:
	tNetSession(tUnet * net,U32 ip,U16 port,int sid); //ip, port in network order
	~tNetSession();
	char * str(char how='s');
	U32 getMaxFragmentSize();
	U32 getMaxDataSize();
	U32 getHeaderSize();
	inline void setTimeout(U32 tout) { conn_timeout=tout; }
	//void setPeerType(Byte wtf) { whoami=wtf; }
	//Byte getPeerType() { return whoami; }
	void send(tmMsgBase & t);
	tNetSessionIte getIte();
	U32 getRTT() { return rtt; }
	inline bool isConnected() { return cabal!=0; }
	void checkAlive(void);
	inline bool isAuthed(void) { return authenticated; }
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
	void duplicateTimeout();
	void increaseCabal();
	void decreaseCabal(bool partial);
	U32 computetts(U32 pkqsize);
	
	void negotiate();
	
	tUnet * net;
	U32 ip; //network order
	U16 port; //network order
	int sid;
	char sock_array[sizeof(struct sockaddr_in)];
	socklen_t a_client_size;
	//server Message counters
	struct {
		Byte val;
		U32 pn;
		Byte tf;
		Byte frn;
		U32 sn;
		Byte pfr;
		U32 ps;
	} server;
	Byte validation; //store the validation level (0,1,2)
	Byte authenticated; //it's the peer authed? 0 = no, 1 = yes, 2 = it just got authed, 10 = the client got an auth challenge
	tNetSessionFlags cflags; //Session flags
	U16 maxPacketSz; //Maxium size of the packets. Must be 1024 (always)

	char * w; //rcv window
	U32 wite;
	U32 rcv_win;
	//flood control
	U32 flood_last_check;
	U32 flood_npkts;
	
	tTime timestamp; //current client time
	tTime nego_stamp; //initial negotiation stamp
	tTime renego_stamp; //remote nego stamp
	U32 conn_timeout; //In secs
	bool negotiating;
	
	Byte passwd[34]; //peer passwd hash (used in V2) (string)

	//flux control
	U32 bandwidth; //client reported bandwidth (negotiated technology) (in bps)
	U32 cabal; //cur avg bw (in bytes per second)
	U32 max_cabal;
	
	U32 last_msg_time; //last snd msg time in usecs
	U32 rtt;
	U32 timeout;
	S32 desviation;
	U32 ack_rtt; //ack rtt
	
	tUnetMsgQ<tUnetAck> * ackq; //Pig acks
	tUnetMsgQ<tUnetUruMsg> * sndq; //outcomming message queue
	tUnetMsgQ<tUnetMsg> * rcvq; //incomming message queue
	
	bool idle;

	friend class tNetSessionMgr;
	friend class tUnet;
	//friend class tUnetUruMsg;
	friend class tmMsgBase;
	friend class tUnetBase;
	friend class tUnetLobbyServerBase; // it has to do the authenticate stuff

public:
	//Byte whoami; //peer type
	bool client; //it's a client or a server?

	Byte bussy; //bussy flag (0,1) If this flag is activated, messages are keept in the rcv buffer
	Byte max_version; //peer major version
	Byte min_version; //peer minor version
	Byte tpots; //tpots version 0=undefined, 1=tpots client, 2=non-tpots client
	U32 proto; //peer unet protocol version
	
	U32 ki; //player set and valid id

	bool terminated; //!< false: connection is established; true: a NetMsgTerminated was sent (and we expect a NetMsgLeave), or a NetMsgLeave was sent
};

#if 0
	U32 alive_stamp; //last time that we send the NetMsgAlive
	st_unet_hmsg hmsg; //the message header
	char name[201]; //peer name (vault, lobby, AvatarCustomization) or player name (string)
	char acct[201]; //peer account name (string)
	char uid[41]; //peer uid (client) (in hex)
	char guid[20]; //peer guid (server) (string)
	
	Byte challenge[16]; //peer challenge (used in auth) (hex)
	int x; //x value
	Byte reason; //reason code
	Byte release; //type of client
	Byte access_level; //the access level of the peer
	Byte status; //the player status, defined inside a states machine (see the states machine doc)
	Byte paged; //0x00 non-paged player, 0x01 player is paged
#endif


}

#endif
