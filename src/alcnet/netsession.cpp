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
	flood_last_check=0;
	flood_npkts=0;
	bandwidth=0;
	cabal=0;
	max_cabal=0;
	next_msg_time=0;
	rtt=0;
	deviation=0;
	timeout=net->timeout;
	conn_timeout=net->conn_timeout;
	nego_stamp.seconds=0;
	nego_stamp.microseconds=0;
	renego_stamp=nego_stamp;
	negotiating=false;
	resetMsgCounters();
	assert(serverMsg.pn==0);
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
	inRoute = false;
	
	DBG(5, "%s Initial timeout: %d\n", str(), timeout);
}
void tNetSession::resetMsgCounters(void) {
	clientPs = 0;
	waitingForFragments = false;
	serverMsg.pn=0;
	serverMsg.sn=0;
	serverMsg.pfr=0;
	serverMsg.ps=0;
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

// functions to calculate cabal and rtt
void tNetSession::updateRTT(U32 newread) {
	if(rtt==0) rtt=newread;
	#if 0 //Original
		const U32 alpha=800;
		rtt=((alpha*rtt)/1000) + (((1000-alpha)*newread)/1000);
		timeout=2*rtt;
	#else //Jacobson/Karels
		S32 alpha=125; // this is effectively 0.125
		S32 u=2;
		S32 delta=4;
		S32 diff=(S32)newread - (S32)rtt;
		rtt=(S32)rtt+((alpha*diff)/1000);
		deviation+=(alpha*(abs(diff)-deviation))/1000;
		timeout=u*rtt + delta*deviation;
	#endif
	if (timeout > 4000000) timeout = 4000000;
	DBG(5,"%s RTT update (sample rtt: %i) new rtt:%i, timeout:%i, deviation:%i\n", str(),newread,rtt,timeout,deviation);
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
	U32 delta=100;
	U32 gamma=333;
	if(partial) cabal-=(gamma*cabal)/1000;
	else {
		cabal=cabal/2;
		max_cabal-=(delta*max_cabal)/1000;
		if(max_cabal<maxPacketSz) max_cabal=maxPacketSz;
	}
	if(cabal<maxPacketSz) cabal=maxPacketSz;
	DBG(5,"-Cabal is now %i (max:%i)\n",cabal,max_cabal);
}

/** computes the time we have to wait after sending the given amount of bytes */
U32 tNetSession::timeToSend(U32 psize) {
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

/**
	puts the message in the session's send queue
	(only Nego and normal messages, ack are handled by another function)
*/
void tNetSession::send(tmBase &msg) {
	net->log->log("<SND> %s\n",msg.str());
	tMBuf buf;
	U32 csize,psize,hsize,pkt_sz,n_pkts;
	Byte flags=msg.bhflags, val, tf;
	
	net->updateNetTime();
	
	tUnetUruMsg * pmsg=NULL;
	
	buf.put(msg);
	psize=buf.size();
	buf.rewind();
	DBG(7,"Ok, I'm going to send a packet of %i bytes, for peer %i, with flags %02X\n",psize,sid,msg.bhflags);

	//now update the other fields
	serverMsg.sn++;
	tf=0x00;

	if(flags & UNetAckReq) {
		tf |= UNetAckReq; //ack flag on
		DBG(7,"ack flag on\n");
	}
	if(flags & UNetNegotiation) {
		tf |= UNetNegotiation; //negotiation packet
		DBG(7,"It's a negotation packet\n");
	}
	if(flags & UNetAckReply) {
		tf |= UNetAckReply; //ack packet
		DBG(7,"It's an ack packet\n");
	}

	if(flags & UNetForce0) {
		val=0x00;
		DBG(7,"forced validation 0\n");
	} else {
		val=validation;
		DBG(7,"validation level is %i\n",val);
	}
	
	//check if we are using alcugs upgraded protocol
	if((cflags & UNetUpgraded) || (flags & UNetExt)) {
		val=0x00;
		tf |= UNetExt;
		DBG(5,"Sending an Alcugs Extended paquet\n");
	}

	//On validation level 1 - ack and negotiations don't have checksum verification
	/* I still don't understand wtf was thinking the network dessigner with doing a MD5 of each
		packet?
	*/
	if((tf & (UNetNegotiation | UNetAckReply)) && (val==0x01)) { val=0x00; }
	DBG(6,"Sending a packet of validation level %i\n",val);

	//fragment the messages and put them in to the send qeue
	
	if(val==0x00) { hsize=28; } else { hsize=32; }
	if(tf & UNetExt) { hsize-=8; }

	pkt_sz=maxPacketSz - hsize; //get maxium message size
	n_pkts=(psize-1)/pkt_sz; //get number of fragments
	DBG(5,"pkt_sz:%i n_pkts:%i\n",pkt_sz,n_pkts);
	if(n_pkts>=256) {
		net->err->log("%s ERR: Attempted to send a packet of size %i bytes, that don't fits inside an uru message\n",str(),psize);
		throw txTooBig(_WHERE("%s packet of %i bytes don't fits inside an uru message\n",str(),psize));
	}
	
	U32 i,tts=0;
	
	for(i=0; i<=n_pkts; i++) {
		//get current paquet size
		if(i==n_pkts) csize=psize - (i*pkt_sz);
		else csize=pkt_sz;
		
		pmsg=new tUnetUruMsg();
		pmsg->val=val;
		//pmsg.pn NOT in this layer (done in the msg sender, tUnet::rawsend)
		//since acks are put at the top of the sndq, the pn would be wrong if we did it differently
		pmsg->tf=tf;
		pmsg->frn=i;
		pmsg->sn=serverMsg.sn;
		pmsg->frt=n_pkts;
		pmsg->pfr=serverMsg.pfr;
		pmsg->ps=serverMsg.ps;
		
		pmsg->data.write(buf.read(csize),csize);
		pmsg->_update();
		
		pmsg->timestamp=net->net_time+tts;
		tts+=timeToSend(csize+hsize+net->ip_overhead);
		
		#ifdef ENABLE_NETDEBUG
		pmsg->timestamp+=net->latency;
		#endif
		
		pmsg->snd_timestamp=pmsg->timestamp;
		
		//Urgent!?
		if(flags & UNetUrgent) {
			net->rawsend(this,pmsg);
			if(flags & UNetAckReq) {
				pmsg->snd_timestamp=net->net_time;
				pmsg->timestamp+=timeout;
				++pmsg->tryes;
				sndq->add(pmsg);
			}
			else
				delete pmsg;
		} else {
			//put pmsg to the qeue
			sndq->add(pmsg);
		}
		
		if(tf & UNetAckReq) { // if this packet has the ack flag on, save it's number
			serverMsg.ps=serverMsg.sn;
			serverMsg.pfr=i;
		}
	}
	
	doWork(); //send messages
}

/** process a recieved msg: put it in the rcvq, assemble fragments, create akcs */
void tNetSession::processMsg(Byte * buf,int size) {
	DBG(5,"Message of %i bytes\n",size);
	//stamp
	timestamp=net->ntime;
	
	int ret;
	
	// when authenticated == 2, we don't expect an encoded packet, but sometimes, we get one. Since alcUruValidatePacket will try to
	// validate the packet both with and without passwd if possible, we tell it to use the passwd whenever we have one - as a result,
	// even if the client for some reason decides to encode a packet while authenticated == 2, we don't care
	ret=alcUruValidatePacket(buf,size,&validation,authenticated==1 || authenticated==2,passwd);
	
	if(ret!=0 && (ret!=1 || net->flags & UNET_ECRC)) {
		if(ret==1) net->err->log("ERR: %s Failed Validating a message!\n", str());
		else net->err->log("ERR: %s Non-Uru protocol packet recieved!\n", str());
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
	
	// if we know the downstream of the peer, set avg cabal using what is smaller: our upstream or the peers downstream
	//  (this is the last part of the negotiationg process)
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
		DBG(5, "INF: Cabal is now %i (%i bps) max: %i (%i bps)\n",cabal,cabal*8,max_cabal,max_cabal*8);
		negotiating=false;
	}
	//How do you say "Cabal" in English?
	
	//Protocol Upgrade
	if(msg.tf & UNetExt) {
		cflags |= UNetUpgraded;
	}

	if(msg.tf & UNetNegotiation) {
		tmNetClientComm comm(this);
		msg.data.rewind();
		msg.data.get(comm);
		net->log->log("<RCV> ");
		net->log->print("%s",(const char *)comm.str());
		bandwidth=comm.bandwidth;
		if(renego_stamp==comm.timestamp) { // it's a duplicate, we already got this message
		    // It is necessary to do the check this way since the usual check by SN would treat a nego on an existing connection as
		    //  "already parsed" since the SN is started from the beginning
			net->log->print(" (ignored)\n");
		} else {
			net->log->nl();
			renego_stamp=comm.timestamp;
			if(!negotiating) {
				// if this nego came unexpectedly, reset everything and send a nego back (since the other peer expects our answer, this
				//  will not result in an endless loop of negos being exchanged)
				resetMsgCounters();
				//clear buffers
				DBG(5,"Clearing buffers\n");
				sndq->clear();
				ackq->clear();
				if(nego_stamp.seconds==0) nego_stamp=timestamp;
				negotiate();
			}
			cabal=0; // re-determine cabal with the new bandwidth
		}
	} else if(bandwidth==0 || cabal==0 || (clientPs==0 && msg.ps != 0)) { // we did not yet negotiate, or there obviously was a problem negotiating
		if(!negotiating) { // and we are not in the process of doing it - so start that process
			nego_stamp=timestamp;
			negotiate();
		}
	}
	
	//fix the problem that happens every 15-30 days of server uptime (prefer doing that when the sndq is empty)
	if((sndq->len() == 0 && (serverMsg.sn>=8378605 || msg.sn>=8378605)) ||
		(serverMsg.sn>=8388605 || msg.sn>=8388605) ) { // that's aproximately 2^23
		net->log->log("%s WARN: Congratulations! You have reached the maxium allowed sequence number, don't worry, this is not an error\n", str());
		net->log->flush();
		resetMsgCounters();
		//clear buffers
		DBG(5,"Clearing buffers\n");
		sndq->clear();
		ackq->clear();
		nego_stamp=timestamp;
		renego_stamp.seconds=0;
		negotiate();
	}

	//check duplicates
	ret=checkDuplicate(msg);

	// if the packet requires it and the above chack told us that'd be ok, send an ack
	if(ret!=2 && (msg.tf & UNetAckReq)) {
		//ack reply
		createAckReply(msg);
	}

	if(isConnected() && (msg.tf & UNetAckReply)) { // if we are connected, we can parse acks out of order (if not, the ack sent in reply
	                                               //  to the nego serves to set the cabal, so it has to be parsed in order)
		//ack update
		ackCheck(msg);
		if(authenticated==2) authenticated=1;
	}
	// if it is ok parse the packet, do that
	else if (ret == 0 && !(msg.tf & UNetAckReply)) {
		//flood control
		if((msg.tf & UNetAckReq) && (msg.frn==0) && (net->flags & UNET_NOFLOOD)) {
			if(net->ntime.seconds - flood_last_check > net->flood_check_sec) {
				flood_last_check=net->ntime.seconds;
				flood_npkts=0;
			} else {
				flood_npkts++;
				if(flood_npkts>net->max_flood_pkts) {
					// send UNET_FLOOD event
					tNetSessionIte ite(ip,port,sid);
					tNetEvent * evt=new tNetEvent(ite,UNET_FLOOD);
					net->events->add(evt);
				}
			}
		} // end flood control
		
		if(!(msg.tf & UNetNegotiation)) {
			assembleMessage(msg);
		}
	}
	
	doWork();
}

/** put this message in the rcvq and put fragments together */
void tNetSession::assembleMessage(tUnetUruMsg &t) {
	U32 frg_size=maxPacketSz - t.hSize();
	tUnetMsg * msg;
	rcvq->rewind();
	msg=rcvq->getNext();
	while(msg!=NULL) {
		if(msg->completed==0 && (net->ntime.seconds - msg->stamp) > net->snd_expire) {
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
	msg->stamp=net->ntime.seconds;
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

			if (t.sn == clientPs) {
				waitingForFragments = false;
			}
		} else {
			msg->fr_count++;

			if (t.sn == clientPs) {
				waitingForFragments = true;
			}
		}
	}

}

/**
	\return 2 - neither parse nor send ack (the packet is after what we expected)
	\return 1 - send ack, but don't parse (we already parsed the packet)
	\return 0 - parse and send ack (it's exactly what we need)
*/
Byte tNetSession::checkDuplicate(tUnetUruMsg &msg) {
	/* There is one problem with this way of checking duplicates: If the first packet of a fragmented message is lost, the additional ones
	   will be dropped. Since there is no sane way to work around that, we have to live with it. */
	#ifdef ENABLE_MSGDEBUG
	net->log->log("%s INF: last acked packet was %d (waitingForFragments: %d), checking new packet %d.%d\n", str(), clientPs, waitingForFragments, msg.sn, msg.frn);
	net->log->flush();
	#endif
	if (msg.ps < clientPs) { // we already got that one - ack it, but don't parse again
		net->log->log("%s INF: Dropped already parsed packet %d.%d (last ack is %d, expected %d)\n", str(), msg.sn, msg.frn, msg.ps, clientPs);
		net->log->flush();
		return 1; // we already got it, send an ack
	}
	else if (msg.ps > clientPs) { // we missed something in between
		DBG(3, "clientLastSn:%d\n", clientLastSn);
		net->log->log("%s WARN: Dropped unexpected packet %d.%d (last ack is %d, expected %d)\n", str(), msg.sn, msg.frn, msg.ps, clientPs);
		net->log->flush();
		return 2; // we can not parse it, the peer has to send it again
	}
	else if (waitingForFragments && msg.sn != clientPs) { // we have a not-yet complete message and this packet does not belong to it
		net->log->log("%s WARN: Dropped unexpected packet %d.%d (%d is not yet complete)\n", str(), msg.sn, msg.frn, clientPs);
		net->log->flush();
		return 2; // we can not parse it, the peer has to send it again
	}
	else if (!waitingForFragments && msg.sn == clientPs) { // this message belongs to the one we already completed, so don't parse it again
		net->log->log("%s INF: Dropped already parsed packet %d.%d (%d is already complete)\n", str(), msg.sn, msg.frn, clientPs);
		net->log->flush();
		return 1; // we already got it, send an ack
	}
	DBG(5, "accepted packet %d.%d\n", msg.sn, msg.frn);
	
	if (msg.tf & UNetAckReq) {
		clientPs = msg.sn;
		#ifdef ENABLE_MSGDEBUG
		net->log->log("%s INF: last acked packet is now %d\n", str(), clientPs);
		#endif
	}
	return 0;
#if 0
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
	
	if (wite == 0 || msg.sn >= (wite+rcv_win)) {
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
#endif
}

/** creates an ack in the ackq */
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
	
	//we must delay either none or all messages, otherwise the rtt will vary too much
	U32 ackWaitTime=timeToSend((((U32)msg.frt-msg.frn)+1) * maxPacketSz); // this is how long transmitting the whole packet will approximately take
	if(ackWaitTime > timeout/4) ackWaitTime=timeout/4; // don't use the rtt as basis, it is 0 at the beginning, resulting in a much too quick first answer, a much too low rtt on the other side and thus the packets being re-sent too early
	ack->timestamp=net->net_time + ackWaitTime;
	// the net timer will be updated when the ackq is checked (which is done since processMsg will call doWork after calling createAckReply)
	DBG(3, "ack tts: %i, %i, %i\n",msg.frt,ackWaitTime,cabal);
	
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

/** puts acks from the ackq in the sndq */
void tNetSession::ackUpdate() {
	U32 i,maxacks=30,tts;
	Byte val, tf;
	if (ackq->len() == 0) return;

	tUnetUruMsg * pmsg;

	tf=UNetAckReply;
	val=validation;
	if(cflags & UNetUpgraded) {
		tf = UNetAckReply | UNetExt;
		val = 0x00;
	}
	if(val==0x01) { val=0x00; }

	ackq->rewind();
	tUnetAck *ack;
	while((ack = ackq->getNext())) {
		if (ack->timestamp > net->net_time) {
			net->updateTimerAbs(ack->timestamp); // come back when we want to process this ack
			continue;
		}
		
		// now we have at least one ack packet to send
		
		pmsg=new tUnetUruMsg;

		//now update the other fields
		serverMsg.sn++;
		
		pmsg->val=val;
		//pmsg.pn NOT in this layer (done in the msg sender, tUnet::rawsend)
		//since acks are put at the top of the sndq, the pn would be wrong if we did it differently
		pmsg->tf=tf;
		pmsg->frn=0;
		pmsg->sn=serverMsg.sn;
		pmsg->frt=0;
		pmsg->pfr=serverMsg.pfr;
		pmsg->ps=serverMsg.ps;

		if(!(pmsg->tf & UNetExt))
			pmsg->data.putU16(0);
		
		i=0;
		do {
			if (ack->timestamp > net->net_time) {
				net->updateTimerAbs(ack->timestamp); // come back when we want to process this ack
				continue;
			}
			pmsg->data.putU32(ack->A);
			if(!(pmsg->tf & UNetExt))
				pmsg->data.putU32(0);
			pmsg->data.putU32(ack->B);
			if(!(pmsg->tf & UNetExt))
				pmsg->data.putU32(0);
			ackq->deleteCurrent();
			i++;
		} while((ack = ackq->getNext()) && i<maxacks);
		assert(i>0);
		
		pmsg->_update();
		pmsg->timestamp=net->net_time;
		pmsg->dsize=i;

		tts=0;
		#ifdef ENABLE_NETDEBUG
		tts+=net->latency;
		#endif
		//tts=0;
		pmsg->timestamp+=tts;
		net->updateTimerAbs(pmsg->timestamp); // come back when we want to send this ack
		
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
			while(kiwi!=NULL && (kiwi->tf & UNetAckReply)) {
				kiwi=sndq->getNext();
				DBG(5,"sndq->getNext()\n");
			}
			sndq->insertBefore(pmsg);
			DBG(5,"insertBefore\n");
		}
#endif
	}
	
}

/** parse the ack and remove the messages it acks from the sndq */
void tNetSession::ackCheck(tUnetUruMsg &t) {

	U32 i,A1,A2,A3;
	#ifdef ENABLE_MSGDEBUG
	U32 sn,ps;
	Byte frn,pfr;
	
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
		#ifdef ENABLE_MSGDEBUG
		frn=A1 & 0x000000FF;
		pfr=A3 & 0x000000FF;
		sn=A1 >> 8;
		ps=A3 >> 8;
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
				if(msg->tryes<=1 && A1==A2) {
					/* possible problem: since this is the last packet which was acked with this ack message, it could be combined
					   with other packets and the ack could be sent almost immediately after the packet went in, without the
					   usual delay  so the rtt is much smaller than the average. But I don't expect this to happen often, so I don't
					   consider that a real problem. */
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
				#if 0
				//Force re-transmission
				if((msg->tf & UNetAckReq) && A3>=A2 && msg->tryes==1) {
					msg->timestamp-=timeout/2;
				}
				#endif
				msg=sndq->getNext();
			}
		}
	}

}

/** Send, and re-send messages, update idle state and set netcore timeout (the last is NECESSARY - otherwise, the idle timer will be used) */
void tNetSession::doWork() {

	tNetEvent * evt;
	tNetSessionIte ite(ip,port,sid);

	idle=false;

	ackUpdate(); //generate ack messages (i.e. put them from the ackq to the sndq)
	
	//check rcvq
	if(!delayMessages && (ackq->len() == 0)) {
		rcvq->rewind();
		tUnetMsg * g;
		while((g=rcvq->getNext())) {
			if (!g->completed) break; // if this is a non-completed message, don't parse it or what comes after it - it would be out of order
			evt=new tNetEvent(ite,UNET_MSGRCV);
			net->events->add(evt);
			break;
		}
	}
	
	if(sndq->isEmpty()) {
		next_msg_time=0;
		if(ackq->isEmpty() && rcvq->isEmpty()) {
			idle=true;
		}
		return;
	}

	if(net->net_time>=next_msg_time) {
		sndq->rewind();
		tUnetUruMsg * curmsg;
		
		U32 quota_max=maxPacketSz;
		U32 cur_quota=0;
		
		U32 minTH=10;
		U32 maxTH=150;
		U32 tts;

		while((curmsg=sndq->getNext())!=NULL && (cur_quota<quota_max)) {
			
			if(curmsg->timestamp<=net->net_time) {
				DBG(8, "%s ok to send a message\n",alcGetStrTime());
				//we can send the message
				if(curmsg->tf & UNetAckReply) {
					//send paquet
					cur_quota+=curmsg->size();
					net->rawsend(this,curmsg);
					sndq->deleteCurrent();
				} else if(curmsg->tf & UNetAckReq) {
					//send paquet
					
					// check if we need to resend
					if(curmsg->tryes!=0) {
						if(curmsg->tryes==1) {
							decreaseCabal(true);
						} else {
							decreaseCabal(false);
						}
						// The server used to duplicate the timeout here - but since the timeout will be overwritten next time updateRTT
						//  is called, that's of no use. So better make the RTT bigger - it is obviously at least the timeout
						// This will result in a more long-term reduction of the timeout
						updateRTT(timeout);
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
			 else {
			 	net->updateTimerAbs(curmsg->timestamp); // come back when we want to send this message
			 	DBG(8,"%s Too soon (%d) to send a message\n",alcGetStrTime(), curmsg->timestamp-net->net_time);
			 }
		} //end while
		tts=timeToSend(cur_quota);
		if (cur_quota > 0) {
			DBG(8,"%s tts is now:%i quota:%i,cabal:%i\n",alcGetStrTime(),tts,cur_quota,cabal);
		}
		next_msg_time=net->net_time + tts;
		net->updateTimerAbs(next_msg_time); // come back when we want to send the next message
	} else {
		net->updateTimerAbs(next_msg_time); // come back when we want to send the next message
		DBG(8,"Too soon (%d) to check sndq\n", next_msg_time-net->net_time);
	}
}

/** send a negotiation to the peer */
void tNetSession::negotiate() {
	U32 sbw;
	// send server downstream (for the peer to know how fast it can send packets)
	DBG(9,"%08X %08X %08X\n",ip,net->lan_mask,net->lan_addr);
	if((ntohl(ip) & 0xFFFFFF00) == 0x7F000000) { //lo
		sbw=100000000;
	} else if((ip & net->lan_mask) == net->lan_addr) { //LAN
		sbw=net->lan_down;
	} else {
		sbw=net->nat_down; //WAN
	}

	tmNetClientComm comm(nego_stamp,sbw,this);
	send(comm);
	negotiating = true;
}

void tNetSession::checkAlive(void)
{	// when we are talking to a non-terminated server, send alive messages
	if (!client && !terminated && (net->ntime.seconds - timestamp.seconds) > conn_timeout/2) {
		tmAlive alive(this);
		send(alive);
	}
}

U32 tNetSession::onlineTime(void)
{
	return alcGetTime()-nego_stamp.seconds;
}

/* End session */

}
