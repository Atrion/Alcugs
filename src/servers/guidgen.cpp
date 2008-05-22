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

////extra includes
#include "guidgen.h"

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	tAgeInfo::tAgeInfo(const char *dir, const char *file)
	{
		// get age name from file name
		strncpy((char *)name, file, 199);
		alcStripExt((char *)name);
		// assemble file name
		char filename[1024];
		strncpy(filename, dir, 512);
		strncat(filename, file, 511);
		DBG(9, "opening %s... ", filename);
		// open and decrypt file
		tFBuf ageFile;
		ageFile.open(filename, "r");
		tWDYSBuf ageContent;
		ageFile.get(ageContent);
		ageContent.decrypt();
		// parse file
		tConfig *cfg = new tConfig;
		tXParser parser;
		parser.setConfig(cfg);
		ageContent.get(parser);
		// get sequence prefix
		tStrBuf prefix = cfg->getVar("SequencePrefix");
		if (prefix.isNull()) throw txParseError(_WHERE("can\'t find the ages SequencePrefix"));
		seqPrefix = prefix.asU16();
		// done!
		DBGM(9, "found sequence prefix %d for age %s\n", seqPrefix, name);
		delete cfg;
	}
	
	tAgeParser::tAgeParser(const char *dir)
	{
		lstd->log("reading age files from %s\n", dir);
		lstd->flush();
		
		size = 0;
		ages = NULL;
		
		tDirectory ageDir;
		tDirEntry *file;
		ageDir.open(dir);
		while( (file = ageDir.getEntry()) != NULL) {
			if (file->type != 8 || strcasecmp(alcGetExt(file->name), "age") != 0) continue;
			
			// grow the array
			++size;
			ages = (tAgeInfo **)realloc(ages, size*sizeof(tAgeInfo*));
			ages[size-1] = new tAgeInfo(dir, file->name);
		}
	}
	
	tAgeParser::~tAgeParser(void)
	{
		if (ages != NULL) {
			for (int i = 0; i < size; ++i) {
				if (ages[i]) delete ages[i];
			}
			free(ages);
		}
	}
	
	tAgeInfo *tAgeParser::getAge(const Byte *name)
	{
		for (int i = 0; i < size; ++i) {
			if (!ages[i]) continue;
			if (strcmp((char *)ages[i]->name, (char *)name) == 0) return ages[i];
		}
		return NULL;
	}
	
	tGuidGen::tGuidGen(void)
	{
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("age");
		if (!var.endsWith("/")) var.writeStr("/");
		ageParser = new tAgeParser((char *)var.c_str());
		// load the list of private ages
		var = cfg->getVar("private_ages");
		if (var.isNull()) strcpy((char *)privateAges, "AvatarCustomization,Personal,Nexus,BahroCave");
		else strncpy((char *)privateAges, (char *)var.c_str(), 1023);
		// load instance mode setting
		var = cfg->getVar("instance_mode");
		if (var.isNull()) instanceMode = 1;
		else instanceMode = var.asByte();
		if (instanceMode != 0 && instanceMode != 1) throw txBase(_WHERE("instance_mode must be 0 or 1"));
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
		tAgeInfo *ageInfo = ageParser->getAge(age);
		if (!ageInfo) return false;
		bool isPrivate = (instanceMode == 1) ? isAgePrivate(age) : false;
		
		/* so we have "The server GUID, aka age guid"
		---------------------------------
		| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
		--------------------------------
		| 0 | ki here       | 0 | s | s |
		--------------------------------
	
		Where s is the sequence prefix.
		This is only a preliminar usage of the age guid. Using the player id, as part of the age,
		we will be completely sure, that all players, at least will have only one instance for his
		own age.
		The 5 byte is reserved for a random number for the hoods, and any other age (for the future)
		And the 1st bit of the 4 byte, should be always 0 (since the Ki number is a signed value, this
		Will happen always. */
		memset(guid, 8, 0);
		if (isPrivate) *(U32 *)(guid+1) = ki;
		*(U16 *)(guid+6) = ageInfo->seqPrefix;
		return true;
	}

} //end namespace alc

