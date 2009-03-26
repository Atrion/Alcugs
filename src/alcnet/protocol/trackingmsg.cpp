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
#include "alcnet.h"
#include "protocol/trackingmsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmCustomSetGuid
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u, const char *serverGuid, const char *age, const char *externalIp, U16 spawnStart, U16 spawnStop)
	 : tmMsgBase(NetMsgCustomSetGuid, plNetAck | plNetVersion | plNetCustom, u), serverGuid(serverGuid), age(age), externalIp(externalIp)
	{
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) setFlags(plNetX | plNetKi); // older protocols have this set, but the value is ignored
#endif
		this->spawnStart = spawnStart;
		this->spawnStop = spawnStop;
	}
	
	void tmCustomSetGuid::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.get(serverGuid);
		if (serverGuid.size() != 16) throw txProtocolError(_WHERE("NetMsgCustomSetGuid.serverGuid must be 16 characters long"));
		t.get(age);
		t.get(externalIp);
		
		/* now comes some crazy protocol magic...
		unet2 format (X and KI flag set): saverGuid|age|netmask|externalIp
		unet3 (X and KI flag set): serverGuid|age|externalIp
		unet3+ (X and KI flag NOT set): serverGuid|age|spawnStart|spawnStop */
		if (!t.eof()) { // if there's still something to read, this is an unet2 or an unet3+ packet, depending on the flags
#ifdef ENABLE_UNET2
			if (hasFlags(plNetX | plNetKi)) { // there's a netmask before the external IP (unet2 protocol)
				t.get(externalIp);
				u->proto = 1; // unet2 protocol
				return;
			}
#endif
			// it's unet3+
			spawnStart = t.getU16();
			spawnStop = t.getU16();
		}
		else { // it is unet3
#ifdef ENABLE_UNET3
			u->proto = 2; // unet3 protocol
#else
			throw txProtocolError(_WHERE("unet3 protocol no longer supported"));
#endif
		}
	}
	
	void tmCustomSetGuid::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(serverGuid);
		t.put(age);
#ifdef ENABLE_UNET2
		if (u->proto == 1) {
			tStrBuf netmask;
			netmask.writeStr("255.255.255.0");
			t.put(netmask);
		}
#endif
		t.put(externalIp);
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) return;
#endif
		t.putU16(spawnStart);
		t.putU16(spawnStop);
	}
	
	void tmCustomSetGuid::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Server GUID: %s, Age filename: %s, external IP: %s", serverGuid.c_str(), age.c_str(), externalIp.c_str());
#ifdef ENABLE_UNET2
		if (u->proto == 1) dbg.printf(" (unet2 protocol)");
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 2) dbg.printf(" (unet3 protocol)");
		if (u->proto == 1 || u->proto == 2) return;
#endif
		dbg.printf(", Spawn Start: %d, Spawn Stop: %d", spawnStart, spawnStop);
	}
	
	//// tmCustomPlayerStatus
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u, U32 ki, U32 sid, const Byte *uid, const char *account, const char *avatar, Byte playerFlag, Byte playerStatus)
	 : tmMsgBase(NetMsgCustomPlayerStatus, plNetAck | plNetVersion | plNetCustom | plNetKi | plNetUID | plNetSid, u), account(account), avatar(avatar)
	{
		this->sid = sid;
		this->ki = ki;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetUID);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			setFlags(plNetX);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
		
		this->playerFlag = playerFlag;
		this->playerStatus = playerStatus;
	}
	
	void tmCustomPlayerStatus::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetUID)) throw txProtocolError(_WHERE("UID flag missing"));
#else
		if (!hasFlags(plNetUID)) memcpy(uid, t.read(16), 16);
#endif
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			if (!hasFlags(plNetX)) throw txProtocolError(_WHERE("X flag missing"));
			sid = x;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
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
		if (u->proto == 1) dbg.printf(" UID (unet2 protocol): %s,", alcGetStrUid(uid));
#endif
		dbg.printf(" Account: %s, Avatar: %s, Flag: 0x%02X, Status: 0x%02X (%s)", account.c_str(), avatar.c_str(), playerFlag, playerStatus, alcUnetGetReasonCode(playerStatus));
	}
	
	//// tmCustomFindServer
	tmCustomFindServer::tmCustomFindServer(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomFindServer::tmCustomFindServer(tNetSession *u, U32 ki, U32 x, U32 sid, U32 ip, U16 port, const char *serverGuid, const char *age)
	 : tmMsgBase(NetMsgCustomFindServer, plNetX | plNetKi | plNetAck | plNetCustom | plNetIP | plNetSid, u), serverGuid(serverGuid), age(age)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
		this->ip = ip;
		this->port = port;
#ifdef ENABLE_UNET2
		if (u->proto == 1)
			unsetFlags(plNetIP);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
	}
	
	void tmCustomFindServer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetIP)) throw txProtocolError(_WHERE("IP flag missing"));
#endif
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
		t.get(serverGuid);
		if (serverGuid.size() != 16) throw txProtocolError(_WHERE("NetMsgCustomFindServer.serverGuid must be 16 characters long"));
		t.get(age);
#ifdef ENABLE_UNET2
		if (!hasFlags(plNetIP)) {
			ip = t.getU32(); // use the tmMsgBase property
			port = 0;
		}
#endif
	}
	
	void tmCustomFindServer::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(serverGuid);
		t.put(age);
#ifdef ENABLE_UNET2
		if (u->proto == 1) t.putU32(ip);
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
	tmCustomForkServer::tmCustomForkServer(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomForkServer::tmCustomForkServer(tNetSession *u, U16 port, const char *serverGuid, const char *name, bool loadSDL)
	: tmMsgBase(NetMsgCustomForkServer, plNetAck | plNetCustom | plNetVersion, u), serverGuid(serverGuid), age(name)
	{
		this->x = 0;
		this->ki = 0;
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) setFlags(plNetX | plNetKi); // older protocols have this set, but the value is ignored
#endif
		
		forkPort = port;
		this->loadSDL = loadSDL;
	}
	
	void tmCustomForkServer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		forkPort = t.getU16();
		t.get(serverGuid);
		if (serverGuid.size() != 16) throw txProtocolError(_WHERE("NetMsgCustomForkServer.serverGuid must be 16 characters long"));
		t.get(age);
		loadSDL = t.getByte();
	}
	
	void tmCustomForkServer::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU16(forkPort);
		t.put(serverGuid);
		t.put(age);
		t.putByte(loadSDL);
	}
	
	void tmCustomForkServer::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Port: %d, Server GUID: %s, Age filename: %s, Load SDL state: ", forkPort, serverGuid.c_str(), age.c_str());
		dbg.printBoolean(loadSDL);
	}
	
	//// tmCustomServerFound
	tmCustomServerFound::tmCustomServerFound(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomServerFound::tmCustomServerFound(tNetSession *u, U32 ki, U32 x, U32 sid, U16 port, const char *ipStr, const char *serverGuid, const char *name)
	: tmMsgBase(NetMsgCustomServerFound, plNetAck | plNetCustom | plNetX | plNetKi | plNetVersion | plNetSid, u), ipStr(ipStr), serverGuid(serverGuid), age(name)
	{
		this->x = x;
		this->ki = ki;
		this->sid = sid;
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
		serverPort = port;
	}
	
	void tmCustomServerFound::store(tBBuf &t)
	{
		tmMsgBase::store(t);
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
		serverPort = t.getU16();
		t.get(ipStr);
		t.get(serverGuid);
		if (serverGuid.size() != 16) throw txProtocolError(_WHERE("NetMsgCustomServerFound.serverGuid must be 16 characters long"));
		t.get(age);
	}
	
	void tmCustomServerFound::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU16(serverPort);
		t.put(ipStr);
		t.put(serverGuid);
		t.put(age);
	}
	
	void tmCustomServerFound::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Port: %d, IP: %s, Server GUID: %s, Age filename: %s", serverPort, ipStr.c_str(), serverGuid.c_str(), age.c_str());
	}
	
	////tmCustomDirectedFwd
	tmCustomDirectedFwd::tmCustomDirectedFwd(tNetSession *u) : tmGameMessageDirected(u)
	{ }
	
	tmCustomDirectedFwd::tmCustomDirectedFwd(tNetSession *u, tmGameMessageDirected &msg)
	 : tmGameMessageDirected(NetMsgCustomDirectedFwd, u, msg)
	{ }
	
	tmCustomDirectedFwd::tmCustomDirectedFwd(tNetSession *u, U32 ki, tpObject *obj)
	 : tmGameMessageDirected(NetMsgCustomDirectedFwd, u, ki, obj)
	{ }
	
	//// tmCustomPlayerToCome
	tmCustomPlayerToCome::tmCustomPlayerToCome(tNetSession *u)
	 : tmMsgBase(NetMsgCustomPlayerToCome, plNetAck | plNetCustom | plNetVersion, u)
	{ }

} //end namespace alc
