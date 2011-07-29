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
#define __U_LOBBYBASEMSG_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "lobbybasemsg.h"

#include "netsession.h"
#include "netexception.h"

#include <cstring>

namespace alc {

	//// tmAuthenticateHello
	void tmAuthenticateHello::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		// the vault manager sends these without X and KI, and the game server gets it with a KI set (which we ignore)
		t.get(account);
		maxPacketSize = t.get16();
		release = t.get8();
	}
	
	tString tmAuthenticateHello::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" account: %s, max packet size: %d, release: 0x%02X (%s)", account.c_str(), maxPacketSize, release, alcUnetGetRelease(release));
		return dbg;
	}
	
	//// tmAuthenticateChallenge	
	tmAuthenticateChallenge::tmAuthenticateChallenge(tNetSession *u, uint32_t x, uint8_t authResult, const uint8_t *challenge)
	: tmNetMsg(NetMsgAuthenticateChallenge, plNetKi | plNetAck | plNetX | plNetVersion, u)
	{
		ki = 0; // we're not yet logged in, so no KI can be set
		this->x = x;
		
		this->authResult = authResult;
		this->challenge.write(challenge, 16);
	}
	
	void tmAuthenticateChallenge::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put8(authResult); // auth result
		t.put(challenge); // challenge
	}
	
	tString tmAuthenticateChallenge::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" auth result: 0x%02X (%s), challenge: %s", authResult, alcUnetGetAuthCode(authResult), alcGetStrUid(challenge.data()).c_str());
		return dbg;
	}
	
	//// tmAuthenticateResponse
	void tmAuthenticateResponse::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		// the vault manager sends these without X and KI, and the game server gets it with a KI set (which we ignore)
		t.get(hash);
		if (hash.size() != 16) throw txProtocolError(_WHERE("tmAuthenticateResponse.hash must be 16 characters long"));
	}
	
	tString tmAuthenticateResponse::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" hash: %s", alcGetStrUid(hash.data()).c_str());
		return dbg;
	}
	
	//// tmAccountAutheticated	
	tmAccountAutheticated::tmAccountAutheticated(tNetSession *u, uint32_t x, uint8_t authResult, const uint8_t *serverGuid)
	: tmNetMsg(NetMsgAccountAuthenticated, plNetKi | plNetAck | plNetX | plNetUID, u)
	{
		memcpy(uid, u->uid, 16);
		this->x = x;
		ki = 0; // we're not yet logged in, so no KI can be set
		
		this->authResult = authResult;
		memcpy(this->serverGuid, serverGuid, 8);
	}
	
	void tmAccountAutheticated::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put8(authResult); // auth result
		t.write(serverGuid, 8); // server guid
	}
	
	tString tmAccountAutheticated::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" auth result: 0x%02X (%s), server guid: %s", authResult, alcUnetGetAuthCode(authResult), alcGetStrGuid(serverGuid).c_str());
		return dbg;
	}
	
	//// tmSetMyActivePlayer
	void tmSetMyActivePlayer::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing")); // VaultManager sends it without X
		t.get(avatar);
		uint8_t unk = t.get8();
		if (unk != 0)
			throw txProtocolError(_WHERE("NetMsgSetMyActivePlayer.unk is not 0 but %d", unk));
	}
	
	tString tmSetMyActivePlayer::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" avatar: %s", avatar.c_str());
		return dbg;
	}
	
	//// tmActivePlayerSet
	tmActivePlayerSet::tmActivePlayerSet(tNetSession *u, uint32_t x) : tmNetMsg(NetMsgActivePlayerSet, plNetAck | plNetKi | plNetX, u)
	{
		this->x = x;
		ki = u->ki;
	}
	
	//// tmFindAge
	tmFindAge::tmFindAge(tNetSession *u, uint32_t sid, const tmFindAge &findAge)
	 : tmNetMsg(NetMsgFindAge, plNetX | plNetKi | plNetAck | plNetSid, u), link(findAge.link)
	{
		this->x = findAge.x;
		this->ki = findAge.ki;
		this->sid = sid;
	}
	
	
	void tmFindAge::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
		if (u->isUruClient() && (ki == 0 || ki != u->ki)) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		t.get(link);
	}
	
	void tmFindAge::stream(alc::tBBuf& t) const
	{
		tmNetMsg::stream(t);
		t.put(link);
	}

	tString tmFindAge::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Age Link: %s", link.str().c_str());
		return dbg;
	}
	
	//// tmFindAgeReply
	tmFindAgeReply::tmFindAgeReply(tNetSession *u, uint32_t x, const tString &ipStr, uint16_t port, const tString &age, const uint8_t *guid)
	 : tmNetMsg(NetMsgFindAgeReply, plNetAck | plNetKi | plNetX, u), age(age), ipStr(ipStr)
	{
		this->x = x;
		ki = u->ki;
	
		this->serverPort = port;
		memcpy(serverGuid, guid, 8);
	}
	
	void tmFindAgeReply::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put8(0x1F); // the flag to define the following structure
		/* this is a combination of:
		  Server Name = 0x01, Server Type = 0x02, Server Address = 0x04,
		  Server Port = 0x08, Server GUID = 0x10 */
		t.put(age);
		t.put8(KGame); // server type
		t.put(ipStr);
		t.put16(serverPort);
		t.write(serverGuid, 8);
	}
	
	tString tmFindAgeReply::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Age filename: %s, IP: %s, Port: %d, GUID: %s", age.c_str(), ipStr.c_str(), serverPort, alcGetStrGuid(serverGuid).c_str());
		return dbg;
	}

} //end namespace alc
