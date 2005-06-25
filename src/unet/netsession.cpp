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
#define __U_NETSESSION_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "urunet/unet.h"

#include "alcdebug.h"

namespace alc {

/* Session */
tNetSession::tNetSession(tUnet * net) {
	DBG(5,"tNetSession()\n");
	this->net=net;
	init();
	sndq = new tUnetOutMsgQ;
}
tNetSession::~tNetSession() {
	DBG(5,"~tNetSession()\n");
	delete sndq;
}
void tNetSession::init() {
	DBG(5,"init()\n");
	ip=0;
	port=0;
	sid=-1;
	validation=0;
	authenticated=0;
	maxPacketSz=1024;
	bandwidth=0;
	cabal=0;
	last_msg_time=0;
	rtt=net->timeout/2;
	timeout=net->timeout;
	desviation=0;
	nego_stamp.seconds=0;
	nego_stamp.microseconds=0;
	memset((void *)&server,0,sizeof(server));
	idle=false;
}
char * tNetSession::str(char how) {
	static char cnt[1024];
	sprintf(cnt,"[%i][%s:%i]",sid,alcGetStrIp(ip),ntohs(port));
	return cnt;
}
void tNetSession::updateRTT(U32 newread) {
	if(rtt==0) rtt=newread;
	#if 1 //Original
		const U32 alpha=800;
		rtt=((alpha*rtt)/1000) + ((1000-alpha)*newread);
		timeout=2*rtt;
	#else //Jacobson/Karels
		S32 alpha=125;
		S32 u=1;
		S32 delta=4;
		S32 diff;
		diff=(S32)newread - (S32)rtt;
		(S32)rtt+=(alpha*diff)/1000;
		desviation+=(alpha*(abs(diff)-desviation))/1000;
		timeout=u*rtt + delta*desviation;
	#endif
}
void tNetSession::increaseCabal() {
	U32 delta=800;
	cabal+=delta;
}
void tNetSession::decreaseCabal() {
	U32 epsilon=4096;
	cabal=cabal/2;
	if(cabal<epsilon) cabal=epsilon;
}
void tNetSession::processMsg(Byte * buf,int size) {
	DBG(5,"Message of %i bytes\n",size);
	//stamp
	timestamp.seconds=alcGetTime();
	timestamp.microseconds=alcGetMicroseconds();
	
	int ret;
	
	ret=alcUruValidatePacket(buf,size,&validation,authenticated,passwd);
	
	if(ret!=0 && (ret!=1 || net->flags & UNET_ECRC)) {
		if(ret==1) net->err->log("ERR: Failed Validating a message!\n");
		else net->err->log("ERR: Non-Uru protocol packet recieved!\n");
	}
	
	#if _DBG_LEVEL_ > 2
	net->log->log("RAW Packet follows: \n");
	net->log->dumpbuf(buf,size);
	net->log->nl();
	#endif
	
	tSBuf mbuf(buf,size);
	
	tUnetUruMsg msg;
	
	mbuf.get(msg);
	net->log->log("<RCV> ");
	msg.dumpheader(net->log);
	net->log->nl();
	msg.htmlDumpHeader(net->ack,0,ip,port);
	
	//set avg cabal
	if(cabal==0 && bandwidth!=0) {
		if((ip & 0x00FFFFFF) == 0x0000007F) { //lo
			cabal=100000000/8; //100Mbps
		} else if((ip & net->lan_mask) == net->lan_addr) { //LAN
			cabal=((net->lan_up > bandwidth) ? bandwidth : net->lan_up) / 8;
		} else { //WAN
			cabal=((net->nat_up > bandwidth) ? bandwidth : net->nat_up) / 8;
		}
		net->log->log("Cabal is now %i (%i bps)\n",cabal,cabal*8);
	}
	//How do you say "Cabal" in English?
	
	if(bandwidth==0 || cabal==0) {
		nego_stamp=timestamp;
		negotiate();
	}
	
}

//Send, and re-send messages
void tNetSession::doWork() {

	if(net->net_time>=last_msg_time) {

		sndq->rewind();
		if(sndq->getNext()==NULL) {
			last_msg_time=0;
			idle=true;
		}
		idle=false;
		sndq->rewind();
		tUnetUruMsg * curmsg;
		
		U32 quota_max=1024;
		U32 cur_quota=0;
		
		U32 minTH=10;
		U32 maxTH=150;

		while((curmsg=sndq->getNext())!=NULL && (cur_quota<quota_max)) {
			
			if(curmsg->timestamp<=net->net_time) {
				//we can send the message
				if(curmsg->tf & 0x80) {
					//send paquet
					cur_quota+=curmsg->size();
					net->rawsend(this,curmsg);
					sndq->deleteCurrent();
				} else if(curmsg->tf & 0x02) {
					//send paquet
					cur_quota+=curmsg->size();
					net->rawsend(this,curmsg);
					curmsg->timestamp+=timeout;
					curmsg->timestamp;
				} else {
					//probabilistic drop (of voice, and other non-ack paquets)
					if(net->net_time-curmsg->timestamp > timeout) {
						//Unacceptable - drop it
						sndq->deleteCurrent();
					} else if(sndq->len()>minTH && (random()%maxTH)>sndq->len()) {
						sndq->deleteCurrent();
					} else {
						//TODO send paquet
						sndq->deleteCurrent();
					}
				} //end prob drop
			} //end time check
		} //end while
		last_msg_time=net->net_time;
	}
}

void tNetSession::negotiate() {
	U32 sbw;
	//server bandwidth
	DBG(8,"%08X %08X %08X\n",ip,net->lan_mask,net->lan_addr);
	if((ip & 0x00FFFFFF) == 0x0000007F) { //lo
		sbw=100000000;
	} else if((ip & net->lan_mask) == net->lan_addr) { //LAN
		sbw=net->lan_down;
	} else {
		sbw=net->nat_down; //WAN
	}

	tmNetClientComm comm(nego_stamp,sbw);
	net->basesend(this,comm);
}

/* End session */

}


