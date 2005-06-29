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
	cflags=0; //default flags
	maxPacketSz=1024;
	//window settings
	rcv_win=4*8;
	wite=0;
	w=(char *)malloc(sizeof(char) * rcv_win);
	memset(w,0,sizeof(char) * rcv_win);
	//end window
	bandwidth=0;
	cabal=0;
	max_cabal=0;
	last_msg_time=0;
	rtt=0; //net->timeout/2;
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
	#if 0 //Original
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
	U32 delta=275;
	U32 inc=(delta*cabal)/1000;
	cabal+=inc;
	if(cabal>max_cabal) {
		cabal=max_cabal;
		max_cabal+=inc/2;
		if(max_cabal>(bandwidth/8)) max_cabal=(bandwidth/8);
	}
	DBG(5,"+Cabal is now %i (max:%i)\n",cabal,max_cabal);
}
void tNetSession::decreaseCabal(bool partial) {
	if(!cabal) return;
	U32 min_cabal=4096;
	U32 delta=100;
	U32 gamma=120;
	if(partial) cabal-=(gamma*cabal)/1000;
	else {
		cabal=cabal/2;
		max_cabal-=(delta*max_cabal)/1000;
		if(max_cabal<min_cabal) max_cabal=min_cabal;
	}
	if(cabal<min_cabal) cabal=min_cabal;
	DBG(5,"-Cabal is now %i (max:%i)\n",cabal,max_cabal);
}

//psize cannot be > 4k
U32 tNetSession::computetts(U32 psize) {
	if (cabal) return((psize*1000000)/cabal);
	return((psize*1000000)/4098);
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
	
	try {
		mbuf.get(msg);
		net->log->log("<RCV> ");
		msg.dumpheader(net->log);
		net->log->nl();
		msg.htmlDumpHeader(net->ack,0,ip,port);
	} catch(txUnexpectedData &t) {
		net->unx->log("%s Unexpected Data %s\nBacktrace:%s\n",str(),t.what(),t.backtrace());
		throw txProtocolError(_WHERE("Cannot parse a message"));
	}
	
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

	/* //Avoid sending ack for messages out of the window
	if(msg.tf & 0x02) {
		//ack reply
		createAckReply(msg);
	}*/
	if(msg.tf & 0x40) {
		tmNetClientComm comm;
		msg.data.rewind();
		msg.data.get(comm);
		net->log->log("<RCV> ");
		net->log->print("%s",(const char *)comm.str());
		bandwidth=comm.bandwidth;
		if(renego_stamp==comm.timestamp || negotiating) {
			net->log->print(" Ignored\n");
			negotiating=false;
		} else {
			net->log->nl();
			renego_stamp=comm.timestamp;
			//clear snd buffer
			DBG(5,"Clearing send buffer\n");
			sndq->clear();
			//TODO reset counters and window
			//if(cabal!=0) {
				if(nego_stamp.seconds==0) nego_stamp=timestamp;
				negotiate();
				negotiating=true;
			//}
			cabal=0;
		}
	} else if(bandwidth==0 || cabal==0) {
		nego_stamp=timestamp;
		negotiate();
		negotiating=true;
	}
	
	//fix the problem that happens every 15-30 days of server uptime
	if(server.sn>=8388605 || msg.sn>=8388605) {
		net->err->log("INF: Congratulations!, you have reached the maxium allowed sequence number, don't worry, this is not an error\n");
		server.pn=0;
		server.frn=0;
		server.sn=0;
		server.pfr=0;
		server.ps=0;
		nego_stamp=timestamp;
		renego_stamp.seconds=0;
		negotiate();
	}

	//check duplicates
	ret=checkDuplicate(msg);
	
	if(ret!=2 && (msg.tf & 0x02)) {
		//ack reply
		createAckReply(msg);
	}

	if(ret==0) {
	
		if(msg.tf & 0x80) {
			//ack update
			if(authenticated==2) authenticated=1;
		} else if(1) {
		
		}
	
	
	
	
	}
	
	doWork();
}

/**
	\return 2 - Out of the window
	\return 1 - Marked
	\return 0 - Non-Marked
*/
Byte tNetSession::checkDuplicate(tUnetUruMsg &msg) {
	//drop already parsed messages
	//nlog(f_err,net,sid,"INF: (Before) window is:\n");
	//dumpbuf(f_err,(Byte *)s->w,rcv_win);
	//lognl(f_err);
	if(!(msg.sn > wite || msg.sn <= (wite+rcv_win))) {
		net->err->log("%s INF: Dropped packet %i out of the window by sn\n",str(),msg.sn);
		net->err->flush();
		return 2;
	} else { //then check if is already marked
		U32 i,start,ck;
		start=wite % (rcv_win*8);
		i=msg.sn % (rcv_win*8);
		if(((w[i/8] >> (i%8)) & 0x01) && msg.frn==0) {
			net->err->log("%s INF: Dropped already parsed packet %i\n",str(),msg.sn);
			net->err->flush();
			return 1;
		} else {
			w[i/8] |= (0x01<<(i%8)); //activate bit
			while((i==start && ((w[i/8] >> (i%8)) & 0x01))) { //move the iterator
				w[i/8] &= ~(0x01<<(i%8)); //deactivate bit
				i++; start++;
				wite++;
				if(i>=rcv_win*8) { i=0; start=0; }
				//nlog(f_err,net,sid,"INF: A bit was deactivated (1)\n");
			}
			ck=(i < start ? i+(rcv_win*8) : i);
			while((ck-start)>((rcv_win*8)/2)) {
				DBG(5,"ck: %i,start:%i\n",ck,start);
				w[start/8] &= ~(0x01<<(start%8)); //deactivate bit
				start++;
				wite++;
				if(start>=rcv_win*8) { start=0; ck=i; }
				//nlog(f_err,net,sid,"INF: A bit was deactivated (2)\n");
			}
			//nlog(f_err,net,sid,"INF: Packet %i accepted to be parsed\n",s->client.sn);
		}
	}
	//nlog(f_err,net,sid,"INF: (After) window is:\n");
	//dumpbuf(f_err,(Byte *)s->w,rcv_win);
	//lognl(f_err);
	return 0;
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
		tts=computetts(msg.frt * maxPacketSz * 1000000);
	}
	if(tts>rtt) tts=rtt;
	net->updatetimer(tts);
	ack->timestamp=net->net_time + tts;
	
	int i=0;

#if 0
	//Non-Plasma like ack's
	ackq->add(ack);
#else
	//Plasma like ack's (acks are retarded, and packed)
	ackq->rewind();
	if(ackq->getNext()==NULL) {
		ackq->add(ack);
	} else {
		ackq->rewind();
		while((cack=ackq->getNext())!=NULL) {
			net->log->log("2store[%i] %i,%i %i,%i\n",i,(ack->A & 0x000000FF),(ack->A >> 8),(ack->B & 0x000000FF),(ack->B >> 8));
			net->log->log("2check[%i] %i,%i %i,%i\n",i,(cack->A & 0x000000FF),(cack->A >> 8),(cack->B & 0x000000FF),(cack->B >> 8));

			i++;
			if(A>=cack->A && B<=cack->A) {
				net->log->log("A\n");
				if(cack->next==NULL) {
					cack->A=A;
					cack->B=(cack->B > B ? B : cack->B);
					break;
				} else {
					B=ack->B=(cack->B > B ? B : cack->B);
					ackq->deleteCurrent();
					ackq->rewind();
					continue;
				}
			} else if(B>cack->A) {
				net->log->log("B\n");
				if(cack->next==NULL) { ackq->add(ack); break; }
				else continue;
			} if(A<cack->B) {
				net->log->log("C\n");
				ackq->insertBefore(ack);
				break;
			} else if(A<=cack->A && A>=cack->B) {
				net->log->log("D\n");
				A=ack->A=cack->A;
				B=ack->B=(cack->B > B ? B : cack->B);
				bool isNull=(cack->next==NULL);
				ackq->deleteCurrent();
				ackq->rewind();
				if(isNull) { ackq->add(ack); break; }
				else continue;
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
	//return;
	U32 i,maxacks=30,hsize,tts;

	ackq->rewind();
	tUnetAck * ack;
	ack=ackq->getNext();
	if(ack==NULL || ack->timestamp>net->net_time) {
		return;
	}

	tUnetUruMsg * pmsg;
	
	server.frn=0;
	server.tf=0x80;
	server.val=validation;
	if(cflags & UNetUpgraded) {
		server.tf = 0x80 | UNetExt;
		server.val = 0x00;
	}
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
		if(!(pmsg->tf & UNetExt))
		pmsg->data.putU16(0);
		
		ackq->rewind();
		i=0;
		while((ack=ackq->getNext()) && i<maxacks) {
			pmsg->data.putU32(ack->A);
			if(!(pmsg->tf & UNetExt))
			pmsg->data.putU32(0);
			pmsg->data.putU32(ack->B);
			if(!(pmsg->tf & UNetExt))
			pmsg->data.putU32(0);
			ackq->deleteCurrent();
			i++;
		}
		
		pmsg->_update();
		pmsg->timestamp=net->net_time;
		pmsg->dsize=i;

		if(!(pmsg->tf & UNetExt))
			tts=computetts((i*16)+2+hsize+net->ip_overhead);
		else
			tts=computetts((i*8)+hsize+net->ip_overhead);
			
		#ifdef _UNET_DBG_
		tts+=net->latency;
		#endif
		pmsg->timestamp+=tts;
		net->updatetimer(tts);
		
		//put pmsg to the qeue
		sndq->add(pmsg);
	}
	
}

//Send, and re-send messages
void tNetSession::doWork() {

	idle=false;

	ackUpdate(); //get ack messages

	if(net->net_time>=last_msg_time) {

		if(sndq->isEmpty()) {
			last_msg_time=0;
			if(ackq->isEmpty()) {
				idle=true;
			} else {
				idle=false;
			}
			return;
		}
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
						//timeout event
						tNetSessionIte ite(ip,port,sid);
						tNetEvent * evt=new tNetEvent(ite,UNET_TIMEOUT);
						net->events->add(evt);
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
		tts=computetts(cur_quota);
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


