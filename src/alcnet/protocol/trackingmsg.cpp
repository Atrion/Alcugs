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

#include "alcnet.h"
#include "protocol/ext-protocol.h"

#include <alcdebug.h>

namespace alc {

	//// tmCustomSetGuid
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u, const char *serverGuid, const char *age, const char *externalIp, U16 spawnStart, U16 spawnStop)
	 : tmMsgBase(NetMsgCustomSetGuid, plNetAck | plNetVersion, u), serverGuid(serverGuid), age(age), externalIp(externalIp)
	{
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
		spawnStart = t.getU16();
		spawnStop = t.getU16();
	}
	
	void tmCustomSetGuid::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(serverGuid);
		t.put(age);
		t.put(externalIp);
		t.putU16(spawnStart);
		t.putU16(spawnStop);
	}
	
	tString tmCustomSetGuid::str() const
	{
		tString dbg = tmMsgBase::str();
		dbg.nl();
		dbg.printf(" Server GUID: %s, Age filename: %s, external IP: %s", serverGuid.c_str(), age.c_str(), externalIp.c_str());
		dbg.printf(", Spawn Start: %d, Spawn Stop: %d", spawnStart, spawnStop);
		return dbg;
	}
	
	//// tmCustomPlayerStatus
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u, tNetSession *playerSession, Byte playerFlag, Byte playerStatus)
	 : tmMsgBase(NetMsgCustomPlayerStatus, plNetAck | plNetVersion | plNetKi | plNetUID | plNetSid, u), account(playerSession->name), avatar(playerSession->avatar)
	{
		this->sid = playerSession->getSid();
		this->ki = playerSession->ki;
		memcpy(this->uid, playerSession->uid, 16);
		
		this->playerFlag = playerFlag;
		this->playerStatus = playerStatus;
	}
	
	void tmCustomPlayerStatus::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("KI, UID or Sid flag missing"));
		
		t.get(account);
		t.get(avatar);
		playerFlag = t.getByte();
		playerStatus = t.getByte();
	}
	
	void tmCustomPlayerStatus::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(account);
		t.put(avatar);
		t.putByte(playerFlag);
		t.putByte(playerStatus);
	}
	
	tString tmCustomPlayerStatus::str() const
	{
		tString dbg = tmMsgBase::str();
		dbg.nl();
		dbg.printf(" Account: %s, Avatar: %s, Flag: 0x%02X, Status: 0x%02X (%s)", account.c_str(), avatar.c_str(), playerFlag, playerStatus, alcUnetGetReasonCode(playerStatus));
		return dbg;
	}
	
	//// tmCustomFindServer
	tmCustomFindServer::tmCustomFindServer(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomFindServer::tmCustomFindServer(tNetSession *u, const tmCustomVaultFindAge &findAge, const char *serverGuid, const tString &age)
	 : tmMsgBase(NetMsgCustomFindServer, plNetX | plNetKi | plNetAck | plNetSid, u), serverGuid(serverGuid), age(age)
	{
		this->ki = findAge.ki;
		this->x = findAge.x;
		this->sid = findAge.sid;
	}
	
	tmCustomFindServer::tmCustomFindServer(tNetSession *u, const tmCustomFindServer &findServer)
	 : tmMsgBase(NetMsgCustomFindServer, plNetX | plNetKi | plNetAck | plNetSid, u), serverGuid(findServer.serverGuid), age(findServer.age)
	{
		this->ki = findServer.ki;
		this->x = findServer.x;
		this->sid = findServer.sid;
	}
	
	void tmCustomFindServer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetSid)) throw txProtocolError(_WHERE("X, KI or Sid flag missing"));
		
		t.get(serverGuid);
		if (serverGuid.size() != 16) throw txProtocolError(_WHERE("NetMsgCustomFindServer.serverGuid must be 16 characters long"));
		t.get(age);
	}
	
	void tmCustomFindServer::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(serverGuid);
		t.put(age);
	}
	
	tString tmCustomFindServer::str() const
	{
		tString dbg = tmMsgBase::str();
		dbg.nl();
		dbg.printf(" Server GUID: %s, Age filename: %s", serverGuid.c_str(), age.c_str());
		return dbg;
	}
	
	//// tmCustomForkServer
	tmCustomForkServer::tmCustomForkServer(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomForkServer::tmCustomForkServer(tNetSession *u, U16 port, const char *serverGuid, const char *name)
	: tmMsgBase(NetMsgCustomForkServer, plNetAck | plNetVersion, u), serverGuid(serverGuid), age(name)
	{
		this->x = 0;
		this->ki = 0;
		
		forkPort = port;
	}
	
	void tmCustomForkServer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		forkPort = t.getU16();
		t.get(serverGuid);
		if (serverGuid.size() != 16) throw txProtocolError(_WHERE("NetMsgCustomForkServer.serverGuid must be 16 characters long"));
		t.get(age);
	}
	
	void tmCustomForkServer::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.putU16(forkPort);
		t.put(serverGuid);
		t.put(age);
	}
	
	tString tmCustomForkServer::str() const
	{
		tString dbg = tmMsgBase::str();
		dbg.nl();
		dbg.printf(" Port: %d, Server GUID: %s, Age filename: %s", forkPort, serverGuid.c_str(), age.c_str());
		return dbg;
	}
	
	//// tmCustomServerFound
	tmCustomServerFound::tmCustomServerFound(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomServerFound::tmCustomServerFound(tNetSession *u, U32 ki, U32 x, U32 sid, U16 port, const char *ipStr, const char *serverGuid, const char *name)
	: tmMsgBase(NetMsgCustomServerFound, plNetAck | plNetX | plNetKi | plNetVersion | plNetSid, u), ipStr(ipStr), serverGuid(serverGuid), age(name)
	{
		this->x = x;
		this->ki = ki;
		this->sid = sid;
		this->serverPort = port;
	}
	
	void tmCustomServerFound::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
		
		serverPort = t.getU16();
		t.get(ipStr);
		t.get(serverGuid);
		if (serverGuid.size() != 16) throw txProtocolError(_WHERE("NetMsgCustomServerFound.serverGuid must be 16 characters long"));
		t.get(age);
	}
	
	void tmCustomServerFound::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.putU16(serverPort);
		t.put(ipStr);
		t.put(serverGuid);
		t.put(age);
	}
	
	tString tmCustomServerFound::str() const
	{
		tString dbg = tmMsgBase::str();
		dbg.nl();
		dbg.printf(" Port: %d, IP: %s, Server GUID: %s, Age filename: %s", serverPort, ipStr.c_str(), serverGuid.c_str(), age.c_str());
		return dbg;
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
	tmCustomPlayerToCome::tmCustomPlayerToCome(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomPlayerToCome::tmCustomPlayerToCome(tNetSession *u, U32 ki)
	 : tmMsgBase(NetMsgCustomPlayerToCome, plNetAck | plNetVersion | plNetKi, u)
	{
		this->ki = ki;
	}
	
	void tmCustomPlayerToCome::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
	}

} //end namespace alc
