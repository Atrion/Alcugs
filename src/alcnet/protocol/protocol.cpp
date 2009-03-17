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

#include "alcugs.h"
#include "alcnet.h"

namespace md5 {
#include "alcutil/md5.h"
}

#include "alcdebug.h"

namespace alc {

/**
 \brief Encodes the specific packet into Uru validation level 2
   k = offset MOD 8
   enc: c = x * 2 ^ k MOD 255
*/
void alcEncodePacket(unsigned char* buf2,unsigned char* buf, int n) {
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
void alcDecodePacket(unsigned char* buf, int n) {
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
  \param alg
	 0 -> prime sum,
	 1 -> live md5 sum,
	 2 -> live md5 + passwd sum,
  \note REMEMBER THAT aux_hash must be 32 bytes and contain an ascii hash
*/
U32 alcUruChecksum(Byte* buf, int size, int alg, char * aux_hash) {
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
				memcpy((void *)&val, buf+i, 4);
#else
				val = *((U32 *)(buf+i));
#endif
				aux = aux + letoh32(val); //V1 - working Checksum algorithm
			}
			whoi=((size-6)%4);
			if(whoi==1 || whoi==2) {
				aux += *((Byte *)(buf+size-1));
			} else if(whoi==0) {
				U32 val;
#if defined(NEED_STRICT_ALIGNMENT)
				memcpy((void *)&val, buf+size-4, 4);
#else
				val = *((U32 *)(buf+size-4));
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
			md5buffer = (Byte *)malloc(sizeof(Byte)*(aux_size+10));
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
			aux = *((U32 *)hash);
			free(md5buffer);
			break;
		default:
			lerr->log("ERR: Uru Checksum V%i is currently not supported in this version of the server.\n\n",alg);
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
int alcUruValidatePacket(Byte * buf,int n,Byte * validation,bool authed,char * phash) {
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
		memcpy((void*)&checksum,buf+0x02,4); //store the checksum
#else
		checksum=*(U32 *)(buf+0x02); //store the checksum
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
	if(tf & UNetExp) throw txUnexpectedData(_WHERE("Expansion flag not supported on this server version"));
	if((tf & UNetNegotiation) && (tf & UNetAckReply)) throw txUnexpectedData(_WHERE("That must be a maliciusly crafted paquet, flags UNetAckReply and UNetNegotiation cannot be set at the same time"));
	if((tf & UNetAckReq) && (tf & UNetAckReply)) throw txUnexpectedData(_WHERE("That must be a maliciusly crafted paquet, flags UNetAckReply and UNetAckReq cannot be set at the same time"));
	if(tf & (UNetForce0 | UNetUrgent)) throw txUnexpectedData(_WHERE("Illegal flags %02X",tf));
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
	if (frt > 0 && ((tf & UNetNegotiation) || (tf & UNetAckReply))) throw txProtocolError(_WHERE("Nego and ack packets must not be fragmented!"));
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
void tUnetUruMsg::stream(tBBuf &t) {
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
	data.rewind();
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
	static int count=0;
	count++;
	if (!log->doesPrint()) return;

	log->print("%04i: ",count);
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
		log->print(" &lt;- %s:%i</b> ",alcGetStrIp(ip),ntohs(port));
	} else {
		log->print(" -&gt; %s:%i ",alcGetStrIp(ip),ntohs(port));
	}

	int i;
	data.rewind();

	switch(tf) {
		case UNetAckReply: //0x80
			log->print("ack");
			data.seek(2);
			for(i=0; i<(int)dsize; i++) {
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
			for(i=0; i<(int)dsize; i++) {
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
			log->print("%i bps, %s",data.getU32(),alcGetStrTime(data.getU32()),data.getU32());
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
}
void tmNetClientComm::stream(tBBuf &t) {
	t.putU32(bandwidth);
	t.put(timestamp);
}
const char * tmNetClientComm::str() {
	static char cnt[1024];
#ifdef ENABLE_MSGLOG
	sprintf(cnt,"(Re)Negotation bandwidth: %i bps time: %s",bandwidth,timestamp.str());
#else
	sprintf(cnt,"(Re)Negotation");
#endif
	// don't use sprintf(cnt, "%s", cnt), valgrind shows a "Source and destination overlap in mempcpy"
	strcat(cnt, " on ");
	strcat(cnt, u->str());
	return cnt;
}

// Ack
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
		ackq.add(ack);
	}
}
void tmNetAck::stream(tBBuf &t)
{
	if (!(bhflags & UNetExt)) t.putU16(0);
	ackq.rewind();
	tUnetAck *ack;
	while ((ack = ackq.getNext())) {
		t.putU32(ack->A);
		if(!(bhflags & UNetExt))
			t.putU32(0);
		t.putU32(ack->B);
		if(!(bhflags & UNetExt))
			t.putU32(0);
	}
}
const char * tmNetAck::str() {
	dbg.clear();
	dbg.printf("Ack on %s", u->str());
	#ifdef ENABLE_MSGLOG
	tUnetAck *ack;
	bool firstOne = true;
	ackq.rewind();
	while ((ack = ackq.getNext())) {
		if (firstOne)
			firstOne = false;
		else
			dbg.printf(" |");
		dbg.printf(" %i,%i %i,%i", ack->A >> 8, ack->A & 0x000000FF, ack->B >> 8, ack->B & 0x000000FF);
	}
	#endif
	return dbg.c_str();
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
U32 tmMsgBase::getFlags() {
	return flags;
}
bool tmMsgBase::hasFlags(U32 f) {
	return (flags | f) == flags; // there can be several flags enabled in f, so a simple & is not enough
}
void tmMsgBase::setUrgent() {
	bhflags |= UNetUrgent;
}
void tmMsgBase::unsetUrgent() {
	bhflags &= ~UNetUrgent;
}
void tmMsgBase::store(tBBuf &t) {
	//base
	cmd=t.getU16();
	flags=t.getU32();
	if(flags & plNetVersion) {
		max_version=t.getByte();
		min_version=t.getByte();
		if (u) { // this overrides existing values, which might be guessed
			u->max_version=max_version;
			u->min_version=min_version;
		}
	} else {
		max_version=0;
		min_version=0;
	}
	//BEGIN ** guess the protocol version from behaviours
	// The first message from Plasma clients is always an auth hello that contains the version numbers
	if(u && u->max_version==0 && !(flags & plNetVersion)) { // don't auto-guess when we got a version number (it could have been a version 0.0)
		// old versions always include the timestamp, newer ones have a flag for that and sometimes dont contain it
		if(flags & plNetTimestamp || t.remaining() < 8) { // when there are less than 8 bytes remaining, no timestamp can be contained
			u->max_version=12; //sure (normally on ping proves)
			u->min_version=6;
		} else {
			u->max_version=12;
			u->min_version=0;
		}
		DBG(5,"Detected version is %i.%i\n",s->max_version,s->min_version);
	}
	//END ** guess protocol version
	
	//NetMsgPing should have always the timestamp enabled in new versions
	if(flags & plNetTimestamp || (u && (u->min_version<6 && u->max_version==12))) {
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
	if(flags & plNetIP) {
		//Unfortunately it looks like the IP address is transmitted in
		// network byte order. This means that if we just use getU32(),
		// on little-endian systems ip will be in network order and on
		// big-endian systems it will be byte-swapped from *both*
		// network and host order (which are the same).
		ip=letoh32(t.getU32());
		//The port is transmitted in little-endian order, so is in host
		// order after getU16().
		port=htons(t.getU16());
	} else {
		ip=0;
		port=0;
	}
	if(flags & plNetSid) {
		sid=t.getU32();
	}
	else sid = 0;

	U32 check=plNetAck | plNetVersion | plNetTimestamp | \
	plNetX | plNetKi | plNetUID | plNetIP | plNetCustom | plNetSid;
	// accept some flags only for certain messages
	if (cmd == NetMsgGameMessage) check |= plNetUnk1 | plNetUnk2;
	else if (cmd == NetMsgGameMessageDirected) check |= plNetDirected;
	else if (cmd == NetMsgJoinReq) check |= plNetP2P;
	else if (cmd == NetMsgGameStateRequest) check |= plNetStateReq;
	else if (cmd == NetMsgSDLState) check |= plNetBcast;
	else if (cmd == NetMsgSDLStateBCast) check |= plNetBcast | plNetUnk2;
	
	//now catch undocumented protocol flags
	if (flags & ~(check))
		throw txProtocolError(_WHERE("%s Problem parsing a plNetMsg header format mask %08X\n",u->str(),flags & ~(check)));
}
void tmMsgBase::stream(tBBuf &t) {
	if (!cmd) throw txProtocolError(_WHERE("attempt to send message without cmd"));
	if((flags & plNetSid) && u->proto!=0 && u->proto<3)
		throw txProtocolError(_WHERE("attempt to send message with sid flag to old client")); // dont send this flag to old peers
	t.putU16(cmd);
	t.putU32(flags);
	if(flags & plNetVersion) {
		t.putByte(max_version);
		t.putByte(min_version);
	}
	if(flags & plNetTimestamp || (u && (u->min_version<6 && u->max_version==12))) {
		if(timestamp.seconds==0) {
			timestamp.seconds=alcGetTime();
			timestamp.microseconds=alcGetMicroseconds();
		}
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
	if(flags & plNetIP) {
		//We have to swap around again on big-endian systems to get
		// the address back to little-endian order so that it's
		// byte-swapped back to big-endian (network) by putU32().
		t.putU32(htole32(ip));
		//Also switch the port back from network to host order before writing.
		t.putU16(ntohs(port));
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
	if(flags & plNetIP) {
		ip=t.ip;
		port=t.port;
	}
	if(flags & plNetSid) {
		sid=t.sid;
	}
}
const char * tmMsgBase::str() {
	dbg.clear();
	dbg.printf("%s",alcUnetGetMsgCode(cmd));
#ifdef ENABLE_MSGLOG
	dbg.printf(" %04X %08X",cmd,flags);
#endif
	dbg.printf(" on %s", u->str());
#ifdef ENABLE_MSGLOG
	dbg.printf("\n Flags:");
	if(flags & plNetAck)
		dbg.writeStr(" ack,");
	if(flags & plNetFirewalled)
		dbg.writeStr(" firewalled,");
	if(flags & plNetP2P)
		dbg.writeStr(" P2P request,");
	if(flags & plNetBcast)
		dbg.writeStr(" bcast,");
	if(flags & plNetCustom)
		dbg.writeStr(" UCPNPI,");
	if (flags & plNetUnk1)
		dbg.writeStr(" Unk1,");
	if (flags & plNetUnk2)
		dbg.writeStr(" Unk2,");
	if(flags & plNetStateReq)
		dbg.writeStr(" InitialStateReq,");
	if (flags & plNetDirected)
		dbg.writeStr(" Directed,");
	if(flags & plNetVersion)
		dbg.printf(" version (%i.%i),",max_version,min_version);
	if(flags & plNetTimestamp) {
		dbg.writeStr(" timestamp ");
		if (timestamp.seconds == 0) // the timestamp will be set on sending, so we can't print it now
			dbg.writeStr("(now)");
		else
			dbg.writeStr(timestamp.str());
		dbg.writeStr(",");
	}
	//dbg.seek(-1); // remove the last comma
	//dbg.nl();
	if(flags & plNetX)
		dbg.printf(" x: %i,",x);
	if(flags & plNetKi)
		dbg.printf(" ki: %i,",ki);
	if(flags & plNetUID)
		dbg.printf(" uid: %s,",alcGetStrUid(uid));
	if(flags & plNetIP)
		dbg.printf(" ip: %s:%i,",alcGetStrIp(ip),ntohs(port));
	if(flags & plNetSid)
		dbg.printf(" sid: %i,",sid);
	dbg.seek(-1); // remove the last comma
	additionalFields();
	dbg.putByte(0); // this is necessary because of the seek() call
#endif
	return dbg.c_str();
}

Byte alcUnetGetVarTypeFromName(tStrBuf type) {
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
	static const char * ret;
	switch(type) {
		case DInteger:
			ret="INT";
			break;
		case DFloat:
			ret="FLOAT";
			break;
		case DBool:
			ret="BOOL";
			break;
		case DUruString:
			ret="STRING32";
			break;
		case DPlKey:
			ret="PLKEY";
			break;
		case DCreatable:
			ret="CREATABLE";
			break;
		case DTime:
			ret="TIME";
			break;
		case DByte:
			ret="BYTE";
			break;
		case DStruct:
			ret="SDL STRUCT";
			break;
		case DShort:
			ret="SHORT";
			break;
		case DAgeTimeOfDay:
			ret="AGETIMEOFDAY";
			break;
		case DVector3:
			ret="VECTOR3";
			break;
		case DPoint3:
			ret="POINT3";
			break;
		case DQuaternion:
			ret="QUATERNION";
			break;
		case DRGB8:
			ret="RGB8";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
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
	static const char * ret;
	switch(dest) {
		case KAgent:
			ret="kAgent";
			break;
		case KLobby:
			ret="kLobby";
			break;
		case KGame:
			ret="kGame";
			break;
		case KVault:
			ret="kVault";
			break;
		case KAuth:
			ret="kAuth";
			break;
		case KAdmin:
			ret="kAdmin";
			break;
		case KLookup: // same as KTracking
			ret="kLookup";
			break;
		case KClient:
			ret="kClient";
			break;
		case KMeta:
			ret="kMeta";
			break;
		case KTest:
			ret="kTest";
			break;
		case KData:
			ret="kData";
			break;
		case KProxy:
			ret="kProxy";
			break;
		case KPlFire:
			ret="kPlFire";
			break;
		case KBcast:
			ret="kBcast";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

const char * alcUnetGetReasonCode(Byte code) {
	static const char * ret;
	switch(code) {
		case RStopResponding:
			ret="StopResponding";
			break;
		case RActive:
			ret="Active";
			break;
		case RInRoute:
			ret="InRoute";
			break;
		case RArriving:
			ret="Arriving";
			break;
		case RJoining:
			ret="Joining";
			break;
		case RLeaving:
			ret="Leaving";
			break;
		case RQuitting:
			ret="Quitting";
			break;
		case RUnknown:
			ret="UnknownReason";
			break;
		case RKickedOff:
			ret="KickedOff";
			break;
		case RTimedOut:
			ret="TimedOut";
			break;
		case RLoggedInElsewhere:
			ret="LoggedInElsewhere";
			break;
		case RNotAuthenticated:
			ret="NotAuthenticated";
			break;
		case RUnprotectedCCR:
			ret="UnprotectedCCR";
			break;
		case RIllegalCCRClient:
			ret="IllegalCCRClient";
			break;
		case RHackAttempt:
			ret="HackAttempt";
			break;
		case RUnimplemented:
			ret="Unimplemented";
			break;
		case RParseError:
			ret="ParseError";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

const char * alcUnetGetAuthCode(Byte code) {
	static const char * ret;
	switch(code) {
		case AAuthSucceeded:
			ret="AuthSucceeded";
			break;
		case AAuthHello:
			ret="AuthHello";
			break;
		case AProtocolOlder:
			ret="ProtocolOlder";
			break;
		case AProtocolNewer:
			ret="ProtocolNewer";
			break;
		case AAccountExpired:
			ret="AccountExpired";
			break;
		case AAccountDisabled:
			ret="AccountDisabled";
			break;
		case AInvalidPasswd:
			ret="InvalidPasswd";
			break;
		case AInvalidUser:
			ret="InvalidUser";
			break;
		case AUnspecifiedServerError:
			ret="UnspecifiedServerError";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

const char * alcUnetGetAvatarCode(Byte code) {
	static const char * ret;
	switch(code) {
		case AOK:
			ret="Ok";
			break;
		case AUnknown:
			ret="UnknownReason";
			break;
		case ANameDoesNotHaveEnoughLetters:
			ret="NameDoesNotHaveEnoughLetters";
			break;
		case ANameIsTooShort:
			ret="NameIsTooShort";
			break;
		case ANameIsTooLong:
			ret="NameIsTooLong";
			break;
		case AInvitationNotFound:
			ret="InvitationNotFound";
			break;
		case ANameIsAlreadyInUse:
			ret="NameIsAlreadyInUse";
			break;
		case ANameIsNotAllowed:
			ret="NameIsNotAllowed";
			break;
		case AMaxNumberPerAccountReached:
			ret="MaxNumberPerAccountReached";
			break;
		case AUnspecifiedServerError:
			ret="UnspecifiedServerError";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

const char * alcUnetGetLinkingRule(Byte rule)
{
	static const char * ret;
	switch (rule) {
		case KBasicLink:
			ret="KBasicLink";
			break;
		case KOriginalBook:
			ret="KOriginalBook";
			break;
		case KSubAgeBook:
			ret="KSubAgeBook";
			break;
		case KOwnedBook:
			ret="KOwnedBook";
			break;
		case KVisitBook:
			ret="KVisitBook";
			break;
		case KChildAgeBook:
			ret="KChildAgeBook";
			break;
		default:
			ret="KUnknown";
			break;
	}
	return ret;
}

const char * alcUnetGetMsgCode(U16 code) {
	static const char * ret;
	switch(code) {
		case NetMsgPagingRoom:
			ret="NetMsgPagingRoom";
			break;
		case NetMsgJoinReq:
			ret="NetMsgJoinReq";
			break;
		case NetMsgJoinAck:
			ret="NetMsgJoinAck";
			break;
		case NetMsgLeave:
			ret="NetMsgLeave";
			break;
		case NetMsgPing:
			ret="NetMsgPing";
			break;
		case NetMsgGroupOwner:
			ret="NetMsgGroupOwner";
			break;
		case NetMsgGameStateRequest:
			ret="NetMsgGameStateRequest";
			break;
		case NetMsgGameMessage:
			ret="NetMsgGameMessage";
			break;
		case NetMsgVoice:
			ret="NetMsgVoice";
			break;
		case NetMsgTestAndSet:
			ret="NetMsgTestAndSet";
			break;
		case NetMsgMembersListReq:
			ret="NetMsgMembersListReq";
			break;
		case NetMsgMembersList:
			ret="NetMsgMembersList";
			break;
		case NetMsgMemberUpdate:
			ret="NetMsgMemberUpdate";
			break;
		case NetMsgCreatePlayer:
			ret="NetMsgCreatePlayer";
			break;
		case NetMsgAuthenticateHello:
			ret="NetMsgAuthenticateHello";
			break;
		case NetMsgAuthenticateChallenge:
			ret="NetMsgAuthenticateChallenge";
			break;
		case NetMsgInitialAgeStateSent:
			ret="NetMsgInitialAgeStateSent";
			break;
		case NetMsgVaultTask:
			ret="NetMsgVaultTask";
			break;
		case NetMsgAlive:
			ret="NetMsgAlive";
			break;
		case NetMsgTerminated:
			ret="NetMsgTerminated";
			break;
		case NetMsgSDLState:
			ret="NetMsgSDLState";
			break;
		case NetMsgSDLStateBCast:
			ret="NetMsgSDLStateBCast";
			break;
		case NetMsgGameMessageDirected:
			ret="NetMsgGameMessageDirected";
			break;
		case NetMsgRequestMyVaultPlayerList:
			ret="NetMsgRequestMyVaultPlayerList";
			break;
		case NetMsgVaultPlayerList:
			ret="NetMsgVaultPlayerList";
			break;
		case NetMsgSetMyActivePlayer:
			ret="NetMsgSetMyActivePlayer";
			break;
		case NetMsgPlayerCreated:
			ret="NetMsgPlayerCreated";
			break;
		case NetMsgFindAge:
			ret="NetMsgFindAge";
			break;
		case NetMsgFindAgeReply:
			ret="NetMsgFindAgeReply";
			break;
		case NetMsgDeletePlayer:
			ret="NetMsgDeletePlayer";
			break;
		case NetMsgAuthenticateResponse:
			ret="NetMsgAuthenticateResponse";
			break;
		case NetMsgAccountAuthenticated:
			ret="NetMsgAccountAuthenticated";
			break;
		case NetMsgRelevanceRegions:
			ret="NetMsgRelevanceRegions";
			break;
		case NetMsgLoadClone:
			ret="NetMsgLoadClone";
			break;
		case NetMsgPlayerPage:
			ret="NetMsgPlayerPage";
			break;
		case NetMsgVault:
			ret="NetMsgVault";
			break;
		case NetMsgVault2:
			ret="NetMsgVault2";
			break;
		case NetMsgSetTimeout:
			ret="NetMsgSetTimeout";
			break;
		case NetMsgActivePlayerSet: // is the same as NetMsgSetTimeout2
			ret="NetMsgActivePlayerSet|NetMsgSetTimeout2";
			break;
		case NetMsgActivePlayerSet2:
			ret="NetMsgActivePlayerSet2";
			break;
		case NetMsgCustomAuthAsk:
			ret="NetMsgCustomAuthAsk";
			break;
		case NetMsgCustomAuthResponse:
			ret="NetMsgCustomAuthResponse";
			break;
		case NetMsgCustomVaultAskPlayerList:
			ret="NetMsgCustomVaultAskPlayerList";
			break;
		case NetMsgCustomVaultPlayerList:
			ret="NetMsgCustomVaultPlayerList";
			break;
		case NetMsgCustomVaultCreatePlayer:
			ret="NetMsgCustomVaultCreatePlayer";
			break;
		case NetMsgCustomVaultPlayerCreated:
			ret="NetMsgCustomVaultPlayerCreated";
			break;
		case NetMsgCustomVaultDeletePlayer:
			ret="NetMsgCustomVaultDeletePlayer";
			break;
		case NetMsgCustomPlayerStatus:
			ret="NetMsgCustomPlayerStatus";
			break;
		case NetMsgCustomVaultCheckKi:
			ret="NetMsgCustomVaultCheckKi";
			break;
		case NetMsgCustomVaultKiChecked:
			ret="NetMsgCustomVaultKiChecked";
			break;
		case NetMsgCustomRequestAllPlStatus:
			ret="NetMsgCustomRequestAllPlStatus";
			break;
		case NetMsgCustomAllPlayerStatus:
			ret="NetMsgCustomAllPlayerStatus";
			break;
		case NetMsgCustomSetGuid:
			ret="NetMsgCustomSetGuid";
			break;
		case NetMsgCustomFindServer:
			ret="NetMsgCustomFindServer";
			break;
		case NetMsgCustomServerFound:
			ret="NetMsgCustomServerFound";
			break;
		case NetMsgCustomForkServer:
			ret="NetMsgCustomForkServer";
			break;
		case NetMsgPlayerTerminated:
			ret="NetMsgPlayerTerminated";
			break;
		case NetMsgCustomVaultPlayerStatus:
			ret="NetMsgCustomVaultPlayerStatus";
			break;
		case NetMsgCustomMetaRegister:
			ret="NetMsgCustomMetaRegister";
			break;
		case NetMsgCustomMetaPing:
			ret="NetMsgCustomMetaPing";
			break;
		case NetMsgCustomServerVault:
			ret="NetMsgCustomServerVault";
			break;
		case NetMsgCustomServerVaultTask:
			ret="NetMsgCustomServerVaultTask";
			break;
		case NetMsgCustomSaveGame:
			ret="NetMsgCustomSaveGame";
			break;
		case NetMsgCustomLoadGame:
			ret="NetMsgCustomLoadGame";
			break;
		case NetMsgCustomCmd:
			ret="NetMsgCustomCmd";
			break;
		case NetMsgCustomDirectedFwd:
			ret="NetMsgCustomDirectedFwd";
			break;
		case NetMsgCustomPlayerToCome:
			ret="NetMsgCustomPlayerToCome";
			break;
		case NetMsgCustomTest:
			ret="NetMsgCustomTest";
			break;
		default:
			ret="UnimplementedMsg";
			break;
	}
	return ret;
}

} //namespace
