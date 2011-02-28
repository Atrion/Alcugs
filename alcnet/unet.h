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

#ifndef __U_UNET_H
#define __U_UNET_H
/* CVS tag - DON'T TOUCH*/
#define __U_UNET_H_ID "$Id$"

#include "netmsgq.h"
#include <alctypes.h>
#include <alcutil/alcthread.h>

#include <netinet/in.h>

namespace alc {
	
	class tLog;
	class tNetSessionIte;
	class tNetEvent;
	class tmBase;
	class tUnetUruMsg;
	class tNetSessionMgr;
	class tNetSession;
	class tmMsgBase;

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
#define UNET_SHUTDOWN_MODE 6 // internal use: tells the worker thread that server shutdown began
#define UNET_KILL_WORKER 7 // internal use: tells the worker thread that it should exit
// only event UNET_MSGRCV will contain a new incoming message (from the affected peer).

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
	tUnet(uint8_t whoami,const tString & lhost="",uint16_t lport=0); //lport in host order
	virtual ~tUnet();
	void setFlags(uint16_t flags);
	void unsetFlags(uint16_t flags);
	uint16_t getFlags();
	void dump(tLog * f=NULL,uint8_t flags=0x01);
	tNetSession * sessionByIte(tNetSessionIte &t);
	void setBindPort(uint16_t lport); //lport in host order
	void setBindAddress(const tString & lhost);
	void send(alc::tmMsgBase& m, tNetTimeDiff delay = 0); //!< delay is in msecs - may be called in worker thread

protected:
	void startOp();
	void stopOp();
	void openLogfiles();
	void destroySession(tNetSessionIte &t);
	tNetSessionIte netConnect(const char * hostname,uint16_t port,uint8_t validation,uint8_t flags,uint8_t peerType=0);
	int sendAndWait(); //!< send enqueued messages, wait, and receive packets and enqueue them (wait time must be set by processQueues()!)
	tNetEvent * getEvent();
	virtual void addEvent(tNetEvent *evt);
	void clearEventQueue(); // use this only if you really know what you do - will loose incoming messages and whatnot!
	tNetTime getNetTime() { return net_time; }
	
	virtual bool canPortBeUsed(uint16_t /*port*/) { return false; }
	
	tNetSession *sessionBySid(size_t n);
	tNetSession *sessionByKi(uint32_t ki);
	bool sessionListEmpty();

private:
	void init();
	tNetTimeDiff processSendQueues(); //!< send messages from the sessions' send queues - also updates the timeouts for the next wait \return the time in usec we should wait before processing again
	
	void neterror(const char * msg);
	
	void updateNetTime();
	
	void rawsend(tNetSession * u,tUnetUruMsg * m);

// properties
protected:
	unsigned int conn_timeout; //!< default timeout (to disconnect a session) (seconds) [5 secs]
	tNetTimeDiff msg_timeout; //!< default timeout when the send clock expires (re-transmission) (microseconds)
	tNetTimeDiff max_sleep; //!< maximum time we sleep in the receive function (microseconds)

	unsigned int max; //!< Maxium number of connections (default 0, unlimited)
	tNetSessionMgr * smgr; //!< session MGR - get below mutex before accessing it! FIXME: apply this everywhere
	tMutex smgrMutex; //!< must never be taken when event list is already taken!

	uint8_t whoami; //!< type of _this_ server

	uint32_t lan_addr; //!< LAN address, in network byte order
	uint32_t lan_mask; //!< LAN mask, in network byte order (default 255.255.255.0)
	// Bandwidth speed (lo interface -> maxium)
	unsigned int lan_up; //!< upstream to the LAN
	unsigned int lan_down; //!< downstream from the LAN
	unsigned int nat_up; //!< upstream to the WAN
	unsigned int nat_down; //!< downstream from the WAN

	tString bindaddr; //!< Server bind address
	uint16_t bindport; //!< Server bind port, in host order

	//!logging subsystem
	tLog * log; //!< stdout
	tLog * err; //!< stderr
	tLog * ack; //!< ack drawing
	tLog * sec; //!< security and access log
	
	//flood control
	unsigned int max_flood_pkts;
	tNetTimeDiff flood_check_interval;
	
	unsigned int receiveAhead; //!< number of future messages to receive and cache
	
	unsigned int ip_overhead;
	//debugging stuff
	#ifdef ENABLE_NETDEBUG
	unsigned int lim_down_cap; //in bytes
	unsigned int lim_up_cap; //in bytes
	unsigned int in_noise; //(0-100)
	unsigned int out_noise; //(0-100)
	unsigned int latency; //(in msecs)
	unsigned int cur_down_quota;
	unsigned int cur_up_quota;
	unsigned int quota_check_interval; //(useconds)
	tTime last_quota_check;
	#endif

private:	
	bool initialized;
	
	uint8_t max_version; //!< default protocol version
	uint8_t min_version; //!< default protocol version

	tNetTime net_time; //!< current netcore time (in usecs) [resolution of 15 minutes] (relative)
	
#ifdef __WIN32__
	WSADATA ws; //!< The winsock stack
	SOCKET sock; //!< The socket
	u_long nNoBlock; //!< non-blocking
#else
	int sock; //!< The socket
#endif
	int opt;
	struct sockaddr_in server; //!< Server sockaddr
	
	uint16_t flags; //!< unet flags, explained -^
	
	tUnetMsgQ<tNetEvent> * events; //!< event queue - get below mutex before accessing it!
	tMutex eventsMutex;

	FORBID_CLASS_COPY(tUnet)
};

}

#endif
