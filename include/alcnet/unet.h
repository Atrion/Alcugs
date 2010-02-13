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

#ifndef __U_UNET_H
#define __U_UNET_H
/* CVS tag - DON'T TOUCH*/
#define __U_UNET_H_ID "$Id$"

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
#define UNET_FLOODCTR 0x0010 /* enable flooding control */
#define UNET_BCAST    0x0020 /* enable broadcast */
#define UNET_DSTDLOG  0x0040 /* Disable standard logging (even if ELOG is set) */
#define UNET_EACKLOG  0x0100 /* Enable ack trace (ELOG must be set, too) */
#define UNET_DSECLOG  0x0800 /* Disable sec log (even if ELOG is set) */
#define UNET_DERRLOG  0x1000 /* Disable error logging (even if ELOG is set) */

//#define UNET_AUTOSP   0x0008 /* auto speed correction */
//#define UNET_NETAUTH  0x0080 /* Enable authentication through servers */
	// UNET_NETAUTH was meant to enable an authentication for auth, tracking and vault against lobby and game and vice versa. it was never implemented though.
//#define UNET_DLCHK    0x0200 /* Disable chk log */
//#define UNET_DLUNE    0x0400 /* Disable une log */
//#define UNET_NOCONN   0x2000 /* Non connected server */

#define UNET_DEFAULT_FLAGS (UNET_NBLOCK | UNET_ELOG | UNET_ECRC | UNET_FLOODCTR)


class tUnet {
	friend class tNetSession; // these two classes work together closely

// methods
public:
	tUnet(Byte whoami,const char * lhost="0.0.0.0",U16 lport=0); //lport in host order
	virtual ~tUnet();
	void setFlags(tUnetFlags flags);
	void unsetFlags(tUnetFlags flags);
	tUnetFlags getFlags();
	void startOp();
	void stopOp();
	void dump(tLog * f=NULL,Byte flags=0x01);
	tNetSession * getSession(tNetSessionIte &t);
	//
	void setIdleTimer(Byte timer) {
		if(timer!=0) this->idle_timer=timer;
	}
	void setBindPort(U16 lport); //lport in host order
	void setBindAddress(const char * lhost);
	inline void send(tmMsgBase &m, U32 delay = 0) { m.getSession()->send(m, delay); } //!< delay is in msecs

protected:
	void openLogfiles();
	void destroySession(tNetSessionIte &t);
	void updateTimerRelative(U32 usecs);
	inline void updateTimerAbs(U32 usecs) { updateTimerRelative(usecs-net_time); }
	tNetSessionIte netConnect(const char * hostname,U16 port,Byte validation,Byte flags,Byte peerType=0);
	int Recv();
	U32 getTime() { return ntime.seconds; }
	//max resolution 15 minutes
	U32 getNetTime() { return net_time; }
	tNetEvent * getEvent();
	
	virtual bool canPortBeUsed(U16 /*port*/) { return false; }

private:
	void init();
	void doWork();
	
	void neterror(const char * msg);
	
	void updateNetTime();
	
	void basesend(tNetSession * u,tmBase & m);
	void rawsend(tNetSession * u,tUnetUruMsg * m);

// properties
protected:
	bool idle; //!< true if all session are idle

	U32 conn_timeout; //!< default timeout (to disconnect a session) (seconds) [5 secs]
	U32 timeout; //!< default timeout when the send clock expires (re-transmission) (microseconds)

	U32 max; //!< Maxium number of connections (default 0, unlimited)
	tNetSessionMgr * smgr; //!< session MGR

	int whoami; //!< type of _this_ server

	U32 lan_addr; //!< LAN address, in network byte order
	U32 lan_mask; //!< LAN mask, in network byte order (default 255.255.255.0)
	// Bandwidth speed (lo interface -> maxium)
	U32 lan_up; //!< upstream to the LAN
	U32 lan_down; //!< downstream from the LAN
	U32 nat_up; //!< upstream to the WAN
	U32 nat_down; //!< downstream from the WAN

	char bindaddr[100]; //!< Server bind address
	U16 bindport; //!< Server bind port, in host order

	//!logging subsystem
	tLog * log; //!< stdout
	tLog * err; //!< stderr
	tLog * ack; //!< ack drawing
	tLog * sec; //!< security and access log
	
	//flood control
	U32 max_flood_pkts;
	U32 flood_check_sec;
	
	U32 receiveAhead; //!< number of future messages to receive and cache
	
	U32 ip_overhead;
	//debugging stuff
	#ifdef ENABLE_NETDEBUG
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

private:	
	bool initialized;
	
	Byte max_version; //!< default protocol version
	Byte min_version; //!< default protocol version

	tTime ntime; //!< current time
	U32 net_time; //!< current time (in usecs) [resolution of 15 minutes] (relative)
	Byte idle_timer; //!< maximum time between two odle operations (sec)
	
#ifdef __WIN32__
	WSADATA ws; //!< The winsock stack
	SOCKET sock; //!< The socket
	u_long nNoBlock; //!< non-blocking
#else
	int sock; //!< The socket
#endif
	int opt;
	struct sockaddr_in server; //!< Server sockaddr
	
	tUnetFlags flags; //!< unet flags, explained -^
	U16 unet_sec; //!< netcore timeout to do another loop (seconds)
	U32 unet_usec; //!< netcore timeout to do another loop (microseconds)
	
	tUnetMsgQ<tNetEvent> * events; //!< event queue

	FORBID_CLASS_COPY(tUnet)
};

}

#endif
