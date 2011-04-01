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

#include <urutypes/uruconsts.h>
#include <urutypes/urubasetypes.h>
#include <alcutil/alclog.h>

#include <vector>

namespace alc {


class tNetSession;
class tLog;
class tUnetAck;

// unet helper types
typedef double tNetTime; // always in seconds
typedef std::pair<tNetTime, bool> tNetTimeBoolPair;


void alcEncodePacket(uint8_t* buf2,const uint8_t* buf, int n);
void alcDecodePacket(uint8_t* buf, int n);

int alcUruValidatePacket(uint8_t * buf,int n,uint8_t * validation,bool authed=false,const char * phash=NULL);
uint32_t alcUruChecksum(const uint8_t* buf, int size, int alg, const char * aux_hash);

const char * alcUnetGetRelease(uint8_t rel);
const char * alcUnetGetDestination(uint8_t dest);
const char * alcUnetGetReasonCode(uint8_t code);
const char * alcUnetGetAuthCode(uint8_t code);
const char * alcUnetGetAvatarCode(uint8_t code);
const char * alcUnetGetMsgCode(uint16_t code);

const char * alcUnetGetVarType(uint8_t type);
uint8_t alcUnetGetVarTypeFromName(tString type);

/** this class is used to save incoming NetMsgs and collect their fragments */
class tUnetMsg : public tBaseType {
public:
	tUnetMsg() { fr_count=0; }
	//virtual ~tUnetMsg() { delete data; }
	virtual void store(tBBuf &/*t*/) {}
	virtual void stream(tBBuf &/*t*/) const {}
	uint16_t cmd;
	uint32_t sn;
	uint8_t fr_count; //!< Number of fragments we already got
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
	
	uint32_t snA(void) const { return A >> 8; }
	uint8_t frA(void) const { return A & 0xFF; }
	uint32_t snB(void) const { return B >> 8; }
	uint8_t frB(void) const { return B & 0xFF; }
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
	void htmlDump(tLog * log, bool outgoing, tNetSession *u);
	
	uint32_t sn(void) const { return csn >> 8; }
	uint8_t fr(void) const { return csn & 0xFF; }
	uint32_t psn(void) const { return cps >> 8; }
	uint8_t pfr(void) const { return cps & 0xFF; }
	void set_csn(uint32_t sn, uint8_t fr) { csn=fr | sn<<8; }
	void set_cps(uint32_t psn, uint8_t pfr) { cps=pfr | psn<<8; }

	tNetTime timestamp; //!< message stamp in usecs (to send) - this is in unet->net_time units
	tNetTime snt_timestamp; //!< time when message was sent
	unsigned int tries;
	const bool urgent;
	// Uru protocol
	uint8_t val; //!< validation level
	uint32_t pn; //!< packet number
	uint8_t bhflags; //!< message type/flags
	uint32_t csn; //!< combined fragment and seq num
	uint8_t frt; //!< total fragments (1 Byte)
	uint32_t cps; //!< combined last acked fragment and seq num
	tMBuf data;
	FORBID_CLASS_COPY(tUnetUruMsg)
};

//// End of low-level protocl classes, here comes the higher level

class tmBase :public tStreamable {
public:
	tmBase(tNetSession *u) : u(u) {}
	virtual tString str() const=0;
	virtual uint8_t bhflags() const=0; //!< returns the correct send flags for this packet
	virtual bool urgent() const=0; //!< returns whether this packet should be put at the top of the sendqueue (only do this for small packages as they will be sent even against cabal limits)
	tNetSession *getSession(void) const { return u; }
protected:
	tNetSession * u; //!< associated session (source for incoming, destination for outgoing)
};

class tmNetClientComm :public tmBase {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmNetClientComm(tNetSession *u, tUnetUruMsg *msg) : tmBase(u) { tmNetClientComm::store(msg->data); }
	tmNetClientComm(const tTime &t,uint32_t bw, tNetSession *u) : tmBase(u), timestamp(t), bandwidth(bw) {}
	virtual tString str() const;
	virtual uint8_t bhflags() const { return UNetNegotiation|UNetAckReq; }
	virtual bool urgent() const { return true; }
	tTime timestamp;
	uint32_t bandwidth;
};

class tmNetAck :public tmBase {
public:
	tmNetAck(tNetSession *u, tUnetUruMsg *msg) : tmBase(u) { tmNetAck::store(msg->data); }
	tmNetAck(tNetSession *u, const tUnetAck &ack);
	
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual tString str() const;
	virtual uint8_t bhflags() const { return UNetAckReply; }
	virtual bool urgent() const { return true; }
	
	typedef std::vector<tUnetAck> tAckList;
	tAckList acks;
	FORBID_CLASS_COPY(tmNetAck)
};

class tmNetMsg :public tmBase {
protected:
	// the comments also apply for the two versions of every sublcass
	tmNetMsg(uint16_t cmd,uint32_t flags,tNetSession * u); //!< when calling this version of the constructor, hold the public data read lock of the session
	tmNetMsg(tNetSession * u) : tmBase(u), cmd(0) {} //!< thread-safe
public:
	virtual ~tmNetMsg() {};
	
	virtual void store(tBBuf &t); //!< public data read lock must NOT be hold, or it will deadlock!
	virtual void stream(tBBuf &t) const;
	
	virtual uint8_t bhflags() const { return flags & plNetAck ? UNetAckReq : 0; }
	virtual bool urgent() const { return false; }
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

#define NETMSG_RECEIVE_CONSTRUCTORS(Type, ParentType) \
	protected: Type(tNetSession *u) : ParentType(u) {} \
	public: Type(tNetSession *u, tUnetMsg *msg) : ParentType(u) \
			{ Type::store(msg->data); u->getLog()->log("%s <RCV> [%d] %s\n", u->str().c_str(), msg->sn, Type::str().c_str()); }

}

#endif
