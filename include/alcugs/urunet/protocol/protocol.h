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

int alcUruValidatePacket(Byte * buf,int n,Byte * validation,Byte authed=0,Byte * phash=NULL);
U32 alcUruChecksum(Byte* buf, int size, int alg, Byte * aux_hash);

char * alcUnetGetRelease(Byte rel);
char * alcUnetGetDestination(Byte dest);
char * alcUnetGetReasonCode(Byte code);
char * alcUnetGetAuthCode(Byte code);
char * alcUnetGetAvatarCode(Byte code);
char * alcUnetGetMsgCode(U16 code);

class tUnet;
class tUnetOutMsgQ;
class tNetSession;

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
	void htmlDumpHeader(tLog * log,Byte flux,U32 ip,U16 port);
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
	U32 pn; //pck num
	Byte tf; //type/flags
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
	//friend class tUnetMsgQ;
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
	virtual U32 size() { return 0; }
	virtual Byte * str()=0;
	Byte bhflags;
};

class tmNetClientComm :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmNetClientComm() { bhflags=0x42; }
	tmNetClientComm(tTime &t,U32 bw) { timestamp=t; bandwidth=bw; bhflags=0x42; }
	Byte * str();
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
	tmMsgBase(Byte flags=0x02);
	virtual ~tmMsgBase();
	U32 ki;
	U32 x;
	U32 ip;
	U32 port;
	U32 sid;
	//et all
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

int parse_plNet_msg(st_unet * net,Byte * buf,int size,int sid);
int put_plNetMsg_header(st_unet * net,Byte * buf,int size,int sid);

void copy_plNetMsg_header(st_unet * net,int sid,int ssid,int flags);
#endif


}

#endif
