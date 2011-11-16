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

#include "alctypes.h"


namespace alc {

	class tAgeInfo
	{
	public:
		tAgeInfo(const tString &dir, const tString &name);
		virtual ~tAgeInfo() {}
		
		bool validPage(uint32_t pageId) const;
		uint32_t getAgePrefix() const { return seqPrefix; }
		const tString &getAgeName() const { return name; }
	protected:
		tAgeInfo(const tString &name);
		/** Parses the age file, generic data, and returns the configuration for it. The caller is responsible for deleting it! */
		class tConfig *parseFile(const tString &filename);
	private:
		uint32_t seqPrefix; // it's actually 3 Bytes
		tString name;
	};

} //End alc namespace

#endif
