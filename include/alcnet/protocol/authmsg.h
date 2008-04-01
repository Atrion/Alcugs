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

/**
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

#ifndef __U_AUTHMSG_H
#define __U_AUTHMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_AUTHMSG_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	class tmCustomAuthAsk : public tmMsgBase {
	public:
		tmCustomAuthAsk(tNetSession *s) : tmMsgBase(NetMsgCustomAuthAsk, 0, s) { } // it's not capable of sending a package, so no flags are set
		virtual void store(tBBuf &t);
		// format
		tUStr login;
		Byte challenge[16], hash[16];
		Byte release;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomAuthResponse : public tmMsgBase {
	public:
		tmCustomAuthResponse(tNetSession *s, tmCustomAuthAsk &authAsk, const Byte *guid, Byte *passwd, Byte result, Byte accessLevel);
		virtual int stream(tBBuf &t);
		// format
		tUStr login, passwd;
		Byte result, accessLevel;
	protected:
		virtual void additionalFields();
	};
	
} //End alc namespace

#endif
