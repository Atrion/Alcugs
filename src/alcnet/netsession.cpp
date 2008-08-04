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

/**
	URUNET 3
*/

/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSION_ID "$Id$"

//#define _DBG_LEVEL_ 3

#include "alcugs.h"
#include "alcnet.h"

#include "alcdebug.h"

namespace alc {

/* Session */
tNetSession::tNetSession(tUnet * net,U32 ip,U16 port,int sid) {
	DBG(5,"tNetSession()\n");
	this->net=net;
	this->ip=ip;
	this->port=port;
	this->sid=sid;
	init();
	sndq = new tUnetMsgQ<tUnetUruMsg>;
	ackq = new tUnetMsgQ<tUnetAck>;
	rcvq = new tUnetMsgQ<tUnetMsg>;
	//new conn event
	tNetSessionIte ite(ip,port,sid);
	tNetEvent * evt=new tNetEvent(ite,UNET_NEWCONN);
	net->events->add(evt);
}
tNetSession::~tNetSession() {
	DBG(5,"~tNetSession()\n");
	if (data) delete data;
	free((void *)w);
	delete sndq;
	delete ackq;
	delete rcvq;
}
void tNetSession::init() {
	DBG(5,"init()\n");
	/*ip=0;
	port=0;
	sid=-1;*/
	validation=0;
	authenticated=0;
	accessLevel=0;
	cflags=0; //default flags
	if(net->flags & UNET_NOCONN) {
		cflags |= UNetNoConn;
		bandwidth=(4096*8)*2;
		cabal=4096;
		max_cabal=4096;
	}
	maxPacketSz=1024;
	//window settings
	rcv_win=4*8;
	wite=0;
	w=(char *)malloc(sizeof(char) * rcv_win);
	memset(w,0,sizeof(char) * rcv_win);
	//end window
	flood_last_check=0;
	flood_npkts=0;
	bandwidth=0;
	cabal=0;
	max_cabal=0;
	last_msg_time=0;
	rtt=net->timeout/2; //this prevents the rtt and thus the timeout from getting too small
	ack_rtt=0;
	timeout=net->timeout;
	conn_timeout=net->conn_timeout;
	nego_stamp.seconds=0;
	nego_stamp.microseconds=0;
	renego_stamp=nego_stamp;
	negotiating=false;
	memset((void *)&server,0,sizeof(server));
	assert(server.pn==0);
	idle=false;
	delayMessages=false;
	whoami=0;
	max_version=0;
	min_version=0;
	tpots=0;
	proto=0; //alcProtoMIN_VER
	ki=0;
	client = true;
	terminated = false;
	data = NULL;
	name[0] = 0;
	
	DBG(5, "%s Initial timeout: %d\n", str(), timeout);
}
const char * tNetSession::str(bool detail) {
	static char cnt[1024], tmp[1024];
	if (!detail) {
		// short string
		sprintf(cnt,"[%s:%i]",alcGetStrIp(ip),ntohs(port));
		return cnt;
	}
	// detailed string
	sprintf(cnt,"[%i][%s:%i]",sid,alcGetStrIp(ip),ntohs(port));
	if (name[0] != 0 && authenticated != 0) {
		if (authenticated == 10) sprintf(tmp, "[%s?]", name); // if the auth server didn't yet confirm that, add a question mark
		else if (ki != 0) sprintf(tmp, "[%s:%d]", name, ki);
		else sprintf(tmp, "[%s]", name);
		strcat(cnt, tmp);
	}
	else if (whoami != 0) {
		sprintf(tmp, "[%s]", alcUnetGetDestination(whoami));
		strcat(cnt, tmp);
	}
	return cnt;
}

U32 tNetSession::getHeaderSize() {
	U32 my=28;
	if(validation>0) my+=4;
	if(cflags & UNetUpgraded) my-=8;
	return my;
}
U32 tNetSession::getMaxFragmentSize() {
	return(maxPacketSz-getHeaderSize());
}
U32 tNetSession::getMaxDataSize() {
	return(getMaxFragmentSize() * 256);
}
void tNetSession::updateRTT(U32 newread) {
	static S32 deviation=0;
	if(rtt==0) rtt=newread;
	#if 0 //Original
		const U32 alpha=800;
		rtt=((alpha*rtt)/1000) + (((1000-alpha)*newread)/1000);
		timeout=2*rtt;
	#else //Jacobson/Karels
		S32 alpha=125; // this is effectively 0.125
		S32 u=1;
		S32 delta=4;
		S32 diff=(S32)newread - (S32)rtt;
		rtt=(S32)rtt+((alpha*diff)/1000);
		deviation+=(alpha*(abs(diff)-deviation))/1000;
		if(deviation!=0) timeout=u*rtt + delta*deviation;
	#endif
	ack_rtt=rtt/8;
	DBG(5,"%s RTT update (sample rtt: %i) new rtt:%i, timeout:%i\n",str(),newread,rtt,timeout);
}
void tNetSession::duplicateTimeout() {
	U32 maxTH=4000000; //
	timeout+=(timeout*666)/1000;
	if(timeout>maxTH) timeout=maxTH;
	DBG(5,"%s timeout update:%i\n",str(),timeout);
	//net->log->log("Abort()\n");
	//abort();
}
void tNetSession::increaseCabal() {
	if(!cabal) return;
	U32 delta=275;
	U32 inc=(delta*cabal)/1000;
	cabal+=inc;
	if(cabal>max_cabal) {
		cabal=max_cabal;
		max_cabal+=inc/8;
		if(max_cabal>(bandwidth/8)) max_cabal=(bandwidth/8);
	}
	DBG(5,"+Cabal is now %i (max:%i)\n",cabal,max_cabal);
}
void tNetSession::decreaseCabal(bool partial) {
	if(!cabal) return;
	U32 min_cabal=maxPacketSz;
	U32 delta=100;
	U32 gamma=333;
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
	if(psize<4000) {
		if (cabal) return((psize*1000000)/cabal);
		return((psize*1000000)/4098);
	} else {
		if (cabal) return(((psize*1000)/cabal)*1000);
		return(((psize*1000)/4098)*1000);
	}
}

tNetSessionIte tNetSession::getIte() {
	return(tNetSessionIte(ip,port,sid));
}
bool tNetSession::blockMsg() {
	return delayMessages || (ackq->len() > 0);
}

void tNetSession::processMsg(Byte * buf,int size) {
	DBG(5,"Message of %i bytes\n",size);
	//stamp
	timestamp.seconds=alcGetTime();
	timestamp.microseconds=alcGetMicroseconds();
	
	int ret; //,ret2;
	
	ret=alcUruValidatePacket(buf,size,&validation,authenticated==1,passwd); // authenticated != 0 is not enough, the passwd is only set when authenticated == 1
	
	if(ret!=0 && (ret!=1 || net->flags & UNET_ECRC)) {
		if(ret==1) net->err->log("ERR: Failed Validating a message!\n");
		else net->err->log("ERR: Non-Uru protocol packet recieved!\n");
		return;
	}
	
	#ifdef ENABLE_MSGDEBUG
	net->log->log("<RCV> RAW Packet follows: \n");
	net->log->dumpbuf(buf,size);
	net->log->nl();
	#endif
	
	tSBuf mbuf(buf,size);
	
	tUnetUruMsg msg;
	
	try {
		mbuf.get(msg);
		#ifdef ENABLE_MSGDEBUG
		net->log->log("<RCV> ");
		msg.dumpheader(net->log);
		net->log->nl();
		#endif
		msg.htmlDumpHeader(net->ack,0,ip,port);
	} catch(txUnexpectedData &t) {
		net->err->log("%s Unexpected Data %s\nBacktrace:%s\n",str(),t.what(),t.backtrace());
		throw txProtocolError(_WHERE("Cannot parse a message"));
	}
	
	//set avg cabal
	if(cabal==0 && bandwidth!=0) {
		if((ntohl(ip) & 0xFFFFFF00) == 0x7F000000) { //lo
			cabal=100000000/8; //100Mbps
		} else if((ip & net->lan_mask) == net->lan_addr) { //LAN
			cabal=((net->lan_up > bandwidth) ? bandwidth : net->lan_up) / 8;
		} else { //WAN
			cabal=((net->nat_up > bandwidth) ? bandwidth : net->nat_up) / 8;
		}
		max_cabal=cabal;
		cabal=(cabal * 250)/1000;
		//cabal=maxPacketSz;
		DBG(5, "INF: Cabal is now %i (%i bps) max: %i (%i bps)\n",cabal,cabal*8,max_cabal,max_cabal*8);
		negotiating=false;
	}
	//How do you say "Cabal" in English?
	
	//Protocol Upgrade
	if(msg.tf & UNetExt) {
		cflags |= UNetUpgraded;
	}

	/* //Avoid sending ack for messages out of the window
	if(msg.tf & UNetAckReq) {
		//ack reply
		createAckReply(msg);
	}*/
	if(msg.tf & UNetNegotiation) {
		tmNetClientComm comm(this);
		msg.data.rewind();
		msg.data.get(comm);
		net->log->log("<RCV> ");
		net->log->print("%s",(const char *)comm.str());
		bandwidth=comm.bandwidth;
		if(renego_stamp==comm.timestamp) {
			net->log->print(" (ignored)\n");
			negotiating=false;
		} else {
			net->log->nl();
			renego_stamp=comm.timestamp;
			//reset counters and window
			wite=msg.sn; //reset win iterator
			memset(w,0,sizeof(char) * rcv_win); //unset all
			if(!negotiating) {
				//clear snd buffer
				DBG(5,"Clearing send buffer\n");
				sndq->clear();
				if(nego_stamp.seconds==0) nego_stamp=timestamp;
				negotiate();
				negotiating=true;
			}
			cabal=0;
		}
	} else if(bandwidth==0 || cabal==0) {
		if(!negotiating) {
			nego_stamp=timestamp;
			negotiate();
			negotiating=true;
		}
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
		negotiating=true;
	}

	//check duplicates
	ret=checkDuplicate(msg);
	
	if(ret!=2 && (msg.tf & UNetAckReq)) {
		//ack reply
		createAckReply(msg);
	}

	if(ret==0) {
	
		if(msg.tf & UNetAckReply) {
			//ack update
			ackCheck(msg);
			if(authenticated==2) authenticated=1;
		} else {
			if((msg.tf & UNetAckReq) && (msg.frn==0) && (net->flags & UNET_NOFLOOD)) { //flood control
				if(net->ntime_sec - flood_last_check > net->flood_check_sec) {
					flood_last_check=net->ntime_sec;
					flood_npkts=0;
				} else {
					flood_npkts++;
					if(flood_npkts>net->max_flood_pkts) {
						//UNET_FLOOD event
						tNetSessionIte ite(ip,port,sid);
						tNetEvent * evt=new tNetEvent(ite,UNET_FLOOD);
						net->events->add(evt);
					}
				}
			} //end flood control
			
			if(!(msg.tf & UNetNegotiation)) {
				assembleMessage(msg);
			}
		}
	}
	
	doWork();
}

void tNetSession::assembleMessage(tUnetUruMsg &t) {
	U32 frg_size=maxPacketSz - t.hSize();
	tUnetMsg * msg;
	rcvq->rewind();
	msg=rcvq->getNext();
	while(msg!=NULL) {
		if(msg->completed==0 && (net->ntime_sec - msg->stamp) > net->snd_expire) {
			net->err->log("%s INF: Message %i expired!\n",str(),msg->sn);
			rcvq->deleteCurrent();
			msg=rcvq->getCurrent();
		} else if(msg->sn==t.sn) {
			break;
		} else if(t.sn<msg->sn) { // the rcvq is ordered by SN, so if the new message has an SN smaller than the current one
								  // and bigger or equal to the last one (since the if wasn't executed then), this is the right place
			//tUnetMsg *msgalt = msg;
			msg=new tUnetMsg((t.frt+1) * frg_size);
			msg->sn=t.sn;
			msg->frt=t.frt;
			msg->hsize=t.hSize();
			rcvq->insertBefore(msg);
			//t.data.rewind();
			//DBG(7, "%s inserted a message %s (SN: %d) before an", str(), alcUnetGetMsgCode(t.data.getU16()), t.sn);
			//DBGM(7, " %s (SN: %d)\n", alcUnetGetMsgCode(msgalt->cmd), msgalt->sn);
			//t.data.rewind();
			msg->data->setSize((t.frt +1) * frg_size);
			break;
		} else {
			msg=rcvq->getNext();
		}
	}
	if(msg==NULL) {
		msg=new tUnetMsg((t.frt+1) * frg_size);
		msg->sn=t.sn;
		msg->frt=t.frt;
		msg->hsize=t.hSize();
		rcvq->add(msg);
		msg->data->setSize((t.frt +1) * frg_size);
	}
	msg->stamp=net->ntime_sec;
	if(msg->frt!=t.frt || msg->hsize!=t.hSize()) throw(txProtocolError(_WHERE("Inconsistency on fragmented stream %i %i %i %i",msg->frt,t.frt,msg->hsize,t.hSize())));
	
	if(!((msg->check[t.frn/8] >> (t.frn%8)) & 0x01)) {
		//found missing fragment
		msg->check[t.frn/8] |= (0x01<<(t.frn%8));
		
		if(t.frn==t.frt) { 
			msg->data->setSize((t.frt * frg_size) + t.data.size());
		}
		
		msg->data->set(t.frn * frg_size);
		t.data.rewind();
		msg->data->write(t.data.read(),t.data.size());

		if(msg->fr_count==t.frt) {
			msg->completed=0x01;
			msg->data->rewind();
			msg->cmd=msg->data->getU16();
			msg->data->rewind();
			//tNetSessionIte ite(ip,port,sid);
			//tNetEvent * evt=new tNetEvent(ite,UNET_MSGRCV);
			//net->events->add(evt);
		} else {
			msg->fr_count++;
		}
	}

}

/**
	\return 2 - After the window (neither parse nor send ack)
	\return 1 - Marked or before the window (send ack, but don't parse)
	\return 0 - Non-Marked (parse and send ack)
*/
Byte tNetSession::checkDuplicate(tUnetUruMsg &msg) {
	//drop already parsed messages
	#ifdef ENABLE_MSGDEBUG
	net->log->log("%s INF: SN %i (Before) window is (wite: %d):\n",str(),msg.sn,wite);
	net->log->dumpbuf((Byte *)w,rcv_win);
	net->log->nl();
	net->log->flush();
	#endif
	
	// Note: for fragmented messages, already parsed packets can not be dropped as all fragments have the same SN.
	// As a result of this, a fragmented message will be parsed twice if it is recieved twice.
	// A solution would be to only mark the packet as recieved after all fragments were recieved, but
	//  that would have to be done in tNetSession::assembleMessage
	
	if(msg.sn >= (wite+rcv_win)) {
		net->err->log("%s INF: Dropped packet %i after the window by sn (wite: %i)\n",str(),msg.sn,wite);
		net->err->flush();
		return 2;
	} else if(msg.sn < wite) { // this means we already parsed it
		if (msg.frn == 0) {
			net->err->log("%s INF: Dropped packet %i before the window by sn (wite: %i)\n",str(),msg.sn,wite);
			net->err->flush();
			return 1;
		} else { // it's fragmented but out of the window, so we can't do anything (see above)
			return 0; 
		}
	} else { //then check if is already marked
		U32 i,start,ck;
		start=wite % (rcv_win*8);
		i=msg.sn % (rcv_win*8);
		if(((w[i/8] >> (i%8)) & 0x01) && msg.frn==0) { // don't drop fragmented messages, see above
			net->err->log("%s INF: Dropped already parsed packet %i\n",str(),msg.sn);
			ack_rtt=ack_rtt/4;
			net->err->flush();
			return 1;
		} else {
			w[i/8] |= (0x01<<(i%8)); //activate bit
			while((i==start && ((w[i/8] >> (i%8)) & 0x01))) { //move the iterator
				w[i/8] &= ~(0x01<<(i%8)); //deactivate bit
				i++; start++;
				wite++;
				if(i>=rcv_win*8) { i=0; start=0; }
				#ifdef ENABLE_MSGDEBUG
				net->log->log("%s INF: A bit was deactivated (1)\n",str());
				#endif
			}
			ck=(i < start ? i+(rcv_win*8) : i);
			while((ck-start)>((rcv_win*8)/2)) {
				DBG(9,"ck: %i,start:%i\n",ck,start);
				w[start/8] &= ~(0x01<<(start%8)); //deactivate bit
				start++;
				wite++;
				if(start>=rcv_win*8) { start=0; ck=i; }
				#ifdef ENABLE_MSGDEBUG
				net->log->log("%s INF: A bit was deactivated (2)\n",str());
				#endif
			}
			#ifdef ENABLE_MSGDEBUG
			net->log->log("%s INF: Packet %i accepted to be parsed\n",str(),msg.sn);
			#endif
		}
	}
	#ifdef ENABLE_MSGDEBUG
	net->log->log("%s INF: SN %i (after) window is (wite: %d):\n",str(),msg.sn,wite);
	net->log->dumpbuf((Byte *)w,rcv_win);
	net->log->nl();
	net->log->flush();
	#endif
	return 0;
}


void tNetSession::createAckReply(tUnetUruMsg &msg) {
	tUnetAck * ack,* cack;
	U32 A,B;
	
	#ifdef ENABLE_ACKDEBUG
	net->log->log("stacking ack %i,%i %i,%i\n",msg.sn,msg.frn,msg.ps,msg.pfr);
	#endif
	
	A=msg.csn;
	B=msg.cps;
	
	ack=new tUnetAck();
	ack->A=msg.csn;
	ack->B=msg.cps;
	U32 tts=0;
	tts=computetts((((U32)msg.frt-msg.frn)+1) * maxPacketSz);
	//tts=computetts(2*maxPacketSz);
	if(tts>ack_rtt) tts=ack_rtt;
	net->updatetimer(tts);
	ack->timestamp=net->net_time + tts;
	#ifdef ENABLE_MSGDEBUG
	net->log->log("tts: %i, %i, %i\n",msg.frt,tts,cabal);
	#endif
	
	int i=0;

#if 0
	//Non-Plasma like ack's
	ackq->add(ack);
#else
	//Plasma like ack's (acks are retarded, and packed)
	ackq->rewind();
	if(ackq->getNext()==NULL) {
		ackq->add(ack);
		ack=NULL;
	} else {
		ackq->rewind();
		while((cack=ackq->getNext())!=NULL) {
			#ifdef ENABLE_ACKDEBUG
			net->log->log("2store[%i] %i,%i %i,%i\n",i,(ack->A & 0x000000FF),(ack->A >> 8),(ack->B & 0x000000FF),(ack->B >> 8));
			net->log->log("2check[%i] %i,%i %i,%i\n",i,(cack->A & 0x000000FF),(cack->A >> 8),(cack->B & 0x000000FF),(cack->B >> 8));
			#endif

			i++;
			if(A>=cack->A && B<=cack->A) {
				#ifdef ENABLE_ACKDEBUG
				net->log->log("A\n");
				#endif
				if(cack->next==NULL) {
					cack->A=A;
					cack->B=(cack->B > B ? B : cack->B);
					break;
				} else {
					B=ack->B=(cack->B > B ? B : cack->B);
					ack->timestamp=cack->timestamp;
					ackq->deleteCurrent();
					ackq->rewind();
					continue;
				}
			} else if(B>cack->A) {
				#ifdef ENABLE_ACKDEBUG
				net->log->log("B\n");
				#endif
				if(cack->next==NULL) { ackq->add(ack); ack=NULL; break; }
				else continue;
			} if(A<cack->B) {
				#ifdef ENABLE_ACKDEBUG
				net->log->log("C\n");
				#endif
				ackq->insertBefore(ack);
				ack=NULL;
				break;
			} else if(A<=cack->A && A>=cack->B) {
				#ifdef ENABLE_ACKDEBUG
				net->log->log("D\n");
				#endif
				A=ack->A=cack->A;
				B=ack->B=(cack->B > B ? B : cack->B);
				ack->timestamp=cack->timestamp;
				bool isNull=(cack->next==NULL);
				ackq->deleteCurrent();
				ackq->rewind();
				if(isNull) { ackq->add(ack); ack=NULL; break; }
				else continue;
			}
		}
	}
	if(ack!=NULL) {
		delete ack;
	}
	
	#ifdef ENABLE_ACKDEBUG
	net->log->log("ack stack TAIL looks like:\n");
	i=0;
	while((cack=ackq->getNext())!=NULL) {
		net->log->log("st-ack[%i] %i,%i %i,%i\n",i++,(cack->A & 0x000000FF),(cack->A >> 8),(cack->B & 0x000000FF),(cack->B >> 8));
	}
	#endif
#endif

	#ifdef ENABLE_ACKDEBUG
	net->log->log("ack stack looks like:\n");
	ackq->rewind();
	i=0;
	while((cack=ackq->getNext())!=NULL) {
		net->log->log("st-ack[%i] %i,%i %i,%i\n",i++,(cack->A & 0x000000FF),(cack->A >> 8),(cack->B & 0x000000FF),(cack->B >> 8));
	}
	#endif
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

	if(server.tf & UNetAckReq) {
		server.ps=server.sn;
		server.pfr=server.frn;
		//the second field, only contains the seq number from the latest packet with
		//the ack flag enabled.
		DBG(8,"The previous sent packet had the ack flag on\n");
	}

	server.frn=0;
	server.tf=UNetAckReply;
	server.val=validation;
	if(cflags & UNetUpgraded) {
		server.tf = UNetAckReply | UNetExt;
		server.val = 0x00;
	}
	if(server.val==0x01) { server.val=0x00; }
	if(server.val==0x00) { hsize=28; } else { hsize=32; }
	if(server.tf & UNetExt) { hsize-=8; }

	ackq->rewind();
	while((ack=ackq->getNext())) {
	
		pmsg=new tUnetUruMsg;

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

		tts=0;
		/*
		if(!(pmsg->tf & UNetExt))
			tts=computetts((i*16)+2+hsize+net->ip_overhead);
		else
			tts=computetts((i*8)+hsize+net->ip_overhead);
		*/
			
		#ifdef ENABLE_NETDEBUG
		tts+=net->latency;
		#endif
		//tts=0;
		pmsg->timestamp+=tts;
		net->updatetimer(tts);
		
		//put pmsg to the qeue
#if 0
		//ack are at the end of the qeue
		sndq->add(pmsg);
#else
		//ensure acks to be present at the top of the qeue
		if(sndq->isEmpty()) {
			sndq->add(pmsg);
			DBG(5,"Ack inserted into void msg qeue\n");
		} else {
			tUnetUruMsg * kiwi;
			sndq->rewind();
			kiwi=sndq->getNext();
			DBG(5,"ack checking q...\n");
			while(kiwi!=NULL && (kiwi->tf & 0x80)) {
				kiwi=sndq->getNext();
				DBG(5,"sndq->getNext()\n");
			}
			sndq->insertBefore(pmsg);
			DBG(5,"insertBefore\n");
		}
		//abort();
#endif
	}
	
}

void tNetSession::ackCheck(tUnetUruMsg &t) {

	U32 i,A1,A2,A3;
	U32 sn,ps;
	Byte frn,pfr;

	#ifdef ENABLE_MSGDEBUG
	net->log->log("<RCV>");
	#endif
	t.data.rewind();
	if(!(t.tf & UNetExt)) {
		if(t.data.getU16()!=0) throw txUnexpectedData(_WHERE("ack unknown data"));
	}
	for(i=0; i<t.dsize; i++) {
		A1=t.data.getU32();
		if(!(t.tf & UNetExt))
			if(t.data.getU32()!=0) throw txUnexpectedData(_WHERE("ack unknown data"));
		A3=t.data.getU32();
		if(!(t.tf & UNetExt))
			if(t.data.getU32()!=0) throw txUnexpectedData(_WHERE("ack unknown data"));
		frn=A1 & 0x000000FF;
		pfr=A3 & 0x000000FF;
		sn=A1 >> 8;
		ps=A3 >> 8;
		#ifdef ENABLE_MSGDEBUG
		if(i!=0) net->log->print("    |");
		net->log->print(" Ack %i,%i %i,%i\n",sn,frn,ps,pfr);
		#endif
		//well, do it
		tUnetUruMsg * msg=NULL;
		sndq->rewind();
		msg=sndq->getNext();
		while((msg!=NULL)) {
			A2=msg->csn;
			
			if(A1>=A2 && A2>A3) {
				//then delete
				#ifdef ENABLE_MSGDEBUG
				net->log->log("Deleting packet %i,%i\n",msg->sn,msg->frn);
				#endif
				if(msg->tryes==1 && A1==A2) {
					U32 crtt=net->net_time-msg->snd_timestamp;
					#ifdef ENABLE_NETDEBUG
					crtt+=net->latency;
					#endif
					updateRTT(crtt);
					increaseCabal();
				}
				if(msg->tf & UNetAckReq) {
					sndq->deleteCurrent();
					msg=sndq->getCurrent();
				} else {
					msg=sndq->getNext();
				}
			} else {
				//Force re-transmission
				if((msg->tf & UNetAckReq) && A3>=A2 && msg->tryes==1) {
					////msg->timestamp-=timeout/2;
				}
				msg=sndq->getNext();
			}
		}
	}

}

//Send, and re-send messages
void tNetSession::doWork() {

	tNetEvent * evt;
	tNetSessionIte ite(ip,port,sid);

	idle=false;

	ackUpdate(); //generate ack messages
	
	//check rcvq
	if(!blockMsg()) {
		rcvq->rewind();
		tUnetMsg * g;
		while((g=rcvq->getNext())) {
			if(g->completed==0x01) {
				evt=new tNetEvent(ite,UNET_MSGRCV);
				net->events->add(evt);
				break;
			}
		}
	}

	if(net->net_time>=last_msg_time) {

		if(sndq->isEmpty()) {
			last_msg_time=0;
			if(ackq->isEmpty() && rcvq->isEmpty()) {
				idle=true;
			} else {
				idle=false;
			}
			return;
		}
		sndq->rewind();
		tUnetUruMsg * curmsg;
		
		U32 quota_max=maxPacketSz;
		U32 cur_quota=0;
		
		U32 minTH=10;
		U32 maxTH=150;
		U32 tts;

		while((curmsg=sndq->getNext())!=NULL && (cur_quota<quota_max)) {
			
			if(curmsg->timestamp<=net->net_time) {
				//we can send the message
				if(curmsg->tf & UNetAckReply) {
					//send paquet
					cur_quota+=curmsg->size();
					net->rawsend(this,curmsg);
					sndq->deleteCurrent();
				} else if(curmsg->tf & UNetAckReq) {
					// if we did not yet get a nego, send only negos, as otherwise the peer might get a non-nego before the first nego... chaos!
					 if (wite == 0 && !(curmsg->tf & UNetNegotiation)) continue;
					//send paquet
					
					// check if we need to resend
					if(curmsg->tryes!=0) {
						//abort();
						if(curmsg->tryes==1) {
							decreaseCabal(true);
						} else {
							decreaseCabal(false);
						}
						duplicateTimeout();
					}
					
					if(curmsg->tryes>=12 || (curmsg->tryes>=2 && terminated)) { // only 1 resend on terminated connections
						sndq->deleteCurrent();
						//timeout event
						evt=new tNetEvent(ite,UNET_TIMEOUT);
						net->events->add(evt);
					} else {
						cur_quota+=curmsg->size();
						curmsg->snd_timestamp=net->net_time;
						net->rawsend(this,curmsg);
						curmsg->timestamp=net->net_time+timeout;
						curmsg->tryes++;
					}
				} else {
					//probabilistic drop (of voice, and other non-ack paquets)
					if(net->net_time-curmsg->timestamp > 4*timeout) {
						//Unacceptable - drop it
						net->err->log("Dropped a 0x00 packet due to unaceptable msg time %i,%i,%i\n",timeout,net->net_time-curmsg->timestamp,rtt);
						sndq->deleteCurrent();
					} else if(sndq->len()>minTH && (random()%maxTH)>sndq->len()) {
						net->err->log("Dropped a 0x00 packet due to a big queue\n");
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
	} else { DBG(8,"Too soon to check sndq\n"); }
}

void tNetSession::negotiate() {
	U32 sbw;
	//server bandwidth
	DBG(9,"%08X %08X %08X\n",ip,net->lan_mask,net->lan_addr);
	if((ntohl(ip) & 0xFFFFFF00) == 0x7F000000) { //lo
		sbw=100000000;
	} else if((ip & net->lan_mask) == net->lan_addr) { //LAN
		sbw=net->lan_down;
	} else {
		sbw=net->nat_down; //WAN
	}

	tmNetClientComm comm(nego_stamp,sbw,this);
	net->basesend(this,comm);
}

void tNetSession::checkAlive(void)
{	// when we are talking to a non-terminated server, send alive messages
	if (!client && !terminated && (alcGetTime() - timestamp.seconds) > conn_timeout/2) {
		tmAlive alive(this);
		net->send(alive);
		timestamp.seconds = alcGetTime();
	}
}

U32 tNetSession::onlineTime(void)
{
	return alcGetTime()-nego_stamp.seconds;
}

/* End session */

}


