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

#ifndef __U_AUTHMSG_H
#define __U_AUTHMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_AUTHMSG_H_ID "$Id$"

#include "protocol.h"

namespace alc {

	class tmCustomAuthAsk : public tmMsgBase {
	public:
		tmCustomAuthAsk(tNetSession *u);
		tmCustomAuthAsk(tNetSession *u, uint32_t x, uint32_t sid, uint32_t ip, tString login, const uint8_t *challenge, const uint8_t *hash, uint8_t release);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint32_t ip; //network order
		tString login;
		uint8_t challenge[16], hash[16];
		uint8_t release;
	};
	
	class tmCustomAuthResponse : public tmMsgBase {
	public:
		tmCustomAuthResponse(tNetSession *u);
		tmCustomAuthResponse(tNetSession *u, tmCustomAuthAsk &authAsk, const uint8_t *uid, tString passwd, uint8_t result, uint8_t accessLevel);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tString login, passwd;
		uint8_t result, accessLevel;
	};
	
} //End alc namespace

#endif
