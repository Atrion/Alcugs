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

#ifndef __U_LOBBYMSG_H
#define __U_LOBBYMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_LOBBYMSG_H_ID "$Id$"

#include "protocol.h"

namespace alc {

	////DEFINITIONS
	class tmRequestMyVaultPlayerList : public tmMsgBase {
	public:
		tmRequestMyVaultPlayerList(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmVaultPlayerList : public tmMsgBase {
	public:
		tmVaultPlayerList(tNetSession *u, U32 x, U16 numberPlayers, tMBuf players, const tString &url);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		U16 numberPlayers;
		tMBuf players;
		tString url;
	};
	
	class tmCreatePlayer : public tmMsgBase {
	public:
		tmCreatePlayer(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		tString avatar, gender, friendName, key;
	};
	
	class tmPlayerCreated : public tmMsgBase {
	public:
		tmPlayerCreated(tNetSession *u, U32 ki, U32 x, Byte result);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		Byte result;
	};
	
	class tmDeletePlayer : public tmMsgBase {
	public:
		tmDeletePlayer(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
} //End alc namespace

#endif
