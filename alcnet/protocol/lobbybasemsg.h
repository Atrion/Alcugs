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

#ifndef __U_LOBBYBASEMSG_H
#define __U_LOBBYBASEMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_LOBBYBASEMSG_H_ID "$Id$"

#include "protocol.h"

namespace alc {

	class tmAuthenticateHello : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmAuthenticateHello, tmNetMsg)
	public:
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		tString account;
		uint16_t maxPacketSize;
		uint8_t release;
	};
	
	class tmAuthenticateChallenge : public tmNetMsg {
	public:
		tmAuthenticateChallenge(tNetSession *u, uint32_t x, uint8_t authResult, const uint8_t *challenge);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t authResult;
		tString challenge;
	};
	
	class tmAuthenticateResponse : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmAuthenticateResponse, tmNetMsg)
	public:
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		tString hash;
	};
	
	class tmAccountAutheticated : public tmNetMsg {
	public:
		tmAccountAutheticated(tNetSession *u, uint32_t x, uint8_t authResult, const uint8_t *serverGuid);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint8_t authResult;
		uint8_t serverGuid[8];
	};
	
	class tmSetMyActivePlayer : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmSetMyActivePlayer, tmNetMsg)
	public:
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		tString avatar;
	};
	
	class tmActivePlayerSet : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmActivePlayerSet, tmNetMsg)
	public:
		tmActivePlayerSet(tNetSession *u, uint32_t x);
	};
	
	class tmFindAge : public tmNetMsg {
		NETMSG_RECEIVE_CONSTRUCTORS(tmFindAge, tmNetMsg)
	public:
		tmFindAge(alc::tNetSession* u, uint32_t sid, const alc::tmFindAge& findAge);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tAgeLinkStruct link;
	};
	
	class tmFindAgeReply : public tmNetMsg {
	public:
		tmFindAgeReply(tNetSession *u, uint32_t x, const tString &ipStr, uint16_t port, const tString &age, const uint8_t *guid);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tString age, ipStr;
		uint16_t serverPort;
		uint8_t serverGuid[8];
	};
	
} //End alc namespace

#endif
