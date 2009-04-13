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

class tpObject;

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
	tUStr(void) : tStrBuf() {}
	tUStr(const char *k) : tStrBuf(k) {}
	tUStr(const tUStr &t) : tStrBuf(t) {}
	tUStr(const tStrBuf &t) : tStrBuf(t) {}
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t);
	
	// assignment
	virtual const tUStr & operator=(const tUStr &t) { copy(t); return *this; }
	virtual const tStrBuf & operator=(const tStrBuf &t) { copy(t); return *this; }
	virtual const tStrBuf & operator=(const char *t) { copy(t); return *this; }
};

/** UruObject */
class tUruObject : public tBaseType { // equivalent to plUoid in Plasma
public:
	tUruObject(void);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t);
	const char *str(void);
	
	bool operator==(const tUruObject &obj);
	inline bool operator!=(const tUruObject &obj) {
		return !(*this == obj);
	}

	bool hasCloneId;
	U32 pageId;
	U16 pageType;
	U16 objType;
	tUStr objName;
	U32 cloneId;
	U32 clonePlayerId;
private:
	tStrBuf dbg;
};

class tUruObjectRef : public tBaseType { // equivalent to the key reader of the resource manager in Plasma
public:
	tUruObjectRef(void);
	tUruObjectRef(const tUruObject &obj);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t);
	const char *str(void);

	bool hasObj;
	tUruObject obj;
};

/** StreamedObject */
class tStreamedObject : public tMBuf {
public:
	tStreamedObject(U16 type = plNull) : tMBuf(), type(type) { format = 0x00; realSize = 0; }
	tStreamedObject(tpObject *obj);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t);
	
	inline U16 getType(void) { return type; }
	void uncompress(void); //!< call this before using it
	void compress(U32 minSize = 256); //!< call this before streaming or sending it
	void eofCheck(void);
	
	// assignment
	const tStreamedObject &operator=(const tStreamedObject &t) { copy(t); return *this; }
protected:
	//! assignment
	virtual void copy(const tStreamedObject &t);
private:
	
	U32 realSize; // if flag is 0x02, this saves the uncompressed size, otherwise, it is zero
	Byte format; // 0x00, 0x03: uncompressed, 0x02: compressed
	U16 type;
};

} //End alc namespace

#endif
