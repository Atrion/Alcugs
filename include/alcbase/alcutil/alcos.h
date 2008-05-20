/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs H'uru Server Team                     *
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
	Alcugs OS related things.
*/

#ifndef __U_ALCOS_H
#define __U_ALCOS_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCOS_H_ID "$Id$"

namespace alc {

/** 
	\param filename name
	\return the extension 
*/
char * alcGetExt(const char * addr);

/** Strips the extension from a filename */
void alcStripExt(char * addr);


/** A Directory entry */
class tDirEntry {
public:
	tDirEntry();
	~tDirEntry();
	char * name;
	int type;
};

/** Directory */
class tDirectory {
public:
	tDirectory();
	~tDirectory();
	void open(const char * path);
	void close();
	tDirEntry * getEntry();
	void rewind();
private:
	std::DIR *dir;
	struct std::dirent *entry;
	tDirEntry ent;
	char path[512];
};

} //End alc namespace

#endif
