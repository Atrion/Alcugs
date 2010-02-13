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
	Alcugs OS related thingyes
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCOS_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include <sys/stat.h>

//alcos.h already included in alcugs.h

#include "alcdebug.h"

namespace alc {


tString alcGetExt(const tString & addr) {
	
	U32 i;
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

void alcStripExt(char * addr) {
	U32 i=0;
	for(i=strlen(addr); ; --i) {
		if(addr[i]=='.') {
			addr[i]='\0';
			break;
		}
		else if(addr[i]=='/' || addr[i]=='\\' || addr[i]==':' || i == 0) {
			break;
		} 
	}
}

/* dir entry */
tDirEntry::tDirEntry() {
	name=NULL;
	entryType=0;
}
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
void tDirectory::open(const char * path) {
	dir=opendir(path);
	if(dir==NULL) throw txBase(_WHERE("OpenDirFailed"));
	alcStrncpy(this->path, path, 511);
}
void tDirectory::close() {
	if(dir!=NULL) closedir(dir);
}
tDirEntry * tDirectory::getEntry() {
	entry=readdir(dir);
	if(entry==NULL) return NULL;
	ent.name=entry->d_name;
	#if defined(__WIN32__) or defined(__CYGWIN__)
	//ent.type=0;
	char * kpath=(char *)malloc(sizeof(char) * (strlen(entry->d_name) + strlen(this->path) + 4));
	strcpy(kpath,this->path);
	strcat(kpath,"\\");
	strcat(kpath,entry->d_name);
	struct stat buf;
	stat(kpath,&buf);
	if(S_ISDIR(buf.st_mode)) ent.entryType=DT_DIR;
	else ent.entryType=DT_REG;
	//ent.type=buf.st_mode;
	#else
	ent.entryType=entry->d_type;
	#endif
	return &ent;
}
void tDirectory::rewind() {
	rewinddir(dir);
}
/* end tDirectory */

} //end namespace alc
