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

#ifndef __U_LOBBYBASEMSG_H
#define __U_LOBBYBASEMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_LOBBYBASEMSG_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	class tmAuthenticateHello : public tmMsgBase {
	public:
		tmAuthenticateHello(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		tString account;
		U16 maxPacketSize;
		Byte release;
	};
	
	class tmAuthenticateChallenge : public tmMsgBase {
	public:
		tmAuthenticateChallenge(tNetSession *u, U32 x, Byte authResult, const Byte *challenge);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		Byte authResult;
		tString challenge;
	};
	
	class tmAuthenticateResponse : public tmMsgBase {
	public:
		tmAuthenticateResponse(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		tString hash;
	};
	
	class tmAccountAutheticated : public tmMsgBase {
	public:
		tmAccountAutheticated(tNetSession *u, U32 x, Byte authResult, const Byte *serverGuid);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		Byte authResult;
		Byte serverGuid[8];
	};
	
	class tmSetMyActivePlayer : public tmMsgBase {
	public:
		tmSetMyActivePlayer(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		tString avatar;
	};
	
	class tmActivePlayerSet : public tmMsgBase {
	public:
		tmActivePlayerSet(tNetSession *u, U32 x);
	};
	
	class tmFindAge : public tmMsgBase {
	public:
		tmFindAge(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		tMBuf message;
	};
	
	class tmFindAgeReply : public tmMsgBase {
	public:
		tmFindAgeReply(tNetSession *u, U32 x, const tString &ipStr, U16 port, const tString &age, const Byte *guid);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tString age, ipStr;
		U16 serverPort;
		Byte serverGuid[8];
	};
	
} //End alc namespace

#endif
