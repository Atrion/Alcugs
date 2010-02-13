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
//#define _DBG_LEVEL_ 10

//Program vars
#define IN_ALC_PROGRAM
#define ALC_PROGRAM_ID "$Id$"
#define ALC_PROGRAM_NAME "UruTestingSuite"

#include <alcnet.h>


#include <alcdebug.h>

using namespace alc;

void parameters_usage() {
	puts(alcVersionText());
	printf("Usage: urumsgtest peer:port [options]\n\n\
 -f x: Set the file to upload\n\
 -z: Compress the file\n\
 -val x: set validation level (0-3) (default 2)\n\
 -nl: enable netcore log files\n\
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
	virtual void stream(tBBuf &t) const;
	tmData(tNetSession * u);
	void setCompressed(void) {
		setFlags(plNetX);
		x = data.size();
	}
	//format
	tMBuf data;
};

tmData::tmData(tNetSession * u)
 :tmMsgBase(NetMsgCustomTest,plNetTimestamp | plNetAck,u) { }
void tmData::store(tBBuf &t) {
	tmMsgBase::store(t);
	data.clear();
	U32 remaining = t.remaining();
	if (hasFlags(plNetX)) {
		tZBuf zdata;
		zdata.write(t.read(), remaining);
		zdata.uncompress(x);
		data = zdata;
		if (data.size() != x) throw txBase(_WHERE("size mismatch (%d != %d)", data.size(), x));
	}
	else
		data.write(t.read(), remaining);
}
void tmData::stream(tBBuf &t) const {
	tmMsgBase::stream(t);
	if (hasFlags(plNetX)) {
		tZBuf zdata;
		zdata.put(data);
		zdata.compress();
		t.put(zdata);
	}
	else {
		t.put(data);
	}
}



class tUnetSimpleFileServer :public tUnetBase {
public:
	tUnetSimpleFileServer(const tString &lhost,U16 lport=0,Byte listen=0);
	virtual ~tUnetSimpleFileServer();
	virtual int onMsgRecieved(tUnetMsg * msg,tNetSession * u);
	virtual bool onConnectionFlood(tNetSession */*u*/) {
		return false; // don't kick nobody
	}
	virtual void onIdle(bool idle);
	virtual void onStart();
	void setDestinationAddress(const tString & d,U16 port);
	void setValidation(Byte val);
	void setUrgent() {
		urgent=true;
	}
	void setFile(const tString &file) {
		this->file = file;
	}
	void setCompressed(void) {
		compressed=true;
	}
private:
	Byte listen;
	tString d_host;
	U16 d_port;
	Byte validation;
	bool urgent;
	bool compressed;
	tString file;
	bool sent;
	tNetSessionIte dstite;
};

tUnetSimpleFileServer::tUnetSimpleFileServer(const tString &lhost,U16 lport,Byte listen) :tUnetBase(KClient) {
	this->setBindPort(lport);
	this->setBindAddress(lhost.c_str());
	this->listen=listen;
	setIdleTimer(1);
	d_port=5000;
	validation=2;
	urgent=false;
	sent=false;
	compressed=false;
	dstite.ip=0;
	dstite.port=0;
	dstite.sid=-1;
}
tUnetSimpleFileServer::~tUnetSimpleFileServer() {

}

void tUnetSimpleFileServer::setDestinationAddress(const tString &d,U16 port) {
	d_host=d;
	d_port=port;
}
void tUnetSimpleFileServer::setValidation(Byte val) {
	validation=val;
}

void tUnetSimpleFileServer::onStart() {
	if(listen==0) {
		dstite=netConnect(d_host.c_str(),d_port,validation,0);
	}
}

void tUnetSimpleFileServer::onIdle(bool idle) {
	if (listen==0) {
		if (!sent) {
			tNetSession * u=NULL;
			u=getSession(dstite);
			if (u==NULL) {
				stop();
				return;
			}
			if (!u->isConnected()) return;
			tmData data(u);
			if (urgent) data.setUrgent();
			tFBuf f1;
			f1.open(file.c_str());
			data.data.clear();
			data.data.put(f1);
			f1.close();
			if (compressed) data.setCompressed(); // do this *AFTER* the data is written to the buffer
			send(data);
			sent=true;
		} else if (idle) {
			stop();
		}
	}
}

int tUnetSimpleFileServer::onMsgRecieved(tUnetMsg * msg,tNetSession * u) {
	int ret=0;

	switch(msg->cmd) {
		case NetMsgCustomTest:
			if(listen!=0) {
				tmData data(u);
				msg->data.get(data);
				log->log("<RCV> [%d] %s\n", msg->sn, data.str().c_str());
				printf("Saving file to rcvmsg.raw...\n");
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

int main(int argc,char * argv[]) {

	int i;
	
	Byte loglevel=2;
	//local settings
	tString l_hostname="0.0.0.0";
	U16 l_port=0;
	//remote settings
	tString hostname="";
	U16 port=5000;
	
	tString file;
	
	Byte val=2; //validation level
	
	//options
	Byte listen=0,nlogs=0,urgent=0,compress=0;

	//parse parameters
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			puts(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-f") && argc>i+1) { i++; file = argv[i]; }
		else if(!strcmp(argv[i],"-lm")) { listen=1; }
		else if(!strcmp(argv[i],"-nl")) { nlogs=1; }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-u")) { urgent=1; }
		else if(!strcmp(argv[i],"-z")) { compress=1; }
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
		alcMain.config()->setVar(tString::fromByte(nlogs).c_str(), "log.enabled");
		alcMain.config()->setVar(tString::fromByte(loglevel).c_str(), "verbose_level");
		alcMain.onApplyConfig();
		
		alcMain.std()->print(alcVersionText());

		tUnetSimpleFileServer netcore(l_hostname,l_port,listen);
		if (!nlogs) netcore.unsetFlags(UNET_ELOG);

		while(listen==0 && hostname.isEmpty()) {
			printf("\nHostname not set, please enter destination host: ");
			hostname = alcConsoleAsk();
		}

		if(listen==0 && file.isEmpty()) {
			printf("No input file specified!\n");
			exit(0);
		}
		
		if(listen==0) {
			printf("Connecting to %s:%i...\n",hostname.c_str(),port);
			printf("Sending file...\n");
		} else {
			printf("Waiting for messages... CTR+C stops\n");
		}

		netcore.setValidation(val);
		netcore.setDestinationAddress(hostname,port);
		if(urgent==1) netcore.setUrgent();
		if(compress==1) netcore.setCompressed();
		netcore.setFile(file);
		
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

