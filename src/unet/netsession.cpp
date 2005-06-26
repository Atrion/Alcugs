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

#define _DBG_LEVEL_ 7

#include "alcugs.h"
#include "urunet/unet.h"

#include "alcdebug.h"

namespace alc {

/* Session */
tNetSession::tNetSession(tUnet * net) {
	DBG(5,"tNetSession()\n");
	this->net=net;
	init();
	sndq = new tUnetMsgQ<tUnetUruMsg>;
	ackq = new tUnetMsgQ<tUnetAck>;
}
tNetSession::~tNetSession() {
	DBG(5,"~tNetSession()\n");
	delete sndq;
	delete ackq;
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
	max_cabal=0;
	last_msg_time=0;
	rtt=net->timeout/2;
	timeout=net->timeout;
	conn_timeout=net->conn_timeout;
	desviation=0;
	success=0;
	nego_stamp.seconds=0;
	nego_stamp.microseconds=0;
	renego_stamp=nego_stamp;
	negotiating=false;
	memset((void *)&server,0,sizeof(server));
	assert(server.pn==0);
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
	DBG(5,"RTT update rtt:%i, timeout:%i\n",rtt,timeout);
}
void tNetSession::increaseCabal() {
	if(!cabal) return;
	U32 delta=800;
	cabal+=delta;
	max_cabal+=delta/50;
	if(max_cabal>bandwidth) max_cabal=bandwidth;
	if(cabal>max_cabal) cabal=max_cabal;
	DBG(5,"+Cabal is now %i (max:%i)\n",cabal,max_cabal);
}
void tNetSession::decreaseCabal(bool partial) {
	if(!cabal) return;
	U32 epsilon=4096;
	U32 delta=800;
	U32 gamma=120;
	if(partial) cabal-=(gamma*cabal)/1000;
	else {
		cabal=cabal/2;
		max_cabal-=delta;
	}
	if(cabal<epsilon) cabal=epsilon;
	if(max_cabal<epsilon) max_cabal=epsilon;
	DBG(5,"-Cabal is now %i (max:%i)\n",cabal,max_cabal);
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
	net->log->log("<RCV> RAW Packet follows: \n");
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
		max_cabal=cabal;
	}
	//How do you say "Cabal" in English?

	if(msg.tf & 0x02) {
		//ack reply
		createAckReply(msg);
	}
	if(msg.tf & 0x40) {
		tmNetClientComm comm;
		msg.data.rewind();
		msg.data.get(comm);
		net->log->log("<RCV> ");
		net->log->print("%s\n",(const char *)comm.str());
		bandwidth=comm.bandwidth;
		if(renego_stamp==comm.timestamp || negotiating) {
			net->log->print(" Ignored");
			negotiating=false;
		} else {
			renego_stamp=comm.timestamp;
			//clear snd buffer
			DBG(5,"Clearing send buffer\n");
			sndq->clear();
			//reset counters and window
			//if(cabal!=0) {
				if(nego_stamp.seconds==0) nego_stamp=timestamp;
				negotiate();
				negotiating=true;
			//}
			cabal=0;
		}
		net->log->nl();
	} else if(bandwidth==0 || cabal==0) {
		nego_stamp=timestamp;
		negotiate();
		negotiating=true;
	}
	
	//check duplicates

	
}


void tNetSession::createAckReply(tUnetUruMsg &msg) {
	tUnetAck * ack,* cack;
	U32 A,B;
	
	net->log->log("stacking ack %i,%i %i,%i\n",msg.sn,msg.frn,msg.ps,msg.pfr);
	
	A=msg.csn;
	B=msg.cps;
	
	ack=new tUnetAck();
	ack->A=msg.csn;
	ack->B=msg.cps;
	U32 tts=0;
	if(msg.frt) {
		if(cabal) tts=((msg.frt * maxPacketSz * 1000000)/cabal);
		else tts=((msg.frt * maxPacketSz * 1000000)/4096);
	}
	if(tts>rtt) tts=rtt;
	ack->timestamp=net->net_time + tts;
	
	int i=0;

	ackq->add(ack);
	
#if 0
	//This crap does not work as intended
	ackq->rewind();
	if(ackq->getNext()==NULL) {
		ackq->add(ack);
	} else {
		ackq->rewind();
		while((cack=ackq->getNext())!=NULL) {
			net->log->log("2store[%i] %i,%i %i,%i\n",i,(ack->A & 0x000000FF),(ack->A >> 8),(ack->B & 0x000000FF),(ack->B >> 8));
			net->log->log("2check[%i] %i,%i %i,%i\n",i,(cack->A & 0x000000FF),(cack->A >> 8),(cack->B & 0x000000FF),(cack->B >> 8));

			i++;
			if(cack->A>=B && cack->A<=A) {
				ack->B=cack->B;
				//ackq->insertBefore(ack);
				if(cack->next==NULL) {
					ackq->add(ack);
					break;
				} else {
					ackq->deleteCurrent();
				}
				net->log->log("A\n");
				//break;
			} else
			if(cack->A<B && cack->next==NULL) {
				ackq->insert(ack);
				net->log->log("B\n");
				break;
			} else
			if(A<cack->B) {
				ackq->insertBefore(ack);
				net->log->log("C\n");
				break;
			} else
			if(A>=cack->B && A<=cack->A) {
				cack->B=(B>cack->B ? cack->B : B);
				net->log->log("D\n");
				break;
			}
		}
	}
	
	net->log->log("ack stack TAIL looks like:\n");
	i=0;
	while((cack=ackq->getNext())!=NULL) {
		net->log->log("st-ack[%i] %i,%i %i,%i\n",i++,(cack->A & 0x000000FF),(cack->A >> 8),(cack->B & 0x000000FF),(cack->B >> 8));
	}
#endif

	net->log->log("ack stack looks like:\n");
	ackq->rewind();
	i=0;
	while((cack=ackq->getNext())!=NULL) {
		net->log->log("st-ack[%i] %i,%i %i,%i\n",i++,(cack->A & 0x000000FF),(cack->A >> 8),(cack->B & 0x000000FF),(cack->B >> 8));
	}
}


void tNetSession::ackUpdate() {

	U32 i,maxacks=30,hsize;

	ackq->rewind();
	tUnetAck * ack;
	ack=ackq->getNext();
	if(ack==NULL || ack->timestamp>net->net_time) {
		if(ack) net->updatetimer(net->net_time-ack->timestamp);
		return;
	}

	tUnetUruMsg * pmsg;
	
	server.frn=0;
	server.tf=0x80;
	server.val=validation;
	if(server.val==0x01) { server.val=0x00; }
	if(server.val==0x00) { hsize=28; } else { hsize=32; }
	if(server.tf & UNetExt) { hsize-=8; }

	ackq->rewind();
	while((ack=ackq->getNext())) {
	
		pmsg=new tUnetUruMsg;
	
		if(server.tf & 0x02) {
			server.ps=server.sn;
			server.pfr=server.frn;
			//the second field, only contains the seq number from the latest packet with
			//the ack flag enabled.
			DBG(8,"The previous sent packet had the ack flag on\n");
		}
		//now update the other fields
		server.sn++;
		
		pmsg->val=server.val;
		//pmsg.pn NOT in this layer (done in the msg sender)
		pmsg->tf=server.tf;
		pmsg->frn=server.frn;
		pmsg->sn=server.sn;
		pmsg->frt=0;
		pmsg->pfr=server.pfr;
		pmsg->ps=server.ps;

		//pmsg->data.write(buf.read(csize),csize);
		pmsg->data.putU16(0);
		
		ackq->rewind();
		i=0;
		while((ack=ackq->getNext()) && i<maxacks) {
			pmsg->data.putU32(ack->A);
			pmsg->data.putU32(0);
			pmsg->data.putU32(ack->B);
			pmsg->data.putU32(0);
			ackq->deleteCurrent();
			i++;
		}
		
		pmsg->_update();
		pmsg->timestamp=net->net_time;
		pmsg->dsize=i;

		if(cabal) {
			pmsg->timestamp+=(((i*16)+2+hsize+net->ip_overhead)*1000000)/cabal;
		}
		#ifdef _UNET_DBG_
		pmsg->timestamp+=net->latency;
		#endif
		
		//put pmsg to the qeue
		sndq->add(pmsg);
	}

	
}

//Send, and re-send messages
void tNetSession::doWork() {

	ackUpdate(); //get ack messages

	if(net->net_time>=last_msg_time) {

		sndq->rewind();
		if(sndq->getNext()==NULL) {
			last_msg_time=0;
			ackq->rewind();
			if(ackq->getNext()==NULL) {
				idle=true;
			} else {
				idle=false;
			}
			return;
		}
		idle=false;
		sndq->rewind();
		tUnetUruMsg * curmsg;
		
		U32 quota_max=1024;
		U32 cur_quota=0;
		
		U32 minTH=10;
		U32 maxTH=150;
		U32 tts;

		while((curmsg=sndq->getNext())!=NULL && (cur_quota<=quota_max)) {
			
			if(curmsg->timestamp<=net->net_time) {
				//we can send the message
				if(curmsg->tf & 0x80) {
					//send paquet
					cur_quota+=curmsg->size();
					net->rawsend(this,curmsg);
					sndq->deleteCurrent();
				} else if(curmsg->tf & 0x02) {
					//send paquet
					success++;
					if(curmsg->tryes!=0) {
						if(curmsg->tryes==1) {
							decreaseCabal(true);
						} else {
							decreaseCabal(false);
						}
						success=0;
					} else if(success>=20) {
						increaseCabal();
					}
					if(curmsg->tryes>=6) {
						sndq->deleteCurrent();
						//TODO timeout event
					} else {
						cur_quota+=curmsg->size();
						net->rawsend(this,curmsg);
						curmsg->timestamp+=timeout;
						curmsg->tryes++;
					}
				} else {
					//probabilistic drop (of voice, and other non-ack paquets)
					if(net->net_time-curmsg->timestamp > timeout) {
						//Unacceptable - drop it
						sndq->deleteCurrent();
					} else if(sndq->len()>minTH && (random()%maxTH)>sndq->len()) {
						sndq->deleteCurrent();
					} else {
						cur_quota+=curmsg->size();
						net->rawsend(this,curmsg);
						sndq->deleteCurrent();
					}
				} //end prob drop
			} //end time check
			 else { DBG(8,"Too soon to send a message\n"); }
		} //end while
		if(cabal) tts=((cur_quota*1000000)/cabal);
		else tts=((cur_quota*1000000)/4096);
		DBG(8,"tts is now:%i quota:%i,cabal:%i\n",tts,cur_quota,cabal);
		last_msg_time=net->net_time + tts;
		net->updatetimer(tts);
	} else { DBG(8,"Too soon to check sndq"); }
}

void tNetSession::negotiate() {
	U32 sbw;
	//server bandwidth
	DBG(9,"%08X %08X %08X\n",ip,net->lan_mask,net->lan_addr);
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


