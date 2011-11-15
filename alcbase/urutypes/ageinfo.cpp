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

//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "ageinfo.h"

#include "alcmain.h"
#include "alcexception.h"
#include "alcutil/alcos.h"
#include "urubasetypes.h"
#include "alcutil/alcparser.h"

namespace alc {
	
	tAgeInfo::tAgeInfo(const tString &dir, const tString &name) : name(name)
	{
		delete parseFile(dir + "/" + name + ".age");
	}
	
	tAgeInfo::tAgeInfo(const tString &name) : name(name)
	{}
	
	tConfig *tAgeInfo::parseFile(const tString &filename)
	{
		// open and decrypt file
		tFBuf ageFile;
		ageFile.open(filename.c_str(), "r");
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
		seqPrefix = prefix.asInt();
		if (seqPrefix > 0x00FFFFFF && seqPrefix < 0xFFFFFFF0) // allow only 3 Bytes (but allow negative prefixes)
			throw txUnexpectedData(_WHERE("A sequence prefix of %d (higher than 0x00FFFFFF) is not allowed)", seqPrefix));
		DBG(9, "found sequence prefix %d for age %s\n", seqPrefix, name.c_str());
		return cfg;
	}
	
	bool tAgeInfo::validPage(uint32_t pageId) const
	{
		try {
			alcPageIdToNumber(seqPrefix, pageId);
			return true;
		}
		catch (const txBase &) {
			return false;
		}
	}


} //end namespace alc

