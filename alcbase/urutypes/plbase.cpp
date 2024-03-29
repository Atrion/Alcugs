/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2020  The Alcugs Server Team                           *
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

//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "plbase.h"

#include "plmessage.h"
#include "alcexception.h"

namespace alc {

	const char *alcGetPlasmaType(uint16_t type)
	{
		switch(type) {
			// message types
			case plAnimCmdMsg: return "plAnimCmdMsg";
			case plControlEventMsg: return "plControlEventMsg";
			case plLoadCloneMsg: return "plLoadCloneMsg";
			case plEnableMsg: return "plEnableMsg";
			case plWarpMsg: return "plWarpMsg";
			case plServerReplyMsg: return "plServerReplyMsg";
			case plAvatarMsg: return "plAvatarMsg";
			case plAvTaskMsg: return "plAvTaskMsg";
			case plAvSeekMsg: return "plAvSeekMsg";
			case plAvOneShotMsg: return "plAvOneShotMsg";
			case plLinkToAgeMsg: return "plLinkToAgeMsg";
			case plNotifyMsg: return "plNotifyMsg";
			case plLinkEffectsTriggerMsg: return "plLinkEffectsTriggerMsg";
			case plOneShotMsg: return "plOneShotMsg";
			case plParticleTransferMsg: return "plParticleTransferMsg";
			case plParticleKillMsg: return "plParticleKillMsg";
			case plAvatarInputStateMsg: return "plAvatarInputStateMsg";
			case plLinkingMgrMsg: return "plLinkingMgrMsg";
			case plClothingMsg: return "plClothingMsg";
			case plInputIfaceMgrMsg: return "plInputIfaceMgrMsg";
			case pfKIMsg: return "pfKIMsg";
			case plAvBrainGenericMsg: return "plAvBrainGenericMsg";
			case plMultistageModMsg: return "plMultistageModMsg";
			case plBulletMsg: return "plBulletMsg";
			case plLoadAvatarMsg: return "plLoadAvatarMsg";
			case plSubWorldMsg: return "plSubWorldMsg";
			case plClimbMsg: return "plClimbMsg";
			case pfMarkerMsg: return "pfMarkerMsg";
			case plAvCoopMsg: return "plAvCoopMsg";
			case plSetNetGroupIDMsg: return "plSetNetGroupIDMsg";
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
	
	//// tpObject
	tpObject *tpObject::createByType(uint16_t type, bool mustBeComplete)
	{
		switch (type) {
			// known message types
			case plLoadCloneMsg: return new tpLoadCloneMsg;
			case plServerReplyMsg: return new tpServerReplyMsg;
			case plAvatarMsg: return new tpAvatarMsg;
			case plParticleTransferMsg: return new tpParticleTransferMsg;
			case plAvatarInputStateMsg: return new tpAvatarInputStateMsg;
			case plAvBrainGenericMsg: return new tpAvBrainGenericMsg;
			case pfKIMsg: return new tpKIMsg;
			case plLoadAvatarMsg: return new tpLoadAvatarMsg;
			// NULL type
			case plNull: return new tpObject(plNull); // the NULL object
			// unknown message types
			case plAnimCmdMsg:
			case plControlEventMsg:
			case plEnableMsg:
			case plWarpMsg:
			case plAvTaskMsg:
			case plAvSeekMsg:
			case plAvOneShotMsg:
			case plLinkToAgeMsg:
			case plNotifyMsg:
			case plLinkEffectsTriggerMsg:
			case plOneShotMsg:
			case plParticleKillMsg:
			case plLinkingMgrMsg:
			case plClothingMsg:
			case plInputIfaceMgrMsg:
			case plMultistageModMsg:
			case plBulletMsg:
			case plSubWorldMsg:
			case plClimbMsg:
			case pfMarkerMsg:
			case plAvCoopMsg:
			case plSetNetGroupIDMsg:
			case plPseudoLinkEffectMsg:
			case pfClimbingWallMsg:
				if (!mustBeComplete) return new tpMessage(type, /*incomplete*/true);
				// if mustBeComplete is true, go on with the default behaviour
			default:
				throw txUnexpectedData(_WHERE("Unknown message type %s (0x%04X)", alcGetPlasmaType(type), type));
		}
	}
	
	tpObject* tpObject::createFromStream(alc::tStreamedObject* stream, bool UUFormat, bool mustBeComplete)
	{
		tpObject *obj = createByType(UUFormat ? alcOpcodeUU2POTS(stream->getType()) : stream->getType(), mustBeComplete);
		obj->setUUFormat(UUFormat);
		stream->get(*obj);
		if (!obj->isIncomplete()) stream->eofCheck();
		return obj;
	}
	
	tString tpObject::str(void) const
	{
		tString strBuf;
		strBuf.printf("%s (0x%04X)\n", alcGetPlasmaType(type), type);
		return strBuf;
	}

} //end namespace alc

