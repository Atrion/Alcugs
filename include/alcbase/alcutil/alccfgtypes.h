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
	tConfigVal(const char * name);
	tConfigVal(const tStrBuf & name);
	~tConfigVal();
	void setName(const char * name);
	void setName(const tStrBuf & name);
	void setVal(const char * val,U16 x=0,U16 y=0);
	void setVal(const tStrBuf & t,U16 x=0,U16 y=0);
	const tStrBuf & getName() const;
	const tStrBuf & getVal(U16 x=0,U16 y=0) const;
	U16 getRows() const { return y; }
	U16 getCols() const { return x; }
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
	void setName(const char * name);
	void setName(const tStrBuf & name);
	const tStrBuf & getName() const { return name; }
	tConfigVal * find(const char * what,bool create=false);
	tConfigVal * find(const tStrBuf & what,bool create=false);
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
	tConfigKey * findKey(const char * where="global",bool create=false);
	tConfigKey * findKey(const tStrBuf & where,bool create=false);
	tConfigVal * findVar(const char * what,const char * where="global",bool create=false);
	const tStrBuf & getVar(const char * what,const char * where="global",U16 x=0,U16 y=0);
	void setVar(const char * val,const char * what,const char * where="global",U16 x=0,U16 y=0);
	void setVar(const tStrBuf &val,const tStrBuf &what,const tStrBuf &where,U16 x=0,U16 y=0);
	void rewind();
	tConfigKey * getNext();
	void copy(const char * to,const char * from);
	void copyKey(const char * tok,const char * fromk,const char * to,const char * from);
private:
	U16 n,off;
	tConfigKey ** values;
};

} //End alc namespace

#endif
