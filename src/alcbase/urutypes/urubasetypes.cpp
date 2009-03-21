/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs H'uru Server Team                     *
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

/* CVS tag - DON'T TOUCH*/
#define __U_URUBASICTYPES_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

namespace wdys {
#include "urutypes/whatdoyousee.h"
}

#include "alcutil/rijndael.h"

//alctypes already included in alcugs.h

#include "alcdebug.h"

namespace alc {

/* wdys buff */
void tWDYSBuf::encrypt() {
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(msize+12+4+8);
	this->buf->zero();
	U32 xsize=msize;
	msize=0;
	off=0;
	
	write("whatdoyousee",(U32)sizeof(char)*12);
	putU32(xsize);
	write(aux->buf,xsize);
	set(16);
	msize=16;
	
	for (U32 i=0; i<xsize; i+=8) {
		wdys::encodeQuad((U32 *)(this->buf->buf+off+i),(U32 *)(this->buf->buf+off+i+4));
		msize+=8;
	}
	
	if(aux->getRefs()<1) delete aux;
	off=0;
}
void tWDYSBuf::decrypt() {
	if(memcmp(this->buf->buf,"whatdoyousee",12)) throw txUnexpectedData(_WHERE("NotAWDYSFile!")); 
	set(12);
	U32 dsize=getU32();
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(msize);
	U32 xsize=msize;
	msize=0;
	off=0;
	
	write(aux->buf+16,xsize-16);
	msize=dsize;
	off=0;
	
	for (U32 i=0; i<msize; i+=8) {
		wdys::decodeQuad((U32 *)(this->buf->buf+off+i),(U32 *)(this->buf->buf+off+i+4));
	}
	
	if(aux->getRefs()<1) delete aux;
	off=0;
}
/* end wdys buff */



/* AES buff */
void tAESBuf::setKey(Byte * key) {
	memcpy(this->key,key,16);
}
void tAESBuf::setM5Key() {
	Byte key[16];
	U32 xorkey=0xCF092676;
	*(U32 *)(key)    = 0xFC2C6B86 ^ xorkey;
	*(U32 *)(key+4)  = 0x952E7BDA ^ xorkey;
	*(U32 *)(key+8)  = 0xF1713EE8 ^ xorkey;
	*(U32 *)(key+12) = 0xC7410A13 ^ xorkey;
	memcpy(this->key,key,16);
}
void tAESBuf::encrypt() {
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(msize+4+4+16);
	this->buf->zero();
	U32 xsize=msize;
	msize=0;
	off=0;
	
	putU32(0x0D874288);
	putU32(xsize);
	write(aux->buf,xsize);
	set(8);
	msize=8;

	Rijndael rin;
	rin.init(Rijndael::ECB,Rijndael::Encrypt,key,Rijndael::Key16Bytes);
	int res = rin.padEncrypt(aux->buf,xsize,this->buf->buf+off);
	if(res<0) throw txUnkErr(_WHERE("Rijndael encrypt error"));
	msize+=res;
	
	if(aux->getRefs()<1) delete aux;
	off=0;
}
void tAESBuf::decrypt() {
	this->rewind();
	if(this->getU32()!=0x0D874288) throw txUnexpectedData(_WHERE("NotAM5CryptedFile!")); 
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(msize);
	U32 xsize=msize;
	msize=0;
	off=0;
	
	write(aux->buf+8,xsize-8);
	msize=*(U32 *)(aux->buf+4);
	off=0;

	Rijndael rin;
	rin.init(Rijndael::ECB,Rijndael::Decrypt,key,Rijndael::Key16Bytes);

	int res = rin.padDecrypt(aux->buf+8,xsize-8,buf->buf+off);
	if(res<0) throw txUnkErr(_WHERE("Rijndael decrypt error %i",res));

	if(aux->getRefs()<1) delete aux;
	off=0;
}
/* end AES buff */


/* tUStr */
tUStr::tUStr(int mode) : tStrBuf() {
	this->version=mode;
}
tUStr::tUStr(const char *k, int mode) : tStrBuf(k) {
	this->version=mode;
}
tUStr::tUStr(const tUStr &t) : tStrBuf(t) {
	this->version = t.version;
}
tUStr::tUStr(const tStrBuf &t, int mode) : tStrBuf(t) {
	this->version = mode;
}
void tUStr::stream(tBBuf &b) {
	if (version == 0x04)
		throw txBase(_WHERE("Can't send version 0x04 (normal+hex)")); 

	//int spos = b.tell();
	U16 ize = msize;
	bool inv=false;
	if(version==0x05) inv=true;
	if(inv) ize|=0xF000;
	b.putU16(ize);
	
	if(!inv) {
		if(version!=0x06) {
			b.write(buf->buf, msize);
		} else { //this is required to parse Myst 5 strings
			Byte key[9]="mystnerd";
			for(U32 i=0; i<msize; i++) {
				b.putByte(buf->buf[i] ^ key[i%8]);
			}
		}
	} else {
		for(U32 i=0; i<msize; i++) {
			b.putByte(~buf->buf[i]);
		}
	}
	//return (b.tell() - spos);
}
void tUStr::store(tBBuf &b) {
	U16 ize = b.getU16();
	U16 how= (ize>>8) & 0xF0;
	U32 nsize = ize & 0x0FFF;
	
	if(nsize>0xF000) throw txBase(_WHERE("TooBig"));
	
	clear();
	if (nsize > 0) { // only read if there's something to read
		if(how==0x00) {
			if(this->version==0x06) {
				Byte key[9]="mystnerd";
				for(U32 i=0; i<nsize; i++) {
					putByte(b.getByte() ^ key[i%8]);
				}
			} else { // make sure nsize is > 0 here, otherwise read(0) will read the rtest of the buffer
				if (version != 0x00 && version != 0x01) { // the given version is neither normal nor auto, so it's WRONG
					throw txUnexpectedData(_WHERE("Version is 0x00, but expected 0x%02X", version));
				}
				if (version == 0x01) version = 0x00; // when we are auto-detecting, save the realy version (normal)
				write(b.read(nsize), nsize);
			}
		} else {
			if(this->version==0x06) {
				throw txUnexpectedData(_WHERE("how should be 0x00 for version 0x06"));
			}
			else if (version != 0x05 && version != 0x01) { // the given version is neither inverted nor auto, so it's WRONG
				throw txUnexpectedData(_WHERE("Version is 0x05, but expected 0x%02X", version));
			}
			if (version == 0x01) version = 0x05; // when we are auto-detecting, save the realy version (inverted)
			for(U32 i=0; i<nsize; i++) {
				putByte(~b.getByte());
			}
		}
	}
}
void tUStr::copy(const tUStr &t)
{
	DBG(9,"tUStr::copy()\n");
	if(this==&t) return;
	this->_pcopy(t);
}
void tUStr::_pcopy(const tUStr &t)
{
	DBG(9, " tUStr::_pcopy()\n");
	if (&t == this) return;
	tStrBuf::_pcopy(t);
	version = t.version;
}
/* end tUStr */

/* tUruObject */
tUruObject::tUruObject(void) : tBaseType()
{
	hasCloneId = 0;
	pageId = 0;
	pageType = objType = 0;
	objName.setVersion(5); // inverted UrurString
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

void tUruObject::stream(tBBuf &t)
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

const char *tUruObject::str(void)
{
	dbg.clear();
	dbg.printf("Page ID: 0x%08X, Page Type: 0x%04X, Object: [0x%04X]%s", pageId, pageType, objType, objName.c_str());
	if (hasCloneId)
		dbg.printf(", Clone ID: %d, Clone Player ID: %d", cloneId, clonePlayerId);
	return dbg.c_str();
}

bool tUruObject::operator==(const tUruObject &obj)
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

/* tUruObjectRef */
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

void tUruObjectRef::stream(tBBuf &t)
{
	t.putByte(hasObj);
	if (hasObj) t.put(obj);
}

const char *tUruObjectRef::str(void)
{
	if (hasObj) return obj.str();
	else return "null";
}

/* tStreamedObject */
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

void tStreamedObject::stream(tBBuf &t)
{
	DBG(8, "tStreamedObject::stream\n");
	if (!size()) {
		// write an empty stream
		t.putU32(0); // real size
		t.putByte(0x00); // compression flag
		t.putU32(0); // sent size
		return;
	}
	compress();
	
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

void tStreamedObject::compress(U32 maxSize)
{
	if (format == 0x02 || size() < maxSize) return; // nothing to do
	DBG(8, "tStreamedObject::compress\n");
	// compress it
	realSize = size();
	tZBuf content(*this);
	content.compress();
	tMBuf::copy(content);
	format = 0x02; // save that we compressed it so that we don't do it again
}

void tStreamedObject::copy(const tStreamedObject &t)
{
	if(this==&t) return;
	this->_pcopy(t);
}
void tStreamedObject::_pcopy(const tStreamedObject &t)
{
	if (&t == this) return;
	tMBuf::_pcopy(t);
	format = t.format;
	type = t.type;
}

} //end namespace alc
