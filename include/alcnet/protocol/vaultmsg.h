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

#ifndef __U_VAULTMSG_H
#define __U_VAULTMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_VAULTMSG_H_H_ID "$Id$"

#include "lobbymsg.h"

namespace alc {

	////DEFINITIONS
	class tmCustomVaultAskPlayerList : public tmMsgBase {
	public:
		tmCustomVaultAskPlayerList(tNetSession *u, Byte x, Byte *guid);
	};
	
	class tmCustomVaultPlayerList : public tmMsgBase {
	public:
		tmCustomVaultPlayerList(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		U16 numberPlayers;
		tMBuf players;
	protected:
		virtual void additionalFields();
	};
	
	class tmVault : public tmMsgBase {
	public:
		tmVault(tNetSession *u);
		tmVault(tNetSession *u, U32 ki, tBaseType *vaultMessage);
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
		// format
		tMBuf message;
	};
	
	class tmCustomVaultCreatePlayer : public tmCreatePlayer {
	public:
		tmCustomVaultCreatePlayer(tNetSession *u, tmCreatePlayer &createPlayer, Byte x, Byte *guid);
	};
	
} //End alc namespace

#endif
