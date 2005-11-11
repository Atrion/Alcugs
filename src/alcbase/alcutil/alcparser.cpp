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
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCPARSER_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"

////extra includes

#include "alcdebug.h"

namespace alc {

tSimpleParser::tSimpleParser() {
	cfg=NULL;
	sep=' ';
}
U32 tSimpleParser::size() {
	tStrBuf s;
	return stream(s);
}
void tSimpleParser::store(tBBuf &t) {
	tStrBuf s(t);
	store(s);
}
int tSimpleParser::stream(tBBuf &t) {
	tStrBuf s;
	int ret=stream(s);
	t.put(s);
	return ret;
}

void tSimpleParser::store(tStrBuf &t) {
	if(!cfg) return;
	Byte mode=0; // 0 none, 1 left, 2 mid, 3 right, 4 end
	tStrBuf key;
	tStrBuf val;
	key = t.getWord();
	val = t.getWord();
	tStrBuf sep;
	tStrBuf k;
	sep = k;
	if(val != sep) {
		printf("hi");
	}
	
}
int tSimpleParser::stream(tStrBuf &t) {
	U32 start=t.tell();
	DBG(4,"stream()\n");
	if(!cfg) return 0;
	DBG(5,"cfg->rewind()\n");
	cfg->rewind();
	tConfigKey * key;
	while((key=cfg->getNext())) {
		DBG(5,"cfg->getNext()\n");
		tConfigVal * val;
		while((val=key->getNext())) {
			DBG(5,"key->getNext()\n");
			t.writeStr(val->getName());
			t.writeStr(" ");
			t.putByte(sep);
			t.writeStr(" ");
			t.writeStr(val->getVal());
			t.nl();
		}
	}
	return (t.tell()-start);
}
void tSimpleParser::setConfig(tConfig * c) {
	DBG(5,"setconfig()\n");
	cfg=c;
}
tConfig * tSimpleParser::getConfig() {
	return cfg;
}

} //end namespace alc

