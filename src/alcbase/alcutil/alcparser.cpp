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
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCPARSER_ID "$Id$"

//#define _DBG_LEVEL_ 10

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
	stream(s);
	return s.size();
}
void tSimpleParser::store(tBBuf &t) {
	tStrBuf s(t);
	store(s);
}
void tSimpleParser::stream(tBBuf &t) {
	tStrBuf s;
	stream(s);
	t.put(s);
}

void tSimpleParser::store(tStrBuf &t) {
	if(!cfg) return;
	tStrBuf key,val;
	DBG(4,"Store\n");
	while(!t.eof()) {
		key = t.getToken();
		DBG(5,"Reading token %s\n",key.c_str());
		if(key=="=") {
			throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token '%s'. A variable name was expected.\n",t.getLineNum(),t.getColumnNum(),key.c_str()));
		} else if(key!="\n") {
			val = t.getToken();
			if(val=="=") {
				val = t.getToken();
				if(val=="=") {
					throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token '%s'. A valid variable value was expected.\n",t.getLineNum(),t.getColumnNum(),val.c_str()));
				}
			}
			if(val=="\n") {
				t.seek(-1);
				t.decreaseLineNum();
				val=" ";
			}
			//cfg->setVar(val.c_str(),key.c_str(),"global");
			//cfg->findVar(key.c_str(),"global",1);
			tStrBuf section("global");
			cfg->setVar(val,key,section);
			val=t.getToken();
			//printf("Reprs:\n%s\n",val.hexToAscii());
			if(val!="\n") {
				throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token '%s'. A newline was expected.\n",t.getLineNum(),t.getColumnNum(),val.c_str()));
			}
		}
	}	
}
void tSimpleParser::stream(tStrBuf &t) {
	//U32 start=t.tell();
	DBG(4,"stream()\n");
	if(!cfg) return;
	DBG(5,"cfg->rewind()\n");
	cfg->rewind();
	tConfigKey * key;
	while((key=cfg->getNext())) {
		DBG(5,"cfg->getNext()\n");
		tConfigVal * val;
		tStrBuf str;
		while((val=key->getNext())) {
			DBG(5,"key->getNext()\n");
			t.writeStr(val->getName());
			//t.writeStr(" ");
			t.putByte(sep);
			//t.writeStr(" ");
			str=val->getVal();
			t.writeStr("\"");
			t.writeStr(str);
			t.writeStr("\"");
			t.nl();
		}
	}
	//return (t.tell()-start);
}
void tSimpleParser::setConfig(tConfig * c) {
	DBG(5,"setconfig()\n");
	cfg=c;
}
tConfig * tSimpleParser::getConfig() {
	return cfg;
}

tXParser::tXParser(bool override) :tSimpleParser() {
	this->override = override;
}

void tXParser::setBasePath(const tStrBuf & base) {
	path=base;
}

void tXParser::store(tBBuf &t) {
	tStrBuf s(t);
	store(s);
}
void tXParser::stream(tBBuf &t) {
	tStrBuf s;
	stream(s);
	t.put(s);
}
void tXParser::store(tStrBuf &t) {
	if(!cfg) return;
	tStrBuf section,key,val;
	DBG(4,"Store\n");
	section="global";
	U16 x,y;

	while(!t.eof()) {
		key = t.getToken();
		if(key!="\n") {
			DBG(9,"Reading token %s\n",key.c_str());
			//check for section
			if(key.startsWith("[") && key.endsWith("]")) {
				section=key.strip('[').strip(']');
				DBG(5,">>Entering section %s\n",section.c_str());
				continue;
			}
			//check for read_config
			if(key=="read_config") {
				val = t.getToken();
				if(val=="=") {
					t.getToken();
				}
				#if defined(__WIN32__) or defined(__CYGWIN__)
					val.convertSlashesFromWinToUnix();
					if(val.getAt(1)==':' || val.getAt(0)=='/') {
						key=val;
					} else {
						key=path + "/" + val;
					}
				#else
					if(val.getAt(0)=='/') {
						key=val;
					} else {
						key=path + "/" + val;
					}
				#endif
				if(!alcParseConfig(key)) {
					throw txParseError(_WHERE("Error attemting to read/parse the file %s found at line %i, column %i.\n",key.c_str(),t.getLineNum(),t.getColumnNum()));
				}
				continue;
			}
			x=0; y=0;
			//get var name
			if(key=="=") {
				throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token '%s'. A variable name was expected.\n",t.getLineNum(),t.getColumnNum(),key.c_str()));
			} else {
				//get table offsets
				y=alcParseKey(key);
				DBG(5,"**Key %s at %i\n",key.c_str(),y);
				//get separator
				val = t.getToken();
				if(val!="=") {
					throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token '%s'. '=' was expected.\n",t.getLineNum(),t.getColumnNum(),val.c_str()));
				}
				//get value(s)
				val=",";
				while(val==",") {
					val = t.getToken();
					if(val==",") { x++; continue; }
					if(val=="=") {
						throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token '%s'. A valid variable value was expected.\n",t.getLineNum(),t.getColumnNum(),val.c_str()));
					}
					if (!override)
						while (!cfg->getVar(key.c_str(),section.c_str(),x,y).isNull()) ++y; // if override is disabled, go to next row
					if(val=="\n") {
						val="";
						cfg->setVar(val,key,section,x++,y);
					} else {
						cfg->setVar(val,key,section,x++,y);
						val=t.getToken();
					}
				}
				if(!t.eof() && val!="\n") {
					throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token '%s'. A newline was expected.\n",t.getLineNum(),t.getColumnNum(),val.c_str()));
				}
			}
		}
	}
}

void tXParser::stream(tStrBuf &t) {
	DBG(4,"stream()\n");
	if(!cfg) return;
	DBG(5,"cfg->rewind()\n");
	cfg->rewind();
	tConfigKey * key;
	while((key=cfg->getNext())) {
		DBG(5,"cfg->getNext()\n");
		tConfigVal * val;
		tStrBuf str;
		t.writeStr("\n[" + key->getName() + "]\n");
		while((val=key->getNext())) {
			DBG(5,"key->getNext()\n");
			U16 x,y,mx,my;
			mx=val->getCols();
			my=val->getRows();

			for(y=0; y<my; y++) {
				t.writeStr(val->getName());
				if(my!=1) {
					t.writeStr("[");
					t.printf("%d", y);
					t.writeStr("]");
				}
				t.writeStr(" = ");
				for(x=0; x<mx; x++) {
					if(x!=0) t.writeStr(",");
					str=val->getVal(x,y);
					t.writeStr("\"");
					t.writeStr(str.escape());
					t.writeStr("\"");
				}
				t.nl();
			}
		}
	}
}


} //end namespace alc

