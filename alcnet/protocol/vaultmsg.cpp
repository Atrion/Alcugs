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
		dbg.printf(" Size of attached message: %d Bytes", message.size());
		return dbg;
	}
	
	//// tmGetPublicAgeList
	tmGetPublicAgeList::tmGetPublicAgeList(tNetSession* u, uint32_t ki, uint32_t x, uint32_t sid, tString age)
	: tmNetMsg(NetMsgGetPublicAgeList, plNetAck | plNetX | plNetKi | plNetVersion | plNetSid, u), age(age)
	{
		this->x = x;
		this->sid = sid;
		this->ki = ki;
	}
	
	void tmGetPublicAgeList::store(tBBuf& t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("Ki or X flag missing")); // when coming from a client, it won't have the sid flag set
		if (u->isUruClient() && (ki == 0 || u->ki != ki)) // don't kick game server if we are the vault and got this message
			throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		t.get(age);
	}
	
	void tmGetPublicAgeList::stream(tBBuf& t) const
	{
		tmNetMsg::stream(t);
		t.put(age);
	}
	
	tString tmGetPublicAgeList::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Age: %s", age.c_str());
		return dbg;
	}
	
	//// tmPublicAgeList
	/** Sending to other server */
	tmPublicAgeList::tmPublicAgeList(tNetSession* u, uint32_t ki, uint32_t x, uint32_t sid)
	: tmNetMsg(NetMsgPublicAgeList, plNetAck | plNetX | plNetKi | plNetVersion | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		this->ki = ki;
	}
	
	/** Sending to other server */
	tmPublicAgeList::tmPublicAgeList(tNetSession* u, uint32_t ki, uint32_t x, uint32_t sid, tAgeList ages)
	: tmNetMsg(NetMsgPublicAgeList, plNetAck | plNetX | plNetKi | plNetVersion | plNetSid, u), ages(ages)
	{
		this->x = x;
		this->sid = sid;
		this->ki = ki;
	}
	
	/** Sending to client */
	tmPublicAgeList::tmPublicAgeList(tNetSession *u, const tmPublicAgeList &ageList)
	: tmNetMsg(NetMsgPublicAgeList, plNetAck | plNetX | plNetKi | plNetSystem, u), ages(ageList.ages), populations(ageList.populations)
	{
		this->x = ageList.x;
		this->ki = ageList.ki;
	}
	
	void tmPublicAgeList::store(tBBuf& t)
	{
		alc::tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetSid)) throw txProtocolError(_WHERE("X or Sid flag missing"));
		
		int count = t.get16();
		ages.clear();
		ages.reserve(count);
		for (int i = 0; i < count; ++i) {
			t.get(*ages.insert(ages.end(), tAgeInfoStruct())); // first insert, then fill with data
		}
		count = t.get16();
		populations.clear();
		populations.reserve(count);
		for (int i = 0; i < count; ++i)
			populations.push_back(t.get32());
	}

	void tmPublicAgeList::stream(tBBuf& t) const
	{
		alc::tmNetMsg::stream(t);
		t.put16(ages.size());
		for (tAgeList::const_iterator it = ages.begin(); it != ages.end(); ++it)
			t.put(*it);
		t.put16(populations.size());
		for (tPopulationList::const_iterator it = populations.begin(); it != populations.end(); ++it)
			t.put32(*it);
	}
	
	tString tmPublicAgeList::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Number of ages: %d", ages.size());
		return dbg;
	}
	
	//// tmCreatePublicAge
	tmCreatePublicAge::tmCreatePublicAge(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const tAgeInfoStruct &age)
	 : tmNetMsg(NetMsgCreatePublicAge, plNetX | plNetKi | plNetAck | plNetSystem | plNetSid, u), age(age)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
	}
	
	void tmCreatePublicAge::store(tBBuf& t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("Ki or X flag missing")); // when coming from a client, it won't have the sid flag set
		if (u->isUruClient() && (ki == 0 || u->ki != ki)) // don't kick game server if we are the vault and got this message
			throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		t.get(age);
		if (!age.hasGuid())
			throw txProtocolError(_WHERE("The createPublicAge request must have the GUID set"));
	}
	
	void tmCreatePublicAge::stream(tBBuf& t) const
	{
		alc::tmNetMsg::stream(t);
		t.put(age);
	}
	
	tString tmCreatePublicAge::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Age: %s", age.str().c_str());
		return dbg;
	}

	//// tmPublicAgeCreated
	/** Send to other server */
	tmPublicAgeCreated::tmPublicAgeCreated(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const tAgeInfoStruct &age)
	 : tmNetMsg(NetMsgPublicAgeCreated, plNetX | plNetKi | plNetAck | plNetSystem | plNetSid, u), age(age)
	{
		this->ki = ki;
		this->x = x;
		this->sid = sid;
	}
	
	/** Send to client */
	tmPublicAgeCreated::tmPublicAgeCreated(tNetSession *u, const tmPublicAgeCreated &ageCreated)
	 : tmNetMsg(NetMsgPublicAgeCreated, plNetX | plNetKi | plNetAck | plNetSystem, u), age(ageCreated.age)
	 {
		this->x = ageCreated.x;
		this->ki = ageCreated.ki;
	 }
	
	void tmPublicAgeCreated::store(tBBuf& t)
	{
		tmNetMsg::store(t);
		if (!hasFlags(plNetX | plNetKi | plNetSid)) throw txProtocolError(_WHERE("Sid, Ki or X flag missing")); // when coming from a client, it won't have the sid flag set
		t.get(age);
	}
	
	void tmPublicAgeCreated::stream(tBBuf& t) const
	{
		alc::tmNetMsg::stream(t);
		t.put(age);
	}
	
	tString tmPublicAgeCreated::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" Age: %s", age.str().c_str());
		return dbg;
	}
	
	//// tmRequestMyVaultPlayerList
	/** Sending to other server */
	tmRequestMyVaultPlayerList::tmRequestMyVaultPlayerList(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid)
	: tmNetMsg(NetMsgRequestMyVaultPlayerList, plNetAck | plNetX | plNetVersion | plNetUID | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
	}
	
	void tmRequestMyVaultPlayerList::store(tBBuf &t)
	{
		tmNetMsg::store(t);
		// the vault manager sends these without X and KI
		if (hasFlags(plNetKi) && ki != 0) throw txProtocolError(_WHERE("KI must be 0 in NetMsgRequestMyVaultPlayerList but is %d", ki));
	}
	
	//// tmVaultPlayerList
	/** Sending to other server */
	tmVaultPlayerList::tmVaultPlayerList(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid)
	: tmNetMsg(NetMsgVaultPlayerList, plNetAck | plNetX | plNetVersion | plNetUID | plNetSid, u)
	{
		this->x = x;
		this->sid = sid;
		memcpy(this->uid, uid, 16);
	}
	
	/** Sending to client */
	tmVaultPlayerList::tmVaultPlayerList(tNetSession *u, const tmVaultPlayerList &playerList, const tString &url)
	: tmNetMsg(NetMsgVaultPlayerList, plNetAck | plNetX | plNetKi, u), avatars(playerList.avatars), url(url)
	{
		this->x = playerList.x;
		ki = 0; // we're not yet logged in, so no KI can be set
	}
	
	void tmVaultPlayerList::tAvatar::store(tBBuf& t)
	{
		ki = t.get32();
		t.get(name);
		flags = t.get8();
	}
	
	void tmVaultPlayerList::store(tBBuf& t)
	{
		tmNetMsg::store(t);
		int count = t.get16();
		avatars.clear();
		avatars.reserve(count);
		for (int i = 0; i < count; ++i)
			t.get(*avatars.insert(avatars.end(), tAvatar())); // first insert, then read
		t.get(url);
	}
	
	void tmVaultPlayerList::tAvatar::stream(tBBuf& t) const
	{
		t.put32(ki);
		t.put(name);
		t.put8(flags);
	}
	
	void tmVaultPlayerList::stream(tBBuf &t) const
	{
		tmNetMsg::stream(t);
		t.put16(avatars.size());
		for (tAvatarList::const_iterator it = avatars.begin(); it != avatars.end(); ++it)
			t.put(*it);
		t.put(url);
	}
	
	tString tmVaultPlayerList::additionalFields(tString dbg) const
	{
		dbg.nl();
		dbg.printf(" number of avatars: %d, URL: %s", avatars.size(), url.c_str());
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
	
} //end namespace alc
