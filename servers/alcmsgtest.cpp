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

#include <alcdefs.h>
#include <unetbase.h>
#include <unetmain.h>
#include <alcversion.h>
#include <alclicense.h>
#include <alcexception.h>
#include <netsessionmgr.h>

#include <cstring>

using namespace alc;

static const size_t maxSize = 250*1000;

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
	void setFirstFragment() {
		ki |= 1;
	}
	bool isFirstFragment() {
		return ki & 1;
	}
	void setLastFragment() {
		ki |= 2;
	}
	bool isLastFragment() {
		return ki & 2;
	}
	//format
	tMBuf data;
};

tmData::tmData(tNetSession * u)
 :tmMsgBase(NetMsgCustomTest,plNetTimestamp | plNetAck | plNetKi,u) { ki=0; }
void tmData::store(tBBuf &t) {
	tmMsgBase::store(t);
	data.clear();
	size_t remaining = t.remaining();
	if (hasFlags(plNetX)) {
		tZBuf zdata;
		zdata.write(t.readAll(), remaining);
		zdata.uncompress(x);
		data = zdata;
		if (data.size() != x) throw txBase(_WHERE("size mismatch (%d != %d)", data.size(), x));
	}
	else
		data.write(t.readAll(), remaining);
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
	tUnetSimpleFileServer(const tString &lhost,uint16_t lport=0,bool listen=false);
	virtual int onMsgRecieved(tUnetMsg * msg,tNetSession * u);
	virtual bool onConnectionFlood(tNetSession */*u*/) {
		return false; // don't kick nobody
	}
	virtual void onIdle();
	virtual void onStart();
	void setDestinationAddress(const tString & d,uint16_t port);
	void setValidation(uint8_t val);
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
	bool listen;
	tString d_host;
	uint16_t d_port;
	uint8_t validation;
	bool urgent;
	bool compressed;
	tString file;
	bool sent;
	tNetSessionIte dstite;
	// for speed measure
	int sentBytes;
	tTime startTime;
};

tUnetSimpleFileServer::tUnetSimpleFileServer(const tString &lhost,uint16_t lport,bool listen) :tUnetBase(KClient) {
	this->setBindPort(lport);
	this->setBindAddress(lhost);
	this->listen=listen;
	max_sleep = 500*1000; // set down max_sleep timer (FIXME: why?)
	d_port=5000;
	validation=2;
	urgent=false;
	sent=false;
	sentBytes=0;
	compressed=false;
	dstite.ip=0;
	dstite.port=0;
	dstite.sid=-1;
}


void tUnetSimpleFileServer::setDestinationAddress(const tString &d,uint16_t port) {
	d_host=d;
	d_port=port;
}
void tUnetSimpleFileServer::setValidation(uint8_t val) {
	validation=val;
}

void tUnetSimpleFileServer::onStart() {
	if(!listen) {
		dstite=netConnect(d_host.c_str(),d_port,validation,0);
	}
}

void tUnetSimpleFileServer::onIdle() {
	if (!listen) {
		if (!sent) {
			tNetSession * u=NULL;
			u=getSession(dstite);
			if (u==NULL) {
				stop();
				return;
			}
			if (!u->isConnected()) return;
			tFBuf f1(file.c_str());
			size_t size;
			bool first = true;
			while ((size = f1.remaining())) {
				if (size > maxSize) size = maxSize;
				tmData data(u);
				if (urgent) data.setUrgent();
				if (first) {
					data.setFirstFragment();
					first = false;
				}
				data.data.clear();
				data.data.write(f1.read(size), size);
				if (f1.eof()) data.setLastFragment();
				if (compressed) data.setCompressed(); // do this *AFTER* the data is written to the buffer
				send(data);
			}
			sentBytes = compressed ? 0 : f1.size();
			sent=true;
			startTime.setToNow();
		} else { // FIXME only really stop if completely done
			if (sentBytes) {
				tTime diff;
				diff.setToNow();
				diff = diff-startTime;
				printf("Sent %d Bytes with %f kBit/s in %s\n", sentBytes, sentBytes*8/diff.asDouble()/1000, diff.str(0x01).c_str());
			}
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
				tFBuf f1;
				if (data.isFirstFragment())
					f1.open("rcvmsg.raw","wb");
				else
					f1.open("rcvmsg.raw","ab");
				f1.put(data.data);
				f1.close();
				if (data.isLastFragment()) printf("Saved received file to rcvmsg.raw\n");
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
	
	uint8_t loglevel=2;
	//local settings
	tString l_hostname="0.0.0.0";
	uint16_t l_port=0;
	//remote settings
	tString hostname="";
	uint16_t port=5000;
	
	tString file;
	
	uint8_t val=2; //validation level
	
	//options
	bool listen=false,nlogs=false,urgent=false,compress=false;

	//parse parameters
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			puts(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-f") && argc>i+1) { i++; file = argv[i]; }
		else if(!strcmp(argv[i],"-lm")) { listen=true; }
		else if(!strcmp(argv[i],"-nl")) { nlogs=true; }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-u")) { urgent=true; }
		else if(!strcmp(argv[i],"-z")) { compress=true; }
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
		alcMain.config()->setVar(tString::fromUInt(nlogs), "log.enabled", "global");
		alcMain.config()->setVar(tString::fromUInt(loglevel), "verbose_level", "global");
		alcMain.onApplyConfig();
		
		alcMain.std()->print(alcVersionText());

		tUnetSimpleFileServer netcore(l_hostname,l_port,listen);
		if (!nlogs) netcore.unsetFlags(UNET_ELOG);

		while(!listen && hostname.isEmpty()) {
			printf("\nHostname not set, please enter destination host: ");
			hostname = alcConsoleAsk();
		}

		if(!listen && file.isEmpty()) {
			printf("No input file specified!\n");
			exit(0);
		}
		
		if(!listen) {
			printf("Connecting to %s:%i...\n",hostname.c_str(),port);
			printf("Sending file...\n");
		} else {
			printf("Waiting for messages... CTR+C stops\n");
		}

		netcore.setValidation(val);
		netcore.setDestinationAddress(hostname,port);
		if (urgent) netcore.setUrgent();
		if (compress) netcore.setCompressed();
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

