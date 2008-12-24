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

/** \brief A configuration value, can be a tupple */
class tConfigVal {
public:
	tConfigVal();
	tConfigVal(const void * name);
	tConfigVal(tStrBuf & name);
	~tConfigVal();
	void setName(const void * name);
	void setName(tStrBuf & name);
	void setVal(const void * val,U16 x=0,U16 y=0);
	void setVal(tStrBuf & t,U16 x=0,U16 y=0);
	tStrBuf & getName();
	tStrBuf & getVal(U16 x=0,U16 y=0);
	U16 getRows() { return y; }
	U16 getCols() { return x; }
	void copy(tConfigVal & t);
	const tConfigVal &operator=(tConfigVal & t) { copy(t); return *this; }
private:
	void init();
	tStrBuf name;
	tStrBuf ** values;
	U16 x; //columns
	U16 y; //rows
	Byte flags; // 0x01 - with quotes
	            // 0x02 - without quotes
	friend class tConfigKey; // for more efficient searching, tConfigKey needs direct access to the var name
};

/** \brief A configuration key, a group of several configuration values */
class tConfigKey {
public:
	tConfigKey();
	~tConfigKey();
	void setName(const void * name);
	void setName(tStrBuf & name);
	tStrBuf & getName() { return name; }
	tConfigVal * find(const void * what,bool create=false);
	tConfigVal * find(tStrBuf & what,bool create=false);
	void copy(tConfigKey & t);
	void merge(tConfigKey & t);
	void add(tConfigVal &t);
	const tConfigKey &operator=(tConfigKey & t) { copy(t); return *this; }
	void rewind();
	tConfigVal * getNext();
private:
	tStrBuf name;
	U16 n,off;
	tConfigVal ** values;
	friend class tConfig; // for more efficient searching, tConfig needs direct access to the key name
};


/** \brief A group of config keys */
class tConfig {
public:
	tConfig();
	~tConfig();
	tConfigKey * findKey(const void * where=(const void *)"global",bool create=false);
	tConfigKey * findKey(tStrBuf & where,bool create=false);
	tConfigVal * findVar(const void * what,const void * where=(const void *)"global",bool create=false);
	tStrBuf & getVar(const void * what,const void * where=(const void *)"global",U16 x=0,U16 y=0);
	void setVar(const void * val,const void * what,const void * where=(const void *)"global",U16 x=0,U16 y=0);
	void setVar(tStrBuf &val,tStrBuf &what,tStrBuf &where,U16 x=0,U16 y=0);
	void rewind();
	tConfigKey * getNext();
	void copy(const void * to,const void * from);
	void copyKey(const void * tok,const void * fromk,const void * to,const void * from);
private:
	U16 n,off;
	tConfigKey ** values;
};

} //End alc namespace

#endif
