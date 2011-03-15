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

#ifndef __U_AGEINFO_H
#define __U_AGEINFO_H
/* CVS tag - DON'T TOUCH*/
#define __U_AGEINFO_H_ID "$Id$"

#include "alctypes.h"

#include <vector>
#include <map>


namespace alc {
	
	class tConfigVal;

	class tPageInfo
	{
	public:
		typedef std::vector<uint32_t> tPlayerList;
		
		tPageInfo(tConfigVal *val, int row);
		bool hasPlayer(uint32_t ki) const;
		bool removePlayer(uint32_t ki); //!< \returns false if that players was not on the list, true if it got removed
		
		tString name;
		uint16_t number;
		bool conditionalLoad;
		
		// data for the messages (filled when we get a NetMsgPagingRom for this page)
		uint32_t owner;
		uint32_t pageId; // this and the next one are used when we send a NetMsgGroupOwner
		uint16_t pageType;
		tPlayerList players;
	};
	
	class tAgeInfo
	{
	public:
		tAgeInfo(tString dir, const tString &name, bool loadPages);
		tPageInfo *getPage(uint32_t pageId);
		bool validPage(uint32_t pageId) const;
		
		uint32_t seqPrefix; // it's actually 3 Bytes
		tString name;
	
		typedef std::map<uint16_t, tPageInfo> tPageList;
		tPageList pages;
	};

} //End alc namespace

#endif
