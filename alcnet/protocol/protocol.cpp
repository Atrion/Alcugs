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

/** The Uru protocol, is here */

//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "protocol.h"

#include "netsession.h"
#include "netexception.h"
#include <alcmain.h>

#include <cstring>
#include <cassert>


namespace alc {

/**
 \brief Encodes the specific packet into Uru validation level 2
   k = offset MOD 8
   enc: c = x * 2 ^ k MOD 255
*/
void alcEncodePacket(uint8_t* buf2,const uint8_t* buf, int n) {
	int i;
	for(i=0; i<n; i++) {
		buf2[i] = buf[i] << (i%8) | buf[i] >> (8-(i%8));
	}
}

/**
 Decodes the specific packet from Uru validation level 2
   k = offset MOD 8
   dec: x = c * 2 ^ (8-k) MOD 255
*/
void alcDecodePacket(uint8_t* buf, int n) {
	int i;
	for(i=0; i<n; i++) {
		buf[i] = buf[i] >> (i%8) | buf[i] << (8-(i%8));
	}
}

#if 0
/** Debug the V1 checksum */
U32 alcUruChecksum1Trace(Byte * buf, int size) {
	int i;
	U32 aux=0;
	//S32 saux=0;
	//Byte * md5buffer;
	//Byte hash[16];
	//int aux_size; //auxiliar size
	int whoi=0;

	DBG(4,"Checksum %i requested, packet size is %i...\n",1,size);
	//be sure that the last chunck of bytes is zero
	for(i=size; i<size+4; i++) {
		if(i>=INC_BUF_SIZE) { break; }
		buf[i]=0;
	}
	for(i=6; i<size; i=i+4) {
		aux = aux + *((U32 *)(buf+i)); //V1 - Checksum algorithm ~ it fails in some messages :/
		print2log(f_chk," %08X   -  %08X\n",*((U32 *)(buf+i)),aux);
	}
	//extension <--
	whoi=((size-6)%4);
	print2log(f_chk,"------------- lst chunck %i bytes\n",whoi);
	print2log(f_chk," %08X  -->>",aux);
	if(whoi==2) {
		aux -= *((U32 *)(buf+size-2));
		aux += *((Byte *)(buf+size-1));
	} else if(whoi==3) {
		aux -= *((U32 *)(buf+size-3));
	}
	//aux=saux;
	print2log(f_chk," %08X\n\n",aux);
	return aux;
}
#endif

/**
  \brief Computes the Uru known checksums
  \param buf The data to calculate the checksum for
  \param size Size of the data
  \param alg Checksum algorithm
	 0 -> prime sum,
	 1 -> live md5 sum,
	 2 -> live md5 + passwd sum,
  \param aux_hash Password hash for algorithm 2
  \note REMEMBER THAT aux_hash must be 32 bytes and contain an ascii hash
*/
uint32_t alcUruChecksum(const uint8_t* buf, int size, int alg, const char * aux_hash) {

	DBG(4,"Checksum %i requested, packet size is %i...\n",alg,size);
	switch(alg) {
		case 0:
		{
			int i;
			uint32_t aux=0; //little-endian order when returned
			int whoi=0;
			for(i=6; i<(size-4); i=i+4) {
				uint32_t val;
				memcpy(&val, buf+i, 4);
				aux = aux + letoh32(val); //V1 - working Checksum algorithm
			}
			whoi=((size-6)%4);
			if(whoi==1 || whoi==2) {
				aux += *(buf+size-1);
			} else if(whoi==0) {
				uint32_t val;
				memcpy(&val, buf+size-4, 4);
				aux += letoh32(val);
			} //else if whoi==3 Noop
			return htole32(aux);
		}
		case 1:
		case 2:
		{
			//code for the V2 - Checksum algorithm
			tMD5Buf md5buf;
			md5buf.write(buf+6, size-6);
			//Concatenate the ASCII passwd md5 hash as required by V2 algorithm
			if(alg==2) {
				md5buf.write(aux_hash, 32);
			}
			md5buf.compute();
			return md5buf.get32();
		}
	}
	alcGetMain()->err()->log("ERR: Uru Checksum V%i is currently not supported in this version of the server.\n\n",alg);
	return 0xFFFFFFFF;
}

/**
\brief Validates a packet
\note 
	1st- Gets the validation level,
	2nd- Checks if is a valid Uru Protocol formated packet,
	3th- compute the checksum,
	4th- Decode the packet,
	5th- put the results in the session struct,

	Net - NO

\return
	 0 -> Yes, all went OK! :)
	 1 -> Ok, but checksum failed :(
	 2 -> Validation level too high
	 3 -> Bogus packet!!
*/
int alcUruValidatePacket(uint8_t * buf,int n,uint8_t * validation,bool authed,const char * phash) {
	uint32_t checksum;
#ifndef _NO_CHECKSUM
	uint32_t aux_checksum;
#endif
	if(n>=6 && buf[0]==0x03) { //magic uru number
		if(buf[1]!=0) {
			*validation=buf[1]; //store the validation level
		}
		if(buf[1]==0x00) { return 0; } //All went OK with validation level 0
		memcpy(&checksum,buf+0x02,4); //store the checksum
#ifndef _NO_CHECKSUM
		if(buf[1]==0x01) { //validation level 1
			aux_checksum=alcUruChecksum(buf, n, 0, NULL);
		} else
#endif
		if(buf[1]==0x02) { //validation level 2
#ifndef _NO_CHECKSUM
			if(authed) { //authenticated?
				aux_checksum=alcUruChecksum(buf, n, 2, phash);
			} else {
				aux_checksum=alcUruChecksum(buf, n, 1, NULL);
			}
#endif
			alcDecodePacket(buf,n); //Now the packet will be readable
			buf[1]=0x02; //be sure to take bak the 0x02 byte to the header
		}
#ifndef _NO_CHECKSUM
		else {
			return 2; //humm??
		}
		if(aux_checksum==checksum) { return 0; } //All went ok with v1/v2
		else {
			//work around (swap the checksum algorithms)
			// however if authed is false, phash is not initialized, so we can't swap algorithms
			if(buf[1]==0x02 && authed) { //validation level 2
				alcEncodePacket(buf,buf,n);
				buf[1]=0x02;
				aux_checksum=alcUruChecksum(buf, n, 1, NULL);
				alcDecodePacket(buf,n); //Now the packet will be readable
				//be sure to take bak the 0x02 byte to the header
				buf[1]=0x02;
				if(aux_checksum==checksum) { return 0; } //All went ok with v1
			}
			/*if (authed)
				lerr->log("ERR: Validation level %i (authed), got: %08X exp: %08X\n",*validation,checksum,aux_checksum);
			else
				lerr->log("ERR: Validation level %i (not authed), got: %08X exp: %08X\n",*validation,checksum,aux_checksum);*/
			/*if(u->validation==1) {
				uru_checksum1trace(buf, n);
			}
			dumpbuf(f_chk,buf,n);
			lognl(f_chk);
			u->validated=0;*/
			return 1;
		} //:(
#else
		return 0; //allow all packets
#endif
	}
	return 3; //aiieee!!!
}

//Unet Uru Message
void tUnetUruMsg::store(tBBuf &t) {
	size_t hsize=28;
	t.seek(1);
	val=t.get8();
	if(val>0) {
		t.seek(4);
		hsize+=4; //32
	}
	pn=t.get32();
	bhflags=t.get8();
	// Catch unknown or unhandled flags
	uint8_t check = UNetNegotiation | UNetAckReq | UNetAckReply | UNetExt;
	if (bhflags & ~(check)) throw txUnexpectedData(_WHERE("Problem parsing uru msg flag %08X\n",bhflags & ~(check)));
	// Catch invalid Flag combinations
	if((bhflags & UNetNegotiation) && !(bhflags & UNetAckReq)) throw txProtocolError(_WHERE("Negos must require an ack"));
	if((bhflags & UNetNegotiation) && (bhflags & UNetAckReply)) throw txUnexpectedData(_WHERE("That must be a maliciusly crafted paquet, flags UNetAckReply and UNetNegotiation cannot be set at the same time"));
	if((bhflags & UNetAckReq) && (bhflags & UNetAckReply)) throw txUnexpectedData(_WHERE("That must be a maliciusly crafted paquet, flags UNetAckReply and UNetAckReq cannot be set at the same time"));
	if(bhflags & UNetExt) {
		hsize-=8; //20 - 24
		csn=t.get32();
		frt=t.get8();
	} else {
		if(t.get32()!=0) throw txUnexpectedData(_WHERE("Non-zero unk1"));
		csn=t.get32();
		frt=t.get8();
		if(t.get32()!=0) throw txUnexpectedData(_WHERE("Non-zero unk2"));
	}
	if (frt > 0 && ((bhflags & UNetNegotiation) || (bhflags & UNetAckReply)))
		throw txProtocolError(_WHERE("Nego and ack packets must not be fragmented!"));
	if (frt > 0 && !(bhflags & UNetAckReq))
		throw txProtocolError(_WHERE("Non-acked packets must not be fragmented!"));
	if (fr() > frt)
		throw txProtocolError(_WHERE("Packet has more fragments than it should?"));
	cps=t.get32();
	uint32_t dsize=t.get32();
	if(dsize==0) throw txUnexpectedData(_WHERE("A zero sized message!?"));
	if (t.tell() != hsize) throw txUnexpectedData(_WHERE("Header size mismatch, %Zd != %Zd", t.tell(), hsize));
	// done with the header - read data and check size
	data.clear();
	if(bhflags & UNetAckReply) {
		uint32_t xdsize= (bhflags & UNetExt) ? dsize*8 : (dsize*16)+2;
		data.write(t.read(xdsize),xdsize);
	} else {
		data.write(t.read(dsize),dsize);
	}
	data.rewind();
	if (!t.eof()) throw txUnexpectedData(_WHERE("Message size mismatch"));
}
void tUnetUruMsg::stream(tBBuf &t) const {
	// this will be overwritten by sender
	t.put8(0x03); //already done by the sender ()
	t.put8(val);
	if(val>0) t.put32(0xFFFFFFFF);
	t.put32(pn); //generic pkg counter (re-written by the sender() )
	//next part is not touched by the sender
	t.put8(bhflags);
	if(bhflags & UNetExt) {
		t.put32(csn);
		t.put8(frt);
	} else {
		t.put32(0); //4 blanks
		t.put32(csn);
		t.put8(frt);
		t.put32(0);
	}
	t.put32(cps);
	// write dsize
	if(bhflags & UNetAckReply) {
		if(bhflags & UNetExt)
			t.put32(data.size()/8);
		else
			t.put32((data.size()-2)/16);
	} else {
		t.put32(data.size());
	}
	t.put(data);
}
size_t tUnetUruMsg::size() {
	return data.size() + hSize();
}
size_t tUnetUruMsg::hSize() {
	size_t hsize=28;
	if(val>0) hsize+=4;
	if(bhflags & UNetExt) hsize-=8;
	return hsize;
}
void tUnetUruMsg::htmlDump(tLog * log, bool outgoing, tNetSession *u) {
	if (!log->doesPrint()) return;
	log->checkRotate(250);
	log->print(tTime::now().str());

	switch(bhflags) {
		case UNetAckReply: //0x80
		case UNetAckReply | UNetExt:
			log->print(": <font color=red>");
			break;
		case UNetNegotiation | UNetAckReq: //0x42
		case UNetNegotiation | UNetAckReq | UNetExt:
			log->print(": <font color=green>");
			break;
		case 0x00: //0x00
		case UNetExt:
			log->print(": <font color=blue>");
			break;
		case UNetAckReq: //0x02
		case UNetAckReq | UNetExt:
			log->print(": <font color=black>");
			break;
		default:
			log->print(": <font color=pink>");
			break;
	}

	if(!outgoing) {
		log->print("<b>me &lt;- "); // incoming
	} else {
		log->print("me -&gt; ");
	}
	log->print("%i {%i,%i (%i) %i,%i} ",pn,sn(),fr(),frt,psn(),pfr());

	if(!outgoing) {
		log->print(" &lt;- %s</b> ", u->str().c_str());
	} else {
		log->print(" -&gt; %s ", u->str().c_str());
	}

	switch(bhflags) {
		case UNetAckReply: //0x80
		case UNetAckReply | UNetExt:
			log->print("Ack");
			break;
		case UNetNegotiation | UNetAckReq: //0x42
		case UNetNegotiation | UNetAckReq | UNetExt:
			log->print("Negotiation");
			break;
		case 0x00: //0x00
		case UNetExt:
			log->print("plNetMsg (non-acked)");
			break;
		case UNetAckReq: //0x02
		case UNetAckReq | UNetExt:
			log->print("plNetMsg (acked)");
			break;
	}

	log->print("</font>: ");
	
	data.rewind(); // in case we are outgoing, we might not be rewinded
	switch (bhflags) {
		case 0x00: //0x00
		case UNetExt:
		case UNetAckReq: //0x02
		case UNetAckReq | UNetExt:
			if (fr() == 0)
				log->print("%s",alcUnetGetMsgCode(data.get16()));
			else
				log->print("Fragment...");
			break;
		case UNetAckReply: //0x80
		case UNetAckReply | UNetExt:
		{
			tmNetAck ack(u, this);
			for (tmNetAck::tAckList::iterator it = ack.acks.begin(); it != ack.acks.end(); ++it) {
				if (it != ack.acks.begin()) log->print("| ");
				log->print("%i,%i %i,%i", it->snA(), it->frA(), it->snB(), it->frB());
			}
			break;
		}
		case UNetNegotiation | UNetAckReq: //0x42
		case UNetNegotiation | UNetAckReq | UNetExt:
		{
			tmNetClientComm nego(u, this);
			log->print("Bandwidth: %i bps, time: %s",nego.bandwidth,nego.timestamp.str().c_str());
			break;
		}
	}
	data.rewind();

	log->print("<br>\n");
}

//Negotiation
void tmNetClientComm::store(tBBuf &t) {
	bandwidth=t.get32();
	t.get(timestamp);
	if (!t.eof()) throw txUnexpectedData(_WHERE("Nego is too long"));
}
void tmNetClientComm::stream(tBBuf &t) const {
	t.put32(bandwidth);
	t.put(timestamp);
}
tString tmNetClientComm::str() const {
	tString str;
#ifdef ENABLE_MSGLOG
	str.printf("(Re)Negotation (bandwidth: %i bps, time: %s)",bandwidth,timestamp.str().c_str());
#else
	str.printf("(Re)Negotation");
#endif
	return str;
}

// Ack
tmNetAck::tmNetAck(tNetSession *u, const tUnetAck &ack) : tmBase(u)
{
	acks.push_back(ack);
}
void tmNetAck::store(tBBuf &t)
{
	bool upgraded = u->useUpdatedProtocol();
	acks.clear();
	if (!upgraded) {
		if(t.get16() != 0) throw txUnexpectedData(_WHERE("ack unknown data"));
	}
	while (!t.eof()) {
		uint32_t A, B;
		A=t.get32();
		if(!upgraded)
			if(t.get32()!=0) throw txUnexpectedData(_WHERE("ack unknown data"));
		B=t.get32();
		if(!upgraded)
			if(t.get32()!=0) throw txUnexpectedData(_WHERE("ack unknown data"));
		if (A <= B) throw txUnexpectedData(_WHERE("ack A value is <= B value: %d <= %d", A, B));
		acks.push_back(tUnetAck(A, B));
	}
	// no need to check for eof, the loop already does that
}
void tmNetAck::stream(tBBuf &t) const
{
	bool upgraded = u->useUpdatedProtocol();
	if (!upgraded) t.put16(0);
	for (tAckList::const_iterator it = acks.begin(); it != acks.end(); ++it) {
		t.put32(it->A);
		if(!upgraded)
			t.put32(0);
		t.put32(it->B);
		if(!upgraded)
			t.put32(0);
	}
}
tString tmNetAck::str() const {
	tString dbg;
	dbg.printf("Ack");
	#ifdef ENABLE_MSGLOG
	bool firstOne = true;
	for (tAckList::const_iterator it = acks.begin(); it != acks.end(); ++it) {
		if (firstOne)
			firstOne = false;
		else
			dbg.printf(" |");
		dbg.printf(" %i,%i %i,%i", it->snA(), it->frA(), it->snB(), it->frB());
	}
	#endif
	return dbg;
}

//Base message
tmNetMsg::tmNetMsg(uint16_t cmd,uint32_t flags,tNetSession * u) : tmBase(u) {
	DBG(5,"tmNetMsg()\n");
	this->cmd=cmd;
	this->flags=flags;
	// get version froms session
	if (this->flags & plNetVersion) {
		max_version = u->max_version;
		min_version = u->min_version;
	}
}
void tmNetMsg::store(tBBuf &t) {
	if (!u)  throw txProtocolError(_WHERE("attempt to save message without session being set"));
	//base
	cmd=t.get16();
	if (u->gameType == tNetSession::UUGame) cmd = alcOpcodeUU2POTS(cmd);
	flags=t.get32();
	if(flags & plNetVersion) {
		max_version=t.get8();
		min_version=t.get8();
		// this overrides existing values
		tWriteLock lock(u->pubDataMutex);
		u->max_version=max_version;
		u->min_version=min_version;
	} else {
		max_version=0;
		min_version=0;
	}
	
	// The first message from Plasma clients is always an auth hello that contains the version numbers
	// besides, NetMsgPing should have always the timestamp enabled in new versions
	
	
	if(flags & plNetTimestamp || (u->min_version<6 && u->max_version==12)) {
		t.get(timestamp);
	} else {
		timestamp = tTime();
	}
	if(flags & plNetX) {
		x=t.get32();
	} else {
		x=0;
	}
	if(flags & plNetKi) {
		ki=t.get32();
	} else {
		ki=0;
	}
	if(flags & plNetUID) {
		memcpy(uid,t.read(16),16);
	} else {
		memset(uid,0,16);
	}
	if(flags & plNetSid) {
		sid=t.get32();
	}
	else sid = 0;

	uint32_t check = plNetAck | plNetVersion | plNetTimestamp | plNetX | plNetKi | plNetUID | plNetSid | plNetSystem /*unknown purpose*/;
	// message-specific flags
	if (cmd == NetMsgGameMessageDirected || cmd == NetMsgCustomDirectedFwd)
		check |= plNetDirected; // Alcugs uses the message type to check if the msg needs to be forwarded to tracking
	if (cmd == NetMsgGameStateRequest)
		check |= plNetStateReq1; // one could also use the number of requested pages to check if this is the initial state request
	// some message-specific flags which most likely tell the client how to treat that whole message
	if (cmd == NetMsgGameMessage || cmd == NetMsgSDLStateBCast)
		check |= plNetRelRegions;
	if (cmd == NetMsgSDLState || cmd == NetMsgSDLStateBCast)
		check |= plNetNewSDL;
	if (cmd == NetMsgGameMessage || cmd == NetMsgGameMessageDirected || cmd == NetMsgCustomDirectedFwd)
		check |= plNetMsgRecvrs; // whatever the purpose of this flag is, the message type is more reliable - this also set for message which don't have a receiver list
	if (cmd == NetMsgJoinReq)
		check |= plNetTimeoutOk;
	
	//now catch undocumented protocol flags
	if (flags & ~(check))
		throw txProtocolError(_WHERE("%s Problem parsing a plNetMsg header format mask %08X\n",u->str().c_str(),flags & ~(check)));
}
void tmNetMsg::stream(tBBuf &t) const {
	if (!u)  throw txProtocolError(_WHERE("attempt to send message without session being set"));
	if (!cmd) throw txProtocolError(_WHERE("attempt to send message without cmd"));
	assert(!u->isUruClient() || !hasFlags(plNetSid));
	
	// fix opcode for UU clients
	if (u->gameType == tNetSession::UUGame)
		t.put16(alcOpcodePOTS2UU(cmd));
	else
		t.put16(cmd);
	
	t.put32(flags);
	if(flags & plNetVersion) {
		t.put8(max_version);
		t.put8(min_version);
	}
	if(flags & plNetTimestamp || (u->min_version<6 && u->max_version==12)) {
		if(timestamp.isNull()) {
			tTime stamp = tTime::now();
			t.put(stamp);
		}
		else
			t.put(timestamp);
	}
	if(flags & plNetX) {
		t.put32(x);
	}
	if(flags & plNetKi) {
		t.put32(ki);
	}
	if(flags & plNetUID) {
		t.write(uid,16);
	}
	if(flags & plNetSid) {
		t.put32(sid);
	}
}
void tmNetMsg::copyProps(tmNetMsg &t) {
	if(flags & plNetVersion) {
		max_version=t.max_version;
		min_version=t.min_version;
	}
	if(flags & plNetTimestamp) {
		timestamp=t.timestamp;
	}
	if(flags & plNetX) {
		x=t.x;
	}
	if(flags & plNetKi) {
		ki=t.ki;
	}
	if(flags & plNetUID) {
		memcpy(uid,t.uid,16);
	}
	if(flags & plNetSid) {
		sid=t.sid;
	}
}
tString tmNetMsg::str() const {
	tString dbg = alcUnetGetMsgCode(cmd);
#ifdef ENABLE_MSGLOG
	dbg.printf(" %04X %08X\n Flags:",cmd,flags);
	if(flags & plNetTimestamp) {
		dbg.writeStr(" timestamp ");
		if (timestamp.isNull()) // the timestamp will be set on sending, so we can't print it now
			dbg.writeStr("(now)");
		else
			dbg.writeStr(timestamp.str());
		dbg.writeStr(",");
	}
	if (flags & plNetMsgRecvrs)
		dbg.writeStr(" game msg receivers,");
	if (flags & plNetTimeoutOk)
		dbg.writeStr(" accept timeout,");
	if(flags & plNetX)
		dbg.printf(" x: %i,",x);
	if(flags & plNetNewSDL)
		dbg.writeStr(" new SDL,");
	if(flags & plNetStateReq1)
		dbg.writeStr(" initial state req,");
	if(flags & plNetKi)
		dbg.printf(" ki: %i,",ki);
	if (flags & plNetRelRegions)
		dbg.writeStr(" use relevance regions,");
	if(flags & plNetUID)
		dbg.printf(" uid: %s,",alcGetStrUid(uid).c_str());
	if (flags & plNetDirected)
		dbg.writeStr(" directed,");
	if(flags & plNetVersion)
		dbg.printf(" version (%i.%i),",max_version,min_version);
	if(flags & plNetSystem)
		dbg.writeStr(" system,");
	if(flags & plNetAck)
		dbg.writeStr(" ack,");
	if(flags & plNetSid)
		dbg.printf(" sid: %i,",sid);
	dbg.cutEnd(dbg.size()-1); // get rid of the last comma
	return additionalFields(dbg);
#else
	return dbg;
#endif
}

uint8_t alcUnetGetVarTypeFromName(tString type) {
	if (type == "INT") return DInteger;
	else if (type == "FLOAT") return DFloat;
	else if (type == "BOOL") return DBool;
	else if (type == "STRING32") return DUruString;
	else if (type == "PLKEY") return DPlKey;
	else if (type == "CREATABLE") return DCreatable;
	else if (type == "TIME") return DTime;
	else if (type == "BYTE" || type == "Byte") return DByte; // don't fail on Prad SDL version 9 which uses "Byte" instead of "BYTE"
	else if (type == "SHORT") return DShort;
	else if (type == "AGETIMEOFDAY") return DAgeTimeOfDay;
	else if (type == "VECTOR3") return DVector3;
	else if (type == "POINT3") return DPoint3;
	else if (type == "QUATERNION") return DQuaternion;
	else if (type == "RGB8") return DRGB8;
	else throw txUnexpectedData(_WHERE("Unknown SDL VAR type %s", type.c_str()));
}

const char * alcUnetGetVarType(uint8_t type) {
	switch(type) {
		case DInteger: return "INT";
		case DFloat: return "FLOAT";
		case DBool: return "BOOL";
		case DUruString: return "STRING32";
		case DPlKey: return "PLKEY";
		case DCreatable: return "CREATABLE";
		case DTime: return "TIME";
		case DByte: return "BYTE";
		case DStruct: return "SDL STRUCT";
		case DShort: return "SHORT";
		case DAgeTimeOfDay: return "AGETIMEOFDAY";
		case DVector3: return "VECTOR3";
		case DPoint3: return "POINT3";
		case DQuaternion: return "QUATERNION";
		case DRGB8: return "RGB8";
		default: return "Unknown";
	}
}

const char * alcUnetGetRelease(uint8_t rel) {
	switch(rel) {
		case TExtRel: return "ExtRel";
		case TIntRel: return "IntRel";
		case TDbg: return "Dbg";
		default: return "Unknown";
	}
}

const char * alcUnetGetDestination(uint8_t dest) {
	switch(dest) {
		case KAgent: return "KAgent";
		case KLobby: return "KLobby";
		case KGame: return "KGame";
		case KVault: return "KVault";
		case KAuth: return "KAuth";
		case KAdmin: return "KAdmin";
		case KLookup: return "KLookup"; // same as KTracking
		case KClient: return "KClient";
		case KMeta: return "KMeta";
		case KTest: return "KTest";
		case KData: return "KData";
		case KProxy: return "KProxy";
		case KPlFire: return "KPlFire";
		case KBcast: return "KBcast";
		default: return "Unknown";
	}
}

const char * alcUnetGetReasonCode(uint8_t code) {
	switch(code) {
		case RStopResponding: return "StopResponding";
		case RActive: return "Active";
		case RInRoute: return "InRoute";
		case RArriving: return "Arriving";
		case RJoining: return "Joining";
		case RLeaving: return "Leaving";
		case RQuitting: return "Quitting";
		case RUnknown: return "UnknownReason";
		case RKickedOff: return "KickedOff";
		case RTimedOut: return "TimedOut";
		case RLoggedInElsewhere: return "LoggedInElsewhere";
		case RNotAuthenticated: return "NotAuthenticated";
		case RUnprotectedCCR: return "UnprotectedCCR";
		case RIllegalCCRClient: return "IllegalCCRClient";
		case RHackAttempt: return "HackAttempt";
		case RUnimplemented: return "Unimplemented";
		case RParseError: return "ParseError";
		default: return "Unknown";
	}
}

const char * alcUnetGetAuthCode(uint8_t code) {
	switch(code) {
		case AAuthSucceeded: return "AuthSucceeded";
		case AAuthHello: return "AuthHello";
		case AProtocolOlder: return "ProtocolOlder";
		case AProtocolNewer: return "ProtocolNewer";
		case AAccountExpired: return "AccountExpired";
		case AAccountDisabled: return "AccountDisabled";
		case AInvalidPasswd: return "InvalidPasswd";
		case AInvalidUser: return "InvalidUser";
		case AUnspecifiedServerError: return "UnspecifiedServerError";
		default: return "Unknown";
	}
}

const char * alcUnetGetAvatarCode(uint8_t code) {
	switch(code) {
		case AOK: return "Ok";
		case AUnknown: return "UnknownReason";
		case ANameDoesNotHaveEnoughLetters: return "NameDoesNotHaveEnoughLetters";
		case ANameIsTooShort: return "NameIsTooShort";
		case ANameIsTooLong: return "NameIsTooLong";
		case AInvitationNotFound: return "InvitationNotFound";
		case ANameIsAlreadyInUse: return "NameIsAlreadyInUse";
		case ANameIsNotAllowed: return "NameIsNotAllowed";
		case AMaxNumberPerAccountReached: return "MaxNumberPerAccountReached";
		case AUnspecifiedServerError: return "UnspecifiedServerError";
		default: return "Unknown";
	}
}

const char * alcUnetGetMsgCode(uint16_t code) {
	switch(code) {
		case NetMsgPagingRoom: return "NetMsgPagingRoom";
		case NetMsgJoinReq: return "NetMsgJoinReq";
		case NetMsgJoinAck: return "NetMsgJoinAck";
		case NetMsgLeave: return "NetMsgLeave";
		case NetMsgPing: return "NetMsgPing";
		case NetMsgGroupOwner: return "NetMsgGroupOwner";
		case NetMsgGameStateRequest: return "NetMsgGameStateRequest";
		case NetMsgGameMessage: return "NetMsgGameMessage";
		case NetMsgVoice: return "NetMsgVoice";
		case NetMsgTestAndSet: return "NetMsgTestAndSet";
		case NetMsgMembersListReq: return "NetMsgMembersListReq";
		case NetMsgMembersList: return "NetMsgMembersList";
		case NetMsgMemberUpdate: return "NetMsgMemberUpdate";
		case NetMsgCreatePlayer: return "NetMsgCreatePlayer";
		case NetMsgAuthenticateHello: return "NetMsgAuthenticateHello";
		case NetMsgAuthenticateChallenge: return "NetMsgAuthenticateChallenge";
		case NetMsgInitialAgeStateSent: return "NetMsgInitialAgeStateSent";
		case NetMsgVaultTask: return "NetMsgVaultTask";
		case NetMsgAlive: return "NetMsgAlive";
		case NetMsgTerminated: return "NetMsgTerminated";
		case NetMsgSDLState: return "NetMsgSDLState";
		case NetMsgSDLStateBCast: return "NetMsgSDLStateBCast";
		case NetMsgGameMessageDirected: return "NetMsgGameMessageDirected";
		case NetMsgRequestMyVaultPlayerList: return "NetMsgRequestMyVaultPlayerList";
		case NetMsgVaultPlayerList: return "NetMsgVaultPlayerList";
		case NetMsgSetMyActivePlayer: return "NetMsgSetMyActivePlayer";
		case NetMsgPlayerCreated: return "NetMsgPlayerCreated";
		case NetMsgFindAge: return "NetMsgFindAge";
		case NetMsgFindAgeReply: return "NetMsgFindAgeReply";
		case NetMsgDeletePlayer: return "NetMsgDeletePlayer";
		case NetMsgAuthenticateResponse: return "NetMsgAuthenticateResponse";
		case NetMsgAccountAuthenticated: return "NetMsgAccountAuthenticated";
		case NetMsgRelevanceRegions: return "NetMsgRelevanceRegions";
		case NetMsgLoadClone: return "NetMsgLoadClone";
		case NetMsgPlayerPage: return "NetMsgPlayerPage";
		case NetMsgVault: return "NetMsgVault";
		case NetMsgPython: return "plNetMsgPython";
		case NetMsgSetTimeout: return "NetMsgSetTimeout";
		case NetMsgActivePlayerSet: return "NetMsgActivePlayerSet";
		case NetMsgGetPublicAgeList: return "NetMsgGetPublicAgeList";
		case NetMsgPublicAgeList: return "NetMsgPublicAgeList";
		case NetMsgCreatePublicAge: return "NetMsgCreatePublicAge";
		case NetMsgPublicAgeCreated: return "NetMsgPublicAgeCreated";
		case NetMsgRemovePublicAge: return "NetMsgRemovePublicAge";
		case NetMsgPublicAgeRemoved: return "NetMsgPublicAgeRemoved";
		// Custom messages
		case NetMsgCustomAuthAsk: return "NetMsgCustomAuthAsk";
		case NetMsgCustomAuthResponse: return "NetMsgCustomAuthResponse";
		case NetMsgCustomVaultAskPlayerList: return "NetMsgCustomVaultAskPlayerList";
		case NetMsgCustomVaultPlayerList: return "NetMsgCustomVaultPlayerList";
		case NetMsgCustomVaultCreatePlayer: return "NetMsgCustomVaultCreatePlayer";
		case NetMsgCustomVaultPlayerCreated: return "NetMsgCustomVaultPlayerCreated";
		case NetMsgCustomVaultDeletePlayer: return "NetMsgCustomVaultDeletePlayer";
		case NetMsgCustomPlayerStatus: return "NetMsgCustomPlayerStatus";
		case NetMsgCustomVaultCheckKi: return "NetMsgCustomVaultCheckKi";
		case NetMsgCustomVaultKiChecked: return "NetMsgCustomVaultKiChecked";
		case NetMsgCustomRequestAllPlStatus: return "NetMsgCustomRequestAllPlStatus";
		case NetMsgCustomAllPlayerStatus: return "NetMsgCustomAllPlayerStatus";
		case NetMsgCustomSetGuid: return "NetMsgCustomSetGuid";
		case NetMsgCustomFindServer: return "NetMsgCustomFindServer";
		case NetMsgCustomServerFound: return "NetMsgCustomServerFound";
		case NetMsgCustomForkServer: return "NetMsgCustomForkServer";
		case NetMsgPlayerTerminated: return "NetMsgPlayerTerminated";
		case NetMsgCustomVaultPlayerStatus: return "NetMsgCustomVaultPlayerStatus";
		case NetMsgCustomMetaRegister: return "NetMsgCustomMetaRegister";
		case NetMsgCustomMetaPing: return "NetMsgCustomMetaPing";
		case NetMsgCustomServerVault: return "NetMsgCustomServerVault";
		case NetMsgCustomServerVaultTask: return "NetMsgCustomServerVaultTask";
		case NetMsgCustomSaveGame: return "NetMsgCustomSaveGame";
		case NetMsgCustomLoadGame: return "NetMsgCustomLoadGame";
		case NetMsgCustomCmd: return "NetMsgCustomCmd";
		case NetMsgCustomDirectedFwd: return "NetMsgCustomDirectedFwd";
		case NetMsgCustomPlayerToCome: return "NetMsgCustomPlayerToCome";
		case NetMsgCustomVaultFindAge: return "NetMsgCustomVaultFindAge";
		case NetMsgCustomTest: return "NetMsgCustomTest";
		default: return "Unknown";
	}
}

} //namespace
