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
#include <alcdefs.h>
#include "vaultmsg.h"

#include "netexception.h"

#include <cstring>

namespace alc {

	//// tmVault
	tmVault::tmVault(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmVault::tmVault(tNetSession *u, U32 ki, U32 x, bool task, tBaseType *vaultMessage) : tmMsgBase(NetMsgVault, plNetAck | plNetKi, u)
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
		tmMsgBase::store(t);
		message.clear();
		U32 remaining = t.remaining();
		message.write(t.readAll(), remaining); // the rest is the message
	}
	
	void tmVault::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(message);
	}
	
	tString tmVault::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Size of attached message: %d Bytes", message.size());
		return dbg;
	}
	
	//// tmCustomVaultAskPlayerList
	tmCustomVaultAskPlayerList::tmCustomVaultAskPlayerList(tNetSession *u, U32 x, U32 sid, const Byte *uid)
	: tmMsgBase(NetMsgCustomVaultAskPlayerList, plNetAck | plNetX | plNetVersion | plNetUID | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
	}
	
	tmCustomVaultAskPlayerList::tmCustomVaultAskPlayerList(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmCustomVaultAskPlayerList::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, UID or Sid flag missing"));
	}
	
	//// tmCustomVaultPlayerList
	tmCustomVaultPlayerList::tmCustomVaultPlayerList(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomVaultPlayerList::tmCustomVaultPlayerList(tNetSession *u, U32 x, U32 sid, const Byte *uid)
	: tmMsgBase(NetMsgCustomVaultPlayerList, plNetAck | plNetX | plNetVersion | plNetUID | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		numberPlayers = 0;
	}
	
	void tmCustomVaultPlayerList::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, UID or Sid flag missing"));
	
		numberPlayers = t.getU16();
		players.clear();
		U32 remaining = t.remaining();
		players.write(t.readAll(), remaining); // the rest is the data about the players
	}
	
	void tmCustomVaultPlayerList::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.putU16(numberPlayers);
		t.put(players);
	}
	
	tString tmCustomVaultPlayerList::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" number of players: %d", numberPlayers);
		return dbg;
	}
	
	//// tmCustomVaultPlayerStatus
	tmCustomVaultPlayerStatus::tmCustomVaultPlayerStatus(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomVaultPlayerStatus::tmCustomVaultPlayerStatus(tNetSession *u, U32 ki, const tString &serverGuid, const tString &age, Byte state, U32 onlineTime)
	 : tmMsgBase(NetMsgCustomVaultPlayerStatus, plNetAck | plNetVersion | plNetKi, u), age(age), serverGuid(serverGuid)
	{
		this->ki = ki;
		this->x = 0;
		this->state = state;
		this->onlineTime = onlineTime;
	}
	
	void tmCustomVaultPlayerStatus::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi))  throw txProtocolError(_WHERE("KI flag missing"));
		t.get(age);
		t.get(serverGuid);
		state = t.getByte();
		onlineTime = t.getU32();
	}
	
	void tmCustomVaultPlayerStatus::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(age);
		t.put(serverGuid);
		t.putByte(state);
		t.putU32(onlineTime);
	}
	
	tString tmCustomVaultPlayerStatus::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Server GUID: %s, Age filename: %s, State: 0x%02X, online time: %d", serverGuid.c_str(), age.c_str(), state, onlineTime);
		return dbg;
	}
	
	//// tmCustomVaultCreatePlayer
	tmCustomVaultCreatePlayer::tmCustomVaultCreatePlayer(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomVaultCreatePlayer::tmCustomVaultCreatePlayer(tNetSession *u, U32 x, U32 sid, const Byte *uid,
	  Byte accessLevel, const tString &login, const tString &avatar, const tString &gender, const tString &friendName, const tString &key)
	 : tmMsgBase(NetMsgCustomVaultCreatePlayer, plNetX | plNetUID | plNetVersion | plNetAck | plNetSid, u), login(login),
	   avatar(avatar), gender(gender), friendName(friendName), key(key)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		this->accessLevel = accessLevel;
	}
	
	void tmCustomVaultCreatePlayer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, UID or Sid flag missing"));
		
		t.get(login);
		accessLevel = t.getByte();
		t.get(avatar);
		t.get(gender);
		t.get(friendName);
		t.get(key);
		U32 unk = t.getU32();
		if (unk != 0) throw txProtocolError(_WHERE("NetMsgCustomVaultCreatePlayer.unk must always be 0"));
	}
	
	void tmCustomVaultCreatePlayer::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(login);
		t.putByte(accessLevel);
		t.put(avatar);
		t.put(gender);
		t.put(friendName);
		t.put(key);
		t.putU32(0);
	}
	
	tString tmCustomVaultCreatePlayer::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" login: %s, ", login.c_str());
		dbg.printf("accessLevel: %d, avatar: %s, gender: %s, friend: %s, key: %s", accessLevel, avatar.c_str(), gender.c_str(), friendName.c_str(), key.c_str());
		return dbg;
	}
	
	//// tmCustomVaultPlayerCreated
	tmCustomVaultPlayerCreated::tmCustomVaultPlayerCreated(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomVaultPlayerCreated::tmCustomVaultPlayerCreated(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid, Byte result)
	 : tmMsgBase(NetMsgCustomVaultPlayerCreated, plNetKi | plNetX | plNetAck | plNetUID | plNetSid, u)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		this->result = result;
	}
	
	void tmCustomVaultPlayerCreated::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, KI, UID or Sid flag missing"));
		
		result = t.getByte();
	}
	
	void tmCustomVaultPlayerCreated::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.putByte(result);
	}
	
	tString tmCustomVaultPlayerCreated::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" result: 0x%02X (%s)", result, alcUnetGetAvatarCode(result));
		return dbg;
	}
	
	//// tmCustomVaultDeletePlayer
	tmCustomVaultDeletePlayer::tmCustomVaultDeletePlayer(tNetSession *u)
	 : tmMsgBase(NetMsgCustomVaultDeletePlayer, plNetX | plNetKi | plNetUID | plNetAck | plNetVersion, u)
	{ }
	
	tmCustomVaultDeletePlayer::tmCustomVaultDeletePlayer(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomVaultDeletePlayer, plNetX | plNetKi | plNetUID | plNetAck | plNetVersion | plNetSid, u)
	{
		this->x = x;
		this->ki = ki;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		this->accessLevel = accessLevel;
	}
	
	void tmCustomVaultDeletePlayer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, KI, UID or Sid flag missing"));
		accessLevel = t.getByte();
	}
	
	void tmCustomVaultDeletePlayer::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.putByte(accessLevel);
	}
	
	tString tmCustomVaultDeletePlayer::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" access level: %d", accessLevel);
		return dbg;
	}
	
	//// tmCustomVaultCheckKi
	tmCustomVaultCheckKi::tmCustomVaultCheckKi(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid)
	 : tmMsgBase(NetMsgCustomVaultCheckKi, plNetX | plNetKi | plNetUID | plNetAck | plNetVersion | plNetSid, u)
	{
		this->x = x;
		this->ki = ki;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
	}
	
	tmCustomVaultCheckKi::tmCustomVaultCheckKi(tNetSession *u)
	 : tmMsgBase(NetMsgCustomVaultCheckKi, plNetX | plNetKi | plNetUID | plNetAck | plNetVersion, u)
	{ }

	void tmCustomVaultCheckKi::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, KI, UID or Sid flag missing"));
	}
	
	void tmCustomVaultCheckKi::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
	}
	
	//// tmCustomVaultKiChecked
	tmCustomVaultKiChecked::tmCustomVaultKiChecked(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid, Byte status, const tString &avatar)
	: tmMsgBase(NetMsgCustomVaultKiChecked, plNetX | plNetKi | plNetUID | plNetAck | plNetVersion | plNetSid, u), avatar(avatar)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		this->status = status;
	}
	
	tmCustomVaultKiChecked::tmCustomVaultKiChecked(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmCustomVaultKiChecked::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetUID | plNetSid)) throw txProtocolError(_WHERE("X, KI, UID or Sid flag missing"));
		
		status = t.getByte();
		t.get(avatar);
	}
	
	void tmCustomVaultKiChecked::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.putByte(status);
		t.put(avatar);
	}
	
	tString tmCustomVaultKiChecked::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" status: 0x%02X, avatar: %s", status, avatar.c_str());
		return dbg;
	}
	
	//// tmCustomVaultFindAge
	tmCustomVaultFindAge::tmCustomVaultFindAge(tNetSession *u, U32 ki, U32 x, U32 sid, const tMBuf &data)
	 : tmMsgBase(NetMsgCustomVaultFindAge, plNetX | plNetKi | plNetAck | plNetSid, u), data(data)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
	}
	
	tmCustomVaultFindAge::tmCustomVaultFindAge(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmCustomVaultFindAge::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetSid)) throw txProtocolError(_WHERE("X, KI or Sid flag missing"));
		// store the whole message
		data.clear();
		U32 remaining = t.remaining();
		data.write(t.readAll(), remaining);
	}
	
	void tmCustomVaultFindAge::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(data);
	}
	
} //end namespace alc
