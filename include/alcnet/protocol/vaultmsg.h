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

namespace alc {

	////DEFINITIONS
	class tmVault : public tmMsgBase { // this is both a vault and a lobbybase msg, but the vault server includes only this file so the class is defined here
	public:
		tmVault(tNetSession *u);
		tmVault(tNetSession *u, U32 ki, tBaseType *vaultMessage);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		tMBuf message;
	};
	
	class tmVaultTask : public tmMsgBase { // this is both a vault and a game msg, but the vault server includes only this file so the class is defined here
	public:
		tmVaultTask(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		tMBuf message;
	};
	
	class tmCustomVaultAskPlayerList : public tmMsgBase {
	public:
		tmCustomVaultAskPlayerList(tNetSession *u, U32 x, const Byte *uid);
		tmCustomVaultAskPlayerList(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmCustomVaultPlayerList : public tmMsgBase {
	public:
		tmCustomVaultPlayerList(tNetSession *u);
		tmCustomVaultPlayerList(tNetSession *u, U32 x, const Byte *uid);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		U16 numberPlayers;
		tMBuf players;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomVaultPlayerStatus : public tmMsgBase {
	public:
		tmCustomVaultPlayerStatus(tNetSession *u, U32 ki, U32 x, const Byte *serverGuid, const Byte *age, Byte state, U32 onlineTime);
		tmCustomVaultPlayerStatus(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		tUStr age, serverGuid;
		Byte state;
		U32 onlineTime;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomVaultCreatePlayer : public tmMsgBase {
	public:
		tmCustomVaultCreatePlayer(tNetSession *u);
		tmCustomVaultCreatePlayer(tNetSession *u, U32 x, Byte *uid, Byte accessLevel, const Byte *login, tUStr &avatar, tUStr &gender, tUStr &friendName, tUStr &key);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		tUStr login, avatar, gender, friendName, key;
		Byte accessLevel;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomVaultPlayerCreated : public tmMsgBase {
	public:
		tmCustomVaultPlayerCreated(tNetSession *u);
		tmCustomVaultPlayerCreated(tNetSession *u, U32 ki, U32 x, const Byte *uid, Byte result);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		Byte result;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomVaultDeletePlayer : public tmMsgBase {
	public:
		tmCustomVaultDeletePlayer(tNetSession *u);
		tmCustomVaultDeletePlayer(tNetSession *u, U32 ki, U32 x, Byte *uid, Byte accessLevel);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		Byte accessLevel;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomVaultCheckKi : public tmMsgBase {
	public:
		tmCustomVaultCheckKi(tNetSession *u, U32 ki, U32 x, Byte *uid);
		tmCustomVaultCheckKi(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomVaultKiChecked : public tmMsgBase {
	public:
		tmCustomVaultKiChecked(tNetSession *u, U32 ki, U32 x, const Byte *uid, Byte status, const Byte *avatar);
		tmCustomVaultKiChecked(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		Byte status;
		tUStr avatar;
	protected:
		virtual void additionalFields();
	};
	
} //End alc namespace

#endif
