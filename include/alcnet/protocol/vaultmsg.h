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
		tmVault(tNetSession *u, U32 ki, U32 x, bool task, tBaseType *vaultMessage);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str() const;
		// format
		tMBuf message;
	};
	
	class tmCustomVaultAskPlayerList : public tmMsgBase {
	public:
		tmCustomVaultAskPlayerList(tNetSession *u, U32 x, U32 sid, const Byte *uid);
		tmCustomVaultAskPlayerList(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmCustomVaultPlayerList : public tmMsgBase {
	public:
		tmCustomVaultPlayerList(tNetSession *u);
		tmCustomVaultPlayerList(tNetSession *u, U32 x, U32 sid, const Byte *uid);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str() const;
		// format
		U16 numberPlayers;
		tMBuf players;
	};
	
	class tmCustomVaultPlayerStatus : public tmMsgBase {
	public:
		tmCustomVaultPlayerStatus(tNetSession *u, U32 ki, const tString &serverGuid, const tString &age, Byte state, U32 onlineTime);
		tmCustomVaultPlayerStatus(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str() const;
		// format
		tString age, serverGuid;
		Byte state;
		U32 onlineTime;
	};
	
	class tmCustomVaultCreatePlayer : public tmMsgBase {
	public:
		tmCustomVaultCreatePlayer(tNetSession *u);
		tmCustomVaultCreatePlayer(tNetSession *u, U32 x, U32 sid, const Byte *uid, Byte accessLevel, const tString &login, const tString &avatar, const tString &gender, const tString &friendName, const tString &key);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str() const;
		// format
		tUruString login;
		tString avatar, gender, friendName, key;
		Byte accessLevel;
	};
	
	class tmCustomVaultPlayerCreated : public tmMsgBase {
	public:
		tmCustomVaultPlayerCreated(tNetSession *u);
		tmCustomVaultPlayerCreated(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid, Byte result);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str() const;
		// format
		Byte result;
	};
	
	class tmCustomVaultDeletePlayer : public tmMsgBase {
	public:
		tmCustomVaultDeletePlayer(tNetSession *u);
		tmCustomVaultDeletePlayer(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid, Byte accessLevel);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str() const;
		// format
		Byte accessLevel;
	};
	
	class tmCustomVaultCheckKi : public tmMsgBase {
	public:
		tmCustomVaultCheckKi(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid);
		tmCustomVaultCheckKi(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
	};
	
	class tmCustomVaultKiChecked : public tmMsgBase {
	public:
		tmCustomVaultKiChecked(tNetSession *u, U32 ki, U32 x, U32 sid, const Byte *uid, Byte status, const tString &avatar);
		tmCustomVaultKiChecked(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str() const;
		// format
		Byte status;
		tString avatar;
	};
	
	class tmCustomVaultFindAge : public tmMsgBase {
	public:
		tmCustomVaultFindAge(tNetSession *u, U32 ki, U32 x, U32 sid, const tMBuf &data);
		tmCustomVaultFindAge(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		// format
		tMBuf data;
	};
	
} //End alc namespace

#endif
