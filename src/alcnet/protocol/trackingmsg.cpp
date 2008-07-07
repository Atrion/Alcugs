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
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u) : tmMsgBase(NetMsgCustomSetGuid, plNetAck | plNetVersion | plNetCustom, u)
	{
		age.setVersion(0); // normal UrurString
		externalIp.setVersion(0); // normal UrurString
	}
	
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u, const Byte *guid, const Byte *age, const Byte *externalIp)
	 : tmMsgBase(NetMsgCustomSetGuid, plNetAck | plNetVersion | plNetCustom, u)
	{
		memcpy(this->guid, guid, 8);
		this->age.setVersion(0); // normal UrurString
		this->age.writeStr(age);
		this->externalIp.setVersion(0); // normal UrurString
		this->externalIp.writeStr(externalIp);
	}
	
	void tmCustomSetGuid::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// there's already a guid member in tmMsgBase, so let's use that (though we need only 8 bytes)
		tUStr guid_str(5); // inverted UruString
		t.get(guid_str);
		alcAscii2Hex(guid, (Byte *)guid_str.c_str(), 8);
		
		t.get(age);
		t.get(externalIp);
#ifdef ENABLE_UNET2
		if (!t.eof()) { // if there's still something to read, there's a netmask before the external IP (unet2 protocol)
			t.get(externalIp);
			if (u) u->proto = 1; // unet2 protocol
		}
#endif
	}
	
	int tmCustomSetGuid::stream(tBBuf &t)
	{
		if (u->proto == 1 || u->proto == 2) { // I don't know why, but old servers have this set
			setFlags(plNetX | plNetKi);
			x = ki = 0;
		}
		
		int off = tmMsgBase::stream(t);
		tUStr guid_str(5); // inverted UruString
		guid_str.writeStr(alcGetStrGuid(guid, 8));
		off += t.put(guid_str);
		
		off += t.put(age);
#ifdef ENABLE_UNET2
		if (u->proto == 1) {
			tUStr netmask(0); // normal UruString
			netmask.writeStr("255.255.255.0");
			off += t.put(netmask);
		}
#endif
		off += t.put(externalIp);
		return off;
	}
	
	void tmCustomSetGuid::additionalFields()
	{
		dbg.nl();
		dbg.printf(" GUID: %s, Age filename: %s, external IP: %s", alcGetStrGuid(guid, 8), age.c_str(), externalIp.c_str());
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf(" (unet2 protocol)");
#endif
	}
	
	//// tmCustomPlayerStatus
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u)
	 : tmMsgBase(NetMsgCustomPlayerStatus, plNetAck | plNetVersion | plNetCustom | plNetX | plNetKi | plNetGUI, u)
	{
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetGUI);
#endif
		account.setVersion(0); // normal UrurString
		avatar.setVersion(0); // normal UrurString
	}
	
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u, U32 ki, U32 x, const Byte *guid, const Byte *account, const Byte *avatar, Byte playerFlag, Byte playerStatus)
	 : tmMsgBase(NetMsgCustomPlayerStatus, plNetAck | plNetVersion | plNetCustom | plNetX | plNetKi | plNetGUI, u)
	{
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetGUI);
#endif
		this->x = x;
		this->ki = ki;
		memcpy(this->guid, guid, 16);
		
		this->account.setVersion(0); // normal UrurString
		this->account.writeStr(account);
		this->avatar.setVersion(0); // normal UrurString
		this->avatar.writeStr(avatar);
		this->playerFlag = playerFlag;
		this->playerStatus = playerStatus;
	}
	
	void tmCustomPlayerStatus::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetGUI)) throw txProtocolError(_WHERE("GUID flag missing"));
#else
		if (!hasFlags(plNetGUI)) memcpy(guid, t.read(16), 16);
#endif
		t.get(account);
		t.get(avatar);
		playerFlag = t.getByte();
		playerStatus = t.getByte();
	}
	
	int tmCustomPlayerStatus::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(guid, 16); off += 16; } // GUID (only for old protocol, the new one sends it in the header)
#endif
		off += t.put(account);
		off += t.put(avatar);
		t.putByte(playerFlag); ++off;
		t.putByte(playerStatus); ++off;
		return off;
	}
	
	void tmCustomPlayerStatus::additionalFields()
	{
		dbg.nl();
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf(" GUID (unet2 protocol): %s,", alcGetStrGuid(guid, 16));
#endif
		dbg.printf(" Account: %s, Avatar: %s (%d), Flag: 0x%02X, Status: 0x%02X (%s)", account.c_str(), avatar.c_str(), avatar.isNull(), playerFlag, playerStatus, alcUnetGetReasonCode(playerStatus));
	}
	
	//// tmCustomFindServer
	tmCustomFindServer::tmCustomFindServer(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		age.setVersion(0); // normal UrurString
	}
	
	void tmCustomFindServer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetIP)) throw txProtocolError(_WHERE("IP flag missing"));
#endif
		// there's already a guid member in tmMsgBase, so let's use that (though we need only 8 bytes)
		tUStr guid_str(5); // inverted UruString
		t.get(guid_str);
		alcAscii2Hex(guid, (Byte *)guid_str.c_str(), 8);
		
		t.get(age);
#ifdef ENABLE_UNET2
		if (!hasFlags(plNetIP)) {
			ip = t.getU32(); // use the tmMsgBase property
			port = 0;
		}
#endif
	}
	
	void tmCustomFindServer::additionalFields()
	{
		dbg.nl();
		dbg.printf(" GUID: %s, Age filename: %s", alcGetStrGuid(guid, 8), age.c_str());
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf(", IP (unet2 protocol): %s,", alcGetStrIp(ip));
#endif
	}
	
	//// tmCustomForkServer
	tmCustomForkServer::tmCustomForkServer(tNetSession *u, U32 ki, U32 x, U16 port, const Byte *guid, const Byte *name, bool loadSDL)
	: tmMsgBase(NetMsgCustomForkServer, plNetAck | plNetCustom | plNetX | plNetKi | plNetVersion, u)
	{
		this->x = x;
		this->ki = ki;
		
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
		recipients = NULL;
		ki = 0;
	}
	
	tmCustomDirectedFwd::tmCustomDirectedFwd(tNetSession *u, tmCustomDirectedFwd &directedFwd)
	 : tmMsgBase(NetMsgCustomDirectedFwd, plNetAck | plNetKi | plNetCustom | plNetVersion, u), gameMessage(directedFwd.gameMessage)
	{
		recipients = NULL;
		ki = directedFwd.ki;
	}
	
	tmCustomDirectedFwd::~tmCustomDirectedFwd(void)
	{
		if (recipients) free(recipients);
	}
	
	void tmCustomDirectedFwd::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		// store the whole message
		gameMessage.clear();
		t.get(gameMessage);
		// now, verify (will throw a txOutOfRange when too small)
		gameMessage.rewind();
		gameMessage.read(5); // ignore the first bytes
		U32 gameMsgSize = gameMessage.getU32();
		gameMessage.read(gameMsgSize); // this is the message itself
		gameMessage.read(1); // ignore 1 byte
		// get list of recipients
		nRecipients = gameMessage.getByte();
		if (recipients) free(recipients);
		recipients = (U32 *)malloc(nRecipients*sizeof(U32));
		for (int i = 0; i < nRecipients; ++i) recipients[i] = gameMessage.getU32();
		if (!gameMessage.eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after the recipient list
	}
	
	int tmCustomDirectedFwd::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		gameMessage.rewind();
		off += t.put(gameMessage);
		return off;
	}

} //end namespace alc
