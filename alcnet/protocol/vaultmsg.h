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

#ifndef __U_VAULTMSG_H
#define __U_VAULTMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_VAULTMSG_H_H_ID "$Id$"

#include "protocol.h"

namespace alc {

	class tmVault : public tmNetMsg { // this is both a vault and a lobbybase msg, but the vault server includes only this file so the class is defined here
		NETMSG_RECEIVE_CONSTRUCTORS(tmVault, tmNetMsg)
	public:
		tmVault(tNetSession *u, uint32_t ki, uint32_t x, bool task, tStreamable *vaultMessage);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tMBuf message;
	};
	
	class tmGetPublicAgeList : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmGetPublicAgeList, tmNetMsg)
	public:
		tmGetPublicAgeList(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, tString age);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tString age;
	};
	
	class tmPublicAgeList : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmPublicAgeList, tmNetMsg)
	public:
		typedef std::vector<tAgeInfoStruct> tAgeList;
		typedef std::vector<uint32_t> tPopulationList;
		
		tmPublicAgeList(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid);
		tmPublicAgeList(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, tAgeList ages);
		tmPublicAgeList(tNetSession *u, const tmPublicAgeList &ageList);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		
		// format
		tAgeList ages;
		tPopulationList populations;
	};
	
	class tmRequestMyVaultPlayerList : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmRequestMyVaultPlayerList, tmNetMsg)
	public:
		tmRequestMyVaultPlayerList(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid);
		virtual void store(tBBuf &t);
	};
	
	class tmVaultPlayerList : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmVaultPlayerList, tmNetMsg)
	public:
		class tAvatar : public tStreamable {
		public:
			tAvatar(uint32_t ki, tString name, uint8_t flags) : ki(ki), name(name), flags(flags) {}
			tAvatar() {}
			virtual void store(tBBuf &t);
			virtual void stream(tBBuf &t) const;
			
			uint32_t ki;
			tString name;
			uint8_t flags;
		};
		typedef std::vector<tAvatar> tAvatarList;
		
		tmVaultPlayerList(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid);
		tmVaultPlayerList(tNetSession *u, const tmVaultPlayerList &playerList, const tString &url);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		
		// format
		tAvatarList avatars;
		tString url;
	};
	
	class tmCustomVaultPlayerStatus : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomVaultPlayerStatus, tmNetMsg)
	public:
		tmCustomVaultPlayerStatus(tNetSession *u, uint32_t ki, const tString &serverGuid, const tString &age, uint8_t state, uint32_t onlineTime);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tString age, serverGuid;
		uint8_t state;
		uint32_t onlineTime;
	};
	
	class tmCustomVaultCreatePlayer : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomVaultCreatePlayer, tmNetMsg)
	public:
		tmCustomVaultCreatePlayer(tNetSession *u, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t accessLevel, const tString &login, const tString &avatar, const tString &gender, const tString &friendName, const tString &key);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tUruString login;
		tString avatar, gender, friendName, key;
		uint8_t accessLevel;
	};
	
	class tmCustomVaultPlayerCreated : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomVaultPlayerCreated, tmNetMsg)
	public:
		tmCustomVaultPlayerCreated(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t result);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t result;
	};
	
	class tmCustomVaultDeletePlayer : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomVaultDeletePlayer, tmNetMsg)
	public:
		tmCustomVaultDeletePlayer(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t accessLevel);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t accessLevel;
	};
	
	class tmCustomVaultCheckKi : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomVaultCheckKi, tmNetMsg)
	public:
		tmCustomVaultCheckKi(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
	};
	
	class tmCustomVaultKiChecked : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomVaultKiChecked, tmNetMsg)
	public:
		tmCustomVaultKiChecked(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const uint8_t *uid, uint8_t status, const tString &avatar);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t status;
		tString avatar;
	};
	
	class tmCustomVaultFindAge : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomVaultFindAge, tmNetMsg)
	public:
		tmCustomVaultFindAge(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, const tMBuf &data);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		// format
		tMBuf data;
	};
	
} //End alc namespace

#endif
