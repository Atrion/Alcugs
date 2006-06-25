/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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

//Program vars
#define IN_ALC_PROGRAM
#define ALC_PROGRAM_ID "$Id$"
#define ALC_PROGRAM_NAME "UruTestingSuit"

#include<alcugs.h>
#include<urunet/unet.h>


#include<alcdebug.h>

using namespace alc;

const char * alc::alcNetName="Client";
Byte alc::alcWhoami=KClient;

void parameters_usage() {
	printf(alcVersionText());
	printf("Usage: urumsgtest peer:port [options]\n\n\
 -f x: Set the file to upload\n\
 -val x: set validation level (0-3) (default 2)\n\
 -nl: enable netcore logs\n\
 -V: show version and end\n\
 -h: Show short help and end\n\
 -v x: Set the verbose level\n\
 -lp x: Set the local port\n\
 -rp x: Set the remote port\n\
 -lh x: Set the local host\n\
 -rh x: Set the remote host\n\
 -lm: Set listenning mode\n\
 -u:   Set Urgent flag.\n\n");
}

class tmData :public tmMsgBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmData(tNetSession * u=NULL);
	//format
	tMBuf data;
};

tmData::tmData(tNetSession * u)
 :tmMsgBase(0x1313,plNetTimestamp | plNetAck,u) {}
void tmData::store(tBBuf &t) {
	tmMsgBase::store(t);
	data.clear();
	t.get(data);
}
int tmData::stream(tBBuf &t) {
	int off;
	off=tmMsgBase::stream(t);
	t.put(data);
	off+=data.size();
	return off;
}



class tUnetSimpleFileServer :public tUnetBase {
public:
	tUnetSimpleFileServer(char * lhost=NULL,U16 lport=0,Byte listen=0);
	virtual ~tUnetSimpleFileServer();
	virtual int onMsgRecieved(tNetEvent * ev,tUnetMsg * msg,tNetSession * u);
	virtual void onConnectionFlood(tNetEvent * ev,tNetSession *u) {
		ev->Veto();
	}
	virtual void onIdle(bool idle);
	virtual void onStart();
	void setDestinationAddress(char * d,U16 port);
	void setValidation(Byte val);
	void setUrgent() {
		urgent=true;
	}
	void setFile(char * file) {
		strcpy(this->file,file);
	}
private:
	Byte listen;
	tLog * out;
	char * d_host;
	U16 d_port;
	Byte validation;
	bool urgent;
	char file[500];
	bool sent;
	tNetSessionIte dstite;
};

tUnetSimpleFileServer::tUnetSimpleFileServer(char * lhost,U16 lport,Byte listen) :tUnetBase() {
	this->setBindPort(lport);
	this->setBindAddress(lhost);
	this->listen=listen;
	out=lstd;
	setTimer(1);
	updatetimer(1000);
	d_host=NULL;
	d_port=5000;
	validation=2;
	urgent=false;
	sent=false;
	dstite.ip=0;
	dstite.port=0;
	dstite.sid=-1;
}
tUnetSimpleFileServer::~tUnetSimpleFileServer() {

}

void tUnetSimpleFileServer::setDestinationAddress(char * d,U16 port) {
	d_host=d;
	d_port=port;
}
void tUnetSimpleFileServer::setValidation(Byte val) {
	validation=val;
}

void tUnetSimpleFileServer::onStart() {
	if(listen==0) {
		dstite=netConnect(d_host,d_port,validation,0);
	}
}

void tUnetSimpleFileServer::onIdle(bool idle) {
	if(listen==0) {
		if(!sent) {
			tmData data;
			tNetSession * u=NULL;
			u=getSession(dstite);
			if(u==NULL) {
				stop();
				return;
			}
			if(!u->isConnected()) return;
			data.setDestination(u);
			if(urgent) data.setUrgent();
			tFBuf f1;
			f1.open(file);
			data.data.clear();
			data.data.put(f1);
			f1.close();
			u->send(data);
			sent=true;
		} else {
			if(idle) {
				stop();
			}
		}
	}
}

int tUnetSimpleFileServer::onMsgRecieved(tNetEvent * ev,tUnetMsg * msg,tNetSession * u) {
	int ret=0;

	tmData data;

	switch(msg->cmd) {
		case 0x1313:
			if(listen!=0) {
				tmData data;
				msg->data->get(data);
				tFBuf f1;
				f1.open("rcvmsg.raw","wb");
				f1.put(data.data);
				f1.close();
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

tUnetSimpleFileServer * netcore=NULL;
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
	
	char file[500];
	memset(file,0,500);
	
	Byte val=2; //validation level
	
	//options
	Byte listen=0,nlogs=0,urgent=0;

	//parse parameters
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			printf(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-f") && argc>i+1) { i++; strcpy(file,argv[i]); }
		else if(!strcmp(argv[i],"-lm")) { listen=1; }
		else if(!strcmp(argv[i],"-nl")) { nlogs=1; }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-u")) { urgent=1; }
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
		
		lstd->print(alcVersionText());

		netcore=new tUnetSimpleFileServer(l_hostname,l_port,listen);
		
		netcore->setFlags(UNET_LQUIET);
		if(nlogs) {
			netcore->setFlags(UNET_ELOG | UNET_FLOG);
		} else {
			netcore->unsetFlags(UNET_ELOG | UNET_FLOG);
		}
		if(loglevel!=0) netcore->setFlags(UNET_ELOG);

		while(listen==0 && !strcmp(hostname,"")) {
			printf("\nHostname not set, please enter destination host: ");
			strcpy(hostname,alcConsoleAsk());
		}

		if(listen==0 && !strcmp(file,"")) {
			printf("No input file specified!\n");
			exit(0);
		}
		
		if(listen==0) {
			printf("Connecting to %s#%s@%s:%i...\n",username,avie,hostname,port);
			printf("Sending file...\n");
		} else {
			printf("Waiting for messages... CTR+C stops\n");
		}

		//alcSignal(SIGTERM, s_handler);
		//alcSignal(SIGINT, s_handler);

		netcore->setValidation(val);
		netcore->setDestinationAddress(hostname,port);
		if(urgent==1) netcore->setUrgent();
		netcore->setFile(file);
		
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
