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

/* CVS tag - DON'T TOUCH*/
#define __U_PLBASE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	const char *alcGetPlasmaType(U16 type)
	{
		switch(type) {
			case plControlEventMsg: return "plControlEventMsg";
			case plLoadCloneMsg: return "plLoadCloneMsg";
			case plEnableMsg: return "plEnableMsg";
			case plWarpMsg: return "plWarpMsg";
			case plServerReplyMsg: return "plServerReplyMsg";
			case plAvTaskMsg: return "plAvTaskMsg";
			case plNotifyMsg: return "plNotifyMsg";
			case plLinkEffectsTriggerMsg: return "plLinkEffectsTriggerMsg";
			case plParticleTransferMsg: return "plParticleTransferMsg";
			case plAvatarInputStateMsg: return "plAvatarInputStateMsg";
			case pfKIMsg: return "pfKIMsg";
			case plAvBrainGenericMsg: return "plAvBrainGenericMsg";
			case plLoadAvatarMsg: return "plLoadAvatarMsg";
			case plSubWorldMsg: return "plSubWorldMsg";
			case plNull: return "plNull";
			default: return "Unknown";
		}
	}
	
	tpObject *alcCreatePlasmaObject(U16 type, bool mustBeComplete)
	{
		switch (type) {
			case plLoadCloneMsg: return new tpLoadCloneMsg();
			case plParticleTransferMsg: return new tpParticleTransferMsg();
			case plLoadAvatarMsg: return new tpLoadAvatarMsg();
			case plNull: return new tpObject(plNull);
			// messages of which the details are unknown
			case plControlEventMsg:
			case plEnableMsg:
			case plWarpMsg:
			case plServerReplyMsg:
			case plAvTaskMsg:
			case plNotifyMsg:
			case plLinkEffectsTriggerMsg:
			case plAvatarInputStateMsg:
			case pfKIMsg:
			case plAvBrainGenericMsg:
			case plSubWorldMsg:
			 // FIXME: what are the names of these?
			case 0x0206:
			case 0x0294:
			case 0x02E1:
			case 0x0346:
			case 0x0352:
			case 0x035E:
			case 0x039E:
				if (!mustBeComplete) return new tpMessage(type); // if mustBeComplete is true, go on with the default behaviour
			// completely unknown types
			default:
				throw txUnexpectedData(_WHERE("Unknown message type %s (0x%04X)", alcGetPlasmaType(type), type));
		}
	}
	
	//// tpObject
	const char *tpObject::str(void)
	{
		strBuf.clear();
		strBuf.printf("%s (0x%04X)\n", alcGetPlasmaType(type), type);
		toString();
		return strBuf.c_str();
	}

} //end namespace alc

