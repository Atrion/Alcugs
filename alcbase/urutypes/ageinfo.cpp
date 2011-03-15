/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

/* CVS tag - DON'T TOUCH*/
#define __U_AGEINFO_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "ageinfo.h"

#include "alcmain.h"
#include "alcexception.h"
#include "alcutil/alcos.h"
#include "urubasetypes.h"
#include "alcutil/alcparser.h"

#include <algorithm>

namespace alc {

	tPageInfo::tPageInfo(tConfigVal *val, int row)
	{
		tString number = val->getVal(1, row), conditionalLoad = val->getVal(2, row);
		name = val->getVal(0, row);
		this->number = number.asUInt();
		if (conditionalLoad.isEmpty()) this->conditionalLoad = false;
		else {
			if (conditionalLoad != "1") throw txBase(_WHERE("if a conditional load value is specified, it must be set to 1"));
			this->conditionalLoad = true;
		}
		DBG(9, "New Page %s: Number %d, conditionalLoad %d\n", this->name.c_str(), this->number, this->conditionalLoad);
		
		owner = 0;
		pageId = pageType = 0;
	}
	
	bool tPageInfo::hasPlayer(uint32_t ki) const
	{
		return std::find(players.begin(), players.end(), ki) != players.end();
	}
	
	bool tPageInfo::removePlayer(uint32_t ki)
	{
		tPlayerList::iterator it = std::find(players.begin(), players.end(), ki);
		if (it == players.end()) return false;
		players.erase(it);
		return true;
	}
	
	tAgeInfo::tAgeInfo(tString dir, const tString &name, bool loadPages)
	{
		// open and decrypt file
		tFBuf ageFile;
		ageFile.open((dir + "/" + name + ".age").c_str(), "r");
		tWDYSBuf ageContent;
		ageContent.put(ageFile);
		ageContent.decrypt(/*mustBeWDYS*/false);
		// parse file
		tConfig *cfg = new tConfig;
		tXParser parser(/*override*/false);
		parser.setConfig(cfg);
		ageContent.get(parser);
		// get sequence prefix
		tString prefix = cfg->getVar("SequencePrefix");
		if (prefix.isEmpty()) throw txUnexpectedData(_WHERE("can\'t find the ages SequencePrefix"));
		seqPrefix = prefix.asUInt();
		if (seqPrefix > 0x00FFFFFF && seqPrefix < 0xFFFFFFF0) // allow only 3 Bytes (but allow negative prefixes)
			throw txUnexpectedData(_WHERE("A sequence prefix of %d (higher than 0x00FFFFFF) is not allowed)", seqPrefix));
		DBG(9, "found sequence prefix %d for age %s\n", seqPrefix, name.c_str());
		// get pages
		if (loadPages) {
			tConfigVal *pageVal = cfg->findVar("Page");
			int nPages = pageVal ? pageVal->getRows() : 0;
			if (!nPages) throw txBase(_WHERE("an age without pages? This is not possible"));
			for (int i = 0; i < nPages; ++i) {
				tPageInfo pageInfo(pageVal, i);
				pages.insert(std::pair<uint32_t, tPageInfo>(pageInfo.number, pageInfo));
			}
		}
		// done!
		delete cfg;
	}
	
	tPageInfo *tAgeInfo::getPage(uint32_t pageId)
	{
		uint16_t number = alcPageIdToNumber(seqPrefix, pageId);
		DBG(9, "pageId 0x%08X => number %d, existing: %Zd\n", pageId, number, pages.count(number));
		tPageList::iterator it = pages.find(number);
		return (it == pages.end() ? NULL : &it->second);
	}
	
	bool tAgeInfo::validPage(uint32_t pageId) const
	{
		uint16_t number = alcPageIdToNumber(seqPrefix, pageId);
		DBG(9, "pageId 0x%08X => number %d, existing: %Zd\n", pageId, number, pages.count(number));
		if (number == 254) return true; // BuiltIn page (accept because we can get SDL states for it)
		return pages.count(number);
	}


} //end namespace alc

