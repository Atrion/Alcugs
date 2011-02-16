/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs Server Team                           *
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
#include <iostream>

#include "alcdebug.h"

namespace alc {

tString tStringTokenizer::getLine(bool nl,bool slash) {
	Byte c=0;
	Byte slashm=0;
	tString out;

	while(!str.eof()) {
		c=str.getByte();
		if(!slash) {
			if(c=='\\') {
				if(slashm) {
					slashm=0;
					out.putByte('\\');
					out.putByte('\\');
				} else {
					slashm=1;
				}
			} else if(c=='\n') {
				if(!str.eof() && str.getByte()!='\r') str.seek(-1);
				line++;
				col=0;
				if(!slashm) {
					break;
				}
				slashm=0;
				c=' ';
			} else if(c=='\r') {
				if(!str.eof() && str.getByte()!='\n') str.seek(-1);
				line++;
				col=0;
				if(!slashm) {
					break;
				}
				slashm=0;
				c=' ';
			} else {
				if(slashm) {
					slashm=0;
					out.putByte('\\');
				}
				out.putByte(c);
			}
		} else {
			if(c=='\n') {
				if(!str.eof() && str.getByte()!='\r') str.seek(-1);
				line++;
				col=0;
				break;
			} else if(c=='\r') {
				if(!str.eof() && str.getByte()!='\n') str.seek(-1);
				line++;
				col=0;
				break;
			} else {
				out.putByte(c);
			}
		}
	}
	if(nl) {
		if(c=='\n' || c=='\r') {
			out.putByte('\n');
		}
	}
	
	return out;
}
tString tStringTokenizer::getToken() {
	DBG(9,"tStringTokenizer::getToken()\n");
	Byte c;
	Byte slash=0;
	Byte quote=0;
	Byte mode=0;
	tString out;
	//out.hasQuotes(true);
	//assert(out.hasQuotes());
	while(!str.eof()) {
		c=str.getByte();
		col++;
		if(quote==0 && (c=='#' || c==';')) {
			if (out.size()) { // we already have something in out, dont attach the newline to it but make it the next token
				col--;
				str.seek(-1);
			} else {
				getLine();
				out.putByte('\n');
			}
			break;
		} else if(slash==1) {
			slash=0;
			if(quote==1 && (c=='n' || c=='r')) {
				if(c=='n') out.putByte('\n');
				else out.putByte('\r');
			} else if(c=='\n' || c=='\r') {
				if(c=='\n') {
					if(!str.eof() && str.getByte()!='\r') str.seek(-1);
					line++;
					col=0;
				} else {
					if(!str.eof() && str.getByte()!='\n') str.seek(-1);
					line++;
					col=0;
				}
			} else {
				if(quote==1) {
					out.putByte(c);
				} else {
					throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected '\\'\n",line,col));
				}
			}
		} else if(c=='\"') {
			if(quote==1) {
				quote=0;
				break;
			} else {
				quote=1;
			}
		} else if(c=='\n' || c=='\r') {
			if(mode==1 && quote==0) {
				str.seek(-1);
				c=0;
				break;
			} else {
				//out.putByte(c);
				out.putByte('\n');
				if(c=='\n') {
					if(!str.eof() && str.getByte()!='\r') str.seek(-1);
					//else out.putByte('\r');
					line++;
					col=0;
				} else {
					if(!str.eof() && str.getByte()!='\n') str.seek(-1);
					//else out.putByte('\n');
					line++;
					col=0;
				}
				if(quote==0) {
					break;
				}
			}
		} else if(c=='\\') {
			slash=1; 
		} else if(quote==0 && (c==' ' || c==sep || c==',' || isblank(c))) {
			if(mode==1) {
				if(c==sep || c==',') str.seek(-1);
				break;
			} else {
				if(c==sep || c==',') {
					out.putByte(c);
					break;
				}
			}
		} else if(isalpha(c) || isprint(c) || alcIsAlpha(c)) {
			out.putByte(c);
			mode=1;
		} else {
			throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected character '%c'\n",line,col,c));
		}
	}
	DBG(9,"    result: %s\n", out.c_str());
	return out;
}

tSimpleParser::tSimpleParser() {
	cfg=NULL;
}
U32 tSimpleParser::size() const {
	tString s;
	stream(s);
	return s.size();
}
void tSimpleParser::store(tBBuf &t) {
	store(tString(t));
}
void tSimpleParser::stream(tBBuf &t) const {
	tString s;
	stream(s);
	t.put(s);
}

void tSimpleParser::store(const tString &str) {
	if(!cfg) return;
	tStringTokenizer t(str);
	tString key,val;
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
			if(val=="\n") { // we arleady have the newline
				val.clear();
			}
			else { // expect the newline now
				if (t.getToken()!="\n") {
					throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token: A newline was expected.\n",t.getLineNum(),t.getColumnNum()));
				}
			}
			cfg->setVar(val,key,"global");
		}
	}	
}
void tSimpleParser::stream(tString &str) const {
	//U32 start=t.tell();
	DBG(4,"stream()\n");
	if(!cfg) return;
	DBG(5,"cfg->rewind()\n");
	cfg->rewind();
	tConfigKey * key;
	while((key=cfg->getNext())) {
		DBG(5,"cfg->getNext()\n");
		tConfigVal * val;
		while((val=key->getNext())) {
			DBG(5,"key->getNext()\n");
			str.writeStr(val->getName());
			//t.writeStr(" ");
			str.putByte(' ');
			//t.writeStr(" ");
			str.writeStr("\"");
			str.writeStr(val->getVal().escape());
			str.writeStr("\"");
			str.nl();
		}
	}
	//return (t.tell()-start);
}
void tSimpleParser::setConfig(tConfig * c) {
	DBG(5,"setconfig()\n");
	cfg=c;
}

tXParser::tXParser(bool override) :tSimpleParser() {
	this->override = override;
}

void tXParser::setBasePath(const tString & base) {
	path=base;
}

void tXParser::store(tBBuf &t) {
	tString s(t);
	store(s);
}
void tXParser::stream(tBBuf &t) const {
	tString s;
	stream(s);
	t.put(s);
}
void tXParser::store(const tString &str) {
	if(!cfg) return;
	tStringTokenizer t(str);
	tString section,key,val;
	DBG(4,"Store\n");
	section="global";
	U16 x,y;
	bool found;

	while(!t.eof()) {
		key = t.getToken();
		if(key!="\n") {
			DBG(9,"Reading token %s\n",key.c_str());
			//check for section
			if(key.startsWith("[") && key.endsWith("]")) {
				section=key.substring(1, key.size()-2);
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
				tString oldpath = path;
				path = key.dirname();
				tFBuf f(key.c_str());
				f.get(*this);
				path = oldpath;
				continue;
			}
			x=0; y=0;
			//get var name
			if(key=="=") {
				throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected token '%s'. A variable name was expected.\n",t.getLineNum(),t.getColumnNum(),key.c_str()));
			} else {
				//get table offsets
				y=alcParseKey(&key);
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
					if (!override) {
						// find next empty row (overwriting is disabled)
						do {
							cfg->getVar(key,section,x,y,&found);
							if (found) ++y;
						} while (found);
					}
					if(val=="\n") {
						val.clear();
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

void tXParser::stream(tString &str) const {
	DBG(4,"stream()\n");
	if(!cfg) return;
	DBG(5,"cfg->rewind()\n");
	cfg->rewind();
	tConfigKey * key;
	while((key=cfg->getNext())) {
		DBG(5,"cfg->getNext()\n");
		tConfigVal * val;
		str.writeStr("\n[" + key->getName() + "]\n");
		while((val=key->getNext())) {
			DBG(5,"key->getNext()\n");
			U16 x,y,mx,my;
			mx=val->getCols();
			my=val->getRows();

			for(y=0; y<my; y++) {
				if (!val->hasVal(0, y)) continue;
				str.writeStr(val->getName());
				if(my!=1) {
					str.writeStr("[");
					str.printf("%d", y);
					str.writeStr("]");
				}
				str.writeStr(" = ");
				for(x=0; x<mx && val->hasVal(x, y); x++) {
					if(x!=0) str.writeStr(",");
					str.writeStr("\"");
					str.writeStr(val->getVal(x,y).escape());
					str.writeStr("\"");
				}
				str.nl();
			}
		}
	}
}


} //end namespace alc

