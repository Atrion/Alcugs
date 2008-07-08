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
#include "protocol/vaultmsg.h"
#include "protocol/lobbymsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmCustomVaultAskPlayerList
	tmCustomVaultAskPlayerList::tmCustomVaultAskPlayerList(tNetSession *u, Byte x, Byte *uid)
	: tmMsgBase(NetMsgCustomVaultAskPlayerList, plNetAck | plNetCustom | plNetX | plNetVersion | plNetUID, u)
	{
		this->x = x;
		memcpy(this->uid, uid, 16);
	}
	
	//// tmCustomVaultPlayerList
	tmCustomVaultPlayerList::tmCustomVaultPlayerList(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{ }
	
	void tmCustomVaultPlayerList::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetUID)) throw txProtocolError(_WHERE("X or UID flag missing"));
		numberPlayers = t.getU16();
		players.clear();
		t.get(players); // the rest is the data about the players
	}
	
	void tmCustomVaultPlayerList::additionalFields()
	{
		dbg.nl();
		dbg.printf(" number of players: %d", numberPlayers);
	}
	
	//// tmCustomVaultPlayerStatus
	tmCustomVaultPlayerStatus::tmCustomVaultPlayerStatus(tNetSession *u, U32 ki, U32 x, const Byte *serverGuid, const Byte *age, Byte state, U32 onlineTime)
	 : tmMsgBase(NetMsgCustomVaultPlayerStatus, plNetAck | plNetCustom | plNetVersion | plNetX | plNetKi, u)
	{
		this->ki = ki;
		this->x = x;
		
		this->serverGuid.setVersion(0); // normal UruString
		this->serverGuid.writeStr(serverGuid);
		this->age.setVersion(0); // normal UruString
		this->age.writeStr(age);
		this->state = state;
		this->onlineTime = onlineTime;
	}
	
	int tmCustomVaultPlayerStatus::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		off += t.put(age);
		off += t.put(serverGuid);
		t.putByte(state); ++off;
		t.putU32(onlineTime); off += 4;
		return off;
	}
	
	void tmCustomVaultPlayerStatus::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Server GUID: %s, Age filename: %s, State: 0x%02X, online time: %d", serverGuid.c_str(), age.c_str(), state, onlineTime);
	}
	
	//// tmVault
	tmVault::tmVault(tNetSession *u) : tmMsgBase(NetMsgVault, plNetAck | plNetKi, u)
	{ ki = 0; }
	
	tmVault::tmVault(tNetSession *u, U32 ki, tBaseType *vaultMessage) : tmMsgBase(NetMsgVault, plNetAck | plNetKi, u)
	{
		message.put(*vaultMessage);
		this->ki = ki;
		if (u->getTpots() == 1) // if were sure it is TPOTS, use TPOTS mod
			cmd = NetMsgVault2;
	}
	
	void tmVault::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// store the whole message
		message.clear();
		t.get(message);
	}
	
	int tmVault::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		off += t.put(message);
		return off;
	}
	
	//// tmCustomVaultCreatePlayer
	tmCustomVaultCreatePlayer::tmCustomVaultCreatePlayer(tNetSession *u, tmCreatePlayer &createPlayer, U32 x, Byte *uid,
	  Byte accessLevel, const Byte *login)
	 : tmMsgBase(NetMsgCustomVaultCreatePlayer, plNetX | plNetUID | plNetVersion | plNetAck | plNetCustom, u),
	   avatar(createPlayer.avatar), gender(createPlayer.gender), friendName(createPlayer.friendName), key(createPlayer.key)
	{
		this->x = x;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
		
		this->accessLevel = accessLevel;
		this->login.setVersion(5); // inverted
		this->login.writeStr(login);
		avatar.setVersion(0); // normal UruString
		gender.setVersion(0); // normal UruString
		friendName.setVersion(0); // normal UruString
		key.setVersion(0); // normal UruString
	}
	
	int tmCustomVaultCreatePlayer::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
		off += t.put(login);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); off += 16; } // UID (only for old protocol, the new one sends it in the header)
#endif
		t.putByte(accessLevel); ++off;
		off += t.put(avatar);
		off += t.put(gender);
		off += t.put(friendName);
		off += t.put(key);
		t.putU32(0); off += 4;
		return off;
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
	tmCustomVaultPlayerCreated::tmCustomVaultPlayerCreated(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{ }
	
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
		result = t.getByte();
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
	tmCustomVaultDeletePlayer::tmCustomVaultDeletePlayer(tNetSession *u, U32 x, U32 ki, Byte *uid, Byte accessLevel)
	 : tmMsgBase(NetMsgCustomVaultDeletePlayer, plNetX | plNetKi | plNetUID | plNetAck | plNetCustom | plNetVersion, u)
	{
		this->x = x;
		this->ki = ki;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
		this->accessLevel = accessLevel;
	}
	
	int tmCustomVaultDeletePlayer::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); off += 16; } // UID (only for old protocol, the new one sends it in the header)
#endif
		t.putByte(accessLevel); ++off;
		return off;
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
	tmCustomVaultCheckKi::tmCustomVaultCheckKi(tNetSession *u, U32 x, U32 ki, Byte *uid)
	 : tmMsgBase(NetMsgCustomVaultCheckKi, plNetX | plNetKi | plNetUID | plNetAck | plNetCustom | plNetVersion, u)
	{
		this->x = x;
		this->ki = ki;
		memcpy(this->uid, uid, 16);
#ifdef ENABLE_UNET2
		if (u->proto == 1) unsetFlags(plNetUID);
#endif
	}
	
	int tmCustomVaultCheckKi::stream(tBBuf &t)
	{
		int off = tmMsgBase::stream(t);
#ifdef ENABLE_UNET2
		if (u->proto == 1) { t.write(uid, 16); off += 16; } // UID (only for old protocol, the new one sends it in the header)
#endif
		return off;
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
	tmCustomVaultKiChecked::tmCustomVaultKiChecked(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
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
		status = t.getByte();
		t.get(avatar);
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
