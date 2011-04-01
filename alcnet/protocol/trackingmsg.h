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

#ifndef __U_TRACKINGMSG_H
#define __U_TRACKINGMSG_H

#include "gamemsg.h"

namespace alc {
	
	class tmFindAge;

	class tmCustomSetGuid : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomSetGuid, tmNetMsg)
	public:
		tmCustomSetGuid(tNetSession *u, const tString &serverGuid, const tString &age, const tString &externalIp, uint16_t spawnStart, uint16_t spawnStop);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		bool validSpawnPorts(void) { return spawnStart && spawnStop && spawnStart <= spawnStop; }
		// format
		tUruString serverGuid;
		tString age, externalIp;
		uint16_t spawnStart, spawnStop;
	};
	
	class tmCustomPlayerStatus : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomPlayerStatus, tmNetMsg)
	public:
		tmCustomPlayerStatus(tNetSession *u, tNetSession *playerSession, uint8_t playerFlag, uint8_t playerStatus);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tString account, avatar;
		uint8_t playerFlag, playerStatus;
	};
	
	class tmCustomFindServer : public tmNetMsg { // also used by vault
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomFindServer, tmNetMsg)
	public:
		tmCustomFindServer(tNetSession *u, const tmFindAge &findAge);
		tmCustomFindServer(tNetSession *u, const tmCustomFindServer &findServer);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tUruString serverGuid;
		tString age;
	};
	
	class tmCustomForkServer : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomForkServer, tmNetMsg)
	public:
		tmCustomForkServer(tNetSession *u, uint16_t port, const tString &serverGuid, const tString &name);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint16_t forkPort;
		tUruString serverGuid;
		tString age;
	};
	
	class tmCustomServerFound : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomServerFound, tmNetMsg)
	public:
		tmCustomServerFound(tNetSession *u, uint32_t ki, uint32_t x, uint32_t sid, uint16_t port, const tString &ipStr, const tString &serverGuid, const tString &name);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint16_t serverPort;
		tString ipStr;
		tUruString serverGuid;
		tString age;
	};
	
	class tmCustomDirectedFwd : public tmGameMessageDirected {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomDirectedFwd, tmGameMessageDirected)
	public:
		tmCustomDirectedFwd(tNetSession *u, tmGameMessageDirected &msg) : tmGameMessageDirected(NetMsgCustomDirectedFwd, u, msg) {}
		tmCustomDirectedFwd(tNetSession *u, uint32_t ki, tpObject *obj) : tmGameMessageDirected(NetMsgCustomDirectedFwd, u, ki, obj) {}
	};
	
	class tmCustomPlayerToCome : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmCustomPlayerToCome, tmNetMsg)
	public:
		tmCustomPlayerToCome(tNetSession *u, uint32_t ki);
		virtual void store(tBBuf &t);
	};
	
} //End alc namespace

#endif
