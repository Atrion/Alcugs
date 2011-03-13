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
//#define _DBG_LEVEL_ 5
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

tUnet::tUnet(uint8_t whoami,const tString & lhost,uint16_t lport) : whoami(whoami) {
	DBG(9,"tUnet()\n");
	initialized=false;
	this->init();
	setBindAddress(lhost);
	bindport=lport;
	
	workerWaiting = false;
	if (pthread_cond_init(&eventAddedCond, NULL))
		throw txBase(_WHERE("Error initializing condition"));
}
tUnet::~tUnet() {
	DBG(9,"~tUnet()\n");
	stopOp();
	if (log != alcGetMain()->std()) delete log;
	if (err != alcGetMain()->err()) delete err;
	delete ack;
	delete sec;
	
	
	if (pthread_cond_destroy(&eventAddedCond))
		throw txBase(_WHERE("Error destroying condition"));
}
/**
	Fills the unet struct with the default values
*/
void tUnet::init() {
	DBG(9,"tUnet::init()\n");
	if(initialized) return;
	flags=UNET_DEFAULT_FLAGS;

	//netcore timeout < min(all RTT's), nope, it must be the min tts (stt)
	max_sleep=10*1000*1000; // wait no more than 10 seconds in select() call

	conn_timeout=60; // default timeout for new sessions (seconds)
	/* This sets the timeout for unet servers from both sides
	It also sets the timeout for Uru clients, where a high timeout is necessary since the connection is already established when changing
	the server in the shard list and when the timeout is only 5 seconds the client gets kicked off too fast. It will be changed
	to 30sec after the client got authed.
	I put this here and not in tUnetServerBase as tUnetBase must be able to override it */
	
	msg_timeout=1000*1000; // 1 second (default time till re-transmission)

	//initial server timestamp
	updateNetTime();
	
	max_version=12;
	min_version=7;

	max=0; // Maxium number of connections (default 0, unlimited)
	smgr=NULL;

	lan_addr=htonl(0xAC1A0000);
	lan_mask=htonl(0xFFFFFF00); // LAN mask, in network byte order (default 255.255.255.0)
	// Bandwidth speed (lo interface -> maxium), all in Bit/s
	lan_up=100 * 1000 * 1000;
	lan_down=100 * 1000 * 1000;
	nat_up=500 * 1000;
	nat_down=500 * 1000;
	
	flood_check_interval=10*1000*1000;
	max_flood_pkts=600; // when first launching a client for an emtpy vault, there are about 100 packets within about 2.5 seconds
	                    // linking to Noloben is a good testcase (500 per 10 seconds was not enough)

	receiveAhead=256; // Receive up to 256 not-yet acceptable packets "in the future" (one complete message, in worst-case)

	//logs
	log=new tLog;
	err=new tLog;
	ack=new tLog;
	sec=new tLog;

	ip_overhead=20+8;

	#ifdef ENABLE_NETDEBUG
	// Noise and latency
	in_noise=2; // percent of dropped packets (0-100)
	out_noise=2; // percent of dropped packets (0-100)
	latency=20000; // in usecs
	// Bandwith: 5kB/10ms = 500kB/second = 4MBit/second
	lim_down_cap=5*1000; // in bytes
	lim_up_cap=5*1000; // in bytes
	quota_check_interval=10*1000; // 10ms
	// the rest are controlling vairables, do not touch!
	cur_down_quota=0;
	cur_up_quota=0;
	last_quota_check = net_time;
	#endif
	
}

void tUnet::updateNetTime() {
	//set stamp
	tTime t = tTime::now();
	net_time=static_cast<tNetTime>(t.seconds)*1000000+t.microseconds;
}

tNetTimeDiff tUnet::remainingTimeTill(tNetTime time)
{
	assert(time == net_time || !timeOverdue(time)); // make sure time is in the future
	return time-net_time;
}

tNetTimeDiff tUnet::passedTimeSince(tNetTime time)
{
	assert(timeOverdue(time));
	return net_time-time;
}

tNetEvent * tUnet::getEvent() {
	tMutexLock lock(eventsMutex);
	if (events.empty()) return NULL;
	return events.pop_front();
}

void tUnet::addEvent(tNetEvent *evt)
{
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
	if(initialized) return;
	//create an udp (17) socket
	DBG(1, "DBG: Linux sockets...\n");
	this->sock=socket(AF_INET,SOCK_DGRAM,0);

	if(this->sock<0)
	{
		throw txUnetWithErrno("ERR: Fatal - Failed Creating socket ");
	}
	DBG(1, "Socket created\n");

	//set non-blocking
	long arg;
	if((arg = fcntl(this->sock,F_GETFL, NULL))<0) {
		throw txUnetWithErrno(_WHERE("ERR: Fatal setting socket as non-blocking (fnctl F_GETFL)\n"));
	}
	arg |= O_NONBLOCK;

	if(fcntl(this->sock, F_SETFL, arg)<0) {
		this->err->log("ERR: Fatal setting socket as non-blocking\n");
		throw txUnetIniErr(_WHERE("Failed setting a non-blocking socket"));
	}
	DBG(1, "Non-blocking socket set\n");
	
	//chk?
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
	initialized=true;
}

/**
	Stops the network operation
*/
void tUnet::stopOp() {
	if(!initialized) return;
	// we could be called because of an exception, so there might still be tons of dirt around - don't assert a clean state!
	delete smgr;
	smgr = NULL;

	close(this->sock);
	DBG(1, "Socket closed\n");
	close(sndPipeReadEnd);
	close(sndPipeWriteEnd);
	initialized=false;
}

tNetSessionRef tUnet::netConnect(const char * hostname,uint16_t port,uint8_t validation,uint8_t flags,uint8_t peerType) {
	struct hostent *host;
	host=gethostbyname(hostname);
	
	if(host==NULL) throw txBase(_WHERE("Cannot resolve host: %s",hostname));
	
	uint32_t ip = *reinterpret_cast<uint32_t *>(host->h_addr_list[0]);
	
	tNetSessionRef u;
	{
		tMutexLock lock(smgrMutex);
		u=smgr->searchAndCreate(ip, htons(port));
	}
	
	u->validation=validation;
	u->cflags |= flags;
	
	if(validation>=3) {
		u->validation=0;
		u->cflags |= UNetUpgraded;
	}
	
	struct sockaddr_in client;
	client.sin_family=AF_INET;
	client.sin_addr.s_addr=u->ip;
	client.sin_port=u->port;
	memcpy(u->sockaddr,&client,sizeof(struct sockaddr_in));
	
	u->client=false;
	if (peerType) u->whoami = peerType;
	
	u->max_version=max_version;
	u->min_version=min_version;
	
	u->nego_stamp.setToNow();
	u->negotiate();
	
	return u;
}

/** Urunet the netcore, it's the heart of the server 
This function recieves new packets and passes them to the correct session, and
it also is responsible for the timer: It will wait exactly unet_sec seconds and unet_usec microseconds
before it asks each session to do what it has to do (tUnet::doWork) */
void tUnet::sendAndWait() {
	// send old messages and calulate timeout
	tNetTimeDiff unet_timeout = processSendQueues();
	if (unet_timeout < 250) {
		DBG(3, "Timeout %d too low, increasing to 250\n", unet_timeout);
		unet_timeout = 250; // don't sleep less than 0.25 milliseconds
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
	tv.tv_sec = unet_timeout / (1000*1000);
	tv.tv_usec = unet_timeout % (1000*1000);

#if _DBG_LEVEL_ >= 2
	log->log("Waiting for %d microseconds...\n", unet_timeout);
#endif
#if _DBG_LEVEL_ >= 8
	tTime start; start.now();
#endif
	valret = select(std::max(this->sock, this->sndPipeReadEnd)+1, &rfds, NULL, NULL, &tv); // this is the command taking the time - now lets process what we got
	// update stamp, since we spent some time in the select function
	updateNetTime();
#if _DBG_LEVEL_ >= 8
	start = ntime-start;
	DBG(8,"waited %u.%06u\n",diff.seconds,diff.microseconds);
#endif
	/* Don't trust tv value after the call */
	
	//BEGIN ** CRITICIAL REGION STARTS HERE **

	if(valret<0) {
		if (errno == EINTR) {
			// simply go around again, a signal was probably delivered
			return;
		}
		throw txUnetWithErrno(_WHERE("ERR in select() "));
	}
	
	// empty the pipe (only in rare occasions where sends happen REALLY fast, there will be several bytes here)
	if (FD_ISSET(this->sndPipeReadEnd, &rfds)) {
		uint8_t data;
		if (read(this->sndPipeReadEnd, &data, 1) != 1)
			throw txUnetWithErrno(_WHERE("Error reading from the pipe"));
	}

#if _DBG_LEVEL_>7
	if (valret) { DBG(9,"Data recieved...\n"); } 
	else { DBG(9,"No data recieved...\n"); }
#endif

	while(true) { // receive all messages we got
		ssize_t n;
		uint8_t buf[INC_BUF_SIZE]; //internal rcv buffer
		
		tNetSessionRef session;

		struct sockaddr_in client; //client struct
		socklen_t client_len=sizeof(struct sockaddr_in); //get client struct size (for recvfrom)
		
		n = recvfrom(this->sock,buf,INC_BUF_SIZE,0,reinterpret_cast<struct sockaddr *>(&client),&client_len);
		
	#ifdef ENABLE_NETDEBUG
		if(n>0) {
			if(!in_noise || (random() % 100) >= in_noise) {
				DBG(8,"Incomming Packet accepted\n");
			} else {
				DBG(5,"Incomming Packet dropped\n");
				n=0;
			}
			//check quotas
			if(passedTimeSince(last_quota_check) > quota_check_interval) {
				cur_up_quota=0;
				cur_down_quota=0;
				last_quota_check = net_time;
			}
			if(n>0 && lim_down_cap) {
				if((cur_down_quota+n+ip_overhead)>lim_down_cap) {
					DBG(5,"Paquet dropped by quotas, in use:%i,req:%li, max:%i\n",cur_down_quota,n+ip_overhead,lim_down_cap);
					log->log("Incomming paquet dropped by quotas, in use:%i,req:%i, max:%i\n",cur_down_quota,n+ip_overhead,lim_down_cap);
					n=0;
				} else {
					cur_down_quota+=n+ip_overhead;
				}
			}
		}
	#endif

		if (n<0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break; // no more messages left
		if (n<0) { //problems?
			throw txUnetWithErrno(_WHERE("ERR: Fatal recieving a message... "));
		}

		if(n>0) { //we have a message
			/* Uru protocol check */
			if(n<=20 || buf[0]!=0x03) { //not an Uru protocol packet don't waste an slot for it
				this->err->log("[ip:%s:%i] ERR: Unexpected Non-Uru protocol packet found\n",alcGetStrIp(client.sin_addr.s_addr).c_str(),ntohs(client.sin_port));
				this->err->dumpbuf(buf,n);
				this->err->nl();
				continue; // read next message
			}

			DBG(8,"Search session...\n");
			
			try {
				tMutexLock lock(smgrMutex);
				session=smgr->searchAndCreate(client.sin_addr.s_addr, client.sin_port);
			} catch(txToMCons) {
				continue; // read next message
			}
			
			//process the message, and do the correct things with it
			memcpy(session->sockaddr,&client,sizeof(struct sockaddr_in));
			try {
				// the net time might be a bit too low, but that's not a large problem - packets might just be sent a bit too early
				session->processIncomingMsg(buf,n);
			} catch(txBase &t) {
				err->log("%s Recieved invalid Uru message - kicking peer\n", session->str().c_str());
				err->log(" Exception %s\n%s\n",t.what(),t.backtrace());
				sec->log("%s Kicked hard due to error in Uru message\n", session->str().c_str());
				tMutexLock lock(smgrMutex);
				smgr->destroy(*session); // no goodbye message or anything, this error was deep on the protocol stack
			}
		}
	}
	
	//END CRITICAL REGION
	// I don't know how much perforcmance this costs, but without flushing it's not possible to follow the server logs using tail -F
	log->flush();
	err->flush();
	sec->flush();
}

/** give each session the possibility to do some stuff and to set the timeout
Each session HAS to set the timeout it needs because it is reset prior to asking them. */
tNetTimeDiff tUnet::processSendQueues() {
	// reset the timer
	tNetTimeDiff unet_timeout=max_sleep;
	
	tNetSession * cur;
	tMutexLock lock(smgrMutex); // FIXME this is not good, holding the lock too long
	smgr->rewind();
	while((cur=smgr->getNext())) {
		updateNetTime(); // let the session calculate its timestamps correctly
		unet_timeout = std::min(cur->processSendQueues(), unet_timeout);
	}
	return unet_timeout;
}

/** sends the message (internal use only)
An uru message can only be 253952 bytes in V0x01 & V0x02 and 254976 in V0x00
Call only with the sendMutex of that session locked! */
void tUnet::rawsend(tNetSession * u,tUnetUruMsg * msg)
{
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	struct sockaddr_in client; //client struct

	//copy the inet struct
	memcpy(&client,u->sockaddr,sizeof(struct sockaddr_in));
	client.sin_family=AF_INET; //UDP IP (????)
	client.sin_addr.s_addr=u->ip; //address
	client.sin_port=u->port; //port

	DBG(9,"Server pn is %08X\n",u->serverMsg.pn);
	DBG(9,"Server sn is %08X,%08X\n",u->serverMsg.sn,msg->sn);
	u->serverMsg.pn++;
	msg->pn=u->serverMsg.pn;
	
	#ifdef ENABLE_MSGDEBUG
	log->log("<SND> ");
	msg->dumpheader(log);
	log->nl();
	#endif
	msg->htmlDumpHeader(ack,1,u->ip,u->port);

	//store message into buffer
	tMBuf * mbuf;
	mbuf = new tMBuf();
	mbuf->put(*msg);

	#ifdef ENABLE_MSGDEBUG
	log->log("<SND> RAW Packet follows: \n");
	log->dumpbuf(*mbuf);
	log->nl();
	#endif
	
	DBG(9,"validation level is %i,%i\n",u->validation,msg->val);
	
	size_t msize=mbuf->size();
	uint8_t * buf, * buf2=NULL;
	buf=const_cast<uint8_t *>(mbuf->data()); // yes, we are writing directly into the tMBuf buffer... this saves us from copying everything

	if(msg->val==2) {
		DBG(8,"Encoding validation 2 packet of %Zi bytes...\n",msize);
		buf2=static_cast<uint8_t *>(malloc(msize));
		if(buf2==NULL) { throw txNoMem(_WHERE("")); }
		alcEncodePacket(buf2,buf,msize);
		buf=buf2; //don't need to decode again
		if(u->authenticated==1) {
			DBG(8,"Client is authenticated, doing checksum...\n");
			uint32_t val=alcUruChecksum(buf,msize,2,u->passwd.c_str());
			memcpy(buf+2,&val,4);
			DBG(8,"Checksum done!...\n");
		} else {
			DBG(8,"Client is not authenticated, doing checksum...\n");
			uint32_t val=alcUruChecksum(buf,msize,1,NULL);
			memcpy(buf+2,&val,4);
			DBG(8,"Checksum done!...\n");
		}
		buf[1]=0x02;
	} else if(msg->val==1) {
		uint32_t val=alcUruChecksum(buf,msize,0,NULL);
		memcpy(buf+2,&val,4);
		buf[1]=0x01;
	} else {
		buf[1]=0x00;
	}
	buf[0]=0x03; //magic number
	DBG(9,"Before the Sendto call...\n");
	//
#ifdef ENABLE_NETDEBUG
	if(!out_noise || (random() % 100) >= out_noise) {
		DBG(8,"Outcomming Packet accepted\n");
	} else {
		DBG(5,"Outcomming Packet dropped\n");
		msize=0;
	}
	//check quotas
	if(passedTimeSince(last_quota_check) > quota_check_interval) {
		cur_up_quota=0;
		cur_down_quota=0;
		last_quota_check = net_time;
	}
	if(msize>0 && lim_up_cap) {
		if((cur_up_quota+msize+ip_overhead)>lim_up_cap) {
			DBG(5,"Paquet dropped by quotas, in use:%i,req:%li, max:%i\n",cur_up_quota,msize+ip_overhead,lim_up_cap);
			log->log("Paquet dropped by quotas, in use:%i,req:%i, max:%i\n",cur_up_quota,msize+ip_overhead,lim_up_cap);
			msize=0;
		} else {
			cur_up_quota+=msize+ip_overhead;
		}
	}
#endif

	if(msize>0) {
		msize = sendto(sock,reinterpret_cast<char *>(buf),msize,0,reinterpret_cast<struct sockaddr *>(&client),sizeof(struct sockaddr));
	}

	DBG(9,"After the Sendto call...\n");
	free(buf2);
	delete mbuf;
	DBG(8,"returning from uru_net_send RET:%Zi\n",msize);
}


} //end namespace
