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

class tConfigVal {
public:
	tConfigVal();
	tConfigVal(Byte * name);
	~tConfigVal();
	void init();
	void setName(Byte * name);
	void setVal(Byte * val,U16 x=0,U16 y=0);
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
};

class tConfigKey {
public:
	tConfigKey();
	~tConfigKey();
private:
	Byte * key;
	U16 n;
	tConfigVal ** vals;
};


class tConfig {
public:
	tConfig();
	~tConfig();
	void setKey(Byte * val,Byte * what,Byte * where);
	//void setKey(tConfigVal & t
	tConfigVal * getKey(Byte * what,Byte * where);
private:
	int n;
	tConfigKey ** keys;
};

} //End alc namespace

#endif
