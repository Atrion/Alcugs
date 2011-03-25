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

/* CVS tag - DON'T TOUCH*/
#define __U_URUBASICTYPES_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "urubasetypes.h"

namespace wdys {
	#include "urutypes/whatdoyousee.h"
}
#include "alcutil/rijndael.h"
#include "alcexception.h"
#include "plbase.h"

#include <cstring>


namespace alc {

const char * alcGetLinkingRule(uint8_t rule)
{
	switch (rule) {
		case KBasicLink: return "KBasicLink";
		case KOriginalBook: return "KOriginalBook";
		case KSubAgeBook: return "KSubAgeBook";
		case KOwnedBook: return "KOwnedBook";
		case KVisitBook: return "KVisitBook";
		case KChildAgeBook: return "KChildAgeBook";
		default: return "Unknown";
	}
}

/* wdys buff */
void tWDYSBuf::encrypt() {
	// first, encrypt ourselves in-place (and make sure we are big enough for that by adding zeroes)
	size_t dataSize = size(), newSize;
	end();
	put32(0);
	put32(0);
	for (newSize=0; newSize<dataSize; newSize+=8) {
		wdys::encodeQuad(reinterpret_cast<unsigned int *>(volatileData()+newSize), reinterpret_cast<unsigned int *>(volatileData()+newSize+4)); // this function encrypts in-place, hence work on a copy
	}
	
	// and now put it into a new buffer and assign that one to us
	tWDYSBuf newBuf;
	newBuf.write("whatdoyousee",sizeof(char)*12);
	newBuf.put32(dataSize);
	newBuf.write(data(),newSize);
	copy(newBuf);
	rewind();
}
void tWDYSBuf::decrypt(bool mustBeWDYS) {
	// check header
	if(memcmp(data(),"whatdoyousee",12) != 0) {
		if (mustBeWDYS) throw txUnexpectedData(_WHERE("NotAWDYSFile!")); 
		return;
	}
	set(12); // seek to the size
	uint32_t dsize=get32();
	
	// decrypt ourselves in-place
	for (size_t i=0; i<dsize; i+=8) {
		// 16 is the header size
		wdys::decodeQuad(reinterpret_cast<unsigned int *>(volatileData()+16+i), reinterpret_cast<unsigned int *>(volatileData()+16+i+4));
	}
	
	// put the decrypted data into a new buffer and assign it to us
	tWDYSBuf newBuf;
	newBuf.write(data()+16, dsize); // 16 is the header size
	copy(newBuf);
	rewind();
}
/* end wdys buff */



/* AES buff */
void tAESBuf::setKey(const void * key) {
	memcpy(this->key,key,16);
}
void tAESBuf::setM5Key() {
	uint32_t xorkey=0xCF092676;
	tMBuf key;
	key.put32(0xFC2C6B86 ^ xorkey);
	key.put32(0x952E7BDA ^ xorkey);
	key.put32(0xF1713EE8 ^ xorkey);
	key.put32(0xC7410A13 ^ xorkey);
	memcpy(this->key,key.data(),16);
}
void tAESBuf::encrypt() {
	// encrypt the buffer
	tAESBuf newBuf(size()+16); // 8 bytes for the header, 8 bytes for AES
	newBuf.put32(0x0D874288);
	newBuf.put32(size());
	Rijndael rin;
	rin.init(Rijndael::ECB,Rijndael::Encrypt,key,Rijndael::Key16Bytes);
	int res = rin.padEncrypt(data(),size(),newBuf.volatileData()+8);
	if(res<0) throw txUnkErr(_WHERE("Rijndael encrypt error"));
	
	// res tells us how many Bytes were really used
	newBuf.cutEnd(res+8); // 8 bytes for the header
	copy(newBuf);
	rewind();
}
void tAESBuf::decrypt() {
	// get the data part
	rewind();
	if(get32()!=0x0D874288) throw txUnexpectedData(_WHERE("NotAM5CryptedFile!"));
	uint32_t dataSize = get32();
	tAESBuf newBuf(size()-8);

	// decrypt it
	Rijndael rin;
	rin.init(Rijndael::ECB,Rijndael::Decrypt,key,Rijndael::Key16Bytes);
	int res = rin.padDecrypt(data()+8,size()-8,newBuf.volatileData());
	if(res<0) throw txUnkErr(_WHERE("Rijndael decrypt error %i",res));
	if(static_cast<size_t>(res) != size()-8) throw txUnkErr(_WHERE("Rijndael decrypt error: Size mismatch"));

	// and use what we need
	newBuf.cutEnd(dataSize);
	copy(newBuf);
	rewind();
}
/* end AES buff */


/* tUruString */
void tUruString::store(tBBuf &t) {
	clear();
	uint16_t bufSize = t.get16();
	if (!(bufSize & 0xF000)) throw txUnexpectedData(_WHERE("This is not an inverted string!"));
	bufSize &= ~0xF000; // remove highest bit
	for(uint16_t i=0; i<bufSize; i++) {
		put8(~t.get8());
	}
	
	/* Myst V decryption
	Byte key[9]="mystnerd";
	for(U32 i=0; i<nsize; i++) {
		putByte(b.getByte() ^ key[i%8]);
	} */
}
void tUruString::stream(tBBuf &t) const {
	if (size() >= 0xF000) throw txUnexpectedData(_WHERE("Uru string is too long"));
	t.put16(size()|0xF000);
	for(size_t i=0; i<size(); i++) {
		t.put8(~*(data()+i));
	}
	
	/* Myst V encryption
	Byte key[9]="mystnerd";
	for(U32 i=0; i<msize; i++) {
		b.putByte(buf->buf[i] ^ key[i%8]);
	} */
}
/* end tUruString */

/* tUruObject (Plasma: plUoid) */
tUruObject::tUruObject(void)
{
	hasCloneId = 0;
	pageId = 0;
	pageType = objType = 0;
	cloneId = clonePlayerId = 0;
}

void tUruObject::store(tBBuf &t)
{
	uint8_t cloneIdFlag = t.get8();
	if (cloneIdFlag != 0x00 && cloneIdFlag != 0x01)
		throw txUnexpectedData(_WHERE("the clone ID flag of an UruObject must be 0x00 or 0x01, not 0x%02X", cloneIdFlag));
	else hasCloneId = cloneIdFlag;
	pageId = t.get32();
	pageType = t.get16();
	objType = t.get16();
	t.get(objName);
	
	// if contained, read the client ID
	if (hasCloneId) {
		cloneId = t.get32();
		clonePlayerId = t.get32();
	}
}

void tUruObject::stream(tBBuf &t) const
{
	t.put8(hasCloneId);
	t.put32(pageId);
	t.put16(pageType);
	t.put16(objType);
	t.put(objName);
	if (hasCloneId) {
		t.put32(cloneId);
		t.put32(clonePlayerId);
	}
}

tString tUruObject::str(void) const
{
	tString dbg;
	dbg.printf("Page ID: 0x%08X, Page Type: 0x%04X, Object: [0x%04X]%s", pageId, pageType, objType, objName.c_str());
	if (hasCloneId)
		dbg.printf(", Clone ID: %d, Clone Player ID: %d", cloneId, clonePlayerId);
	return dbg;
}

bool tUruObject::operator==(const tUruObject &obj) const
{
	if (pageId != obj.pageId) return false;
	if (pageType != obj.pageType) return false;
	if (objType != obj.objType) return false;
	if (objName != obj.objName) return false;
	if (hasCloneId != obj.hasCloneId) return false;
	if (cloneId != obj.cloneId) return false;
	if (clonePlayerId != obj.clonePlayerId) return false;
	return true;
}
/* end tUruObject */

/* tUruObjectRef (Plasma: plKey) */
tUruObjectRef::tUruObjectRef(void)
{
	hasObj = false;
}

tUruObjectRef::tUruObjectRef(const tUruObject &obj) : obj(obj)
{
	hasObj = true;
}

void tUruObjectRef::store(tBBuf &t)
{
	uint8_t hasObjFlag = t.get8();
	if (hasObjFlag != 0x00 && hasObjFlag != 0x01)
		throw txUnexpectedData(_WHERE("the hasObjFlag of an UruObjectRef must be 0x00 or 0x01, not 0x%02X", hasObjFlag));
	hasObj = hasObjFlag;
	if (hasObj) t.get(obj);
}

void tUruObjectRef::stream(tBBuf &t) const
{
	t.put8(hasObj);
	if (hasObj) t.put(obj);
}

tString tUruObjectRef::str(void) const
{
	if (hasObj) return obj.str();
	else return "null";
}

/* tStreamedObject (Plasma: plNetMsgStreamHelper) */
tStreamedObject::tStreamedObject(tpObject *obj, bool UUFormat) : tMBuf(), maxSize(256) // make sure this is the same maxSize as in urubasetypes.h
{
	obj->setUUFormat(UUFormat);
	type = UUFormat ? alcOpcodePOTS2UU(obj->getType()) : obj->getType();
	format = 0x00;
	realSize = 0;
	put(*obj);
	compress();
}

void tStreamedObject::store(tBBuf &t)
{
	DBG(8, "tStreamedObject::store\n");
	clear();
	
	realSize = t.get32();
	format = t.get8();
	if (format != 0x00 && format != 0x02 && format != 0x03)
		throw txUnexpectedData(_WHERE("Invalid stream format 0x%02X", format));
	if (format != 0x02 && realSize)
		throw txUnexpectedData(_WHERE("Uncompressed stream must not have real size set"));
	uint32_t sentSize = t.get32();
	if (sentSize == 0) {
		type = plNull;
		return;
	}
	// strip out the two type bytes
	realSize -= 2;
	sentSize -= 2;
	
	type = t.get16();
	write(t.read(sentSize), sentSize);
	
	uncompress();
}

void tStreamedObject::stream(tBBuf &t) const
{
	DBG(8, "tStreamedObject::stream\n");
	if (!size()) {
		// write an empty stream
		t.put32(0); // real size
		t.put8(0x00); // compression flag
		t.put32(0); // sent size
		return;
	}
	else if (size() > maxSize && format == 0x00) // an uncompressed stream of that size?
		throw txBase(_WHERE("Someone forgot to call tStreamedObject::compress()"));
	
	// it's not yet empty, so we have to write something
	t.put32(format == 0x02 ? realSize+2 : 0); // add the two type bytes
	t.put8(format);
	t.put32(size()+2); // add the two type bytes
	t.put16(type);
	tMBuf::stream(t);
}

void tStreamedObject::uncompress(void)
{
	if (format == 0x02) {
		DBG(8, "tStreamedObject::uncompress\n");
		tZBuf content(*this);
		content.uncompress(realSize);
		tMBuf::copy(content);
		format = 0x00;
	}
	rewind();
}

void tStreamedObject::compress(void)
{
	if (format == 0x02 || size() <= maxSize) return; // nothing to do
	DBG(8, "tStreamedObject::compress\n");
	// compress it
	realSize = size();
	tZBuf content(*this);
	content.compress();
	tMBuf::copy(content);
	format = 0x02; // save that we compressed it so that we don't do it again
}

void tStreamedObject::eofCheck(void)
{
	if (!eof())
		throw txUnexpectedData(_WHERE("Got a %s which is too long: %d bytes remaining after parsing", alcGetPlasmaType(type), remaining()));
}

tMBuf tStreamedObject::fullContent(void)
{
	uncompress();
	tMBuf res;
	res.put16(type);
	tMBuf::stream(res);
	res.rewind();
	return res;
}

void tStreamedObject::copy(const tStreamedObject &t)
{
	if (&t == this) return;
	tMBuf::copy(t);
	format = t.format;
	type = t.type;
}

//// tAgeInfoStruct
tAgeInfoStruct::tAgeInfoStruct(const tString &filename, const tString &instanceName, const tString &userDefName, const tString &displayName, const uint8_t *guid)
: filename(filename), instanceName(instanceName), userDefName(userDefName), displayName(displayName)
{
	flags = 0x01 | 0x02 | 0x04 | 0x08 | 0x20; // instanceName, filename, GUID, user defined name, display name
	memcpy(this->guid, guid, 8);
}

tAgeInfoStruct::tAgeInfoStruct(const tString &filename, const uint8_t *guid) : filename(filename)
{
	flags =  0x02 | 0x04; // filename, GUID
	memcpy(this->guid, guid, 8);
}

tAgeInfoStruct::tAgeInfoStruct(const uint8_t *guid)
{
	flags =  0x04; // GUID
	memcpy(this->guid, guid, 8);
}

tAgeInfoStruct::tAgeInfoStruct(const tString &filename, const tString &instanceName, const uint8_t *guid, uint32_t sequenceNumber)
: filename(filename), instanceName(instanceName), sequenceNumber(sequenceNumber)
{
	flags =  0x01 | 0x02 | 0x04 | 0x10; // filename, instance name, GUID, sequence number
	memcpy(this->guid, guid, 8);
}

void tAgeInfoStruct::store(tBBuf &t)
{
	//AgeInfoStruct flags
	//Found:
	// 0x02 filename
	// 0x03 filename,instance name
	// 0x0B filename,instance name,user name
	// 0x0F filename,instance name,guid,user name
	// 0x17 filename,instance name,guid,sequence number
	// 0x2F filename,instance name,guid,user name,display name
	// 0x6F filename,instance name,guid,user name,display name,language
	//Supposicions:
	// 0x01: instance name
	// 0x02: filename (must always be set)
	// 0x04: The Age Guid
	// 0x08: The user defined name
	// 0x10: Sequence number (only seen in public age list)
	// 0x20: DisplayName (Desc's name)
	// 0x40: Language
	flags = t.get8();
	uint8_t check = 0x02 | 0x01 | 0x04 | 0x08 | 0x10 | 0x20 | 0x40;
	if (flags & ~(check))
		throw txUnexpectedData(_WHERE("unknown flag 0x%02X for AgeInfoStruct", flags));
	if (!(flags & 0x02)) // this must always be set (filename)
		throw txUnexpectedData(_WHERE("the 0x02 flag must always be set in AgeInfoStruct"));
	
	t.get(filename);
	if (filename.isEmpty()) throw txUnexpectedData(_WHERE("An empty age filename is not allowed"));
	
	if (flags & 0x01) // instance name
		t.get(instanceName);
	else { // instance name disabled
		throw txUnexpectedData(_WHERE("instance name flag not set... what to do?"));
	}
	
	if (flags & 0x04) // GUID
		memcpy(guid, t.read(8), 8);
	else
		memset(guid, 0, 8); // some parts of the server rely on this being all zero when no GUID is set
	
	if (flags & 0x08) // user defined name
		t.get(userDefName);
	
	if (flags & 0x10) // sequence number
		sequenceNumber = t.get32();
	else
		sequenceNumber = 0;
	
	if (flags & 0x20) // display name
		t.get(displayName);
	
	if (flags & 0x40) { // language
		// this is not the language of the client, but something else
		uint32_t language = t.get32(); // always seen 0
		if (language != 0) throw txUnexpectedData(_WHERE("Language value of an AgeInfoStruct is 0x%08X instead of 0\n", language));
	}
}

void tAgeInfoStruct::stream(tBBuf &t) const
{
	if (!hasFilename()) throw txUnexpectedData(_WHERE("Can not stream a tAgeInfoStruct without filename"));
	// see store for description of flags
	t.put8(flags);
	
	t.put(filename);
	
	if (flags & 0x01) // instance name
		t.put(instanceName);
	
	if (flags & 0x04) // GUID
		t.write(guid, 8);
	
	if (flags & 0x08) // user defined name
		t.put(userDefName);
	
	if (flags & 0x10) // sequence number
		t.put32(sequenceNumber);
	
	if (flags & 0x20) // display name
		t.put(displayName);
	
	if (flags & 0x40) // language
		t.put32(0);
}

tString tAgeInfoStruct::str(void) const
{
	tString dbg;
	dbg.printf("Filename: %s", filename.c_str());
	if (flags & 0x01) // instance name
		dbg.printf(", Instance Name: %s", instanceName.c_str());
	if (flags & 0x04) // GUID
		dbg.printf(", GUID: %s", alcGetStrGuid(guid).c_str());
	if (flags & 0x08) // user defined name
		dbg.printf(", User defined name: %s", userDefName.c_str());
	if (flags & 0x20) // display name
		dbg.printf(", Display name: %s", displayName.c_str());
	if (flags & 0x40) // language
		dbg.printf(", Language: 0");
	return dbg;
}

//// tSpawnPoint
tSpawnPoint::tSpawnPoint(const tString &title, const tString &name, const tString &cameraStack) : title(title), name(name), cameraStack(cameraStack)
{
	flags = 0x00000007;
}

void tSpawnPoint::store(tBBuf &t)
{
	//tSpawnPoint flags
	//Found:
	// always 0x00000007
	//Supposicions:
	// 0x00000007: 3 bits for title, name and cameraStack
	flags = t.get32();
	if (flags != 0x00000007) throw txUnexpectedData(_WHERE("The SpawnPoint flag must always be 0x00000007 (it is 0x%08X)", flags));
	t.get(title);
	t.get(name);
	t.get(cameraStack);
}

void tSpawnPoint::stream(tBBuf &t) const
{
	// see store for description of flags
	t.put32(flags);
	t.put(title);
	t.put(name);
	t.put(cameraStack);
}

tString tSpawnPoint::str(void) const
{
	tString dbg;
	dbg.printf("Title: %s, Name: %s, Camera Stack: %s", title.c_str(), name.c_str(), cameraStack.c_str());
	return dbg;
}

//// tAgeLinkStruct
void tAgeLinkStruct::store(tBBuf &t)
{
	//AgeLinkStruct flags
	//Found:
	// 0x0023 In VaultTasks
	// 0x0033 In FindAge msg's
	// 0x0073 Found when linking to Ahnonay (temple) from Restoration Guild
	//Supposicions:
	// 0x0023: 3 bits for AgeInfoStruct LinkingRules and SpawnPoint (must always be set)
	// 0x0010: CCR flag
	// 0x0040: Parent age name (according to libPlasma)
	flags = t.get16();
	uint16_t check = 0x0023 | 0x0010 | 0x0040;
	if (flags & ~(check))
		throw txUnexpectedData(_WHERE("unknown flag 0x%04X for AgeLinkStruct", flags));
	if (!(flags & 0x0023)) // this must always be set (AgeInfoStruct, LinkingRules and SpawnPoint)
		throw txUnexpectedData(_WHERE("the 0x0023 flag must always be set in AgeLinkStruct"));
	
	t.get(ageInfo);
	linkingRule = t.get8();
	uint32_t unk = t.get32(); // unknown, always seen 0x00000001
	if (unk != 0x00000001)
		throw txUnexpectedData(_WHERE("unknown unk value for AgeLinkStruct, must always be 0x00000001 but is 0x%08X", unk));
	t.get(spawnPoint);
	
	// now come the optional fields
	if (flags & 0x0010) // CCR
		ccr = t.get8();
	else
		ccr = 0;
	
	if (flags & 0x0040) // parent age name
		t.get(parentAgeName);
}

void tAgeLinkStruct::stream(tBBuf &t) const
{
	// see store for description of flags
	t.put16(flags);
	
	t.put(ageInfo);
	t.put8(linkingRule);
	t.put32(0x00000001); // unknown
	t.put(spawnPoint);
	
	// optional fields
	if (flags & 0x0010) // CCR
		t.put8(ccr);
	if (flags & 0x0040) // parent age name
		t.put(parentAgeName);
}

tString tAgeLinkStruct::str(void) const
{
	tString dbg;
	dbg.printf("Age Info [%s], Linking Rule: 0x%02X (%s), Spawn Point [%s]", ageInfo.str().c_str(),
			   linkingRule, alcGetLinkingRule(linkingRule), spawnPoint.str().c_str());
	if (flags & 0x0010) // CCR
		dbg.printf(", CCR: 0x%02X", ccr);
	if (flags & 0x0040) // parent age name
		dbg.printf(", Parent Age: %s", parentAgeName.c_str());
	return dbg;
}

} //end namespace alc
