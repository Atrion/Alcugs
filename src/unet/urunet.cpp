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
	this->init();
	this->StartOp(lport,lhost);
}
tUnet::~tUnet() {
	DBG(9,"~tUnet()\n");
	this->StopOp();
}
/**
	Fills the unet struct with the default values
*/
void tUnet::init() {
	DBG(9,"tUnet::init()\n");
	flags=UNET_NBLOCK | UNET_ELOG | UNET_ECRC | UNET_AUTOSP | UNET_NOFLOOD | UNET_FLOG | UNET_NETAUTH;
	whoami=0; //None (new connection) //KClient; //default type of peer

	unet_sec=1; //netcore timeout to do another loop (seconds)
	unet_usec=0; //netcore timeout to do another loop (microseconds)

	conn_timeout=3; //default timeout
	timeout=2000; //2 seconds (re-transmission)

	//initial server timestamp
	ntime=alcGetCurrentTime('u');

	this->n=0; //<! Current number of connections (internal, private)
	this->max=0; //<! Maxium number of connections (default 0, unlimited)
	this->smgr=NULL;

	this->lan_addr=0x00001AAC;
	this->lan_mask=0x00FFFFFF; //<! LAN mask, in network byte order (default 255.255.255.0)
	//! Bandwidth speed (lo interface -> maxium)
	this->lan_up=100 * 1000 * 1000;
	this->lan_down=100 * 1000 * 1000;
	this->nat_up=128 * 1000;
	this->nat_down=512 * 1000;

	//peers
	this->auth=-1;
	this->vault=-1;
	this->tracking=-1;
	this->meta=-1;

	//logs
	this->log=NULL;
	this->err=NULL;
	this->unx=NULL;
	this->ack=NULL;
	this->chk=NULL;
	this->sec=NULL;
}

/** private error function
*/
void tUnet::neterror(char * msg) {
#ifdef __WIN32__
	this->err->log("%s: winsock error code:%i\n",msg,WSAGetLastError());
#else
	this->err->logerr(msg);
#endif
}

/**
	Starts the network operation
	\param port to listen,
	\param FQHN e.g. "localhost", or "0.0.0.0" to bind all addresses
	\return 0 if success, non-zero failed
*/
int tUnet::StartOp(U16 port,char * hostname) {
	dumpBuffers(0x00);
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

#ifdef __WIN32__
	//start up winsock
	memset(&this->ws,0,sizeof(this->ws));
	log->log->log("INF: Hasecorp Windoze sockets XP...\n");

	if(WSAStartup(MAKEWORD(1,1),&this->ws)!=0) {
		neterror("ERR: Cannot start up winsock ");
		//winsock error here
		return UNET_FINIT;
	}

	this->log->print("Winsock Version %d, %d\n",this->ws.wHighVersion,this->ws.wVersion);
	this->log->print(" %s - %s\n",this->ws.szDescription,this->ws.szSystemStatus);
	this->log->print(" maxsockets:%i, maxUdpDg:%i\n",this->ws.iMaxSockets,\
	this->ws.iMaxUdpDg);
	this->log->print(" vendor: %s\n",this->ws.lpVendorInfo);
#else
	this->log->print("INF: Linux sockets...\n");
#endif

	//creating the socket
	//udp is listed as 17, but always 0 (ip) is used
	this->sock=socket(AF_INET,SOCK_DGRAM,0);

#ifdef __WIN32__
	if(this->sock==INVALID_SOCKET) {
#else
	if(this->sock<0) {
#endif
		neterror("ERR: Fatal - Failed Creating socket ");
		return UNET_FINIT;
	}
	this->log->log("DBG: Socket created\n");

	if(this->flags & UNET_NBLOCK) {

#ifdef __WIN32__
		//set non-blocking
		this->nNoBlock = 1;
		if(ioctlsocket(this->sock, FIONBIO, &this->nNoBlock)!=0) {
			neterror("ERR: Fatal setting socket as non-blocking\n");
		}
#else
		//set non-blocking
		long arg;
		if((arg = fcntl(this->sock,F_GETFL, NULL))<0) {
			this->err->logerr("ERR: Fatal setting socket as non-blocking (fnctl F_GETFL)\n");
			return UNET_FINIT;
		}
		arg |= O_NONBLOCK;

		if(fcntl(this->sock, F_SETFL, arg)<0) {
			this->err->log("ERR: Fatal setting socket as non-blocking\n");
			return UNET_FINIT;
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
		if(setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, (const char *)&this->opt, sizeof(int))!=0) {
			neterror(this->err,"ERR: Fatal - Failed setting BCAST socket ");
			return UNET_FINIT;
		}
		#else
		if(setsockopt(this->sock, SOL_SOCKET, SO_BROADCAST, &this->opt, sizeof(int))!=0) {
			neterror("ERR: Fatal - Failed setting BCAST socket ");
			return UNET_FINIT;
		}
		#endif
	}
	
	//chk?

	//set network specific options
	this->server.sin_family=AF_INET; //UDP IP

	if(!strcmp("0.0.0.0",hostname)) { //gethostbyname already does that, but just in case
		this->server.sin_addr.s_addr=htonl(INADDR_ANY); //any address
	} else {
		struct hostent *host;
		host=gethostbyname(hostname); //<- non-freed structure reported by dmalloc, huh :/ ?
		if(host==NULL) {
			this->err->log("ERR: Fatal cannot resolve address %s:%i\n",hostname,port);
			return UNET_INHOST;
		}
		this->server.sin_addr.s_addr=*(U32 *)host->h_addr_list[0];
	}

	this->server.sin_port=htons(port); //port 5000 default

	//binding port
	if(bind(this->sock,(struct sockaddr *)&this->server,sizeof(this->server))<0) {
		this->err->log("ERR: Fatal - Failed binding to address %s:%i\n",hostname,port);
		neterror("bind() ");
		return UNET_NOBIND;
	}
	// 10 February 2004 - Alcugs development starts from scratch.
	// 10 February 2005 - Alcugs development continues..
	// The next line of code was originally written in 10/Feb/2004,
	// when the first listenning udp server named urud (uru daemon)
	// was compiled on that day.
	this->log->log("DBG: Listening to incoming datagrams on %s port udp %i\n\n",hostname,port);

	smgr=new tNetSessionMgr(this->max);

	if(this->max!=0) {
		this->log->log("DBG: Accepting up to %i connections\n",this->max);
	} else {
		this->log->log("DBG: Accepting unlimited connections\n",this->max);
	}

	this->log->flush();

	dumpBuffers(0x01);

	return UNET_OK; //return success code
}

/**
	Stops the network operation
*/
void tUnet::StopOp() {
	int i;
	dumpBuffers(0x01);
	if(smgr!=NULL) delete smgr;
	dumpBuffers(0x01);

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
}


/**
	Urunet the netcore, does all, it's the heart of the server
	/param the unet descriptor and a place to store the peer_id
	/return an unet event
*/
int tUnet::Recv(int * sid) {
	int n,i,ret,ret2; //,off; //size of packet, and iterator, return codes & 2
	int s_old; //identifier for and old session found
	int s_new; //identifier for a void slot session found
	Byte buf[INC_BUF_SIZE]; //internal rcv buffer

	static char checker=0;

	struct sockaddr_in client; //client struct
	socklen_t client_len; //client len size
	client_len=sizeof(struct sockaddr_in); //get client struct size

	*sid=-1; n=0; s_old=-1; s_new=-1;

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
	this->ntime=alcGetCurrentTime('u');
	DBG(6,"Stamp set...\n");

	if(!valret || checker>50) {
		checker=0;
		//perform basic netcore things
		// - send non-acked packets, check dead hosts & resend pending packets
		// returns on UNET_TIMEOUT
		/*
		DBG(8,"for i=0; i<%i; i++\n",net->n);
		for(i=0; i<(int)net->n; i++) {
			DBG(9,"forstp i=%i\n",i);
			if(net->s[i].flag==0x00) break; //stop (there is nothing else ahead)
			else if(net->s[i].flag==0x01) {
				//check timeout
				if((net->timestamp - net->s[i].timestamp)>=net->s[i].timeout) {
					//timeout event
					DBG(5,"timeout event on peer:%i ip:%s:%i\n",i,get_ip(net->s[i].ip),ntohs(net->s[i].port));
					net->s[i].flag=0x03; //terminated/timeout
					*sid=i;
					return UNET_TIMEOUT;
				}
				//check non-acked packets
				if((net->timestamp - net->s[i].ack_stamp)>=net->ack_timeout) {
					DBG(6,"plNetReSendMessages(net,%i)\n",i);
					if(plNetReSendMessages(net,i)==UNET_TIMEOUT) {
						DBG(5,"ack: timeout event on peer:%i ip:%s:%i\n",i,get_ip(net->s[i].ip),ntohs(net->s[i].port));
						net->s[i].flag=0x03; //terminated/timeout
						*sid=i;
						return UNET_TIMEOUT;
					}
				}
			}
		}
		*/
	} else {
		checker++;
	}

	DBG(5,"Before recvfrom\n");
	//while() //get 10 messages from the buffer in a single row //TODO
			//return on UNET_TERMINATED, UNET_MSGRCV or UNET_NEWCONN

	//waiting for packets
#ifdef __WIN32__
	n = recvfrom(this->sock,(char *)buf,INC_BUF_SIZE,0,(struct sockaddr *)&client,&client_len);
#else
	n = recvfrom(this->sock,(void *)buf,INC_BUF_SIZE,0,(struct sockaddr *)&client,&client_len);
#endif

	DBG(5,"After recvfrom\n");

#ifdef _NOISE_
	if((random() % 100) >= in_noise) {
		//nothing, goes well
		DBG(5,"Incomming Packet accepted\n");
	} else {
		DBG(5,"Incomming Packet dropped\n");
		n=0; //drop the message
	}
#endif

	//DBG(5,"I'm here at line 1699, well, it was line 1699, I'm sure that still is line 1699...\n");
	if(n<0 && valret) { //problems?
		//DBG(5,"I'm going to call to neterr()..\n");
		neterror("ERR: Fatal recieving a message... ");
		//DBG(5,"Now, I'm getting out with an UNET_ERROR return code\n");
		return UNET_ERR;
	}
	//DBG(5,"Windows Sucks..\n");

	if(n>0) { //we have a message
		/*
			TODO: Do firewall checking here.
			return UNET_REJECTED if the host is blacklisted
		*/

		if(n>OUT_BUFFER_SIZE) {
			this->err->log("[ip:%s:%i] ERR: Recieved a really big message of %i bytes\n",alcGetStrIp(client.sin_addr.s_addr),ntohs(client.sin_port),n);
			return UNET_TOOBIG;
		} //catch up impossible big messages
		//do things with the message here

		/*
			Uru protocol check
			Drop packet if it is not an Uru packet without wasting a session
		*/
		if(buf[0]!=0x03) { //not an Uru protocol packet don't waste an slot for it
			this->unx->log("[ip:%s:%i] ERR: Unexpected Non-Uru protocol packet found\n",alcGetStrIp(client.sin_addr.s_addr),ntohs(client.sin_port));
			this->unx->dumpbuf(buf,n);
			this->unx->nl();
			return UNET_NONURU;
		}

		#if 0
		/*
		find the session id, and control where to manage and save data
		*/
		DBG(8,"Search session...\n");
		////ret=plNetSearchSession(net,client.sin_addr.s_addr,client.sin_port,sid);
		DBG(4,"search result %i\n",ret);

		switch(ret) {
			case UNET_NEWCONN:
				DBG(5,"Initializing session %i\n",*sid);
				plNetInitSession(net,*sid);
				net->s[*sid].ip=client.sin_addr.s_addr;
				net->s[*sid].port=client.sin_port;
				nlog(net->log,net,*sid,"INF: New peer - incomming message of %i bytes\n",n);
				nlog(net->sec,net,*sid,"INF: New incomming peer\n");
				dumpSessions(net->sec,net);
				logflush(net->sec);
				time((time_t *)&net->s[*sid].nego_stamp);
				net->s[*sid].nego_micros=get_microseconds();
				net->s[*sid].alive_stamp=net->s[*sid].nego_stamp; //set last alive stamp
				//time((time_t *)&net->s[*sid].last_check);
				break;
			case UNET_MSGRCV:
				nlog(net->log,net,*sid,"INF: Old peer - incomming message of %i bytes\n",n);
				break;
			case UNET_TOMCONS:
				ret=UNET_TOMCONS;
				*sid=-1;
				break;
			default:
				ret=UNET_ERR;
				*sid=-1;
				break;
		}

		if(ret==UNET_NEWCONN || ret==UNET_MSGRCV) {
			//copy structures
			DBG(5,"before memcpy()\n");
			memcpy(net->s[*sid].sock_array,&client,sizeof(struct sockaddr_in));
			net->s[*sid].a_client_size=client_len;
			DBG(5,"after memcpy()\n");

			//call message processor and assembler
			DBG(5,"before processClientMsg() call\n");
			ret2=processClientMsg(net,buf,n,*sid);
			DBG(5,"after processClientMsg() call\n");
			if(ret2!=UNET_MSGRCV) ret=ret2;

			//if i'm bussy
			if(net->s[*sid].bussy==1) ret=UNET_OK; //nothing to see here, move along...

		}

	} else {
		//we have nothing
		ret=UNET_OK;
	}

	st_uru_client * u=NULL;

	//check for pending messages if ret!=UNET_NEWCONN or UNET_MSGRCV
	if(ret!=UNET_NEWCONN && ret!=UNET_MSGRCV && ret!=UNET_FLOOD) {
		DBG(5,"pending messages check...\n");

		for(i=0; i<(int)net->n; i++) {
			if(net->s[i].flag==0x00) break; //stop (there is nothing else ahead)
			else if(net->s[i].flag==0x01 && net->s[i].bussy==0) {
				//first check that it's not bussy
				//then
				u=&net->s[i];
				st_unet_rcvmsg * ite; //iterator, that goes through all messages
				st_unet_rcvmsg * prev; //pointer to the previous message
				ite=u->rcvmsg;
				prev=u->rcvmsg;

				DBG(7,"Search for a message...\n");
				while(ite!=NULL) {
					if((net->timestamp - ite->stamp) > unet_snd_expire) { //expire messages
						nlog(net->err,net,i,"INF: Message %i expired!\n",ite->sn);
						// delete the expired message
						//It's the first one
						if(ite==u->rcvmsg) {
							//then set to the next one
							u->rcvmsg=(st_unet_rcvmsg *)(ite->next);
							prev=u->rcvmsg; //update prev
							if(ite->buf!=NULL) free((void *)ite->buf);
							free((void *)ite);
							ite=prev;
						} else {
							prev->next=ite->next;
							if(ite->buf!=NULL) free((void *)ite->buf);
							free((void *)ite);
							ite=(st_unet_rcvmsg *)prev->next;
						}
					} else {
						if(ite->completed==0x01) { //we found it!
							*sid=i;
							return UNET_MSGRCV;
						}
						//set the next message
						prev=ite;
						ite=(st_unet_rcvmsg *)ite->next;
					}
				}
				DBG(7,"End message search...\n");
			}
		} 	
#endif

	}

	DBG(7,"Returning from plNetRecv with ret:%i\n",ret);
	return ret;
}


/*
	Dumps all data structures, with the address and the contents in hex
	flags
	0x01 append to the last log file (elsewhere destroy the last one)
*/
void tUnet::dumpBuffers(Byte flags) {
#if _DBG_LEVEL_ > 2
	tLog * f=new tLog;
	if(flags & 0x01) {
		f->open("memdump.log",5,DF_APPEND);
	} else {
		f->open("memdump.log",5,0);
	}
	if(f==NULL) return;

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
	f->print("net->timestamp:%s\n",alcGetStrTime(this->ntime,'u'));
	f->print("net->conn_timeout:%i\n",this->conn_timeout);
	f->print("net->timeout:%i\n",this->timeout);
	f->print("net->n:%i\n",this->n);
	f->print("net->max:%i\n",this->max);
	//f->print("net->s:0x%08X\n",(int)this->s);
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
	//////dumpSessions(f,net);

	/*
	if(net->s!=NULL) {
		dumpbuf(f,(Byte *)net->s,sizeof(*net->s));
		lognl(f);
		int i;
		for(i=0; i<(int)net->n; i++) {
			dumpSessionBuffers(f,net,i);
		}
	} */
	f->log("Ending memory report\n");
	f->close();
	delete f;
#endif
}

}


#if 0

//enable noise in the netcore?
//#define _NOISE_

#include <fcntl.h>
#include <errno.h>

#include "prot.h" //protocol
#include "protocol.h"

#ifdef _NOISE_
/* Noise (0-100)*/
const int out_noise=10;
const int in_noise=10;
#endif

/* RCV window size */
const int rcv_win=2*8;

/* SND Window size */
const int max_win=60; //200 ~ 40
const int min_win=4;

/* magic number */
const int magic_delta=8 * (OUT_BUFFER_SIZE+25); //300

const U32 unet_snd_expire = 70; //40 seconds should be enough for a 56Kbps user

const int unet_flood_pckts = 80; //needs to be adjusted


/** network logging function
*/
void nlog(st_log * log,st_unet * net,int sid,char * msg,...) {
	va_list ap;
	static char buf[1025];

	if(log==NULL) return;

	va_start(ap,msg);
	vsnprintf(buf,sizeof(buf)-1,msg,ap);
	va_end(ap);

	//protection
	if(sid>=(int)net->n || sid<0) {
		plog(f_err,"Attempted access to an address out of range %i out of %i...\n",sid,net->n);
		plog(log,"[%i] %s",sid,buf);
		return;
	}

	plog(log,"[%i][%s:%i][%i,%i](%s,%s,%s) %s",sid,get_ip(net->s[sid].ip),\
	ntohs(net->s[sid].port),net->s[sid].whoami,net->s[sid].ki,net->s[sid].acct,net->s[sid].name,\
	create_str_guid((Byte *)net->s[sid].uid),buf); //get_guid((Byte *)net->s[sid].guid)
}



/**
  Sends and encrypts(if necessary) the correct packet
	Returns size if all gone well
	returns <0 if something went wrong
	(this is a low level function)
*/
int uru_net_send(st_unet * net,Byte * buf,int n,int sid) {
	//inet
	struct sockaddr_in client; //client struct

	st_uru_client * u=&net->s[sid];

	Byte * buf2=NULL;

	if(n>OUT_BUFFER_SIZE) {
		plog(net->err,"ERR: Attempted to send a message of %i bytes, and that is not possible!\n",n);
		return -1;
	}

	//copy the inet struct
	memcpy(&client,u->sock_array,sizeof(struct sockaddr_in));
	client.sin_family=AF_INET; //UDP IP (????)
	client.sin_addr.s_addr=u->ip; //address
	client.sin_port=u->port; //port

#if _DBG_LEVEL_ > 3
	print2log(net->log,"DBG: <SND>");
	uru_print_header(net->log,&u->server);
	print2log(net->log,"\n");
	logflush(net->log);
#endif

	htmlDumpHeaderRaw(net,net->ack,*u,buf,n,1);

	DBG(5,"validation level is %i\n",u->server.ch);

	//if(buf[1]==0x01 && buf[6]==0x42) abort();
	//if(buf[6]==0x42) {
		//printf("presh enter to continue...\n"); fflush(0);
		//ask();
	//}
	if(u->server.ch != buf[1]) {
		DBG(7,"Validation mismatch %i vs %i!\n",u->server.ch,buf[1]);
		plog(net->err,"ERR: Validation mismatch %i vs %i!\n",u->server.ch,buf[1]);
		return -1;
		//abort();
	}

#ifdef _STEP_BY_STEP_
	printf("presh enter to continue...\n"); fflush(0);
	ask();
#endif

	if(u->server.ch==2) {
		//DBG(4,"Validation 2 packet found...\n");
		DBG(4,"Enconding validation 2 packet of %i bytes...\n",n);
		buf2=(Byte *)malloc(sizeof(Byte) * OUT_BUFFER_SIZE);
		if(buf2==NULL) { plog(net->err,"Fatal attmeting to allocate buffer in uru_net_send()"); return -1; }
		encode_packet(buf2,buf,n);
		buf=buf2; //don't need to decode again
		if(u->authenticated==1) {
			DBG(4,"Client is authenticated, doing checksum...\n");
			//DBG(4,"Doing checksum...\n");
			*((U32 *)(buf+2))=uru_checksum(buf,n,2,u->passwd);
			DBG(4,"Checksum done!...\n");
		} else {
			DBG(4,"Client is not authenticated, doing checksum...\n");
			//DBG(4,"Doing checksum...\n");
			*((U32 *)(buf+2))=uru_checksum(buf,n,1,NULL);
			DBG(4,"Checksum done!...\n");
		}
		buf[1]=0x02;
	} else if(u->server.ch==1) {
		*((U32 *)(buf+2))=uru_checksum(buf,n,0,NULL);
		buf[1]=0x01;
	} else {
		buf[1]=0x00;
	}
	//DBG(4,"Magic number added...\n");
	buf[0]=0x03; //magic number
	DBG(4,"Before the Sendto call...\n");
	//
#ifdef _NOISE_
	if((random() % 100) >= out_noise) {
#endif

#ifdef __WIN32__
	n = sendto(net->sock,(const char *)buf,n,0,(struct sockaddr *)&client,sizeof(struct sockaddr));
#else
	n = sendto(net->sock,(char *)buf,n,0,(struct sockaddr *)&client,sizeof(struct sockaddr));
#endif

#ifdef _NOISE_
	} else {
		n=0;
	}
#endif

	if(n<0) {
		ERR(2,"n<0 ?, %i",n);
		neterror(net->err," sendto() ");
	}
	DBG(4,"After the Sendto call...\n");
	if(buf2!=NULL) { free((void *)buf2); buf2=NULL; }
	DBG(2,"returning from uru_net_send RET:%i\n",n);
	return n;
}

/** Generates an ack reply
		At current time, we are acking each packet individually
			TODO, put them in a buffer, and send them after processing 10-20 messages
			from the buffer
			NO!, imagine that, a single ACK packet acking 20 big messages is lost!!,
			now, we must resend again these 20 big messages, because we are not sure
			if the client have recieved them. I prefer single ack packets instead of paying
			this high price if a multiple ack packet is lost.
*/
int send_ackReply(st_unet * net,int sid) {
	st_uru_client * u=&net->s[sid];
	int n,start;//,off;
	Byte buf[OUT_BUFFER_SIZE]; //an ack buffer

	//update the pck counter
	u->server.p_n++;

	/* Check if the last message sent, had the ack flag on
		as a rule, messages must be sent in order */
	if(u->server.t & 0x02) {
		u->server.ps=u->server.sn;
		u->server.fr_ack=u->server.fr_n;
		//the second field, only contains the seq number from the latest packet with
		//the ack flag enabled.
	}
	//now update the other fields
	u->server.sn++;
	u->server.fr_n=0;
	u->server.fr_t=0;
	u->server.t=0x80;
	u->server.size=0x01;

	//V1 has the validation level 1 disabled on all ACK packets
	if(u->validation<=0x01) {
		u->server.ch=0x00;
	} else {
		u->server.ch=0x02;
	}
	u->server.cs=0xFFFFFFFF;

	start=uru_put_header(buf,&u->server);
	n=start;

	//now create the packet
	*(U16 *)(buf+n)=0x00;
	// only one ack this time
	n+=2;
	*(Byte *)(buf+n)=u->client.fr_n;
	n++;
	*(U32 *)(buf+n)=u->client.sn & 0x00FFFFFF;
	n+=3;
	*(U32 *)(buf+n)=0x00;
	n+=4;

	*(Byte *)(buf+n)=u->client.fr_ack;
	n++;
	*(U32 *)(buf+n)=u->client.ps & 0x00FFFFFF;
	n+=3;
	*(U32 *)(buf+n)=0x00;
	n+=4;

	print2log(net->log,"<SND> Ack (%i,%i) (%i,%i)\n",\
	u->client.fr_n,u->client.sn,u->client.fr_ack,u->client.ps);

	//uru_print_header(net->log,&u->server);
	//htmlDumpHeader(net->ack,*u,u->server,buf+start,n,1);

#if _DBG_LEVEL_ > 3
	dumpbuf(net->log,buf,n);
	lognl(net->log);
#endif
	return(uru_net_send(net,buf,n,sid));
}

/**
	Sends up to windowsize packets
	window size will be decreassed by one if the autospeed mode is on and a packet
	has been sent more than 2 times
	window size will be increassed by one if the autospeed mode is on, and the latest
	200 sent packets were sent only one time. (the first one)
	If a packet has been send more than 6 times, then a UNET_TIMEOUT event code will be
	returned if not, it will return UNET_OK
*/
int plNetReSendMessages(st_unet * net,int sid) {
	st_uru_client * u=&net->s[sid];
	int i=0;
	int start;

	const Byte auto_dec = 2;
	const Byte auto_inc = 200;
	const Byte timeout = 10; //number of tryes before the timeout

	Byte wk=0;

#ifdef _SINGLE_MSG_
	int last_sn=0,cur_sn; //last sequence number form a previous message
	Byte n_last_frags=0;
#endif

	//DBG(5,"dmalloc_verify()\n");
	//dmalloc_verify(NULL);

	st_unet_sndmsg * ite=u->sndmsg;

	if((net->timestamp - u->ack_stamp )>= net->ack_timeout) {
		u->ack_stamp=net->timestamp;
		u->vpos=0;
	}

	//DBG(5,"dmalloc_verify()\n");
	//dmalloc_verify(NULL);

	while((i<u->window || (wk<2 && u->window==0)) && ite!=NULL) {
		wk++; //at least always, two are sent

		if(i>=u->vpos) { //check last sent message id
			u->vpos=i+1; //update the counter

			DBG(6,"message %i\n",i);
			//send messages
			if(ite->tryes>timeout || ite->buf==NULL) return UNET_TIMEOUT; //timeout event on this peer, or buf is NULL, something that is not possible
			//small fix
			if(*(Byte *)((ite->buf)+1)==0x00) { start=2; }
			else { start=6; }

#ifdef _SINGLE_MSG_
			cur_sn=(*(U32 *)((ite->buf)+start+10) & 0x00FFFFFF);

			if(n_last_frags!=0 && cur_sn!=last_sn) {
				//UruExplorer hates this, so wait a little before sending the next message.
				u->vpos=0;
				break;
			}
			n_last_frags=*(Byte *)((ite->buf)+start+13);
			last_sn=cur_sn;
#endif
//Update, seems that UruExplorer, may accept it, but it may have some small issues.

			//end small fix
			if(ite->tryes>0) u->success=0; //reset success counter
			if(u->window!=0 && ite->tryes>auto_dec && (net->flags & UNET_AUTOSP)) {
				u->window--;
				if(u->window<min_win) u->window=min_win;
				nlog(net->log,net,sid,"Now window size is:%i\n",u->window);
			}
			ite->tryes++;
			u->server.p_n++;
			u->server.ch=*(Byte *)((ite->buf)+1);
			DBG(6,"Validation level is:%i\n",*(Byte *)((ite->buf)+1));

	#if _DBG_LEVEL_ > 9
			DBG(4,"Buffer details before updating counter %i bytes:\n",ite->size);
			dumpbuf(net->log,ite->buf,ite->size);
			lognl(net->log);
			if(*(Byte *)((ite->buf)+1)==0x00 && (*(Byte *)((ite->buf)+start+4)!=0x42) && *(Byte *)((ite->buf)+start+4)!=0x02) {
				//abort();
			}
			if(*(Byte *)((ite->buf)+1)==0x01 && (*(Byte *)((ite->buf)+start+4)==0x42)) {
				//abort();
			}
	#endif
			*(U32 *)((ite->buf)+start)=u->server.p_n; //update counter
			//htmlDumpHeader(net->ack,*u,u->server,ite->buf+start,ite->size,1);
			//send the message
			//buffer dumper
	#if _DBG_LEVEL_ > 8
			DBG(4,"Buffer details after updating counter %i bytes:\n",ite->size);
			dumpbuf(net->log,ite->buf,ite->size);
			lognl(net->log);
	#endif
			DBG(5,"uru_net_send call, size:%i bytes\n",ite->size);
			uru_net_send(net,ite->buf,ite->size,sid);
		} //end check vpos
		i++;
		ite=(st_unet_sndmsg *)ite->next; //set the next one (argh!)
	}

	if(u->success>auto_inc && (net->flags & UNET_AUTOSP)) {
		u->window++;
		if(u->window<max_win) u->window=max_win;
		nlog(net->log,net,sid,"Now window size is:%i\n",u->window);
	}

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);
	return UNET_OK;
}

/**
	Sends the message from the net struct in the Uru protocol
	buf is a valid uru message from the next layer
	n is the size.
	An uru message can only be 253952 bytes in V0x01 & V0x02 and 254976 in V0x00
	So, if your offline savegames are bigger than 250KBytes, blame to the Cyan network
	designer, not me.

	flags are
	0x00 - normal non ack flag packet
	0x02 - request for the ack flag
	0x40 - it's a negotiation
	0x80 - dump the packet to the buffer
	0x20 - force validation 0
*/
int plNetSend(st_unet * net, Byte * msg, int size,int sid,Byte flags) {
	if(net_check_address(net,sid)!=0) return UNET_ERR; //Hmm?
	st_uru_client * u=&net->s[sid];
	Byte * buf=NULL;

	int start,off,pkt_sz,n_packets,i;

	DBG(7,"Ok, I'm going to send a packet of %i bytes, for peer %i, with flags %02X\n",size,sid,flags);

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	/*
		1st) non-ack packets are directly send
		2nd) others go trough this process
			A) Fragment it, if it is necessary
			B) Store each fragment in the msg cue
			C) Send only up to window size packets (call the int plNetReSendMessages(st_unet * net,int sid) function to do it
	*/

	//store in one var the total number of bytes sent in one second,
	// if that number is bigger than the available configured in the wan up param,
	// then start dropping packets.
	// reset to zero, each new second

	//pck seq numbers

	/* Check if the last message sent, had the ack flag on
		as a rule, messages must be sent in order */
	if(u->server.t & 0x02) {
		u->server.ps=u->server.sn;
		u->server.fr_ack=u->server.fr_n;
		//the second field, only contains the seq number from the latest packet with
		//the ack flag enabled.
		DBG(5,"The previous sent packet had the ack flag on\n");
	}
	//now update the other fields
	u->server.sn++;
	u->server.fr_n=0;

	u->server.t=0x00;

	if(flags & 0x02) {
		u->server.t |= 0x02; //ack flag on
		DBG(5,"ack flag on\n");
	}
	if(flags & 0x40) {
		u->server.t |= 0x40; //negotiation packet
		DBG(5,"It's a negotation packet\n");
	}

	if(flags & 0x20) {
		u->server.ch=0x00;
		DBG(5,"forced validation 0\n");
	} else {
		u->server.ch=u->validation;
		DBG(6,"validation level is %i\n",u->server.ch);
	}

	//////
	//if(u->server.ch==0x00 && flags!=0x42) abort();
	/////

	if((u->server.t & 0x40) && (u->server.ch==0x01)) { u->server.ch=0x00; }
	DBG(6,"Sending a packet of validation level %i\n",u->server.ch);

	//////
	//if(u->server.ch==0x01 && flags==0x42) abort();
	//////

	if(u->server.ch==0x00) { start=28; } else { start=32; }
	u->server.cs=0xFFFFFFFF;

	off=start;

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	//ok 0x00 are directly sent
	if(!(u->server.t & 0x02)) {
		//update the pck counter
		u->server.p_n++;
		u->server.size=size;
		off+=size;
		buf=(Byte *)malloc(sizeof(Byte) * (off+50));
		if(buf==NULL) return UNET_ERR;
		uru_put_header(buf,&u->server);
		memcpy(buf+start,msg,size);
		DBG(6,"uru_net_send call, with a size of %i for sid %i\n",off,sid);
#if _DBG_LEVEL_ > 5
		dumpbuf(net->log,buf,off);
		lognl(net->log);
		logflush(net->log);
#endif
		uru_net_send(net,buf,off,sid);
		if(buf!=NULL) {
			free((void *)buf);
			buf=NULL;
		}
	} else {
		//Ok, do all the hard stuff
		pkt_sz=OUT_BUFFER_SIZE - start; //get maxium message size
		n_packets=(size-1)/pkt_sz; //get number of fragments
		DBG(5,"pkt_sz:%i n_pkts:%i\n",pkt_sz,n_packets);
		if(n_packets>=256) {
			nlog(net->err,net,sid,"ERR: Attempted to send a packet of size %i bytes, that don't fits inside an uru message, sorry! :(\n",size);
			return UNET_TOOBIG;
		}
		u->server.fr_t=n_packets; //set it

		for(i=0; i<=n_packets; i++) {
			if(i!=0) { //<- Troublemaker
				u->server.ps=u->server.sn;
				u->server.fr_ack=u->server.fr_n;
			}
			u->server.fr_n=i; //set fragment number

			if(i==n_packets) {
				u->server.size=size - (i*pkt_sz);
			} else {
				u->server.size=pkt_sz;
			}
			/* sanity check */
			if(u->server.size>OUT_BUFFER_SIZE) {
				nlog(net->err,net,sid,"ERR: A fragment has %i bytes!!, that's impossible, pkt_sz:%i\n",u->server.size,pkt_sz);
			}
			//ok, we are here, then store the message in the buffer

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

			if(u->sndmsg==NULL) {
				u->sndmsg=(st_unet_sndmsg *)malloc(sizeof(st_unet_sndmsg) * 1);
				if(u->sndmsg==NULL) return UNET_ERR;
				memset(u->sndmsg,0,sizeof(st_unet_sndmsg));
				u->sndmsg->next=NULL;
				u->sndmsg->size=start + u->server.size;
				#if _DBG_LEVEL_ > 3
					if(start + u->server.size<=0) { abort(); }
				#endif
				u->sndmsg->buf=(Byte *)malloc(sizeof(Byte) * (u->sndmsg->size+5));
				buf=u->sndmsg->buf;
			} else {
				st_unet_sndmsg * ite=NULL;
				ite=u->sndmsg;
				while(ite->next!=NULL) {
					ite=(st_unet_sndmsg *)ite->next; //go to the end of the message cue
				}
				ite->next=malloc(sizeof(st_unet_sndmsg) * 1);
				if(ite->next==NULL) return UNET_ERR;
				memset((st_unet_sndmsg *)ite->next,0,sizeof(st_unet_sndmsg));
				ite=(st_unet_sndmsg *)ite->next;
				ite->next=NULL;
				ite->size=start + u->server.size;
				ite->buf=(Byte *)malloc(sizeof(Byte) * (ite->size+5));
				buf=ite->buf;
			}
			if(buf==NULL) return UNET_ERR;
#if _DBG_LEVEL_ > 7
			DBG(6,"Buffer details:\n");
			dumpbuf(net->log,buf,start+u->server.size);
			lognl(net->log);
			logflush(net->log);
#endif
			uru_put_header(buf,&u->server);
#if _DBG_LEVEL_ > 6
			DBG(6,"Buffer details after put_header():\n");
			dumpbuf(net->log,buf,start+u->server.size);
			lognl(net->log);
			logflush(net->log);
#endif
			memcpy(buf+start,msg+(i*pkt_sz),u->server.size);
#if _DBG_LEVEL_ > 6
			DBG(6,"Buffer details after memcpy:\n");
			dumpbuf(net->log,buf,start+u->server.size);
			lognl(net->log);
			logflush(net->log);
#endif
		}
		//now send them
		//printf("presh enter to continue...\n"); fflush(0);
		//ask();

		DBG(5,"before plNetReSendMessages call\n");
		plNetReSendMessages(net,sid);
		DBG(5,"after plNetReSendMessages call\n");
	}

		DBG(5,"dmalloc_verify()\n");
		dmalloc_verify(NULL);
	//if(buf!=NULL) free((void *)buf);

	return UNET_OK;
}

/**
	Sends a negotation request
	(returns the error code returned by net_send)
*/
int plNetClientComm(st_unet * net, int sid) {
	st_uru_client * u=&net->s[sid];
	int n,start;
	char * timestamp;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

#if 0

	//update the pck counter
	u->server.p_n++;

	/* Check if the last message sent, had the ack flag on
		as a rule, messages must be sent in order */
	if(u->server.t & 0x02) {
		u->server.ps=u->server.sn;
		u->server.fr_ack=u->server.fr_n;
		//the second field, only contains the seq number from the latest packet with
		//the ack flag enabled.
	}
	//now update the other fields
	u->server.sn++;
	u->server.fr_n=0;
	u->server.t=0x42; //Negotiation
	u->server.size=0x0C;

	//V1 has the validation level 1 disabled on all negotiation packets
	if(u->validation<=0x01) {
		u->server.ch=0x00;
	} else {
		u->server.ch=0x02;
	}
	u->server.cs=0xFFFFFFFF;

	start=uru_put_header(buf,&u->server);
	n=start;

#else
	start=0;
	n=start;
#endif

	//now create the packet
	//server bandwidth
	DBG(8,"%08X %08X %08X\n",u->ip,net->lan_mask,net->lan_addr);
	if((u->ip & 0x00FFFFFF) == 0x0000007F) { //lo
		*(U32 *)(buf+n)=100000000;
	} else if((u->ip & net->lan_mask) == net->lan_addr) { //LAN
		*(U32 *)(buf+n)=net->lan_down;
	} else {
		*(U32 *)(buf+n)=net->nat_down; //WAN
	}
	n+=4;

	timestamp=ctime((const time_t *)&u->nego_stamp);

	print2log(net->log,"<SND> (Re)Negotation us: %i bandwidth: %i bps time: %s",u->nego_micros,*(U32 *)(buf+start),timestamp);

	//store timestamp
	*(U32 *)(buf+n)=u->nego_stamp;
	n+=4;
	*(U32 *)(buf+n)=u->nego_micros;
	n+=4;

//	htmlDumpHeader(net->ack,*u,u->server,buf+start,n,1);

#if _DBG_LEVEL_ > 3
	dumpbuf(net->log,buf,n);
	lognl(net->log);
#endif

#if 0
	return(uru_net_send(net,buf,n,sid));
#else
	plNetSend(net,buf,n,sid,0x42);
#endif

	DBG(3,"I'm here...\n");
	//printf("presh enter to continue...\n"); fflush(0);
	//ask();

	//if(u->server.ch==0x01) abort();

	return 0;
}

int parse_negotiation(st_unet * net,Byte * buf,int n,int sid) {
	st_uru_client * u=&net->s[sid];
	char * timestamp;
	U32 stamp,micros;
	int off=0;

	u->bandwidth=*(U32 *)(buf+off);
	off+=4;
	stamp=*(U32 *)(buf+off);
	off+=4;
	micros=*(U32 *)(buf+off);
	off+=4;

	timestamp=ctime((const time_t *)&stamp);

	print2log(net->log,"<RCV> (Re)Negotation us: %i bandwidth: %i bps time: %s",micros,u->bandwidth,timestamp);

	if(stamp!=u->renego_stamp && micros!=u->renego_micros) {
		//reset server/client pn, sn, frn, ... et all
		//u->server.p_n=0;
		//u->server.fr_n=0;
		//u->server.sn=0;
		//u->server.fr_ack=0;
		//u->server.ps=0;
		u->old_p_n=0; //last packet
		u->wite=u->client.sn; //the window iterator
		memset(u->w,0,sizeof(char) * rcv_win); //unset all

		u->renego_stamp=stamp;
		u->renego_micros=micros;
		DBG(5,"the window is:%i\n",u->window);
		if(u->window!=0) {
			DBG(5,"Sending a negotiation packet here...\n");
			plNetClientComm(net,sid);
		}
		u->window=0; //reset the window size instead (this will force to recalculate the window)
		//we need to send it, if the client sends a different time
	} else {
		plog(net->log,"Recieved an old re-negotiation message\n");
	}
	return 0;
}

/**
	Message assembler
	1) Find the slot in the cue (& delete old messages)
		1.1) If exists put the message on it
		1.2) If not create a new one in the correct place
	2) Put the piece in the correct part and update the bitmask
	3) Check if the message is completed
*/
int net_assemble_message(st_unet * net,Byte * buf,int n,int sid) {
	st_uru_client * u=&net->s[sid];

	int start;

	U32 sn=u->client.sn; //the message number
	Byte frn=u->client.fr_n; //the message fragment
	Byte nfrs=u->client.fr_t; //total number of fragments

	st_unet_rcvmsg * ite; //iterator, that goes through all messages
	st_unet_rcvmsg * prev; //pointer to the previous message
	st_unet_rcvmsg * found=NULL; //we found it?
	st_unet_rcvmsg * fetch=NULL; //store the message with the sn<current_sn
	ite=u->rcvmsg;
	prev=u->rcvmsg;

	DBG(7,"Search for a message...\n");
	while(ite!=NULL) {
		if((net->timestamp - ite->stamp) > unet_snd_expire) { //expire messages
			nlog(net->err,net,sid,"INF: Message %i expired!\n",ite->sn);
			// delete the expired message
			//It's the first one
			if(ite==u->rcvmsg) {
				//then set to the next one
				u->rcvmsg=(st_unet_rcvmsg *)(ite->next);
				prev=u->rcvmsg; //update prev
				if(ite->buf!=NULL) free((void *)ite->buf);
				free((void *)ite);
				ite=prev;
			} else {
				prev->next=ite->next;
				if(ite->buf!=NULL) free((void *)ite->buf);
				free((void *)ite);
				ite=(st_unet_rcvmsg *)prev->next;
			}
		} else {
			if(ite->sn==sn) { //we found it!
				found=ite;
			}
			if(ite->sn<sn) { //fetch for insertion
				fetch=ite;
			}
			//set the next message
			prev=ite;
			ite=(st_unet_rcvmsg *)ite->next;
		}
	}
	DBG(7,"End message search...\n");

	if(found==NULL) { //not found, then create it
		ite=(st_unet_rcvmsg *)malloc(sizeof(st_unet_rcvmsg) * 1);
		if(ite==NULL) {
			plog(net->err,"FATAL, not enough memory allocating a buffer in the net_assembler!\n");
			return -1;
		}
		memset((void *)ite,0,sizeof(st_unet_rcvmsg));
		ite->sn=sn;
		ite->buf=(Byte *)malloc(sizeof(Byte) * (OUT_BUFFER_SIZE*(nfrs+1)));
		ite->stamp=net->timestamp;
		if(ite->buf==NULL) {
			plog(net->err,"FATAL, not enough memory allocating a message buffer in the net_assembler!\n");
			free((void *)ite);
			return -1;
		}
		if(fetch==NULL) {
			ite->next=(void *)(u->rcvmsg);
			u->rcvmsg=ite;
		} else {
			ite->next=(void *)(fetch->next);
			fetch->next=ite;
		}
		found=ite;
	}

	DBG(7,"buf.. message insertion code worked...\n");

	//wow, that was ugly, now this is more ugly ;)

	//Assemble the messages
	//additional paranoic sanity check
	if(found==NULL || found->buf==NULL) {
		plog(net->err,"FATAL, found or buf are NULL! - this is terrible!\n");
		return -1;
	}

	start=uru_get_header_start(&u->client);

	if(!((found->check[frn/8] >> (frn%8)) & 0x01)) {
		//Ok, we found a missing fragment
		found->check[frn/8] |= (0x01<<(frn%8));

		//perform sanity checks
		if((int)u->client.size!=(int)n || ((int)(OUT_BUFFER_SIZE-(int)start)!=(int)n && (int)frn!=(int)nfrs)) { //to lazy to check which one was causing the warning
			plog(net->err,"FATAL, sanity check on message size failed %i=?%i=?%i!!!!\n",u->client.size,n,OUT_BUFFER_SIZE-start);
			return -1;
		}

		memcpy(found->buf+(frn * (OUT_BUFFER_SIZE-start)),buf,u->client.size);

		found->size+=u->client.size;

		if(found->fr_count==nfrs) { //well, we finished, notify the caller
			DBG(8,"We have a full message in the message assembler...\n");
			found->completed=0x01;
			return 1;
		} else {
			found->fr_count++;
		}
	} //else, nope, this fragmet may be from a repeated fragment

	DBG(8,"No full message this time...\n");
	return 0;
}

/**
	Does the ack thingy
*/
int ack_update(st_unet * net,Byte * buf,int size,int sid) {
	st_uru_client * u=&net->s[sid];

	int i,off,sn,snf,start,gotsn;
	Byte frn,frnf,gotfrn,nfrags;
	off=2;
	Byte * biff=NULL;

	int A1,A2,A3;

	st_unet_sndmsg * ite=NULL;
	st_unet_sndmsg * prev=NULL;

	print2log(net->log,"<RCV>");
	for(i=0; i<(int)u->client.size; i++) {
		A1=*(U32 *)(buf+off); //<--
		frn=*(Byte *)(buf+off);
		off++;
		sn=*(U32 *)(buf+off) & 0x00FFFFFF;
		off+=3;
		off+=4;
		A3=*(U32 *)(buf+off); //<--
		frnf=*(Byte *)(buf+off);
		off++;
		snf=*(U32 *)(buf+off) & 0x00FFFFFF;
		off+=3;
		off+=4;
		if(i!=0) print2log(net->log,"    |");
		print2log(net->log," Ack %i,%i %i,%i\n",frn,sn,frnf,snf);
		//well, do it
		ite=u->sndmsg;
		prev=ite;
		while(ite!=NULL) {
			if(ite->buf==NULL) { plog(net->err,"Fatal error occurred, the ack buffer is NULL!\n"); return UNET_ERR; }
			biff=ite->buf;
			if(biff[1]==0x00) start=11;
			else start=15;
			A2=*(U32 *)(biff+start); //<--
			gotfrn=*(Byte *)(biff+start);
			gotsn=*(U32 *)(biff+start+1) & 0x00FFFFFF;
			nfrags=*(Byte *)(biff+start+4);

			//if((gotsn>snf || (gotsn==snf && gotfrn>frnf && gotfrn<=frn)) && gotsn<=sn) {
			//if((A1==A2) || (nfrags==0 && A1>=A2 && A2>A3) || (sn==gotsn && nfrags!=0 && gotsn==snf && frn>=gotfrn && gotfrn>frnf)) { //hmmm
			if(A1>=A2 && A2>A3) {
				//then delete
				plog(net->log,"Deleting packet %i,%i\n",gotfrn,gotsn);
				u->vpos--;
				if(u->vpos<0) { u->vpos=0; }
				//1st
				dumpBuffers(net,0x01);
				if(ite==u->sndmsg) {
					u->sndmsg=(st_unet_sndmsg *)(ite->next);
					prev=u->sndmsg;
					if(ite->buf!=NULL) free((void *)ite->buf);
					free((void *)ite);
					ite=prev;
				} else {
					prev->next=ite->next;
					if(ite->buf!=NULL) free((void *)ite->buf);
					free((void *)ite);
					ite=(st_unet_sndmsg *)prev->next;
				}
			} else {
				prev=ite;
				ite=(st_unet_sndmsg *)ite->next; //set next one
			}
		}
	}
	return UNET_OK;
}




void dumpSession(st_log * log,st_unet * net,int i) {
	print2log(log,"[%i] f:%i,w:%i,auth:%i,val:%i,ip:%s:%i,wins:%i,a:(%s,%s)[%s]\n",\
	i,net->s[i].flag,net->s[i].whoami,net->s[i].authenticated,net->s[i].validated,\
	get_ip(net->s[i].ip),ntohs(net->s[i].port),net->s[i].window,net->s[i].name,\
	net->s[i].acct,net->s[i].uid);
}

/**
	Dumps the session table
*/
void dumpSessions(st_log * log,st_unet * net) {
	int i;
	for(i=0; i<(int)net->n; i++) {
		dumpSession(log,net,i);
	}
}

/**
	Creates all the required data structures with the correct default values
*/
void plNetInitSession(st_unet * net,int sid) {

	DBG(7,"init sid %i...\n",sid);
	memset((void *)&net->s[sid],0,sizeof(st_uru_client));
	net->s[sid].flag=0x01; //Set In use
	net->s[sid].sid=sid; //set sid

	//set timeout
	net->s[sid].timeout=net->timeout;

	//memset should be already doing that, but just in case
	//net->s[sid].name=NULL;
	//net->s[sid].acct=NULL;
	//net->s[sid].uid=NULL;
	//net->s[sid].passwd=NULL;
	net->s[sid].w=(char *)malloc(sizeof(char) * rcv_win);
	memset(net->s[sid].w,0,sizeof(char) * rcv_win);
	net->s[sid].wite=0;

	net->s[sid].rcvmsg=NULL;
	net->s[sid].sndmsg=NULL;

	DBG(7,"session %i initialized...\n",sid);
}

/** Internal only, destroys all data structures from one slot,
    never use it from another layer
*/
void destroyAllDataStructures(st_uru_client * who) {

	DBG(8,"destroyAllDataStructures() init ...\n");
	//if(who->name!=NULL) { free((void *)who->name); who->name=NULL; }
	//if(who->acct!=NULL) { free((void *)who->acct); who->acct=NULL; }
	//if(who->uid!=NULL) { free((void *)who->uid); who->uid=NULL; }
	//if(who->passwd!=NULL) { free((void *)who->passwd); who->passwd=NULL; }

	free((void *)who->w);

	st_unet_rcvmsg * msg;
	while(who->rcvmsg!=NULL) {
		msg=who->rcvmsg;
		who->rcvmsg=(st_unet_rcvmsg *)(who->rcvmsg->next);
		if(msg->buf!=NULL) {
			free((void *)msg->buf);
		}
		free((void *)msg);
	}
	DBG(8,"OK, incomming message buffer succesfully destroyed!\n");

	st_unet_sndmsg * msg2;
	while(who->sndmsg!=NULL) {
		msg2=who->sndmsg;
		who->sndmsg=(st_unet_sndmsg *)(who->sndmsg->next);
		if(msg2->buf!=NULL) {
			free((void *)msg2->buf);
		}
		free((void *)msg2);
	}
	DBG(8,"OK, outcomming message buffer succesfully destroyed!\n");

	memset(who,0,sizeof(st_uru_client)); //just in case
	DBG(8,"destroyAllDataStructures() ending...\n");
}

/**
	Destroys all the data structures from an specific session
*/
void plNetDestroySession(st_unet * net,int sid) {
	//0x00 latest new session in the struct
	//0x01 in use
	//0x02 deleted
	//0x03 terminated/timeout (waiting for a decission from the app layer)

	if(net_check_address(net,sid)!=0) { return; } //avoid reading out of range
	nlog(net->sec,net,sid,"Connection to peer destroyed...\n");
	logflush(net->sec);

	int vflag=-1;

	int n;
	n=net->n-1;
	DBG(6,"destroying session %i\n",sid);

	//catch up strange impossible situations, and notify them (sanity check)
	if(sid<n && net->s[sid+1].flag==0x00) {
		plog(net->err,"ERR: Something weird in the session table!\n");
		dumpSessions(net->err,net);
	}

	if(sid==n) { // || net->s[sid+1].flag==0x00) {
		net->s[sid].flag=0x00;
		vflag=0;
		//go back deleting stuff
		while(sid>=0 && (net->s[sid].flag==0x02 || net->s[sid].flag==0x00)) {
			destroyAllDataStructures(&net->s[sid]);
			sid--;
		}
		n=sid;

		net->n=n+1;
		if(net->n==0) {
			free((void *)net->s);
			net->s=NULL;
		} else {
			st_uru_client * aux;
			aux=(st_uru_client *)realloc((void *)net->s,sizeof(st_uru_client) * net->n);
			if(aux==NULL) {
				plog(net->err,"FATAL: Not enough memory!, session was not destroyed\n");
			} else {
				net->s=aux;
			}
		}

	} else {
		destroyAllDataStructures(&net->s[sid]); //this sets the flag to 0!!!
		net->s[sid].flag=0x02;
		vflag=0x02;
	}

	plog(net->sec,"Session destroyer results are: flag:%i,sid:%i,net->n:%i\n",vflag,sid,net->n);
	dumpSessions(net->sec,net);

	DBG(7,"session destroyed...\n");
}

/**
	Returns the session sid from the specified ip address.
	If also will create a new session, if no session was found
	It will return one of the next events: UNET_TOMCONS, UNET_NEWCONN, UNET_OK.
*/
int plNetSearchSession(st_unet * net,U32 ip,U16 port,int * sid) {

	int i;
	int s_new=-1, s_old=-1;
	
//Debug sid's
#define DBGSID1L 0 //6
#define DBGSID2L 0 //8
#define DBGSID3L 0 //9

	for(i=0; i<(int)net->n; i++) {
		DBG(DBGSID2L,"i:%i,net->n:%i\n",i,net->n);
		if(net->s[i].flag==0x00) { //This is not going to happen never
			plog(net->err,"Now, I think that the problem is serious, it may be the weather, but the problem is serious\n");
			dumpBuffers(net,0x00);
			//plog(net->err,_WHERE("FATAL, Abort call on unet3 code\n"));
			//abort();
			_DIE("flag should not be 0x00\n");
			//TODO, flag=0 is defunct, now there are only these flags.
			//  1=in use, 2=deleted, 3=timeout  0 is only used for initizialitzation
			if(s_new!=-1 && s_new>i) {
				s_new=i;
				DBG(DBGSID1L,"lowest new sid found:%i\n",s_new);
			} else {
				if(i>0 && net->s[i-1].flag==0x02) {
					net->s[i-1].flag=0x00; //go back
				}
			}
			break; //stop (there is nothing else ahead)
		} else if(net->s[i].flag==0x02) {
			if(s_new!=-1 && s_new>i) {
				s_new=i;
				DBG(DBGSID1L,"lowest new recycled sid found:%i\n",s_new);
			}
		} else if(net->s[i].flag==0x01 || net->s[i].flag==0x03) {
			if(net->s[i].ip==ip && net->s[i].port==port) {
				s_old=i; //we found it
				DBG(DBGSID1L,"lowest old sid found:%i\n",s_old);
				break; //all done!
			}
		} else {
			plog(net->err,"Houston, We Have A Problem!\n");
		}
	}

	*sid=-1;
	if(s_old!=-1) {
		DBG(DBGSID2L,"Old sid:%i\n",*sid);
		*sid=s_old;
		if(net->s[*sid].flag==0x03) {
			//plNetDestroySession(net,i); //avoid memory garbage
			//destroyAllDataStructures(&net->s[*sid]);
			//return UNET_NEWCONN;
			return UNET_MSGRCV;
		} else {
			return UNET_MSGRCV;
		}
	} else if(s_new!=-1) {
		DBG(DBGSID2L,"recycled sid:%i\n",*sid);
		*sid=s_new;
		destroyAllDataStructures(&net->s[*sid]);
		return UNET_NEWCONN;
	} else if(net->n<net->max || net->max==0) {
		//well do the job here
		net->n++;
		st_uru_client * aux;
		aux=(st_uru_client *)realloc((void *)net->s,sizeof(st_uru_client) * net->n);
		if(aux==NULL) { return UNET_ERR; }
		net->s=aux;
		*sid=net->n-1;
		DBG(DBGSID2L,"New sid:%i\n",*sid);
		return UNET_NEWCONN;
	} else {
		DBG(DBGSID3L,"something went wrong\n");
		return UNET_TOMCONS;
	}
	DBG(DBGSID3L,"this part is never reached...\n");
	_DIE("got into an unreachable part of the code\n");
	return UNET_ERR;
}

/** Internal only low level message processor

*/

int processClientMsg(st_unet * net,Byte * buf,int n,int sid) {

	int ret2,off,ret;

	st_uru_client * s=&net->s[sid];

	//update client session time
	time((time_t *)&s->timestamp);
	s->microseconds=get_microseconds();

	//nlog(net->log,net,sid,"what's up %i:%i\n",s->timestamp,s->microseconds);

	//validate the message
	ret2=uru_validate_packet(buf,n,s);

	if(ret2!=0 && net->flags & UNET_ECRC) {
		if(ret2==1) {
			nlog(net->chk,net,sid,"ERR: Failed validating the message!\n");
			dumpbuf(net->chk,buf,n);
			lognl(net->chk);
			return UNET_CRCERR;
		} else {
			nlog(net->unx,net,sid,"ERR: Non-Uru protocol packet recieved!\n");
			dumpbuf(net->unx,buf,n);
			lognl(net->unx);
			return UNET_NONURU;
		}
	}

	ret=UNET_OK;

	#if _DBG_LEVEL_ > 2
		DBG(2,"RAW Packet follows: \n");
		dumpbuf(net->log,buf,n);
		lognl(net->log);
	#endif

	//get client header
	off=uru_get_header(buf,n,&s->client);
	print2log(net->log,"<RCV>");
	uru_print_header(net->log,&s->client);
	lognl(net->log);

	htmlDumpHeader(net->ack,*s,s->client,buf+off,n-off,0);

	int window;
	//Check negotiation status & peer technology level
	if(s->window==0 && s->bandwidth!=0) {
		if((s->ip & 0x00FFFFFF) == 0x0000007F) { //lo
			window=200;
		} else if((s->ip & net->lan_mask) == net->lan_addr) { //LAN
			window=((net->lan_up > s->bandwidth) ? s->bandwidth : net->lan_up)\
			 / magic_delta;
		} else { //WAN
			window=((net->nat_up > s->bandwidth) ? s->bandwidth : net->nat_up)\
			 / magic_delta;
		}
		if(window>max_win) window=max_win;
		else if(window<min_win) window=min_win;
		s->window=window;
		plog(net->log,"Set a window of %i\n",window);
	}

	if(s->window==0 || s->bandwidth==0) {
		//request a negotiation message
		plNetClientComm(net,sid);
	}
	//<--

	if(s->client.t & 0x02) {
		//this packet must be acked
		send_ackReply(net,sid); //Ack replyes are immediatly sent
	}
	if(s->client.t==0x42) { //Negotiation Message
		parse_negotiation(net,buf+off,n-off,sid);
		ret=UNET_OK;
	}

	//fix the problem that happens every 15-30 days of server uptime
#if 1
	if(s->server.sn>=8388605 || s->client.sn>=8388605) {
		plog(f_err,"INF: Congratulations!, you have reached the maxium allowed sequence number, don't worry, this is not an error\n");
		s->server.p_n=0;
		s->server.fr_n=0;
		s->server.sn=0;
		s->server.fr_ack=0;
		s->server.ps=0;
		s->nego_stamp=(U32)time(NULL);
		s->nego_micros=get_microseconds();
		s->renego_stamp=0,
		s->renego_micros=0;
		plNetClientComm(net,sid);
	}
#endif

	//check for duplicates ***************
	//drop already parsed messages
	//nlog(f_err,net,sid,"INF: (Before) window is:\n");
	//dumpbuf(f_err,(Byte *)s->w,rcv_win);
	//lognl(f_err);
	if(!(s->client.sn > s->wite || s->client.sn <= (s->wite+rcv_win))) {
		nlog(f_err,net,sid,"INF: Dropped packet %i out of the window by sn\n",\
		s->client.sn);
		logflush(f_err);
		return UNET_OK;
	} else { //then check if is already marked
		int i,start,ck;
		start=s->wite % (rcv_win*8);
		i=s->client.sn % (rcv_win*8);
		if(((s->w[i/8] >> (i%8)) & 0x01) && s->client.fr_n==0 && s->client.t!=0x42) {
			nlog(f_err,net,sid,"INF: Dropped already parsed packet %i\n",s->client.sn);
			logflush(f_err);
			return UNET_OK;
		} else {
			s->w[i/8] |= (0x01<<(i%8)); //activate bit
			while((i==start && ((s->w[i/8] >> (i%8)) & 0x01))) { //move the iterator
				s->w[i/8] &= ~(0x01<<(i%8)); //deactivate bit
				i++; start++;
				s->wite++;
				if(i>=rcv_win*8) { i=0; start=0; }
				//nlog(f_err,net,sid,"INF: A bit was deactivated (1)\n");
			}
			ck=(i < start ? i+(rcv_win*8) : i);
			while((ck-start)>((rcv_win*8)/2)) {
				DBG(5,"ck: %i,start:%i\n",ck,start);
				s->w[start/8] &= ~(0x01<<(start%8)); //deactivate bit
				start++;
				s->wite++;
				if(start>=rcv_win*8) { start=0; ck=i; }
				//nlog(f_err,net,sid,"INF: A bit was deactivated (2)\n");
			}
			//nlog(f_err,net,sid,"INF: Packet %i accepted to be parsed\n",s->client.sn);
		}
	}
	//nlog(f_err,net,sid,"INF: (After) window is:\n");
	//dumpbuf(f_err,(Byte *)s->w,rcv_win);
	//lognl(f_err);
	//end duplicate check ***********


/*	if(s->client.p_n<=s->old_p_n && s->client.t!=NetClientComm && s->client.t!=0x80) { //0x42
		nlog(net->err,net,sid,"INF: Discarded an old message %i\n",s->client.p_n);
		return UNET_OK;
	}
	s->old_p_n=s->client.p_n;
*/

	if(s->client.t==NetAck) { //0x80
		//process here the ack packet
		ack_update(net,buf+off,n-off,sid);
		//V2 auth ext
		if(s->authenticated==2) { s->authenticated=1; }
		ret=UNET_OK;
	} else {
		if(s->client.t==0x02) { //before was a &, instead of an ==
			//flooding control
			if(net->flags & UNET_NOFLOOD) {
				if(net->timestamp - s->last_check > 5) {
					time((time_t *)&s->last_check);
					s->npkts=0;
				} else {
					s->npkts++;
					if(s->npkts>unet_flood_pckts) { //needs to be adjusted
						ret=UNET_FLOOD;
					}
				}
			}
			//end flooding control

			//this packet must be acked
			//send_ackReply(net,sid); //Ack replyes are immediatly sent

		}
		/*
		if(s->client.t==0x42) { //Negotiation Message
			parse_negotiation(net,buf+off,n-off,sid);
			ret=UNET_OK;
		}
		*/

		if(s->client.t==0x02 || s->client.t==0x00) {
			//call the message assembler
			ret2=net_assemble_message(net,buf+off,n-off,sid);

			if(ret2==0) { ret=UNET_OK; }
			else if(ret2==1) {
				if(ret!=UNET_FLOOD) ret=UNET_MSGRCV;
			}
			else {
				ret=UNET_ERR;
			}
		}
	}

	/*
	if(ret==UNET_MSGRCV || ret==UNET_FLOOD) {
		//determine here if we need to set UNET_OK because the client is bussy
		//already done in another part of the code

	}
	*/

	//dumpbuf(net->log,buf,n);
	//lognl(net->log);

	return ret;
}


/**
	Starts a connection to an specific host

	flags:
		0x01 - force validation level 1
		0x02 - force validation level 0
		0x01 & 0x02 - force validation level 3 (* not implemented *)
*/

int plNetConnect(st_unet * net,int * sid,char * address,U16 port,Byte flags) {

	int ret;

	struct sockaddr_in client;
	struct hostent *host;
	host=gethostbyname(address);
	if(host==NULL) {
		return UNET_INHOST;
	}

	//printf("%08X\n",flags);
	//abort();
	if((flags & 0x03)==0x03) {
		ret=UNET_ERR;
		plog(net->err,"WAR: Validation level 3 still not implemented!\n");
		return ret;
	}

	//now search a session for the new connection
	ret=plNetSearchSession(net,*(U32 *)host->h_addr_list[0],htons(port),sid);

	switch(ret) {
		case UNET_NEWCONN:
			plNetInitSession(net,*sid);
			net->s[*sid].ip=*(U32 *)host->h_addr_list[0];
			net->s[*sid].port=htons(port);
			nlog(net->log,net,*sid,"INF: New peer - outcomming message\n");
			nlog(net->sec,net,*sid,"INF: New outcomming peer\n");
			logflush(net->sec);
			time((time_t *)&net->s[*sid].nego_stamp);
			net->s[*sid].nego_micros=get_microseconds();
			net->s[*sid].alive_stamp=net->s[*sid].nego_stamp; //set last alive stamp
			//time((time_t *)&net->s[*sid].last_check);
			//set default version
			net->s[*sid].max_version=12;
			net->s[*sid].min_version=6;
			break;
		case UNET_MSGRCV:
			nlog(net->log,net,*sid,"INF: Old peer - outcomming message\n");
			break;
		case UNET_TOMCONS:
			ret=UNET_TOMCONS;
			break;
		default:
			ret=UNET_ERR;
			break;
	}

	if(ret==UNET_NEWCONN || ret==UNET_MSGRCV) {
		st_uru_client * s=&net->s[*sid];
		//copy structures
		client.sin_family=AF_INET; //UDP IP
		client.sin_addr.s_addr=s->ip; //address
		client.sin_port=s->port; //port
		memcpy(net->s[*sid].sock_array,&client,sizeof(struct sockaddr_in));
		net->s[*sid].a_client_size=sizeof(struct sockaddr_in);

		//update client session time
		time((time_t *)&s->timestamp);
		s->microseconds=get_microseconds();
		ret=UNET_OK;

		if(flags & 0x02) {
			if(flags & 0x01) {
				//validation level 3 requested
				s->validation=3;
				ret=UNET_ERR;
				nlog(net->err,net,*sid,"ERR: Validation level 3 still not implemented!\n");
			}
			s->validation=0;
		} else if(flags & 0x01) {
			s->validation=1;
		} else {
			s->validation=2;
		}
		//do the initial negotiation
		if(ret==UNET_OK) {
			plNetClientComm(net,*sid);
		}
	}

	return ret;
}

/**
	Returns the string explaining the error that ocurred
*/

char * get_unet_error(S16 code) {
	switch (code) {
		case UNET_REJECTED:
			return "The connection was rejected";
		case UNET_TOMCONS:
			return "Reached the maxium number of connections supported by this system";
		case UNET_NONURU:
			return "Ignored a non-uru message, generated by another protocol, a port scan, or another unknown system";
		case UNET_CRCERR:
			return "Ignored an incomming message due to a checksum missmatch";
		case UNET_TOOBIG:
			return "The message was discarded, because it was biggest than the Uru maxium transmission unit";
		case UNET_ERR:
			return "A generic system error occurred";
		case UNET_NOBIND:
			return "Cannot bind to the requested address";
		case UNET_INHOST:
			return "Cannot resolve or bind the requested address";
		case UNET_FINIT:
			return "Fatal error occurred on netcore initialization";
		case UNET_OK:
			return "The last operation was succesfull";
		case UNET_MSGRCV:
			return "A new message has been recieved";
		case UNET_NEWCONN:
			return "A new connection has been stablished";
		case UNET_TIMEOUT:
			return "Connection timeout";
		case UNET_TERMINATED:
			return "Connection terminated by peer";
		case UNET_FLOOD:
			return "The Peer is flooding the netcore";
		default:
			return "Undefined, unexpected, unknown and unwanted error code";
	}
}

/**
	Gets the next message from the queue
	remember to destroy the buffer after parsing it!
*/

int plNetGetMsg(st_unet * net,int sid,Byte ** msg) {

	int size;

	st_unet_rcvmsg * ite=NULL;
	st_unet_rcvmsg * prev=NULL;
	ite=net->s[sid].rcvmsg;
	prev=ite;

	while(ite!=NULL && ite->completed!=1) {
		prev=ite;
		ite=(st_unet_rcvmsg *)ite->next;
	}

	if(ite!=NULL) {

		*msg=ite->buf;
		size=ite->size;
		#if _DBG_LEVEL_ > 3
		DBG(4,"message size is %i\n",size);
		if(size<=0) {
			DBG(5,"abort condition\n");
			abort();
		}
		#endif

		if(ite==net->s[sid].rcvmsg) {
			net->s[sid].rcvmsg=(st_unet_rcvmsg *)(ite->next);
			free((void *)ite);
		} else {
			prev->next=ite->next;
			free((void *)ite);
		}

	} else {
		*msg=NULL;
		size=0;
		DBG(6,"ite is NULL? (sid:%i)\n",sid);
		#if _DBG_LEVEL_ > 3
		dumpBuffers(net,0x01);
		//abort();
		#endif
	}
	return size;
}

/**
	Sends a single message to a single host. (Unicast)
		flags are, the same as plNetSend
	0x00 - normal non ack flag packet
	0x02 - request for the ack flag
	0x40 - it's a negotiation
	0x80 - dump the packet to the buffer
	0x20 - force validation 0
*/
int plNetSendMsg(st_unet * net,Byte * msg,int size,int sid,Byte flags) {
	st_uru_client * u=&net->s[sid];
	int n,off=0;
	//char flags=0;
	Byte * buf=NULL;

	//set avg header size
	#define AVG_HEADER_SIZE 500

	//determine if we need to set the ack flag
	if(u->hmsg.flags & plNetAck) {
		flags |=0x02;
	} else {
		flags |=0x00;
	}

	//temporany fix V1
	//if(u->validation==1) { flags |=0x20; }
	//end temporany fix for V1

	DBG(5,"I'm going to send a message of %i bytes...\n",size);
	#if _DBG_LEVEL_ > 2
		if(size<0) {
			DBG(4,"Message size of %i bytes is impossible, abort condition\n",size);
			abort();
		}
	#endif

	buf=(Byte *)malloc(sizeof(Byte) * (size+AVG_HEADER_SIZE));
	if(buf==NULL) return -1;

#if _DBG_LEVEL_ > 2
	print2log(net->log,"\n");
	dump_packet(net->log,buf-0x20,off+0x80,0,7);
	print2log(net->log,"\n-------------\n");
#endif

	off=put_plNetMsg_header(net,buf,size+AVG_HEADER_SIZE,sid);
	//udpate the header flags with the correct size and all other data
	//uru_put_header(buf,&u->server); <-- not yet..

#if _DBG_LEVEL_ > 2
	print2log(net->log,"\n");
	dump_packet(net->log,buf-0x20,off+0x80,0,7);
	print2log(net->log,"\n-------------\n");
#endif

	if(size>0) {
		memcpy(buf+off,msg,size);
		off+=size;
	}

	nlog(net->log,net,sid,"<SND %s %i>\n",unet_get_msg_code(u->hmsg.cmd),off);

#if _DBG_LEVEL_ > 2
	print2log(net->log,"\n");
	dump_packet(net->log,buf-0x20,off+0x80,0,7);
	print2log(net->log,"\n-------------\n");
#endif

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);
	DBG(5,"before plNetSend %i bytes flags: %i, peer: %i...\n",off,flags,sid);
	n=plNetSend(net, buf,off,sid,flags);
	DBG(5,"after plNetSend...\n");
	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	DBG(5,"allocated struct has %i bytes...\n",size+AVG_HEADER_SIZE);
	DBG(5,"buffer contents:\n");
#if _DBG_LEVEL_ > 2
	print2log(net->log,"\n");
	dump_packet(net->log,buf-0x20,off+0x80,0,7);
	print2log(net->log,"\n-------------\n");
#endif
	DBG(5,"before a free...\n");
	if(buf!=NULL) free((void *)buf);
	DBG(5,"after a free...\n");

	return n;
}


void dumpHmsg(st_log * f,st_unet_hmsg * h) {
	f->print("hmsg->cmd:%i\n",h->cmd);
	print2log(f,"hmsg->flags:0x%08X\n",h->flags);
	print2log(f,"hmsg->max_version:%i\n",h->max_version);
	print2log(f,"hmsg->min_version:%i\n",h->min_version);
	print2log(f,"hmsg->timestamp:%s\n",get_stime(h->stamp,h->micros));
	print2log(f,"hmsg->x:%i\n",h->x);
	print2log(f,"hmsg->ki:%i\n",h->ki);
	print2log(f,"hmsg->guid:%s\n",get_guid(h->guid));
	print2log(f,"hmsg->ip:%i %s\n",h->ip,get_ip(htonl(h->ip)));
	print2log(f,"hmsg->port:%i\n",h->port);
}

void dumpSessionBuffers(st_log * f,st_unet * net,int sid) {
	st_uru_client * s = &net->s[sid];

	print2log(f,"sid:%i\n",sid);
	print2log(f,"s->flag:%i\n",s->flag);
	print2log(f,"s->sid:%i\n",s->sid);
	print2log(f,"s->client:");
	uru_print_header(f,&s->client);
	lognl(f);
	print2log(f,"s->server:");
	uru_print_header(f,&s->server);
	lognl(f);
	print2log(f,"s->sock_array.sin_family:%02X\n",((struct sockaddr_in *)s->sock_array)->sin_family);
	print2log(f,"s->sock_array.sin_port:%02X (%i)\n",((struct sockaddr_in*)s->sock_array)->sin_port,htons(((struct sockaddr_in*)s->sock_array)->sin_port));
	print2log(f,"s->sock_array.sin_addr:%s\n",get_ip(((struct sockaddr_in*)s->sock_array)->sin_addr.s_addr));
	print2log(f,"s->a_client_size:%i\n",s->a_client_size);
	print2log(f,"s->old_p_n:%i\n",s->old_p_n);
	print2log(f,"s->whoami:%i\n",s->whoami);
	print2log(f,"s->validation:%i\n",s->validation);
	print2log(f,"s->authenticated:%i\n",s->authenticated);
	print2log(f,"s->validated:%i\n",s->validated);
	print2log(f,"s->max_version:%i\n",s->max_version);
	print2log(f,"s->min_version:%i\n",s->min_version);
	print2log(f,"s->ip:%i %s\n",s->ip,get_ip(s->ip));
	print2log(f,"s->port:%i %i\n",s->port,ntohs(s->port));
	print2log(f,"s->timeout:%i\n",s->timeout);
	print2log(f,"s->timestamp:%s\n",get_stime(s->timestamp,s->microseconds));
	print2log(f,"s->ack_stamp:%s\n",get_stime(s->ack_stamp,s->ack_micros));
	print2log(f,"s->nego_stamp:%s\n",get_stime(s->nego_stamp,s->nego_micros));
	print2log(f,"s->renego_stamp:%s\n",get_stime(s->renego_stamp,s->renego_micros));
	dumpHmsg(f,&s->hmsg);
	print2log(f,"s->name:%s\n",s->name);
	print2log(f,"s->acct:%s\n",s->acct);
	print2log(f,"s->uid:%s\n",s->uid);
//	print2log(f,"s->guid:%s\n",get_guid((Byte *)s->guid));
	print2log(f,"s->passwd:%s\n",s->passwd);
	print2log(f,"s->ki:%i\n",s->ki);
	print2log(f,"s->reason:%i\n",s->reason);
	print2log(f,"s->release:%i\n",s->release);
	print2log(f,"s->bandwidth:%i\n",s->bandwidth);
	print2log(f,"s->window:%i\n",s->window);
	print2log(f,"s->last_check:%s\n",get_stime(s->last_check,0));
	print2log(f,"s->npkts:%i\n",s->npkts);
	print2log(f,"s->sucess:%i\n",s->success);
	print2log(f,"s->vpos:%i\n",s->vpos);
	int i=0;
	st_unet_rcvmsg * rcv;
	rcv=s->rcvmsg;
	print2log(f,"s->rcvmsg:0x%08X\n",(int)s->rcvmsg);
	print2log(f,"s->sndmsg:0x%08X\n",(int)s->sndmsg);
	print2log(f,"RCV msg buffer\n");
	while(rcv!=NULL) {
		print2log(f,"rcvmsg %i at 0x%08X ----\n",i,(int)rcv);
		dumpbuf(f,(Byte *)rcv,sizeof(*rcv));
		lognl(f);
		print2log(f,"rcvmsg->size:%i\n",rcv->size);
		print2log(f,"rcvmsg->buf:\n");
		if(rcv->buf!=NULL) {
			dumpbuf(f,rcv->buf,rcv->size);
		} else {
			print2log(f,"null");
		}
		lognl(f);
		print2log(f,"rcvmsg->sn:%i\n",rcv->sn);
		print2log(f,"rcvmsg->stamp:%s\n",get_stime(rcv->stamp,0));
		print2log(f,"rcvmsg->check:\n");
		dumpbuf(f,(Byte *)rcv->check,sizeof(rcv->check));
		lognl(f);
		print2log(f,"rcvmsg->next:0x%08X\n",(int)rcv->next);
		print2log(f,"rcvmsg->fr_count:%i\n",rcv->fr_count);
		print2log(f,"rcvmsg->completed:%i\n",rcv->completed);
		rcv=(st_unet_rcvmsg *)rcv->next;
		i++;
	}

	i=0;
	st_unet_sndmsg * snd;
	snd=s->sndmsg;
	print2log(f,"SND msg buffer\n");
	while(snd!=NULL) {
		print2log(f,"sndmsg %i at 0x%08X ----\n",i,(int)snd);
		dumpbuf(f,(Byte *)snd,sizeof(*snd));
		lognl(f);
		print2log(f,"sndmsg->size:%i\n",snd->size);
		print2log(f,"sndmsg->buf:\n");
		if(snd->buf!=NULL) {
			dumpbuf(f,snd->buf,snd->size);
		} else {
			print2log(f,"null");
		}
		lognl(f);
		print2log(f,"sndmsg->tryes:%i\n",snd->tryes);
		print2log(f,"sndmsg->next:0x%08X\n",(int)snd->next);
		snd=(st_unet_sndmsg *)snd->next;
		i++;
	}

}

char net_check_address(st_unet * net,int sid) {
	if(sid<0 || sid>=(int)net->n) {
		plog(net->err,"WAR: Address out of range %i out of %i\n",sid,net->n);
		//abort();
		return -1;
	}
	return 0;
}

char plNetIsFini(st_unet * net,int sid) {
	if(net_check_address(net,sid)!=0) { return 1; }
	if(net->s[sid].sndmsg==NULL) { return 1; }
	return 0;
}

int plNetClientSearchByIp(st_unet * net,U32 ip,U16 port) {
	int i;
	for(i=0; i<(int)net->n; i++) {
		if(net->s[i].ip==htonl(ip) && net->s[i].port==htons(port)) {
			return i;
		}
	}
	return -1;
}


int plNetClientSearchByKI(st_unet * net,U32 ki) {
	int i;
	DBG(5,"plNetclientSearchByKi %i\n",ki);
	for(i=0; i<(int)net->n; i++) {
		DBG(5,"[%i] %i=%i?\n",i,ki,net->s[i].ki);
		if((int)net->s[i].ki==(int)ki) {
			return i;
		}
	}
	return -1;
}


/**
	Gets a valid sid, for an specific server service
*/
int plNetServerSearch(st_unet * net,Byte type) {

	int sid=-1;

	switch(type) {
		case KAuth:
			sid=net->auth;
			break;
		case KVault:
			sid=net->vault;
			break;
		case KTracking:
			sid=net->tracking;
			break;
		case KMeta:
			sid=net->meta;
			break;
		default:
			return -1;
	}

	if(net_check_address(net,sid)!=0) return -1;

	if(net->s[sid].whoami!=type) {
		return -1;
	}

	return sid;
}

/**
	You must be cleanning dead/kicked peer by this way to avoid nasty problems
*/
void plNetEndConnection(st_unet * net,int sid) {
	if(net_check_address(net,sid)!=0) return;
	nlog(net->sec,net,sid,"Connection to peer ended...\n");
	logflush(net->sec);
	net->s[sid].timeout=5; //set 5 timeout seconds, and then sayonara
	net->s[sid].authenticated=0;
	net->s[sid].whoami=0;
	net->s[sid].bussy=0;
	net->s[sid].ki=0;
	net->s[sid].status=0;
}

#endif
