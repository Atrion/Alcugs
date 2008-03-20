/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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

#include <alcugs.h>
#include <unet.h>

////extra includes
#include "authmsg.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	void tmAuthAsk::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.get(login);
		alcHex2Ascii(challenge, t.read(16), 16);
		alcHex2Ascii(hash, t.read(16), 16);
		release = t.getByte();
		if(!(flags & plNetIP)) {
			ip = t.getU32();
			if (u) u->proto = 1; // unet2 protocol
		}
	}
	
	Byte *tmAuthAsk::str()
	{
		#ifdef _UNET_MSGDBG_
		tmMsgBase::str();
		dbg.end();
		dbg.seek(-1);
		if (u && u->proto == 1) dbg.printf(" ip (unet2 protocol): %s,", alcGetStrIp(ip));
		dbg.printf(" login: %s,\n challenge: %s, hash: %s, build: %i (%s)", login.str(), challenge, hash, release, alcUnetGetRelease(release));
		dbg.putByte(0);
		dbg.rewind();
		return dbg.read();
		#else
		return tmMsgBase::str();
		#endif
	}
	
	tmAuthResponse::tmAuthResponse(tNetSession *u, tmAuthAsk &authAsk, Byte *guid, Byte *passwd, Byte result, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomAuthResponse, plNetAck | plNetCustom | plNetX | plNetVersion | plNetIP | plNetGUI, u) {
		// copy stuff from the authAsk
		x = authAsk.x;
		min_version = authAsk.min_version;
		max_version = authAsk.max_version;
		ip = authAsk.ip;
		port = authAsk.port;
		if (u && u->proto == 1)
			unsetFlags(plNetIP | plNetGUI);
		login.set(authAsk.login.str());
		
		memcpy(this->guid, alcGetHexUid(guid), 16);
		this->passwd.set(passwd);
		this->result = result;
		this->accessLevel = accessLevel;
	}
	
	int tmAuthResponse::stream(tBBuf &t) {
		int off;
		off = tmMsgBase::stream(t);
		off += t.put(login); // login
		t.putByte(result); ++off; // result
		off += t.put(passwd); // passwd
		if (u && u->proto == 1) { t.write(guid, 16); off += 16; } // GUID (only for old protocol)
		t.putByte(accessLevel); ++off; // acess level
		return off;
	}
	
	Byte *tmAuthResponse::str()
	{
		#ifdef _UNET_MSGDBG_
		tmMsgBase::str();
		dbg.end();
		dbg.seek(-1);
		if (u && u->proto == 1) dbg.printf(" guid (unet2 protocol): %s,", alcGetStrGuid(guid));
		dbg.printf(" login: %s, passwd: %s, result: %d (%s), accessLevel: %d", login.str(), passwd.str(), result, alcUnetGetAuthCode(result), accessLevel);
		dbg.putByte(0);
		dbg.rewind();
		return dbg.read();
		#else
		return tmMsgBase::str();
		#endif
	}

} //end namespace alc
