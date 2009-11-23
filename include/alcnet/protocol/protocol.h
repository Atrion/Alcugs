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

class tNetSession;

void alcEncodePacket(Byte* buf2,const Byte* buf, int n);
void alcDecodePacket(Byte* buf, int n);

int alcUruValidatePacket(Byte * buf,int n,Byte * validation,bool authed=false,const char * phash=NULL);
U32 alcUruChecksum(const Byte* buf, int size, int alg, const char * aux_hash);
U16 alcFixUUNetMsgCommand(U16 cmd, const tNetSession *u);

const char * alcUnetGetRelease(Byte rel);
const char * alcUnetGetDestination(Byte dest);
const char * alcUnetGetReasonCode(Byte code);
const char * alcUnetGetAuthCode(Byte code);
const char * alcUnetGetAvatarCode(Byte code);
const char * alcUnetGetLinkingRule(Byte rule);
const char * alcUnetGetMsgCode(U16 code);

const char * alcUnetGetVarType(Byte type);
Byte alcUnetGetVarTypeFromName(tStrBuf type);

/** this class is used to save incoming NetMsgs and collect their fragments */
class tUnetMsg {
public:
	tUnetMsg(U32 size=1024) : data(size) { next=NULL; fr_count=0; }
	//virtual ~tUnetMsg() { delete data; }
	tUnetMsg * next;
	U16 cmd;
	U32 sn;
	Byte fr_count; //Number of fragments we already got
	tMBuf data;
private:
	// prevent copying
	tUnetMsg(const tUnetMsg &);
	const tUnetMsg &operator=(const tUnetMsg &);
};

/** this class is used to save acks in an ackq */
class tUnetAck {
public:
	tUnetAck() { next=NULL; timestamp=0; }
	U32 timestamp;
	U32 A;
	U32 B;
	tUnetAck * next;
private:
	// prevent copying
	tUnetAck(const tUnetAck &);
	const tUnetAck &operator=(const tUnetAck &);
};

/** this is the class responsible for the UruMsg header. The data must be filled with a class derived from tmBase. */
class tUnetUruMsg : public tBaseType {
public:
	tUnetUruMsg() { next=NULL; tryes=0; }
	virtual ~tUnetUruMsg() {}
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual U32 size();
	/** Get header size */
	U32 hSize();
	void dumpheader(tLog * f);
	void htmlDumpHeader(tLog * log,Byte flux,U32 ip,U16 port); //ip, port in network order
	tUnetUruMsg * next;

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
	Byte frn; //!< num fragment(1 byte)
	U32 sn; //!< seq num (3 bytes)
	U32 csn; //!< combined fragment and seq num
	Byte frt; //!< total fragments (1 Byte)
	//U32 unkB
	Byte pfr; //!< last acked fragment
	U32 ps; //!< last acked seq num
	U32 cps; //!< combined last acked fragment and seq num
	U32 dsize; //!< size of data / number of acks in the packet
	tMBuf data;
private:
	// prevent copying
	tUnetUruMsg(const tUnetUruMsg &);
	const tUnetUruMsg &operator=(const tUnetUruMsg &);
};

class tmBase :public tBaseType {
public:
	tmBase(Byte bhflags, tNetSession *u) : bhflags(bhflags), u(u) { }
	virtual const char * str()=0;
	inline tNetSession *getSession(void) const { return u; }
	Byte bhflags;
protected:
	tNetSession * u; //!< associated session (source for incoming, destination for outgoing)
};

class tmNetClientComm :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmNetClientComm(tNetSession *u) : tmBase(UNetNegotiation|UNetAckReq|UNetUrgent, u) { }
	tmNetClientComm(tTime &t,U32 bw, tNetSession *u) : tmBase(UNetNegotiation|UNetAckReq|UNetUrgent, u) { timestamp=t; bandwidth=bw; }
	virtual const char * str();
	tTime timestamp;
	U32 bandwidth;
};

class tmNetAck :public tmBase {
public:
	tmNetAck(tNetSession *u) : tmBase(UNetAckReply|UNetUrgent, u) { }
	virtual ~tmNetAck();
	
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual const char * str();
	
	typedef std::vector<tUnetAck *> tAckList;
	tAckList ackq;
private:
	tStrBuf dbg;

	// prevent copying
	tmNetAck(const tmNetAck &);
	const tmNetAck &operator=(const tmNetAck &);
};

class tmMsgBase :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmMsgBase(U16 cmd,U32 flags,tNetSession * u);
	tmMsgBase(tNetSession * u);
	virtual ~tmMsgBase() {};
	void setFlags(U32 f);
	void unsetFlags(U32 f);
	void setUrgent();
	void unsetUrgent();
	U32 getFlags() const;
	bool hasFlags(U32 f) const;
	virtual const char * str();
	
	U16 cmd;
	U32 flags;
	Byte max_version;
	Byte min_version;
	tTime timestamp;
	U32 x;
	U32 ki;
	Byte uid[16];
	U32 sid;
protected:
	virtual void additionalFields() {} //!< writes the additional fields of this message type to the dbg buffer (called by str() to print the package)
	
	tStrBuf dbg;
private:
	void copyProps(tmMsgBase &t);
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

}

#endif
