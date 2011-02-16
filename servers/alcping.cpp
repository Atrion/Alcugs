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

//#define _DBG_LEVEL_ 10

#include <alcnet.h>
#include <cstring>
#include <alcdebug.h>

using namespace alc;

void parameters_usage() {
	puts(alcVersionText());
	printf("Usage: uruping server:port [options]\n\n\
 -val x: set validation level (0-3) (default 2)\n\
 -nl: enable netcore log files\n\
 -t x: set the sleep time in seconds\n\
 -f x: Flooding multiplier\n\
 -n x: set the number of probes, 0=infinite (CTR+C stops), 5 if not set\n\
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
 -u:   Set Urgent flag.\n\
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
	tUnetPing(const tString & lhost,U16 lport=0,Byte listen=0,double time=1,int num=5,int flood=1);
	virtual ~tUnetPing();
	virtual int onMsgRecieved(tUnetMsg * msg,tNetSession * u);
	virtual bool onConnectionFlood(tNetSession */*u*/) {
		return false; // don't kick nobody
	}
	virtual void onLeave(Byte reason,tNetSession * u);
	virtual void onIdle(bool idle);
	virtual void onStop();
	void setSource(Byte s);
	void setDestination(Byte d);
	void setDestinationAddress(const tString &d,U16 port);
	void setValidation(Byte val);
	void setUrgent() {
		urgent=true;
	}
private:
	Byte listen;
	double time;
	int num;
	int flood;
	Byte destination;
	tString d_host;
	U16 d_port;
	Byte validation;
	int count;
	tNetSessionIte dstite;
	double current;
	double startup;
	double rcv;
	double min,max,avg;
	int rcvn;
	bool urgent;
};

tUnetPing::tUnetPing(const tString &lhost,U16 lport,Byte listen,double time,int num,int flood) :tUnetBase(KClient) {
	this->setBindPort(lport);
	this->setBindAddress(lhost);
	this->listen=listen;
	setIdleTimer(1);
	this->time=time;
	this->num=num;
	this->flood=flood;
	destination=KLobby;
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
	urgent=false;
}
tUnetPing::~tUnetPing() {

}

void tUnetPing::setSource(Byte s) {
	whoami=s;
}
void tUnetPing::setDestination(Byte d) {
	destination=d;
}
void tUnetPing::setDestinationAddress(const tString &d,U16 port) {
	d_host=d;
	d_port=port;
}
void tUnetPing::setValidation(Byte val) {
	validation=val;
}

void tUnetPing::onStop() {
	if(listen==0 && count > 1) {
		count=flood*count;
		printf("\nStats:\nrecieved %i packets of %i sent, %i%% packet loss, time: %0.3f ms\n",\
		rcvn,count,(100-((rcvn*100)/count)),(current-startup)*1000);
		printf("min/avg/max times = %0.3f/%0.3f/%0.3f\n",min*1000,(avg/rcvn)*1000,max*1000);
	}
}

int tUnetPing::onMsgRecieved(tUnetMsg * msg,tNetSession * u) {
	int ret=0;

	switch(msg->cmd) {
		case NetMsgPing:
		{
			tmPing ping(u);
			msg->data.get(ping);
			log->log("<RCV> [%d] %s\n", msg->sn, ping.str().c_str());
			if(listen==0) {
				if(dstite==u) {
					current=alcGetCurrentTime();
					rcv=current-ping.mtime;
					printf("Pong from %s:%i x=%i dest=%i %s time=%0.3f ms\n",\
						alcGetStrIp(u->getIp()).c_str(),ntohs(u->getPort()),ping.x,ping.destination,\
						alcUnetGetDestination(ping.destination),rcv*1000);
					rcvn++;
					avg+=rcv;
					if(rcv<min) min=rcv;
					if(rcv>max) max=rcv;
				}
			} else {
				printf("Ping from %s:%i x=%i dest=%i %s time=%0.3f ms .... pong....\n",\
					alcGetStrIp(u->getIp()).c_str(),ntohs(u->getPort()),ping.x,ping.destination,\
					alcUnetGetDestination(ping.destination),ping.mtime*1000);
				if(urgent) ping.setUrgent();
				send(ping);
			}
			ret=1;
			break;
		}
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

void tUnetPing::onLeave(Byte reason,tNetSession * u)
{
	if(listen!=0) {
		printf("Leave from %s:%i reason=%i %s\n", alcGetStrIp(u->getIp()).c_str(), ntohs(u->getPort()), reason, alcUnetGetReasonCode(reason));
	}
}

void tUnetPing::onIdle(bool /*idle*/) {
	int i;
	if(listen==0) {

		updateTimerRelative(100);
		if(count==0) {
			dstite=netConnect(d_host.c_str(),d_port,validation,0);
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
				tmPing ping(u, destination);
				if(urgent) ping.setUrgent();

				count++;
				for(i=0; i<flood; i++) {
					ping.x = (flood * (count-1)) + i;
					ping.mtime = current = alcGetCurrentTime();
					send(ping);
				}
			} else if((rcv-current) > (4*time) || (u && (rcv-current) > ((u->getRTT()+(u->getRTT()/2))/1000000))) {
				stop();
			}
		}
	}
}


int main(int argc,char * argv[]) {

	int i;
	
	Byte loglevel=2;
	//local settings
	tString l_hostname="0.0.0.0";
	U16 l_port=0;
	//remote settings
	tString hostname="";
	U16 port=5000;
	
	Byte val=2; //validation level
	
	double time=1; //time
	Byte destination=KLobby,source=0;
#ifdef ENABLE_ADMIN
	Byte admin=0;
#endif
	
	//options
	int num=5,flood=1; //num probes & flood multiplier
	Byte bcast=0,listen=0,mrtg=0,nlogs=0,urgent=0;
	
	//parse parameters
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			puts(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-b")) { bcast=1; }
		else if(!strcmp(argv[i],"-lm")) { listen=1; }
#ifdef ENABLE_ADMIN
		else if(!strcmp(argv[i],"-i_know_what_i_am_doing")) { admin=1; }
#endif
		else if(!strcmp(argv[i],"-nl")) { nlogs=1; }
		else if(!strcmp(argv[i],"-t") && argc>i+1) { i++; time=atof(argv[i]); }
		else if(!strcmp(argv[i],"-n") && argc>i+1) { i++; num=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-d") && argc>i+1) { i++; destination=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-s") && argc>i+1) { i++; source=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-f") && argc>i+1) { i++; flood=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-one")) { mrtg=1; }
		else if(!strcmp(argv[i],"-u")) { urgent=1; }
		else if(!strcmp(argv[i],"-v") && argc>i+1) { i++; loglevel=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-lh") && argc>i+1) {
			i++;
			l_hostname = argv[i];
		}
		else if(!strcmp(argv[i],"-l")) {
			puts(alcVersionTextShort());
			puts(alcLicenseText());
			return 0;
		}
		else if(!strcmp(argv[i],"-rh") && argc>i+1) {
			i++;
			hostname = argv[i];
		}
		else {
			if(i==1) {
				if(!alcGetLoginInfo(argv[1],NULL,&hostname,&port)) {
					parameters_usage();
					return -1;
				}
			} else {
				parameters_usage();
				return -1;
			}
		}
	}
	
	//start Alcugs library
	tAlcUnetMain alcMain("Client");
	try {
		alcMain.config()->setVar(tString::fromByte(nlogs), "log.enabled", "global");
		alcMain.config()->setVar(tString::fromByte(loglevel), "verbose_level", "global");
		alcMain.onApplyConfig();
		
		//special mode
		if(mrtg==0) {
			alcGetMain()->std()->print(alcVersionText());
		}
		
		if(flood<=0) flood=1;
		
#ifdef ENABLE_ADMIN
		if(time<0.2 && admin==0) {
			printf("\nOnly the administrator can set less than 0.2 seconds\n Setting up to 1 second. (enable admin mode with -i_know_what_i_am_doing)\n");
			time=1;
		}
		if(flood>1 && admin==0) {
			printf("\nOnly the administrator can perform stressing flood tests to the server.\n Disabling flooding. (enable admin mode with -i_know_what_i_am_doing)\n");
			flood=1;
		}
#else
		if(time<0.2) {
			printf("\nTime must be bigger than 0.2 seconds\n Setting up to 1 second.\n");
			time=1;
		}
		if(flood>1) {
			printf("\nCannot perform stressing flood tests to the server.\n Disabling flooding\n");
			flood=1;
		}
#endif
		
		if(mrtg==1) num=1;

		tUnetPing netcore(l_hostname,l_port,listen,time,num,flood);
		if (!nlogs) netcore.unsetFlags(UNET_ELOG);
		
		if(bcast) netcore.setFlags(UNET_BCAST);
		else netcore.unsetFlags(UNET_BCAST);

		while(listen==0 && hostname.isEmpty()) {
			printf("\nHostname not set, please enter destination host: ");
			hostname = alcConsoleAsk();
		}

		if(listen==0 && mrtg==0) {
			printf("Connecting to %s:%i...\n",hostname.c_str(),port);
			printf("Sending ping probe to %i %s...\n",destination,alcUnetGetDestination(destination));
		}
		if(listen!=0) {
			printf("Waiting for messages... CTR+C stops\n");
		}

		//alcSignal(SIGTERM, s_handler);
		//alcSignal(SIGINT, s_handler);
		
		netcore.setSource(source);
		netcore.setDestination(destination);
		netcore.setDestinationAddress(hostname,port);
		netcore.setValidation(val);
		if(urgent==1) netcore.setUrgent();
		
		netcore.run();
		
	} catch(txBase &t) {
		printf("Exception %s\n%s\n",t.what(),t.backtrace());
		return -1;
	} catch(...) {
		printf("Unknown Exception\n");
		return -1;
	}
	
	return 0;
}

