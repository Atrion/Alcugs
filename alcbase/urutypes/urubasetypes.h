/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

#ifndef __U_URUBASETYPES_H
#define __U_URUBASETYPES_H
/* CVS tag - DON'T TOUCH*/
#define __U_URUBASETYPES_H_ID "$Id$"

#include "alctypes.h"
#include "uruconsts.h"

namespace alc {
	
// helper fucntions
inline uint16_t alcOpcodeUU2POTS(uint16_t opcode)
{
	return opcode > 0x0400 && opcode < 0x500 ? opcode+1 : opcode;
}
inline uint16_t alcOpcodePOTS2UU(uint16_t opcode)
{
	return opcode > 0x0400 && opcode < 0x0500 ? opcode-1 : opcode;
}

/** convert pageIDs to pageNumbers and the other way around - wired, but whatever... */
inline uint16_t alcPageIdToNumber(uint32_t seqPrefix, uint32_t pageId)
{
	return pageId - (seqPrefix << 8) - 33;
}
inline uint32_t alcPageNumberToId(uint32_t seqPrefix, uint16_t number)
{
	return (seqPrefix << 8) + 33 + number;
}

const char * alcGetLinkingRule(uint8_t rule);

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
	void setKey(const void * key);
	void setM5Key();
protected:
	tAESBuf(size_t size) : tMBuf(size) {}
private:
	uint8_t key[16];
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
	const tUruString & operator=(const tUruString &t) { copy(t); return *this; }
	const tString & operator=(const tString &t) { copy(t); return *this; }
	const tString & operator=(const char *t) { copy(t); return *this; }
};

/** UruObject */
class tUruObject : public tStreamable { // equivalent to plUoid in Plasma
public:
	tUruObject(void);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tString str(void) const;
	
	bool operator==(const tUruObject &obj) const;
	bool operator!=(const tUruObject &obj) const {
		return !(*this == obj);
	}

	bool hasCloneId;
	uint32_t pageId;
	uint16_t pageType;
	uint16_t objType;
	tUruString objName;
	uint32_t cloneId;
	uint32_t clonePlayerId;
};

class tUruObjectRef : public tStreamable { // equivalent to the key reader of the resource manager in Plasma
public:
	tUruObjectRef(void);
	tUruObjectRef(const tUruObject &obj);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tString str(void) const;

	bool hasObj;
	tUruObject obj;
};

/** StreamedObject */
class tStreamedObject : public tMBuf {
public:
	tStreamedObject() : tMBuf(), maxSize(256), type(plNull) // make sure this is the same maxSize as in urubasetypes.cpp
		{ format = 0x00; realSize = 0; }
	tStreamedObject(tpObject *obj, bool UUFormat);
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	
	uint16_t getType(void) { return type; }
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
	const size_t maxSize;
	
	size_t realSize; // if flag is 0x02, this saves the uncompressed size, otherwise, it is zero
	uint8_t format; // 0x00, 0x03: uncompressed, 0x02: compressed
	uint16_t type; //!< the sent type - not canonized to TPOTS!
};

class tAgeInfoStruct : public tStreamable {
public:
	tAgeInfoStruct(const tString &filename, const tString &instanceName, const tString &userDefName, const tString &displayName, const uint8_t *guid);
	tAgeInfoStruct(const tString &filename, const uint8_t *guid);
	tAgeInfoStruct(const tAgeInfoStruct &);
	tAgeInfoStruct(void) {}
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	bool hasGuid(void) { return (flags & 0x04); }
	tString str(void) const;
	// format
	uint8_t flags;
	tString filename, instanceName;
	uint8_t guid[8];
	tString userDefName, displayName;
};

class tSpawnPoint : public tStreamable {
public:
	tSpawnPoint(const tString &title, const tString &name, const tString &cameraStack = "");
	tSpawnPoint(void) {}
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tString str(void) const;
	// format
	uint32_t flags;
	tString title, name, cameraStack;
};

class tAgeLinkStruct : public tStreamable {
public:
	tAgeLinkStruct(void) {}
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tString str(void) const;
	// format
	uint16_t flags;
	tAgeInfoStruct ageInfo;
	uint8_t linkingRule;
	tSpawnPoint spawnPoint;
	uint8_t ccr;
	tString parentAgeName;
};

} //End alc namespace

#endif
