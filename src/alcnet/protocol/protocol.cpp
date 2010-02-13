/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs H'uru Server Team                     *
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

/* The Uru protocol, is here */

/*

Uru Protocol
No sockets please

 --*/

/* CVS tag - DON'T TOUCH*/
#define __U_PROTOCOL_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcnet.h"

namespace md5 {
#include "alcutil/md5.h"
}

#include <alcdebug.h>

namespace alc {

/**
 \brief Encodes the specific packet into Uru validation level 2
   k = offset MOD 8
   enc: c = x * 2 ^ k MOD 255
*/
void alcEncodePacket(Byte* buf2,const Byte* buf, int n) {
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
void alcDecodePacket(Byte* buf, int n) {
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
U32 alcUruChecksum(const Byte* buf, int size, int alg, const char * aux_hash) {
	int i;
	U32 aux=0; //little-endian order when returned
	//S32 saux=0;
	Byte * md5buffer;
	Byte hash[16];
	int aux_size; //auxiliar size
	int whoi=0;

	DBG(4,"Checksum %i requested, packet size is %i...\n",alg,size);
	switch(alg) {
		case 0:
			for(i=6; i<(size-4); i=i+4) {
				U32 val;
#if defined(NEED_STRICT_ALIGNMENT)
				memcpy(&val, buf+i, 4);
#else
				val = *reinterpret_cast<U32 *>(buf+i);
#endif
				aux = aux + letoh32(val); //V1 - working Checksum algorithm
			}
			whoi=((size-6)%4);
			if(whoi==1 || whoi==2) {
				aux += *(buf+size-1);
			} else if(whoi==0) {
				U32 val;
#if defined(NEED_STRICT_ALIGNMENT)
				memcpy(&val, buf+size-4, 4);
#else
				val = *reinterpret_cast<U32 *>(buf+size-4);
#endif
				aux += letoh32(val);
			} //else if whoi==3 Noop
			aux = htole32(aux);
			break;
		case 1:
		case 2:
			//code for the V2 - Checksum algorithm
			aux_size=size-6;
			if(alg==2) { aux_size+=32; }
			//allocate the space for the buffer
			DBG(4,"Allocating md5buffer - %i bytes...\n",aux_size);
			md5buffer = static_cast<Byte *>(malloc(sizeof(Byte)*(aux_size+10)));
			if (md5buffer == NULL) throw txNoMem(_WHERE("NoMem"));
			for(i=6; i<size; i++) {
				md5buffer[i-6]=buf[i];
			}
			//Concatenate the ASCII passwd md5 hash as required by V2 algorithm
			if(alg==2) {
				for(i=size; i<(aux_size+6); i++) {
					md5buffer[i-6]=aux_hash[i-size];
				}
				//print2log(f_chkal,"passwd_hash: %s\n",aux_hash);
			}
			//print2log(f_chkal,"to be md5sumed:\n");
			//dump_packet(f_chkal,md5buffer,aux_size,0,5);
			/*if(alg==2) {
				dumpbuf(f_uru,md5buffer,aux_size);
				abort();
			}*/
			md5::MD5(md5buffer, aux_size, hash);
			//print2log(f_chkal,"\n<-\n");
			aux = *reinterpret_cast<U32 *>(hash);
			free(md5buffer);
			break;
		default:
			alcGetMain()->err()->log("ERR: Uru Checksum V%i is currently not supported in this version of the server.\n\n",alg);
			aux = 0xFFFFFFFF;
	}
	return aux;
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
int alcUruValidatePacket(Byte * buf,int n,Byte * validation,bool authed,const char * phash) {
	U32 checksum;
#ifndef _NO_CHECKSUM
	U32 aux_checksum;
#endif
	if(n>=6 && buf[0]==0x03) { //magic uru number
		if(buf[1]!=0) {
			*validation=buf[1]; //store the validation level
		}
		if(buf[1]==0x00) { return 0; } //All went OK with validation level 0
#if defined(NEED_STRICT_ALIGNMENT)
		memcpy(&checksum,buf+0x02,4); //store the checksum
#else
		checksum=*reinterpret_cast<U32 *>(buf+0x02); //store the checksum
#endif
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

U16 alcFixUUNetMsgCommand(U16 cmd, const tNetSession *u)
{
	// we might have to fix the message type
	if (u->tpots == 2
			&& (cmd == NetMsgVault_UU || cmd == NetMsgPython_UU || cmd == NetMsgSetTimeout_UU || cmd == NetMsgActivePlayerSet_UU))
		return cmd+1; // these values are incremented by 1 in TPOTS (remember to aslso update tmMsgBase::stream!)
	return cmd;
}

//Unet Uru Message
void tUnetUruMsg::store(tBBuf &t) {
	U32 hsize=28;
	t.seek(1);
	val=t.getByte();
	if(val>0) {
		t.seek(4);
		hsize+=4; //32
	}
	pn=t.getU32();
	tf=t.getByte();
	// Catch unknown or unhandled flags
	Byte check = UNetNegotiation | UNetAckReq | UNetAckReply | UNetExt;
	if (tf & ~(check)) throw txUnexpectedData(_WHERE("Problem parsing uru msg flag %08X\n",tf & ~(check)));
	// Catch invalid Flag combinations
	if((tf & UNetNegotiation) && (tf & UNetAckReply)) throw txUnexpectedData(_WHERE("That must be a maliciusly crafted paquet, flags UNetAckReply and UNetNegotiation cannot be set at the same time"));
	if((tf & UNetAckReq) && (tf & UNetAckReply)) throw txUnexpectedData(_WHERE("That must be a maliciusly crafted paquet, flags UNetAckReply and UNetAckReq cannot be set at the same time"));
	if(tf & UNetExt) {
		hsize-=8; //20 - 24
		csn=t.getU32();
		frt=t.getByte();
	} else {
		if(t.getU32()!=0) throw txUnexpectedData(_WHERE("Non-zero unk1"));
		csn=t.getU32();
		frt=t.getByte();
		if(t.getU32()!=0) throw txUnexpectedData(_WHERE("Non-zero unk2"));
	}
	if (frt > 0 && ((tf & UNetNegotiation) || (tf & UNetAckReply)))
		throw txProtocolError(_WHERE("Nego and ack packets must not be fragmented!"));
	if (frt > 0 && !(tf & UNetAckReq))
		throw txProtocolError(_WHERE("Non-acked packets must not be fragmented!"));
	cps=t.getU32();
	dsize=t.getU32();
	if(dsize==0) throw txUnexpectedData(_WHERE("A zero sized message!?"));
	U32 xdsize=dsize;
	//check size
	if(tf & UNetAckReply) {
		if(tf & UNetExt) {
			xdsize=dsize*8;
			if(t.size()-hsize!=xdsize) throw txUnexpectedData(_WHERE("Alcugs Protocol Ack reply incorrect size!\n"));
		} else {
			xdsize=(dsize*16)+2;
			if(t.size()-hsize!=xdsize) throw txUnexpectedData(_WHERE("Uru Protocol Ack reply incorrect size!\n"));
		}
	} else {
		if(t.size()-hsize!=dsize) throw txUnexpectedData(_WHERE("Message size check failed %i!=%i\n",dsize,t.size()-hsize));
	}
	data.clear();
	data.write(t.read(),xdsize);
	frn=csn & 0x000000FF;
	sn=csn >> 8;
	pfr=cps & 0x000000FF;
	ps=cps >> 8;
	if (frn > frt) throw txProtocolError(_WHERE("A message must not have more fragments than it says it would have"));
}
void tUnetUruMsg::stream(tBBuf &t) const {
	DBG(5,"[%i] ->%02X<- {%i,%i (%i) %i,%i} - %02X|%i bytes\n",pn,tf,sn,frn,frt,ps,pfr,dsize,dsize);
	t.putByte(0x03); //already done by the sender ()
	t.putByte(val);
	if(val>0) t.putU32(0xFFFFFFFF);
	t.putU32(pn); //generic pkg counter (re-written by the sender() )
	//next part is not touched by the sender
	t.putByte(tf);
	if(tf & UNetExt) {
		t.putU32(csn);
		t.putByte(frt);
	} else {
		t.putU32(0); //4 blanks
		t.putU32(csn);
		t.putByte(frt);
		t.putU32(0);
	}
	t.putU32(cps);
	t.putU32(dsize);
	t.put(data);
}
U32 tUnetUruMsg::size() {
	return data.size() + hSize();
}
U32 tUnetUruMsg::hSize() {
	U32 hsize=28;
	if(val>0) hsize+=4;
	if(tf & UNetExt) hsize-=8;
	return hsize;
}
void tUnetUruMsg::_update() {
	// update dsize
	if(tf & UNetAckReply) {
		if(tf & UNetExt)
			dsize=data.size()/8;
		else
			dsize=(data.size()-2)/16;
	} else {
		dsize=data.size();
	}
	// update combined message counters
	csn=frn | sn<<8;
	cps=pfr | ps<<8;
}
void tUnetUruMsg::dumpheader(tLog * f) {
	f->print("[%i] ->%02X<- {%i,%i (%i) %i,%i} - %02X|%i bytes",pn,tf,sn,frn,frt,ps,pfr,data.size(),data.size());
}
//flux 0 client -> server, 1 server -> client
void tUnetUruMsg::htmlDumpHeader(tLog * log,Byte flux,U32 ip,U16 port) {
	if (!log->doesPrint()) return;
	log->stamp();

	switch(tf) {
		case UNetAckReply: //0x80
		case UNetAckReply | UNetExt:
			log->print("<font color=red>");
			break;
		case UNetNegotiation | UNetAckReq: //0x42
		case UNetNegotiation | UNetAckReq | UNetExt:
			log->print("<font color=green>");
			break;
		case 0x00: //0x00
		case UNetExt:
			log->print("<font color=blue>");
			break;
		case UNetAckReq: //0x02
		case UNetAckReq | UNetExt:
			log->print("<font color=black>");
			break;
		default:
			log->print("<font color=pink>");
			break;
	}

	if(flux==0) {
		log->print("<b> me &lt;- ");
	} else {
		log->print(" me -&gt; ");
	}
	log->print("%i %i,%i (%i) %i,%i ",pn,sn,frn,frt,ps,pfr);

	if(flux==0) {
		log->print(" &lt;- %s:%i</b> ",alcGetStrIp(ip).c_str(),ntohs(port));
	} else {
		log->print(" -&gt; %s:%i ",alcGetStrIp(ip).c_str(),ntohs(port));
	}

	U32 i;
	data.rewind();

	switch(tf) {
		case UNetAckReply: //0x80
			log->print("ack");
			data.seek(2);
			for(i=0; i<dsize; i++) {
				if(i!=0) { log->print(" |"); }
				Byte i1=data.getByte();
				U32 i2=data.getU32();
				data.seek(3);
				Byte i3=data.getByte();
				data.seek(-1);
				U32 i4=data.getU32();
				log->print(" %i,%i %i,%i",i2,i1,i4>>8,i3);
				data.seek(3);
			}
			break;
		case UNetAckReply | UNetExt:
			log->print("aack");
			for(i=0; i<dsize; i++) {
				if(i!=0) { log->print(" |"); }
				Byte i1=data.getByte();
				data.seek(-1);
				U32 i2=data.getU32();
				Byte i3=data.getByte();
				data.seek(-1);
				U32 i4=data.getU32();
				log->print(" %i,%i %i,%i",i2>>8,i1,i4>>8,i3);
			}
			break;
		case UNetNegotiation | UNetAckReq: //0x42
		case UNetNegotiation | UNetAckReq | UNetExt:
			log->print("Negotiation ");
			//char * times;
			//times=ctime((const time_t *)(buf+4));
			//log->print("%i bps, %s",*(U32 *)(buf),alcGetStrTime(*(U32 *)(buf+4),*(U32 *)(buf+8)));
			log->print("%i bps, %s",data.getU32(),alcGetStrTime(data.getU32()).c_str(),data.getU32());
			break;
		case 0x00: //0x00
		case UNetExt:
			log->print("plNetMsg0 ");
			break;
		case UNetAckReq: //0x02
		case UNetAckReq | UNetExt:
			log->print("plNetMsg1 ");
			break;
		default:
			log->print("ERROR! ");
			break;
	}

	log->print("</font>");

	if((tf==0x00 || tf==UNetAckReq || tf==UNetExt || tf==(UNetAckReq | UNetExt))) {
		if(frn==0) {
			U32 msgcode=data.getU16();
			//log->print("(%04X) %s %08X",*(U16 *)(buf),alcUnetGetMsgCode(*(U16 *)(buf)),*(U32 *)(buf+2));
			log->print("(%04X) %s %08X",msgcode,alcUnetGetMsgCode(msgcode),data.getU32());
		} else {
			log->print("frg..");
		}
	}
	log->print("<br>\n");
	log->flush();
}

//Negotiation
void tmNetClientComm::store(tBBuf &t) {
	bandwidth=t.getU32();
	t.get(timestamp);
	if (!t.eof()) throw txUnexpectedData(_WHERE("Nego is too long"));
}
void tmNetClientComm::stream(tBBuf &t) const {
	t.putU32(bandwidth);
	t.put(timestamp);
}
tString tmNetClientComm::str() const {
	tString str;
#ifdef ENABLE_MSGLOG
	str.printf("(Re)Negotation (bandwidth: %i bps time: %s) on %s",bandwidth,timestamp.str().c_str(),u->str().c_str());
#else
	str.printf("(Re)Negotation on %s",u->str().c_str());
#endif
	return str;
}

// Ack
tmNetAck::~tmNetAck()
{
	tAckList::iterator it = ackq.begin();
	while (it != ackq.end()) {
		delete *it;
		it = ackq.erase(it);
	}
}
void tmNetAck::store(tBBuf &t)
{
	ackq.clear();
	if (!(bhflags & UNetExt)) {
		if(t.getU16() != 0) throw txUnexpectedData(_WHERE("ack unknown data"));
	}
	while (!t.eof()) {
		tUnetAck *ack = new tUnetAck;
		ack->A=t.getU32();
		if(!(bhflags & UNetExt))
			if(t.getU32()!=0) throw txUnexpectedData(_WHERE("ack unknown data"));
		ack->B=t.getU32();
		if(!(bhflags & UNetExt))
			if(t.getU32()!=0) throw txUnexpectedData(_WHERE("ack unknown data"));
		ackq.push_back(ack);
	}
	// no need to check for eof, the loop already does that
}
void tmNetAck::stream(tBBuf &t) const
{
	if (!(bhflags & UNetExt)) t.putU16(0);
	for (tAckList::const_iterator it = ackq.begin(); it != ackq.end(); ++it) {
		t.putU32((*it)->A);
		if(!(bhflags & UNetExt))
			t.putU32(0);
		t.putU32((*it)->B);
		if(!(bhflags & UNetExt))
			t.putU32(0);
	}
}
tString tmNetAck::str() const {
	tString dbg;
	dbg.printf("Ack on %s", u->str().c_str());
	#ifdef ENABLE_MSGLOG
	bool firstOne = true;
	for (tAckList::const_iterator it = ackq.begin(); it != ackq.end(); ++it) {
		if (firstOne)
			firstOne = false;
		else
			dbg.printf(" |");
		dbg.printf(" %i,%i %i,%i", (*it)->A >> 8, (*it)->A & 0x000000FF, (*it)->B >> 8, (*it)->B & 0x000000FF);
	}
	#endif
	return dbg;
}

//Base message
tmMsgBase::tmMsgBase(U16 cmd,U32 flags,tNetSession * u) : tmBase(0, u) {
	DBG(5,"tmMsgBase()\n");
	this->cmd=cmd;
	this->flags=flags;
	this->timestamp.seconds=0; // the timestamp is unitialized per default (this removes a valgrind error)
	//set bhflags
	if(this->flags & plNetAck)
		bhflags |= UNetAckReq;
	// get version froms session
	if (this->flags & plNetVersion) {
		max_version = u->max_version;
		min_version = u->min_version;
	}
}
tmMsgBase::tmMsgBase(tNetSession * u) : tmBase(0, u) {
	DBG(5,"tmMsgBase()\n");
	this->cmd=0;
	this->flags=0;
	this->timestamp.seconds=0; // the timestamp is unitialized per default (this removes a valgrind error)
}
void tmMsgBase::setFlags(U32 f) {
	this->flags |= f;
	if(f & plNetAck)
		bhflags |= UNetAckReq;
}
void tmMsgBase::unsetFlags(U32 f) {
	this->flags &= ~f;
	if(f & plNetAck)
		bhflags &= ~UNetAckReq;
}
U32 tmMsgBase::getFlags() const {
	return flags;
}
bool tmMsgBase::hasFlags(U32 f) const {
	return (flags | f) == flags; // there can be several flags enabled in f, so a simple & is not enough
}
void tmMsgBase::setUrgent() {
	bhflags |= UNetUrgent;
}
void tmMsgBase::unsetUrgent() {
	bhflags &= ~UNetUrgent;
}
void tmMsgBase::store(tBBuf &t) {
	if (!u)  throw txProtocolError(_WHERE("attempt to save message without session being set"));
	//base
	cmd=alcFixUUNetMsgCommand(t.getU16(), u);
	flags=t.getU32();
	if(flags & plNetVersion) {
		max_version=t.getByte();
		min_version=t.getByte();
		// this overrides existing values
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
		timestamp.seconds=0;
		timestamp.microseconds=0;
	}
	if(flags & plNetX) {
		x=t.getU32();
	} else {
		x=0;
	}
	if(flags & plNetKi) {
		ki=t.getU32();
	} else {
		ki=0;
	}
	if(flags & plNetUID) {
		memcpy(uid,t.read(16),16);
	} else {
		memset(uid,0,16);
	}
	if(flags & plNetSid) {
		sid=t.getU32();
	}
	else sid = 0;

	U32 check = plNetAck | plNetVersion | plNetTimestamp | plNetX | plNetKi | plNetUID | plNetSystem /*unknown purpose*/;
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
	// accept custom types from Alcugs servers only
	if (u->isAlcugsServer())
		check |= plNetSid;
	
	//now catch undocumented protocol flags
	if (flags & ~(check))
		throw txProtocolError(_WHERE("%s Problem parsing a plNetMsg header format mask %08X\n",u->str().c_str(),flags & ~(check)));
}
void tmMsgBase::stream(tBBuf &t) const {
	if (!u)  throw txProtocolError(_WHERE("attempt to send message without session being set"));
	if (!cmd) throw txProtocolError(_WHERE("attempt to send message without cmd"));
	if (!u->isAlcugsServer() && (flags & plNetSid)) // check for internal flags
		throw txProtocolError(_WHERE("Custom flags must only be sent to Alcugs servers"));
	
	// fix for UU clients
	if (u->tpots == 2 && (cmd == NetMsgVault || cmd == NetMsgPython || cmd == NetMsgSetTimeout || cmd == NetMsgActivePlayerSet))
		t.putU16(cmd-1); // these are incremented by 1 in POTS (remember to also update alcFixUUNetMsgCommand!)
	else
		t.putU16(cmd);
	
	t.putU32(flags);
	if(flags & plNetVersion) {
		t.putByte(max_version);
		t.putByte(min_version);
	}
	if(flags & plNetTimestamp || (u->min_version<6 && u->max_version==12)) {
		if(timestamp.seconds==0) {
			tTime stamp;
			stamp.setToNow();
			t.put(stamp);
		}
		else
			t.put(timestamp);
	}
	if(flags & plNetX) {
		t.putU32(x);
	}
	if(flags & plNetKi) {
		t.putU32(ki);
	}
	if(flags & plNetUID) {
		t.write(uid,16);
	}
	if(flags & plNetSid) {
		t.putU32(sid);
	}
}
void tmMsgBase::copyProps(tmMsgBase &t) {
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
tString tmMsgBase::str() const {
	tString dbg;
	dbg.printf("%s",alcUnetGetMsgCode(cmd));
#ifdef ENABLE_MSGLOG
	dbg.printf(" %04X %08X",cmd,flags);
#endif
	dbg.printf(" on %s", u->str().c_str());
#ifdef ENABLE_MSGLOG
	dbg.printf("\n Flags:");
	if(flags & plNetTimestamp) {
		dbg.writeStr(" timestamp ");
		if (timestamp.seconds == 0) // the timestamp will be set on sending, so we can't print it now
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

	dbg.seek(-1); // remove the last comma
	dbg.putByte(0); // this is necessary because of the seek() call
#endif
	return dbg;
}

Byte alcUnetGetVarTypeFromName(tString type) {
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

const char * alcUnetGetVarType(Byte type) {
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

const char * alcUnetGetRelease(Byte rel) {
	switch(rel) {
		case TExtRel: return "ExtRel";
		case TIntRel: return "IntRel";
		case TDbg: return "Dbg";
		default: return "Unknown";
	}
}

const char * alcUnetGetDestination(Byte dest) {
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

const char * alcUnetGetReasonCode(Byte code) {
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

const char * alcUnetGetAuthCode(Byte code) {
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

const char * alcUnetGetAvatarCode(Byte code) {
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

const char * alcUnetGetLinkingRule(Byte rule)
{
	switch (rule) {
		case KBasicLink: return "KBasicLink";
		case KOriginalBook: return "KOriginalBook";
		case KSubAgeBook: return "KSubAgeBook";
		case KOwnedBook: return "KOwnedBook";
		case KVisitBook: return "KVisitBook";
		case KChildAgeBook: return "KChildAgeBook";
		default: return "Unknown";
	}
}

const char * alcUnetGetMsgCode(U16 code) {
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
