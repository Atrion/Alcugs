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

#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include <algorithm>

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	tPageInfo::tPageInfo(tConfigVal *val, int row)
	{
		tStrBuf name = val->getVal(0, row), number = val->getVal(1, row), conditionalLoad = val->getVal(2, row);
		strncpy(this->name, name.c_str(), 199);
		this->number = number.asU16();
		if (conditionalLoad.isNull()) this->conditionalLoad = false;
		else {
			if (conditionalLoad != "1") throw txBase(_WHERE("if a conditional load value is specified, it must be set to 1"));
			this->conditionalLoad = true;
		}
		DBG(9, "New Page %s: Number %d, conditionalLoad %d\n", this->name, this->number, this->conditionalLoad);
		
		owner = 0;
		pageId = pageType = 0;
	}
	
	bool tPageInfo::hasPlayer(U32 ki) const
	{
		return std::find(players.begin(), players.end(), ki) != players.end();
	}
	
	bool tPageInfo::removePlayer(U32 ki)
	{
		tPlayerList::iterator it = std::find(players.begin(), players.end(), ki);
		if (it == players.end()) return false;
		players.erase(it);
		return true;
	}
	
	tAgeInfo::tAgeInfo(const tStrBuf &dir, const char *file, bool loadPages)
	{
		// get age name from file name
		strncpy(name, file, 199);
		alcStripExt(name);
		// open and decrypt file
		tFBuf ageFile;
		ageFile.open((dir + file).c_str(), "r");
		tWDYSBuf ageContent;
		ageContent.put(ageFile);
		ageContent.decrypt();
		// parse file
		tConfig *cfg = new tConfig;
		tXParser parser(/*override*/false);
		parser.setConfig(cfg);
		ageContent.get(parser);
		// get sequence prefix
		tStrBuf prefix = cfg->getVar("SequencePrefix");
		if (prefix.isNull()) throw txUnexpectedData(_WHERE("can\'t find the ages SequencePrefix"));
		seqPrefix = prefix.asU32();
		if (seqPrefix > 0x00FFFFFF && seqPrefix < 0xFFFFFFF0) // allow only 3 Bytes (but allow negative prefixes)
			throw txUnexpectedData(_WHERE("A sequence prefix of %d (higher than 0x00FFFFFF) is not allowed)", seqPrefix));
		DBGM(9, "found sequence prefix %d for age %s\n", seqPrefix, name);
		// get pages
		if (loadPages) {
			tConfigVal *pageVal = cfg->findVar("Page");
			int nPages = pageVal ? pageVal->getRows() : 0;
			if (!nPages) throw txBase(_WHERE("an age without pages? This is not possible"));
			for (int i = 0; i < nPages; ++i) {
				tPageInfo pageInfo(pageVal, i);
				pages.insert(std::pair<U32, tPageInfo>(pageInfo.number, pageInfo));
			}
		}
		// done!
		delete cfg;
	}
	
	tPageInfo *tAgeInfo::getPage(U32 pageId)
	{
		U16 number = alcPageIdToNumber(seqPrefix, pageId);
		tPageList::iterator it = pages.find(number);
		return (it == pages.end() ? NULL : &it->second);
	}
	
	bool tAgeInfo::validPage(U32 pageId) const
	{
		U16 number = alcPageIdToNumber(seqPrefix, pageId);
		if (number == 254) return true; // BuiltIn page
		return pages.count(number);
	}
	
	tAgeInfoLoader::tAgeInfoLoader(const char *name, bool loadPages)
	{
		// load age file dir and age files
		tConfig *cfg = alcGetConfig();
		tStrBuf dir = cfg->getVar("age");
		if (dir.size() < 2) throw txBase(_WHERE("age directory is not defined"));
		if (!dir.endsWith("/")) dir.writeStr("/");
		
		if (!name) { // we should load all ages
			lstd->log("Reading age files from %s\n", dir.c_str());
			lstd->flush();
			
			tDirectory ageDir;
			tDirEntry *file;
			ageDir.open(dir.c_str());
			while( (file = ageDir.getEntry()) != NULL) {
				if (!file->isFile() || strcasecmp(alcGetExt(file->name), "age") != 0) continue;
				// load it
				DBG(9, "Reading age file %s%s:", dir.c_str(), file->name);
				ages.push_back(tAgeInfo(dir, file->name, loadPages));
			}
		}
		else { // we should load only one certain age
			lstd->log("Reading age file %s%s.age\n", dir.c_str(), name);
			lstd->flush();
			
			char filename[200];
			sprintf(filename, "%s.age", name);
			// load it
			ages.push_back(tAgeInfo(dir, filename, loadPages));
		}
		DBG(9, "Done reading age file(s)\n");
	}
	
	tAgeInfo *tAgeInfoLoader::getAge(const char *name)
	{
		for (tAgeList::iterator i = ages.begin(); i != ages.end(); ++i) {
			if (strcmp(i->name, name) == 0) return &(*i);
		}
		return NULL;
	}

} //end namespace alc

