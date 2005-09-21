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

/* CVS tag - DON'T TOUCH*/
#define __U_URUBASICTYPES_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include <cstdlib>
#include <cstring>

namespace wdys {
#include "urutypes/whatdoyousee.h"
}

#include "alcutil/rijndael.h"

//alctypes already included in alcugs.h

#include "alcdebug.h"

namespace alc {

using namespace std;

/* wdys buff */
void tWDYSBuf::encrypt() {
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(msize+12+4+8);
	this->buf->zero();
	U32 xsize=msize;
	U32 xstart=mstart;
	msize=0;
	mstart=0;
	off=0;
	
	write((Byte *)"whatdoyousee",(U32)sizeof(char)*12);
	putU32(xsize);
	write(aux->buf+xstart,xsize);
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
	if(memcmp(this->buf->buf+mstart,"whatdoyousee",12)) throw txUnexpectedData(_WHERE("NotAWDYSFile!")); 
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(msize);
	U32 xsize=msize;
	U32 xstart=mstart;
	msize=0;
	mstart=0;
	off=0;
	
	write(aux->buf+xstart+16,xsize-16);
	msize=*(U32 *)(aux->buf+xstart+12);
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
	U32 xstart=mstart;
	msize=0;
	mstart=0;
	off=0;
	
	putU32(0x0D874288);
	putU32(xsize);
	write(aux->buf+xstart,xsize);
	set(8);
	msize=8;

	Rijndael rin;
	rin.init(Rijndael::ECB,Rijndael::Encrypt,key,Rijndael::Key16Bytes);
	int res = rin.padEncrypt(aux->buf+xstart,xsize,this->buf->buf+off);
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
	U32 xstart=mstart;
	msize=0;
	mstart=0;
	off=0;
	
	write(aux->buf+xstart+8,xsize-8);
	msize=*(U32 *)(aux->buf+xstart+4);
	off=0;

	Rijndael rin;
	rin.init(Rijndael::ECB,Rijndael::Decrypt,key,Rijndael::Key16Bytes);

	int res = rin.padDecrypt(aux->buf+xstart+8,xsize-8,buf->buf+off);
	if(res<0) throw txUnkErr(_WHERE("Rijndael decrypt error %i",res));

	if(aux->getRefs()<1) delete aux;
	off=0;
}
/* end AES buff */


/* tUStr */
tUStr::tUStr(Byte mode) {
	this->version=mode;
	name=NULL;
	msize=0;
}
tUStr::~tUStr() {
	if(name!=NULL) free((void *)name);
}
int tUStr::stream(tBBuf &buf,bool inv) {
	int spos = buf.tell();
	U16 ize = msize;
	if(version!=0x01) inv=false;
	if(version==0x05) inv=true;
	if(inv) ize|=0xF000;
	buf.putU16(ize);
	if(!inv) {
		if(version!=0x06) {
			buf.write(name,msize);
		} else { //this is required to parse Myst 5 strings
			Byte key[9]="mystnerd";
			for(int i=0; i<msize; i++) {
				buf.putByte(name[i] ^ key[i%8]);
			}
		}
	} else {
		for(int i=0; i<msize; i++) {
			buf.putByte(~name[i]);
		}
	}
	return (buf.tell() - spos);
}
int tUStr::stream(tBBuf &buf) {
	return(this->stream(buf,0));
}
void tUStr::store(tBBuf &buf) {
	U16 ize = buf.getU16();
	U16 how= (ize>>8) & 0xF0;
	msize = ize & 0x0FFF;
	
	if(name!=NULL) free((void *)name);
	
	if(msize>0xF000) throw txBase(_WHERE("TooBig"));
	name=(Byte *)malloc(sizeof(Byte) * (msize+1));
	if(name==NULL) throw txNoMem(_WHERE("NoMem"));

	if(how==0x00) {
		if(this->version==0x06) {
			Byte key[9]="mystnerd";
			for(int i=0; i<msize; i++) {
				name[i]=buf.getByte() ^ key[i%8];
			}
		} else {
			memcpy(name,buf.read(msize),msize);
		}
	} else {
		if(this->version==0x06) {
			throw txUnexpectedData(_WHERE("how should be 0x00 for version 0x06"));
		}
		for(int i=0; i<msize; i++) {
			name[i]=~buf.getByte();
		}
	}
	name[msize]=0;
}
U32 tUStr::size() { return msize+2; }
U32 tUStr::len() { return msize; }
void tUStr::set(Byte * val,U32 _n) {
	if(name!=NULL) free((void *)name);
	if(_n==0) {
		msize=strlen((const char *)val);
	} else {
		msize=_n;
	}
	if(msize>0xF000) throw txBase(_WHERE("TooBig"));
	name=(Byte *)malloc(sizeof(Byte) * (msize+1));
	memset(name,0,sizeof(Byte) * (msize+1));
	if(name==NULL) throw txNoMem(_WHERE("NoMem"));
	strncpy((char *)name,(char *)val,msize);
}
void tUStr::set(char * val,U32 _n) {
	this->set((Byte *)val,_n);
}
Byte * tUStr::str() {
	return name;
}
/* end tUStr */

} //end namespace alc
