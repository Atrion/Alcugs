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
	Alcugs Basic data types, and buffer classes.
*/

//#define _DBG_LEVEL_ 5
#include "alcdefs.h"
#include "alccfgtypes.h"

#include "alcexception.h"

#include <cstdlib>

namespace alc {

static const tString emptyString;

//// tConfigVal
tConfigVal::tConfigVal() { init(); }
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
	unsigned int i;
	//if(name!=NULL) free((void *)name);
	if(values!=NULL) {
		for(i=0; i<x*y; i++) {
			DBG(9,"i:%i\n",i);
			delete values[i];
		}
		free(values);
	}
}
void tConfigVal::setName(const tString & name) {
	this->name=name;
}
void tConfigVal::setVal(const tString & t,unsigned int x,unsigned int y) {
	DBG(9,"x: %i,y: %i\n",x,y);
	unsigned int ox,oy,my,j,k;
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
const tString & tConfigVal::getVal(unsigned int x,unsigned int y,bool *found) const {
	if (hasVal(x, y)) {
		if (found) *found = true;
		return **(values+((this->x*y)+x));
	}
	if (found) *found = false;
	return emptyString;
}
bool tConfigVal::hasVal(unsigned int x,unsigned int y) const {
	if(values==NULL || x>=this->x || y>=this->y) {
		return false;
	}
	if(*(values+((this->x*y)+x))==NULL) {
		return false;
	}
	return true;
}
void tConfigVal::copy(const tConfigVal &t) {
	unsigned int i;
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

//// tConfigKey
tConfigKey::tConfigKey() {
	//name=NULL;
	off=n=0;
	values=NULL;
}
tConfigKey::~tConfigKey() {
	unsigned int i;
	//if(name!=NULL) free((void *)name);
	if(values!=NULL) {
		for(i=0; i<n; i++) {
			delete values[i];
		}
		free(values);
	}
}
tConfigVal * tConfigKey::find(tString what,bool create) {
	unsigned int i;
	what = what.lower(); // this also implicitly makes all added values lower-case as it's only here they are created
	for(i=0; i<n; i++) {
		if(values[i]->name==what) {
			return values[i];
		}
	}
	if(!create) return NULL;
	n++;
	values=static_cast<tConfigVal **>(realloc(values,sizeof(tConfigVal *) * n));
	values[n-1]=new tConfigVal(what);
	return values[n-1];
}
void tConfigKey::copy(const tConfigKey &t) {
	unsigned int i;
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
tConfigVal * tConfigKey::getNext() {
	DBG(9,"n:%i,off:%i\n",n,off);
	if(off>=n) { off=0; return NULL; }
	off++;
	return values[off-1];
}

//// tConfig
tConfig::tConfig() {
	off=n=0;
	values=NULL;
}
tConfig::~tConfig() {
	if(values!=NULL) {
		for(unsigned int i=0; i<n; i++) {
			delete values[i];
		}
		free(values);
	}
}
tConfigKey * tConfig::findKey(tString where,bool create) {
	unsigned int i;
	where = where.lower(); // this also implicitly makes all added keys lower-case as it's only here they are created
	for(i=0; i<n; i++) {
		if(values[i]->name==where) {
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
tConfigVal * tConfig::findVar(const tString & what,const tString & where,bool create) {
	tConfigKey * mykey;
	mykey=findKey(where,create);
	if(mykey==NULL) return NULL;
	return(mykey->find(what,create));
}
void tConfig::setVar(const tString &val, const tString &what, const tString &where,unsigned int x,unsigned int y) {
	tConfigVal * myvar;
	DBG(5,"findVar...");
	myvar=findVar(what,where,true);
	DBGM(5," done\n");
	myvar->setVal(val,x,y);
}
const tString & tConfig::getVar(const tString & what,const tString & where,unsigned int x,unsigned int y,bool *found) {
	tConfigVal * myvar;
	myvar=findVar(what,where,false);
	if(myvar==NULL) {
		if (found) *found = false;
		return emptyString;
	}
	return myvar->getVal(x,y, found);
}
tConfigKey * tConfig::getNext() {
	if(off>=n) { off=0; return NULL; }
	off++;
	return values[off-1];
}
void tConfig::copyKey(const tString & to, const tString & from) {
	tConfigKey * dst;
	tConfigKey * src;
	src=findKey(from,0);
	if(src==NULL) return;
	dst=findKey(to,1);
	dst->merge(*src);
}
void tConfig::copyValue(const tString & tok, const tString & fromk,const tString & to, const tString & from) {
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

