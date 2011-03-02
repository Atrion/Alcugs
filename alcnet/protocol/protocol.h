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

#include "unet.h"
#include <urutypes/uruconsts.h>
#include <urutypes/urubasetypes.h>

#include <vector>

namespace alc {

class tNetSession;
class tLog;

void alcEncodePacket(uint8_t* buf2,const uint8_t* buf, int n);
void alcDecodePacket(uint8_t* buf, int n);

int alcUruValidatePacket(uint8_t * buf,int n,uint8_t * validation,bool authed=false,const char * phash=NULL);
uint32_t alcUruChecksum(const uint8_t* buf, int size, int alg, const char * aux_hash);

uint16_t alcFixUUNetMsgCommand(uint16_t cmd, const tNetSession *u);

const char * alcUnetGetRelease(uint8_t rel);
const char * alcUnetGetDestination(uint8_t dest);
const char * alcUnetGetReasonCode(uint8_t code);
const char * alcUnetGetAuthCode(uint8_t code);
const char * alcUnetGetAvatarCode(uint8_t code);
const char * alcUnetGetLinkingRule(uint8_t rule);
const char * alcUnetGetMsgCode(uint16_t code);

const char * alcUnetGetVarType(uint8_t type);
uint8_t alcUnetGetVarTypeFromName(tString type);

/** this class is used to save incoming NetMsgs and collect their fragments */
class tUnetMsg {
public:
	tUnetMsg() { next=NULL; fr_count=0; }
	//virtual ~tUnetMsg() { delete data; }
	tUnetMsg * next;
	uint16_t cmd;
	uint32_t sn;
	uint8_t fr_count; //Number of fragments we already got
	tMBuf data;
	FORBID_CLASS_COPY(tUnetMsg)
};

/** this class is used to save acks in an ackq */
class tUnetAck {
public:
	tUnetAck() { next=NULL; timestamp=0; }
	tNetTime timestamp;
	uint32_t A;
	uint32_t B;
	tUnetAck * next;
	FORBID_CLASS_COPY(tUnetAck)
};

/** this is the class responsible for the UruMsg header. The data must be filled with a class derived from tmBase. */
class tUnetUruMsg : public tBaseType {
public:
	tUnetUruMsg() { next=NULL; tryes=0; }
	virtual ~tUnetUruMsg() {}
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual size_t size();
	/** Get header size */
	size_t hSize();
	void dumpheader(tLog * f);
	void htmlDumpHeader(tLog * log,uint8_t flux,uint32_t ip,uint16_t port); //ip, port in network order
	tUnetUruMsg * next;

	void _update();
	tNetTime timestamp; //!< message stamp in usecs (to send) - this is in unet->net_time units
	tNetTime snt_timestamp; //!< time when message was sent
	unsigned int tryes;
	//Uru protocol
	//Byte vid 0x03
	uint8_t val; // 0x00,0x01,0x02
	//U32 cs (only if val>0)
	uint32_t pn; //!< packet number
	uint8_t tf; //!< message type/flags
	//U32 unkA
	uint8_t frn; //!< num fragment(1 byte)
	uint32_t sn; //!< seq num (3 bytes)
	uint32_t csn; //!< combined fragment and seq num
	uint8_t frt; //!< total fragments (1 Byte)
	//U32 unkB
	uint8_t pfr; //!< last acked fragment
	uint32_t ps; //!< last acked seq num
	uint32_t cps; //!< combined last acked fragment and seq num
	uint32_t dsize; //!< size of data / number of acks in the packet
	tMBuf data;
	FORBID_CLASS_COPY(tUnetUruMsg)
};

class tmBase :public tBaseType {
public:
	tmBase(uint8_t bhflags, tNetSession *u) : bhflags(bhflags), u(u) { }
	virtual tString str() const=0;
	inline tNetSession *getSession(void) const { return u; }
	uint8_t bhflags;
protected:
	tNetSession * u; //!< associated session (source for incoming, destination for outgoing)
};

class tmNetClientComm :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmNetClientComm(tNetSession *u) : tmBase(UNetNegotiation|UNetAckReq|UNetUrgent, u) { }
	tmNetClientComm(tTime &t,uint32_t bw, tNetSession *u) : tmBase(UNetNegotiation|UNetAckReq|UNetUrgent, u) { timestamp=t; bandwidth=bw; }
	virtual tString str() const;
	tTime timestamp;
	uint32_t bandwidth;
};

class tmNetAck :public tmBase {
public:
	tmNetAck(tNetSession *u) : tmBase(UNetAckReply|UNetUrgent, u) { }
	virtual ~tmNetAck();
	
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual tString str() const;
	
	typedef std::vector<tUnetAck *> tAckList;
	tAckList ackq;
	FORBID_CLASS_COPY(tmNetAck)
};

class tmMsgBase :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmMsgBase(uint16_t cmd,uint32_t flags,tNetSession * u);
	tmMsgBase(tNetSession * u);
	virtual ~tmMsgBase() {};
	void setFlags(uint32_t f);
	void unsetFlags(uint32_t f);
	void setUrgent();
	void unsetUrgent();
	uint32_t getFlags() const;
	bool hasFlags(uint32_t f) const;
	virtual tString str() const;
	
	uint16_t cmd;
	uint32_t flags;
	uint8_t max_version;
	uint8_t min_version;
	tTime timestamp;
	uint32_t x;
	uint32_t ki;
	uint8_t uid[16];
	uint32_t sid;
protected:
	virtual tString additionalFields(tString dbg) const { return dbg; } // this makes it much easier to disable message logging via #ifdef
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
