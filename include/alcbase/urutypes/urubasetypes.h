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
	void decrypt(bool mustBeWDYS = true);
};

/** AES buffer */
class tAESBuf :public tMBuf {
public:
	tAESBuf() :tMBuf() {}
	void encrypt();
	void decrypt();
	void setKey(const Byte * key);
	void setM5Key();
private:
	Byte key[16];
};

/** Urustring */
class tUruString :public tString {
public:
	tUruString(void) : tString() {}
	tUruString(const tUruString &t) : tString(t) {}
	tUruString(const char *k) : tString(k) {}
	tUruString(const tString &t) : tString(t) {}
	
	// interface
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	
	// assignment
	inline const tUruString & operator=(const tUruString &t) { copy(t); return *this; }
	inline const tString & operator=(const tString &t) { copy(t); return *this; }
	inline const tString & operator=(const char *t) { copy(t); return *this; }
};

/** UruObject */
class tUruObject : public tBaseType { // equivalent to plUoid in Plasma
public:
	tUruObject(void);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	const char *str(void) const;
	
	bool operator==(const tUruObject &obj) const;
	inline bool operator!=(const tUruObject &obj) const {
		return !(*this == obj);
	}

	bool hasCloneId;
	U32 pageId;
	U16 pageType;
	U16 objType;
	tUruString objName;
	U32 cloneId;
	U32 clonePlayerId;
private:
	mutable tString dbg;
};

class tUruObjectRef : public tBaseType { // equivalent to the key reader of the resource manager in Plasma
public:
	tUruObjectRef(void);
	tUruObjectRef(const tUruObject &obj);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	const char *str(void) const;

	bool hasObj;
	tUruObject obj;
};

/** StreamedObject */
class tStreamedObject : public tMBuf {
public:
	tStreamedObject(U16 type = plNull) : tMBuf(), maxSize(256), type(type) // make sure this is the same maxSize as in urubasetypes.cpp
		{ format = 0x00; realSize = 0; }
	tStreamedObject(tpObject *obj);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	
	inline U16 getType(void) { return type; }
	void uncompress(void); //!< call this before using it
	void compress(void); //!< call this before streaming or sending it
	void eofCheck(void);
	tMBuf fullContent(void);
	
	// assignment
	const tStreamedObject &operator=(const tStreamedObject &t) { copy(t); return *this; }
protected:
	//! assignment
	virtual void copy(const tStreamedObject &t);
private:

	const U32 maxSize;
	
	U32 realSize; // if flag is 0x02, this saves the uncompressed size, otherwise, it is zero
	Byte format; // 0x00, 0x03: uncompressed, 0x02: compressed
	U16 type;
};

} //End alc namespace

#endif
