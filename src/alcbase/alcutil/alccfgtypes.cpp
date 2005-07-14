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

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include "alcdebug.h"

namespace alc {

tConfigVal::tConfigVal() { init(); }
tConfigVal::tConfigVal(Byte * name) {
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
			if(values[i]!=NULL) {
				delete values[i];
			}
		}
		free((void *)values);
	}
}
void tConfigVal::setName(Byte * name) {
	if(name!=NULL) free((void *)name);
	name=(Byte *)malloc(sizeof(Byte) * (strlen((const char *)name)+1));
	strcpy((char *)name,(char *)name);
}
void tConfigVal::setVal(tStrBuf & t,U16 x,U16 y) {
	U16 ox,oy,my,j,k;
	tStrBuf ** ovalues=this->values;
	
	ox = this->x;
	oy = this->y;
	my = ( oy > (y+1) ? oy : (y+1));

	if(ox<(x+1)) {
		//resize and copy
		values=(tStrBuf **)malloc(sizeof(tStrBuf *) * ((my) * (x+1)));
		memset(values,0,sizeof(tStrBuf *) * ((my) * (x+1)));
		for(j=0; j<oy; j++) {
			for(k=0; k<ox; k++) {
				*(values+(((x+1)*j)+k))=*(ovalues+((ox*j)+k));
			}
		}
		this->x = x+1;
		this->y = my;
		ox = x+1;
		oy = my;
		free((void *)ovalues);
		ovalues=values;
	}
	
	if(oy<(y+1)) {
		values=(tStrBuf **)realloc((void *)values,sizeof(tStrBuf *) * ((y+1)*(ox)));
		if(values==NULL) throw txNoMem(_WHERE("."));
		memset((values+((ox)*(oy))),0,sizeof(tStrBuf *) * ((((y+1)*(ox)))-((ox)*(oy))));
		this->y = my;
		ovalues=values;
	}
	
	tStrBuf * myval=NULL;
	myval = *(values+((ox*y)+x));
	
	if(myval==NULL) {
		myval = new tStrBuf(t);
	} else {
		*myval = t;
	}
}
void tConfigVal::setVal(Byte * val,U16 x,U16 y) {
	tStrBuf w;
	w.writeStr(val);
	setVal(w,x,y);
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

} //end namespace alc

