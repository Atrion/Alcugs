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
#include "unet.h"
#include "protocol/lobbybasemsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmAuthenticateHello
	tmAuthenticateHello::tmAuthenticateHello(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		account.setVersion(0); // normal UruString
	}
	
	void tmAuthenticateHello::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// the vault manager sends these without X and KI
		if (ki != 0) throw txProtocolError(_WHERE("KI must be 0 in NetMsgAuthenticateHello"));
		t.get(account);
		maxPacketSize = t.getU16();
		release = t.getByte();
		
		u->x = x;
	}
	
	void tmAuthenticateHello::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" account: %s, max packet size: %d, release: 0x%02X (%s)", account.c_str(), maxPacketSize, release, alcUnetGetRelease(release));
	}
	
	//// tmAuthenticateChallenge	
	tmAuthenticateChallenge::tmAuthenticateChallenge(tNetSession *u, Byte authResult, const Byte *challenge)
	: tmMsgBase(NetMsgAuthenticateChallenge, plNetKi | plNetAck | plNetX | plNetVersion | plNetCustom, u)
	{
		ki = 0; // we're not yet logged in, so no KI can be set
		x = u->x;
		
		this->authResult = authResult;
		this->challenge.write(challenge, 16);
		this->challenge.setVersion(0); // normal UruString, but in Hex, not Ascii
	}
	
	int tmAuthenticateChallenge::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		t.putByte(authResult); ++off; // auth result
		off += t.put(challenge); // challenge
		return off;
	}
	
	void tmAuthenticateChallenge::additionalFields()
	{
		dbg.nl();
		dbg.printf(" auth result: 0x%02X (%s), challenge: %s", authResult, alcUnetGetAuthCode(authResult), alcGetStrUid(challenge.readAll()));
	}
	
	//// tmAuthenticateResponse
	tmAuthenticateResponse::tmAuthenticateResponse(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending are set
	{
		hash.setVersion(0); // normal UruString, but in Hex, not Ascii
	}
	
	void tmAuthenticateResponse::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// the vault manager sends these without X and KI
		if (ki != 0) throw txProtocolError(_WHERE("KI must be 0 in NetMsgAuthenticateResponse"));
		t.get(hash);
		if (hash.size() != 16) throw txProtocolError(_WHERE("tmAuthenticateResponse.hash must be 16 byte long"));
		
		u->x = x;
	}
	
	void tmAuthenticateResponse::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" hash: %s", alcGetStrUid(hash.readAll()));
	}
	
	//// tmAccountAutheticated	
	tmAccountAutheticated::tmAccountAutheticated(tNetSession *u, Byte authResult, const Byte *serverGuid)
	: tmMsgBase(NetMsgAccountAuthenticated, plNetKi | plNetAck | plNetX | plNetUID | plNetCustom, u)
	{
		memcpy(uid, u->uid, 16);
		x = u->x;
		ki = 0; // we're not yet logged in, so no KI can be set
		
		this->authResult = authResult;
		memcpy(this->serverGuid, serverGuid, 8);
	}
	
	int tmAccountAutheticated::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		t.putByte(authResult); ++off; // auth result
		t.write(serverGuid, 8); off += 8; // server guid
		return off;
	}
	
	void tmAccountAutheticated::additionalFields()
	{
		dbg.nl();
		dbg.printf(" auth result: 0x%02X (%s), server guid: %s", authResult, alcUnetGetAuthCode(authResult), alcGetStrGuid(serverGuid));
	}
	
	//// tmSetMyActivePlayer
	tmSetMyActivePlayer::tmSetMyActivePlayer(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		avatar.setVersion(0); // normal UruString
	}
	
	void tmSetMyActivePlayer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		t.get(avatar);
		Byte unk = t.getByte();
		if (unk != 0) {
			lerr->log("NetMsgSetMyActivePlayer.unk is not null but %d\n", unk);
			throw txProtocolError(_WHERE("NetMsgSetMyActivePlayer.unk is not 0"));
		}
		
		u->x = x;
	}
	
	void tmSetMyActivePlayer::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" avatar: %s", avatar.c_str());
	}
	
	//// tmActivePlayerSet
	tmActivePlayerSet::tmActivePlayerSet(tNetSession *u) : tmMsgBase(NetMsgActivePlayerSet, plNetAck | plNetCustom | plNetKi | plNetX, u)
	{
		x = u->x;
		ki = u->ki;
		if (u->getTpots() != 2) // if it is TPOTS or we are unsure, use TPOTS mod
			cmd = NetMsgActivePlayerSet2;
	}

} //end namespace alc
