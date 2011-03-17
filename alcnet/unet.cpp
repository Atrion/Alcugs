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

/**
	URUNET 3+
*/

/* CVS tag - DON'T TOUCH*/
#define __U_UNET_ID "$Id$"
//#define _DBG_LEVEL_ 3
#include <alcdefs.h>
#include "unet.h"

#include "netmsgq.h"
#include "netexception.h"
#include <alcmain.h>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <cerrno>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <cassert>
#include <pthread.h>

// Some of the macros implie an old-style cast
#pragma GCC diagnostic ignored "-Wold-style-cast"


//udp packet max buffer size (0xFFFF) - no packet should be bigger.
#define INC_BUF_SIZE 65535

namespace alc {
	
static txUnet txUnetWithErrno(tString msg) {
	msg.printf(" Errno %i: %s\n",errno,strerror(errno));
	return txUnet(msg);
}

tUnet::tUnet(uint8_t whoami,const tString & lhost,uint16_t lport) : smgr(NULL), whoami(whoami),
		max_version(12), min_version(7)
{
	setBindAddress(lhost);
	bindport=lport;
	
	workerWaiting = false;
	if (pthread_cond_init(&eventAddedCond, NULL))
		throw txBase(_WHERE("Error initializing condition"));
	
	// fill in default values
	flags=UNET_DEFAULT_FLAGS;

	//netcore timeout < min(all RTT's), nope, it must be the min tts (stt)
	max_sleep=10; // wait no more than 10 seconds in select() call

	conn_timeout=150; // default timeout for new sessions (seconds)
	/* This sets the timeout for unet servers from both sides.
	It also sets the timeout for Uru clients, where a high timeout is necessary since the connection is already established when changing
	the server in the shard list and when the timeout is only 5 seconds the client gets kicked off too fast. It will be changed
	to 30sec after the client got authed.
	I put this here and not in tUnetServerBase as tUnetBase must be able to override it */
	
	msg_timeout=1; // 1 second (default time till re-transmission)

	//initial server timestamp
	updateNetTime();

	max=0; // Maxium number of connections (default 0, unlimited)
	smgr=NULL;

	lan_addr=htonl(0xAC1A0000);
	lan_mask=htonl(0xFFFFFF00); // LAN mask, in network byte order (default 255.255.255.0)
	// Bandwidth speed (lo interface -> maxium), all in Bit/s
	lan_up=100 * 1000 * 1000;
	lan_down=100 * 1000 * 1000;
	nat_up=500 * 1000;
	nat_down=500 * 1000;
	
	flood_check_interval=10;
	max_flood_pkts=1000; // when first launching a client for an emtpy vault, there are about 100 packets within about 2.5 seconds
	                    // linking to Noloben is a good testcase (500 per 10 seconds was not enough)

	receiveAhead=256; // Receive up to 256 not-yet acceptable packets "in the future" (one complete message, in worst-case)

	//logs
	log=new tLog;
	err=new tLog;
	ack=new tLog;
	sec=new tLog;

	#ifdef ENABLE_NETDEBUG
	// Noise and latency
	in_noise=2; // percent of dropped packets (0-100)
	out_noise=2; // percent of dropped packets (0-100)
	latency=0.05; // in secs
	// Bandwith: 5kB/10ms = 500kB/second = 4MBit/second
	lim_down_cap=5*1000; // in bytes
	lim_up_cap=5*1000; // in bytes
	quota_check_interval=0.01; // 10ms
	// the rest are controlling vairables, do not touch!
	cur_down_quota=0;
	cur_up_quota=0;
	last_quota_check = net_time;
	#endif
}
tUnet::~tUnet() {
	DBG(9,"~tUnet()\n");
	if (smgr) stopOp();
	if (log != alcGetMain()->std()) delete log;
	if (err != alcGetMain()->err()) delete err;
	delete ack;
	delete sec;
	
	
	if (pthread_cond_destroy(&eventAddedCond))
		throw txBase(_WHERE("Error destroying condition"));
}

void tUnet::updateNetTime() {
	tSpinLock lock(net_time_mutex);
	//set stamp
	net_time=alcGetCurrentTime();
}

tNetTime tUnet::remainingTimeTill(tNetTime time)
{
	tSpinLock lock(net_time_mutex);
	assert(net_time <= time); // make sure time is in the future
	return time-net_time;
}

tNetTime tUnet::passedTimeSince(tNetTime time)
{
	tSpinLock lock(net_time_mutex);
	assert(net_time >= time); // make sure time is in the past
	return net_time-time;
}

tNetEvent * tUnet::getEvent() {
	tMutexLock lock(eventsMutex);
	if (events.empty()) return NULL;
	return events.pop_front();
}

void tUnet::addEvent(tNetEvent *evt)
{
	DBG(5, "Enqueuing event %d for %s\n", evt->id, *evt->u ? evt->u->str().c_str() : "<>");
	tMutexLock lock(eventsMutex);
	events.push_back(evt);
	if (workerWaiting) {
		if (pthread_cond_signal(&eventAddedCond))
			throw txBase(_WHERE("Error signalling condition"));
	}
}

void tUnet::clearEventQueue()
{
	tMutexLock lock(eventsMutex);
	events.clear();
}

void tUnet::openLogfiles() {
	//open unet log files
	bool elog = this->flags & UNET_ELOG;
	
	if (log != alcGetMain()->std()) delete log;
	if (elog && !(this->flags & UNET_DSTDLOG)) {
		log = alcGetMain()->std();
	} else {
		log = new tLog;
	}
	
	if (err != alcGetMain()->err()) delete err;
	if (elog && !(this->flags & UNET_DERRLOG)) {
		err = alcGetMain()->err();
	} else {
		err = new tLog;
	}

	if(elog && (this->flags & UNET_EACKLOG)) {
		this->ack->open("ack.html",DF_HTML);
	} else {
		this->ack->open(DF_HTML); // still set correctly flag, so it does not get printed on stdout
	}
	

	if(elog && !(this->flags & UNET_DSECLOG)) {
		this->sec->open("access.log");
	} else {
		this->sec->close();
	}
}

void tUnet::startOp() {
	assert(smgr == NULL);
	//create an udp (17) socket
	DBG(1, "DBG: Linux sockets...\n");
	this->sock=socket(AF_INET,SOCK_DGRAM,0);

	if(this->sock<0)
	{
		throw txUnetWithErrno("ERR: Fatal - Failed Creating socket ");
	}
	DBG(1, "Socket created\n");


	//set network specific options
	this->server.sin_family=AF_INET; //UDP IP

	if(bindaddr == "0.0.0.0") { //gethostbyname already does that, but just in case
		this->server.sin_addr.s_addr=htonl(INADDR_ANY); //any address
	} else {
		struct hostent *host;
		host=gethostbyname(bindaddr.c_str());
		if(host==NULL) {
			this->err->log("ERR: Fatal cannot resolve address %s:%i\n",bindaddr.c_str(),bindport);
			throw txUnetIniErr(_WHERE("Cannot resolve address %s:%i",bindaddr.c_str(),bindport));
		}
		this->server.sin_addr.s_addr=*reinterpret_cast<in_addr_t *>(host->h_addr_list[0]);
	}
	this->server.sin_port=htons(bindport); //port 5000 default

	// binding to port - some servers can try different ports, too (used by game)
	bool error;
	do {
		error = bind(this->sock,reinterpret_cast<struct sockaddr *>(&this->server),sizeof(this->server))<0;
		if (error) {
			DBG(3, "Probing alternative port %d\n", bindport+15);
			if (canPortBeUsed(bindport+15)) { // we got an error, BUT me may try again on another port
				bindport += 15;
				this->server.sin_port=htons(bindport);
			}
			else
				break; // we can't try another port
		}
	} while (error);
	setCloseOnExec(sock); // close socket when forking a game server
	if (error) {
		this->err->log("ERR: Fatal - Failed binding to address %s:%i\n",bindaddr.c_str(),bindport);
		throw txUnetWithErrno(_WHERE("Cannot bind to address %s:%i",bindaddr.c_str(),bindport));
	}
	// 10 February 2004 - Alcugs development starts from scratch.
	// 10 February 2005 - Alcugs development continues..
	// The next line of code was originally written in 10/Feb/2004,
	// when the first listenning udp server named urud (uru daemon)
	// was compiled on that day.
	this->log->log("INF: Listening to incoming datagrams on %s port udp %i\n",bindaddr.c_str(),bindport);

	smgr=new tNetSessionMgr(this,this->max);

	if(this->max!=0) {
		this->log->log("INF: Accepting up to %i connections\n",this->max);
	} else {
		this->log->log("INF: Accepting unlimited connections\n",this->max);
	}
	
	// create the pipe
	int pipeEnds[2];
	if (pipe(pipeEnds)) throw txUnetIniErr(_WHERE("Failed to create pipe"));
	sndPipeReadEnd = pipeEnds[0];
	sndPipeWriteEnd = pipeEnds[1];
	
	// done!
	this->log->flush();
}

void tUnet::wakeUpMainThread(void )
{
	// we are in the worker thread... send a byte to the pipe so that the main thread wakes up and conciders this new packet
	uint8_t data = 0;
	if (write(sndPipeWriteEnd, &data, 1) != 1)
		throw txUnet(_WHERE("Failed to write the pipe?"));
}

/**
	Stops the network operation
*/
void tUnet::stopOp() {
	for (tNetSessionMgr::tIterator it(smgr); it.next();)
		err->log("ERR: %s is still connected, but we are going down ultimatively\n", it->str().c_str());
	// we could be called because of an exception, so there might still be tons of dirt around - don't assert a clean state!
	delete smgr;
	smgr = NULL;

	close(this->sock);
	DBG(1, "Socket closed\n");
	close(sndPipeReadEnd);
	close(sndPipeWriteEnd);
}

tNetSessionRef tUnet::netConnect(const char * hostname,uint16_t port,uint8_t validation) {
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	struct hostent *host=gethostbyname(hostname);
	if(host==NULL) throw txBase(_WHERE("Cannot resolve host: %s",hostname));
	uint32_t ip = *reinterpret_cast<uint32_t *>(host->h_addr_list[0]);
	
	tWriteLock lock(smgrMutex);
	return smgr->searchAndCreate(ip, htons(port), /*client*/false, validation);
}

void tUnet::removeConnection(tNetSession *u)
{
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	sec->log("%s Ended\n",u->str().c_str());
	tWriteLock lock(smgrMutex);
	smgr->destroy(u);
}

/** Urunet the netcore, it's the heart of the server 
This function recieves new packets and passes them to the correct session */
bool tUnet::sendAndWait() {
	// send old messages and calulate timeout
	tNetTimeBoolPair result = processSendQueues();
	tNetTime unet_timeout = result.first;
	bool idle = !result.second; // we are idle if there is nothing to send
	if (unet_timeout < 0.0005) {
		DBG(3, "Timeout %f too low, increasing to 500 microseconds\n", unet_timeout);
		unet_timeout = 0.0005; // don't sleep less than 0.5 milliseconds
	}

	//waiting for packets - timeout
	fd_set rfds;
	struct timeval tv;
	int valret;

	/* Check socket for new messages */
	FD_ZERO(&rfds);
	FD_SET(this->sock, &rfds);
	FD_SET(this->sndPipeReadEnd, &rfds);
	/* Set timeout */
	tv.tv_sec = unet_timeout; // round down to seconds
	tv.tv_usec = (unet_timeout-tv.tv_sec) * (1000*1000); // get the microseconds part

#if _DBG_LEVEL_ >= 2
	log->log("Waiting for %f seconds...\n", unet_timeout);
#endif
#if _DBG_LEVEL_ >= 8
	tTime start = tTime::now();
#endif
	valret = select(std::max(this->sock, this->sndPipeReadEnd)+1, &rfds, NULL, NULL, &tv); // this is the command taking the time - now lets process what we got
	// update stamp, since we spent some time in the select function
	updateNetTime();
#if _DBG_LEVEL_ >= 8
	start = tTime::now()-start;
	DBG(8,"waited %u.%06u\n",diff.seconds,diff.microseconds);
#endif
	/* Don't trust tv value after the call */
	
	//BEGIN ** CRITICIAL REGION STARTS HERE **

	if(valret<0) {
		if (errno == EINTR) {
			// simply go around again, a signal was probably delivered
			return idle;
		}
		throw txUnetWithErrno(_WHERE("ERR in select() "));
	}
	
	// empty the pipe (only in rare occasions where sends happen REALLY fast, there will be several bytes here)
	if (FD_ISSET(this->sndPipeReadEnd, &rfds)) {
		uint8_t data;
		if (read(this->sndPipeReadEnd, &data, 1) != 1)
			throw txUnetWithErrno(_WHERE("Error reading from the pipe"));
	}

	// check if we can read a packet
	if (!FD_ISSET(this->sock, &rfds)) return idle; // nothing to do

	// receive the message we got
	ssize_t n;
	uint8_t buf[INC_BUF_SIZE]; //internal rcv buffer

	struct sockaddr_in client; //client struct
	socklen_t client_len=sizeof(struct sockaddr_in); //get client struct size (for recvfrom)
	n = recvfrom(this->sock,buf,INC_BUF_SIZE,0,reinterpret_cast<struct sockaddr *>(&client),&client_len);
	
	if (n <= 0) { //problems or no message? select promised us data!
		throw txUnetWithErrno(_WHERE("ERR: Fatal recieving a message... "));
	}
	
#ifdef ENABLE_NETDEBUG
	if(!in_noise || (random() % 100) >= in_noise) {
		DBG(8,"Incomming Packet accepted\n");
	} else {
		return idle; // nothing to do
	}
	//check quotas
	if(passedTimeSince(last_quota_check) > quota_check_interval) {
		cur_up_quota=0;
		cur_down_quota=0;
		last_quota_check = getNetTime();
	}
	if(lim_down_cap) {
		if((cur_down_quota+n)>lim_down_cap) {
			DBG(5,"Paquet dropped by quotas, in use:%i,req:%li, max:%i\n",cur_down_quota,n,lim_down_cap);
			log->log("Incomming paquet dropped by quotas, in use:%i,req:%i, max:%i\n",cur_down_quota,n,lim_down_cap);
			n=0;
		} else {
			cur_down_quota+=n;
		}
	}
#endif

	/* Uru protocol check */
	if(n<=20 || buf[0]!=0x03) { //not an Uru protocol packet don't waste an slot for it
		this->err->log("[ip:%s:%i] ERR: Unexpected Non-Uru protocol packet found\n",alcGetStrIp(client.sin_addr.s_addr).c_str(),ntohs(client.sin_port));
		this->err->dumpbuf(buf,n);
		this->err->nl();
		return idle; // nothing to do
	}

	tNetSessionRef session;
	try {
		tWriteLock lock(smgrMutex);
		session=smgr->searchAndCreate(client.sin_addr.s_addr, client.sin_port);
	} catch(txToMCons) {
		return idle; // nothing to do
	}
	
	//process the message, and do the correct things with it
	try {
		// the net time might be a bit too low, but that's not a large problem - packets might just be sent a bit too early
		session->processIncomingMsg(buf,n);
	} catch(txBase &t) {
		err->log("%s Recieved invalid Uru message - kicking peer\n", session->str().c_str());
		err->log(" Exception %s\n%s\n",t.what(),t.backtrace());
		sec->log("%s Kicked hard due to error in Uru message\n", session->str().c_str());
		removeConnection(*session); // no goodbye message or anything, this error was deep on the protocol stack
	}
	
	//END CRITICAL REGION
	
	return idle; // actually, we might no longer be idle - but we *were* idle before we got the nw message, that's good enough (after all, we want the idle signal to be triggered from time to time...)
}

/** give each session the possibility to do some stuff and to set the timeout
Each session HAS to set the timeout it needs because it is reset prior to asking them. */
tNetTimeBoolPair tUnet::processSendQueues() {
	// reset the timer
	tNetTime unet_timeout=max_sleep;
	bool anythingToSend = false;
	std::list<tNetSession *> sessionsToDelete;
	
	// do the main work, with only a read lock
	{
		tReadLock lock(smgrMutex);
		for (tNetSessionMgr::tIterator it(smgr); it.next();) {
			updateNetTime(); // let the session calculate its timestamps correctly
			tNetTimeBoolPair result = it->processSendQueues();
			unet_timeout = std::min(result.first, unet_timeout);
			if (result.second)
				sessionsToDelete.push_back(*it); // can't delete now: we are iterating, and we hold the read lock
			else
				anythingToSend = anythingToSend || it->anythingToSend();
		}
	}
	
	// now check for the session to be deleted, with a write lock
	for (std::list<tNetSession *>::iterator it = sessionsToDelete.begin(); it != sessionsToDelete.end(); ++it)
		removeConnection(*it);
	
	// return what we found
	return tNetTimeBoolPair(unet_timeout, anythingToSend);
}


} //end namespace
