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

/* CVS tag - DON'T TOUCH*/
#define __U_LOBBYBASEMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "alcnet.h"
#include "protocol/lobbybasemsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmAuthenticateHello
	tmAuthenticateHello::tmAuthenticateHello(tNetSession *u) : tmMsgBase(u) // it's not capable of sending
	{
		account.setVersion(0); // normal UruString
	}
	
	void tmAuthenticateHello::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// the vault manager sends these without X and KI, and the game server gets it with a KI set (which we ignore)
		t.get(account);
		maxPacketSize = t.getU16();
		release = t.getByte();
	}
	
	void tmAuthenticateHello::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" account: %s, max packet size: %d, release: 0x%02X (%s)", account.c_str(), maxPacketSize, release, alcUnetGetRelease(release));
	}
	
	//// tmAuthenticateChallenge	
	tmAuthenticateChallenge::tmAuthenticateChallenge(tNetSession *u, U32 x, Byte authResult, const Byte *challenge)
	: tmMsgBase(NetMsgAuthenticateChallenge, plNetKi | plNetAck | plNetX | plNetVersion | plNetCustom, u)
	{
		ki = 0; // we're not yet logged in, so no KI can be set
		this->x = x;
		
		this->authResult = authResult;
		this->challenge.write(challenge, 16);
		this->challenge.setVersion(0); // normal UruString, but in Hex, not Ascii
	}
	
	void tmAuthenticateChallenge::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putByte(authResult); // auth result
		t.put(challenge); // challenge
	}
	
	void tmAuthenticateChallenge::additionalFields()
	{
		dbg.nl();
		dbg.printf(" auth result: 0x%02X (%s), challenge: %s", authResult, alcUnetGetAuthCode(authResult), alcGetStrUid(challenge.readAll()));
	}
	
	//// tmAuthenticateResponse
	tmAuthenticateResponse::tmAuthenticateResponse(tNetSession *u) : tmMsgBase(u) // it's not capable of sending
	{
		hash.setVersion(0); // normal UruString, but in Hex, not Ascii
	}
	
	void tmAuthenticateResponse::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// the vault manager sends these without X and KI, and the game server gets it with a KI set (which we ignore)
		t.get(hash);
		if (hash.size() != 16) throw txProtocolError(_WHERE("tmAuthenticateResponse.hash must be 16 characters long"));
	}
	
	void tmAuthenticateResponse::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" hash: %s", alcGetStrUid(hash.readAll()));
	}
	
	//// tmAccountAutheticated	
	tmAccountAutheticated::tmAccountAutheticated(tNetSession *u, U32 x, Byte authResult, const Byte *serverGuid)
	: tmMsgBase(NetMsgAccountAuthenticated, plNetKi | plNetAck | plNetX | plNetUID | plNetCustom, u)
	{
		memcpy(uid, u->uid, 16);
		this->x = x;
		ki = 0; // we're not yet logged in, so no KI can be set
		
		this->authResult = authResult;
		memcpy(this->serverGuid, serverGuid, 8);
	}
	
	void tmAccountAutheticated::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putByte(authResult); // auth result
		t.write(serverGuid, 8); // server guid
	}
	
	void tmAccountAutheticated::additionalFields()
	{
		dbg.nl();
		dbg.printf(" auth result: 0x%02X (%s), server guid: %s", authResult, alcUnetGetAuthCode(authResult), alcGetStrGuid(serverGuid));
	}
	
	//// tmSetMyActivePlayer
	tmSetMyActivePlayer::tmSetMyActivePlayer(tNetSession *u) : tmMsgBase(u) // it's not capable of sending
	{
		avatar.setVersion(0); // normal UruString
	}
	
	void tmSetMyActivePlayer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing")); // VaultManager sends it without X
		t.get(avatar);
		Byte unk = t.getByte();
		if (unk != 0) {
			lerr->log("NetMsgSetMyActivePlayer.unk is not null but %d\n", unk);
			throw txProtocolError(_WHERE("NetMsgSetMyActivePlayer.unk is not 0"));
		}
	}
	
	void tmSetMyActivePlayer::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" avatar: %s", avatar.c_str());
	}
	
	//// tmActivePlayerSet
	tmActivePlayerSet::tmActivePlayerSet(tNetSession *u, U32 x) : tmMsgBase(NetMsgActivePlayerSet, plNetAck | plNetCustom | plNetKi | plNetX, u)
	{
		this->x = x;
		ki = u->ki;
		if (u->tpots == 2) // if we are sure it is UU, use that value
			cmd = NetMsgActivePlayerSet_UU;
	}
	
	//// tmFindAge
	tmFindAge::tmFindAge(tNetSession *u) : tmMsgBase(u) // it's not capable of sending
	 { }
	
	void tmFindAge::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		// store the whole message
		message.clear();
		U32 remaining = t.remaining();
		message.write(t.read(), remaining);
	}
	
	//// tmFindAgeReply
	tmFindAgeReply::tmFindAgeReply(tNetSession *u, U32 x, tUStr &ipStr, U16 port, tUStr &age, const Byte *guid)
	 : tmMsgBase(NetMsgFindAgeReply, plNetAck | plNetCustom | plNetKi | plNetX, u), age(age), ipStr(ipStr)
	{
		this->x = x;
		ki = u->ki;
	
		this->ipStr.setVersion(0); // normal UruString
		this->serverPort = port;
		this->age.setVersion(0); // normal UruString
		memcpy(serverGuid, guid, 8);
	}
	
	void tmFindAgeReply::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putByte(0x1F); // seems to be some response code
		t.put(age);
		t.putByte(KGame); // server type
		t.put(ipStr);
		t.putU16(serverPort);
		t.write(serverGuid, 8);
	}
	
	void tmFindAgeReply::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Age filename: %s, IP: %s, Port: %d, GUID: %s", age.c_str(), ipStr.c_str(), serverPort, alcGetStrGuid(serverGuid));
	}

} //end namespace alc
