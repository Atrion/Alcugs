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

#include "protocol.h"

namespace alc {

	class tmVault : public tmMsgBase { // this is both a vault and a lobbybase msg, but the vault server includes only this file so the class is defined here
	public:
		tmVault(tNetSession *u);
		tmVault(tNetSession *u, uint32_t ki, uint32_t x, bool task, tStreamable *vaultMessage);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tMBuf message;
	};
	
	class tmCustomVaultAskPlayerList : public tmMsgBase {
	public:
		tmCustomVaultAskPlayerList(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid);
		tmCustomVaultAskPlayerList(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmCustomVaultPlayerList : public tmMsgBase {
	public:
		tmCustomVaultPlayerList(tNetSession *u);
		tmCustomVaultPlayerList(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint16_t numberPlayers;
		tMBuf players;
	};
	
	class tmCustomVaultPlayerStatus : public tmMsgBase {
	public:
		tmCustomVaultPlayerStatus(tNetSession *u, uint32_t ki, const tString &serverGuid, const tString &age, uint8_t state, uint32_t onlineTime);
		tmCustomVaultPlayerStatus(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tString age, serverGuid;
		uint8_t state;
		uint32_t onlineTime;
	};
	
	class tmCustomVaultCreatePlayer : public tmMsgBase {
	public:
		tmCustomVaultCreatePlayer(tNetSession *u);
		tmCustomVaultCreatePlayer(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t accessLevel, const tString &login, const tString &avatar, const tString &gender, const tString &friendName, const tString &key);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tUruString login;
		tString avatar, gender, friendName, key;
		uint8_t accessLevel;
	};
	
	class tmCustomVaultPlayerCreated : public tmMsgBase {
	public:
		tmCustomVaultPlayerCreated(tNetSession *u);
		tmCustomVaultPlayerCreated(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t result);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t result;
	};
	
	class tmCustomVaultDeletePlayer : public tmMsgBase {
	public:
		tmCustomVaultDeletePlayer(tNetSession *u);
		tmCustomVaultDeletePlayer(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t accessLevel);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t accessLevel;
	};
	
	class tmCustomVaultCheckKi : public tmMsgBase {
	public:
		tmCustomVaultCheckKi(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid);
		tmCustomVaultCheckKi(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
	};
	
	class tmCustomVaultKiChecked : public tmMsgBase {
	public:
		tmCustomVaultKiChecked(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t status, const tString &avatar);
		tmCustomVaultKiChecked(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t status;
		tString avatar;
	};
	
	class tmCustomVaultFindAge : public tmMsgBase {
	public:
		tmCustomVaultFindAge(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const tMBuf &data);
		tmCustomVaultFindAge(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		// format
		tMBuf data;
	};
	
} //End alc namespace

#endif
