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

/* The Uru protocol, is here */

/*

Uru Protocol
No sockets please

 --*/

/* CVS tag - DON'T TOUCH*/
#define __U_PROTOCOL_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "urunet/unet.h"

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
U32 alcUruChecksum(Byte* buf, int size, int alg, Byte * aux_hash) {
	int i;
	U32 aux=0;
	//S32 saux=0;
	Byte * md5buffer;
	Byte hash[16];
	int aux_size; //auxiliar size
	int whoi=0;

	DBG(4,"Checksum %i requested, packet size is %i...\n",alg,size);
	switch(alg) {
		case 0:
			for(i=6; i<(size-4); i=i+4) {
				aux = aux + *((U32 *)(buf+i)); //V1 - working Checksum algorithm
			}
			whoi=((size-6)%4);
			if(whoi==1 || whoi==2) {
				aux += *((Byte *)(buf+size-1));
			} else if(whoi==0) {
				aux += *((U32 *)(buf+size-4));
			} //else if whoi==3 Noop
			break;
		case 1:
		case 2:
			//code for the V2 - Checksum algorithm
			aux_size=size-6;
			if(alg==2) { aux_size+=32; }
			//allocate the space for the buffer
			DBG(4,"Allocating md5buffer - %i bytes...\n",aux_size);
			md5buffer = (Byte *)malloc(sizeof(Byte)*(aux_size+10));
			if(md5buffer==NULL) {
				DBG(4,"md5buffer is NULL?\n");
				return 0xFFFFFFFF;
			}
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
			lerr->log("ERR: Uru Checksum V%i is currenlty not supported in this version of the server.\n\n",alg);
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
int alcUruValidatePacket(Byte * buf,int n,Byte * validation,Byte authed,Byte * phash) {
	U32 checksum;
#ifndef _NO_CHECKSUM
	U32 aux_checksum;
#endif
	if(n>=2 && buf[0]==0x03) { //magic uru number
		if(buf[1]!=0) {
			*validation=buf[1]; //store the validation level
		}
		if(buf[1]==0x00) { return 0; } //All went OK with validation level 0
		checksum=*(U32 *)(buf+0x02); //store the checksum
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
			if(buf[1]==0x02) { //validation level 2
				alcEncodePacket(buf,buf,n);
				buf[1]=0x02;
				if(authed) { //authenticated?
					aux_checksum=alcUruChecksum(buf, n, 1, NULL);
				} else {
					aux_checksum=alcUruChecksum(buf, n, 2, phash);
				}
				alcDecodePacket(buf,n); //Now the packet will be readable
				//be sure to take bak the 0x02 byte to the header
				buf[1]=0x02;
				if(aux_checksum==checksum) { return 0; } //All went ok with v1
			}
			lerr->log("ERR: Validation level %i, got: %08X exp: %08X\n",*validation,checksum,aux_checksum);
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
	if((tf & 0x40) && (tf & 0x80)) throw txUnexpectedData(_WHERE("That must be a maliciusly crafted paquet, flags 0x80 and 0x40 cannot be set at the same time"));
	if(tf & (0x20 | 0x08 | 0x04)) throw txUnexpectedData(_WHERE("Illegal flags %02X",tf));
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
}
int tUnetUruMsg::stream(tBBuf &t) {
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
	return this->size();
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
	dsize=data.size();
	//frn=csn & 0x000000FF;
	//sn=csn >> 8;
	//pfr=cps & 0x000000FF;
	//ps=cps >> 8;
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
		log->print("<b> me <- ");
	} else {
		log->print(" me -> ");
	}
	log->print("%i %i,%i (%i) %i,%i ",pn,sn,frn,frt,ps,pfr);

	if(flux==0) {
		log->print(" <- %s:%i</b> ",alcGetStrIp(ip),ntohs(port));
	} else {
		log->print(" -> %s:%i ",alcGetStrIp(ip),ntohs(port));
	}

	int i;
	data.rewind();
	Byte * buf;
	buf=data.read();
	

	switch(tf) {
		case UNetAckReply: //0x80
			log->print("ack");
			for(i=0; i<(int)dsize; i++) {
				if(i!=0) { log->print(" |"); }
				log->print(" %i,%i %i,%i",*(Byte *)((buf+2)+i*0x10),*(U32 *)((buf+3)+i*0x10),*(Byte *)((buf+2+8)+i*0x10),*(U32 *)((buf+3+8)+i*0x10));
			}
			break;
		case UNetAckReply | UNetExt: //0x80
			log->print("aack");
			for(i=0; i<(int)dsize; i++) {
				if(i!=0) { log->print(" |"); }
				log->print(" %i,%i %i,%i",*(Byte *)((buf)+i*0x08),*(U32 *)((buf+1)+i*0x08),*(Byte *)((buf+4)+i*0x08),*(U32 *)((buf+1+4)+i*0x08));
			}
			break;
		case UNetNegotiation | UNetAckReq: //0x42
		case UNetNegotiation | UNetAckReq | UNetExt:
			log->print("Negotiation ");
			char * times;
			times=ctime((const time_t *)(buf+4));
			log->print("%i bps, %s",*(U32 *)(buf),alcGetStrTime(*(U32 *)(buf+4),*(U32 *)(buf+8)));
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

	if((tf==0x00 || tf==0x02 || tf==UNetExt || tf==(0x02 | UNetExt))) {
		if(frn==0) {
			log->print("(%04X) %s %08X",*(U16 *)(buf),alcUnetGetMsgCode(*(U16 *)(buf)),*(U32 *)(buf+2));
		} else {
			log->print("frg..");
		}
	}
	log->print("<br>\n");
	log->flush();
}

//Base message
void tmNetClientComm::store(tBBuf &t) {
	bandwidth=t.getU32();
	t.get(timestamp);
}
int tmNetClientComm::stream(tBBuf &t) {
	t.putU32(bandwidth);
	t.put(timestamp);
	return 12;
}
Byte * tmNetClientComm::str() {
	static Byte cnt[1024];
	sprintf((char *)cnt,"(Re)Negotation bandwidth: %i bps time: %s",bandwidth,(char *)timestamp.str());
	return cnt;
}

#if 0

/** Gets all plNet msg header vars
		/return returns the size
*/
int parse_plNet_msg(st_unet * net,Byte * buf,int size,int sid) {
	st_uru_client * u=&net->s[sid];
	int off=0;
	if(buf==NULL) {
		DBG(3,"Null buffer? (size:%i,sid:%i)\n",size,sid);
		return -1;
	}
	u->hmsg.cmd=*(U16 *)(buf+off);
	off+=2;
	u->hmsg.flags=*(U32 *)(buf+off);
	off+=4;

	//put here a cmd printer
	print2log(net->log,"\n %04X %08X\n",u->hmsg.cmd,u->hmsg.flags);

	if(u->hmsg.flags & plNetAck) {
		print2log(net->log," Ack flag on\n");
	} else {
		print2log(net->log," Ack flag off\n");
	}
	if(u->hmsg.flags & plNetFirewalled) {
		print2log(net->log," Firewalled flag ON\n");
	}
	if(u->hmsg.flags & plNetBcast) {
		print2log(net->log," Bcast flag ON\n");
	}
	if(u->hmsg.flags & plNetCustom) {
		print2log(net->log," UCPNPI flag ON\n");
	}

	if(u->hmsg.flags & plNetVersion) {
		u->hmsg.max_version=*(Byte *)(buf+off);
		off++;
		u->hmsg.min_version=*(Byte *)(buf+off);
		off++;
		print2log(net->log," Version: %i.%i\n",u->hmsg.max_version,u->hmsg.min_version);
		//now set up version in u var (compatibility)
		//IA-guess
		if(u->max_version==0) {
			u->max_version=u->hmsg.max_version;
			u->min_version=u->hmsg.min_version;
		}
		// END IA
	}// else {
		//u->adv_msg.min_version=0;
		//u->adv_msg.max_version=0;
	//}

	//IA-guess the protocol version from behaviours
	// The first message is always an auth hello that contains the version numbers
	if(u->hmsg.flags & plNetTimestamp && u->max_version==0) {
		u->max_version=12; //sure (normally on ping proves)
		u->min_version=6;
	} else if(u->max_version==0) {
		u->max_version=12;
		u->min_version=0;
	}// END IA

	DBG(5,"Detected version is %i.%i\n",u->max_version,u->min_version);
	//abort();

	//NetMsgPing should have always the timestamp enabled in new versions.
	if(u->hmsg.flags & plNetTimestamp || u->min_version<0x06) { // || u->hmsg.cmd==NetMsgPing) {
		u->hmsg.stamp=*(U32 *)(buf+off);
		off+=4;
		u->hmsg.micros=*(U32 *)(buf+off);
		off+=4;
		print2log(f_uru," Timestamp: %s\n",get_stime(u->hmsg.stamp,u->hmsg.micros));
	} else {
		u->hmsg.stamp=0;
		u->hmsg.micros=0;
	}
	if(u->hmsg.flags & plNetX) {
		u->hmsg.x=*(U32 *)(buf+off);
		off+=4;
		print2log(net->log," x: %i\n",u->hmsg.x);
	} else {
		u->hmsg.x=0;
	}
	if(u->hmsg.flags & plNetKi) {
		u->hmsg.ki=*(U32 *)(buf+off);
		off+=4;
		print2log(net->log," ki: %i\n",u->hmsg.ki);
	} else {
		u->hmsg.ki=0;
	}
	if(u->hmsg.flags & plNetGUI) {
		memcpy(u->hmsg.guid,buf+off,16);
		off+=16;
		print2log(net->log," guid: %s\n",create_str_guid(u->hmsg.guid));
	}
	if(u->hmsg.flags & plNetIP) {
		u->hmsg.ip=*(U32 *)(buf+off);
		off+=4;
		u->hmsg.port=*(U16 *)(buf+off);
		off+=2;
		print2log(f_uru," ip: %s:%i\n",get_ip(u->hmsg.ip),u->hmsg.port);
	}
	//now catch undocumented protocol flags

	if(u->hmsg.flags & ~(plNetAck | plNetBcast | plNetVersion | plNetTimestamp | \
	plNetX | plNetKi | plNetGUI | plNetIP | plNetCustom)) {
		nlog(net->unx,net,sid,"Problem parsing a plNetMsg header format mask %08X\n",u->hmsg.flags & ~(plNetAck | plNetBcast | plNetVersion | plNetTimestamp | \
	plNetX | plNetKi | plNetGUI | plNetIP | plNetCustom));
		dumpbuf(net->unx,buf,size);
		lognl(net->unx);
		lognl(net->unx);
	}

	return off;
}

/**
	puts in the buffer all plnet msg header vars
*/
int put_plNetMsg_header(st_unet * net,Byte * buf,int size,int sid) {
	st_uru_client * u=&net->s[sid];
	int off=0;

	//the msg command
	*(U16 *)(buf+off)=u->hmsg.cmd;
	off+=2;
	//format flags
	*(U32 *)(buf+off)=u->hmsg.flags;
	off+=4;

	//put here a cmd printer
	print2log(net->log,"\n %04X %08X\n",u->hmsg.cmd,u->hmsg.flags);

	if(u->hmsg.flags & plNetAck) {
		print2log(net->log," Ack flag on\n");
	} else {
		print2log(net->log," Ack flag off\n");
	}
	if(u->hmsg.flags & plNetFirewalled) {
		print2log(net->log," Firewalled flag ON\n");
	}
	if(u->hmsg.flags & plNetBcast) {
		print2log(net->log," Bcast flag ON\n");
	}
	if(u->hmsg.flags & plNetCustom) {
		print2log(net->log," UCPNPI flag ON\n");
	}

	//put things
	if(u->hmsg.flags & plNetVersion) {
		print2log(net->log," Version: %i.%i\n",u->hmsg.max_version,u->hmsg.min_version);
		*(Byte *)(buf+off)=u->hmsg.max_version;
		off++;
		*(Byte *)(buf+off)=u->hmsg.min_version;
		off++;
	}

	//we must specify a valid version number after sending the first message
#if 0
	//IA-guess the protocol version from behaviours
	if(u->hmsg.flags & plNetTimestamp && u->max_version==0) {
		u->max_version=12; //sure (normally on ping proves)
		u->min_version=6;
	} else if(u->max_version==0) {
		u->max_version=12;
		u->min_version=0;
	}// END IA
#endif

	//NetMsgPing should have always the timestamp enabled in new versions.
	if(u->hmsg.flags & plNetTimestamp || (u->min_version<6 && u->max_version==12)) { // || u->adv_msg.cmd==NetMsgPing) {
		DBG(2,"minor version is %i\n",u->min_version);
		if(u->hmsg.stamp==0) {
			time((time_t *)&u->hmsg.stamp);
			u->hmsg.micros=get_microseconds();
		}
		print2log(net->log," Timestamp: %s\n",get_stime(u->hmsg.stamp,u->hmsg.micros));
		*(U32 *)(buf+off)=u->hmsg.stamp;
		off+=4;
		*(U32 *)(buf+off)=u->hmsg.micros;
		off+=4;
	}
	if(u->hmsg.flags & plNetX) {
		print2log(net->log," x: %i\n",u->hmsg.x);
		*(U32 *)(buf+off)=u->hmsg.x;
		off+=4;
	}
	if(u->hmsg.flags & plNetKi) {
		print2log(net->log," ki: %i\n",u->hmsg.ki);
		*(U32 *)(buf+off)=u->hmsg.ki;
		off+=4;
	}

	if(u->hmsg.flags & plNetGUI) {
		memcpy(buf+off,u->hmsg.guid,16);
		off+=16;
		print2log(net->log," guid: %s\n",create_str_guid(u->hmsg.guid));
	}
	if(u->hmsg.flags & plNetIP) {
		*(U32 *)(buf+off)=u->hmsg.ip;
		off+=4;
		*(U16 *)(buf+off)=u->hmsg.port;
		off+=2;
		print2log(f_uru," ip: %s:%i\n",get_ip(u->hmsg.ip),u->hmsg.port);
	}

#if 0
	//now catch undocumented protocol flags
	if(u->adv_msg.format & 0xEFF8ADDE) { //0xFFF8ADFE
		stamp2log(f_une);
		ip2log(f_une,u);
		plog(f_une,"[%s,%s]",u->login,u->avatar_name);
		print2log(f_une,"Problem parsing a plNetMsg header format mask %08X\n",u->adv_msg.format & 0xFFF8ADFE);
		//dump_packet(f_une,buf,u->client.size,0,0x07);
		//print2log(f_une,"\n");
		//off=-1;
	}
#endif //this was useless

	return off;
}


/**
	copies one plNetMsg header to another one
	sid=destination
	ssid=source
*/
void copy_plNetMsg_header(st_unet * net,int sid,int ssid,int flags) {
	if(net_check_address(net,sid)!=0) return;
	if(net_check_address(net,ssid)!=0) return;
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	//int off=0;

	//the msg command
	u->hmsg.cmd=s->hmsg.cmd;
	//format flags
	u->hmsg.flags=s->hmsg.flags;

	//put here a cmd printer
	//print2log(net->log," %04X %08X\n",u->hmsg.cmd,u->hmsg.flags);

	/*
	if(flags & plNetAck) {
		print2log(net->log," Ack flag on\n");
	} else {
		print2log(net->log," Ack flag off\n");
	}
	if(u->hmsg.flags & plNetFirewalled) {
		print2log(net->log," Firewalled flag ON\n");
	}
	if(u->hmsg.flags & plNetBcast) {
		print2log(net->log," Bcast flag ON\n");
	}
	if(u->hmsg.flags & plNetCustom) {
		print2log(net->log," UCPNPI flag ON\n");
	}
	*/

	//put things
	if(flags & plNetVersion) {
		//print2log(net->log," Version: %i.%i\n",u->hmsg.max_version,u->hmsg.min_version);
		u->hmsg.max_version=s->hmsg.max_version;
		u->hmsg.min_version=s->hmsg.min_version;
	}

	//NetMsgPing should have always the timestamp enabled in new versions.
	if(flags & plNetTimestamp) {
		//DBG(2,"minor version is %i\n",u->min_version);
		//print2log(net->log," Timestamp: %s\n",get_stime(u->hmsg.stamp,u->hmsg.micros));
		u->hmsg.stamp=s->hmsg.stamp;
		u->hmsg.micros=s->hmsg.stamp;
	}
	if(flags & plNetX) {
		//print2log(net->log," x: %i\n",u->hmsg.x);
		u->hmsg.x=s->hmsg.x;
	}
	if(flags & plNetKi) {
		//print2log(net->log," ki: %i\n",u->hmsg.ki);
		u->hmsg.ki=s->hmsg.ki;
	}

	if(flags & plNetGUI) {
		memcpy(u->hmsg.guid,s->hmsg.guid,16);
		//print2log(net->log,"\n guid: %s\n",get_guid(u->hmsg.guid));
	}
	if(flags & plNetIP) {
		u->hmsg.ip=s->hmsg.ip;
		u->hmsg.port=s->hmsg.port;
		//print2log(f_uru," ip: %s:%i\n",get_ip(u->hmsg.ip),u->hmsg.port);
	}

}

#endif

char * alcUnetGetRelease(Byte rel) {
	static char * ret;
	switch(rel) {
		case 0x03:
			ret="ExtRel";
			break;
		case 0x02:
			ret="IntRel";
			break;
		case 0x01:
			ret="Dbg";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * alcUnetGetDestination(Byte dest) {
	static char * ret;
	switch(dest) {
		case 0x01:
			ret="kAgent";
			break;
		case 0x02:
			ret="kLobby";
			break;
		case 0x03:
			ret="kGame";
			break;
		case 0x04:
			ret="kVault";
			break;
		case 0x05:
			ret="kAuth";
			break;
		case 0x06:
			ret="kAdmin";
			break;
		case 0x07:
			ret="kLookup";
			break;
		case 0x08:
			ret="kClient";
			break;
		case KMeta:
			ret="kMeta";
			break;
		case KData:
			ret="kData";
			break;
		case KBcast:
			ret="kBcast";
			break;
		case KTest:
			ret="kTest";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * alcUnetGetReasonCode(Byte code) {
	static char * ret;
	switch(code) {
		case 0x00:
			ret="StopResponding";
			break;
		case 0x14:
			ret="Active";
			break;
		case 0x16:
			ret="InRoute";
			break;
		case 0x17:
			ret="Arriving";
			break;
		case 0x18:
			ret="Joining";
			break;
		case 0x19:
			ret="Leaving";
			break;
		case 0x1A:
			ret="Quitting";
			break;
		case 0x01:
			ret="Unknown";
			break;
		case 0x02:
			ret="KickedOff";
			break;
		case 0x03:
			ret="TimedOut";
			break;
		case 0x04:
			ret="LoggedInElsewhere";
			break;
		case 0x05:
			ret="NotAuthenticated";
			break;
		case 0x06:
			ret="UnprotectedCCR";
			break;
		case 0x07:
			ret="IllegalCCRClient";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * alcUnetGetAuthCode(Byte code) {
	static char * ret;
	switch(code) {
		case 0x00:
			ret="AuthSucceeded";
			break;
		case 0x01:
			ret="AuthHello";
			break;
		case 0xF7:
			ret="ProtocolOlder";
			break;
		case 0xF8:
			ret="ProtocolNewer";
			break;
		case 0xFB:
			ret="AccountExpired";
			break;
		case 0xFC:
			ret="AccountDissabled";
			break;
		case 0xFD:
			ret="InvalidPasswd";
			break;
		case 0xFE:
			ret="InvalidUser";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * alcUnetGetAvatarCode(Byte code) {
	static char * ret;
	switch(code) {
		case 0x00:
			ret="Ok";
			break;
		case 0xF8:
			ret="NameDoesNotHaveEnoughLetters";
			break;
		case 0xF9:
			ret="NameIsTooShort";
			break;
		case 0xFA:
			ret="NameIsTooLong";
			break;
		case 0xFB:
			ret="InvitationNotFound";
			break;
		case 0xFC:
			ret="NameIsAlreadyInUse";
			break;
		case 0xFD:
			ret="NameIsNotAllowed";
			break;
		case 0xFE:
			ret="MaxNumberPerAccountReached";
			break;
		case 0xFF:
			ret="UnspecifiedServerError";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * alcUnetGetMsgCode(U16 code) {
	static char * ret;
	switch(code) {
		case 0x0218:
			ret="NetMsgPagingRoom";
			break;
		case 0x025A:
			ret="NetMsgJoinReq";
			break;
		case 0x025B:
			ret="NetMsgJoinAck";
			break;
		case 0x025C:
			ret="NetMsgLeave";
			break;
		case 0x025D:
			ret="NetMsgPing";
			break;
		case 0x025F:
			ret="NetMsgGroupOwner";
			break;
		case 0x0260:
			ret="NetMsgGameStateRequest";
			break;
		case 0x0266:
			ret="NetMsgGameMessage";
			break;
		case 0x0274:
			ret="NetMsgVoice";
			break;
		case 0x0278:
			ret="NetMsgTestAndSet";
			break;
		case 0x02A8:
			ret="NetMsgMembersListReq";
			break;
		case 0x02A9:
			ret="NetMsgMembersList";
			break;
		case 0x02AC:
			ret="NetMsgMemberUpdate";
			break;
		case 0x02AE:
			ret="NetMsgCreatePlayer";
			break;
		case 0x02AF:
			ret="NetMsgAuthenticateHello";
			break;
		case 0x02B0:
			ret="NetMsgAuthenticateChallenge";
			break;
		case 0x02B3:
			ret="NetMsgInitialAgeStateSent";
			break;
		case 0x02BE:
			ret="NetMsgVaultTask";
			break;
		case 0x02C5:
			ret="NetMsgAlive";
			break;
		case 0x02C6:
			ret="NetMsgTerminated";
			break;
		case 0x02C8:
			ret="NetMsgSDLState";
			break;
		case 0x0324:
			ret="NetMsgSDLStateBCast";
			break;
		case 0x034E:
			ret="NetMsgRequestMyVaultPlayerList";
			break;
		case 0x0373:
			ret="NetMsgVaultPlayerList";
			break;
		case 0x0374:
			ret="NetMsgSetMyActivePlayer";
			break;
		case 0x0377:
			ret="NetMsgPlayerCreated";
			break;
		case 0x037A:
			ret="NetMsgFindAge";
			break;
		case 0x037B:
			ret="NetMsgFindAgeReply";
			break;
		case 0x0384:
			ret="NetMsgDeletePlayer";
			break;
		case 0x0393:
			ret="NetMsgAuthenticateResponse";
			break;
		case 0x0394:
			ret="NetMsgAccountAuthenticated";
			break;
		case 0x03AE:
			ret="NetMsgLoadClone";
			break;
		case 0x03AF:
			ret="NetMsgPlayerPage";
			break;
		case 0x0428:
			ret="NetMsgVault";
			break;
		case 0x0429:
			ret="NetMsgVault2";
			break;
		case 0x0464:
			ret="NetMsgSetTimeout";
			break;
		case 0x0465:
			ret="NetMsgActivePlayerSet_NetMsgSetTimeout2";
			break;
		case 0x0466:
			ret="NetMsgActivePlayerSet2";
			break;
		case 0x1001:
			ret="NetMsgCustomAuthAsk";
			break;
		case 0x1002:
			ret="NetMsgCustomAuthResponse";
			break;
		case 0x1003:
			ret="NetMsgCustomVaultAskPlayerList";
			break;
		case 0x1004:
			ret="NetMsgCustomVaultPlayerList";
			break;
		case 0x1005:
			ret="NetMsgCustomVaultCreatePlayer";
			break;
		case 0x1006:
			ret="NetMsgCustomVaultPlayerCreated";
			break;
		case 0x1007:
			ret="NetMsgCustomVaultDeletePlayer";
			break;
		case 0x1008:
			ret="NetMsgCustomPlayerStatus";
			break;
		case 0x1009:
			ret="NetMsgCustomVaultCheckKi";
			break;
		case 0x100A:
			ret="NetMsgCustomVaultKiChecked";
			break;
		case 0x100B:
			ret="NetMsgCustomRequestAllPlStatus";
			break;
		case 0x100C:
			ret="NetMsgCustomAllPlayerStatus";
			break;
		case 0x100D:
			ret="NetMsgCustomSetGuid";
			break;
		case 0x100E:
			ret="NetMsgCustomFindServer";
			break;
		case 0x100F:
			ret="NetMsgCustomServerFound";
			break;
		case 0x1010:
			ret="NetMsgCustomForkServer";
			break;
		case 0x1011:
			ret="NetMsgPlayerTerminated";
			break;
		case 0x1012:
			ret="NetMsgCustomVaultPlayerStatus";
			break;
		case 0x1013:
			ret="NetMsgCustomMetaRegister";
			break;
		case 0x1014:
			ret="NetMsgCustomMetaPing";
			break;
		default:
			ret="fix_me";
			break;
	}
	return ret;
}


#if 0

//flux 0 client -> server, 1 server -> client
void htmlDumpHeader(st_log * log,st_uru_client c,st_uru_head h,Byte * buf,int size,int flux) {
	if(log==NULL) return;

	static int count=0;
	count++;

	print2log(log,"%i: ",count);
	stamp2log(log);

	switch(h.t) {
		case NetAck: //0x80
			print2log(log,"<font color=red>");
			break;
		case NetClientComm: //0x42
			print2log(log,"<font color=green>");
			break;
		case NetMsg0: //0x00
			print2log(log,"<font color=blue>");
			break;
		case NetMsg1: //0x02
			print2log(log,"<font color=black>");
			break;
		default:
			print2log(log,"<font color=pink>");
			break;
	}


	if(flux==0) {
		print2log(log,"<b> me <- ");
	} else {
		print2log(log," me -> ");
	}
	print2log(log,"%i %i,%i (%i) %i,%i ",h.p_n,h.fr_n,h.sn,h.fr_t,h.fr_ack,h.ps);

	if(flux==0) {
	print2log(log," <- %s:%i</b> ",get_ip(c.ip),ntohs(c.port));
	} else {
	print2log(log," -> %s:%i ",get_ip(c.ip),ntohs(c.port));
	}

	int i;

	switch(h.t) {
		case NetAck: //0x80
			print2log(log,"ack");
			for(i=0; i<(int)h.size; i++) {
				if(i!=0) { print2log(log," |"); }
				print2log(log," %i,%i %i,%i",*(Byte *)((buf+2)+i*0x10),*(U32 *)((buf+3)+i*0x10),*(Byte *)((buf+2+8)+i*0x10),*(U32 *)((buf+3+8)+i*0x10));
			}
			break;
		case NetClientComm: //0x42
			print2log(log,"Negotiation ");
			char * times;
			times=ctime((const time_t *)(buf+4));
			print2log(log,"%i bps, %s %i ms",*(U32 *)(buf),times,*(U32 *)(buf+8));
			break;
		case NetMsg0: //0x00
			print2log(log,"plNetMsg0 ");
			break;
		case NetMsg1: //0x02
			print2log(log,"plNetMsg1 ");
			break;
		default:
			print2log(log,"ERROR! ");
			break;
	}

	print2log(log,"</font>");

	if((h.t==NetMsg0 || h.t==NetMsg1)) {
		if(h.fr_n==0) {
			print2log(log,"(%04X) %s %08X",*(U16 *)(buf),unet_get_msg_code(*(U16 *)(buf)),*(U32 *)(buf+2));
		} else {
			print2log(log,"frg..");
		}
	}
	print2log(log,"<br>\n");

	logflush(log);

}

void htmlDumpHeaderRaw(st_unet * net,st_log * log,st_uru_client c,Byte * buf,int size,int flux) {

	st_uru_head h;
	int aux;

	aux=uru_get_header(buf,size,&h);

	if(flux==1) {
		print2log(net->log,"<SND>");
	} else {
		print2log(net->log,"<RCV>");
	}
	uru_print_header(net->log,&h);
	print2log(net->log,"\n");

	htmlDumpHeader(log,c,h,buf+aux,size,flux);
}

#endif

} //namespace
