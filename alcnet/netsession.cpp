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

/**
	URUNET 3
*/

/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSION_ID "$Id$"
//#define _DBG_LEVEL_ 5
#include <alcdefs.h>
#include "netsession.h"

#include "netmsgq.h"
#include "unet.h"
#include "netexception.h"
#include "protocol/umsgbasic.h"
#include <alcutil/alclog.h>
#include <alcmain.h>

#include <cassert>
#include <unistd.h>
#include <math.h>

namespace alc {

/* Session */
tNetSession::tNetSession(alc::tUnet* net, uint32_t ip, uint16_t port, uint32_t sid, bool client, uint8_t validation)
: net(net), ip(ip), port(port), sid(sid), validation(validation), maxPacketSz(1024), rcv(NULL), client(client), refs(1) {
	DBG(5,"tNetSession()\n");
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	// validation and upgraded protocol hack
	if (validation >= 3) {
		this->validation = 0;
		upgradedProtocol = true;
	}
	else
		upgradedProtocol = false;
	// fill in default values
	authenticated=false;
	accessLevel=0;
	cabal=0;
	consecutiveCabalIncreases = 0;
	flood_last_check=net->net_time;
	flood_npkts=0;
	receive_stamp = send_stamp = net->net_time;
	next_msg_time=net->net_time;
	deviation=0;
	msg_timeout=net->msg_timeout;
	rtt=0;
	conn_timeout=net->conn_timeout*1000*1000;
	negotiating=false;
	resetMsgCounters();
	assert(serverMsg.pn==0);
	rejectMessages=false;
	terminated = false;
	
	// socket data
	sockaddr.sin_family=AF_INET;
	sockaddr.sin_addr.s_addr=ip;
	sockaddr.sin_port=port;
	
	// public data
	max_version=client ? 0 : net->max_version;
	min_version=client ? 0 : net->min_version;
	gameType=UnknownGame;
	ki=0;
	data = NULL;
	joined = false;
	
	DBG(5, "%s Initial msg_timeout: %d\n", str().c_str(), msg_timeout);
	// session created
	created_stamp.setToNow();
	net->sec->log("%s New Connection\n",str().c_str());
	net->addEvent(new tNetEvent(this,UNET_NEWCONN));
	
	// start negotating of we are a talking to a server
	if (!client) negotiate();
}
tNetSession::~tNetSession() {
	DBG(5,"~tNetSession() (sndq: %Zd messages left)\n", sndq.size());
	delete data;
	delete rcv;
}
void tNetSession::resetMsgCounters(void) {
	tMutexLock lock(sendMutex);
	DBG(3, "tNetSession::resetMsgCounters\n");
	clientMsg.pfr=0;
	clientMsg.ps=0;
	serverMsg.pn=0;
	serverMsg.sn=0;
	serverMsg.pfr=0;
	serverMsg.ps=0;
	// empty queues
	sndq.clear();
	ackq.clear();
	rcvq.clear();
	delete rcv;
	rcv = NULL;
}
int8_t tNetSession::compareMsgNumbers(uint32_t sn1, uint8_t fr1, uint32_t sn2, uint8_t fr2)
{
	if (sn1 < sn2 || (sn1 == sn2 && fr1 < fr2)) return -1;
	else if (sn1 == sn2 && fr1 == fr2) return 0;
	else return 1;
}
tString tNetSession::str() {
	tString dbg;
	dbg.printf("[%i][%s:%i]",sid,alcGetStrIp(ip).c_str(),ntohs(port));
	tReadLock lockPub(pubDataMutex);
	tReadLock lockPrv(prvDataMutex);
	if (!name.isEmpty() && authenticated) {
		if (ki != 0) dbg.printf("[%s:%s,%d]", name.c_str(), avatar.c_str(), ki);
		else dbg.printf("[%s]", name.c_str());
	}
	else if (!name.isEmpty() && net->whoami == KTracking) { // we are tracking and this is a game server
		dbg.printf("[%s:%s]", name.c_str(), alcGetStrGuid(serverGuid).c_str());
	}
	return dbg;
}

size_t tNetSession::getHeaderSize() {
	size_t my=28;
	if(validation>0) my+=4;
	if(upgradedProtocol) my-=8;
	return my;
}

// functions to calculate cabal and rtt
void tNetSession::updateRTT(tNetTimeDiff newread) {
	if(rtt==0) rtt=2*newread; // start with a large deviation, in case this was like an "early shot"
	//Jacobson/Karels
	const int alpha=125; // this is effectively 0.125
	const int delta=4;
	const int diff=newread - rtt;
	rtt       += (alpha*diff)/1000;
	deviation += (alpha*(abs(diff)-deviation))/1000;
	msg_timeout= rtt + delta*deviation;
	if (msg_timeout > 5000000) msg_timeout = 5000000; // max. timeout: 5secs
	DBG(3,"%s RTT update (sample rtt: %i) new rtt:%i, msg_timeout:%i, deviation:%i\n", str().c_str(),newread,rtt,msg_timeout,deviation);
}
void tNetSession::increaseCabal() {
	if(!cabal) return;
	++consecutiveCabalIncreases;
	cabal += maxPacketSz/7.5*log(consecutiveCabalIncreases);
	DBG(3,"%s +Cabal is now %i\n",str().c_str(),cabal);
}
void tNetSession::decreaseCabal(bool emergency) {
	consecutiveCabalIncreases = 0;
	if(!cabal) return;
	const unsigned int delta = emergency ? 25 : 2;
	cabal -= (delta*cabal)/100;
	if(cabal < 5u*maxPacketSz) cabal = 5u*maxPacketSz; // don't drop below 5kByte/s
	DBG(3,"%s %sCabal is now %i\n",str().c_str(),emergency ? "--" : "-",cabal);
}

/** computes the time we have to wait after sending the given amount of bytes */
tNetTimeDiff tNetSession::timeToSend(size_t psize) {
	if(psize<4000) {
		if (cabal) return((psize*1000000)/cabal);
		return((psize*1000000)/maxPacketSz);
	} else {
		if (cabal) return(((psize*1000)/cabal)*1000);
		return(((psize*1000)/maxPacketSz)*1000);
	}
}

/**
	puts the message in the session's send queue
*/
void tNetSession::send(const tmBase &msg, tNetTimeDiff delay) {
	if (msg.getSession() != this)
		throw txUnexpectedData(_WHERE("Message sent for wrong session!"));
	uint8_t flags = msg.bhflags(), val = validation;
#if _DBG_LEVEL_ < 1
	if (!(flags & UNetAckReply))
#endif
		net->log->log("<SND> %s\n",msg.str().c_str());
	tMBuf buf;
	size_t csize,psize,hsize,pkt_sz;
	unsigned int n_pkts;
	
	buf.put(msg);
	psize=buf.size();
	buf.rewind();
	DBG(7,"Ok, I'm going to send a packet of %Zi bytes, for peer %i, with flags %02X\n",psize,sid,flags);


	if((flags & UNetNegotiation) && (flags & UNetAckReply))
		throw txUnexpectedData(_WHERE("Flags UNetAckReply and UNetNegotiation cannot be set at the same time"));
	if((flags & UNetAckReq) && (flags & UNetAckReply))
		throw txUnexpectedData(_WHERE("Flags UNetAckReply and UNetAckReq cannot be set at the same time"));

	//On validation level 1 - ack and negotiations don't have checksum verification
	/* I still don't understand wtf was thinking the network dessigner with doing a MD5 of each
		packet?
	*/
	if((flags & (UNetNegotiation | UNetAckReply)) && (val==0x01)) { val=0x00; }
	DBG(6,"Sending a packet of validation level %i\n",val);

	//fragment the messages and put them in to the send qeue
	
	if(val==0x00) { hsize=28; } else { hsize=32; }
	if(upgradedProtocol) { hsize-=8; flags |= UNetExt; }

	pkt_sz=maxPacketSz - hsize; //get maxium message size
	n_pkts=(psize-1)/pkt_sz; //get number of fragments (0 means everything is sent in one packet, which must be the case if psize == pkt_sz)
	if(n_pkts>=256)
		throw txTooBig(_WHERE("%s packet of %i bytes doesn't fit inside an Uru message\n",str().c_str(),psize));
	if (n_pkts > 0 && ((flags & UNetNegotiation) || (flags & UNetAckReply)))
		throw txProtocolError(_WHERE("Nego and ack packets must not be fragmented!"));
	
	net->updateNetTime(); // make sure we got correct time reference
	delay *= 1000; // make it usecs
	{
		tMutexLock lock(sendMutex);
		serverMsg.sn++;
		for(unsigned int i=0; i<=n_pkts; i++) {
			//get current paquet size
			if(i==n_pkts) csize=psize - (i*pkt_sz);
			else csize=pkt_sz;
			
			tUnetUruMsg * pmsg=new tUnetUruMsg(msg.urgent);
			pmsg->val=val;
			//pmsg.pn NOT in this layer (done in the msg sender, tUnet::rawsend)
			//since urgent messages are put at the top of the sndq, the pn would be wrong if we did it differently
			pmsg->bhflags=flags;
			pmsg->frn=i;
			pmsg->sn=serverMsg.sn;
			pmsg->frt=n_pkts;
			pmsg->pfr=serverMsg.pfr;
			pmsg->ps=serverMsg.ps;
			
			pmsg->data.write(buf.read(csize),csize);
			pmsg->_update();
			
			pmsg->timestamp=net->net_time+delay; // no need to take tts into account now, the send queue worker does that (and our tts estimate might change till this packet is actually sent)
			
			#ifdef ENABLE_NETDEBUG
			pmsg->timestamp+=net->latency;
			#endif
			
			if(flags & UNetAckReq) { // if this packet has the ack flag on, save it's number
				serverMsg.ps=pmsg->sn;
				serverMsg.pfr=pmsg->frn;
			}
			
			// Urgent messages are added at the top of the send queue but BEHIND other urgent packets, the rest at the back
			// this is important, or the first ack might be sent before the first nego!
			if(pmsg->urgent) {
				// search for the first non-urgent message
				for (tPointerList<tUnetUruMsg>::iterator it = sndq.begin(); it != sndq.end(); ++it) {
					if (!(*it)->urgent) {
						sndq.insert(it, pmsg);
						pmsg = NULL;
						break;
					}
				}
			}
			if (pmsg) { // non-urgent packets or a totally urgent sndq
				sndq.push_back(pmsg);
			}
		}
	}
	
	if (alcGetSelfThreadId() != alcGetMain()->threadId()) net->wakeUpMainThread();
}

/** sends the message (internal use only)
An uru message can only be 253952 bytes in V0x01 & V0x02 and 254976 in V0x00
Call only with the sendMutex locked! */
void tNetSession::rawsend(tUnetUruMsg *msg)
{
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());

	DBG(9,"Server pn is %08X\n",serverMsg.pn);
	DBG(9,"Server sn is %08X,%08X\n",serverMsg.sn,msg->sn);
	serverMsg.pn++;
	msg->pn=serverMsg.pn;
	
	#ifdef ENABLE_MSGDEBUG
	net->log->log("<SND> ");
	msg->dumpheader(net->log);
	net->log->nl();
	#endif
	msg->htmlDumpHeader(net->ack,1,ip,port);

	//store message into buffer
	tMBuf * mbuf;
	mbuf = new tMBuf();
	mbuf->put(*msg);

	#ifdef ENABLE_MSGDEBUG
	net->log->log("<SND> RAW Packet follows: \n");
	net->log->dumpbuf(*mbuf);
	net->log->nl();
	#endif
	
	DBG(9,"validation level is %i,%i\n",validation,msg->val);
	
	size_t msize=mbuf->size();
	uint8_t * buf, * buf2=NULL;
	buf=const_cast<uint8_t *>(mbuf->data()); // yes, we are writing directly into the tMBuf buffer... this saves us from copying everything

	if(msg->val==2) {
		DBG(8,"Encoding validation 2 packet of %Zi bytes...\n",msize);
		buf2=static_cast<uint8_t *>(malloc(msize));
		if(buf2==NULL) { throw txNoMem(_WHERE("")); }
		alcEncodePacket(buf2,buf,msize);
		buf=buf2; //don't need to decode again
		if(authenticated) {
			DBG(8,"Client is authenticated, doing checksum...\n");
			uint32_t val=alcUruChecksum(buf,msize,2,passwd.c_str());
			memcpy(buf+2,&val,4);
			DBG(8,"Checksum done!...\n");
		} else {
			DBG(8,"Client is not authenticated, doing checksum...\n");
			uint32_t val=alcUruChecksum(buf,msize,1,NULL);
			memcpy(buf+2,&val,4);
			DBG(8,"Checksum done!...\n");
		}
		buf[1]=0x02;
	} else if(msg->val==1) {
		uint32_t val=alcUruChecksum(buf,msize,0,NULL);
		memcpy(buf+2,&val,4);
		buf[1]=0x01;
	} else if(msg->val==0) {
		buf[1]=0x00;
	}
	else
		throw txUnexpectedData(_WHERE("Invalid validation level %d", msg->val));
	buf[0]=0x03; //magic number
	DBG(9,"Before the Sendto call...\n");
	//
#ifdef ENABLE_NETDEBUG
	if(!net->out_noise || (random() % 100) >= net->out_noise) {
		DBG(8,"Outcomming Packet accepted\n");
	} else {
		DBG(5,"Outcomming Packet dropped\n");
		msize=0;
	}
	//check quotas
	if(net->passedTimeSince(net->last_quota_check) > net->quota_check_interval) {
		net->cur_up_quota=0;
		net->cur_down_quota=0;
		net->last_quota_check = net->net_time;
	}
	if(msize>0 && net->lim_up_cap) {
		if((net->cur_up_quota+msize+net->ip_overhead)>net->lim_up_cap) {
			DBG(5,"Paquet dropped by quotas, in use:%i,req:%li, max:%i\n",net->cur_up_quota,msize+net->ip_overhead,net->lim_up_cap);
			net->log->log("Paquet dropped by quotas, in use:%i,req:%i, max:%i\n",net->cur_up_quota,msize+net->ip_overhead,net->lim_up_cap);
			msize=0;
		} else {
			net->cur_up_quota+=msize+net->ip_overhead;
		}
	}
#endif

	if(msize>0) {
		msize = sendto(net->sock,reinterpret_cast<char *>(buf),msize,0,reinterpret_cast<struct sockaddr *>(&sockaddr),sizeof(struct sockaddr));
	}

	DBG(9,"After the Sendto call...\n");
	free(buf2);
	delete mbuf;
	DBG(8,"returning from uru_net_send RET:%Zi\n",msize);
}

/** send a negotiation to the peer */
void tNetSession::negotiate() {
	unsigned int sbw;
	// send server downstream (for the peer to know how fast it can send packets)
	DBG(9,"%08X %08X %08X\n",ip,net->lan_mask,net->lan_addr);
	if((ntohl(ip) & 0xFFFFFF00) == 0x7F000000 || (ip & net->lan_mask) == net->lan_addr) { //lo or LAN
		sbw=net->lan_down;
	} else {
		sbw=net->nat_down; // WAN
	}

	tmNetClientComm comm(tTime::now(),sbw,this);
	send(comm);
	negotiating = true;
}

/** process a recieved msg: put it in the rcvq, assemble fragments, create akcs */
void tNetSession::processIncomingMsg(void * buf,size_t size) {
	DBG(7,"Message of %Zi bytes\n",size);
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	// check max packet size
	if (size > maxPacketSz) { // catch impossible big messages
		throw txProtocolError(_WHERE("[%s] Recieved a too big message of %i bytes\n",str().c_str(),size));
	}
	//stamp
	receive_stamp = net->net_time;
	
	/* when authenticated is false, but passwd already set, we don't expect an encoded packet, but sometimes, we get one. Since
	 * alcUruValidatePacket will try to validate the packet both with and without passwd if possible, we tell it to use the passwd
	 * whenever we have one - as a result, even if the client for some reason decides to encode a packet before we actually know
	 * the passwd, that's fine. */
	int ret;
	{
		tReadLock lock(prvDataMutex);
		ret=alcUruValidatePacket(static_cast<uint8_t*>(buf),size,&validation,!passwd.isEmpty(),passwd.c_str());
	}
	
	if(ret!=0 && (ret!=1 || net->flags & UNET_ECRC)) {
		if(ret==1) {
			net->err->log("ERR: %s Failed validating a message in validation level %d", str().c_str(), validation);
			if (!passwd.isEmpty())
				net->err->print(" (authed)!\n");
			else
				net->err->print(" (not authed)!\n");
		}
		else net->err->log("ERR: %s Non-Uru protocol packet recieved!\n", str().c_str());
		return;
	}
	
	#ifdef ENABLE_MSGDEBUG
	net->log->log("<RCV> %s RAW Packet follows: \n", str().c_str());
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
	if(msg->bhflags & UNetExt) {
		tWriteLock lock(prvDataMutex);
		upgradedProtocol = true;
	}

	// process negotation
	if(msg->bhflags & UNetNegotiation) {
		tmNetClientComm comm(this, msg);
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
				negotiate();
			}
			cabal=0; // re-determine cabal with the new bandwidth
		}
		
		// calculate connection bandwidth
		if (!isConnected()) {
			// if we know the downstream of the peer, set avg cabal using what is smaller: our upstream or the peers downstream
			//  (this is the last part of the negotiationg process)
			
			// save our upstream in cabal (in bytes per second)
			if((ntohl(ip) & 0xFFFFFF00) == 0x7F000000 || (ip & net->lan_mask) == net->lan_addr) { //lo/LAN
				cabal=net->lan_up / 8;
			} else { //WAN
				cabal=net->nat_up / 8;
			}
			// determine connection limits
			comm.bandwidth /= 8; // we want bytes per second
			unsigned int maxBandwidth = std::max(cabal, comm.bandwidth);
			unsigned int minBandwidth = std::min(cabal, comm.bandwidth);
			cabal=std::min(minBandwidth, maxBandwidth/4); // don't start too fast, the nego just gives us an estimate!
			if (cabal < maxPacketSz) cabal = maxPacketSz;
			DBG(5, "%s Initial cabal is %i\n",str().c_str(),cabal);
			negotiating=false;
		}
	} else if (!isConnected()) { // we did not yet negotiate
		if(!negotiating) { // and we are not in the process of doing it - so start that process
			net->log->log("%s WARN: Obviously a new connection was started with something different than a nego\n", str().c_str());
			negotiate();
			clientMsg.ps = msg->ps; // this message is the beginning of a new connection, so accept everything from here on
			clientMsg.pfr = msg->pfr;
		}
	}
	
	// fix the problem that happens every 15-30 days of server uptime - but only if sndq is empty, or we will loose packets
	{
		sendMutex.lock();
		uint32_t sn = serverMsg.sn;
		sendMutex.unlock(); // unlock here, because anythingToSend and resetMsgCounters will lock again
		if (!anythingToSend() && (sn>=8378608 || msg->sn>=8378608)) { // 8378608 = 2^23 - 10000
			net->log->log("%s WARN: Congratulations! You have reached the maxium allowed sequence number, don't worry, this is not an error\n", str().c_str());
			net->log->flush();
			resetMsgCounters();
			renego_stamp = tTime();
			negotiate();
		}
	}

	if (msg->bhflags & UNetAckReply) {
		// Process ack replies, no matter whether they are out of roder or not
		if (isConnected()) {
			// don't parse acks before we are connected (if we do, we might get SN confusion if connections don't start with a Nego)
			ackCheck(*msg);
			// authentication confirmed?
			tWriteLock lock(prvDataMutex);
			if (!authenticated && !passwd.isEmpty()) authenticated = true;
		}
	}
	else {
		tReadLock lock(prvDataMutex);
		// Negotiation (processed above, but we have to do the ack checking) and normal packets

		//check uplicates
		if (rejectMessages) ret = 1; // we curently don't accept messages - please come back later
		else ret = compareMsgNumbers(msg->ps, msg->pfr, clientMsg.ps, clientMsg.pfr);
		/* ret values:
		  <0: This is an old packet, ack, but don't accept
		   0: This is what we need
		  >0: This is a future packet (1: Drop it, 2: Cache it) */
		if (ret < 0) {
			net->log->log("%s WARN: Dropping re-sent old packet %d.%d (last ack: %d.%d, expected: %d.%d)\n", str().c_str(), msg->sn, msg->frn, msg->ps, msg->pfr, clientMsg.ps, clientMsg.pfr);
		}
		else if (ret > 0) {
			if (!(msg->bhflags & UNetNegotiation) && !rejectMessages && rcvq.size() < net->receiveAhead)
				ret = 2; // preserve for future use - but don't do this for negos (we can not get here for ack replies)
			else
				net->log->log("%s WARN: Dropped packet I can not yet parse: %d.%d (last ack: %d.%d, expected: %d.%d)\n", str().c_str(), msg->sn, msg->frn, msg->ps, msg->pfr, clientMsg.ps, clientMsg.pfr);
		}

		// if the packet requires it and the above check told us that'd be ok, send an ack
		if (ret != 1 && (msg->bhflags & UNetAckReq)) {
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
			if((msg->bhflags & UNetAckReq) && (msg->frn==0) && (net->flags & UNET_FLOODCTR)) {
				if(net->passedTimeSince(flood_last_check) > net->flood_check_interval) {
					flood_last_check=net->net_time;
					flood_npkts=0;
				} else {
					flood_npkts++;
					if(flood_npkts>net->max_flood_pkts) {
						// send UNET_FLOOD event
						net->sec->log("%s Flood Attack\n",str().c_str());
						net->addEvent(new tNetEvent(this,UNET_FLOOD));
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
	if (t->bhflags & UNetAckReq) {
		clientMsg.ps = t->sn;
		clientMsg.pfr = t->frn;
	}
	if (t->bhflags & UNetNegotiation) {
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

		tNetEvent *evt=new tNetEvent(this, UNET_MSGRCV, rcv);
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
	net->log->log("%s Queuing %d.%d for future use (can not yet accept it - last ack: %d.%d, expected: %d.%d)\n", str().c_str(), msg->sn, msg->frn, msg->ps, msg->pfr, clientMsg.ps, clientMsg.pfr);
	// the queue is sorted ascending
	for (tPointerList<tUnetUruMsg>::iterator it = rcvq.begin(); it != rcvq.end(); ++it) {
		tUnetUruMsg *cur = *it;
		int ret = compareMsgNumbers(msg->sn, msg->frn, cur->sn, cur->frn);
		if (ret < 0) { // we have to put it after the current one!
			rcvq.insert(it, msg);
			return;
		}
		else if (ret == 0) { // the message is already in the queue
			delete msg;
			return;
		}
	}
	rcvq.push_back(msg); // insert it at the end
}

/** Checks if messages saved by queueReceivedMessage can now be used */
void tNetSession::checkQueuedMessages(void)
{
	tPointerList<tUnetUruMsg>::iterator it = rcvq.begin();
	while (it != rcvq.end()) {
		tUnetUruMsg *cur = *it;
		int ret = compareMsgNumbers(cur->ps, cur->pfr, clientMsg.ps, clientMsg.pfr);
		if (ret < 0) it = rcvq.eraseAndDelete(it); // old packet
		else if (ret == 0) {
			it = rcvq.erase(it); // do not delete!
			acceptMessage(cur);
		}
		else return; // These can still not be accepted
	 }
}

/** creates an ack in the ackq */
void tNetSession::createAckReply(tUnetUruMsg &msg) {
	uint32_t A,B;
	
	// we can ack everything between B and A
	A=msg.csn;
	B=msg.cps;
	assert(A > B);
#if 0
	// send acks directly
	tmNetAck ackMsg(this);
	ackMsg.ackq.push_back(new tUnetAck(A, B)); // put it into message
	send(ackMsg);
#else
	// retard and collect acks
	// we must delay either none or all messages, otherwise the rtt will vary too much
	// do not use the rtt as basis for the delay, or the rtts of both sides will wind up endlessly
	tNetTimeDiff ackWaitTime = 2*timeToSend(maxPacketSz); // wait for some batches of packets
	if (ackWaitTime > msg_timeout/4) ackWaitTime=msg_timeout/4; // but do not wait too long
	tNetTime timestamp=net->net_time + ackWaitTime;
	// the net timer will be updated when the ackq is checked (which is done since processMsg will call doWork after calling createAckReply)
	DBG(3, "%s Enqueueing new ack, wait time: %i\n",str().c_str(),ackWaitTime);

	//Plasma like ack's (acks are retarded, and packed)
	tPointerList<tUnetAck>::iterator it = ackq.begin();
	while(it != ackq.end()) {
		tUnetAck *cack = *it;

		if(A>=cack->B && B<=cack->A) { // the two acks intersect, merge them
			// calculate new bounds and timestamp
			if (cack->A > A) A = cack->A;
			if (cack->B < B) B = cack->B;
			if (cack->timestamp < timestamp) timestamp = cack->timestamp; // use smaller timestamp
			// remove existing ack and complete merge
			ackq.eraseAndDelete(it); // remove old ack (we merged it into ours)
			it = ackq.begin(); // and restart
			DBG(9, "merged two acks, restarting ack search\n");
			continue;
		} else if(B>cack->A) { // we are completely after this ack
			++it;
			continue; // go on searching and looking
		} else if(A<cack->B) { // we are completely before this ack
			ackq.insert(it, new tUnetAck(A, B, timestamp)); // insert ourselves in the list at the right position, and be done
			return; // done!
		}
	}
	ackq.push_back(new tUnetAck(A, B, timestamp));
#endif
}

/** parse the ack and remove the messages it acks from the sndq */
void tNetSession::ackCheck(tUnetUruMsg &t) {
	uint32_t A1,A2,A3;
	tmNetAck ackMsg(this, &t);
#if _DBG_LEVEL_ >= 1
	net->log->log("<RCV> [%d] %s\n",t.sn,ackMsg.str().c_str());
#endif
	tUnetAck *ack;
	tMutexLock lock(sendMutex);
	for (tmNetAck::tAckList::iterator it = ackMsg.acks.begin(); it != ackMsg.acks.end(); ++it) {
		ack = *it;
		A1 = ack->A;
		A3 = ack->B;
		//well, do it
		tPointerList<tUnetUruMsg>::iterator jt = sndq.begin();
		while (jt != sndq.end()) {
			tUnetUruMsg *msg = *jt;
			A2=msg->csn;
			
			if(msg->bhflags & UNetAckReq && A1>=A2 && A2>A3) {
				// this packet got acked, delete it
				increaseCabal(); // a packet was properly delivered
				if(msg->tries == 1) { // only update rtt for a first-time success
					// calculate the time it took to ack this packet
					tNetTimeDiff crtt=net->passedTimeSince(msg->snt_timestamp);
					#ifdef ENABLE_NETDEBUG
					crtt+=net->latency;
					#endif
					updateRTT(crtt);
				}
				// remvoe message from queue
				jt = sndq.eraseAndDelete(jt);
			}
			else
				++jt;
		}
	}
}

/** puts acks from the ackq in the sndq */
tNetTimeDiff tNetSession::ackSend() {
	tNetTimeDiff timeout = -1; // -1 is biggest possible value
	
	tPointerList<tUnetAck>::iterator it = ackq.begin();
	while(it != ackq.end()) {
		tUnetAck *ack = *it;
		if (!net->timeOverdue(ack->timestamp)) {
			timeout = std::min(timeout, net->remainingTimeTill(ack->timestamp)); // come back when we want to process this ack
			++it;
		}
		else {
			DBG(5, "%s Sending an ack, %d after time\n", str().c_str(), net->passedTimeSince(ack->timestamp));
			/* send one message per ack: we do not want to be too pwned if that apcket got lost. And if we have "holes" in the
			 * sequence of packets (which is the only occasion in which there would be several acks in a message), the connection
			 * is already problematic. */
			tmNetAck ackMsg(this, ack);
			it = ackq.erase(it); // remove ack from queue, but do not delete it
			send(ackMsg);
		}
	}
	return timeout;
}

/** Send, and re-send messages */
tNetTimeDiff tNetSession::processSendQueues()
{
	tReadLock dataLock(prvDataMutex);
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	// when we are talking to a non-terminated server, send alive messages
	if (!client && !terminated && (net->passedTimeSince(send_stamp) > (conn_timeout/2))) {
		tmAlive alive(this, ki);
		send(alive);
	}
	
	tNetTimeDiff timeout = ackSend(); //generate ack messages (i.e. put them from the ackq to the sndq)
	
	tMutexLock sendLock(sendMutex);
	if (!sndq.empty()) {
		// cabal control (don't send too muczh at once!)
		tNetTimeDiff cur_size=0;
		// control for auto-drop of old non-acked messages
		const unsigned int minTH=15;
		const unsigned int maxTH=100;
		// max. number of allowed re-sends before timeout
		const unsigned int resendLimit = terminated ? 3 : (authenticated ? 10 : 5); // Uru clients get more resends due to their missing multi-threading
		
		// urgent packets
		tPointerList<tUnetUruMsg>::iterator it = sndq.begin();
		bool urgentSend = true; // urgent messages are founda t the beginning of the queue, special mode for them
		while (it != sndq.end()) {
			tUnetUruMsg *curmsg = *it;
			if (urgentSend && !curmsg->urgent) { // fist non-urgent messages, check next_msg_time
				if (!net->timeOverdue(next_msg_time)) break; // stop here if we are not really supposed to send messages
				urgentSend = false;
			}
			assert(urgentSend == curmsg->urgent);
			if (!urgentSend && cur_size >= maxPacketSz) break; // sent too much already (but don't stop urgent messages by tts)
			
			if(net->timeOverdue(curmsg->timestamp)) {
				DBG(8, "%s ok to send a message\n",str().c_str());
				//we can send the message
				if(curmsg->bhflags & UNetAckReq) {
					//send packet
					
					// check if we need to resend
					if(curmsg->tries) {
						DBG(3, "%s Re-sending a message (%d tries so far)\n", str().c_str(), curmsg->tries);
						decreaseCabal(curmsg->tries > 1); // if even a re-send got lost, pull the emergency break!
					}
					
					if (curmsg->tries >= resendLimit) {
						it = sndq.eraseAndDelete(it); // this is the next one
						//timeout event
						net->sec->log("%s Timeout (didn't ack a packet)\n", str().c_str());
						tNetEvent *evt=new tNetEvent(this,UNET_TIMEOUT);
						net->addEvent(evt);
					} else {
						DBG(5, "%s sending a %Zi byte acked message %d after time\n", str().c_str(), curmsg->size(), net->passedTimeSince(curmsg->timestamp));
						cur_size += curmsg->size();
						send_stamp=curmsg->snt_timestamp=net->net_time;
						rawsend(curmsg);
						curmsg->tries++;
						curmsg->timestamp=net->net_time + msg_timeout;
						if (authenticated) curmsg->timestamp += curmsg->tries*msg_timeout/2; // choose much higher timeout for client connections and icnrease it during re-sends - the clients netcore sometimes seems to be blocked long beyond any reasonable time
						if (timeout > msg_timeout) timeout = msg_timeout; // be sure to check this message on time
						++it; // go on
					}
				} else {
					//probabilistic drop (of voice, and other non-ack packets) - make sure we don't drop ack replies though!
					if(curmsg->bhflags == 0x00 && net->passedTimeSince(curmsg->timestamp) > 4*msg_timeout) {
						//Unacceptable - drop it
						net->err->log("%s Dropped a 0x00 packet due to unaceptable msg time %i,%i,%i\n",str().c_str(),msg_timeout,net->passedTimeSince(curmsg->timestamp),rtt);
					} else if(curmsg->bhflags == 0x00 && (sndq.size() > static_cast<size_t>((minTH + random()%(maxTH-minTH)))) ) {
						net->err->log("%s Dropped a 0x00 packet due to a big queue (%d messages)\n", str().c_str(), sndq.size());
					}
					//end prob drop
					else {
						DBG(5, "%s sending a %Zi byte non-acked message %d after time\n", str().c_str(), curmsg->size(), net->passedTimeSince(curmsg->timestamp));
						cur_size += curmsg->size();
						rawsend(curmsg);
						send_stamp=net->net_time;
					}
					it = sndq.eraseAndDelete(it); // go to next message, delete this one
				}
			} //end time check
			else {
				timeout = std::min(timeout, net->remainingTimeTill(curmsg->timestamp)); // come back when we want to send this message
				DBG(8,"%s Too soon (%d) to send a message\n",str().c_str(),net->remainingTimeTill(curmsg->timestamp));
				++it; // go on
			}
		} //end while
		// calculate how long it will take us to send what we just sent
		DBG(5, "%s sent packets with %d bytes\n", str().c_str(), cur_size);
		if (net->timeOverdue(next_msg_time)) {
			// "regular" send
			tNetTimeDiff cur_tts = timeToSend(cur_size);
			next_msg_time=net->net_time + cur_tts;
			if (it != sndq.end() && cur_tts < timeout) timeout = cur_tts; // if there is still something to send, but the quota does not let us, do that ASAP
		}
		else {
			// sending before we should
			DBG(5,"%s Too soon (%d) to check sndq completely\n",str().c_str(),net->remainingTimeTill(next_msg_time));
			next_msg_time += timeToSend(cur_size); // we might have sent urgent messages, account for that
			if (it != sndq.end()) timeout = std::min(timeout, net->remainingTimeTill(next_msg_time)); // come back when we want to send the next message
		}
	}
	
	// check this session's timeout
	if (net->timeOverdue(receive_stamp+conn_timeout)) {
		// timeout while terminating? Scrap this session, get away with it
		if (isTerminated()) {
			sndq.clear();
			ackq.clear();
			// this should suffice to convince the netcore to terminate us
		}
		else {
			// create timeout event
			net->sec->log("%s Timeout (didn't send a packet for %d microseconds)\n",str().c_str(),conn_timeout);
			net->addEvent(new tNetEvent(this,UNET_TIMEOUT));
		}
		return timeout;
	}
	else
		return std::min(timeout, net->remainingTimeTill(receive_stamp+conn_timeout)); // do not miss our timeout - *after* reducing it, of course
}

void tNetSession::terminating()
{
	tWriteLock lock(prvDataMutex);
	terminated = true;
}

void tNetSession::setAuthData(uint8_t accessLevel, const tString &passwd)
{
	tWriteLock lockPrv(prvDataMutex);
	this->client = true; // no matter how this connection was established, the peer definitely acts like a client
	this->accessLevel = accessLevel;
	this->passwd = passwd; // passwd is needed for validating packets
}

void tNetSession::decRefs()
{
	int refs = __sync_sub_and_fetch(&this->refs, 1);
	if (refs == 0) delete this; // we are alone :(
}

/* End session */

}
