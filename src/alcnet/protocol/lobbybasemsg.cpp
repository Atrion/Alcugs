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
		if (!hasFlags(plNetX | plNetKi | plNetVersion)) throw txProtocolError(_WHERE("X, KI or Version flag missing"));
		t.get(account);
		maxPacketSize = t.getU16();
		release = t.getByte();
	}
	
	void tmAuthenticateHello::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" account: %s, max packet size: %d, release: %02X (%s)", account.c_str(), maxPacketSize, release, alcUnetGetRelease(release));
	}
	
	//// tmAuthenticateChallenge	
	tmAuthenticateChallenge::tmAuthenticateChallenge(tNetSession *u, Byte authResult, const Byte *challenge)
	: tmMsgBase(NetMsgAuthenticateChallenge, plNetKi | plNetAck | plNetX | plNetVersion | plNetCustom, u)
	{
		// copy stuff from the packet we're answering to
		ki = u->ki;
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
		dbg.printf(" auth result: %02X (%s), challenge: %s", authResult, alcUnetGetAuthCode(authResult), alcGetStrGuid(challenge.readAll(), 16));
	}
	
	//// tmAuthenticateResponse
	tmAuthenticateResponse::tmAuthenticateResponse(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending are set
	{
		hash.setVersion(0); // normal UruString, but in Hex, not Ascii
	}
	
	void tmAuthenticateResponse::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) {
			x = ki = 0; // the vault manager sends these without X and KI
		}
		t.get(hash);
	}
	
	void tmAuthenticateResponse::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" hash: %s", alcGetStrGuid(hash.readAll(), hash.size()));
	}
	
	//// tmAccountAutheticated	
	tmAccountAutheticated::tmAccountAutheticated(tNetSession *u, Byte authResult, const Byte *serverGuid)
	: tmMsgBase(NetMsgAccountAuthenticated, plNetKi | plNetAck | plNetX | plNetGUI | plNetCustom, u)
	{
		memcpy(guid, u->guid, 16);
		this->x = u->getX();
		this->ki = u->getKI();
		
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
		dbg.printf(" auth result: %02X (%s), server guid: %s", authResult, alcUnetGetAuthCode(authResult), alcGetStrGuid(serverGuid, 8));
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
		Byte lastByte = t.getByte();
		if (lastByte != 0) {
			lerr->log("ERR: Got a NetMsgSetMyActivePlayer where the last byte was not 0x00. Kicking the player.");
			throw txProtocolError(_WHERE("Last Byte of NetMsgSetMyActivePlayer was not 0x00"));
		}
	}
	
	void tmSetMyActivePlayer::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" avatar: %s", avatar.c_str());
	}

} //end namespace alc
