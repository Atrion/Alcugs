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

#ifndef __U_URUNET_H
#define __U_URUNET_H
/* CVS tag - DON'T TOUCH*/
#define __U_URUNET_H_ID "$Id$"

namespace alc {

//udp packet max buffer size (0xFFFF) - any packet should be bigger.
#define INC_BUF_SIZE 65535
#define OUT_BUFFER_SIZE 1024

//! Urunet event table <0 errors, 0 ok, >0 events
#define UNET_PARSEERR -12 /* !< Error parsing a plNet Msg */
#define UNET_OUTOFRANGE -11 /* !<  Out of range */
#define UNET_NOTAUTHED -10 /* !< Not authenticated */
#define UNET_REJECTED -9 /* !< Connection was rejected */
#define UNET_TOMCONS -8 /* !< Too many connections, new connection ignored */
#define UNET_NONURU -7 /* !< Recieved a message of a unknown protocol */
#define UNET_CRCERR -6 /* !< checksum validation failed */
#define UNET_TOOBIG -5 /* !< message too big */
#define UNET_ERR -4 /* !< an error occurred */
#define UNET_NOBIND -3 /* !< Impossible to bind to address */
#define UNET_INHOST -2 /* !< Invalid host (cannot resolve or bind to address) */
#define UNET_FINIT -1  /* !< Cannot initizalize the netcore */
#define UNET_OK 0 /* !< Successfull operation (or netcore timeout loop) */
#define UNET_MSGRCV 1 /* !< A new message has been recieved */
#define UNET_NEWCONN 2 /* !< New incomming connection stablished */
#define UNET_TIMEOUT 3 /* !< Connection to peer has ended the timer */
#define UNET_TERMINATED 4 /* !< Connection to peer terminated */
#define UNET_FLOOD 5 /* !< This event occurs when a player is flooding the server */
//note events UNET_FLOOD, UNET_TERMINATED, UNET_NEWCONN, and UNET_MSGRCV always contain
// a new incoming message (from the affected peer), if the size of it is non-zero.
//other events don't contain an incomming message, and the message size will be always zero

typedef U16 tUnetFlags;

//! Urunet flags
#define UNET_NBLOCK   0x0001 /* non-blocking socket */
#define UNET_ELOG     0x0002 /* enable netcore logging */
#define UNET_ECRC     0x0004 /* crc check enabled */
#define UNET_AUTOSP   0x0008 /* auto speed correction */
#define UNET_NOFLOOD  0x0010 /* enable flooding control */
#define UNET_BCAST    0x0020 /* enable broadcast */
#define UNET_FLOG     0x0040 /* enable file based logging, (also, 0x02 must be present) */
#define UNET_NETAUTH  0x0080 /* Enable authentication through servers */
#define UNET_DLACK    0x0100 /* Dissable ack trace */
#define UNET_DLCHK    0x0200 /* Dissable chk log */
#define UNET_DLUNE    0x0400 /* Dissable une log */
#define UNET_DLSEC    0x0800 /* Dissable sec log */


#define UNET_DEFAULT_FLAGS UNET_NBLOCK | UNET_ELOG | UNET_ECRC | UNET_AUTOSP | UNET_NOFLOOD | UNET_FLOG | UNET_NETAUTH


class tUnet {
public:
	tUnet(char * lhost="0.0.0.0",U16 lport=0);
	void setFlags(tUnetFlags flags);
	void unsetFlags(tUnetFlags flags);
	tUnetFlags getFlags();
	void startOp();
	void stopOp();
	virtual ~tUnet();
	void dump(tLog * f=NULL,Byte flags=0x01);
	int Recv();
	U32 getTime() { return ntime_sec; }
	U32 getMicroseconds() { return ntime_usec; }
	//max resolution 15 minutes
	U32 getNetTime() { return net_time; }
	void updateNetTime();
	tNetEvent * getEvent();
	tNetSession * getSession(tNetSessionIte &t);
	void destroySession(tNetSessionIte &t);
private:
	void init();
	void _openlogs();
	
	void updatetimer(U32 usecs);
	void doWork();
	
	void neterror(char * msg);
	
	//void send();
	void basesend(tNetSession * u,tmBase & m);
	void rawsend(tNetSession * u,tUnetUruMsg * m);
	
	friend class tNetSession;
	friend class tmMsgBase;
	
	bool initialized;

	bool idle;
#ifdef __WIN32__
	WSADATA ws; //<! The winsock stack
	SOCKET sock; //<! The socket
	u_long nNoBlock; //<! non-blocking (private)
#else
	int sock; //<! The socket
#endif
	int opt;
	struct sockaddr_in server; //<! Server sockaddr
	//!blocking socket
	tUnetFlags flags; //explained -^
	U16 unet_sec; //netcore timeout to do another loop (seconds)
	U32 unet_usec; //netcore timeout to do another loop (microseconds)

	Byte max_version; //Default protocol version
	Byte min_version;

	U32 ntime_sec; //current time (in secs)
	U32 ntime_usec; //(in usecs)
	U32 net_time; //(in usecs) [resolution of 15 minutes] (relative)

	U32 conn_timeout; //default timeout (to disconnect a session) (seconds) [3 secs]
	U32 timeout; //default timeout when the send clock expires (re-transmission) (microseconds)

	U32 max; //<! Maxium number of connections (default 0, unlimited)
	tNetSessionMgr * smgr; //Session MGR

	int whoami; //type of _this_ server

	U32 lan_addr; //<! LAN address, in network byte order
	U32 lan_mask; //<! LAN mask, in network byte order (default 255.255.255.0)
	//! Bandwidth speed (lo interface -> maxium)
	U32 lan_up;
	U32 lan_down;
	U32 nat_up;
	U32 nat_down;

	char name[200]; //<! The system/server name, normally the age filename
	char guid[18]; //<! This system guid (age guid) (in Ascii)

	char address[100]; //<! This system public address (in Ascii)
	char bindaddr[100]; //<! Server bind address
	U16 bindport; //<! Server bind port

	U16 spawn_start; //first port to spawn
	U16 spawn_stop; //last port to spawn (gameservers)

#if 0
	//peers
	int auth; // (Game/Lobby)
	int vault; // (Game/Lobby/Tracking)
	int tracking; // (Game/Lobby)
	int meta; // (Tracking(local meta)/Lobby(remote metas))
	int data; // (Meta) [hmm]

	//protocol versions (0=newest)
	int pro_auth;
	int pro_vault;
	int pro_tracking;
	int pro_meta;
	int pro_data;
#endif

protected:
	//!logging subsystem
	tLog * log; //stdout
	tLog * err; //stderr
	tLog * unx; //unexpected
	tLog * ack; //ack drawing
	//tLog * chk; //checksum results
	tLog * sec; //security log
private:
	
	//event queue
	tUnetMsgQ<tNetEvent> * events;

	//flood control
	U32 max_flood_pkts;
	U32 flood_check_sec;
	
	U32 snd_expire; //(seconds to expire a message in the rcv buffer)
	
	U32 ip_overhead;
	//debugging stuff
	#ifdef _UNET_DBG_
	U32 lim_down_cap; //in bytes
	U32 lim_up_cap; //in bytes
	Byte in_noise; //(0-100)
	Byte out_noise; //(0-100)
	U32 latency; //(in msecs)
	U32 cur_down_quota;
	U32 cur_up_quota;
	Byte quota_check_sec; //(in seconds)
	U32 quota_check_usec; //(useconds)
	U32 time_quota_check_sec; //last quota check (in seconds)
	U32 time_quota_check_usec;
	#endif
};

}

#if 0

int plNetConnect(st_unet * net,int * sid,char * address,U16 port,Byte flags);
#define UNET_VAL0 0x02
#define UNET_VAL1 0x01
#define UNET_VAL2 0x00
#define UNET_VAL3 0x03

int plNetSendMsg(st_unet * net,Byte * msg,int size,int sid,Byte flags);

int plNetGetMsg(st_unet * net,int sid,Byte ** msg);

char plNetIsFini(st_unet * net,int sid);

int plNetClientSearchByKI(st_unet * net,U32 ki);
int plNetClientSearchByIp(st_unet * net,U32 ip,U16 port);
int plNetServerSearch(st_unet * net,Byte type);

void plNetEndConnection(st_unet * net,int sid);

#endif

#endif
