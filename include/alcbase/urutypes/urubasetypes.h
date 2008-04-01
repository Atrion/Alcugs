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

#ifndef __U_URUBASETYPES_H
#define __U_URUBASETYPES_H
/* CVS tag - DON'T TOUCH*/
#define __U_URUBASETYPES_H_ID "$Id$"

namespace alc {

/** Wdys buffer */
class tWDYSBuf :public tMBuf {
public:
	tWDYSBuf() :tMBuf() {}
	void encrypt();
	void decrypt();
};

/** AES buffer */
class tAESBuf :public tMBuf {
public:
	tAESBuf() :tMBuf() {}
	void encrypt();
	void decrypt();
	void setKey(Byte * key);
	void setM5Key();
private:
	Byte key[16];
};

/** Urustring */
class tUStr :public tStrBuf {
public:
	/**
		\param mode
		0 - normal
		1 - auto (normal/inverted) Uru (plasma 2.0) [Please avoid using this mode]
		5 - inverted
		6 - myst5 (plasma 2.1)
	*/
	tUStr(Byte mode=1);
	virtual int stream(tBBuf &buf);
	virtual int stream(tBBuf &buf,bool inv);
	virtual void store(tBBuf &buf);
	virtual U32 size();
	void setVersion(Byte version) { this->version=version; }
	void set(Byte * val,U32 _s=0);
	void set(char * val,U32 _s=0);
	void set(tUStr &str);
	Byte * str();
	U32 len();
	virtual ~tUStr();
	virtual tUStr & operator=(tStrBuf &t) { this->copy(t); return *this; }
	virtual tUStr & operator=(const tStrBuf &t) { this->copy((tStrBuf &)t); return *this; }
	virtual tUStr & operator=(const void * str) { this->copy(str); return *this; }

private:
	Byte version;
	Byte * name;
	U16 msize;
};

} //End alc namespace

#endif
