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

#ifndef __U_GUIDGEN_H
#define __U_GUIDGEN_H
/* CVS tag - DON'T TOUCH*/
#define __U_GUIDGEN_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	class tAgeFile
	{
	public:
		tAgeFile(const char *dir, const char *file);
		int seqPrefix;
		Byte name[200];
	};
	
	class tAgeParser
	{
	public:
		tAgeParser(const char *dir);
		~tAgeParser(void);
		tAgeFile *getAge(Byte *name);
	private:
		int size;
		tAgeFile **ages;
	};
	
	class tGuidGen
	{
	public:
		tGuidGen(void);
		~tGuidGen(void) { delete ageParser; }
		
		tAgeParser *ageParser;
	};

} //End alc namespace

#endif
