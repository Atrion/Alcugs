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
	This is only the protocol.
	No sockets here, Please!!
*/

#ifndef __U_PROTOCOL_H
#define __U_PROTOCOL_H
/* CVS tag - DON'T TOUCH*/
#define __U_PROTOCOL_H_ID "$Id$"

//disable checksum checks
//#define _NO_CHECKSUM

namespace alc {

void alcEncodePacket(unsigned char* buf2,unsigned char* buf, int n);
void alcDecodePacket(unsigned char* buf, int n);

int alcUruValidatePacket(Byte * buf,int n,Byte * validation,bool authed=false,Byte * phash=NULL);
U32 alcUruChecksum(Byte* buf, int size, int alg, Byte * aux_hash);

const char * alcUnetGetRelease(Byte rel);
const char * alcUnetGetDestination(Byte dest);
const char * alcUnetGetReasonCode(Byte code);
const char * alcUnetGetAuthCode(Byte code);
const char * alcUnetGetAvatarCode(Byte code);
const char * alcUnetGetMsgCode(U16 code);

class tUnet;
class tUnetOutMsgQ;
class tNetSession;

class tUnetMsg {
public:
	tUnetMsg(U32 size=1024) { next=NULL; completed=0; fr_count=0; data=new tMBuf(size); memset(check,0,32); }
	virtual ~tUnetMsg() { delete data; }
	tUnetMsg * next;
	U16 cmd;
	U32 sn;
	U32 stamp;
	Byte completed;
	Byte fr_count; //Number of fragments
	char check[32]; //bitmap
	tMBuf * data;
	//sanity check
	U32 frt;
	Byte hsize;
};

class tUnetUruMsg :public tBaseType {
public:
	tUnetUruMsg() { next=NULL; tryes=0; }
	virtual ~tUnetUruMsg() {}
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	virtual U32 size();
	/** Get header size */
	U32 hSize();
	void dumpheader(tLog * f);
	void htmlDumpHeader(tLog * log,Byte flux,U32 ip,U16 port); //ip, port in network order
	tUnetUruMsg * next;
private:
	void _update();
	U32 timestamp; //message stamp in usecs (to send)
	U32 snd_timestamp; //original send stamp
	Byte tryes;
	//Uru protocol
	//Byte vid 0x03
	Byte val; // 0x00,0x01,0x02
	//U32 cs (only if val>0)
	U32 pn; //!< packet number
	Byte tf; //!< message type/flags
	//U32 unkA
	Byte frn; //num fragment(1 byte)
	U32 sn; //seq num (3 bytes)
	U32 csn; // -
	Byte frt; //total fragments (1 Byte)
	//U32 unkB
	Byte pfr;
	U32 ps;
	U32 cps; // -
	U32 dsize;
	tMBuf data;
	friend class tUnet;
	friend class tNetSession;
};

class tUnetAck {
public:
	tUnetAck() { next=NULL; }
	U32 timestamp;
	U32 A;
	U32 B;
	tUnetAck * next;
};

class tmBase :public tBaseType {
public:
	virtual void store(tBBuf &t)=0;
	virtual int stream(tBBuf &t)=0;
	virtual Byte * str()=0;
	Byte bhflags;
};

class tmNetClientComm :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmNetClientComm(tNetSession *s = NULL) { bhflags=0x42; this->s = s; }
	tmNetClientComm(tTime &t,U32 bw, tNetSession *s = NULL) { timestamp=t; bandwidth=bw; bhflags=0x42; this->s = s; }
	Byte * str();
	tNetSession *s;
	tTime timestamp;
	U32 bandwidth;
};

class tmNetAck :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmNetAck() { bhflags=0x80; }
	virtual ~tmNetAck();
	void add(tUnetAck * a);
	void clear();
private:
	tUnetMsgQ<tUnetAck> * ackq;
};

class tmMsgBase :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmMsgBase(U16 cmd,U32 flags,tNetSession * u);
	virtual ~tmMsgBase() {};
	void setFlags(U32 f);
	void unsetFlags(U32 f);
	void setUrgent();
	void unsetUrgent();
	U32 getFlags();
	bool hasFlags(U32 f);
	inline tNetSession *getSession(void) { return u; }
	void copyProps(tmMsgBase &t);
	Byte * str();
	
	U16 cmd;
	U32 flags;
	Byte max_version;
	Byte min_version;
	tTime timestamp;
	U32 x;
	U32 ki;
	Byte guid[16];
	U32 ip; //network order
	U32 port; //network order
	U32 sid;
protected:
	virtual void additionalFields() {} //!< writes the additional fields of this message type to the dbg buffer (called by str() to print the package)
	
	tNetSession * u; //!< associated session (source for incoming, destination for outgoing)
	tStrBuf dbg;
};

/* You must derive any message from the above class, e.g:
class tmAuthenticateHello :public tmMsgBase {
public:
	//...
	String Login;
	U32 maxpaquetsize;
	Byte release;
};

*/

#if 0
void htmlDumpHeaderRaw(st_unet * net,st_log * log,st_uru_client c,Byte * buf,int size,int flux);
#endif

}

#endif
