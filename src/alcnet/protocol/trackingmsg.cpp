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
#define __U_TRACKINGMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"
#include "protocol/trackingmsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmCustomSetGuid
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		age.setVersion(0); // normal UrurString
	}
	
	void tmCustomSetGuid::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetVersion)) throw txProtocolError(_WHERE("Version flag missing"));
		// there's already a guid member in tmMsgBase, so let's use that (though we need only 8 bytes)
		tUStr guid_str(5); // inverted UruString
		t.get(guid_str);
		alcAscii2Hex(guid, (Byte *)guid_str.c_str(), 8);
		
		t.get(age);
#ifdef _UNET2_SUPPORT
		if (!t.eof()) {
			tUStr tmp;
			t.get(tmp); // these are both ignored (the first is netmask, the 2nd IP)
			t.get(tmp);
			if (u) u->proto = 1; // unet2 protocol
		}
#endif
	}
	
	void tmCustomSetGuid::additionalFields()
	{
		dbg.nl();
		dbg.printf(" GUID: %s, Age filename: %s", alcGetStrGuid(guid, 8), age.c_str());
		if (u && u->proto == 1) dbg.printf(" (unet2 protocol)");
	}
	
	//// tmCustomPlayerStatus
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		account.setVersion(0); // normal UrurString
		avatar.setVersion(0); // normal UrurString
	}
	
	void tmCustomPlayerStatus::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetVersion)) throw txProtocolError(_WHERE("X, KI or Version flag missing"));
		memcpy(guid, t.read(16), 16);
		t.get(account);
		t.get(avatar);
		playerFlag = t.getByte();
		playerStatus = t.getByte();
	}
	
	void tmCustomPlayerStatus::additionalFields()
	{
		dbg.nl();
		dbg.printf(" GUID: %s, Account: %s, Avatar: %s, Flag: 0x%02X, Status: 0x%02X (%s)", alcGetStrGuid(guid, 16), account.c_str(), avatar.c_str(), playerFlag, playerStatus, alcUnetGetReasonCode(playerStatus));
	}
	
	//// tmCustomFindServer
	tmCustomFindServer::tmCustomFindServer(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		age.setVersion(0); // normal UrurString
	}
	
	void tmCustomFindServer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetVersion)) throw txProtocolError(_WHERE("X, KI or Version flag missing"));
		// there's already a guid member in tmMsgBase, so let's use that (though we need only 8 bytes)
		tUStr guid_str(5); // inverted UruString
		t.get(guid_str);
		alcAscii2Hex(guid, (Byte *)guid_str.c_str(), 8);
		
		t.get(age);
		ip = t.getU32(); // use the tmMsgBase property
	}
	
	void tmCustomFindServer::additionalFields()
	{
		dbg.nl();
		dbg.printf(" GUID: %s, Age filename: %s, IP: %s", alcGetStrGuid(guid, 8), age.c_str(), alcGetStrIp(ip));
	}
	
	//// tmCustomForkServer
	tmCustomForkServer::tmCustomForkServer(tNetSession *u, U32 ki, U32 x, U16 port, const Byte *guid, const Byte *name, bool loadSDL)
	: tmMsgBase(NetMsgCustomForkServer, plNetAck | plNetCustom | plNetX | plNetKi | plNetVersion, u)
	{
		this->x = x;
		this->ki = ki;
		max_version = u->max_version;
		min_version = u->min_version;
		
		fork_port = port;
		memcpy(this->guid, guid, 8);
		age.writeStr(name);
		age.setVersion(0); // normal UruString
		this->loadSDL = loadSDL;
	}
	
	int tmCustomForkServer::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		t.putU16(fork_port); off += 2;
		
		tUStr guid_str(5);
		guid_str.writeStr(alcGetStrGuid(guid, 8));
		t.put(guid_str); off += 8;
		
		off += t.put(age);
		t.putByte(loadSDL); ++off;
		return off;
	}
	
	void tmCustomForkServer::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Port: %d, GUID: %s, Age filename: %s, Load SDL state: ", fork_port, alcGetStrGuid(guid, 8), age.c_str());
		if (loadSDL) dbg.printf("yes");
		else         dbg.printf("no");
	}
	
	//// tmCustomServerFound
	tmCustomServerFound::tmCustomServerFound(tNetSession *u, U32 ki, U32 x, U16 port, const Byte *ip_str, const Byte *guid, const Byte *name)
	: tmMsgBase(NetMsgCustomServerFound, plNetAck | plNetCustom | plNetX | plNetKi | plNetVersion, u)
	{
		this->x = x;
		this->ki = ki;
		max_version = u->max_version;
		min_version = u->min_version;
		
		server_port = port;
		this->ip_str.writeStr(ip_str);
		this->ip_str.setVersion(0); // normal UruString
		memcpy(this->guid, guid, 8);
		age.writeStr(name);
		age.setVersion(0); // normal UruString
	}
	
	int tmCustomServerFound::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		t.putU16(server_port); off += 2;
		off += t.put(ip_str);
		
		tUStr guid_str(5);
		guid_str.writeStr(alcGetStrGuid(guid, 8));
		t.put(guid_str); off += 8;
		
		off += t.put(age);
		return off;
	}
	
	void tmCustomServerFound::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Port: %d, IP: %s, GUID: %s, Age filename: %s", server_port, ip_str.c_str(), alcGetStrGuid(guid, 8), age.c_str());
	}
	
	////tmCustomDirectedFwd
	tmCustomDirectedFwd::tmCustomDirectedFwd(tNetSession *u)
	: tmMsgBase(NetMsgCustomDirectedFwd, plNetAck | plNetKi | plNetCustom | plNetVersion, u)
	{
		ki = 0;
		max_version = u->max_version;
		min_version = u->min_version;
	}
	
	tmCustomDirectedFwd::tmCustomDirectedFwd(tNetSession *u, tmCustomDirectedFwd &directedFwd)
	 : tmMsgBase(NetMsgCustomDirectedFwd, plNetAck | plNetKi | plNetCustom | plNetVersion, u), gameMessage(directedFwd.gameMessage),
	   recipients(directedFwd.recipients)
	{
		ki = directedFwd.ki;
		max_version = u->max_version;
		min_version = u->min_version;
	}
	
	void tmCustomDirectedFwd::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		t.read(5); // ignore the first bytes
		U32 gameMsgSize = t.getU32();
		gameMessage.write(t.read(gameMsgSize), gameMsgSize);
		t.read(1); // ignore 1 byte
		Byte n_recipients = t.getByte();
		recipients.write(t.read(n_recipients*4), n_recipients*4);
	}
	
	int tmCustomDirectedFwd::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		Byte zeros[5];
		memset(zeros, 0, 5);
		t.write(zeros, 5); off += 5; // 5 zero bytes
		t.putU32(gameMessage.size()); off += 4;
		off += t.put(gameMessage);
		t.write(zeros, 1); ++off; // 1 zero byte
		t.putByte(recipients.size()/4); ++off;
		off += t.put(recipients);
		return off;
	}

} //end namespace alc
