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

/* CVS tag - DON'T TOUCH*/
#define __U_AUTHMSG_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "authmsg.h"

#include "netexception.h"

#include <cstring>

namespace alc {

	//// tmCustomAuthAsk
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *u, uint32_t x, uint32_t sid, uint32_t ip, tString login, const uint8_t *challenge, const uint8_t *hash, uint8_t release)
	: tmNetMsg(NetMsgCustomAuthAsk, plNetAck | plNetX | plNetVersion | plNetSid, u), login(login)
	{
		this->sid = sid; // this is the SID the lobby uses for the connection to the client to be authed
		this->x = x; // the X value the client sent to the lobby
		
		this->ip = ip;
		memcpy(this->challenge, challenge, 16);
		memcpy(this->hash, hash, 16);
		this->release = release;
	}
	
	void tmCustomAuthAsk::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetSid)) throw txProtocolError(_WHERE("X or Sid flag missing"));

		ip = letoh32(t.get32());
		t.get(login);
		memcpy(challenge, t.read(16), 16);
		memcpy(hash, t.read(16), 16);
		release = t.get8();
	}
	
	void tmCustomAuthAsk::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put32(htole32(ip));
		t.put(login);
		t.write(challenge, 16);
		t.write(hash, 16);
		t.put8(release);
	}
	
	tString tmCustomAuthAsk::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" IP: %s, login: %s, challenge: %s, hash: %s, build: 0x%02X (%s)", alcGetStrIp(ip).c_str(), login.c_str(), alcGetStrUid(challenge).c_str(), alcGetStrUid(hash).c_str(), release, alcUnetGetRelease(release));
		return dbg;
	}
	
	//// tmCustomAuthResponse
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *u, tmCustomAuthAsk &authAsk, const uint8_t *uid, tString passwd, uint8_t result, uint8_t accessLevel)
	 : tmNetMsg(NetMsgCustomAuthResponse, plNetAck | plNetX | plNetVersion | plNetSid | plNetUID, u), login(authAsk.login), passwd(passwd)
	 {
		// copy stuff from the authAsk
		sid = authAsk.sid; // this is the SID the lobby uses for the connection to the client to be authed
		x = authAsk.x; // the X value the client sent to the lobby
		
		memcpy(this->uid, uid, 16);
		this->result = result;
		this->accessLevel = accessLevel;
	}
	
	void tmCustomAuthResponse::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, UID or Sid flag missing"));
		
		t.get(login);
		result = t.get8();
		t.get(passwd);
		accessLevel = t.get8();
	}
	
	void tmCustomAuthResponse::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put(login); // login
		t.put8(result); // result
		t.put(passwd); // passwd
		t.put8(accessLevel); // acess level
	}
	
	tString tmCustomAuthResponse::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" login: %s, passwd: (hidden), result: 0x%02X (%s), accessLevel: %d", login.c_str(), result, alcUnetGetAuthCode(result), accessLevel);
		return dbg;
	}

} //end namespace alc
