/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs Project Server Team                   *
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

/* CVS tag - DON'T TOUCH*/
#define __U_ALCPKG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

////extra includes

#include "alcdebug.h"

namespace alc {

//tPkgFile
tPkgFile::tPkgFile() {
	msize=0;
	name.setVersion(0);
}
tPkgFile::~tPkgFile() {}
void tPkgFile::store(tBBuf &t) {
	t.get(name);
	msize=t.getU32();
	if(msize>0) { pkg.write(t.read(msize),msize); pkg.rewind(); }
}
void tPkgFile::stream(tBBuf &t) {
	//U32 xsize=0;
	//xsize+=
	t.put(name);
	msize=pkg.size();
	t.putU32(msize);
	//xsize+=4;
	//xsize+=
	t.put(pkg);
	//return xsize;
}
U32 tPkgFile::size() {
	return(name.size() + 4 + msize);
}
U32 tPkgFile::avgsize() { return this->size(); }
void tPkgFile::setName(const void * x) {
	name=(char *)x;
}
void tPkgFile::setName(const tStrBuf & x) {
	name=x;
}
tStrBuf & tPkgFile::getName() {
	return name;
}
//end tPkgFile

//tPkg
tPkg::tPkg() {
	ite=n=version=0;
	lpkgs=NULL;
}
tPkg::~tPkg() {
	if(lpkgs!=NULL) {
		for(U32 i=0; i<n; i++) {
			delete lpkgs[i];
		}
		free((void *)lpkgs);
	}
}
void tPkg::store(tBBuf &t) {
	t.check("alcugs",6);
	version=t.getU32();
	if (version!=0) throw txBase(_WHERE("Package version mismatch!"));
	n=t.getU32();
	lpkgs=(tPkgFile **)malloc(sizeof(tPkgFile *) * n);
	if(lpkgs==NULL) throw txNoMem(_WHERE("NoMem"));
	//printf("there are %i tPkgs\n",n);
	for(U32 i=0; i<n; i++) {
		//printf("loading tPkg %i/%i\n",i,n);
		lpkgs[i] = new tPkgFile();
		//printf("getting it...\n");
		//printf("size:%u,off:%u\n",t.size(),t.tell());
		t.get(*lpkgs[i]);
		//printf("got.\n");
	}
	printf("done loading tPkgs\n");
}
void tPkg::stream(tBBuf &t) {
	//U32 s=6+8;
	t.write((Byte *)"alcugs",6);
	t.putU32(version);
	t.putU32(n);
	for(U32 i=0; i<n; i++) {
		//s+=
		t.put(*lpkgs[i]);
	}
	//return s;
}
U32 tPkg::size() {
	U32 s=8+6;
	for(U32 i=0; i<n; i++) {
		s+=lpkgs[i]->size();
	}
	return s;	
}
U32 tPkg::avgsize() { return this->size(); }
void tPkg::add(char * name,tBBuf &t) {
	n++;
	lpkgs=(tPkgFile **)realloc(lpkgs,sizeof(tPkgFile *) * n);
	
	lpkgs[n-1] = new tPkgFile();

	lpkgs[n-1]->pkg.put(t);
	lpkgs[n-1]->setName((Byte *)name);
}
void tPkg::rewind() {
	ite=0;
}
tPkgFile * tPkg::getNext() {
	if(ite>=n) return NULL;
	return lpkgs[ite++];
}
tPkgFile * tPkg::find(char * what) {
	for(U32 i=ite; i<n; i++) {
		if(lpkgs[i]->getName()==what) {
			return lpkgs[i];
		}
	}
	return NULL;
}
//end tPkg


} //end namespace alc

