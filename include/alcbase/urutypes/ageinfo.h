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

#ifndef __U_AGEINFO_H
#define __U_AGEINFO_H
/* CVS tag - DON'T TOUCH*/
#define __U_AGEINFO_H_ID "$Id$"


namespace alc {

	////DEFINITIONS
	class tPageInfo
	{
	public:
		typedef std::vector<U32> tPlayerList;
		
		tPageInfo(tConfigVal *val, int row);
		tPlayerList::iterator getPlayer(U32 ki);
		inline bool hasPlayer(U32 ki) { return getPlayer(ki) != players.end(); }
		
		char name[200];
		U16 id;
		bool conditionalLoad;
		U32 owner;
		
		// data for the messages (filled when we get a NetMsgPagingRom for this page)
		U32 plasmaPageId;
		U16 plasmaPageType;
		
		tPlayerList players;
	};
	
	class tAgeInfo
	{
	public:
		tAgeInfo(const tStrBuf &dir, const char *file, bool loadPages);
		tPageInfo *getPage(U32 pageId);
		
		U32 seqPrefix; // it's actually 3 Bytes
		char name[200];
		
		typedef std::vector<tPageInfo> tPageList;
		tPageList pages;
	};
	
	class tAgeInfoLoader
	{
	public:
		tAgeInfoLoader(const char *name = NULL, bool loadPages = false);
		tAgeInfo *getAge(const char *name);
	private:
		typedef std::vector<tAgeInfo> tAgeList;
		tAgeList ages;
	};

} //End alc namespace

#endif
