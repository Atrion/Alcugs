/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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

/**
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_AUTHMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "alcnet.h"
#include "protocol/authmsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmCustomAuthAsk
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *u, U32 x, U32 sid, U32 ip, U16 port, const Byte *login, const Byte *challenge, const Byte *hash, Byte release)
	: tmMsgBase(NetMsgCustomAuthAsk, plNetAck | plNetCustom | plNetX | plNetVersion | plNetSid | plNetIP, u)
	{
		this->sid = sid; // this is the SID the lobby uses for the connection to the client to be authed
		this->x = x; // the X value the client sent to the lobby
		this->ip = ip; // the client's IP and Port (for logging)
		this->port = port;
		
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetIP);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
		
		this->login.setVersion(0); // normal UruString
		this->login.writeStr(login);
		memcpy(this->challenge, challenge, 16);
		memcpy(this->hash, hash, 16);
		this->release = release;
	}
	
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *u) : tmMsgBase(u)
	{
		login.setVersion(0); // normal UruString
	}
	
	void tmCustomAuthAsk::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX)) throw txProtocolError(_WHERE("X flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetIP)) throw txProtocolError(_WHERE("IP flag missing"));
#endif
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
		t.get(login);
		memcpy(challenge, t.read(16), 16);
		memcpy(hash, t.read(16), 16);
		release = t.getByte();
#ifdef ENABLE_UNET2
		if(!hasFlags(plNetIP)) {
			ip = t.getU32();
			port = 0;
			 u->proto = 1; // unet2 protocol
		}
#endif
	}
	
	void tmCustomAuthAsk::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(login);
		t.write(challenge, 16);
		t.write(hash, 16);
		t.putByte(release);
#ifdef ENABLE_UNET2
		if (u->proto == 1) {
			t.putU32(ip);
		}
#endif
	}
	
	void tmCustomAuthAsk::additionalFields()
	{
		dbg.nl();
#ifdef ENABLE_UNET2
		if (u->proto == 1) dbg.printf(" ip (unet2 protocol): %s,", alcGetStrIp(ip));
#endif
		// use two printf commands as alcGetStrGuid uses a static array and when using one command it would seem as if challenge and hash would be the same
		dbg.printf(" login: %s, challenge: %s, ", login.c_str(), alcGetStrUid(challenge));
		dbg.printf("hash: %s, build: 0x%02X (%s)", alcGetStrUid(hash), release, alcUnetGetRelease(release));
	}
	
	//// tmCustomAuthResponse
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *u, tmCustomAuthAsk &authAsk, const Byte *uid, const Byte *passwd, Byte result, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomAuthResponse, plNetAck | plNetCustom | plNetX | plNetVersion | plNetSid | plNetIP | plNetUID, u)
	 {
		// copy stuff from the authAsk
		sid = authAsk.sid; // this is the SID the lobby uses for the connection to the client to be authed
		x = authAsk.x; // the X value the client sent to the lobby
		ip = authAsk.ip; // the client's IP and Port (for finding the correct session)
		port = authAsk.port;

#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetIP | plNetUID);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
		
		login = authAsk.login;
		login.setVersion(0); // normal UruString
		
		memcpy(this->uid, uid, 16);
		this->passwd = passwd;
		this->passwd.setVersion(0); // normal UruString
		this->result = result;
		this->accessLevel = accessLevel;
	}
	
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *u) : tmMsgBase(u)
	{
		login.setVersion(0); // normal UruString
		passwd.setVersion(0); // normal UruString
	}
	
	void tmCustomAuthResponse::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX)) throw txProtocolError(_WHERE("X flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetIP | plNetUID)) throw txProtocolError(_WHERE("IP or UID flag missing"));
#endif
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
		t.get(login);
		result = t.getByte();
		t.get(passwd);
#ifdef ENABLE_UNET2
		if (!hasFlags(plNetUID)) {
			memcpy(uid, t.read(16), 16);
			ip = port = 0; // they should be initialized
			u->proto = 1; // unet2 protocol
		}
#endif
		accessLevel = t.getByte();
	}
	
	void tmCustomAuthResponse::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(login); // login
		t.putByte(result); // result
		t.put(passwd); // passwd
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); } // UID (only for old protocol, the new one sends it in the header)
#endif
		t.putByte(accessLevel); // acess level
	}
	
	void tmCustomAuthResponse::additionalFields()
	{
		dbg.nl();
#ifdef ENABLE_UNET2
		if (u->proto == 1) dbg.printf(" uid (unet2 protocol): %s,", alcGetStrUid(uid));
#endif
		dbg.printf(" login: %s, passwd: (hidden), result: 0x%02X (%s), accessLevel: %d", login.c_str(), result, alcUnetGetAuthCode(result), accessLevel);
	}

} //end namespace alc
