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

#include <dirent.h>
#include <cstdlib>

namespace alc {

/** 
	\param addr filename
	\return the extension 
*/
tString alcGetExt(const tString & addr);

/** Strips the extension from a filename */
tString alcStripExt(const tString &addr);

/** creates a directory and its parent directories */
void alcMkdir(const tString &path, mode_t mode);


/** A Directory entry */
class tDirEntry {
public:
	tDirEntry();
	~tDirEntry();
	tString name;
	int entryType;
	inline bool isDir(void) { return entryType == DT_DIR; }
	inline bool isFile(void) { return entryType == DT_REG || entryType == DT_UNKNOWN; } // for some reasons, some Linux systems give DT_UNKNOWN
};

/** Directory */
class tDirectory {
public:
	tDirectory();
	~tDirectory();
	void open(const tString &path);
	void close();
	tDirEntry * getEntry();
	void rewind();
private:
	DIR *dir;
	struct dirent *entry;
	tDirEntry ent;
	tString path;
	
	FORBID_CLASS_COPY(tDirectory)
};

} //End alc namespace

#endif
