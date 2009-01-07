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

#ifndef __U_GAMESUBMSG_H
#define __U_GAMESUBMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_GAMESUBMSG_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	class tLoadAvatarMsg : public tBaseType {
	public:
		tLoadAvatarMsg(void) : tBaseType() {}
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		void check(tmLoadClone &loadClone);
		// format
		U32 unk7; // seen 0x00000840 and 0x00000040
		tUruObject clonedObj;
		bool isLoad;
		bool isPlayerAvatar;
		bool hasParentObj; // when true, the message contains the parent object
		tUruObject parentObj;
	};

} //End alc namespace

#endif
