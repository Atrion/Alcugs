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

/* CVS tag - DON'T TOUCH*/
#define __U_UNET_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "unet.h"

#include "netsessionmgr.h"
#include "netexception.h"
#include <alcmain.h>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <cerrno>
#include <unistd.h>
#include <cstring>
#include <netdb.h>

// Some of the macros implie an old-style cast
#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace alc {

tUnet::tUnet(uint8_t whoami,const tString & lhost,uint16_t lport) : whoami(whoami) {
	DBG(9,"tUnet()\n");
	initialized=false;
	this->init();
	setBindAddress(lhost);
	bindport=lport;
	events=new tUnetMsgQ<tNetEvent>;
}
tUnet::~tUnet() {
	DBG(9,"~tUnet()\n");
	stopOp();
	delete events;
	if (log != alcGetMain()->std()) delete log;
	if (err != alcGetMain()->err()) delete err;
	delete ack;
	delete sec;
}
void tUnet::setBindPort(uint16_t lport) {
	bindport=lport;
}
void tUnet::setBindAddress(const tString & lhost) {
	if(lhost.isEmpty()) bindaddr = "0.0.0.0";
	else bindaddr = lhost;
}
/**
	Fills the unet struct with the default values
*/
void tUnet::init() {
	DBG(9,"tUnet::init()\n");
	if(initialized) return;
	flags=UNET_DEFAULT_FLAGS;

	//netcore timeout < min(all RTT's), nope, it must be the min tts (stt)
	max_sleep=5*1000*1000; // wait no more than 5 seconds in select() call

	conn_timeout=5*60; // default timeout for new sessions (seconds)
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
	//! Bandwidth speed (lo interface -> maxium)
	lan_up=100 * 1000 * 1000;
	lan_down=100 * 1000 * 1000;
	nat_up=128 * 1000;
	nat_down=512 * 1000;
	
	flood_check_sec=10;
	max_flood_pkts=600; // when first launching a client for an emtpy vault, there are about 100 packets within about 2.5 seconds
	                    // linking to Noloben is a good testcase (500 per 10 seconds was not enough)

	receiveAhead=50; // Receive up to 50 not-yet acceptable packets "in the future"

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
	// Bandwith: 10kB/0,66seconds = 15kB/second = 120kBit/second
	lim_down_cap=10000; // in bytes
	lim_up_cap=10000; // in bytes
	quota_check_interval=666666; // 2/3 of a second
	// the rest are controlling vairables, do not touch!
	cur_down_quota=0;
	cur_up_quota=0;
	last_quota_check = ntime;
	#endif
	
}
void tUnet::setFlags(uint16_t flags) {
	this->flags |= flags;
}
void tUnet::unsetFlags(uint16_t flags) {
	this->flags &= ~flags;
}
uint16_t tUnet::getFlags() { return this->flags; }

void tUnet::updateNetTime() {
	//set stamp
	ntime.setToNow();
	net_time=(((ntime.seconds % 1000)*1000000)+ntime.microseconds);
}


tNetEvent * tUnet::getEvent() {
	tMutexLock lock(eventMutex);
	events->rewind();
	if(events->getNext()==NULL) return NULL;
	return events->unstackCurrent();
}

void tUnet::addEvent(tNetEvent *evt)
{
	tMutexLock lock(eventMutex);
	events->add(evt);
}

tNetSession * tUnet::getSession(tNetSessionIte &t) {
	return(smgr->search(t,false));
}

void tUnet::destroySession(tNetSessionIte &t) {
	smgr->destroy(t);
}

void tUnet::neterror(const char * msg) {
	if(!initialized) return;
#ifdef __WIN32__
	this->err->log("%s: winsock error code:%i\n",msg,WSAGetLastError());
#else
	this->err->logErr(msg);
#endif
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
		this->ack->open(DF_HTML);
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
#ifdef __WIN32__
	memset(&this->ws,0,sizeof(this->ws));
	log->log("INF: Hasecorp Windoze sockets XP...\n");
	if(WSAStartup(MAKEWORD(1,1),&this->ws)!=0) {
		neterror("ERR: Cannot start up winsock ");
		throw txUnetIniErr(_WHERE("cannot start winsock"));
	}
	this->log->print(" Winsock Version %d, %d\n",this->ws.wHighVersion,this->ws.wVersion);
	this->log->print(" %s - %s\n",this->ws.szDescription,this->ws.szSystemStatus);
	this->log->print(" maxsockets:%i, maxUdpDg:%i\n",this->ws.iMaxSockets,\
	this->ws.iMaxUdpDg);
	this->log->print(" vendor: %s\n",this->ws.lpVendorInfo);
#else
	DBG(1, "DBG: Linux sockets...\n");
#endif
	this->sock=socket(AF_INET,SOCK_DGRAM,0);

#ifdef __WIN32__
	if(this->sock==INVALID_SOCKET)
#else
	if(this->sock<0)
#endif
	{
		neterror("ERR: Fatal - Failed Creating socket ");
		throw txUnetIniErr(_WHERE("cannot create socket"));
	}
	DBG(1, "Socket created\n");

	if(this->flags & UNET_NBLOCK) {

#ifdef __WIN32__
		//set non-blocking
		this->nNoBlock = 1;
		if(ioctlsocket(this->sock, FIONBIO, &this->nNoBlock)!=0) {
			neterror("ERR: Fatal setting socket as non-blocking\n");
			throw txUnetIniErr(_WHERE("Failed setting a non-blocking socket"));
		}
#else
		//set non-blocking
		long arg;
		if((arg = fcntl(this->sock,F_GETFL, NULL))<0) {
			this->err->logErr("ERR: Fatal setting socket as non-blocking (fnctl F_GETFL)\n");
			throw txUnetIniErr(_WHERE("Failed setting a non-blocking socket"));
		}
		arg |= O_NONBLOCK;

		if(fcntl(this->sock, F_SETFL, arg)<0) {
			this->err->log("ERR: Fatal setting socket as non-blocking\n");
			throw txUnetIniErr(_WHERE("Failed setting a non-blocking socket"));
		}
#endif
		DBG(1, "Non-blocking socket set\n");
	} else {
		DBG(1, "blocking socket set\n");
	}

	//broadcast ?
	if(this->flags & UNET_BCAST) {
		this->opt = 1;
		#ifdef __WIN32__
		if(setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, (const char *)&this->opt, sizeof(int))!=0)
		#else
		if(setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, &this->opt, sizeof(int))!=0)
		#endif
		{
			neterror("ERR: Fatal - Failed setting BCAST socket ");
			throw txUnetIniErr(_WHERE("Failed setting a BCAST socket"));
		}
	}
	
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
		neterror("bind() ");
		throw txUnetIniErr(_WHERE("Cannot bind to address %s:%i",bindaddr.c_str(),bindport));
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
	this->log->flush();
	initialized=true;
}

/**
	Stops the network operation
*/
void tUnet::stopOp() {
	if(!initialized) return;
	delete smgr;
	smgr = NULL;

#ifdef __WIN32__
	closesocket(this->sock);
	if(WSACleanup()!=0) {
		neterror("WSACleanup() ");
	}
#else
	close(this->sock);
#endif
	DBG(1, "Socket closed\n");
	initialized=false;
}

tNetSessionIte tUnet::netConnect(const char * hostname,uint16_t port,uint8_t validation,uint8_t flags,uint8_t peerType) {
	tNetSessionIte ite;
	
	struct sockaddr_in client;
	struct hostent *host;
	host=gethostbyname(hostname);
	
	if(host==NULL) throw txBase(_WHERE("Cannot resolve host: %s",hostname));
	
	ite.ip=*reinterpret_cast<uint32_t *>(host->h_addr_list[0]);
	ite.port=htons(port);
	ite.sid=-1;
	
	tNetSession * u=smgr->search(ite, true);
	
	u->validation=validation;
	u->cflags |= flags;
	
	if(validation>=3) {
		u->validation=0;
		u->cflags |= UNetUpgraded;
	}
	
	client.sin_family=AF_INET;
	client.sin_addr.s_addr=ite.ip;
	client.sin_port=ite.port;
	memcpy(u->sockaddr,&client,sizeof(struct sockaddr_in));
	
	u->timestamp.seconds=alcGetTime();
	u->timestamp.microseconds=alcGetMicroseconds();
	u->client=false;
	if (peerType) u->whoami = peerType;
	
	u->max_version=max_version;
	u->min_version=min_version;
	
	u->nego_stamp=u->timestamp;
	u->negotiate();
	
	return ite;
}

/** Urunet the netcore, it's the heart of the server 
This function recieves new packets and passes them to the correct session, and
it also is responsible for the timer: It will wait exactly unet_sec seconds and unet_usec microseconds
before it asks each session to do what it has to do (tUnet::doWork) */
int tUnet::sendAndWait() {
	// send old messages and calulate timeout
	unsigned int unet_timeout = processSendQueues();
	if (unet_timeout < 500) unet_timeout = 500; // don't sleep less than 0.5 milliseconds
	
	ssize_t n;
	uint8_t buf[INC_BUF_SIZE]; //internal rcv buffer
	
	tNetSessionIte ite;
	tNetSession * session=NULL;

	struct sockaddr_in client; //client struct
	socklen_t client_len=sizeof(struct sockaddr_in); //get client struct size (for recvfrom)

	//waiting for packets - timeout
	fd_set rfds;
	struct timeval tv;
	int valret;

	/* Check socket for new messages */
	FD_ZERO(&rfds);
	FD_SET(this->sock, &rfds);
	/* Set timeout */
	tv.tv_sec = unet_timeout / (1000*1000);
	tv.tv_usec = unet_timeout % (1000*1000);

#if _DBG_LEVEL_ >= 8
	DBG(8,"waiting for incoming messages (%u)...\n", unet_timeout);
	tTime start; start.now();
#endif
	valret = select(this->sock+1, &rfds, NULL, NULL, &tv); // this is the command taking the time - now lets process what we got
	// update stamp, since we spent some time in the select function, and doWork() [see below] needs the current time
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
			return UNET_OK;
		}
		neterror("ERR in select() ");
		return UNET_ERR;
	}

#if _DBG_LEVEL_>7
	if (valret) { DBG(9,"Data recieved...\n"); } 
	else { DBG(9,"No data recieved...\n"); }
#endif

	DBG(9,"Before recvfrom\n");
#ifdef __WIN32__
	n = recvfrom(this->sock,reinterpret_cast<char *>(buf),INC_BUF_SIZE,0,reinterpret_cast<struct sockaddr *>(&client),&client_len);
#else
	n = recvfrom(this->sock,buf,INC_BUF_SIZE,0,reinterpret_cast<struct sockaddr *>(&client),&client_len);
#endif
	DBG(9,"After recvfrom\n");
	
#ifdef ENABLE_NETDEBUG
	if(n>0) {
		if(!in_noise || (random() % 100) >= in_noise) {
			DBG(8,"Incomming Packet accepted\n");
		} else {
			DBG(5,"Incomming Packet dropped\n");
			n=0;
		}
		//check quotas
		if((ntime-last_quota_check).asNumber('u') > quota_check_interval) {
			cur_up_quota=0;
			cur_down_quota=0;
			last_quota_check = ntime;
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

	if(n<0 && valret) { //problems?
		neterror("ERR: Fatal recieving a message... ");
		return UNET_ERR;
	}

	if(n>0) { //we have a message
		if(n>OUT_BUFFER_SIZE) {
			this->err->log("[ip:%s:%i] ERR: Recieved a really big message of %i bytes\n",alcGetStrIp(client.sin_addr.s_addr).c_str(),ntohs(client.sin_port),n);
			return UNET_TOOBIG;
		} //catch up impossible big messages
		/* Uru protocol check */
		if(n<=20 || buf[0]!=0x03) { //not an Uru protocol packet don't waste an slot for it
			this->err->log("[ip:%s:%i] ERR: Unexpected Non-Uru protocol packet found\n",alcGetStrIp(client.sin_addr.s_addr).c_str(),ntohs(client.sin_port));
			this->err->dumpbuf(buf,n);
			this->err->nl();
			return UNET_NONURU;
		}

		DBG(8,"Search session...\n");
		ite.ip=client.sin_addr.s_addr;
		ite.port=client.sin_port;
		ite.sid=-1;
		
		try {
			session=smgr->search(ite, true);
		} catch(txToMCons) {
			return UNET_TOMCONS;
		}
		
		//process the message, and do the correct things with it
		memcpy(session->sockaddr,&client,sizeof(struct sockaddr_in));
		try {
			session->processIncomingMsg(buf,n);
		} catch(txProtocolError &t) {
			this->err->log("%s Protocol Error %s\nBacktrace:%s\n",session->str().c_str(),t.what(),t.backtrace());
			return UNET_ERR;
		} catch(txBase &t) {
			this->err->log("%s FATAL ERROR (perhaps someone is attempting something nasty, or you have found a bug)\n\
%s\nBacktrace:%s\n",session->str().c_str(),t.what(),t.backtrace());
			return UNET_ERR;
		}

	}
	
	//END CRITICAL REGION
	
	return UNET_OK;
}

/** give each session the possibility to do some stuff and to set the timeout
Each session HAS to set the timeout it needs because it is reset prior to asking them. */
unsigned int tUnet::processSendQueues() {
	// reset the timer
	unsigned int unet_timeout=max_sleep;
	
	tNetSession * cur;
	smgr->rewind();
	while((cur=smgr->getNext())) {
		/* Also create the timeout when it's exactly the same time.
		  This way the time from a session being marked as deleteable till it is deleted is kept short */
		if(static_cast<unsigned int>(ntime.seconds) >= cur->conn_timeout + cur->timestamp.seconds) {
			//timeout event
			if (!cur->isTerminated())
				sec->log("%s Timeout (didn't send a packet for %d seconds)\n",cur->str().c_str(),cur->conn_timeout);
			tNetSessionIte ite(cur->ip,cur->port,cur->sid);
			tNetEvent * evt=new tNetEvent(ite,UNET_TIMEOUT);
			addEvent(evt);
		} else {
			unet_timeout = std::min(cur->processSendQueues(), unet_timeout);
		}
	}
	return unet_timeout;
}

/** sends the message (internal use only)
An uru message can only be 253952 bytes in V0x01 & V0x02 and 254976 in V0x00 */
void tUnet::rawsend(tNetSession * u,tUnetUruMsg * msg) {
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
	
	DBG(5,"validation level is %i,%i\n",u->validation,msg->val);
	
	size_t msize=mbuf->size();
	uint8_t * buf, * buf2=NULL;
	buf=const_cast<uint8_t *>(mbuf->data()); // yes, we are writing directly into the tMBuf buffer... this saves us from copying everything

	if(msg->val==2) {
		DBG(8,"Encoding validation 2 packet of %li bytes...\n",msize);
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
	if((ntime-last_quota_check).asNumber('u') > quota_check_interval) {
		cur_up_quota=0;
		cur_down_quota=0;
		last_quota_check = ntime;
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
#ifdef __WIN32__
		msize = sendto(sock,reinterpret_cast<const char *>(buf),msize,0,reinterpret_cast<struct sockaddr *>(&client),sizeof(struct sockaddr));
#else
		msize = sendto(sock,reinterpret_cast<char *>(buf),msize,0,reinterpret_cast<struct sockaddr *>(&client),sizeof(struct sockaddr));
#endif
	}

	DBG(9,"After the Sendto call...\n");
	free(buf2);
	delete mbuf;
	DBG(8,"returning from uru_net_send RET:%li\n",msize);
}

void tUnet::send(tmMsgBase &m, unsigned int delay)
	{ m.getSession()->send(m, delay); }

/**
	Dumps all data structures, with the address and the contents in hex
	flags
	0x01 append to the last log file (elsewhere destroy the last one)
*/
void tUnet::dump(tLog * sf,uint8_t flags) {
#if _DBG_LEVEL_ > 2
	tLog * f;
	if(sf==NULL) {
		f=new tLog;
		if(flags & 0x01) {
			f->open("memdump.log",5,DF_APPEND);
		} else {
			f->open("memdump.log",5,0);
		}
		if(f==NULL) return;
	} else {
		f=sf;
	}

	//well dump all
	f->log("Starting up Netcore memory report\n\n");

	f->print("Unet at 0x%08X\n",(int)this);
	f->dumpbuf((Byte *)this,sizeof(*this));
	f->nl();
	#ifndef __WIN32__
	f->print("net->sock:%i\n",this->sock);
	#endif
	f->print("net->server.sin_family:%02X\n",this->server.sin_family);
	f->print("net->server.sin_port:%02X (%i)\n",this->server.sin_port,ntohs(this->server.sin_port));
	f->print("net->server.sin_addr:%s\n",alcGetStrIp(this->server.sin_addr.s_addr));
	f->print("net->flags:%i\n",this->flags);
	f->print("net->unet_sec:%i\n",this->unet_sec);
	f->print("net->unet_usec:%i\n",this->unet_usec);
	f->print("net->max_version:%i\n",this->max_version);
	f->print("net->min_version:%i\n",this->min_version);
	f->print("net->timestamp:%s\n",this->ntime.str());
	f->print("net->conn_timeout:%i\n",this->conn_timeout);
	f->print("net->timeout:%i\n",this->timeout);
	f->print("net->max:%i\n",this->max);
	f->print("net->whoami:%i\n",this->whoami);
	f->print("net->lan_addr:%i %s\n",this->lan_addr,alcGetStrIp(this->lan_addr));
	f->print("net->lan_mask:%i %s\n",this->lan_mask,alcGetStrIp(this->lan_mask));
	f->print("net->lan_up:%i\n",this->lan_up);
	f->print("net->lan_down:%i\n",this->lan_down);
	f->print("net->nat_up:%i\n",this->nat_up);
	f->print("net->nat_down:%i\n",this->nat_down);
	f->print("net->nat_up:%i\n",this->nat_up);
	f->print("net->nat_down:%i\n",this->nat_down);
	f->print("Session table\n");
	//session table
	
	f->log("Ending memory report\n");
	if(sf==NULL) {
		f->close();
		delete f;
	}
#endif
	(void)sf; (void)flags; // mark as used
}


} //end namespace
