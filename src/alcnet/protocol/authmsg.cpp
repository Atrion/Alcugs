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

	////IMPLEMENTATION
	
	void tmCustomAuthAsk::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.get(login);
		memcpy(challenge, t.read(16), 16);
		memcpy(hash, t.read(16), 16);
		release = t.getByte();
		if(!(flags & plNetIP)) {
			ip = t.getU32();
			if (s) s->proto = 1; // unet2 protocol
		}
	}
	
	void tmCustomAuthAsk::additionalFields()
	{
		dbg.nl();
		if (s && s->proto == 1) dbg.printf(" ip (unet2 protocol): %s,", alcGetStrIp(ip));
		dbg.printf(" login: %s, challenge: %s, hash: %s, build: 0x%02X (%s)", login.c_str(), alcGetStrGuid(challenge, 16), alcGetStrGuid(hash, 16), release, alcUnetGetRelease(release));
	}
	
	tmCustomAuthResponse::tmCustomAuthResponse(tNetSession *s, tmCustomAuthAsk &authAsk, const Byte *guid, Byte *passwd, Byte result, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomAuthResponse, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP | plNetGUI, s)
	 {
		// copy stuff from the authAsk
		x = authAsk.x;
		min_version = authAsk.min_version;
		max_version = authAsk.max_version;
		ip = authAsk.ip;
		port = authAsk.port;
		if (s && s->proto == 1)
			unsetFlags(plNetIP | plNetGUI);
		login = authAsk.login;
		
		memcpy(this->guid, guid, 16);
		this->passwd = passwd;
		this->result = result;
		this->accessLevel = accessLevel;
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
