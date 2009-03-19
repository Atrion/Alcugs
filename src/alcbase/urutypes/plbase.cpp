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
			// message types
			case plAnimCmdMsg: return "plAnimCmdMsg";
			case plControlEventMsg: return "plControlEventMsg";
			case plLoadCloneMsg: return "plLoadCloneMsg";
			case plEnableMsg: return "plEnableMsg";
			case plWarpMsg: return "plWarpMsg";
			case plServerReplyMsg: return "plServerReplyMsg";
			case plAvTaskMsg: return "plAvTaskMsg";
			case plLinkToAgeMsg: return "plLinkToAgeMsg";
			case plNotifyMsg: return "plNotifyMsg";
			case plLinkEffectsTriggerMsg: return "plLinkEffectsTriggerMsg";
			case plParticleTransferMsg: return "plParticleTransferMsg";
			case plAvatarInputStateMsg: return "plAvatarInputStateMsg";
			case plLinkingMgrMsg: return "plLinkingMgrMsg";
			case plClothingMsg: return "plClothingMsg";
			case plInputIfaceMgrMsg: return "plInputIfaceMgrMsg";
			case pfKIMsg: return "pfKIMsg";
			case plAvBrainGenericMsg: return "plAvBrainGenericMsg";
			case plLoadAvatarMsg: return "plLoadAvatarMsg";
			case plSubWorldMsg: return "plSubWorldMsg";
			case plPseudoLinkEffectMsg: return "plPseudoLinkEffectMsg";
			// vault types
			case plAgeLinkStruct: return "plAgeLinkStruct";
			case plCreatableGenericValue: return "plCreatableGenericValue";
			case plCreatableStream: return "plCreatableStream";
			case plServerGuid: return "plServerGuid";
			case plVaultNodeRef: return "plVaultNodeRef";
			case plVaultNode: return "plVaultNode";
			// NULL type and unknown
			case plNull: return "plNull";
			default: return "Unknown";
		}
	}
	
	tpObject *alcCreatePlasmaObject(U16 type, bool mustBeComplete)
	{
		switch (type) {
			// known message types
			case plLoadCloneMsg: return new tpLoadCloneMsg();
			case plParticleTransferMsg: return new tpParticleTransferMsg();
			case pfKIMsg: return new tpKIMsg();
			case plLoadAvatarMsg: return new tpLoadAvatarMsg();
			// NULL type
			case plNull: return new tpObject(plNull); // the NULL object
			// unknown message types
			case plAnimCmdMsg:
			case plControlEventMsg:
			case plEnableMsg:
			case plWarpMsg:
			case plServerReplyMsg:
			case plAvTaskMsg: // I thought this was a plMessage, but the message sent when opening the KI looks... different
			case plLinkToAgeMsg:
			case plNotifyMsg:
			case plLinkEffectsTriggerMsg:
			case plAvatarInputStateMsg:
			case plLinkingMgrMsg:
			case plClothingMsg:
			case plInputIfaceMgrMsg:
			case plAvBrainGenericMsg:
			case plSubWorldMsg:
			case plPseudoLinkEffectMsg:
			case 0x0294: // FIXME: what are the names of these?
			case 0x039E:
				if (!mustBeComplete) return new tpMessage(type); // if mustBeComplete is true, go on with the default behaviour
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

