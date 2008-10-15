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
#define __U_LOBBYMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "alcnet.h"
#include "protocol/lobbymsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmRequestMyVaultPlayerList
	tmRequestMyVaultPlayerList::tmRequestMyVaultPlayerList(tNetSession *u) : tmMsgBase(u) // it's not capable of sending
	{ }
	
	void tmRequestMyVaultPlayerList::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// the vault manager sends these without X and KI
		if (ki != 0) throw txProtocolError(_WHERE("KI must be 0 in NetMsgRequestMyVaultPlayerList but is %d", ki));
		
		u->x = x;
	}
	
	//// tmVaultPlayerList
	tmVaultPlayerList::tmVaultPlayerList(tNetSession *u, U16 numberPlayers, tMBuf players, const Byte *url)
	: tmMsgBase(NetMsgVaultPlayerList, plNetAck | plNetCustom | plNetX | plNetKi, u)
	{
		x = u->x;
		ki = 0; // we're not yet logged in, so no KI can be set
		
		this->numberPlayers = numberPlayers;
		this->players = players;
		this->url.setVersion(0); // normal UrurString
		this->url.writeStr(url);
	}
	
	void tmVaultPlayerList::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU16(numberPlayers);
		t.put(players);
		t.put(url);
	}
	
	void tmVaultPlayerList::additionalFields()
	{
		dbg.nl();
		dbg.printf(" number of avatars: %d, URL: %s", numberPlayers, url.c_str());
	}
	
	//// tmCreatePlayer
	tmCreatePlayer::tmCreatePlayer(tNetSession *u) : tmMsgBase(u) // it's not capable of sending
	{
		avatar.setVersion(0); // normal UruString
		gender.setVersion(0); // normal UruString
		friendName.setVersion(0); // normal UruString
		key.setVersion(0); // normal UruString
	}
	
	void tmCreatePlayer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
		// a KI value might be set here, but it should be ignored
		t.get(avatar);
		t.get(gender);
		t.get(friendName);
		t.get(key);
		U32 unk = t.getU32();
		if (unk != 0) {
			throw txProtocolError(_WHERE("NetMsgCreatePlayer.unk is not 0 but 0x%08X", unk));
		}
		
		u->x = x;
	}
	
	void tmCreatePlayer::additionalFields(void)
	{
		dbg.nl();
		dbg.printf(" avatar: %s, gender: %s, friend: %s, key: %s", avatar.c_str(), gender.c_str(), friendName.c_str(), key.c_str());
	}
	
	//// tmPlayerCreated
	tmPlayerCreated::tmPlayerCreated(tNetSession *u, U32 ki, Byte result)
	 : tmMsgBase(NetMsgPlayerCreated, plNetX | plNetKi | plNetAck | plNetCustom, u)
	{
		x = u->x;
		this->ki = ki; // the KI of the newly created player, not the one set for the session
		this->result = result;
	}
	
	void tmPlayerCreated::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putByte(result);
	}
	
	void tmPlayerCreated::additionalFields()
	{
		dbg.nl();
		dbg.printf(" result: 0x%02X (%s)", result, alcUnetGetAvatarCode(result));
	}
	
	//// tmDeletePlayer
	tmDeletePlayer::tmDeletePlayer(tNetSession *u) : tmMsgBase(u) // it's not capable of sending
	{ }
	
	void tmDeletePlayer::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
		U16 unk = t.getU16();
		if (unk != 0) {
			throw txProtocolError(_WHERE("NetMsgDeletePlayer.unk is not 0 but 0x%08X", unk));
		}
		
		u->x = x;
	}

} //end namespace alc
