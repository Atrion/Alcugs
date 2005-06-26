/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs H'uru Server Team                     *
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

class tNetSessionMgr;
class tUnet;

class tNetSession {
public:
	tNetSession(tUnet * net);
	~tNetSession();
	char * str(char how='s');
private:
	void init();
	void processMsg(Byte * buf,int size);
	void doWork();

	void createAckReply(tUnetUruMsg &msg);
	void ackUpdate();

	void updateRTT(U32 newread);
	void increaseCabal();
	void decreaseCabal(bool partial);
	
	void negotiate();
	
	tUnet * net;
	U32 ip;
	U16 port;
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
	Byte authenticated; //it's the peer authed? (0,1,2)
	U16 maxPacketSz; //Maxium size of the packets. Must be 1024 (always)

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
	U32 success;
	
	U32 last_msg_time; //last snd msg time in usecs
	U32 rtt;
	U32 timeout;
	S32 desviation;
	
	tUnetMsgQ<tUnetAck> * ackq; //Pig acks
	tUnetMsgQ<tUnetUruMsg> * sndq; //outcomming message queue
	//tUnetInMsgQ * rcvq; //incomming message queue
	
	bool idle;

	friend class tNetSessionMgr;
	friend class tUnet;
	//friend class tUnetUruMsg;
};

#if 0
//! Urunet kernel client header -
typedef struct {
	st_uru_head client; //!<client session vars (remote)
	st_uru_head server; //!<server session vars (local)

	U32 old_p_n; //previus p_n, used in the aging control
	char * w; //window
	U32 wite;
	//---
	int whoami; //peer type

	Byte bussy; //bussy flag (0,1) If this flag is activated, messages are keept in the rcv buffer
	Byte max_version; //peer major version
	Byte min_version; //peer minor version
	Byte tpots; //tpots version 0=undefined, 1=tpots client, 2=non-tpots client
	
	U32 alive_stamp; //last time that we send the NetMsgAlive
	st_unet_hmsg hmsg; //the message header
	char name[201]; //peer name (vault, lobby, AvatarCustomization) or player name (string)
	char acct[201]; //peer account name (string)
	char uid[41]; //peer uid (client) (in hex)
	char guid[20]; //peer guid (server) (string)
	
	Byte challenge[16]; //peer challenge (used in auth) (hex)
	int ki; //player set and valid id
	int x; //x value
	Byte reason; //reason code
	Byte release; //type of client
	Byte access_level; //the access level of the peer
	Byte status; //the player status, defined inside a states machine (see the states machine doc)
	Byte paged; //0x00 non-paged player, 0x01 player is paged
	//flux control
	U32 bandwidth; //client reported bandwidth (negotiated technology)
	Byte window; //the peer window size
	//flood control
	U32 last_check; //time of last check
	int npkts; //number of packets since last check
} st_uru_client;

#endif


}

#endif
