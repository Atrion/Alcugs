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
#define __U_AGEINFO_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "alcnet.h"

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	tPageInfo::tPageInfo(tConfigVal *val, int row)
	{
		tStrBuf name = val->getVal(0, row), id = val->getVal(1, row), conditionalLoad = val->getVal(2, row);
		strncpy((char *)this->name, (char *)name.c_str(), 199);
		this->id = id.asU16();
		if (conditionalLoad.isNull()) this->conditionalLoad = false;
		else {
			if (conditionalLoad != "1") throw txBase(_WHERE("if a conditional load value is specified, it must be set to 1"));
			this->conditionalLoad = conditionalLoad.asByte();
		}
		DBG(9, "New Page %s: ID %d, conditionalLoad %d\n", this->name, this->id, this->conditionalLoad);
	}
	
	tAgeInfo::tAgeInfo(const char *dir, const char *file, bool loadPages)
	{
		// get age name from file name
		strncpy((char *)name, file, 199);
		alcStripExt((char *)name);
		// assemble file name
		char filename[1024];
		strncpy(filename, dir, 512);
		strncat(filename, file, 511);
		// open and decrypt file
		tFBuf ageFile;
		ageFile.open(filename, "r");
		tWDYSBuf ageContent;
		ageFile.get(ageContent);
		ageContent.decrypt();
		// parse file
		tConfig *cfg = new tConfig;
		tXParser parser(/*override*/false);
		parser.setConfig(cfg);
		ageContent.get(parser);
		// get sequence prefix
		tStrBuf prefix = cfg->getVar("SequencePrefix");
		if (prefix.isNull()) throw txParseError(_WHERE("can\'t find the ages SequencePrefix"));
		seqPrefix = prefix.asU32();
		if (seqPrefix > 0x00FFFFFF && seqPrefix < 0xFFFFFFF0) // allow only 3 Bytes (but allow negative prefixes)
			throw txUnexpectedData(_WHERE("A sequence prefix of %d (higher than 0x00FFFFFF) is not allowed)", seqPrefix));
		DBGM(9, "found sequence prefix %d for age %s\n", seqPrefix, name);
		// get pages
		if (loadPages) {
			tConfigVal *pageVal = cfg->findVar("Page");
			nPages = pageVal ? pageVal->getRows() : 0;
			if (!nPages) throw txBase(_WHERE("an age without pages? This is not possible"));
			pages = (tPageInfo *)malloc(nPages*sizeof(tPageInfo));
			if (pages==NULL) throw txNoMem(_WHERE("NoMem"));
			for (int i = 0; i < nPages; ++i)
				pages[i] = tPageInfo(pageVal, i);
		}
		else {
			nPages = 0;
			pages = NULL;
		}
		// done!
		delete cfg;
	}
	
	tAgeInfo &tAgeInfo::operator=(const tAgeInfo &ageInfo)
	{
		seqPrefix = ageInfo.seqPrefix;
		strncpy((char *)name, (char *)ageInfo.name, 199);
		nPages = ageInfo.nPages;
		if (nPages) {
			pages = (tPageInfo *)malloc(nPages*sizeof(tPageInfo));
			if (pages==NULL) throw txNoMem(_WHERE("NoMem"));
			memcpy(pages, ageInfo.pages, nPages*sizeof(tPageInfo));
		}
		else
			pages = NULL;
		return *this;
	}
	
	tAgeInfoLoader::tAgeInfoLoader(const Byte *name, bool loadPages)
	{
		// load age file dir and age files
		tConfig *cfg = alcGetConfig();
		tStrBuf dir = cfg->getVar("age");
		if (dir.size() < 2) throw txBase(_WHERE("age directory is not defined"));
		if (!dir.endsWith("/")) dir.writeStr("/");
		
		if (!name) { // we should load all ages
			lstd->log("reading age files from %s\n", dir.c_str());
			lstd->flush();
			
			tDirectory ageDir;
			tDirEntry *file;
			ageDir.open((char *)dir.c_str());
			while( (file = ageDir.getEntry()) != NULL) {
				if (file->type != 8 || strcasecmp(alcGetExt(file->name), "age") != 0) continue;
				// load it
				ages.push_back(tAgeInfo((char *)dir.c_str(), file->name, loadPages));
			}
		}
		else { // we should load only one certain age
			lstd->log("reading age file %s%s.age\n", dir.c_str(), name);
			lstd->flush();
			
			char filename[200];
			sprintf(filename, "%s.age", name);
			// load it
			ages.push_back(tAgeInfo((char *)dir.c_str(), filename, loadPages));
		}
	}
	
	tAgeInfo *tAgeInfoLoader::getAge(const Byte *name)
	{
		for (std::vector<tAgeInfo>::iterator i = ages.begin(); i != ages.end(); ++i) {
			if (strcmp((char *)i->name, (char *)name) == 0) return &(*i);
		}
		return NULL;
	}

} //end namespace alc

