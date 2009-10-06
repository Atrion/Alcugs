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
		bool hasPlayer(U32 ki) const;
		bool removePlayer(U32 ki); //!< \returns false if that players was not on the list, true if it got removed
		
		char name[200];
		U16 number;
		bool conditionalLoad;
		
		// data for the messages (filled when we get a NetMsgPagingRom for this page)
		U32 owner;
		U32 pageId; // this and the next one are used when we send a NetMsgGroupOwner
		U16 pageType;
		tPlayerList players;
	};
	
	class tAgeInfo
	{
	public:
		tAgeInfo(const char *file, bool loadPages);
		tPageInfo *getPage(U32 pageId);
		bool validPage(U32 pageId) const;
		
		U32 seqPrefix; // it's actually 3 Bytes
		char name[200];
	
		typedef std::map<U16, tPageInfo> tPageList;
		tPageList pages;
	};

} //End alc namespace

#endif
