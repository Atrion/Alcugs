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
#include "unet.h"
#include "protocol/authmsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmCustomAuthAsk
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *s, U32 x, U32 ip, U16 port, Byte *login, Byte *challenge, Byte *hash, Byte release)
	: tmMsgBase(NetMsgCustomAuthAsk, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP, s)
	{
		if (s && s->proto == 1)
			unsetFlags(plNetIP);
		
		this->max_version = s->max_version;
		this->min_version = s->min_version;
		this->x = x;
		this->ip = ip;
		this->port = port;
		
		this->login.setVersion(0); // normal UrurString
		this->login.writeStr(login);
		memcpy(this->challenge, challenge, 16);
		memcpy(this->hash, hash, 16);
		this->release = release;
	}
	
	tmCustomAuthAsk::tmCustomAuthAsk(tNetSession *s)
	: tmMsgBase(NetMsgCustomAuthAsk, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP, s)
	{
		login.setVersion(0); // normal UrurString
	}
	
	void tmCustomAuthAsk::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.get(login);
		memcpy(challenge, t.read(16), 16);
		memcpy(hash, t.read(16), 16);
		release = t.getByte();
		if(!(flags & plNetIP)) {
			ip = t.getU32();
			port = 0;
			if (s) s->proto = 1; // unet2 protocol
		}
	}
	
	int tmCustomAuthAsk::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		off += t.put(login);
		t.write(challenge, 16); off += 16;
		t.write(hash, 16); off += 16;
		t.putByte(release); ++off;
		if (s && s->proto == 1) {
			t.putU32(ip); off += 4;
		}
		return off;
	}
	
	void tmCustomAuthAsk::additionalFields()
	{
		dbg.nl();
		if (s && s->proto == 1) dbg.printf(" ip (unet2 protocol): %s,", alcGetStrIp(ip));
		// use two printf commands as alcGetStrGuid uses a static array and when using one command it would seems as if challenge and hash would be the same
		dbg.printf(" login: %s, challenge: %s, ", login.c_str(), alcGetStrGuid(challenge, 16));
		dbg.printf("hash: %s, build: 0x%02X (%s)", alcGetStrGuid(hash, 16), release, alcUnetGetRelease(release));
	}
	
	//// tmCustomAuthResponse
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *s, tmCustomAuthAsk &authAsk, const Byte *guid, Byte *passwd, Byte result, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomAuthResponse, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP | plNetGUI, s)
	 {
	 	if (s && s->proto == 1)
			unsetFlags(plNetIP | plNetGUI);
		// copy stuff from the authAsk
		x = authAsk.x;
		min_version = authAsk.min_version;
		max_version = authAsk.max_version;
		ip = authAsk.ip; // the client's IP and Port (for logging)
		port = authAsk.port;
		login = authAsk.login;
		login.setVersion(0); // normal UrurString
		
		memcpy(this->guid, guid, 16);
		this->passwd = passwd;
		this->passwd.setVersion(0); // normal UrurString
		this->result = result;
		this->accessLevel = accessLevel;
	}
	
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *s)
	: tmMsgBase(NetMsgCustomAuthResponse, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP | plNetGUI, s)
	{
		login.setVersion(0); // normal UrurString
		passwd.setVersion(0); // normal UrurString
	}
	
	void tmCustomAuthResponse::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.get(login);
		result = t.getByte();
		t.get(passwd);
		if(!(flags & plNetGUI)) {
			memcpy(guid, t.read(16), 16);
			if (s) s->proto = 1; // unet2 protocol
		}
		accessLevel = t.getByte();
	}
	
	int tmCustomAuthResponse::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		off += t.put(login); // login
		t.putByte(result); ++off; // result
		off += t.put(passwd); // passwd
		if (s && s->proto == 1) { t.write(guid, 16); off += 16; } // GUID (only for old protocol, the new one sends it in the header)
		t.putByte(accessLevel); ++off; // acess level
		return off;
	}
	
	void tmCustomAuthResponse::additionalFields()
	{
		dbg.nl();
		if (s && s->proto == 1) dbg.printf(" guid (unet2 protocol): %s,", alcGetStrGuid(guid, 16));
		dbg.printf(" login: %s, passwd: (hidden), result: 0x%02X (%s), accessLevel: %d", login.c_str(), result, alcUnetGetAuthCode(result), accessLevel);
	}

} //end namespace alc
