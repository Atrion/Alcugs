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
	URUNET 3+
*/

//#define _DBG_LEVEL_ 3
#include <alcdefs.h>
#include "netsession.h"

#include "unet.h"
#include "netexception.h"
#include "protocol/umsgbasic.h"
#include <alcmain.h>

#include <cassert>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <unistd.h>

static const unsigned int maxPacketSz = 1024; //!< maxium size of the packets
static const unsigned int minCabal = 5*maxPacketSz; //!< send at least 5 packets per second

namespace alc {

/* Session */
tNetSession::tNetSession(alc::tUnet* net, uint32_t ip, uint16_t port, uint32_t sid, bool client, uint8_t validation)
: net(net), ip(ip), port(port), sid(sid), validation(validation), rcv(NULL), client(client), refs(1) {
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
	cabal=minCabal;
	consecutiveCabalIncreases = 0;
	flood_npkts=0;
	flood_last_check = next_msg_time = receive_stamp = send_stamp = net->getNetTime();
	rtt=deviation=0;
	msg_timeout=net->msg_timeout;
	conn_timeout=net->conn_timeout;
	state = Unknown;
	rejectMessages=false;
	
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
	
	DBG(5, "%s Initial msg_timeout: %f\n", str().c_str(), msg_timeout);
	// session created
	net->sec->log("%s New Connection\n",str().c_str());
	net->addEvent(new tNetEvent(this,UNET_NEWCONN));
	
	// start negotating if we are talking to a server - for a client, parse the first message, to know the validation type!
	if (!client) negotiate();
	else resetMsgCounters();
}
tNetSession::~tNetSession() {
	DBG(5,"~tNetSession() (sndq: %zd messages left)\n", sndq.size());
	delete data;
	delete rcv;
}
void tNetSession::resetMsgCounters(void) {
	tMutexLock lock(sendMutex);
	DBG(3, "tNetSession::resetMsgCounters\n");
	clientMsg.cps=0;
	serverMsg.pn=0;
	serverMsg.sn=0;
	serverMsg.cps=0;
	// empty queues
#ifdef ENABLE_ACKCACHE
	ackq.clear();
#endif
	sndq.clear();
	rcvq.clear();
	delete rcv;
	rcv = NULL;
}
tLog* tNetSession::getLog(void)
{
	return net->log;
}
tString tNetSession::str() {
	tString dbg;
	dbg.printf("[%i][%s:%i]",sid,alcGetStrIp(ip).c_str(),ntohs(port));
	tReadLock lockPub(pubDataMutex);
	if (!name.isEmpty()) {
		if (ki != 0) dbg.printf("[%s:%s,%d]", name.c_str(), avatar.c_str(), ki);
		else dbg.printf("[%s]", name.c_str());
	}
	else if (!name.isEmpty() && net->whoami == KTracking) { // we are tracking and this is a game server
		dbg.printf("[%s:%s]", name.c_str(), alcGetStrGuid(serverGuid).c_str());
	}
	return dbg;
}

size_t tNetSession::getMaxPacketSz(void) const
{
	return maxPacketSz;
}

// functions to calculate cabal and rtt
void tNetSession::updateRTT(tNetTime newread) {
	if(rtt==0) rtt=2*newread; // start with a large deviation, in case this was like an "early shot"
	//Jacobson/Karels - also see https://csel.cs.colorado.edu/~netsys03/CSCI_4273_5273_Spring_2003/Lecture_Slides/Chapter6.1.ppt
	const double alpha=0.125, delta=4;
	const double diff=newread - rtt;
	rtt       += alpha*diff;
	deviation += alpha*(fabs(diff)-deviation);
	msg_timeout= rtt + delta*deviation;
	if (msg_timeout > 5) msg_timeout = 5; // max. timeout: 5secs
	else if (msg_timeout < 0.005) msg_timeout = 0.005; // min. timeout: 5 milliseconds
	DBG(3,"%s RTT update (sample rtt: %f) new rtt:%f, msg_timeout:%f, deviation:%f\n", str().c_str(),newread,rtt,msg_timeout,deviation);
}
void tNetSession::increaseCabal() {
	++consecutiveCabalIncreases;
	cabal += maxPacketSz*log(consecutiveCabalIncreases)/10.0;
	DBG(3,"%s +Cabal is now %i (%i consecutive increases)\n",str().c_str(),cabal,consecutiveCabalIncreases);
}
void tNetSession::decreaseCabal(bool emergency) {
	consecutiveCabalIncreases = 0;
	const unsigned int delta = emergency ? 25 : 5;
	cabal -= (delta*cabal)/100;
	if(cabal < minCabal) cabal = minCabal; // don't drop below minCabal
	DBG(3,"%s %sCabal is now %i\n",str().c_str(),emergency ? "--" : "-",cabal);
}

/**
	puts the message in the session's send queue
*/
void tNetSession::send(const tmBase &msg, tNetTime delay) {
	if (msg.getSession() != this)
		throw txUnexpectedData(_WHERE("Message sent for wrong session!"));
	uint8_t flags = msg.bhflags(), val = validation;
	if (getState() >= Leaving && !(flags & (UNetAckReply|UNetNegotiation))) { // on left sessions, only allow negos and acks
		net->log->log("%s ERR: Connection already down, will not send a message to it.\n", str().c_str());
		return;
	}
	if (!(flags & UNetAckReply))
		net->log->log("%s <SND> %s\n",str().c_str(),msg.str().c_str());
	tMBuf buf;
	size_t csize,psize,hsize,pkt_sz;
	unsigned int n_pkts;
	
	buf.put(msg);
	psize=buf.size();
	buf.rewind();
	DBG(7,"Ok, I'm going to send a packet of %zi bytes, for peer %i, with flags %02X\n",psize,sid,flags);


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
		throw txTooBig(_WHERE("%s packet of %zi bytes doesn't fit inside an Uru message\n",str().c_str(),psize));
	if (n_pkts > 0 && ((flags & UNetNegotiation) || (flags & UNetAckReply)))
		throw txProtocolError(_WHERE("Nego and ack packets must not be fragmented!"));
	
	#ifdef ENABLE_NETDEBUG
	delay += net->latency;
	#endif
	net->updateNetTime(); // make sure we got correct time reference (this is also required for non-delayed packages, e.g. for dropping too old non-acked messages)
	{
		tMutexLock lock(sendMutex);
		serverMsg.sn++;
		for(unsigned int i=0; i<=n_pkts; i++) {
			//get current paquet size
			if(i==n_pkts) csize=psize - (i*pkt_sz);
			else csize=pkt_sz;
			
			tUnetUruMsg * pmsg=new tUnetUruMsg(msg.urgent());
			pmsg->val=val;
			//pmsg.pn NOT in this layer (done in the msg sender, tUnet::rawsend)
			//since urgent messages are put at the top of the sndq, the pn would be wrong if we did it differently
			pmsg->bhflags=flags;
			pmsg->set_csn(serverMsg.sn, i);
			pmsg->frt=n_pkts;
			pmsg->cps=serverMsg.cps;
			pmsg->timestamp=net->getNetTime()+delay; // no need to take tts into account now, the send queue worker does that (and our tts estimate might change till this packet is actually sent)
			pmsg->data.write(buf.read(csize),csize);
			
			if(flags & UNetAckReq) { // if this packet has the ack flag on, save it's number
				serverMsg.cps=pmsg->csn;
			}
			
			// Urgent messages are added at the top of the send queue but BEHIND other urgent packets, the rest at the back
			// this is important, or the first ack might be sent before the first nego!
			if(pmsg->urgent) {
				// search for the first non-urgent message
				for (tNetQeue<tUnetUruMsg>::iterator it = sndq.begin(); it != sndq.end(); ++it) {
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
	assert(buf.eof());
	
	if (alcGetSelfThreadId() != alcGetMain()->threadId()) net->wakeUpMainThread();
}

/** sends the message (internal use only)
An uru message can only be 253952 bytes in V0x01 & V0x02 and 254976 in V0x00
Call only with the sendMutex locked! */
void tNetSession::rawsend(tUnetUruMsg *msg)
{
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());

	DBG(9,"Server pn is %08X\n",serverMsg.pn);
	DBG(9,"Server sn is %08X,%08X\n",serverMsg.sn,msg->sn());
	serverMsg.pn++;
	msg->pn=serverMsg.pn;
	
	msg->htmlDump(net->ack,/*outgoing*/true,this);

	//store message into buffer
	tMBuf * mbuf;
	mbuf = new tMBuf();
	mbuf->put(*msg);
	
	DBG(9,"validation level is %i,%i\n",validation,msg->val);
	
	size_t msize=mbuf->size();
	uint8_t * buf, * buf2=NULL;
	buf=const_cast<uint8_t *>(mbuf->data()); // yes, we are writing directly into the tMBuf buffer... this saves us from copying everything

	if(msg->val==2) {
		DBG(8,"Encoding validation 2 packet of %zi bytes...\n",msize);
		buf2=new uint8_t[msize];
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
		net->last_quota_check = net->getNetTime();
	}
	if(msize>0 && net->lim_up_cap) {
		if((net->cur_up_quota+msize)>net->lim_up_cap) {
			DBG(5,"Paquet dropped by quotas, in use:%i,req:%li, max:%i\n",net->cur_up_quota,msize,net->lim_up_cap);
			net->log->log("Paquet dropped by quotas, in use:%i,req:%i, max:%i\n",net->cur_up_quota,msize,net->lim_up_cap);
			msize=0;
		} else {
			net->cur_up_quota+=msize;
		}
	}
#endif

	if(msize>0) {
		msize = sendto(net->sock,reinterpret_cast<char *>(buf),msize,0,reinterpret_cast<struct sockaddr *>(&sockaddr),sizeof(struct sockaddr));
		if (msize != mbuf->size())
			throw txUnet(_WHERE("Error sending a message"));
	}

	DBG(9,"After the Sendto call...\n");
	delete[] buf2;
	delete mbuf;
	DBG(8,"returning from uru_net_send RET:%zi\n",msize);
}

/** send a negotiation to the peer */
void tNetSession::negotiate() {
	unsigned int sbw;
	// send server downstream (for the peer to know how fast it can send packets)
	if((ntohl(ip) & 0xFFFFFF00) == 0x7F000000 || (ip & net->lan_mask) == net->lan_addr) { //lo or LAN
		sbw=net->lan_down;
	} else {
		sbw=net->nat_down; // WAN
	}

	resetMsgCounters();
	send(tmNetClientComm(tTime::now(),sbw,this));
	tWriteLock lock(prvDataMutex);
	state = Negotiating;
}

/** process a recieved msg: put it in the rcvq, assemble fragments, create akcs */
void tNetSession::processIncomingMsg(void * buf,size_t size) {
	DBG(7,"Message of %zi bytes\n",size);
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	// check max packet size
	if (size > maxPacketSz) { // catch impossible big messages
		throw txProtocolError(_WHERE("[%s] Recieved a too big message of %zi bytes\n",str().c_str(),size));
	}
	//stamp
	receive_stamp = net->getNetTime();
	
	/* when authenticated is false, but passwd already set, we don't expect an encoded packet, but sometimes, we get one. Since
	 * alcUruValidatePacket will try to validate the packet both with and without passwd if possible, we tell it to use the passwd
	 * whenever we have one - as a result, even if the client for some reason decides to encode a packet before we actually know
	 * the passwd, that's fine. */
	{
		int ret;
		tReadLock lock(prvDataMutex);
		ret=alcUruValidatePacket(static_cast<uint8_t*>(buf),size,&validation,!passwd.isEmpty(),passwd.c_str());
	
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
	}
	
	tSBuf mbuf(buf,size);
	
	tUnetUruMsg *msg = new tUnetUruMsg;
	
	try {
		mbuf.get(*msg);
		//Protocol Upgrade (BEFORE dumping!)
		if(msg->bhflags & UNetExt) {
			tWriteLock lock(prvDataMutex);
			upgradedProtocol = true;
		}
		msg->htmlDump(net->ack,/*outgoing*/false,this);
	} catch(txUnexpectedData &t) {
		net->err->log("%s Unexpected Data %s\nBacktrace:%s\n",str().c_str(),t.what(),t.backtrace());
		delete msg;
		throw txProtocolError(_WHERE("Cannot parse a message"));
	}

	// process negotation (will always require an ack)
	if(msg->bhflags & UNetNegotiation) {
		tmNetClientComm comm(this, msg);
		net->log->log("%s <RCV> [%d] %s",str().c_str(),msg->sn(),comm.str().c_str());
		// check what to do
		if (getState() >= Terminating) { // we do not accept negos anymore, just drop it
			net->log->print(" (dropped)\n");
			delete msg;
			return;
		}
		if(renego_stamp==comm.timestamp) { // it's a duplicate, we already got this message
			net->log->print(" (ignored)\n");
			/* It is necessary to do the check this way since the usual check by SN would treat a re-nego on an existing connection as
			 * "already parsed" since the SN is started from the beginning */
			if (msg->cps == clientMsg.cps) // messgae has correct SN
				clientMsg.cps = msg->csn; // accept it
			send(tmNetAck(this, tUnetAck(msg->csn, msg->cps))); // ack it
			delete msg; // and be done
			return;
		}
		net->log->nl();
		renego_stamp=comm.timestamp;
		if(getState() != Negotiating) {
			// if this nego came unexpectedly, reset everything and send a nego back (since the other peer expects our answer, this
			//  will not result in an endless loop of negos being exchanged)
			if (msg->csn != (1<<8) || msg->cps != 0) {
				net->err->log("%s ERR: Got a nego with a sn of 0x%08X (expected 0x%08X) and a previous ack of 0x%08X (expected 0x%08X)\n",
							  str().c_str(), msg->csn, 1<<8, msg->cps, 0);
			}
			negotiate();
		}
		clientMsg.cps = msg->csn; // the nego marks the beginning of a new connection, so accept everything from here on
		send(tmNetAck(this, tUnetAck(msg->csn, msg->cps))); // ack it after sending the nego!
		delete msg;
		
		// calculate connection bandwidth
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
		tWriteLock lock(prvDataMutex);
		state = Connected;
		return;
	}

	// Acks
	if (msg->bhflags & UNetAckReply) {
		// Process ack replies, no matter whether they are out of roder or not
		if (getState() >= Connected) {
			// don't parse acks before we are connected (if we do, we might get SN confusion if connections don't start with a Nego)
			ackCheck(*msg);
			// authentication confirmed?
			tWriteLock lock(prvDataMutex);
			if (!authenticated && !passwd.isEmpty()) authenticated = true;
		}
		delete msg;
		return;
	}
	
	// Be sure to be in the right state
	if (getState() < Connected) { // we did not yet negotiate, go ahead and DO IT
		net->log->log("%s WARN: Obviously a new connection was started with something different than a nego\n", str().c_str());
		negotiate();
		clientMsg.cps = msg->cps; // this message is the beginning of a new connection, so make below code accept it
	}
	
	// Normal packets
	{
		tReadLock lock(prvDataMutex);
		
		if (rejectMessages) {
			net->log->log("%s WARN: Rejecting messages currently, dropping packet", str().c_str());
			delete msg;
			return;
		}
	}
	// future packet and queue is full - drop
	if (msg->cps > clientMsg.cps && rcvq.size() > net->receiveAhead) {
		net->log->log("%s WARN: Dropped packet I can not yet parse: %d.%d (last ack: %d.%d, expected: %d.%d)\n", str().c_str(), msg->sn(), msg->fr(), msg->psn(), msg->pfr(), clientMsg.psn(), clientMsg.pfr());
		delete msg;
		return;
	}

	// We will accept the packet, so if send an ack if required
	if (msg->bhflags & UNetAckReq) {
		//ack reply
		createAckReply(msg);
	}
	
	// We may be able to cache it
	if (msg->cps > clientMsg.cps) {
		queueReceivedMessage(msg);
	}
	// or we already parsed it, so we drop it
	else if (msg->cps < clientMsg.cps) {
		net->log->log("%s Dropping re-sent old packet %d.%d (last ack: %d.%d, expected: %d.%d)\n", str().c_str(), msg->sn(), msg->fr(), msg->psn(), msg->pfr(), clientMsg.psn(), clientMsg.pfr());
		delete msg;
	}
	else {
		// or we parse it now
		//flood control
		if((msg->bhflags & UNetAckReq) && (msg->fr()==0) && (net->flags & UNET_FLOODCTR)) {
			if(net->passedTimeSince(flood_last_check) > net->flood_check_interval) {
				flood_last_check=net->getNetTime();
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
		acceptMessage(msg); // will delete the message

		// We might be able to accept cached messages now
		checkQueuedMessages();
	}
}

/** add this to the received message and put fragments together - this deletes the passed message! */
void tNetSession::acceptMessage(tUnetUruMsg *t)
{
	assert(!(t->bhflags & UNetNegotiation));
	assert(t->cps == clientMsg.cps);
	DBG(5, "Accepting %d.%d\n", t->sn(), t->fr());
	if (t->bhflags & UNetAckReq) {
		clientMsg.cps = t->csn;
	}

	size_t frg_size = maxPacketSz - t->hSize();
	if (!rcv) { // this is a brand new message
		rcv=new tUnetMsg();
		rcv->sn=t->sn();
	}
	else {
		if (rcv->sn != t->sn()) throw txProtocolError(_WHERE("I am assembling message nr. %d and received nr. %d?!?", rcv->sn, t->sn()));
	}
	if (t->fr() != rcv->fr_count) throw txProtocolError(_WHERE("Expected fragment %d, got %d\n", rcv->fr_count, t->fr()));

	// Got the next one!
	rcv->data.put(t->data);

	if(rcv->fr_count==t->frt) {
		size_t size = t->frt * frg_size + t->data.size();
		if (rcv->data.size() != size)
			throw txProtocolError(_WHERE("Expected a size of %zd, got %zd\n", size, rcv->data.size()));
		// We are done!
		rcv->data.rewind();
		rcv->cmd=rcv->data.get16();
		if (gameType == UUGame) rcv->cmd = alcOpcodeUU2POTS(rcv->cmd);
		rcv->data.rewind();
		
		if (parseBasicMsg(rcv))
			delete rcv; // we dealt with the message ourselves
		else {
			tNetEvent *evt=new tNetEvent(this, UNET_MSGRCV, rcv);
			net->addEvent(evt);
		}
		rcv = NULL; // this packet is finished, don't deal with it any longer
	}
	else {
		rcv->fr_count++;
	}
	delete t;
}

/** Saves a received, not-yet accepted message for future use */
void tNetSession::queueReceivedMessage(tUnetUruMsg *msg)
{
	net->log->log("%s Queuing %d.%d for future use (can not yet accept it - last ack: %d.%d, expected: %d.%d)\n", str().c_str(), msg->sn(), msg->fr(), msg->psn(), msg->pfr(), clientMsg.psn(), clientMsg.pfr());
	// the queue is sorted ascending
	for (tNetQeue<tUnetUruMsg>::iterator it = rcvq.begin(); it != rcvq.end(); ++it) {
		if ((*it)->csn > msg->csn) { // we have to put it before the current one! (it's the first one to be bigger)
			rcvq.insert(it, msg);
			return;
		}
		else if (msg->csn == (*it)->csn) { // the message is already in the queue
			delete msg;
			return;
		}
	}
	rcvq.push_back(msg); // insert it at the end
}

/** Checks if messages saved by queueReceivedMessage can now be used */
void tNetSession::checkQueuedMessages(void)
{
	tNetQeue<tUnetUruMsg>::iterator it = rcvq.begin();
	while (it != rcvq.end()) {
		if ((*it)->cps < clientMsg.cps) it = rcvq.eraseAndDelete(it); // old packet
		else if ((*it)->cps == clientMsg.cps) {
			DBG(5, "%s can now accept queued message %d.%d\n", str().c_str(), (*it)->sn(), (*it)->fr());
			acceptMessage(*it);
			it = rcvq.erase(it); // do not delete!
		}
		else return; // These can still not be accepted
	 }
}

/** creates an ack in the ackq */
void tNetSession::createAckReply(const tUnetUruMsg *msg) {
	uint32_t A,B;
	
	// we can ack everything between B and A
	A=msg->csn;
	B=msg->cps;
	assert(A > B);
#ifndef ENABLE_ACKCACHE
	// send acks directly
	send(tmNetAck(this, tUnetAck(A, B)));
#else
	// retard and collect acks
	// we must delay either none or all messages, otherwise the rtt will vary too much
	// do not use the rtt as basis for the delay, or the rtts of both sides will wind up endlessly
	tNetTime ackWaitTime = 2.0*timeToSend(maxPacketSz); // wait for some batches of packets
	if (ackWaitTime > msg_timeout/4.0) ackWaitTime=msg_timeout/4.0; // but do not wait too long
	tNetTime timestamp=net->getNetTime() + ackWaitTime;
	// the net timer will be updated when the ackq is checked (which is done since processMsg will call doWork after calling createAckReply)
	DBG(3, "%s Enqueueing new ack, wait time: %f\n",str().c_str(),ackWaitTime);

	//Plasma like ack's (acks are retarded, and packed)
	tAckList::iterator it = ackq.begin();
	while (it != ackq.end()) {
		if (A>=it->B && B<=it->A) { // the two acks intersect, merge them
			// calculate new bounds and timestamp
			if (it->A > A) A = it->A;
			if (it->B < B) B = it->B;
			if (it->timestamp < timestamp) timestamp = it->timestamp; // use smaller timestamp
			// remove existing ack and complete merge
			ackq.erase(it); // remove old ack (we merged it into ours)
			it = ackq.begin(); // and restart
			DBG(9, "merged two acks, restarting ack search\n");
			continue;
		} else if (B>it->A) { // we are completely after this ack
			++it;
			continue; // go on searching and looking
		} else if (A<it->B) { // we are completely before this ack
			ackq.insert(it, tUnetAck(A, B, timestamp)); // insert ourselves in the list at the right position, and be done
			return; // done!
		} else {
			throw txBase(_WHERE("The impossible happened"));
		}
	}
	ackq.push_back(tUnetAck(A, B, timestamp));
#endif
}

/** parse the ack and remove the messages it acks from the sndq */
void tNetSession::ackCheck(tUnetUruMsg &t) {
	uint32_t A1,A2,A3;
	tmNetAck ackMsg(this, &t);
	tMutexLock lock(sendMutex);
	for (tmNetAck::tAckList::iterator it = ackMsg.acks.begin(); it != ackMsg.acks.end(); ++it) {
		A1 = it->A;
		A3 = it->B;
		//well, do it
		tNetQeue<tUnetUruMsg>::iterator jt = sndq.begin();
		while (jt != sndq.end()) {
			tUnetUruMsg *msg = *jt;
			A2=msg->csn;
			
			if(msg->bhflags & UNetAckReq && A1>=A2 && A2>A3) {
				// this packet got acked, delete it
				increaseCabal(); // a packet was properly delivered
				if(msg->tries == 1) { // only update rtt for a first-time success
					// calculate the time it took to ack this packet
					tNetTime crtt=net->passedTimeSince(msg->snt_timestamp);
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

/** Deals with messages that are actually part of the netcore: keep-alive and connection shutdown. */
bool tNetSession::parseBasicMsg(tUnetMsg * msg)
{
	switch(msg->cmd) {
		/* I am not sure what the correct "end of connection" procedure is, but here are some observations:
		- When the client leaves, it sends a NetMsgLeaves and expects an ack
		- When I send the client a NetMsgTerminated, it sends an ack back and then starts the normal leave (i.e., it sends a NetMsgLeave)
		- When I send the client a NetMsgLeave, it acks and ignores the message
		So I conclude that only the client can actually terminate a connection, the server must ask the client to get away.
		Which is of course crazy, so we remember that we sent a terminate and generally treat this session as terminated. */
		case NetMsgLeave:
		{
			// accept it even if it is NOT a client - in that case, the peer obviously thinks it is a client, so lets respect its wish, it doesn't harm
			tmLeave msgleave(this, msg);
			/* Ack the current message, then terminate the connection */
			tWriteLock lock(prvDataMutex);
			if (state <= Connected)
				net->addEvent(new tNetEvent(this, UNET_CONNCLS, new tContainer<uint8_t>(msgleave.reason))); // trigger the event in the worker
			state = Leaving;
			return true;
		}
		case NetMsgTerminated:
		{
			// accept it even if it IS a client - in that case, the peer obviously thinks it is a server, so lets respect its wish, it doesn't harm
			tmTerminated msgterminated(this, msg);
			/* Ack the current message and terminate the connection */
			terminate(msgterminated.reason);
			return true;
		}
		case NetMsgAlive:
		{
			tmAlive alive(this, msg);
			return true;
		}
	}
	return false;
}

#ifdef ENABLE_ACKCACHE
/** puts acks from the ackq in the sndq */
tNetTime tNetSession::ackSend() {
	tNetTime timeout = std::numeric_limits<double>::max();
	
	tAckList::iterator it = ackq.begin();
	while (it != ackq.end()) {
		if (!net->timeOverdue(it->timestamp)) {
			timeout = std::min(timeout, net->remainingTimeTill(it->timestamp)); // come back when we want to process this ack
			++it;
		}
		else {
			DBG(5, "%s Sending an ack, %f after time\n", str().c_str(), net->passedTimeSince(it->timestamp));
			/* send one message per ack: we do not want to be too pwned if that apcket got lost. And if we have "holes" in the
			 * sequence of packets (which is the only occasion in which there would be several acks in a message), the connection
			 * is already problematic. */
			send(tmNetAck(this, *it));
			it = ackq.erase(it); // remove ack from queue
		}
	}
	return timeout;
}
#endif

/** Send, and re-send messages */
tNetTimeBoolPair tNetSession::processSendQueues()
{
	tReadLock dataLock(prvDataMutex);
	assert(alcGetSelfThreadId() == alcGetMain()->threadId());
	// when we are talking to a non-terminated server, send alive messages
	if (!client && state == Connected && (net->passedTimeSince(send_stamp) > (conn_timeout/2))) {
		send(tmAlive(this, ki));
	}
	
#ifdef ENABLE_ACKCACHE
	tNetTime timeout = ackSend(); //generate ack messages (i.e. put them from the ackq to the sndq)
#else
	tNetTime timeout = std::numeric_limits<double>::max();
#endif
	
	{
		tMutexLock sendLock(sendMutex);
		if (!isUruClient() && sndq.empty() && !acksToSend()) { // send and ack queue empty (the Uru client accepts a re-nego but will not decrease the numbers)
			// fix the problem that happens every 15-30 days of server uptime - but only if sndq is empty, or we will loose packets
			if (serverMsg.sn >= (1 << 22) || clientMsg.psn() >= (1 << 22)) { // = 2^22
				net->log->log("%s WARN: Congratulations! You have reached the maxium allowed sequence number. "
							"Don't worry, this is not an error\n", str().c_str());
				renego_stamp = tTime();
				sendMutex.unlock(); // unlock here, because negotiate will lock again
				negotiate();
				sendMutex.lock(); // and relock
				// do this before the send queue check, so that the nego has a chance to be sent immediately
			}
		}
		if (!sndq.empty()) {
			// cabal control (don't send too much at once!)
			unsigned int cur_size=0;
			// control for auto-drop of old non-acked messages
			const unsigned int minTH=15;
			const unsigned int maxTH=100;
			/* Max. number of allowed re-sends before timeout:
			* In production, I have the weird issue of the whole server being "asleep" for 0.5 to 1.0 seconds, and the connections to
			* other servers dropping in that time. So do as many resends as are necessary to wait some seconds:
			* n*(n+1)/2*timeout = waittime  <=>  n^2 + n - 2*waittime/timeout = 0  <=>  n = -0.5 + sqrt(-0.25 + 2*waittime/timeout)
			* Also, don't wait more than conn_timeout. */
			const tNetTime minWaitTime = std::min(conn_timeout, 3.0);
			const double numResends = sqrt(2.0*minWaitTime/msg_timeout - 0.25) - 0.5;
			const unsigned int resendLimit = std::max(5u, static_cast<unsigned int>(ceil(numResends))); // always round up, and give it at least 5 chances
			
			// urgent packets
			tNetQeue<tUnetUruMsg>::iterator it = sndq.begin();
			bool urgentSend = true; // urgent messages are found at the beginning of the queue, special mode for them
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
						
						/* Choose higher timeout and limits for client connections - the client's netcore sometimes seems
						 * to be blocked long beyond any reasonable time (even when *sending* a whole lot of
						 * packets, e.g. when linking to Noloben, it does not reply to acks). Hence we add some more retries,
						 * and we extend the timeouts per retry. */
						const double client_slack = authenticated ? 1.5 : 1.0;
						
						if (curmsg->tries >= resendLimit*client_slack) {
							it = sndq.eraseAndDelete(it); // this is the next one
							//timeout event
							net->sec->log("%s Timeout (didn't ack a packet)\n", str().c_str());
							tNetEvent *evt=new tNetEvent(this,UNET_TIMEOUT);
							net->addEvent(evt);
						} else {
							DBG(5, "%s sending a %zi byte acked message %f after time\n", str().c_str(), curmsg->size(), net->passedTimeSince(curmsg->timestamp));
							cur_size += curmsg->size();
							send_stamp=curmsg->snt_timestamp=net->getNetTime();
							rawsend(curmsg);
							curmsg->tries++;
							double curmsg_timeout = msg_timeout*curmsg->tries*client_slack; // increase timeout for further re-sends
							curmsg->timestamp=net->getNetTime() + curmsg_timeout;
							if (timeout > curmsg_timeout) timeout = curmsg_timeout; // be sure to check this message on time
							++it; // go on
						}
					} else {
						//probabilistic drop (of voice, and other non-ack packets) - make sure we don't drop ack replies though!
						if(curmsg->bhflags == 0x00 && net->passedTimeSince(curmsg->timestamp) > 4*msg_timeout) {
							//Unacceptable - drop it
							net->err->log("%s Dropped a 0x00 packet due to unaceptable msg time %f,%f,%f\n",
										str().c_str(),msg_timeout,net->passedTimeSince(curmsg->timestamp),rtt);
						} else if(curmsg->bhflags == 0x00 && (sndq.size() > static_cast<size_t>((minTH + random()%(maxTH-minTH)))) ) {
							net->err->log("%s Dropped a 0x00 packet due to a big queue (%zd messages)\n", str().c_str(), sndq.size());
						}
						//end prob drop
						else {
							DBG(5, "%s sending a %zi byte non-acked message %f after time\n", str().c_str(), curmsg->size(), net->passedTimeSince(curmsg->timestamp));
							cur_size += curmsg->size();
							rawsend(curmsg);
							send_stamp=net->getNetTime();
						}
						it = sndq.eraseAndDelete(it); // go to next message, delete this one
					}
				} //end time check
				else {
					timeout = std::min(timeout, net->remainingTimeTill(curmsg->timestamp)); // come back when we want to send this message
					DBG(8,"%s Too soon (%f) to send a message\n",str().c_str(),net->remainingTimeTill(curmsg->timestamp));
					++it; // go on
				}
			} //end while
			// calculate how long it will take us to send what we just sent
			DBG(5, "%s sent packets with %d bytes at cabal %d\n", str().c_str(), cur_size, cabal);
			if (net->timeOverdue(next_msg_time)) {
				// "regular" send
				tNetTime cur_tts = timeToSend(cur_size);
				next_msg_time=net->getNetTime() + cur_tts;
				if (it != sndq.end() && cur_tts < timeout) timeout = cur_tts; // if there is still something to send, but the quota does not let us, do that ASAP
			}
			else {
				// sending before we should
				DBG(5,"%s Too soon (%f) to check sndq completely\n",str().c_str(),net->remainingTimeTill(next_msg_time));
				next_msg_time += timeToSend(cur_size); // we might have sent urgent messages, account for that
				if (it != sndq.end()) timeout = std::min(timeout, net->remainingTimeTill(next_msg_time)); // come back when we want to send the next message
			}
		}
	} // unlock the send mutex: it is *within* the prvDatMutex
	if (state == Leaving && !anythingToSend()) { // we are totaly idle and got nothing to do
		DBG(5, "Session isleft and queues are empty, waiting only a little longer\n");
		prvDataMutex.unlock();
		prvDataMutex.write(); // will be unlocked by destructor of the prvLock
		conn_timeout = 5*msg_timeout; // wait for some time in case we get retarded acks etc.
	}
	
	// check this session's timeout
	if (net->timeOverdue(receive_stamp+conn_timeout)) {
		// timeout while terminating? We are done!
		if (state == Leaving) {
			return tNetTimeBoolPair(timeout, true);
		}
		else {
			// create timeout event
			net->sec->log("%s Timeout (didn't send a packet for %f seconds)\n",str().c_str(),conn_timeout);
			net->addEvent(new tNetEvent(this,UNET_TIMEOUT));
			return tNetTimeBoolPair(timeout, false);
		}
	}
	return tNetTimeBoolPair(std::min(timeout, net->remainingTimeTill(receive_stamp+conn_timeout)), false); // do not miss our timeout - *after* reducing it, of course
}

void tNetSession::terminate(uint8_t reason)
{
	DBG(5, "%s is being terminated\n", str().c_str());
	tReadLock lock(prvDataMutex);
	if (state >= Terminating) { // 2nd attempt to terminate, its enough
		net->log->log("%s WARN: Is already terminated, speeding up disconnect...\n", str().c_str());
		prvDataMutex.unlock();
		prvDataMutex.write(); // will be unlocked by destructor of lock
		state = Leaving; // make sure this session goes down ASAP
		if (alcGetSelfThreadId() != alcGetMain()->threadId()) net->wakeUpMainThread(); // in case this was triggered from another thread
		return;
	}
	else
		net->addEvent(new tNetEvent(this, UNET_CONNCLS, new tContainer<uint8_t>(reason))); // trigger the event in the worker
	// it's our wish for that client to get away, so try to do it nice and polite
	if (!reason) reason = client ? RKickedOff : RQuitting;
	if (client) {
		{
			tReadLock lock(pubDataMutex); // we might be in main thread
			send(tmTerminated(this,reason));
		}
		// we expect a NetMsgLeave from the other side, then we can go down
		prvDataMutex.unlock();
		prvDataMutex.write(); // will be unlocked by destructor of lock
		state = Terminating;
	}
	else {
		{
			tReadLock lock(pubDataMutex); // we might be in main thread
			send(tmLeave(this,reason));
		}
		// Now that we sent that message, the other side must ack it, then tNetSession will be deleted
		prvDataMutex.unlock();
		prvDataMutex.write(); // will be unlocked by destructor of lock
		state = Leaving;
	}
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
