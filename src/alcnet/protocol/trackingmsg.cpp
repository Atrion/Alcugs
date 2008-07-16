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
		serverGuid.setVersion(5); // inverted UruString
		age.setVersion(0); // normal UrurString
		externalIp.setVersion(0); // normal UrurString
	}
	
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u, const Byte *serverGuid, const Byte *age, const Byte *externalIp)
	 : tmMsgBase(NetMsgCustomSetGuid, plNetAck | plNetVersion | plNetCustom, u)
	{
		this->serverGuid.setVersion(5); // inverted UrurString
		this->serverGuid.writeStr(serverGuid);
		this->age.setVersion(0); // normal UrurString
		this->age.writeStr(age);
		this->externalIp.setVersion(0); // normal UrurString
		this->externalIp.writeStr(externalIp);
	}
	
	void tmCustomSetGuid::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.get(serverGuid);
		t.get(age);
		t.get(externalIp);
#ifdef ENABLE_UNET2
		if (!t.eof()) { // if there's still something to read, there's a netmask before the external IP (unet2 protocol)
			t.get(externalIp);
			if (u) u->proto = 1; // unet2 protocol
		}
#endif
	}
	
	void tmCustomSetGuid::stream(tBBuf &t)
	{
		if (u->proto == 1 || u->proto == 2) { // I don't know why, but old servers have this set
			setFlags(plNetX | plNetKi);
			x = ki = 0;
		}
		
		tmMsgBase::stream(t);
		t.put(serverGuid);
		t.put(age);
#ifdef ENABLE_UNET2
		if (u->proto == 1) {
			tUStr netmask(0); // normal UruString
			netmask.writeStr("255.255.255.0");
			t.put(netmask);
		}
#endif
		t.put(externalIp);
	}
	
	void tmCustomSetGuid::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Server GUID: %s, Age filename: %s, external IP: %s", serverGuid.c_str(), age.c_str(), externalIp.c_str());
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf(" (unet2 protocol)");
#endif
	}
	
	//// tmCustomPlayerStatus
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u)
	 : tmMsgBase(NetMsgCustomPlayerStatus, plNetAck | plNetVersion | plNetCustom | plNetX | plNetKi | plNetUID, u)
	{
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetUID);
#endif
		account.setVersion(0); // normal UrurString
		avatar.setVersion(0); // normal UrurString
	}
	
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u, U32 ki, U32 x, const Byte *uid, const Byte *account, const Byte *avatar, Byte playerFlag, Byte playerStatus)
	 : tmMsgBase(NetMsgCustomPlayerStatus, plNetAck | plNetVersion | plNetCustom | plNetX | plNetKi | plNetUID, u)
	{
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetUID);
#endif
		this->x = x;
		this->ki = ki;
		memcpy(this->uid, uid, 16);
		
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
		if (!hasFlags(plNetUID)) throw txProtocolError(_WHERE("UID flag missing"));
#else
		if (!hasFlags(plNetUID)) memcpy(uid, t.read(16), 16);
#endif
		t.get(account);
		t.get(avatar);
		playerFlag = t.getByte();
		playerStatus = t.getByte();
	}
	
	void tmCustomPlayerStatus::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); } // UID (only for old protocol, the new one sends it in the header)
#endif
		t.put(account);
		t.put(avatar);
		t.putByte(playerFlag);
		t.putByte(playerStatus);
	}
	
	void tmCustomPlayerStatus::additionalFields()
	{
		dbg.nl();
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf(" UID (unet2 protocol): %s,", alcGetStrUid(uid));
#endif
		dbg.printf(" Account: %s, Avatar: %s, Flag: 0x%02X, Status: 0x%02X (%s)", account.c_str(), avatar.c_str(), playerFlag, playerStatus, alcUnetGetReasonCode(playerStatus));
	}
	
	//// tmCustomFindServer
	tmCustomFindServer::tmCustomFindServer(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		serverGuid.setVersion(5); // inverted UruString
		age.setVersion(0); // normal UrurString
	}
	
	void tmCustomFindServer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetIP)) throw txProtocolError(_WHERE("IP flag missing"));
#endif
		t.get(serverGuid);
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
		dbg.printf(" Server GUID: %s, Age filename: %s", serverGuid.c_str(), age.c_str());
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf(", IP (unet2 protocol): %s,", alcGetStrIp(ip));
#endif
	}
	
	//// tmCustomForkServer
	tmCustomForkServer::tmCustomForkServer(tNetSession *u, U32 ki, U32 x, U16 port, const Byte *serverGuid, const Byte *name, bool loadSDL)
	: tmMsgBase(NetMsgCustomForkServer, plNetAck | plNetCustom | plNetX | plNetKi | plNetVersion, u)
	{
		this->x = x;
		this->ki = ki;
		
		fork_port = port;
		this->serverGuid.setVersion(5); // inverted UruString
		this->serverGuid.writeStr(serverGuid);
		age.writeStr(name);
		age.setVersion(0); // normal UruString
		this->loadSDL = loadSDL;
	}
	
	void tmCustomForkServer::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU16(fork_port);
		t.put(serverGuid);
		t.put(age);
		t.putByte(loadSDL);
	}
	
	void tmCustomForkServer::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Port: %d, Server GUID: %s, Age filename: %s, Load SDL state: ", fork_port, serverGuid.c_str(), age.c_str());
		if (loadSDL) dbg.printf("yes");
		else         dbg.printf("no");
	}
	
	//// tmCustomServerFound
	tmCustomServerFound::tmCustomServerFound(tNetSession *u, U32 ki, U32 x, U16 port, const Byte *ip_str, const Byte *serverGuid, const Byte *name)
	: tmMsgBase(NetMsgCustomServerFound, plNetAck | plNetCustom | plNetX | plNetKi | plNetVersion, u)
	{
		this->x = x;
		this->ki = ki;
		
		server_port = port;
		this->ip_str.writeStr(ip_str);
		this->ip_str.setVersion(0); // normal UruString
		this->serverGuid.setVersion(5); // inverted UruString
		this->serverGuid.writeStr(serverGuid);
		age.writeStr(name);
		age.setVersion(0); // normal UruString
	}
	
	void tmCustomServerFound::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU16(server_port);
		t.put(ip_str);
		t.put(serverGuid);
		t.put(age);
	}
	
	void tmCustomServerFound::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Port: %d, IP: %s, Server GUID: %s, Age filename: %s", server_port, ip_str.c_str(), serverGuid.c_str(), age.c_str());
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
	
	void tmCustomDirectedFwd::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		gameMessage.rewind();
		t.put(gameMessage);
	}

} //end namespace alc
