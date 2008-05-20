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

#define _DBG_LEVEL_ 10

#include <alcugs.h>

////extra includes
#include "guidgen.h"

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	tAgeParser::tAgeFile::tAgeFile(const char *dir, const char *file)
	{
		char filename[1024];
		strncpy(filename, dir, 512);
		strncat(filename, file, 511);
		DBG(9, "opening %s\n", filename);
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
			ages = (tAgeFile **)realloc(ages, size*sizeof(tAgeFile*));
			ages[size-1] = new tAgeFile(dir, file->name);
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
	
	tGuidGen::tGuidGen(void)
	{
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("age");
		if (!var.endsWith("/")) var.writeStr("/");
		ageParser = new tAgeParser((char *)var.c_str());
	}

} //end namespace alc

