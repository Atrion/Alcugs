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
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *u, U32 x, U32 ip, U16 port, const Byte *login, const Byte *challenge, const Byte *hash, Byte release)
	: tmMsgBase(NetMsgCustomAuthAsk, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP, u)
	{
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetIP);
#endif
		this->x = x; // this is the SID the lobby uses for the connection to the client to be authed
		this->ip = ip; // the client's IP and Port (for logging)
		this->port = port;
		
		this->login.setVersion(0); // normal UrurString
		this->login.writeStr(login);
		memcpy(this->challenge, challenge, 16);
		memcpy(this->hash, hash, 16);
		this->release = release;
	}
	
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *u)
	: tmMsgBase(NetMsgCustomAuthAsk, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP, u)
	{
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetIP);
#endif
		login.setVersion(0); // normal UrurString
	}
	
	void tmCustomAuthAsk::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX)) throw txProtocolError(_WHERE("X flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetIP)) throw txProtocolError(_WHERE("IP flag missing"));
#endif
		t.get(login);
		memcpy(challenge, t.read(16), 16);
		memcpy(hash, t.read(16), 16);
		release = t.getByte();
#ifdef ENABLE_UNET2
		if(!hasFlags(plNetIP)) {
			ip = t.getU32();
			port = 0;
			if (u) u->proto = 1; // unet2 protocol
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
		// use two printf commands as alcGetStrGuid uses a static array and when using one command it would seems as if challenge and hash would be the same
		dbg.printf(" login: %s, challenge: %s, ", login.c_str(), alcGetStrUid(challenge));
		dbg.printf("hash: %s, build: 0x%02X (%s)", alcGetStrUid(hash), release, alcUnetGetRelease(release));
	}
	
	//// tmCustomAuthResponse
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *u, tmCustomAuthAsk &authAsk, const Byte *uid, const Byte *passwd, Byte result, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomAuthResponse, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP | plNetUID, u)
	 {
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetIP | plNetUID);
#endif
		// copy stuff from the authAsk
		x = authAsk.x; // this is the SID the lobby uses for the connection to the client to be authed
		ip = authAsk.ip; // the client's IP and Port (for finding the correct session)
		port = authAsk.port;
		login = authAsk.login;
		login.setVersion(0); // normal UrurString
		
		memcpy(this->uid, uid, 16);
		this->passwd = passwd;
		this->passwd.setVersion(0); // normal UrurString
		this->result = result;
		this->accessLevel = accessLevel;
	}
	
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *u)
	: tmMsgBase(NetMsgCustomAuthResponse, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP | plNetUID, u)
	{
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetIP | plNetUID);
#endif
		login.setVersion(0); // normal UrurString
		passwd.setVersion(0); // normal UrurString
	}
	
	void tmCustomAuthResponse::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX)) throw txProtocolError(_WHERE("X flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetIP | plNetUID)) throw txProtocolError(_WHERE("IP or UID flag missing"));
#endif
		t.get(login);
		result = t.getByte();
		t.get(passwd);
#ifdef ENABLE_UNET2
		if (!hasFlags(plNetUID)) {
			memcpy(uid, t.read(16), 16);
			ip = port = 0; // they should be initialized
			if (u) u->proto = 1; // unet2 protocol
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
