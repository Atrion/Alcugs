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
#include <alcexception.h>
#include <netsessionmgr.h>

#include <cstring>

using namespace alc;

static const size_t maxSize = 250*1000;

void parameters_usage() {
	puts(alcVersionText());
	printf("Usage: alcmsgtest peer:port [options]\n\n\
 -f x: Set the file to upload\n\
 -z: Compress the file\n\
 -val x: set validation level (0-3) (default 2)\n\
 -nl: enable netcore log files\n\
 -V: show version and end\n\
 -h: Show short help and end\n\
 -v x: Set the verbose level\n\
 -lp x: Set the local port (enables listen mode)\n\
 -rp x: Set the remote port\n\
 -lh x: Set the local host (enables listen mode)\n\
 -rh x: Set the remote host\n\
 -u:   Set Urgent flag.\n\n");
}

class tmData :public tmNetMsg {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmData(tNetSession *u) : tmNetMsg(u) {}
	tmData(tNetSession * u, bool compressed);
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

tmData::tmData(tNetSession* u, bool compressed)
 :tmNetMsg(NetMsgCustomTest,plNetTimestamp | plNetAck | plNetKi,u)
 {
	 ki=0;
	 if (compressed) setFlags(plNetX);
}
void tmData::store(tBBuf &t) {
	x = data.size(); // used when compressed
	tmNetMsg::store(t);
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
	tmNetMsg::stream(t);
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
	tNetSessionRef dstSession;
	// for speed measure
	int sentBytes;
	tTime startTime;
};

tUnetSimpleFileServer::tUnetSimpleFileServer(const tString &lhost,uint16_t lport,bool listen) :tUnetBase(KClient) {
	this->setBindPort(lport);
	this->setBindAddress(lhost);
	this->listen=listen;
	this->unsetFlags(UNET_FLOODCTR); // disable flood protection
	d_port=5000;
	validation=2;
	urgent=false;
	sent=false;
	sentBytes=0;
	compressed=false;
	// don't miss anything
	if (!listen) max_sleep = 100*1000;
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
		dstSession=netConnect(d_host.c_str(),d_port,validation);
	}
}

void tUnetSimpleFileServer::onIdle() {
	// onMsgReceive and onIdle are not both processed in the same instance (one for listener, one for sender), so no locking required
	if (listen || !*dstSession) return;
	DBG(5, "OnIdle called\n");
	if (dstSession->isTerminated()) {
		DBG(5, "seems we lost the connection, ouch - going down\n");
		stop();
		return;
	}
	if (!sent) {
		DBG(5, "Starting to send\n");
		tFBuf f1(file.c_str());
		size_t size;
		bool first = true;
		tReadLock lock(dstSession->pubDataMutex);
		while ((size = f1.remaining())) {
			if (size > maxSize) size = maxSize;
			tmData data(*dstSession, compressed);
			if (urgent) data.urgent = true;
			if (first) {
				data.setFirstFragment();
				first = false;
			}
			data.data.clear();
			data.data.write(f1.read(size), size);
			if (f1.eof()) data.setLastFragment();
			send(data);
		}
		sentBytes = compressed ? 0 : f1.size();
		sent=true;
		startTime.setToNow();
	} else if (!dstSession->anythingToSend()) { // message sent, and nothing more left in the buffers
		if (sentBytes) {
			tTime diff = tTime::now();
			diff = diff-startTime;
			printf("Sent %d Bytes with %f kBit/s in %s\n", sentBytes, sentBytes*8/diff.asDouble()/1000, diff.str(/*relative*/true).c_str());
		}
		DBG(5, "Finished, going down\n");
		stop();
	}
}

int tUnetSimpleFileServer::onMsgRecieved(tUnetMsg * msg,tNetSession * u) {
	// onMsgReceive and onIdle are not both processed in the same instance (one for listener, one for sender), so no locking required
	if (!listen) return 0;
	switch(msg->cmd) {
		case NetMsgCustomTest:
		{
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
			return 1;
		}
		default:
			return 0;
	}
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
	
	//start Alcugs library (before parsing params, or license text won't work!)
	tAlcUnetMain alcMain("Client");

	//parse parameters
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			puts(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; listen=true; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-f") && argc>i+1) { i++; file = argv[i]; }
		else if(!strcmp(argv[i],"-nl")) { nlogs=true; }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-u")) { urgent=true; }
		else if(!strcmp(argv[i],"-z")) { compress=true; }
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

