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
#define __U_PYPKG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
//python package

tPyPkgFile::tPyPkgFile() {
	msize=0;
	name.setVersion(5);
}
tPyPkgFile::~tPyPkgFile() {}
void tPyPkgFile::store(tBBuf &t) {
	msize=t.getU32();
	if(msize>0) {
		//Python 2.2 magic
		pkg.putByte(0x2D);
		pkg.putByte(0xED);
		pkg.putByte(0x0D);
		pkg.putByte(0x0A);
		pkg.putU32((U32 )time(NULL));
		pkg.write(t.read(msize),msize);
		pkg.rewind();
	}
}
void tPyPkgFile::stream(tBBuf &t) {
	msize=pkg.size();
	t.putU32(msize-8);
	pkg.rewind();
	pkg.read(8);
	t.write(pkg.read(),msize-8);
	//return msize-4;
}
U32 tPyPkgFile::size() {
	return(pkg.size()-4);
}
U32 tPyPkgFile::avgsize() { return this->size(); }
void tPyPkgFile::setName(const char * x) {
	//printf("assing %s\n",x);
	name=x;
}
void tPyPkgFile::setName(const tStrBuf & x) {
	name=x;
}
tStrBuf & tPyPkgFile::getName() {
	//printf("getName %s\n",name.str());
	return name;
}

tPyPkg::tPyPkg() {
	n=ite=0;
	lpkgs=NULL;
}
tPyPkg::~tPyPkg() {
	if(lpkgs!=NULL) {
		for(U32 i=0; i<n; i++) {
			delete lpkgs[i];
		}
		free((void *)lpkgs);
	}
}
void tPyPkg::store(tBBuf &t) {
	U32 myoff,poff;
	tUStr name(5);
	n=t.getU32();
	lpkgs=(tPyPkgFile **)malloc(sizeof(tPyPkgFile *) * n);
	if(lpkgs==NULL) throw txNoMem(_WHERE("NoMem"));
	//printf("there are %i packages\n",n);
	printf("size: %u\n",t.size());
	for(U32 i=0; i<n; i++) {
		//printf("loading package %i/%i\n",i,n);
		lpkgs[i] = new tPyPkgFile();
		//printf("getting it...\n");
		//printf("size:%u,off:%u\n",t.size(),t.tell());
		t.get(name);
		char xxxname[300];
		strcpy(xxxname,name.c_str());
		strcat(xxxname,"c");
		lpkgs[i]->setName(xxxname);
		poff=t.getU32();
		myoff=t.tell();
		t.set(poff);
		t.get(*lpkgs[i]);
		t.set(myoff);
		//printf("got.\n");
	}
	//printf("done loading packages\n");
}
void tPyPkg::stream(tBBuf &t) {
	U32 base;
	//U32 s=0;
	base=t.tell();
	t.putU32(n);
	//s+=4;
	U32 hsize=4,wtf;
	tUStr work(5);
	//printf("size: %u\n",t.size());
	//printf("hsize: %u\n",hsize);
	for(U32 i=0; i<n; i++) {
		hsize+=strlen(lpkgs[i]->getName().c_str())+5;
		//printf("hsize: %u\n",hsize);
	}
	wtf=t.tell();
	//printf("size: %u\n",t.size());
	for(U32 i=0; i<hsize-4; i++) {
		t.putByte(0);
		//printf("size: %u\n",t.size());
	}
	for(U32 i=0; i<n; i++) {
		//printf("size-start%i: %u\n",i,t.size());
		//work.assing(lpkgs[i]->getName(),strlen((char *)lpkgs[i]->getName())-1);
		work=lpkgs[i]->getName();
		t.set(wtf);
		//s+=
		work.stream(t);
		//printf("size-work: %u\n",t.size());
		t.putU32(hsize);
		//printf("size-hsize: %u\n",t.size());
		//s+=4;
		wtf=t.tell();
		t.set(base+hsize);
		//s+=
		t.put(*lpkgs[i]);
		//printf("size-lpkgs: %u\n",t.size());
		hsize+=lpkgs[i]->size();
		//printf("size-end%i: %u\n",i,t.size());
	}
	//printf("size-final: %u\n",t.size());
	//return s;
}
U32 tPyPkg::size() {
	U32 s=4;
	for(U32 i=0; i<n; i++) {
		s+=lpkgs[i]->size();
		s+=strlen(lpkgs[i]->getName().c_str())+5;
	}
	return s;	
}
U32 tPyPkg::avgsize() { return this->size(); }
void tPyPkg::add(char * name,tBBuf &t) {
	n++;
	lpkgs=(tPyPkgFile **)realloc(lpkgs,sizeof(tPyPkgFile *) * n);
	
	lpkgs[n-1] = new tPyPkgFile();

	lpkgs[n-1]->pkg.put(t);
	lpkgs[n-1]->setName(name);
}
void tPyPkg::rewind() {
	ite=0;
}
tPyPkgFile * tPyPkg::getNext() {
	if(ite>=n) return NULL;
	return lpkgs[ite++];
}
tPyPkgFile * tPyPkg::find(char * what) {
	for(U32 i=ite; i<n; i++) {
		//if(!strcmp((const char *)lpkgs[i]->getName(),what)) {
		if(lpkgs[i]->getName()==what) {
			return lpkgs[i];
		}
	}
	return NULL;
}
//end python package

} //end namespace alc

