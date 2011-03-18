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

//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include <unetbase.h>
#include <unetmain.h>
#include <alcversion.h>
#include <alclicense.h>
#include <netsessionmgr.h>
#include <protocol/umsgbasic.h>
#include <netexception.h>

#include <cstring>
#include <limits>

using namespace alc;

void parameters_usage() {
	puts(alcVersionText());
	printf("Usage: alcping server:port [options]\n\n\
 -val x: set validation level (0-3) (default 2)\n\
 -nl: enable netcore log files\n\
 -t x: set the sleep time in seconds\n\
 -f x: Flooding multiplier\n\
 -n x: set the number of probes, 0=infinite (CTR+C stops), 5 if not set\n\
 -V: show version and end\n\
 -h: Show short help and end\n\
 -v x: Set the verbose level\n\
 -lp x: Set the local port (enables listen mode)\n\
 -rp x: Set the remote port\n\
 -lh x: Set the local host (enables listen mode)\n\
 -rh x: Set the remote host\n\
 -one: Does only one ping probe and displays that value\n\
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

class tUnetPing : public tUnetBase {
public:
	tUnetPing(const tString & lhost,uint16_t lport,bool listen=false,double time=1,int num=5,int flood=1,uint8_t source=KClient);
	virtual int onMsgRecieved(tUnetMsg * msg,tNetSession * u);
	virtual void onIdle();
	virtual void onStop();
	void setDestination(uint8_t d);
	void setDestinationAddress(const tString &d,uint16_t port);
	void setValidation(uint8_t val);
	void setUrgent() {
		urgent=true;
	}
private:
	bool listen;
	double time;
	int num; //!< total number of pings to be sent
	int flood;
	uint8_t destination;
	tString d_host;
	uint16_t d_port;
	uint8_t validation;
	int count; //!< number of pings sent so far
	tNetSessionRef dstSession;
	tNetTime lastSend;
	double min,max,avg;
	int rcvn;
	bool urgent;
	
	tMutex mutex; //!< for all the counting and stuff
};

tUnetPing::tUnetPing(const tString &lhost,uint16_t lport,bool listen,double time,int num,int flood,uint8_t source) :tUnetBase(source) {
	this->setBindPort(lport);
	this->setBindAddress(lhost);
	this->listen=listen;
	this->unsetFlags(UNET_FLOODCTR); // disable flood protection
	if (!listen) max_sleep = 0.1; // set down max_sleep timer to send pings in one-second intervals
	this->time=time;
	this->num=num;
	this->flood=flood;
	destination=KLobby;
	d_port=5000;
	count=0;
	validation=2;
	min=std::numeric_limits<double>::max();
	max=0;
	avg=0;
	rcvn=0;
	urgent=false;
}

void tUnetPing::setDestination(uint8_t d) {
	destination=d;
}
void tUnetPing::setDestinationAddress(const tString &d,uint16_t port) {
	d_host=d;
	d_port=port;
}
void tUnetPing::setValidation(uint8_t val) {
	validation=val;
}

void tUnetPing::onStop() {
	if(listen==0 && count > 1) {
		count=flood*count;
		printf("\nStats:\nrecieved %i packets of %i sent, %i%% packet loss\n",
				rcvn,count,(100-((rcvn*100)/count)));
		printf("min/avg/max times = %0.3f/%0.3f/%0.3f\n",min*1000,(avg/rcvn)*1000,max*1000);
	}
}

int tUnetPing::onMsgRecieved(tUnetMsg * msg,tNetSession * u) {

	switch(msg->cmd) {
		case NetMsgPing:
		{
			tmPing ping(u, msg);
			tMutexLock lock(mutex);
			if(listen==0) {
				if(*dstSession==u) {
					double rtt=getNetTime()-ping.mtime;
					printf("Pong from %s:%i x=%i dest=%i %s time=%0.3f ms\n",
						alcGetStrIp(u->getIp()).c_str(),ntohs(u->getPort()),ping.x,ping.destination,
						alcUnetGetDestination(ping.destination),rtt*1000);
					rcvn++;
					avg+=rtt;
					if(rtt<min) min=rtt;
					if(rtt>max) max=rtt;
				}
			} else {
				printf("Ping from %s:%i x=%i dest=%i %s time=%0.3f ms .... pong....\n",
					alcGetStrIp(u->getIp()).c_str(),ntohs(u->getPort()),ping.x,ping.destination,
					alcUnetGetDestination(ping.destination),ping.mtime*1000);
				ping.urgent = urgent;
				send(ping);
			}
			return 1;
		}
		default:
			return !listen;
	}
}

void tUnetPing::onIdle() {
	if(listen) return;
	DBG(9, "Sender is idle\n");

	tMutexLock lock(mutex);
	if (*dstSession == NULL) {
		DBG(5, "Establishing connection\n");
		dstSession=netConnect(d_host.c_str(),d_port,validation);
		return;
	}

	if(dstSession->isTerminated()) {
		DBG(5, "Lost peer, going down\n");
		stop();
		return;
	}
	
	if (num != 0 && count >= num) {
		if (!dstSession->anythingToSend()) {
			DBG(5, "Done sending, going down\n");
			stop();
		}
		return;
	}

	if (timeOverdue(lastSend+time)) {
		DBG(5, "Time to send again\n");
		//snd ping message
		tmPing ping(*dstSession, destination);
		ping.urgent = urgent;

		count++;
		lastSend = getNetTime();
		for(int i=0; i<flood; i++) {
			ping.x = (flood * (count-1)) + i;
			ping.mtime = lastSend;
			send(ping);
		}
	}
}


int main(int argc,char * argv[]) {
	
	uint8_t loglevel=2;
	//local settings
	tString l_hostname="0.0.0.0";
	uint16_t l_port=0;
	//remote settings
	tString hostname="";
	uint16_t port=5000;
	
	uint8_t val=2; //validation level
	
	double time=1; //time
	uint8_t destination=KLobby,source=0;
#ifdef ENABLE_ADMIN
	uint8_t admin=0;
#endif
	
	//options
	int num=5,flood=1; //num probes & flood multiplier
	bool listen=false,mrtg=false,nlogs=false,urgent=false;
	
	//start Alcugs library (before parameters, for the license text!)
	tAlcUnetMain alcMain("Client");
	
	//parse parameters
	for (int i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			puts(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; listen=true; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
#ifdef ENABLE_ADMIN
		else if(!strcmp(argv[i],"-i_know_what_i_am_doing")) { admin=true; }
#endif
		else if(!strcmp(argv[i],"-nl")) { nlogs=true; }
		else if(!strcmp(argv[i],"-t") && argc>i+1) { i++; time=atof(argv[i]); }
		else if(!strcmp(argv[i],"-n") && argc>i+1) { i++; num=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-d") && argc>i+1) { i++; destination=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-s") && argc>i+1) { i++; source=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-f") && argc>i+1) { i++; flood=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-one")) { mrtg=true; }
		else if(!strcmp(argv[i],"-u")) { urgent=true; }
		else if(!strcmp(argv[i],"-v") && argc>i+1) { i++; loglevel=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-lh") && argc>i+1) {
			i++;
			listen=true;
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
	
	try {
		alcMain.config()->setVar(tString::fromUInt(nlogs), "log.enabled", "global");
		alcMain.config()->setVar(tString::fromUInt(loglevel), "verbose_level", "global");
		alcMain.applyConfig();
		
		//special mode
		if(!mrtg) {
			alcGetMain()->std()->print(alcVersionText());
		}
		
		if(flood<=0) flood=1;
		
#ifdef ENABLE_ADMIN
		if(time<0.1 && !admin) {
			printf("\nOnly the administrator can set less than 0.1 seconds. Setting up to 0.1 seconds. (enable admin mode with -i_know_what_i_am_doing)\n");
			time=0.1;
		}
		if(flood>1 && !admin) {
			printf("\nOnly the administrator can perform stressing flood tests to the server.\n Disabling flooding. (enable admin mode with -i_know_what_i_am_doing)\n");
			flood=1;
		}
#else
		if(time<0.1) {
			printf("\nTime must be no less than than 0.1 seconds. Setting up to 0.1 seconds.\n");
			time=0.1;
		}
		if(flood>1) {
			printf("\nCannot perform stressing flood tests to the server.\n Disabling flooding\n");
			flood=1;
		}
#endif
		
		if(mrtg) num=1;

		tUnetPing netcore(l_hostname,l_port,listen,time,num,flood,source);
		if (!nlogs) netcore.unsetFlags(UNET_ELOG);

		while(!listen && hostname.isEmpty()) {
			printf("\nHostname not set, please enter destination host: ");
			hostname = alcConsoleAsk();
		}

		if(!listen && !mrtg) {
			printf("Connecting to %s:%i...\n",hostname.c_str(),port);
			printf("Sending ping probe to %i %s...\n",destination,alcUnetGetDestination(destination));
		}
		if(listen) {
			printf("Waiting for messages... CTR+C stops\n");
		}
		
		netcore.setDestination(destination);
		netcore.setDestinationAddress(hostname,port);
		netcore.setValidation(val);
		if(urgent) netcore.setUrgent();
		
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

