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

#define _DBG_LEVEL_ 10

#include<alcugs.h>

//Program vars
const char * alcXID = "$Id$";
const char * alcXBUILD =  __DATE__ " " __TIME__;
const char * alcXSNAME = "UruPing";
const char * alcXVERSION = alcSTR_VER;

#include<urunet/unet.h>


#include<alcdebug.h>

using namespace alc;

void parameters_usage() {
	printf(alcVersionText());
	printf("Usage: uruping server:port [options]\n\n\
 -val x: set validation level (0-3) (default 2)\n\
 -nl: enable netcore logs\n\
 -t x: set the sleep time in seconds\n\
 -f x: Flooding multiplier\n\
 -n x: set the number of proves, 0=infinite (CTR+C stops), 5 if not set\n\
 -V: show version and end\n\
 -h: Show short help and end\n\
 -v x: Set the verbose level\n\
 -lp x: Set the local port\n\
 -rp x: Set the remote port\n\
 -lh x: Set the local host\n\
 -rh x: Set the remote host\n\
 -lm: Set listenning mode\n\
 -one: Does only one ping probe and displays that value\n\
 -b: Pings to the broadcast\n\
 -d x: Set the destination.\n\
 -s x: Set the source.\n\
 Valid source/destination addresses are:\n\
  [1]   Agent\n\
  [2]   Lobby\n\
  [3]   Game\n\
  [4]   Vault\n\
  [5]   Auth\n\
  [6]   Admin\n\
  [7]   Tracking/Lookup\n\
  [8]   Client\n\
  [9]   Meta\n\
  [10]  Test\n\
  [11]  Data\n\
  [255] Broadcast\n\n");
}

class tUnetPing :public tUnetBase {
public:
	tUnetPing(char * lhost=NULL,U16 lport=0,Byte listen=0,double time=1,int num=5,int flood=1);
	virtual ~tUnetPing();
	virtual void onNewConnection(tNetEvent * ev,tNetSession * u);
	virtual int onMsgRecieved(tNetEvent * ev,tUnetMsg * msg,tNetSession * u);
	virtual void onConnectionFlood(tNetEvent * ev,tNetSession *u) {
		ev->Veto();
	}
	virtual void onIdle(bool idle);
	virtual void onStop();
	void setSource(Byte s);
	void setDestination(Byte d);
	void setDestinationAddress(char * d,U16 port);
	void setValidation(Byte val);
private:
	Byte listen;
	tLog * out;
	double time;
	int num;
	int flood;
	Byte destination;
	char * d_host;
	U16 d_port;
	Byte validation;
	int count;
	tNetSessionIte dstite;
	double current;
	double startup;
	double rcv;
	double min,max,avg;
	int rcvn;
};

tUnetPing::tUnetPing(char * lhost,U16 lport,Byte listen,double time,int num,int flood) :tUnetBase(lhost,lport) {
	this->listen=listen;
	out=lstd;
	setTimer(1);
	updatetimer(1000);
	this->time=time;
	this->num=num;
	this->flood=flood;
	destination=KLobby;
	d_host=NULL;
	d_port=5000;
	count=0;
	dstite.ip=0;
	dstite.port=0;
	dstite.sid=-1;
	validation=2;
	min=10000;
	max=0;
	avg=0;
	rcvn=0;
}
tUnetPing::~tUnetPing() {

}

void tUnetPing::setSource(Byte s) {
	whoami=s;
}
void tUnetPing::setDestination(Byte d) {
	destination=d;
}
void tUnetPing::setDestinationAddress(char * d,U16 port) {
	d_host=d;
	d_port=port;
}
void tUnetPing::setValidation(Byte val) {
	validation=val;
}

void tUnetPing::onStop() {
	if(listen==0) {
		count=flood*count;
		out->print("\nStats:\nrecieved %i packets of %i sent, %i%% packet loss, time: %0.3f ms\n",\
		rcvn,count,(100-((rcvn*100)/count)),(current-startup)*1000);
		out->print("min/avg/max times = %0.3f/%0.3f/%0.3f\n",min*1000,(avg/rcvn)*1000,max*1000);
	}
}

void tUnetPing::onNewConnection(tNetEvent * ev,tNetSession * u) {
	if(listen==0) {
		//out->log("%s Connection refused\n",u->str());
		//terminate(ev->sid,true);
	}
}

int tUnetPing::onMsgRecieved(tNetEvent * ev,tUnetMsg * msg,tNetSession * u) {
	int ret=0;

	tmPing ping;

	switch(msg->cmd) {
		case NetMsgPing:
			ping.setSource(u);
			msg->data->get(ping);
			if(listen==0) {
				if(dstite==ev->sid) {
					current=alcGetCurrentTime();
					rcv=current-ping.mtime;
					out->log("Pong from %s:%i x=%i dest=%i %s time=%0.3f ms\n",\
					alcGetStrIp(ev->sid.ip),ntohs(ev->sid.port),ping.x,ping.destination,\
					alcUnetGetDestination(ping.destination),rcv*1000);
					rcvn++;
					avg+=rcv;
					if(rcv<min) min=rcv;
					if(rcv>max) max=rcv;
				}
			} else {
				out->log("Ping from %s:%i x=%i dest=%i %s time=%0.3f ms .... pong....\n",\
				alcGetStrIp(ev->sid.ip),ntohs(ev->sid.port),ping.x,ping.destination,\
				alcUnetGetDestination(ping.destination),ping.mtime*1000);
				ping.setReply();
				u->send(ping);
			}
			ret=1;
			break;
		default:
			if(listen==0) {
				ret=1;
			} else {
				ret=0;
			}
			break;
	}
	return ret;
}

void tUnetPing::onIdle(bool idle) {
	int i;
	if(listen==0) {

		updatetimer(100);
		if(count==0) {
			dstite=netConnect(d_host,d_port,validation,0);
			current=startup=alcGetCurrentTime();
		}

		tNetSession * u=NULL;
		u=getSession(dstite);
		if(u==NULL) {
			stop();
			return;
		}

		rcv=alcGetCurrentTime();
		if((rcv-current)>time || count==0) {
			if(count<num || num==0 || count==0) {
				//snd ping message
				tmPing ping;
				ping.destination=destination;
				ping.setDestination(u);
				ping.setFlags(plNetTimestamp);

				count++;
				for(i=0; i<flood; i++) {
					ping.x = (flood * (count-1)) + i;
					current = alcGetCurrentTime(),
					ping.mtime = current;
					u->send(ping);
				}
			} else if((rcv-current) > (4*time) || (u && (rcv-current) > ((u->getRTT()+(u->getRTT()/2))/1000000))) {
				stop();
			}
		}
	}
}


tUnetPing * netcore=NULL;
Byte __state_running=1;

//handler
void s_handler(int s) {
	lstd->log("INF: Catch up signal %i\n",s);
	if(__state_running==0) {
		lerr->log("killed\n");
		exit(-1);
	}
	__state_running=0;
	netcore->stop(5);
}

int main(int argc,char * argv[]) {

	int i;
	
	Byte loglevel=2;
	//local settings
	char l_hostname[100]="0.0.0.0";
	U16 l_port=0;
	//remote settings
	char hostname[100]="";
	char username[100]="";
	char avie[100]="";
	U16 port=5000;
	
	Byte val=2; //validation level
	
	double time=1; //time
	Byte destination=KLobby,source=0,admin=0;
	
	if (__VTC) admin=1;
	
	//options
	int num=5,flood=1; //num probes & flood multiplier
	Byte bcast=0,listen=0,mrtg=0,nlogs=0;;

	//parse parameters
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			printf(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-b")) { bcast=1; }
		else if(!strcmp(argv[i],"-lm")) { listen=1; }
		else if(!strcmp(argv[i],"-i_know_what_i_am_doing")) { admin=1; }
		else if(!strcmp(argv[i],"-nl")) { nlogs=1; }
		else if(!strcmp(argv[i],"-t") && argc>i+1) { i++; time=atof(argv[i]); }
		else if(!strcmp(argv[i],"-n") && argc>i+1) { i++; num=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-d") && argc>i+1) { i++; destination=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-s") && argc>i+1) { i++; source=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-f") && argc>i+1) { i++; flood=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-one")) { mrtg=1; }
		else if(!strcmp(argv[i],"-v") && argc>i+1) { i++; loglevel=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-lh") && argc>i+1) {
			i++;
			strcpy(l_hostname,argv[i]);
		}
		else if(!strcmp(argv[i],"-l")) {
			printf(alcVersionTextShort());
			printf(alcLicenseText());
			return 0;
		}
		else if(!strcmp(argv[i],"-rh") && argc>i+1) {
			i++;
			strcpy(hostname,argv[i]);
		}
		else {
			if(i==1) {
				if(alcGetLoginInfo(argv[1],hostname,username,((U16 *)&port),avie)!=1) {
					parameters_usage();
					return -1;
				}
			} else {
				parameters_usage();
				return -1;
			}
		}
	}

	try {
	
		//start Alcugs library
		alcInit(argc,argv,nlogs!=1);
		alcLogSetLogLevel(loglevel);
		
		//special mode
		if(mrtg==0) {
			lstd->print(alcVersionText());
		}
		
		if(time<0.2 && (admin==0 || __WTC==0)) {
			if(__WTC) {
				printf("\nOnly the administrator can set less than 0.2 seconds\n Setting up to 1 second. (enable admin mode with -i_know_what_i_am_doing)\n");
			} else {
				printf("\nTime must be bigger than 0.2 seconds\n Setting up to 1 second.\n");
			}
			time=1;
		}
		
		if(flood<=0) flood=1;
		
		if(flood>1 && (admin==0 || __WTC==0)) {
			if(__WTC) {
				printf("\nOnly the administrator can perform stressing flood tests to the server.\n Dissabling flooding.\n");
			} else {
				printf("\nFlood dissabled.\n");
			}
			flood=1;
		}
		
		if(mrtg==1) num=1;

		netcore=new tUnetPing(l_hostname,l_port,listen,time,num,flood);
		
		netcore->setFlags(UNET_LQUIET);
		if(nlogs) {
			netcore->setFlags(UNET_ELOG | UNET_FLOG);
		} else {
			netcore->unsetFlags(UNET_ELOG | UNET_FLOG);
		}
		if(loglevel!=0) netcore->setFlags(UNET_ELOG);
		
		if(bcast) netcore->setFlags(UNET_BCAST);
		else netcore->unsetFlags(UNET_BCAST);

		while(listen==0 && !strcmp(hostname,"")) {
			printf("\nHostname not set, please enter destination host: ");
			strcpy(hostname,alcConsoleAsk());
		}

		if(listen==0 && mrtg==0) {
			printf("Connecting to %s#%s@%s:%i...\n",username,avie,hostname,port);
			printf("Sending ping probe to %i %s...\n",destination,alcUnetGetDestination(destination));
		}

		alcSignal(SIGTERM, s_handler);
		alcSignal(SIGINT, s_handler);
		
		netcore->setSource(source);
		netcore->setDestination(destination);
		netcore->setDestinationAddress(hostname,port);
		netcore->setValidation(val);
		
		netcore->run();
	
		delete netcore;
	
		//stop Alcugs library (optional, not required)
		//alcShutdown();
		
	} catch(txBase &t) {
		printf("Exception %s\n%s\n",t.what(),t.backtrace());
	} catch(...) {
		printf("Unknown Exception\n");
	}
	
	return 0;
}


#if 0
	int num=5,count=0,rcvn=0; //number of probes
	double avg=0,min=10000,max=0; //avg latency time
	int flooding_wk=1;
	
	//time structs - to compute the rtt
	double startup;
	double current;
	double rcv;

	//set up net params here
	if(time<2) { //reconfigure netcore time loop
		net.unet_sec=((int)(time/7));
		net.unet_usec= ((int)((time/7)*1000*1000))%1000000;
		//printf("%i - %i\n",net.unet_sec,net.unet_usec);
		//abort();
	}

	//set startup time
	startup = get_current_time(); //time(NULL); // + (get_microseconds() / 1000000);

	if(listen==0) {

		if(source==-1) {
			net.whoami = KClient;
		} else {
			net.whoami = source;
		}

		if(plNetConnect(&net,&sid,hostname,port,flags)!=UNET_OK || sid<0) {
			fprintf(stderr,"ERR: Failed connection to %s:%i\n",hostname,port);
			return -1;
		}
		isid=sid;

		net.s[sid].whoami = destination;
		//send the initial ping message here
		count++;
		for(i=1; i<=flood; i++) {
			net.s[sid].hmsg.x=(flood * (count-1)) + i;
			current = get_current_time();
			plNetMsgPing(&net,0x00,current,destination,sid,sid);
		}

	} else { //listenning mode
		if(source==-1) {
			net.whoami = KBcast;
		} else {
			net.whoami = source;
		}
		printf("Waiting for messages... CTR+C stops\n");
		fflush(0);
	}

	DBG(3,"__state_running is %i\n",__state_running);
	while(__state_running) { // && (listen!=0 || num==0)) { //rcvn<num ||
		ret=plNetRecv(&net,&sid);
		DBG(2,"plNetRcv event:%i from peer:%i\n",ret,sid);

		if(sid>=0) {

			switch(ret) {
				case UNET_NEWCONN: //this event never happens, but just in case
				case UNET_MSGRCV:
				case UNET_FLOOD:
					DBG(3,"ret:%i peer:%i, new message from %s:%i\n",ret,sid,get_ip(net.s[sid].ip),htons(net.s[sid].port));
					DBG(5,"now I'm going to parse the message...\n");
					size=plNetGetMsg(&net,sid,&msg); //get the message
					DBG(5,"Got a message from sid:%i of %i bytes\n",sid,size);
					//parse the message
					#if _DBG_LEVEL_ > 3
					dumpbuf(f_uru,msg,size);
					lognl(f_uru);
					#endif
					off=parse_plNet_msg(&net,msg,size,sid); //parse the header
					//ret=process_basic_plNetMsg(&net,msg+off,size-off,sid);
					//take post actions
					//if(ret==UNET_OK) {
					if(net.s[sid].hmsg.cmd==NetMsgPing) {
						if(listen==0) {
							current = get_current_time();
							//parse the pong
							rcv=*(double *)(msg+off);
							rcv=current-rcv;
							dst=*(Byte *)(msg+off+8);
							printf("Pong from %s:%i x=%i dest=%i %s time=%0.3f ms\n",\
							get_ip(net.s[sid].ip),ntohs(net.s[sid].port),\
							net.s[sid].hmsg.x,dst,unet_get_destination(dst),rcv*1000);
							rcvn++;
							avg+=rcv; //set average time
							if(rcv<min) min=rcv;
							if(rcv>max) max=rcv;
							if(num!=0 && (int)net.s[sid].hmsg.x==(flood*num)) {
								__state_running=0; //stop the netcore after recieving the last message
							}
						} else {
							if(ret==UNET_FLOOD && flooding_wk==1) {
								printf("Flood from %s:%i\n",get_ip(net.s[sid].ip),htons(net.s[sid].port));
								plNetMsgTerminated(&net,RKickedOff,sid);
								plNetDestroySession(&net,sid);
							} else {
								//parse the ping and send a pong
								rcv=*(double *)(msg+off);
								dst=*(Byte *)(msg+off+8);
								//ret=process_basic_plNetMsg(&net,msg+off,size-off,sid);
								printf("Ping from %s:%i x=%i dest=%i %s time=%0.3f ms .... pong....\n",\
								get_ip(net.s[sid].ip),ntohs(net.s[sid].port),\
								net.s[sid].hmsg.x,dst,unet_get_destination(dst),rcv*1000);
								//send the pong
								plNetMsgPing(&net,0x01,rcv,dst,sid,sid);
							}
						}
					} else if(net.s[sid].hmsg.cmd==NetMsgLeave) {
						if(listen!=0) {
							dst=*(Byte *)(msg+off);
							printf("Leave from %s:%i reason=%i %s\n",\
							get_ip(net.s[sid].ip),ntohs(net.s[sid].port),\
							dst,unet_get_reason_code(dst));
							plNetDestroySession(&net,sid);
						}
					} else if(net.s[sid].hmsg.cmd==NetMsgTerminated) {
						if(listen==0 && flooding_wk==1) {
							dst=*(Byte *)(msg+off);
							printf("Terminated from %s:%i reason=%i %s\n",\
							get_ip(net.s[sid].ip),ntohs(net.s[sid].port),\
							dst,unet_get_reason_code(dst));
							plNetDestroySession(&net,sid);
							__state_running=0;
						}
					}

					//}
					if(msg!=NULL) free((void *)msg); //<- vip
					DBG(5,"the message has been succesfully parsed...\n");
					break;
				case UNET_TIMEOUT:
				case UNET_TERMINATED: //this event is not implemented, yet. It must be handled in the app layer
					DBG(3,"ret:%i peer:%i, connection terminated/timeout from %s:%i\n",ret,sid,get_ip(net.s[sid].ip),htons(net.s[sid].port));
					if(listen!=0) { //Only on listen mode
						printf("Timeout from %s:%i\n",get_ip(net.s[sid].ip),htons(net.s[sid].port));
						//plNetMsgTerminated(&net,RTimedOut,sid);
						plNetDestroySession(&net,sid);
					}
					break;
				default: //ignore the other events
					DBG(4,"ret:%i peer:%i, address:%s:%i, error:%s\n",ret,sid,get_ip(net.s[sid].ip),htons(net.s[sid].port),get_unet_error(ret));
					break;
			}
		} else { //ignore the generic events
			DBG(4,"ret:%i error:%s\n",ret,get_unet_error(ret));
		}

		if(listen==0) {
			DBG(5,"I'm here\n");
			rcv = get_current_time();
			DBG(2,"rcv:%0.3f > %f\n",(rcv-current)*1000,time*1000);
			if((rcv-current) > time) {
				if(count<num || num==0) {
					count++;
					for(i=1; i<=flood; i++) {
						net.s[isid].hmsg.x=(flood * (count-1)) + i;
						current = get_current_time();
						DBG(6,"plNetMsgPing()\n");
						plNetMsgPing(&net,0x00,current,destination,isid,isid);
					}
				} else {
					if((rcv-current) > (4*time)) { //a simple timeout (when all has been already sent)
						__state_running=0; //stop the netcore
					}
				}
			}
		}
	} //while (main loop)

	if(listen==0) {
		plNetMsgLeave(&net,RQuitting,isid);

		rcv = get_current_time();
		while(plNetIsFini(&net,isid)==0) {
			ret=plNetRecv(&net,&sid);
			DBG(2,"plNetRcv event:%i from peer:%i\n",ret,sid);
			current = get_current_time();
			if((current-rcv) > 2) {
				printf("\nThere was a problem disconnecting your session...\n");
				break;
			}
		}

		count=flood*count;
		current = get_current_time();
		printf("\nstats:\nrecieved %i packets of %i sent, %i%% packet loss, time: %0.3f ms\n",rcvn,count,(100-((rcvn*100)/count)),(current-startup)*1000);
		printf("min/avg/max times = %0.3f/%0.3f/%0.3f\n",min*1000,(avg/rcvn)*1000,max*1000);
		if((100-((rcvn*100)/count))<0) {
			printf("\nPinging to a Plasma server?, then set a time < 0.4 seconds.\n");
		}
	} else {
		for(i=0; i<(int)net.n; i++) {
			plNetMsgTerminated(&net,RKickedOff,i);
		}
	}

	plNetStopOp(&net);
	DBG(3,"INF: Service sanely terminated\n");
	log_shutdown();

	return 0;
}

#endif
