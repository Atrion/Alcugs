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
#include "protocol/vaultmsg.h"

#include <alcdebug.h>

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
		else if (u->tpots == 2) // if were sure it is UU, use that value
			cmd = NetMsgVault_UU;
	}
	
	void tmVault::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// store the whole message
		message.clear();
		t.get(message);
	}
	
	void tmVault::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(message);
	}
	
	void tmVault::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Size of attached message: %d Bytes", message.size());
	}
	
	//// tmCustomVaultAskPlayerList
	tmCustomVaultAskPlayerList::tmCustomVaultAskPlayerList(tNetSession *u, U32 x, U32 sid, const Byte *uid)
	: tmMsgBase(NetMsgCustomVaultAskPlayerList, plNetAck | plNetCustom | plNetX | plNetVersion | plNetUID | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
	}
	
	tmCustomVaultAskPlayerList::tmCustomVaultAskPlayerList(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmCustomVaultAskPlayerList::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetUID)) throw txProtocolError(_WHERE("X or UID flag missing"));
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
	}
	
	//// tmCustomVaultPlayerList
	tmCustomVaultPlayerList::tmCustomVaultPlayerList(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomVaultPlayerList::tmCustomVaultPlayerList(tNetSession *u, U32 x, U32 sid, const Byte *uid)
	: tmMsgBase(NetMsgCustomVaultPlayerList, plNetAck | plNetCustom | plNetX | plNetVersion | plNetUID | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
		numberPlayers = 0;
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
	}
	
	void tmCustomVaultPlayerList::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetUID)) throw txProtocolError(_WHERE("X or UID flag missing"));
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
		numberPlayers = t.getU16();
		players.clear();
		t.get(players); // the rest is the data about the players
	}
	
	void tmCustomVaultPlayerList::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU16(numberPlayers);
		players.rewind();
		t.put(players);
	}
	
	void tmCustomVaultPlayerList::additionalFields()
	{
		dbg.nl();
		dbg.printf(" number of players: %d", numberPlayers);
	}
	
	//// tmCustomVaultPlayerStatus
	tmCustomVaultPlayerStatus::tmCustomVaultPlayerStatus(tNetSession *u) : tmMsgBase(u)
	{
		serverGuid.setVersion(0); // normal UruString
		age.setVersion(0); // normal UruString
	}
	
	tmCustomVaultPlayerStatus::tmCustomVaultPlayerStatus(tNetSession *u, U32 ki, const char *serverGuid, const char *age, Byte state, U32 onlineTime)
	 : tmMsgBase(NetMsgCustomVaultPlayerStatus, plNetAck | plNetCustom | plNetVersion | plNetKi, u)
	{
		this->ki = ki;
		this->x = 0;
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) setFlags(plNetX); // older protocols have this set, but the value is ignored
#endif
		
		this->serverGuid.setVersion(0); // normal UruString
		this->serverGuid.writeStr(serverGuid);
		this->age.setVersion(0); // normal UruString
		this->age.writeStr(age);
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
	
	void tmCustomVaultPlayerStatus::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(age);
		t.put(serverGuid);
		t.putByte(state);
		t.putU32(onlineTime);
	}
	
	void tmCustomVaultPlayerStatus::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Server GUID: %s, Age filename: %s, State: 0x%02X, online time: %d", serverGuid.c_str(), age.c_str(), state, onlineTime);
	}
	
	//// tmCustomVaultCreatePlayer
	tmCustomVaultCreatePlayer::tmCustomVaultCreatePlayer(tNetSession *u) : tmMsgBase(u)
	{
		this->login.setVersion(5); // inverted
		this->avatar.setVersion(0); // normal UruString
		this->gender.setVersion(0); // normal UruString
		this->friendName.setVersion(0); // normal UruString
		this->key.setVersion(0); // normal UruString
	}
	
	tmCustomVaultCreatePlayer::tmCustomVaultCreatePlayer(tNetSession *u, U32 x, U32 sid, Byte *uid,
	  Byte accessLevel, const char *login, tUStr &avatar, tUStr &gender, tUStr &friendName, tUStr &key)
	 : tmMsgBase(NetMsgCustomVaultCreatePlayer, plNetX | plNetUID | plNetVersion | plNetAck | plNetCustom | plNetSid, u),
	   avatar(avatar), gender(gender), friendName(friendName), key(key)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			setFlags(plNetX);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
		
		this->accessLevel = accessLevel;
		this->login.setVersion(5); // inverted
		this->login.writeStr(login);
		this->avatar.setVersion(0); // normal UruString
		this->gender.setVersion(0); // normal UruString
		this->friendName.setVersion(0); // normal UruString
		this->key.setVersion(0); // normal UruString
	}
	
	void tmCustomVaultCreatePlayer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX)) throw txProtocolError(_WHERE("X flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetUID)) throw txProtocolError(_WHERE("UID flag missing"));
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
		t.get(login);
#ifdef ENABLE_UNET2
		if (!hasFlags(plNetUID)) {
			memcpy(uid, t.read(16), 16);
			u->proto = 1; // unet2 protocol
		}
# endif
		accessLevel = t.getByte();
		t.get(avatar);
		t.get(gender);
		t.get(friendName);
		t.get(key);
		U32 unk = t.getU32();
		if (unk != 0) throw txProtocolError(_WHERE("NetMsgCustomVaultCreatePlayer.unk must always be 0"));
	}
	
	void tmCustomVaultCreatePlayer::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(login);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); } // UID (only for old protocol, the new one sends it in the header)
#endif
		t.putByte(accessLevel);
		t.put(avatar);
		t.put(gender);
		t.put(friendName);
		t.put(key);
		t.putU32(0);
	}
	
	void tmCustomVaultCreatePlayer::additionalFields()
	{
		dbg.nl();
		dbg.printf(" login: %s, ", login.c_str());
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf("uid (unet2 protocol): %s, ", alcGetStrUid(uid));
#endif
		dbg.printf("accessLevel: %d, avatar: %s, gender: %s, friend: %s, key: %s", accessLevel, avatar.c_str(), gender.c_str(), friendName.c_str(), key.c_str());
	}
	
	//// tmCustomVaultPlayerCreated
	tmCustomVaultPlayerCreated::tmCustomVaultPlayerCreated(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmCustomVaultPlayerCreated::tmCustomVaultPlayerCreated(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid, Byte result)
	 : tmMsgBase(NetMsgCustomVaultPlayerCreated, plNetKi | plNetX | plNetAck | plNetCustom | plNetUID | plNetSid, u)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif

		this->result = result;
	}
	
	void tmCustomVaultPlayerCreated::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetUID)) throw txProtocolError(_WHERE("UID flag missing"));
#else
		if (!hasFlags(plNetUID)) {
			memcpy(uid, t.read(16), 16);
			u->proto = 1; // unet2 protocol
		}
# endif
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
		result = t.getByte();
	}
	
	void tmCustomVaultPlayerCreated::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); } // UID (only for old protocol, the new one sends it in the header)
#endif
		t.putByte(result);
	}
	
	void tmCustomVaultPlayerCreated::additionalFields()
	{
		dbg.nl();
#ifdef ENABLE_UNET2
		if (u->proto == 1) dbg.printf(" uid (unet2 protocol): %s,", alcGetStrUid(uid));
#endif
		dbg.printf(" result: 0x%02X (%s)", result, alcUnetGetAvatarCode(result));
	}
	
	//// tmCustomVaultDeletePlayer
	tmCustomVaultDeletePlayer::tmCustomVaultDeletePlayer(tNetSession *u)
	 : tmMsgBase(NetMsgCustomVaultDeletePlayer, plNetX | plNetKi | plNetUID | plNetAck | plNetCustom | plNetVersion, u)
	{
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
	}
	
	tmCustomVaultDeletePlayer::tmCustomVaultDeletePlayer(tNetSession *u, U32 ki, U32 x, U32 sid, Byte *uid, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomVaultDeletePlayer, plNetX | plNetKi | plNetUID | plNetAck | plNetCustom | plNetVersion | plNetSid, u)
	{
		this->x = x;
		this->ki = ki;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
		this->accessLevel = accessLevel;
	}
	
	void tmCustomVaultDeletePlayer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetUID)) throw txProtocolError(_WHERE("UID flag missing"));
#else
		if (!hasFlags(plNetUID)) {
			memcpy(uid, t.read(16), 16);
			u->proto = 1; // unet2 protocol
		}
# endif
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
		accessLevel = t.getByte();
	}
	
	void tmCustomVaultDeletePlayer::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); } // UID (only for old protocol, the new one sends it in the header)
#endif
		t.putByte(accessLevel);
	}
	
	void tmCustomVaultDeletePlayer::additionalFields()
	{
		dbg.nl();
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf(" uid (unet2 protocol): %s,", alcGetStrUid(uid));
#endif
		dbg.printf(" access level: %d", accessLevel);
	}
	
	//// tmCustomVaultCheckKi
	tmCustomVaultCheckKi::tmCustomVaultCheckKi(tNetSession *u, U32 ki, U32 x, U32 sid, Byte *uid)
	 : tmMsgBase(NetMsgCustomVaultCheckKi, plNetX | plNetKi | plNetUID | plNetAck | plNetCustom | plNetVersion | plNetSid, u)
	{
		this->x = x;
		this->ki = ki;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
	}
	
	tmCustomVaultCheckKi::tmCustomVaultCheckKi(tNetSession *u)
	 : tmMsgBase(NetMsgCustomVaultCheckKi, plNetX | plNetKi | plNetUID | plNetAck | plNetCustom | plNetVersion, u)
	{ }

	void tmCustomVaultCheckKi::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetUID)) throw txProtocolError(_WHERE("UID flag missing"));
#else
		if (!hasFlags(plNetUID)) {
			memcpy(uid, t.read(16), 16);
			u->proto = 1; // unet2 protocol
		}
# endif
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
	}
	
	void tmCustomVaultCheckKi::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); } // UID (only for old protocol, the new one sends it in the header)
#endif
	}
	
	void tmCustomVaultCheckKi::additionalFields()
	{
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) {
			dbg.nl();
			dbg.printf(" uid (unet2 protocol): %s,", alcGetStrUid(uid));
		}
#endif
	}
	
	//// tmCustomVaultKiChecked
	tmCustomVaultKiChecked::tmCustomVaultKiChecked(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid, Byte status, const char *avatar)
	: tmMsgBase(NetMsgCustomVaultKiChecked, plNetX | plNetKi | plNetUID | plNetAck | plNetCustom | plNetVersion | plNetSid, u)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
#ifdef ENABLE_UNET3
		if (u->proto == 1 || u->proto == 2) {
			unsetFlags(plNetSid);
			this->x = sid; // older protocols have the sid in the X field
		}
#endif
	
		this->status = status;
		this->avatar.setVersion(0); // normal UruString
		this->avatar.writeStr(avatar);
	}
	
	tmCustomVaultKiChecked::tmCustomVaultKiChecked(tNetSession *u) : tmMsgBase(u)
	{
		avatar.setVersion(0); // normal UruString
	}
	
	void tmCustomVaultKiChecked::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
#ifndef ENABLE_UNET2
		if (!hasFlags(plNetUID)) throw txProtocolError(_WHERE("UID flag missing"));
#else
		if (!hasFlags(plNetUID)) {
			memcpy(uid, t.read(16), 16);
			u->proto = 1; // unet2 protocol
		}
# endif
#ifndef ENABLE_UNET3
		if (!hasFlags(plNetSid)) throw txProtocolError(_WHERE("Sid flag missing"));
#else
		if (!hasFlags(plNetSid)) { // unet3+ carries the sid in the sid field, older protocols have it in the X field
			sid = x;
			x = 0;
			if (u->proto != 1) u->proto = 2; // unet3 protocol
		}
#endif
		status = t.getByte();
		t.get(avatar);
	}
	
	void tmCustomVaultKiChecked::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); } // UID (only for old protocol, the new one sends it in the header)
#endif
		t.putByte(status);
		t.put(avatar);
	}
	
	void tmCustomVaultKiChecked::additionalFields()
	{
		dbg.nl();
#ifdef ENABLE_UNET2
		if (u && u->proto == 1) dbg.printf(" uid (unet2 protocol): %s,", alcGetStrUid(uid));
#endif
		dbg.printf(" status: 0x%02X, avatar: %s", status, avatar.c_str());	
	}
	
} //end namespace alc
