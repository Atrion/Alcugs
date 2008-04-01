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
#define __U_LOBBYMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"
#include "protocol/lobbymsg.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	void tmAuthHello::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.get(account);
		maxPacketSize = t.getU16();
		release = t.getByte();
	}
	
	void tmAuthHello::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" account: %s, max packet size: %d, release: %02X (%s)", account.str(), maxPacketSize, release, alcUnetGetRelease(release));
	}
	
	tmAuthChallenge::tmAuthChallenge(tNetSession *u, Byte authresult, tmAuthHello &msg)
	: tmMsgBase(NetMsgAuthenticateChallenge, plNetKi | plNetAck | plNetX | plNetVersion | plNetCustom, u)
	{
		// copy stuff from the packet we're answering to
		ki = msg.ki;
		x = msg.x;
		max_version = msg.max_version;
		min_version = msg.min_version;
		
		this->authresult = authresult;
		// init the challenge to the MD5 of the current system time and other garbage
		tMD5Buf md5buffer;
		md5buffer.putU32(alcGetTime());
		md5buffer.putU32(alcGetMicroseconds());
		srandom(alcGetTime());
		md5buffer.putU32(random());
		md5buffer.putU32(alcGetUptime().seconds);
		md5buffer.put(msg.account);
		md5buffer.putU32(alcGetBornTime().seconds);
		md5buffer.compute();
		memcpy(challenge, md5buffer.read(16), 16);
	}
	
	int tmAuthChallenge::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		t.putByte(authresult); ++off; // authresult
		t.write(challenge, 16); off += 16; // challenge
		return off;
	}
	
	void tmAuthChallenge::additionalFields()
	{
		Byte challenge_str[33];
		alcHex2Ascii(challenge_str, challenge, 16);
		dbg.nl();
		dbg.printf(" auth result: %02X (%s), challenge: %s", authresult, alcUnetGetAuthCode(authresult), challenge_str);
	}

} //end namespace alc
