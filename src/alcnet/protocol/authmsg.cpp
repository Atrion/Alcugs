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

#include "alcnet.h"
#include "protocol/ext-protocol.h"

#include <alcdebug.h>

namespace alc {

	//// tmCustomAuthAsk
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *u, U32 x, U32 sid, U32 ip, U16 port, const char *login, const Byte *challenge, const Byte *hash, Byte release)
	: tmMsgBase(NetMsgCustomAuthAsk, plNetAck | plNetX | plNetVersion | plNetSid | plNetIP, u), login(login)
	{
		this->sid = sid; // this is the SID the lobby uses for the connection to the client to be authed
		this->x = x; // the X value the client sent to the lobby
		this->ip = ip; // the client's IP and Port (for logging)
		this->port = port;
		
		memcpy(this->challenge, challenge, 16);
		memcpy(this->hash, hash, 16);
		this->release = release;
	}
	
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmCustomAuthAsk::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetIP | plNetSid)) throw txProtocolError(_WHERE("X, IP or Sid flag missing"));

		t.get(login);
		memcpy(challenge, t.read(16), 16);
		memcpy(hash, t.read(16), 16);
		release = t.getByte();
	}
	
	void tmCustomAuthAsk::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(login);
		t.write(challenge, 16);
		t.write(hash, 16);
		t.putByte(release);
	}
	
	void tmCustomAuthAsk::additionalFields()
	{
		dbg.nl();
		// use two printf commands as alcGetStrGuid uses a static array and when using one command it would seem as if challenge and hash would be the same
		dbg.printf(" login: %s, challenge: %s, ", login.c_str(), alcGetStrUid(challenge));
		dbg.printf("hash: %s, build: 0x%02X (%s)", alcGetStrUid(hash), release, alcUnetGetRelease(release));
	}
	
	//// tmCustomAuthResponse
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *u, tmCustomAuthAsk &authAsk, const Byte *uid, const char *passwd, Byte result, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomAuthResponse, plNetAck | plNetX | plNetVersion | plNetSid | plNetIP | plNetUID, u), login(authAsk.login), passwd(passwd)
	 {
		// copy stuff from the authAsk
		sid = authAsk.sid; // this is the SID the lobby uses for the connection to the client to be authed
		x = authAsk.x; // the X value the client sent to the lobby
		ip = authAsk.ip; // the client's IP and Port (for finding the correct session)
		port = authAsk.port;
		
		memcpy(this->uid, uid, 16);
		this->result = result;
		this->accessLevel = accessLevel;
	}
	
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmCustomAuthResponse::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetIP | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, IP, UID or Sid flag missing"));
		
		t.get(login);
		result = t.getByte();
		t.get(passwd);
		accessLevel = t.getByte();
	}
	
	void tmCustomAuthResponse::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(login); // login
		t.putByte(result); // result
		t.put(passwd); // passwd
		t.putByte(accessLevel); // acess level
	}
	
	void tmCustomAuthResponse::additionalFields()
	{
		dbg.nl();
		dbg.printf(" login: %s, passwd: (hidden), result: 0x%02X (%s), accessLevel: %d", login.c_str(), result, alcUnetGetAuthCode(result), accessLevel);
	}

} //end namespace alc
