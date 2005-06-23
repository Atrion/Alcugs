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
	URUNET 3
*/

/* CVS tag - DON'T TOUCH*/
#define __U_URUNET_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "urunet/unet.h"

#include <unistd.h>
#include <fcntl.h>

#include "alcdebug.h"

namespace alc {

tUnet::tUnet(char * lhost,U16 lport) {
	DBG(9,"tUnet()\n");
	initialized=false;
	this->init();
	if(lhost==NULL) strcpy(bindaddr,"0.0.0.0");
	else strcpy(bindaddr,lhost);
	bindport=lport;
}
tUnet::~tUnet() {
	DBG(9,"~tUnet()\n");
	stopOp();
}
/**
	Fills the unet struct with the default values
*/
void tUnet::init() {
	DBG(9,"tUnet::init()\n");
	if(initialized) return;
	flags=UNET_NBLOCK | UNET_ELOG | UNET_ECRC | UNET_AUTOSP | UNET_NOFLOOD | UNET_FLOG | UNET_NETAUTH;
	whoami=0; //None (new connection) //KClient; //default type of peer

	//netcore timeout < min(all RTT's)
	unet_sec=1; //(seconds)
	unet_usec=0; //(microseconds)

	conn_timeout=5; //default timeout (seconds) (sensible to NetMsgSetTimeout (higher when connected)
	/* set to 30 when authed (client should send alive every 10 seconds)*/
	timeout=2000; //2 seconds (re-transmission) [initial RTT]

	//initial server timestamp
	ntime_sec=alcGetTime();
	ntime_usec=alcGetMicroseconds();
	
	max_version=12;
	min_version=7;

	max=0; //<! Maxium number of connections (default 0, unlimited)
	smgr=NULL;

	lan_addr=0x00001AAC;
	lan_mask=0x00FFFFFF; //<! LAN mask, in network byte order (default 255.255.255.0)
	//! Bandwidth speed (lo interface -> maxium)
	lan_up=100 * 1000 * 1000;
	lan_down=100 * 1000 * 1000;
	nat_up=128 * 1000;
	nat_down=512 * 1000;

	//peers
	auth=-1;
	vault=-1;
	tracking=-1;
	meta=-1;

	//logs
	log=NULL;
	err=NULL;
	unx=NULL;
	ack=NULL;
	chk=NULL;
	sec=NULL;
	
	#ifdef _UNET_DBG_
	lim_down_cap=4000; //in bytes
	lim_up_cap=4000; //in bytes
	in_noise=25; //(0-100)
	out_noise=25; //(0-100)
	latency=500; //(in msecs)
	cur_down_quota=0;
	cur_up_quota=0;
	ip_overhead=20+8;
	quota_check=1;
	time_quota_check=ntime_sec;
	#endif
	
}
void tUnet::setFlags(tUnetFlags flags) {
	this->flags |= flags;
}
void tUnet::unsetFlags(tUnetFlags flags) {
	this->flags &= ~flags;
}
tUnetFlags tUnet::getFlags() { return this->flags; }

void tUnet::neterror(char * msg) {
	if(!initialized) return;
#ifdef __WIN32__
	this->err->log("%s: winsock error code:%i\n",msg,WSAGetLastError());
#else
	this->err->logerr(msg);
#endif
}

void tUnet::_openlogs() {
	//open unet log files
	if(this->flags & UNET_ELOG) {
		if(this->log==NULL) {
			if(lstd==NULL) {
				this->log=new tLog;
				this->log->open(NULL,3,DF_STDOUT);
				lstd=this->log;
			} else {
				this->log=lstd;
			}
		}
		if(this->err==NULL) {
			if(lerr==NULL) {
				this->err=new tLog;
				this->err->open(NULL,2,DF_STDERR);
				lerr=this->err;
			} else {
				this->err=lerr;
			}
		}
		if(this->unx==NULL && (this->flags & UNET_FLOG) && !(this->flags & UNET_DLUNE)) {
			this->unx=new tLog;
			this->unx->open("unexpected.log",4,0);
		} else {
			this->unx=new tLog;
			this->unx->open(NULL,4,0);
		}
		if(this->ack==NULL && (this->flags & UNET_FLOG) && !(this->flags & UNET_DLACK)) {
			this->ack=new tLog;
			this->ack->open("ack.html",4,DF_HTML);
		} else {
			this->ack=new tLog;
			this->ack->open(NULL,4,DF_HTML);
		}
		if(this->chk==NULL && (this->flags & UNET_FLOG) && !(this->flags & UNET_DLCHK)) {
			this->chk=new tLog;
			this->chk->open("chk.log",4,0);
		} else {
			this->chk=new tLog;
			this->chk->open(NULL,4,0);
		}
		if(this->sec==NULL && (this->flags & UNET_FLOG) && !(this->flags & UNET_DLSEC)) {
			this->sec=new tLog;
			this->sec->open("access.log",4,0);
		} else {
			this->sec=new tLog;
			this->sec->open(NULL,4,0);
		}
	}
}


void tUnet::startOp() {
	if(initialized) return;
	_openlogs();
	//create an udp (17) socket
#ifdef __WIN32__
	memset(&this->ws,0,sizeof(this->ws));
	log->log("INF: Hasecorp Windoze sockets XP...\n");
	if(WSAStartup(MAKEWORD(1,1),&this->ws)!=0) {
		neterror("ERR: Cannot start up winsock ");
		throw txUnetIniErr(_WHERE("cannot start winsock"));
	}
	this->log->print("Winsock Version %d, %d\n",this->ws.wHighVersion,this->ws.wVersion);
	this->log->print(" %s - %s\n",this->ws.szDescription,this->ws.szSystemStatus);
	this->log->print(" maxsockets:%i, maxUdpDg:%i\n",this->ws.iMaxSockets,\
	this->ws.iMaxUdpDg);
	this->log->print(" vendor: %s\n",this->ws.lpVendorInfo);
#else
	this->log->print("INF: Linux sockets...\n");
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
	this->log->log("DBG: Socket created\n");

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
			this->err->logerr("ERR: Fatal setting socket as non-blocking (fnctl F_GETFL)\n");
			throw txUnetIniErr(_WHERE("Failed setting a non-blocking socket"));
		}
		arg |= O_NONBLOCK;

		if(fcntl(this->sock, F_SETFL, arg)<0) {
			this->err->log("ERR: Fatal setting socket as non-blocking\n");
			throw txUnetIniErr(_WHERE("Failed setting a non-blocking socket"));
		}
#endif
		this->log->log("DBG: Non-blocking socket set\n");
	} else {
		this->log->log("DBG: blocking socket set\n");
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

	if(!strcmp("0.0.0.0",bindaddr)) { //gethostbyname already does that, but just in case
		this->server.sin_addr.s_addr=htonl(INADDR_ANY); //any address
	} else {
		struct hostent *host;
		host=gethostbyname(bindaddr); //<- non-freed structure reported by dmalloc, huh :/ ?
		if(host==NULL) {
			this->err->log("ERR: Fatal cannot resolve address %s:%i\n",bindaddr,bindport);
			throw txUnetIniErr(_WHERE("Cannot resolve address %s:%i",bindaddr,bindport));
		}
		this->server.sin_addr.s_addr=*(U32 *)host->h_addr_list[0];
	}

	this->server.sin_port=htons(bindport); //port 5000 default

	//binding port
	if(bind(this->sock,(struct sockaddr *)&this->server,sizeof(this->server))<0) {
		this->err->log("ERR: Fatal - Failed binding to address %s:%i\n",bindaddr,bindport);
		neterror("bind() ");
		throw txUnetIniErr(_WHERE("Cannot bind to address %s:%i",bindaddr,bindport));
	}
	// 10 February 2004 - Alcugs development starts from scratch.
	// 10 February 2005 - Alcugs development continues..
	// The next line of code was originally written in 10/Feb/2004,
	// when the first listenning udp server named urud (uru daemon)
	// was compiled on that day.
	this->log->log("DBG: Listening to incoming datagrams on %s port udp %i\n\n",bindaddr,bindport);

	smgr=new tNetSessionMgr(this,this->max);

	if(this->max!=0) {
		this->log->log("DBG: Accepting up to %i connections\n",this->max);
	} else {
		this->log->log("DBG: Accepting unlimited connections\n",this->max);
	}
	this->log->flush();
	initialized=true;
}

/**
	Stops the network operation
*/
void tUnet::stopOp() {
	if(!initialized) return;
	if(smgr!=NULL) delete smgr;

#ifdef __WIN32__
	closesocket(this->sock);
	if(WSACleanup()!=0) {
		neterror("WSACleanup() ");
	}
#else
	close(this->sock);
#endif
	this->log->log("DBG: Socket clossed\n");
	if(this->log!=lstd) {
		this->log->close();
		delete this->log;
		this->log=NULL;
	}
	if(this->err!=lerr) {
		this->err->close();
		delete this->err;
		this->err=NULL;
	}
	this->ack->close();
	this->chk->close();
	this->unx->close();
	this->sec->close();
	delete this->ack;
	delete this->chk;
	delete this->unx;
	delete this->sec;
	this->ack=NULL;
	this->chk=NULL;
	this->unx=NULL;
	this->sec=NULL;
	initialized=false;
}


/** Urunet the netcore, does all, it's the heart of the server */
int tUnet::Recv() {
	int n;
	Byte buf[INC_BUF_SIZE]; //internal rcv buffer
	
	tNetSessionIte ite;
	tNetSession * session=NULL;

	struct sockaddr_in client; //client struct
	socklen_t client_len; //client len size
	client_len=sizeof(struct sockaddr_in); //get client struct size

	//waiting for packets - timeout
	fd_set rfds;
	struct timeval tv;
	int valret;

	/* Check socket for new messages */
	FD_ZERO(&rfds);
	FD_SET(this->sock, &rfds);
	/* Set timeout */
	tv.tv_sec = this->unet_sec;
	tv.tv_usec = this->unet_usec;

	DBG(7,"waiting for incoming messages...\n");
	valret = select(this->sock+1, &rfds, NULL, NULL, &tv);
	/* Don't trust tv value after the call */

	if(valret<0) {
		if (errno == EINTR) {
			// simply go around again, a signal was probably delivered
			return UNET_OK;
		}
		neterror("ERR in select() ");
		return UNET_ERR;
	}

#if _DBG_LEVEL_>7
	if (valret) { DBG(7,"Data recieved...\n"); } 
	else { DBG(7,"No data recieved...\n"); }
#endif
	//set stamp
	ntime_sec=alcGetTime();
	ntime_usec=alcGetMicroseconds();
	
	//Here, the old netcore performed some work (ack check, retransmission, timeout, pending paquets to send...)

	DBG(5,"Before recvfrom\n");
#ifdef __WIN32__
	n = recvfrom(this->sock,(char *)buf,INC_BUF_SIZE,0,(struct sockaddr *)&client,&client_len);
#else
	n = recvfrom(this->sock,(void *)buf,INC_BUF_SIZE,0,(struct sockaddr *)&client,&client_len);
#endif
	DBG(5,"After recvfrom\n");

#ifdef _UNET_DBG_
	if(n>0) {
		if(in_noise && (random() % 100) >= in_noise) {
			DBG(5,"Incomming Packet accepted\n");
		} else {
			DBG(5,"Incomming Packet dropped\n");
			n=0;
		}
		//check quotas
		if(ntime_sec-time_quota_check>=quota_check) {
			cur_down_quota=0;
			time_quota_check=ntime_sec;
		}
		if(n>0 && (cur_down_quota+n+ip_overhead)>lim_down_cap) {
			DBG(5,"Paquet dropped by quotas\n");
		} else {
			cur_down_quota+=n+ip_overhead;
		}
	}
#endif

	if(n<0 && valret) { //problems?
		neterror("ERR: Fatal recieving a message... ");
		return UNET_ERR;
	}

	if(n>0) { //we have a message
		/*TODO: Do firewall checking here.
			return UNET_REJECTED if the host is blacklisted
		*/

		if(n>OUT_BUFFER_SIZE) {
			this->err->log("[ip:%s:%i] ERR: Recieved a really big message of %i bytes\n",alcGetStrIp(client.sin_addr.s_addr),ntohs(client.sin_port),n);
			return UNET_TOOBIG;
		} //catch up impossible big messages
		/* Uru protocol check */
		if(buf[0]!=0x03) { //not an Uru protocol packet don't waste an slot for it
			this->unx->log("[ip:%s:%i] ERR: Unexpected Non-Uru protocol packet found\n",alcGetStrIp(client.sin_addr.s_addr),ntohs(client.sin_port));
			this->unx->dumpbuf(buf,n);
			this->unx->nl();
			return UNET_NONURU;
		}

		DBG(8,"Search session...\n");
		ite.ip=client.sin_addr.s_addr;
		ite.port=client.sin_port;
		ite.sid=-1;
		
		try {
			session=smgr->search(ite);
		} catch(txToMCons) {
			return UNET_TOMCONS;
		}
		
		//process the message, and do the correct things with it
		memcpy(session->sock_array,&client,sizeof(struct sockaddr_in));
		session->a_client_size=client_len;
		session->processMsg(buf,n);

	}
	return UNET_OK;
}


/**
	Dumps all data structures, with the address and the contents in hex
	flags
	0x01 append to the last log file (elsewhere destroy the last one)
*/
void tUnet::dump(tLog * sf,Byte flags) {
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
	f->print("net->server.sin_port:%02X (%i)\n",this->server.sin_port,htons(this->server.sin_port));
	f->print("net->server.sin_addr:%s\n",alcGetStrIp(this->server.sin_addr.s_addr));
	f->print("net->flags:%i\n",this->flags);
	f->print("net->unet_sec:%i\n",this->unet_sec);
	f->print("net->unet_usec:%i\n",this->unet_usec);
	f->print("net->max_version:%i\n",this->max_version);
	f->print("net->min_version:%i\n",this->min_version);
	f->print("net->timestamp:%s\n",alcGetStrTime(this->ntime_sec,this->ntime_usec));
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
}


} //end namespace



