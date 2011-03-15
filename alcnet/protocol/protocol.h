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

/* The Uru protocol, is here */

/*
	This is only the protocol.
	No sockets here, Please!!
*/

#ifndef __U_PROTOCOL_H
#define __U_PROTOCOL_H
/* CVS tag - DON'T TOUCH*/
#define __U_PROTOCOL_H_ID "$Id$"

#include <urutypes/uruconsts.h>
#include <urutypes/urubasetypes.h>

#include <vector>

namespace alc {
	
	// unet time types
	typedef unsigned long int tNetTime; // timestamp used by netcore (don't want to carry those tTimes around there), microseconds. this may overflow, to use with caution!
	typedef signed long int tNetTimeSigned; // must be the same as tNetTime, but signed - to be used only by the overdue check!
	typedef unsigned int tNetTimeDiff; // time difference between two net times, microseconds

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
class tUnetMsg : public tBaseType {
public:
	tUnetMsg() { next=NULL; fr_count=0; }
	//virtual ~tUnetMsg() { delete data; }
	virtual void store(tBBuf &/*t*/) {}
	virtual void stream(tBBuf &/*t*/) const {}
	tUnetMsg * next;
	uint16_t cmd;
	uint32_t sn;
	uint8_t fr_count; //Number of fragments we already got
	tMBuf data;
	FORBID_CLASS_COPY(tUnetMsg)
};

/** this class is used to save acks in an ackq */
class tUnetAck : public tBaseType {
public:
	tUnetAck(uint32_t A, uint32_t B, tNetTime timestamp) : timestamp(timestamp), A(A), B(B) { }
	tUnetAck(uint32_t A, uint32_t B) : timestamp(0), A(A), B(B) { }
	tNetTime timestamp;
	uint32_t A;
	uint32_t B;
	FORBID_CLASS_COPY(tUnetAck)
};

/** this is the class responsible for the UruMsg header. The data must be filled with a class derived from tmBase. */
class tUnetUruMsg : public tStreamable {
public:
	tUnetUruMsg() : tries(0), urgent(false) {}
	tUnetUruMsg(bool urgent) : tries(0), urgent(urgent) {}
	virtual ~tUnetUruMsg() {}
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual size_t size();
	/** Get header size */
	size_t hSize();
	void dumpheader(tLog * f);
	void htmlDumpHeader(tLog * log,uint8_t flux,uint32_t ip,uint16_t port); //ip, port in network order

	void _update();
	tNetTime timestamp; //!< message stamp in usecs (to send) - this is in unet->net_time units
	tNetTime snt_timestamp; //!< time when message was sent
	unsigned int tries;
	const bool urgent;
	//Uru protocol
	uint8_t val;
	uint32_t pn; //!< packet number
	uint8_t bhflags; //!< message type/flags
	uint8_t frn; //!< num fragment(1 byte)
	uint32_t sn; //!< seq num (3 bytes)
	uint32_t csn; //!< combined fragment and seq num
	uint8_t frt; //!< total fragments (1 Byte)
	uint8_t pfr; //!< last acked fragment
	uint32_t ps; //!< last acked seq num
	uint32_t cps; //!< combined last acked fragment and seq num
	uint32_t dsize; //!< size of data / number of acks in the packet
	tMBuf data;
	FORBID_CLASS_COPY(tUnetUruMsg)
};

class tmBase :public tStreamable {
public:
	tmBase(tNetSession *u) : urgent(false), u(u) {}
	virtual tString str() const=0;
	virtual uint8_t bhflags() const=0; //!< returns the correct send flags for this packet
	tNetSession *getSession(void) const { return u; }
	
	bool urgent;
protected:
	tNetSession * u; //!< associated session (source for incoming, destination for outgoing)
};

class tmNetClientComm :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmNetClientComm(tNetSession *u, tUnetUruMsg *msg) : tmBase(u) { tmNetClientComm::store(msg->data); }
	tmNetClientComm(const tTime &t,uint32_t bw, tNetSession *u) : tmBase(u), timestamp(t), bandwidth(bw)
			{ urgent = true; }
	virtual tString str() const;
	virtual uint8_t bhflags() const { return UNetNegotiation|UNetAckReq; }
	tTime timestamp;
	uint32_t bandwidth;
};

class tmNetAck :public tmBase {
public:
	tmNetAck(tNetSession *u, tUnetUruMsg *msg) : tmBase(u) { tmNetAck::store(msg->data); }
	tmNetAck(tNetSession *u, tUnetAck *ack);
	virtual ~tmNetAck();
	
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual tString str() const;
	virtual uint8_t bhflags() const { return UNetAckReply; }
	
	typedef std::vector<tUnetAck *> tAckList;
	tAckList acks;
	FORBID_CLASS_COPY(tmNetAck)
};

class tmNetMsg :public tmBase {
public:
	// the comments also apply for the two versions of every sublcass
	tmNetMsg(uint16_t cmd,uint32_t flags,tNetSession * u); //!< when calling this version of the constructor, hold the public data read lock of the session
	tmNetMsg(tNetSession * u); //!< thread-safe
	virtual ~tmNetMsg() {};
	
	virtual void store(tBBuf &t); //!< public data read lock must NOT be hold, or it will deadlock!
	virtual void stream(tBBuf &t) const;
	
	virtual uint8_t bhflags() const { return flags & plNetAck ? UNetAckReq : 0; }
	void setFlags(uint32_t f) { this->flags |= f; }
	void unsetFlags(uint32_t f) { this->flags &= ~f; }
	uint32_t getFlags() const { return flags; }
	bool hasFlags(uint32_t f) const { return (flags | f) == flags; } // there can be several flags enabled in f, so a simple & is not enough
	virtual tString str() const;
	
	uint16_t cmd;
	uint32_t flags; //!< flags of net msg
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
	void copyProps(tmNetMsg &t);
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
