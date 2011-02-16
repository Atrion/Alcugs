/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs H'uru Server Team                     *
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

/* wdys buff */
void tWDYSBuf::encrypt() {
	// first, encrypt ourselves in-place (and make sure we are big enough for that by adding zeroes)
	U32 dataSize = size(), newSize;
	end();
	putU32(0);
	putU32(0);
	for (newSize=0; newSize<dataSize; newSize+=8) {
		wdys::encodeQuad(reinterpret_cast<U32 *>(volatileData()+newSize), reinterpret_cast<U32 *>(volatileData()+newSize+4)); // this function encrypts in-place, hence work on a copy
	}
	
	// and now put it into a new buffer and assign that one to us
	tWDYSBuf newBuf;
	newBuf.write("whatdoyousee",sizeof(char)*12);
	newBuf.putU32(dataSize);
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
	U32 dsize=getU32();
	
	// decrypt ourselves in-place
	for (U32 i=0; i<dsize; i+=8) {
		// 16 is the header size
		wdys::decodeQuad(reinterpret_cast<U32 *>(volatileData()+16+i), reinterpret_cast<U32 *>(volatileData()+16+i+4));
	}
	
	// put the decrypted data into a new buffer and assign it to us
	tWDYSBuf newBuf;
	newBuf.write(data()+16, dsize); // 16 is the header size
	copy(newBuf);
	rewind();
}
/* end wdys buff */



/* AES buff */
void tAESBuf::setKey(const Byte * key) {
	memcpy(this->key,key,16);
}
void tAESBuf::setM5Key() {
	U32 xorkey=0xCF092676;
	tMBuf key;
	key.putU32(0xFC2C6B86 ^ xorkey);
	key.putU32(0x952E7BDA ^ xorkey);
	key.putU32(0xF1713EE8 ^ xorkey);
	key.putU32(0xC7410A13 ^ xorkey);
	memcpy(this->key,key.data(),16);
}
void tAESBuf::encrypt() {
	// encrypt the buffer
	tAESBuf newBuf(size()+16); // 8 bytes for the header, 8 bytes for AES
	newBuf.putU32(0x0D874288);
	newBuf.putU32(size());
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
	if(getU32()!=0x0D874288) throw txUnexpectedData(_WHERE("NotAM5CryptedFile!"));
	U32 dataSize = getU32();
	tAESBuf newBuf(size()-8);

	// decrypt it
	Rijndael rin;
	rin.init(Rijndael::ECB,Rijndael::Decrypt,key,Rijndael::Key16Bytes);
	int res = rin.padDecrypt(data()+8,size()-8,newBuf.volatileData());
	if(res<0) throw txUnkErr(_WHERE("Rijndael decrypt error %i",res));
	if(static_cast<U32>(res) != size()-8) throw txUnkErr(_WHERE("Rijndael decrypt error: Size mismatch"));

	// and use what we need
	newBuf.cutEnd(dataSize);
	copy(newBuf);
	rewind();
}
/* end AES buff */


/* tUruString */
void tUruString::store(tBBuf &t) {
	clear();
	U32 bufSize = t.getU16();
	if (!(bufSize & 0xF000)) throw txUnexpectedData(_WHERE("This is not an inverted string!"));
	bufSize &= ~0xF000; // remove highest bit
	for(U32 i=0; i<bufSize; i++) {
		putByte(~t.getByte());
	}
	
	/* Myst V decryption
	Byte key[9]="mystnerd";
	for(U32 i=0; i<nsize; i++) {
		putByte(b.getByte() ^ key[i%8]);
	} */
}
void tUruString::stream(tBBuf &t) const {
	t.putU16(size()|0xF000);
	for(U32 i=0; i<size(); i++) {
		t.putByte(~*(data()+i));
	}
	
	/* Myst V encryption
	Byte key[9]="mystnerd";
	for(U32 i=0; i<msize; i++) {
		b.putByte(buf->buf[i] ^ key[i%8]);
	} */
}
/* end tUruString */

/* tUruObject (Plasma: plUoid) */
tUruObject::tUruObject(void) : tBaseType()
{
	hasCloneId = 0;
	pageId = 0;
	pageType = objType = 0;
	cloneId = clonePlayerId = 0;
}

void tUruObject::store(tBBuf &t)
{
	Byte cloneIdFlag = t.getByte();
	if (cloneIdFlag != 0x00 && cloneIdFlag != 0x01)
		throw txUnexpectedData(_WHERE("the clone ID flag of an UruObject must be 0x00 or 0x01, not 0x%02X", cloneIdFlag));
	else hasCloneId = cloneIdFlag;
	pageId = t.getU32();
	pageType = t.getU16();
	objType = t.getU16();
	t.get(objName);
	
	// if contained, read the client ID
	if (hasCloneId) {
		cloneId = t.getU32();
		clonePlayerId = t.getU32();
	}
}

void tUruObject::stream(tBBuf &t) const
{
	t.putByte(hasCloneId);
	t.putU32(pageId);
	t.putU16(pageType);
	t.putU16(objType);
	t.put(objName);
	if (hasCloneId) {
		t.putU32(cloneId);
		t.putU32(clonePlayerId);
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
tUruObjectRef::tUruObjectRef(void) : tBaseType()
{
	hasObj = false;
}

tUruObjectRef::tUruObjectRef(const tUruObject &obj) : obj(obj)
{
	hasObj = true;
}

void tUruObjectRef::store(tBBuf &t)
{
	Byte hasObjFlag = t.getByte();
	if (hasObjFlag != 0x00 && hasObjFlag != 0x01)
		throw txUnexpectedData(_WHERE("the hasObjFlag of an UruObjectRef must be 0x00 or 0x01, not 0x%02X", hasObjFlag));
	hasObj = hasObjFlag;
	if (hasObj) t.get(obj);
}

void tUruObjectRef::stream(tBBuf &t) const
{
	t.putByte(hasObj);
	if (hasObj) t.put(obj);
}

tString tUruObjectRef::str(void) const
{
	if (hasObj) return obj.str();
	else return "null";
}

/* tStreamedObject (Plasma: plNetMsgStreamHelper) */
tStreamedObject::tStreamedObject(tpObject *obj) : tMBuf(), maxSize(256) // make sure this is the same maxSize as in urubasetypes.h
{
	type = obj->getType();
	format = 0x00;
	realSize = 0;
	put(*obj);
	compress();
}

void tStreamedObject::store(tBBuf &t)
{
	DBG(8, "tStreamedObject::store\n");
	clear();
	
	realSize = t.getU32();
	format = t.getByte();
	if (format != 0x00 && format != 0x02 && format != 0x03)
		throw txUnexpectedData(_WHERE("Invalid stream format 0x%02X", format));
	if (format != 0x02 && realSize)
		throw txUnexpectedData(_WHERE("Uncompressed stream must not have real size set"));
	U32 sentSize = t.getU32();
	if (sentSize == 0) {
		type = plNull;
		return;
	}
	// strip out the two type bytes
	realSize -= 2;
	sentSize -= 2;
	
	type = t.getU16();
	write(t.read(sentSize), sentSize);
	
	uncompress();
}

void tStreamedObject::stream(tBBuf &t) const
{
	DBG(8, "tStreamedObject::stream\n");
	if (!size()) {
		// write an empty stream
		t.putU32(0); // real size
		t.putByte(0x00); // compression flag
		t.putU32(0); // sent size
		return;
	}
	else if (size() > maxSize && format == 0x00) // an uncompressed stream of that size?
		throw txBase(_WHERE("Someone forgot to call tStreamedObject::compress()"));
	
	// it's not yet empty, so we have to write something
	t.putU32(format == 0x02 ? realSize+2 : 0); // add the two type bytes
	t.putByte(format);
	t.putU32(size()+2); // add the two type bytes
	t.putU16(type);
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
	res.putU16(type);
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

} //end namespace alc
