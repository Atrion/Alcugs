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

#ifndef __U_UNET_H
#define __U_UNET_H
/* CVS tag - DON'T TOUCH*/
#define __U_UNET_H_ID "$Id$"

#include "netsessionmgr.h"
#include "protocol/protocol.h"

#include <netinet/in.h>

namespace alc {

	class tLog;
	class tNetEvent;


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
#define UNET_NEWCONN 2 /* !< New connection created (still in contructor) */
#define UNET_TIMEOUT 3 /* !< Connection to peer has ended the timer (no activity for too long, or a packet was not acked) */
#define UNET_CONNCLS 4 /* !< connection is closing - i.e., terminate() was called on it */
#define UNET_FLOOD 5 /* !< This event occurs when a player is flooding the server */
#define UNET_KILL_WORKER 6 // internal use: tells the worker thread that it should exit
// only event UNET_MSGRCV will contain a new incoming message (from the affected peer).

//! Urunet flags
#define UNET_ELOG     0x0002 /* enable netcore logging */
#define UNET_ECRC     0x0004 /* crc check enabled */
#define UNET_FLOODCTR 0x0010 /* enable flooding control */
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

#define UNET_DEFAULT_FLAGS (UNET_ELOG | UNET_ECRC | UNET_FLOODCTR)


/**
 * The netcore: Opening and closing the socket, offering functionality to establish a connection,
 * and manage the list of active sessions. Forwarding incoming messages to the correct session.
 * It offers a function to be called in a loop, that gives all messages a chance to send messages
 * and then sleeps for as long as sessions said they want. */
class tUnet {
	friend class tNetSession; // needs to send messages, print logs, do the NETDEBUG stuff, and so on

// methods
public:
	tUnet(uint8_t whoami,const tString & lhost="",uint16_t lport=0); //lport in host order
	virtual ~tUnet();
	void setFlags(uint16_t flags) { this->flags |= flags; }
	void unsetFlags(uint16_t flags) { this->flags &= ~flags; }
	uint16_t getFlags() { return this->flags; }
	void setBindPort(uint16_t lport) { bindport=lport; } //lport in host order
	uint16_t getBindPort(void) { return bindport; }
	void setBindAddress(const tString & lhost) {
		if(lhost.isEmpty()) bindaddr = "0.0.0.0";
		else bindaddr = lhost;
	}
	tString getBindAddress(void) { return bindaddr; }
	void send(const tmNetMsg& m, tNetTime delay = 0) { m.getSession()->send(m, delay); } //!< delay is in secs - may be called in worker thread

protected:
	void startOp();
	void stopOp();
	void openLogfiles();
	tNetSessionRef netConnect(const char * hostname,uint16_t port,uint8_t validation);
	bool sendAndWait(); //!< send enqueued messages, wait, and receive packets and enqueue them (wait time must be set by processQueues()!) \return if the netcode is idle
	tNetEvent *getEvent(); //!<  (thread-safe)
	void addEvent(tNetEvent *evt); //!<  (thread-safe)
	void clearEventQueue(); //!< use this only if you really know what you do - will loose incoming messages and whatnot! (thread-safe)
	tNetTime getNetTime() { tSpinLock lock(net_time_mutex); return net_time; }
	bool timeOverdue(tNetTime timeout) { //!< returns whether the timout is overdue
		return getNetTime() >= timeout;
	}
	tNetTime remainingTimeTill(tNetTime time); //!< returns time remaining till given timestamp
	tNetTime passedTimeSince(tNetTime time); //!< returns time passed since given timestamp
	void wakeUpMainThread(void); //!< call this from worker to wake up the main thread from its sleep
	virtual bool canPortBeUsed(uint16_t /*port*/) { return false; }
	
	tNetSessionRef sessionBySid(size_t sid) //!< (thread-safe)
	{
		tReadLock lock(smgrMutex);
		return tNetSessionRef(smgr->get(sid));
	}
	tNetSessionRef sessionByKi(uint32_t ki) { //!< (thread-safe)
		tReadLock lock(smgrMutex);
		return tNetSessionRef(smgr->findByKi(ki));
	}
	bool sessionListEmpty() { //!< (thread-safe)
		tReadLock lock(smgrMutex);
		return smgr->isEmpty();
	}

private:
	tNetTimeBoolPair processSendQueues(); //!< send messages from the sessions' send queues - also updates the timeouts for the next wait \return the time in usec we should wait before processing again, and a bool saying whether any session has anything to send
	void updateNetTime();
	void removeConnection(tNetSession *u); //!< destroy that session, must be called in main thread

// properties
protected:
	tNetTime conn_timeout; //!< default timeout (to disconnect a session) (seconds) [5 secs]
	tNetTime msg_timeout; //!< default timeout when the send clock expires (re-transmission) (microseconds)
	tNetTime max_sleep; //!< maximum time we sleep in the receive function (microseconds)

	unsigned int max; //!< Maxium number of connections (default 0, unlimited)
	tNetSessionMgr * smgr; //!< session MGR - get below mutex before accessing it, and keep a tNetSessionRef to session pointers you keep after releasing the mutex!
	tReadWriteEx smgrMutex; //!< must never be taken when event list is already taken! Also, if you got a pointer to a session without this lock hold, be sure to have a tNetSessionRef to it, so that it is not deleted

	const uint8_t whoami; //!< type of _this_ server

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
	tLog *log; //!< stdout
	tLog *err; //!< stderr
	tLog *ack; //!< ack drawing
	tLog *sec; //!< security and access log
	
	//flood control
	unsigned int max_flood_pkts;
	tNetTime flood_check_interval;
	
	unsigned int receiveAhead; //!< number of future messages to receive and cache
	
	//debugging stuff
	#ifdef ENABLE_NETDEBUG
	unsigned int lim_down_cap; //in bytes
	unsigned int lim_up_cap; //in bytes
	unsigned int in_noise; //(0-100)
	unsigned int out_noise; //(0-100)
	tNetTime latency; //(in secs)
	unsigned int cur_down_quota;
	unsigned int cur_up_quota;
	tNetTime quota_check_interval; //(seconds)
	tNetTime last_quota_check;
	#endif
	
	
	tNetQeue<tNetEvent> events; //!< event queue - get below mutex before accessing it!
	tMutex eventsMutex;
	bool workerWaiting; //!< signals whether a worker is waiting for an event using below condition, protected by above mutex
	pthread_cond_t eventAddedCond; //!< condition used to signal the worker thread that it can wake up, protected by above mutex

private:
	const uint8_t max_version; //!< default protocol version
	const uint8_t min_version; //!< default protocol version

	/** current netcore time (in secs). Protected by the spinlock, */
	tNetTime net_time;
	tSpinEx net_time_mutex;
	
	int sock; //!< The socket
	struct sockaddr_in server; //!< Server sockaddr
	int sndPipeReadEnd, sndPipeWriteEnd; //!< the ends of the pipe used to signal a packet wants to be sent
	
	uint16_t flags; //!< unet flags, explained -^
	

	FORBID_CLASS_COPY(tUnet)
};

}

#endif
