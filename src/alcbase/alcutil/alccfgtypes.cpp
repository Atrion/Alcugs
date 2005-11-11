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
	Alcugs Basic data types, and buffer classes.
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCCFGTYPES_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include "alcdebug.h"

namespace alc {

tConfigVal::tConfigVal() { init(); }
tConfigVal::tConfigVal(const void * name) {
	init();
	setName(name);
}
void tConfigVal::init() {
	name=NULL;
	values=NULL;
	x=y=0;
}
tConfigVal::~tConfigVal() {
	int i;
	if(name!=NULL) free((void *)name);
	if(values!=NULL) {
		for(i=0; i<x*y; i++) {
			DBG(4,"i:%i\n",i);
			if(values[i]!=NULL) {
				DBG(6,"deleting...\n");
				delete values[i];
			}
		}
		free((void *)values);
	}
}
void tConfigVal::setName(const void * name) {
	if(this->name!=NULL) free((void *)this->name);
	this->name=(Byte *)malloc(sizeof(Byte) * (strlen((const char *)name)+1));
	strcpy((char *)this->name,(char *)name);
}
void tConfigVal::setVal(tStrBuf & t,U16 x,U16 y) {
	DBG(4,"x: %i,y: %i\n",x,y);
	U16 ox,oy,my,j,k;
	tStrBuf ** ovalues=this->values;
		dmalloc_verify(NULL);
	ox = this->x;
	oy = this->y;
	my = ( oy > (y+1) ? oy : (y+1));
	dmalloc_verify(NULL);
	if(ox<(x+1)) {
		//resize and copy
		values=(tStrBuf **)malloc(sizeof(tStrBuf *) * ((my) * (x+1)));
		memset(values,0,sizeof(tStrBuf *) * ((my) * (x+1)));
		for(j=0; j<oy; j++) {
			for(k=0; k<ox; k++) {
				*(values+(((x+1)*j)+k))=*(ovalues+((ox*j)+k));
			}
		}
			dmalloc_verify(NULL);
		this->x = x+1;
			dmalloc_verify(NULL);
		this->y = my;
			dmalloc_verify(NULL);
		ox = x+1;
			dmalloc_verify(NULL);
		oy = my;
			dmalloc_verify(NULL);
		if((void *)ovalues!=NULL) {
			free((void *)ovalues);
		}
			dmalloc_verify(NULL);
		ovalues=values;
		dmalloc_verify(NULL);
	}
		dmalloc_verify(NULL);
	if(oy<(y+1)) {
		values=(tStrBuf **)realloc((void *)values,sizeof(tStrBuf *) * ((y+1)*(ox)));
		if(values==NULL) throw txNoMem(_WHERE("."));
		memset((values+((ox)*(oy))),0,sizeof(tStrBuf *) * ((((y+1)*(ox)))-((ox)*(oy))));
		this->y = my;
		ovalues=values;
	}
		dmalloc_verify(NULL);
	tStrBuf ** myval=NULL;
	myval = (values+((ox*y)+x));
		dmalloc_verify(NULL);
	if(*myval==NULL) {
		*myval = new tStrBuf(t);
	} else {
		**myval = t;
	}
		dmalloc_verify(NULL);
}
void tConfigVal::setVal(const void * val,U16 x,U16 y) {
	tStrBuf w;
		dmalloc_verify(NULL);
	w.writeStr((char *)val);
		dmalloc_verify(NULL);
	setVal(w,x,y);
		dmalloc_verify(NULL);
}
Byte * tConfigVal::getName() {
	return name;
}
tStrBuf * tConfigVal::getVal(U16 x,U16 y) {
	if(values==NULL || x>=this->x || y>=this->y) return NULL;
	U16 nx=this->x;
	return *(values+((nx*y)+x));
}
void tConfigVal::copy(tConfigVal &t) {
	U16 i;
	setName(t.getName());
	if(values!=NULL) {
		for(i=0; i<x*y; i++) {
			if(values[i]!=NULL) {
				delete values[i];
			}
		}
		free((void *)values);
	}
	x=t.x;
	y=t.y;
	values=(tStrBuf **)malloc(sizeof(tStrBuf *) * (x*y));
	memset(values,0,sizeof(tStrBuf *) * (x*y));
	for(i=0; i<x*y; i++) {
		if(t.values[i]!=NULL) {
			values[i] = new tStrBuf(*t.values[i]);
		}
	}
}

tConfigKey::tConfigKey() {
	name=NULL;
	off=n=0;
	values=NULL;
}
tConfigKey::~tConfigKey() {
	U16 i;
	if(name!=NULL) free((void *)name);
	if(values!=NULL) {
		for(i=0; i<n; i++) {
			if(values[i]!=NULL) {
				delete values[i];
			}
		}
		free((void *)values);
	}
}
void tConfigKey::setName(const Byte * name) {
	if(this->name!=NULL) free((void *)this->name);
	this->name=(Byte *)malloc(sizeof(Byte) * (strlen((const char *)name)+1));
	strcpy((char *)this->name,(char *)name);
}
tConfigVal * tConfigKey::find(const void * what,bool create) {
	U16 i;
		dmalloc_verify(NULL);
	for(i=0; i<n; i++) {
		if(!strcmp((const char *)what,(const char *)values[i]->name)) {
			return values[i];
		}
	}
		dmalloc_verify(NULL);
	if(!create) return NULL;
		dmalloc_verify(NULL);
	n++;
		dmalloc_verify(NULL);
	values=(tConfigVal **)realloc((void *)values,sizeof(tConfigVal *) * n);
		dmalloc_verify(NULL);
	values[n-1]=new tConfigVal(what);
		dmalloc_verify(NULL);
	return values[n-1];
}
void tConfigKey::copy(tConfigKey &t) {
	U16 i;
	setName(t.getName());
	if(values!=NULL) {
		for(i=0; i<n; i++) {
			if(values[i]!=NULL) {
				delete values[i];
			}
		}
		free((void *)values);
	}
	n=t.n;
	values=(tConfigVal **)malloc(sizeof(tConfigVal *) * (n));
	memset(values,0,sizeof(tConfigVal *) * (n));
	for(i=0; i<n; i++) {
		if(t.values[i]!=NULL) {
			values[i] = new tConfigVal();
			*values[i] = *t.values[i];
		}
	}
}
void tConfigKey::rewind() {
	off=0;
}
tConfigVal * tConfigKey::getNext() {
	DBG(5,"n:%i,off:%i\n",n,off);
	if(off>=n) { off=0; return NULL; }
	off++;
	return values[off-1];
}

tConfig::tConfig() {
	off=n=0;
	values=NULL;
}
tConfig::~tConfig() {
	U16 i;
	if(values!=NULL) {
		for(i=0; i<n; i++) {
			if(values[i]!=NULL) {
				delete values[i];
			}
		}
		free((void *)values);
	}
}
tConfigKey * tConfig::findKey(const void * where,bool create) {
	U16 i;
	for(i=0; i<n; i++) {
		if(!strcmp((const char *)where,(const char *)values[i]->name)) {
			return values[i];
		}
	}
	dmalloc_verify(NULL);
	if(!create) return NULL;
	dmalloc_verify(NULL);
	n++;
		dmalloc_verify(NULL);
	values=(tConfigKey **)realloc((void *)values,sizeof(tConfigKey *) * n);
		dmalloc_verify(NULL);
	values[n-1]=new tConfigKey();
		dmalloc_verify(NULL);
	values[n-1]->setName((const Byte *)where);
		dmalloc_verify(NULL);
	return values[n-1];
}
tConfigVal * tConfig::findVar(const void * what,const void * where,bool create) {
	tConfigKey * mykey;
		dmalloc_verify(NULL);
	mykey=findKey(where,create);
		dmalloc_verify(NULL);
	if(mykey==NULL) return NULL;
		dmalloc_verify(NULL);
	return(mykey->find(what,create));
}
void tConfig::setVar(const void * val,const void * what,const void * where) {
	tConfigVal * myvar;
		dmalloc_verify(NULL);
	myvar=findVar(what,where,true);
		dmalloc_verify(NULL);
	myvar->setVal(val);
		dmalloc_verify(NULL);
}
void tConfig::rewind() {
	off=0;
}
tConfigKey * tConfig::getNext() {
	if(off>=n) { off=0; return NULL; }
	off++;
	return values[off-1];
}


} //end namespace alc

