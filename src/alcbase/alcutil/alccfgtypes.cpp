/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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

//#define _DBG_LEVEL_ 5

#include "alcugs.h"

#include "alcdebug.h"

namespace alc {

static const tString emptyString;

tConfigVal::tConfigVal() { init(); }
tConfigVal::tConfigVal(const char * name) {
	init();
	setName(name);
}
tConfigVal::tConfigVal(const tString & name) {
	init();
	setName(name);
}
void tConfigVal::init() {
	name="";
	values=NULL;
	x=y=0;
}
tConfigVal::~tConfigVal() {
	int i;
	//if(name!=NULL) free((void *)name);
	if(values!=NULL) {
		for(i=0; i<x*y; i++) {
			DBG(9,"i:%i\n",i);
			delete values[i];
		}
		free(values);
	}
}
void tConfigVal::setName(const char * name) {
	this->name=name;
}
void tConfigVal::setName(const tString & name) {
	this->name=name;
}
void tConfigVal::setVal(const tString & t,U16 x,U16 y) {
	DBG(9,"x: %i,y: %i\n",x,y);
	U16 ox,oy,my,j,k;
	tString ** ovalues=this->values;
	ox = this->x;
	oy = this->y;
	my = ( oy > (y+1) ? oy : (y+1));
	if(ox<(x+1)) {
		//resize and copy
		values=static_cast<tString **>(malloc(sizeof(tString *) * ((my) * (x+1))));
		memset(values,0,sizeof(tString *) * ((my) * (x+1)));
		for(j=0; j<oy; j++) {
			for(k=0; k<ox; k++) {
				*(values+(((x+1)*j)+k))=*(ovalues+((ox*j)+k));
			}
		}
		this->x = x+1;
		this->y = my;
		ox = x+1;
		oy = my;
		free(ovalues);
		ovalues=values;
	}
	if(oy<(y+1)) {
		values=static_cast<tString **>(realloc(values,sizeof(tString *) * ((y+1)*(ox))));
		if(values==NULL) throw txNoMem(_WHERE("."));
		memset((values+((ox)*(oy))),0,sizeof(tString *) * ((((y+1)*(ox)))-((ox)*(oy))));
		this->y = my;
		ovalues=values;
	}
	tString ** myval=NULL;
	myval = (values+((ox*y)+x));
	if(*myval==NULL) {
		*myval = new tString(t);
	} else {
		**myval = t;
	}
}
void tConfigVal::setVal(const char * val,U16 x,U16 y) {
	tString w(val);
	setVal(w,x,y);
}
const tString & tConfigVal::getName() const {
	return name;
}
const tString & tConfigVal::getVal(U16 x,U16 y,bool *found) const {
	if (hasVal(x, y)) {
		if (found) *found = true;
		return **(values+((this->x*y)+x));
	}
	if (found) *found = false;
	return emptyString;
}
bool tConfigVal::hasVal(U16 x,U16 y) const {
	if(values==NULL || x>=this->x || y>=this->y) {
		return false;
	}
	if(*(values+((this->x*y)+x))==NULL) {
		return false;
	}
	return true;
}
void tConfigVal::copy(const tConfigVal &t) {
	U16 i;
	setName(t.getName());
	if(values!=NULL) {
		for(i=0; i<x*y; i++) {
			delete values[i];
		}
		free(values);
	}
	x=t.x;
	y=t.y;
	values=static_cast<tString **>(malloc(sizeof(tString *) * (x*y)));
	memset(values,0,sizeof(tString *) * (x*y));
	for(i=0; i<x*y; i++) {
		if(t.values[i]!=NULL) {
			values[i] = new tString(*t.values[i]);
		}
	}
}

tConfigKey::tConfigKey() {
	//name=NULL;
	off=n=0;
	values=NULL;
}
tConfigKey::~tConfigKey() {
	U16 i;
	//if(name!=NULL) free((void *)name);
	if(values!=NULL) {
		for(i=0; i<n; i++) {
			delete values[i];
		}
		free(values);
	}
}
void tConfigKey::setName(const char * name) {
	this->name=name;
}
void tConfigKey::setName(const tString & name) {
	this->name=name;
}
tConfigVal * tConfigKey::find(const tString & what,bool create) {
	U16 i;
	for(i=0; i<n; i++) {
		if(values[i]->name.lower()==what.lower()) {
			return values[i];
		}
	}
	if(!create) return NULL;
	n++;
	values=static_cast<tConfigVal **>(realloc(values,sizeof(tConfigVal *) * n));
	values[n-1]=new tConfigVal(what);
	return values[n-1];
}
tConfigVal * tConfigKey::find(const char * what,bool create) {
	tString name(what);
	return find(name,create);
}
void tConfigKey::copy(const tConfigKey &t) {
	U16 i;
	setName(t.getName());
	if(values!=NULL) {
		for(i=0; i<n; i++) {
			delete values[i];
		}
		free(values);
	}
	n=t.n;
	values=static_cast<tConfigVal **>(malloc(sizeof(tConfigVal *) * (n)));
	memset(values,0,sizeof(tConfigVal *) * (n));
	for(i=0; i<n; i++) {
		if(t.values[i]!=NULL) {
			values[i] = new tConfigVal();
			*values[i] = *t.values[i];
		}
	}
}
void tConfigKey::add(tConfigVal &t) {
	tConfigVal * val;
	val=find(t.getName(),1);
	*val=t;
}
void tConfigKey::merge(tConfigKey &t) {
	tConfigVal * val;
	t.rewind();
	while((val=t.getNext())) {
		add(*val);
	}
}
void tConfigKey::rewind() {
	off=0;
}
tConfigVal * tConfigKey::getNext() {
	DBG(9,"n:%i,off:%i\n",n,off);
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
			delete values[i];
		}
		free(values);
	}
}
tConfigKey * tConfig::findKey(const tString & where,bool create) {
	U16 i;
	for(i=0; i<n; i++) {
		DBG(9,"checking %s %s %s %s \n",values[i]->name.c_str(),where.c_str(),values[i]->name.lower().c_str(),where.lower().c_str());
		if(values[i]->name.lower()==where.lower()) {
			return values[i];
		}
	}
	if(!create) return NULL;
	n++;
	values=static_cast<tConfigKey **>(realloc(values,sizeof(tConfigKey *) * n));
	values[n-1]=new tConfigKey();
	values[n-1]->setName(where);
	return values[n-1];
}
tConfigKey * tConfig::findKey(const char * where,bool create) {
	tString name(where);
	return findKey(name,create);
}
//This code needs to be optimized
tConfigVal * tConfig::findVar(const char * what,const char * where,bool create) {
	tConfigKey * mykey;
	mykey=findKey(where,create);
	if(mykey==NULL) return NULL;
	return(mykey->find(what,create));
}
void tConfig::setVar(const char * val,const char * what,const char * where,U16 x,U16 y) {
	tConfigVal * myvar;
	DBG(5,"findVar...");
	myvar=findVar(what,where,true);
	DBGM(5," done\n");
	myvar->setVal(val,x,y);
}
void tConfig::setVar(const tString &val, const tString &what, const tString &where,U16 x,U16 y) {
	tConfigVal * myvar;
	DBG(5,"findVar...");
	myvar=findVar(what.c_str(),where.c_str(),true);
	DBGM(5," done\n");
	myvar->setVal(val,x,y);
}
const tString & tConfig::getVar(const char * what,const char * where,U16 x,U16 y,bool *found) {
	tConfigVal * myvar;
	myvar=findVar(what,where,false);
	if(myvar==NULL) {
		if (found) *found = false;
		return emptyString;
	}
	return myvar->getVal(x,y, found);
}
void tConfig::rewind() {
	off=0;
}
tConfigKey * tConfig::getNext() {
	if(off>=n) { off=0; return NULL; }
	off++;
	return values[off-1];
}
void tConfig::copyKey(const char * to, const char * from) {
	tConfigKey * dst;
	tConfigKey * src;
	src=findKey(from,0);
	if(src==NULL) return;
	dst=findKey(to,1);
	dst->merge(*src);
}
void tConfig::copyValue(const char * tok, const char * fromk,const char * to, const char * from) {
	tConfigKey * dst;
	tConfigKey * src;
	src=findKey(from,0);
	if(src==NULL) return;
	dst=findKey(to,1);
	tConfigVal * val;
	val=src->find(fromk,0);
	if(val==NULL) return;
	tConfigVal myval=*val;
	myval.setName(tok);
	dst->add(myval);
}


} //end namespace alc

