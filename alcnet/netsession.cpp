/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs Server Team                           *
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
#include <alcdefs.h>
#include "netsession.h"

#include "netsessionmgr.h"
#include "unet.h"
#include "netexception.h"
#include "protocol/umsgbasic.h"
#include <alcutil/alclog.h>
#include <alcutil/alcthread.h>
#include <alcmain.h>

#include <cassert>

namespace alc {

/* Session */
tNetSession::tNetSession(alc::tUnet* net, uint32_t ip, uint16_t port, uint32_t sid) {
	DBG(5,"tNetSession()\n");
	assert(alcGetSelfThreadId() == alcGetMain()->threadId()); // FIXME is this correct? will choke if netConnect is called in worker
	this->net=net;
	this->ip=ip;
	this->port=port;
	this->sid=sid;
	sndq = new tUnetMsgQ<tUnetUruMsg>;
	ackq = new tUnetMsgQ<tUnetAck>;
	rcvq = new tUnetMsgQ<tUnetUruMsg>;
	rcv = NULL;
	init();
	//new conn event
	tNetSessionIte ite(ip,port,sid);
	tNetEvent * evt=new tNetEvent(ite,UNET_NEWCONN);
	net->addEvent(evt);
}
tNetSession::~tNetSession() {
	DBG(5,"~tNetSession() (sndq: %ld)\n", sndq->len());
	delete data;
	delete rcv;
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
	maxBandwidth=minBandwidth=cabal=0;
	maxPacketSz=1024;
	flood_last_check=net->net_time;
	flood_npkts=0;
	activity_stamp = net->net_time;
	next_msg_time=0;
	rtt=0;
	deviation=0;
	msg_timeout=net->msg_timeout;
	conn_timeout=net->conn_timeout*1000*1000;
	nego_stamp.seconds=0;
	renego_stamp=nego_stamp;
	negotiating=false;
	resetMsgCounters();
	assert(serverMsg.pn==0);
	rejectMessages=false;
	whoami=0;
	max_version=0;
	min_version=0;
	tpots=0;
	ki=0;
	client = true;
	terminated = false;
	data = NULL;
	joined = false;
	
	DBG(5, "%s Initial msg_timeout: %d\n", str().c_str(), msg_timeout);
}
void tNetSession::resetMsgCounters(void) {
	DBG(3, "tNetSession::resetMsgCounters\n");
	clientMsg.pfr=0;
	clientMsg.ps=0;
	serverMsg.pn=0;
	serverMsg.sn=0;
	serverMsg.pfr=0;
	serverMsg.ps=0;
	// empty queues
	sndq->clear();
	ackq->clear();
	rcvq->clear();
	delete rcv;
	rcv = NULL;
}
int8_t tNetSession::compareMsgNumbers(uint32_t sn1, uint8_t fr1, uint32_t sn2, uint8_t fr2)
{
	if (sn1 < sn2 || (sn1 == sn2 && fr1 < fr2)) return -1;
	else if (sn1 == sn2 && fr1 == fr2) return 0;
	else return 1;
}
tString tNetSession::str(bool detail) {
	tString dbg;
	if (detail) dbg.printf("[%i]", sid);
	dbg.printf("[%s:%i]",alcGetStrIp(ip).c_str(),ntohs(port));
	if (!detail) return dbg;
	// detailed string
	if (!name.isEmpty() && authenticated != 0) {
		if (authenticated == 10) dbg.printf("[%s?]", name.c_str()); // if the auth server didn't yet confirm that, add a question mark
		else if (ki != 0) dbg.printf("[%s:%s,%d]", name.c_str(), avatar.c_str(), ki);
		else dbg.printf("[%s]", name.c_str());
	}
	else if (!name.isEmpty() && net->whoami == KTracking) { // we are tracking and this is a game server
		dbg.printf("[%s:%s]", name.c_str(), alcGetStrGuid(serverGuid).c_str());
	}
	else if (whoami != 0) {
		dbg.printf("[%s]", alcUnetGetDestination(whoami));
	}
	return dbg;
}

size_t tNetSession::getHeaderSize() {
	size_t my=28;
	if(validation>0) my+=4;
	if(cflags & UNetUpgraded) my-=8;
	return my;
}

// functions to calculate cabal and rtt
void tNetSession::updateRTT(tNetTimeDiff newread) {
	if(rtt==0) rtt=newread;
	//Jacobson/Karels
	const unsigned int alpha=125; // this is effectively 0.125
	const unsigned int u=2;
	const unsigned int delta=4;
	const int diff=newread - rtt;
	rtt       += (alpha*diff)/1000;
	deviation += (alpha*(abs(diff)-deviation))/1000;
	msg_timeout=u*rtt + delta*deviation;
	if (msg_timeout > 5000000) msg_timeout = 5000000; // max. timeout: 5secs
	DBG(5,"%s RTT update (sample rtt: %i) new rtt:%i, msg_timeout:%i, deviation:%i\n", str().c_str(),newread,rtt,msg_timeout,deviation);
}
void tNetSession::increaseCabal() {
	if(!cabal) return;
	unsigned int inc = 5000;
	if(cabal+inc > minBandwidth) inc /= 4;
	cabal+=inc;
	if(cabal > maxBandwidth) cabal = maxBandwidth;
	DBG(5,"%s +Cabal is now %i\n",str().c_str(),cabal);
}
void tNetSession::decreaseCabal(bool small) {
	if(!cabal) return;
	const unsigned int delta = small ? 200 : 333;
	unsigned int dec = (delta*cabal)/1000;
	if (cabal-dec < minBandwidth) dec /= 4;
	cabal -= dec;
	if(cabal < maxPacketSz) cabal = maxPacketSz;
	DBG(5,"%s -Cabal is now %i\n",str().c_str(),cabal);
}

/** computes the time we have to wait after sending the given amount of bytes */
tNetTimeDiff tNetSession::timeToSend(size_t psize) {
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
*/
void tNetSession::send(tmBase &msg, tNetTimeDiff delay) { // FIXME make thread-safe
#ifndef ENABLE_ACKLOG
	if (!(msg.bhflags & UNetAckReply))
#endif
	net->log->log("<SND> %s\n",msg.str().c_str());
	tMBuf buf;
	size_t csize,psize,hsize,pkt_sz;
	unsigned int n_pkts;
	uint8_t flags=msg.bhflags, val, tf;
	
	tUnetUruMsg * pmsg=NULL;
	
	if((cflags & UNetUpgraded)) msg.bhflags |= UNetExt; // tmNetAck has to know that it writes an extended packet
	buf.put(msg);
	psize=buf.size();
	buf.rewind();
	DBG(7,"Ok, I'm going to send a packet of %li bytes, for peer %i, with flags %02X\n",psize,sid,msg.bhflags);

	//now update the other fields
	serverMsg.sn++;
	tf=0x00;

	if((flags & UNetNegotiation) && (flags & UNetAckReply))
		throw txUnexpectedData(_WHERE("Flags UNetAckReply and UNetNegotiation cannot be set at the same time"));
	if((flags & UNetAckReq) && (flags & UNetAckReply))
		throw txUnexpectedData(_WHERE("Flags UNetAckReply and UNetAckReq cannot be set at the same time"));
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
		DBG(7,"Sending an Alcugs Extended packet\n");
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
	n_pkts=(psize-1)/pkt_sz; //get number of fragments (0 means everything is sent in one packet, which must be the case if psize == pkt_sz)
	DBG(5,"pkt_sz:%li n_pkts:%i\n",pkt_sz,n_pkts);
	if(n_pkts>=256) {
		net->err->log("%s ERR: Attempted to send a packet of size %i bytes, that don't fits inside an uru message\n",str().c_str(),psize);
		throw txTooBig(_WHERE("%s packet of %i bytes don't fits inside an uru message\n",str().c_str(),psize));
	}
	if (n_pkts > 0 && ((flags & UNetNegotiation) || (flags & UNetAckReply)))
		throw txProtocolError(_WHERE("Nego and ack packets must not be fragmented!"));
	
	
	delay *= 1000; // make it usecs
	for(unsigned int i=0; i<=n_pkts; i++) {
		//get current paquet size
		if(i==n_pkts) csize=psize - (i*pkt_sz);
		else csize=pkt_sz;
		
		pmsg=new tUnetUruMsg();
		pmsg->val=val;
		//pmsg.pn NOT in this layer (done in the msg sender, tUnet::rawsend)
		//since urgent messages are put at the top of the sndq, the pn would be wrong if we did it differently
		pmsg->tf=tf;
		pmsg->frn=i;
		pmsg->sn=serverMsg.sn;
		pmsg->frt=n_pkts;
		pmsg->pfr=serverMsg.pfr;
		pmsg->ps=serverMsg.ps;
		
		pmsg->data.write(buf.read(csize),csize);
		pmsg->_update();
		
		pmsg->timestamp=net->net_time+delay; // no need to take tts into account now, the send queue worker does that
		
		#ifdef ENABLE_NETDEBUG
		pmsg->timestamp+=net->latency;
		#endif
		
		//Urgent!?
		if(flags & UNetUrgent) { // FIXME this will break when called in worker
			net->rawsend(this,pmsg);
			//update time to send next message according to cabal
			if (next_msg_time) next_msg_time += timeToSend(pmsg->size());
			else next_msg_time = net->net_time + timeToSend(pmsg->size());
			DBG(5, "%s Packet sent urgently, next one in %ld\n", str().c_str(), next_msg_time-net->net_time);
			// if necessary, put in queue as "sent once"
			if(flags & UNetAckReq) {
				pmsg->snt_timestamp=net->net_time;
				pmsg->timestamp+=msg_timeout;
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
			serverMsg.ps=pmsg->sn;
			serverMsg.pfr=pmsg->frn;
		}
	}
	
	// FIXME cancel the wait in the main thread
}

/** process a recieved msg: put it in the rcvq, assemble fragments, create akcs */
void tNetSession::processIncomingMsg(void * buf,size_t size) {
	DBG(5,"Message of %li bytes\n",size);
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	//stamp
	activity_stamp = net->net_time;
	
	// when authenticated == 2, we don't expect an encoded packet, but sometimes, we get one. Since alcUruValidatePacket will try to
	// validate the packet both with and without passwd if possible, we tell it to use the passwd whenever we have one - as a result,
	// even if the client for some reason decides to encode a packet while authenticated == 2, we don't care
	int ret=alcUruValidatePacket(static_cast<uint8_t*>(buf),size,&validation,authenticated==1 || authenticated==2,passwd.c_str());
	
	if(ret!=0 && (ret!=1 || net->flags & UNET_ECRC)) {
		if(ret==1) {
			net->err->log("ERR: %s Failed validating a message in validation level %d", str().c_str(), validation);
			if (authenticated==1 || authenticated==2)
				net->err->print(" (authed)!\n");
			else
				net->err->print(" (not authed)!\n");
		}
		else net->err->log("ERR: %s Non-Uru protocol packet recieved!\n", str().c_str());
		return;
	}
	
	#ifdef ENABLE_MSGDEBUG
	net->log->log("<RCV> RAW Packet follows: \n");
	net->log->dumpbuf(buf,size);
	net->log->nl();
	#endif
	
	tSBuf mbuf(buf,size);
	
	tUnetUruMsg *msg = new tUnetUruMsg;
	
	try {
		mbuf.get(*msg);
		#ifdef ENABLE_MSGDEBUG
		net->log->log("<RCV> ");
		msg->dumpheader(net->log);
		net->log->nl();
		#endif
		msg->htmlDumpHeader(net->ack,0,ip,port);
	} catch(txUnexpectedData &t) {
		net->err->log("%s Unexpected Data %s\nBacktrace:%s\n",str().c_str(),t.what(),t.backtrace());
		delete msg;
		throw txProtocolError(_WHERE("Cannot parse a message"));
	}
	
	//Protocol Upgrade
	if(msg->tf & UNetExt) {
		cflags |= UNetUpgraded;
	}

	if(msg->tf & UNetNegotiation) {
		tmNetClientComm comm(this);
		msg->data.rewind();
		msg->data.get(comm);
		net->log->log("<RCV> [%d] %s",msg->sn,comm.str().c_str());
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
				if (msg->sn != 1 || msg->ps != 0) {
					net->err->log("%s ERR: Got a nego with a sn of %d (expected 1) and a previous ack of %d (expected 0)\n", str().c_str(), msg->sn, msg->ps);
					clientMsg.ps = msg->ps; // the nego marks the beginning of a new connection, so accept everything from here on
					clientMsg.pfr = msg->pfr;
				}
				if(nego_stamp.seconds==0) nego_stamp.setToNow();
				negotiate();
			}
			cabal=0; // re-determine cabal with the new bandwidth
		}
		
		// calculate connection bandwidth
		if (!isConnected()) {
			// if we know the downstream of the peer, set avg cabal using what is smaller: our upstream or the peers downstream
			//  (this is the last part of the negotiationg process)
			
			// save our upstream in cabal (in bytes per second)
			if((ntohl(ip) & 0xFFFFFF00) == 0x7F000000) { //lo
				cabal=100 * 1000 * 1000/8; //100Mbps
			} else if((ip & net->lan_mask) == net->lan_addr) { //LAN
				cabal=net->lan_up / 8;
			} else { //WAN
				cabal=net->nat_up / 8;
			}
			// determine connection limits
			comm.bandwidth /= 8; // we want bytes per second
			maxBandwidth = std::max(cabal, comm.bandwidth);
			minBandwidth = std::min(cabal, comm.bandwidth);
			cabal=minBandwidth;
			DBG(5, "%s Initial cabal is %i\n",str().c_str(),cabal);
			negotiating=false;
		}
	} else if (!isConnected()) { // we did not yet negotiate
		if(!negotiating) { // and we are not in the process of doing it - so start that process
			net->log->log("%s WARN: Obviously a new connection was started with something different than a nego\n", str().c_str());
			nego_stamp.setToNow();
			negotiate();
			clientMsg.ps = msg->ps; // this message is the beginning of a new connection, so accept everything from here on
			clientMsg.pfr = msg->pfr;
		}
	}
	else
		assert(maxBandwidth && minBandwidth); // we are connected, so these *must* be set
	
	// fix the problem that happens every 15-30 days of server uptime - but only if sndq is empty, or we will loose packets
	if (!anythingToSend() && (serverMsg.sn>=8378608 || msg->sn>=8378608)) { // 8378608 = 2^23 - 10000
		net->log->log("%s WARN: Congratulations! You have reached the maxium allowed sequence number, don't worry, this is not an error\n", str().c_str());
		net->log->flush();
		resetMsgCounters();
		nego_stamp.setToNow();
		renego_stamp.seconds=0;
		negotiate();
	}

	if (msg->tf & UNetAckReply) {
		// Process ack replies, no matter whether they are out of roder or not
		if (isConnected()) {
			// don't parse acks before we are connected (if we do, we might get SN confusion if connections don't start with a Nego)
			ackCheck(*msg);
			if(authenticated==2) authenticated=1;
		}
	}
	else {
		// Negotiation (processed above, but we have to do the ack checking) and normal packets

		//check uplicates
		ret = compareMsgNumbers(msg->ps, msg->pfr, clientMsg.ps, clientMsg.pfr);
		if (rejectMessages) ret = 1; // we curently don't accept messages - please come back later
		/* ret values:
		  <0: This is an old packet, ack, but don't accept
		   0: This is what we need
		  >0: This is a future packet (1: Drop it, 2: Cache it) */
		if (ret < 0) {
			net->log->log("WARN: Dropping re-sent old packet %d.%d (last ack: %d.%d, expected: %d.%d)\n", msg->sn, msg->frn, msg->ps, msg->pfr, clientMsg.ps, clientMsg.pfr);
		}
		else if (ret > 0) {
			if (!(msg->tf & UNetNegotiation) && !rejectMessages && rcvq->len() < net->receiveAhead)
				ret = 2; // preserve for future use - but don't do this for negos (we can not get here for ack replies)
			else
				net->log->log("WARN: Dropped packet I can not yet parse: %d.%d (last ack: %d.%d, expected: %d.%d)\n", msg->sn, msg->frn, msg->ps, msg->pfr, clientMsg.ps, clientMsg.pfr);
		}

		// if the packet requires it and the above check told us that'd be ok, send an ack
		if (ret != 1 && (msg->tf & UNetAckReq)) {
			//ack reply
			createAckReply(*msg);
		}
		
		// We may be able to cache it
		if (ret == 2) {
			queueReceivedMessage(msg);
			msg = NULL; // don't delete
		}
		// if it is ok to parse the packet, do that
		else if (ret == 0) {
			//flood control
			if((msg->tf & UNetAckReq) && (msg->frn==0) && (net->flags & UNET_FLOODCTR)) {
				if(net->passedTimeSince(flood_last_check) > net->flood_check_interval) {
					flood_last_check=net->net_time;
					flood_npkts=0;
				} else {
					flood_npkts++;
					if(flood_npkts>net->max_flood_pkts) {
						// send UNET_FLOOD event
						tNetEvent * evt=new tNetEvent(getIte(),UNET_FLOOD);
						net->addEvent(evt);
					}
				}
			}
			// end flood control
			
			// We can use it!
			acceptMessage(msg);
			msg = NULL; // acceptMessage will delete it

			// We might be able to accept cached messages now
			checkQueuedMessages();
		}
	}
	delete msg;
}

/** add this to the received message and put fragments together - this deletes the passed message! */
void tNetSession::acceptMessage(tUnetUruMsg *t)
{
	DBG(5, "Accepting %d.%d\n", t->sn, t->frn);
	if (t->tf & UNetAckReq) {
		clientMsg.ps = t->sn;
		clientMsg.pfr = t->frn;
	}
	if (t->tf & UNetNegotiation) {
		delete t;
		return; // These are already processed
	}

	size_t frg_size = maxPacketSz - t->hSize();
	if (!rcv) { // this is a brand new message
		rcv=new tUnetMsg();
		rcv->sn=t->sn;
	}
	else {
		if (rcv->sn != t->sn) throw txProtocolError(_WHERE("I am assembling message nr. %d and received nr. %d?!?", rcv->sn, t->sn));
	}
	if (t->frn != rcv->fr_count) throw txProtocolError(_WHERE("Expected fragment %d, got %d\n", rcv->fr_count, t->frn));

	// Got the next one!
	rcv->data.put(t->data);

	if(rcv->fr_count==t->frt) {
		size_t size = t->frt * frg_size + t->data.size();
		if (rcv->data.size() != size)
			throw txProtocolError(_WHERE("Expected a size of %d, got %d\n", size, rcv->data.size()));
		// We are done!
		rcv->data.rewind();
		rcv->cmd=alcFixUUNetMsgCommand(rcv->data.get16(), this);
		rcv->data.rewind();

		tNetEvent *evt=new tNetEvent(getIte(), UNET_MSGRCV, rcv);
		net->addEvent(evt);
		rcv = NULL;
	}
	else {
		rcv->fr_count++;
	}
	delete t;
}

/** Saves a received, not-yet accepted message for future use */
void tNetSession::queueReceivedMessage(tUnetUruMsg *msg)
{
	net->log->log("Queuing %d.%d for future use (can not yet accept it - last ack: %d.%d, expected: %d.%d)\n", msg->sn, msg->frn, msg->ps, msg->pfr, clientMsg.ps, clientMsg.pfr);
	tUnetUruMsg *cur;
	rcvq->rewind();
	// the queue is sorted ascending
	while ((cur = rcvq->getNext())) {
		int ret = compareMsgNumbers(msg->sn, msg->frn, cur->sn, cur->frn);
		if (ret < 0) { // we have to put it after the current one!
			rcvq->insertBefore(msg);
			return;
		}
		else if (ret == 0) { // the message is already in the queue
			delete msg;
			return;
		}
	}
	rcvq->add(msg); // insert it at the end
}

/** Checks if messages saved by queueReceivedMessage can now be used */
void tNetSession::checkQueuedMessages(void)
{
	rcvq->rewind();
	tUnetUruMsg *cur = rcvq->getNext();
	while (cur) {
		int ret = compareMsgNumbers(cur->ps, cur->pfr, clientMsg.ps, clientMsg.pfr);
		if (ret < 0) rcvq->deleteCurrent(); // old packet
		else if (ret == 0) acceptMessage(rcvq->unstackCurrent());
		else return; // These can still not be accepted
		cur = rcvq->getCurrent();
	 }
}

/** creates an ack in the ackq */
void tNetSession::createAckReply(tUnetUruMsg &msg) {
	tUnetAck * ack,* cack;
	uint32_t A,B;
	
	#ifdef ENABLE_ACKDEBUG
	net->log->log("stacking ack %i,%i %i,%i\n",msg.sn,msg.frn,msg.ps,msg.pfr);
	#endif
	
	A=msg.csn;
	B=msg.cps;
	
	ack=new tUnetAck();
	ack->A=msg.csn;
	ack->B=msg.cps;
	
	//we must delay either none or all messages, otherwise the rtt will vary too much
	tNetTimeDiff ackWaitTime=timeToSend((msg.frt-msg.frn+1) * maxPacketSz); // this is how long transmitting the whole packet will approximately take
	if(ackWaitTime > msg_timeout/4) ackWaitTime=msg_timeout/4; // don't use the rtt as basis, it is 0 at the beginning, resulting in a much too quick first answer, a much too low rtt on the other side and thus the packets being re-sent too early
	ack->timestamp=net->net_time + ackWaitTime;
	// the net timer will be updated when the ackq is checked (which is done since processMsg will call doWork after calling createAckReply)
	DBG(5, "ack tts: %i, %i, %i\n",msg.frt,ackWaitTime,cabal);
	
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
	delete ack;
	
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
tNetTimeDiff tNetSession::ackUpdate() {
	const size_t maxacks=30;
	tNetTimeDiff timeout = -1; // -1 is biggest possible value
	if (ackq->len() == 0) return timeout;
	
	ackq->rewind();
	tUnetAck *ack = ackq->getNext();
	while(ack) {
		tmNetAck ackMsg(this);
		while (ack) {
			if (!net->timeOverdue(ack->timestamp)) {
				timeout = std::min(timeout, net->remainingTimeTill(ack->timestamp)); // come back when we want to process this ack
				ack = ackq->getNext();
			}
			else {
				ackMsg.ackq.push_back(ackq->unstackCurrent()); // will switch to the next one
				ack = ackq->getCurrent();
				if (ackMsg.ackq.size() >= maxacks) break;
			}
		}
		if (ackMsg.ackq.size() > 0)
			send(ackMsg);
	}
	return timeout;
}

/** parse the ack and remove the messages it acks from the sndq */
void tNetSession::ackCheck(tUnetUruMsg &t) {

	uint32_t A1,A2,A3;
	tmNetAck ackMsg(this);
	if (t.tf & UNetExt) ackMsg.bhflags |= UNetExt; // tmNetAck has to know that it reads an extended packet
	t.data.rewind();
	t.data.get(ackMsg);
#ifdef ENABLE_ACKLOG
	net->log->log("<RCV> [%d] %s\n", t.sn, ackMsg.str().c_str());
#endif
	tUnetAck *ack;
	for (tmNetAck::tAckList::iterator it = ackMsg.ackq.begin(); it != ackMsg.ackq.end(); ++it) {
		ack = *it;
		A1 = ack->A;
		A3 = ack->B;
		//well, do it
		tUnetUruMsg * msg=NULL;
		sndq->rewind();
		msg=sndq->getNext();
		while((msg!=NULL)) {
			A2=msg->csn;
			
			if(A1>=A2 && A2>A3) {
				//then delete
				#ifdef ENABLE_ACKDEBUG
				net->log->log("Deleting packet %i,%i\n",msg->sn,msg->frn);
				#endif
				if(msg->tryes<=1 && A1==A2) {
					/* possible problem: since this is the last packet which was acked with this ack message, it could be combined
					   with other packets and the ack could be sent almost immediately after the packet went in, without the
					   usual delay so the rtt is much smaller than the average. But I don't expect this to happen often, so I don't
					   consider that a real problem. */
					tNetTimeDiff crtt=net->passedTimeSince(msg->snt_timestamp);
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
				msg=sndq->getNext();
			}
		}
	}

}

/** Send, and re-send messages, update idle state and set netcore timeout (the last is NECESSARY - otherwise, the idle timer will be used) */
tNetTimeDiff tNetSession::processSendQueues()
{
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	// when we are talking to a non-terminated server, send alive messages
	if (!client && !terminated && (net->passedTimeSince(activity_stamp) > (conn_timeout/2))) {
		tmAlive alive(this);
		send(alive);
	}
	
	
	tNetTimeDiff timeout = ackUpdate(); //generate ack messages (i.e. put them from the ackq to the sndq)

	if (!anythingToSend() && terminated) conn_timeout /= 5; // we are done, completely done - but wait some little more time for possible re-sends
	
	// for terminating session, make sure they are cleaned up on time
	if (terminated) {
	   timeout = std::min(timeout, conn_timeout);
	   DBG(3, "Terminated session %s setting timeout %d\n", str().c_str(), timeout);
	}
	
	if(sndq->isEmpty()) {
		next_msg_time=0;
		return timeout;
	}
	
	if(net->timeOverdue(next_msg_time)) {
		sndq->rewind();
		tUnetUruMsg * curmsg = sndq->getNext();
		
		unsigned int quota_max=maxPacketSz;
		unsigned int cur_quota=0;
		
		const unsigned int minTH=15;
		const unsigned int maxTH=100;

		while(curmsg!=NULL && (cur_quota<quota_max)) {
			if(net->timeOverdue(curmsg->timestamp)) {
				DBG(8, "%s ok to send a message\n",str().c_str());
				//we can send the message
				if(curmsg->tf & UNetAckReq) {
					//send packet
					
					// check if we need to resend
					if(curmsg->tryes!=0) {
						DBG(3, "%s Re-sending a message\n", str().c_str());
						if(curmsg->tryes==1) {
							decreaseCabal(true); // true = small
						} else {
							decreaseCabal(false); // false = big
						}
						// The server used to duplicate the timeout here - but since the timeout will be overwritten next time updateRTT
						//  is called, that's of no use. So better make the RTT bigger - it is obviously at least the timeout
						// This will result in a more long-term increase of the timeout
						updateRTT(3*msg_timeout); // this is necessary because the vault server blocks the netcore when doing SQL - reduce this when multi-threading is implemented
					}
					
					if(curmsg->tryes>=10 || (curmsg->tryes>=2 && terminated)) { // max. 2 sends on terminated connections, max. 10 for the rest
						sndq->deleteCurrent();
						curmsg = sndq->getCurrent(); // this is the next one
						//timeout event
						net->sec->log("%s Timeout (didn't ack a packet)\n", str().c_str());
						tNetEvent *evt=new tNetEvent(getIte(),UNET_TIMEOUT);
						net->addEvent(evt);
					} else {
						cur_quota+=curmsg->size();
						curmsg->snt_timestamp=net->net_time;
						net->rawsend(this,curmsg);
						curmsg->timestamp=net->net_time+msg_timeout;
						curmsg->tryes++;
						curmsg = sndq->getNext(); // go on
					}
				} else {
					//probabilistic drop (of voice, and other non-ack packets) - make sure we don't drop ack replies though!
					if(curmsg->tf == 0x00 && net->passedTimeSince(curmsg->timestamp) > 4*msg_timeout) {
						//Unacceptable - drop it
						net->err->log("%s Dropped a 0x00 packet due to unaceptable msg time %i,%i,%i\n",str().c_str(),msg_timeout,net->passedTimeSince(curmsg->timestamp),rtt);
					} else if(curmsg->tf == 0x00 && (sndq->len() > static_cast<size_t>((minTH + random()%(maxTH-minTH)))) ) {
						net->err->log("%s Dropped a 0x00 packet due to a big queue (%d messages)\n", str().c_str(), sndq->len());
					}
					//end prob drop
					else {
						cur_quota+=curmsg->size();
						net->rawsend(this,curmsg);
					}
					sndq->deleteCurrent();
					curmsg = sndq->getCurrent(); // this is the next one
				}
			} //end time check
			else {
				timeout = std::min(timeout, net->remainingTimeTill(curmsg->timestamp)); // come back when we want to send this message
				DBG(8,"%s Too soon (%d) to send a message\n",str().c_str(),net->remainingTimeTill(curmsg->timestamp));
				curmsg = sndq->getNext(); // go on
			}
		} //end while
		// calculate how long it will take us to send what we just sent
		tNetTimeDiff tts=timeToSend(cur_quota);
		DBG(8,"%s %ld tts is now:%i quota:%i,cabal:%i\n",str().c_str(),net->net_time,tts,cur_quota,cabal);
		next_msg_time=net->net_time + tts;
		// if there is still something to send, but the quota does not let us, do that ASAP
		if (curmsg && tts < timeout) timeout = tts;
	} else {
		// Still wait before sending a message
		DBG(8,"%s Too soon (%d) to check sndq\n",str().c_str(),net->remainingTimeTill(next_msg_time));
		timeout = std::min(timeout, net->remainingTimeTill(next_msg_time)); // come back when we want to send the next message
	}
	return timeout;
}

/** send a negotiation to the peer */
void tNetSession::negotiate() {
	unsigned int sbw;
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

void tNetSession::terminate(int tout)
{
	conn_timeout = tout*1000*1000;
	// FIXME somehow make sure we do not sleep too long?
	terminated = true;
	whoami = 0; // it's terminated, so it's no one special anymore
	activity_stamp = net->net_time; // FIXME called in worker thread
}

void tNetSession::setAuthData(uint8_t accessLevel, const tString &passwd)
{
	this->client = true; // no matter how this connection was established, the peer definitely acts like a client
	this->whoami = KClient; // it's a real client now
	this->authenticated = 2; // the player is authenticated!
	this->accessLevel = accessLevel;
	this->passwd = passwd; // passwd is needed for validating packets
}

/* End session */

}
