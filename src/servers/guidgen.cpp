/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Project Server Team                   *
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
#define __U_GUIDGEN_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>

////extra includes
#include "guidgen.h"

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	tGuidGen::tGuidGen(void)
	{
		// load age files
		ageInfos = new tAgeInfoLoader();
		// load the list of private ages
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("private_ages");
		if (var.isNull()) strcpy((char *)privateAges, "AvatarCustomization,Personal,Nexus,BahroCave,DniCityX2Finale");
		else strncpy((char *)privateAges, (char *)var.c_str(), 1023);
		// load instance mode setting
		var = cfg->getVar("instance_mode");
		if (var.isNull()) instanceMode = 1;
		else instanceMode = var.asByte();
		if (instanceMode != 0 && instanceMode != 1) throw txBase(_WHERE("instance_mode must be 0 or 1 but is %d", instanceMode));
	}
	
	bool tGuidGen::isAgePrivate(const Byte *age)
	{
		// local copy of private age list as strsep modifies it
		char ages[1024];
		strcpy(ages, (char *)privateAges);
		
		char *buf = ages;
		char *p = strsep(&buf, ",");
		while (p != 0) {
			if (strcmp(p, (char *)age) == 0) return true;
			p = strsep(&buf, ",");
		}
		return false;
	}
	
	bool tGuidGen::generateGuid(Byte *guid, const Byte *age, U32 ki)
	{
		tAgeInfo *ageInfo = getAge(age);
		if (!ageInfo) return false;
		if (ageInfo->seqPrefix > 0x00FFFFFF) return false; // obviously he wants to link to an age like GlobalMarkers
		bool isPrivate = (instanceMode == 1) ? isAgePrivate(age) : false;
		
		/* so we have "The server GUID, aka age guid"
		---------------------------------
		| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
		--------------------------------
		| 0 | ki here       | s | s | s |
		--------------------------------
	
		Where s is the sequence prefix.
		This is only a preliminar usage of the age guid. Using the player id, as part of the age,
		we will be completely sure, that all players, at least will have only one instance for his
		own age.
		The 0 byte is reserved for a random number for the hoods, and any other age (for the future).
		And the 1st bit of the 4 byte, should be always 0 (since the Ki number is a signed value, this
		will happen always. */
		tMBuf buf;
		buf.putByte(0);
		buf.putU32(isPrivate ? ki : 0);
		
		// 3 byte sequence prefix: First the one which usually is zero, to keep compatability
		buf.putByte(ageInfo->seqPrefix >> 16);
		// then the remaining two bytes
		buf.putU16(ageInfo->seqPrefix & 0x0000FFFF);
		
		buf.rewind();
		memcpy(guid, buf.read(8), 8);
		return true;
	}

} //end namespace alc

