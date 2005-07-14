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

#ifndef __U_ALCCFGTYPES_H
#define __U_ALCCFGTYPES_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCCFGTYPES_H_ID "$Id$"

namespace alc {

class tConfigKey;
class tConfig;

class tConfigVal {
public:
	tConfigVal();
	tConfigVal(const Byte * name);
	~tConfigVal();
	void init();
	void setName(const Byte * name);
	void setVal(const Byte * val,U16 x=0,U16 y=0);
	void setVal(tStrBuf & t,U16 x=0,U16 y=0);
	Byte * getName();
	tStrBuf * getVal(U16 x=0,U16 y=0);
	U16 getRows() { return y; }
	U16 getCols() { return x; }
	void copy(tConfigVal & t);
	void operator=(tConfigVal & t) { copy(t); }
private:
	Byte * name;
	tStrBuf ** values;
	U16 x; //columns
	U16 y; //rows
	friend class tConfigKey;
};

class tConfigKey {
public:
	tConfigKey();
	~tConfigKey();
	void setName(const Byte * name);
	Byte * getName() { return name; }
	tConfigVal * find(const Byte * what,bool create=false);
	tConfigVal * find(const char * what,bool create=false) { return(find((Byte *)what,create)); }
	void copy(tConfigKey & t);
	void operator=(tConfigKey & t) { copy(t); }
private:
	Byte * name;
	U16 n;
	tConfigVal ** values;
	friend class tConfig;
};


class tConfig {
public:
	tConfig();
	~tConfig();
	tConfigKey * findKey(const Byte * where=(const Byte *)"global",bool create=false);
	tConfigVal * findVar(const Byte * what,const Byte * where=(const Byte *)"global",bool create=false);
	void setVar(const Byte * val,const Byte * what,const Byte * where);
private:
	int n;
	tConfigKey ** values;
};

} //End alc namespace

#endif
