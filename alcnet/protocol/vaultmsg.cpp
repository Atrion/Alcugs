/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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
#include <alcdefs.h>
#include "vaultmsg.h"

#include "netexception.h"

#include <cstring>

namespace alc {

	//// tmVault
	tmVault::tmVault(tNetSession *u, uint32_t ki, uint32_t x, bool task, tStreamable *vaultMessage) : tmNetMsg(NetMsgVault, plNetAck | plNetKi, u)
	{
		message.put(*vaultMessage);
		this->ki = ki;
		this->x = x;
		if (task) {
			cmd = NetMsgVaultTask;
			flags |= plNetX;
		}
	}
	
	void tmVault::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		message.clear();
		size_t remaining = t.remaining();
		message.write(t.readAll(), remaining); // the rest is the message
	}
	
	void tmVault::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put(message);
	}
	
	tString tmVault::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Size of attached message: %Zd Bytes", message.size());
		return dbg;
	}
	
	//// tmCustomVaultAskPlayerList
	tmCustomVaultAskPlayerList::tmCustomVaultAskPlayerList(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid)
	: tmNetMsg(NetMsgCustomVaultAskPlayerList, plNetAck | plNetX | plNetVersion | plNetUID | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
	}
	
	void tmCustomVaultAskPlayerList::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, UID or Sid flag missing"));
	}
	
	//// tmCustomVaultPlayerList
	tmCustomVaultPlayerList::tmCustomVaultPlayerList(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid)
	: tmNetMsg(NetMsgCustomVaultPlayerList, plNetAck | plNetX | plNetVersion | plNetUID | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		numberPlayers = 0;
	}
	
	void tmCustomVaultPlayerList::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, UID or Sid flag missing"));
	
		numberPlayers = t.get16();
		players.clear();
		size_t remaining = t.remaining();
		players.write(t.readAll(), remaining); // the rest is the data about the players
	}
	
	void tmCustomVaultPlayerList::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put16(numberPlayers);
		t.put(players);
	}
	
	tString tmCustomVaultPlayerList::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" number of players: %d", numberPlayers);
		return dbg;
	}
	
	//// tmCustomVaultPlayerStatus
	tmCustomVaultPlayerStatus::tmCustomVaultPlayerStatus(tNetSession *u, uint32_t ki, const tString &serverGuid, const tString &age, uint8_t state, uint32_t onlineTime)
	 : tmNetMsg(NetMsgCustomVaultPlayerStatus, plNetAck | plNetVersion | plNetKi, u), age(age), serverGuid(serverGuid)
	{
		this->ki = ki;
		this->x = 0;
		this->state = state;
		this->onlineTime = onlineTime;
	}
	
	void tmCustomVaultPlayerStatus::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetKi))  throw txProtocolError(_WHERE("KI flag missing"));
		t.get(age);
		t.get(serverGuid);
		state = t.get8();
		onlineTime = t.get32();
	}
	
	void tmCustomVaultPlayerStatus::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put(age);
		t.put(serverGuid);
		t.put8(state);
		t.put32(onlineTime);
	}
	
	tString tmCustomVaultPlayerStatus::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Server GUID: %s, Age filename: %s, State: 0x%02X, online time: %d", serverGuid.c_str(), age.c_str(), state, onlineTime);
		return dbg;
	}
	
	//// tmCustomVaultCreatePlayer
	tmCustomVaultCreatePlayer::tmCustomVaultCreatePlayer(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid,
	  uint8_t accessLevel, const tString &login, const tString &avatar, const tString &gender, const tString &friendName, const tString &key)
	 : tmNetMsg(NetMsgCustomVaultCreatePlayer, plNetX | plNetUID | plNetVersion | plNetAck | plNetSid, u), login(login),
	   avatar(avatar), gender(gender), friendName(friendName), key(key)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		this->accessLevel = accessLevel;
	}
	
	void tmCustomVaultCreatePlayer::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, UID or Sid flag missing"));
		
		t.get(login);
		accessLevel = t.get8();
		t.get(avatar);
		t.get(gender);
		t.get(friendName);
		t.get(key);
		uint32_t unk = t.get32();
		if (unk != 0) throw txProtocolError(_WHERE("NetMsgCustomVaultCreatePlayer.unk must always be 0"));
	}
	
	void tmCustomVaultCreatePlayer::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put(login);
		t.put8(accessLevel);
		t.put(avatar);
		t.put(gender);
		t.put(friendName);
		t.put(key);
		t.put32(0);
	}
	
	tString tmCustomVaultCreatePlayer::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" login: %s, ", login.c_str());
		dbg.printf("accessLevel: %d, avatar: %s, gender: %s, friend: %s, key: %s", accessLevel, avatar.c_str(), gender.c_str(), friendName.c_str(), key.c_str());
		return dbg;
	}
	
	//// tmCustomVaultPlayerCreated
	tmCustomVaultPlayerCreated::tmCustomVaultPlayerCreated(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t result)
	 : tmNetMsg(NetMsgCustomVaultPlayerCreated, plNetKi | plNetX | plNetAck | plNetUID | plNetSid, u)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		this->result = result;
	}
	
	void tmCustomVaultPlayerCreated::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, KI, UID or Sid flag missing"));
		
		result = t.get8();
	}
	
	void tmCustomVaultPlayerCreated::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put8(result);
	}
	
	tString tmCustomVaultPlayerCreated::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" result: 0x%02X (%s)", result, alcUnetGetAvatarCode(result));
		return dbg;
	}
	
	//// tmCustomVaultDeletePlayer
	tmCustomVaultDeletePlayer::tmCustomVaultDeletePlayer(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t accessLevel)
	 : tmNetMsg(NetMsgCustomVaultDeletePlayer, plNetX | plNetKi | plNetUID | plNetAck | plNetVersion | plNetSid, u)
	{
		this->x = x;
		this->ki = ki;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		this->accessLevel = accessLevel;
	}
	
	void tmCustomVaultDeletePlayer::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, KI, UID or Sid flag missing"));
		accessLevel = t.get8();
	}
	
	void tmCustomVaultDeletePlayer::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put8(accessLevel);
	}
	
	tString tmCustomVaultDeletePlayer::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" access level: %d", accessLevel);
		return dbg;
	}
	
	//// tmCustomVaultCheckKi
	tmCustomVaultCheckKi::tmCustomVaultCheckKi(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid)
	 : tmNetMsg(NetMsgCustomVaultCheckKi, plNetX | plNetKi | plNetUID | plNetAck | plNetVersion | plNetSid, u)
	{
		this->x = x;
		this->ki = ki;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
	}
	
	void tmCustomVaultCheckKi::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, KI, UID or Sid flag missing"));
	}
	
	void tmCustomVaultCheckKi::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
	}
	
	//// tmCustomVaultKiChecked
	tmCustomVaultKiChecked::tmCustomVaultKiChecked(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t status, const tString &avatar)
	: tmNetMsg(NetMsgCustomVaultKiChecked, plNetX | plNetKi | plNetUID | plNetAck | plNetVersion | plNetSid, u), avatar(avatar)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		this->status = status;
	}
	
	void tmCustomVaultKiChecked::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, KI, UID or Sid flag missing"));
		
		status = t.get8();
		t.get(avatar);
	}
	
	void tmCustomVaultKiChecked::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put8(status);
		t.put(avatar);
	}
	
	tString tmCustomVaultKiChecked::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" status: 0x%02X, avatar: %s", status, avatar.c_str());
		return dbg;
	}
	
	//// tmCustomVaultFindAge
	tmCustomVaultFindAge::tmCustomVaultFindAge(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const tMBuf &data)
	 : tmNetMsg(NetMsgCustomVaultFindAge, plNetX | plNetKi | plNetAck | plNetSid, u), data(data)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
	}
	
	void tmCustomVaultFindAge::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetSid)) throw txProtocolError(_WHERE("X, KI or Sid flag missing"));
		// store the whole message
		data.clear();
		size_t remaining = t.remaining();
		data.write(t.readAll(), remaining);
	}
	
	void tmCustomVaultFindAge::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put(data);
	}
	
} //end namespace alc
