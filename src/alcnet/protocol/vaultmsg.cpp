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

#include <alcdebug.h>

namespace alc {

	//// tmCustomVaultAskPlayerList
	tmCustomVaultAskPlayerList::tmCustomVaultAskPlayerList(tNetSession *u, Byte x, Byte *guid)
	: tmMsgBase(NetMsgCustomVaultAskPlayerList, plNetAck | plNetCustom | plNetX | plNetVersion | plNetGUI, u)
	{
		this->x = x;
		memcpy(this->guid, guid, 16);
	}
	
	//// tmCustomVaultPlayerList
	tmCustomVaultPlayerList::tmCustomVaultPlayerList(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{ }
	
	void tmCustomVaultPlayerList::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetGUI | plNetVersion)) throw txProtocolError(_WHERE("X, GUID or Version flag missing"));
		numberPlayers = t.getU16();
		players.clear();
		t.get(players); // the rest is the data about the players
	}
	
	void tmCustomVaultPlayerList::additionalFields()
	{
		dbg.nl();
		dbg.printf(" number of players: %d", numberPlayers);
	}
	
	////tmVault
	tmVault::tmVault(tNetSession *u) : tmMsgBase(NetMsgVault, plNetAck | plNetKi, u)
	{ ki = 0; }
	
	tmVault::tmVault(tNetSession *u, U32 ki, tBaseType *vaultMessage) : tmMsgBase(NetMsgVault, plNetAck | plNetKi, u)
	{
		message.put(*vaultMessage);
		this->ki = ki;
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
	tmCustomVaultCreatePlayer::tmCustomVaultCreatePlayer(tNetSession *u, tmCreatePlayer &createPlayer, Byte x, Byte *guid)
	 : tmCreatePlayer(NetMsgCustomVaultCreatePlayer, plNetX | plNetGUI | plNetVersion | plNetAck | plNetCustom, u, createPlayer)
	{
		this->x = x;
		memcpy(this->guid, guid, 16);
	}
	
} //end namespace alc
