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

#ifndef __U_LOBBYMSG_H
#define __U_LOBBYMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_LOBBYMSG_H_ID "$Id$"

#include "protocol.h"
#include "netsession.h"

namespace alc {

	class tmRequestMyVaultPlayerList : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmRequestMyVaultPlayerList, tmNetMsg)
	public:
		virtual void store(tBBuf &t);
	};
	
	class tmVaultPlayerList : public tmNetMsg {
	public:
		tmVaultPlayerList(tNetSession *u, uint32_t x, uint16_t numberPlayers, tMBuf players, const tString &url);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint16_t numberPlayers;
		tMBuf players;
		tString url;
	};
	
	class tmCreatePlayer : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCreatePlayer, tmNetMsg)
	public:
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		tString avatar, gender, friendName, key;
	};
	
	class tmPlayerCreated : public tmNetMsg {
	public:
		tmPlayerCreated(tNetSession *u, uint32_t ki, uint32_t x, uint8_t result);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t result;
	};
	
	class tmDeletePlayer : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmDeletePlayer, tmNetMsg)
	public:
		virtual void store(tBBuf &t);
	};
	
} //End alc namespace

#endif
