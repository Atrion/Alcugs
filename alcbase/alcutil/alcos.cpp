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

/**
	Alcugs file system related code
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCOS_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "alcos.h"

#include "alcexception.h"

#include <sys/stat.h>


namespace alc {


tString alcGetExt(const tString & addr) {
	
	size_t i;
	for(i=addr.size()-1; ; --i) {
		char ch = addr.getAt(i);
		if(ch=='/' || ch=='\\' || ch==':' || ch=='.' || i == 0) {
			break;
		}
	}
	
	if(addr.getAt(i)=='.') {
		return addr.substring(i+1);
	} else {
		return tString();
	}
	
}

tString alcStripExt(const tString &addr) {
	for(size_t i=addr.size()-1; ; --i) {
		uint8_t c = addr.getAt(i);
		if(c=='.') {
			return addr.substring(0, i);
		}
		else if(c=='/' || c=='\\' || c==':' || i == 0) {
			return addr;
		} 
	}
}

/** creates a directory and its parent directories */
void alcMkdir(const tString &path, mode_t mode)
{
	tString subpath;
	for(size_t i=0; i<path.size(); i++) {
		uint8_t c = path.getAt(i);
		subpath.put8(c);
		if(c=='/' || c=='\\' || i == path.size()-1) {
			mkdir(subpath.c_str(), mode);
		}
	}
}

/* dir entry */
tDirEntry::tDirEntry(alc::tString name, int entryType) : name(name), entryType(entryType) {}
tDirEntry::~tDirEntry() {}
/* end dir entry */

/* tDirectory */
tDirectory::tDirectory() {
	dir=NULL;
	entry=NULL;
}
tDirectory::~tDirectory() {
	this->close();
}
void tDirectory::open(const tString &path) {
	dir=opendir(path.c_str());
	if(dir==NULL) throw txBase(_WHERE("OpenDirFailed for %s", path.c_str()));
	this->path = path;
}
void tDirectory::close() {
	if(dir!=NULL) closedir(dir);
}
tDirEntry tDirectory::getEntry() {
	entry=readdir(dir);
	if(entry==NULL) return tDirEntry();
	return tDirEntry(entry->d_name, entry->d_type);
}
void tDirectory::rewind() {
	rewinddir(dir);
}
/* end tDirectory */

} //end namespace alc
