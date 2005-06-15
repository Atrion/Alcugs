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
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(msize);
	U32 xsize=msize;
	U32 xstart=mstart;
	msize=0;
	mstart=0;
	off=0;
	
	if(memcmp(aux->buf+xstart,"whatdoyousee",12)) throw txBase(_WHERE("NotAWDYSFile!")); 
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

/* tUStr */
tUStr::tUStr() {
	name=NULL;
	msize=0;
}
tUStr::~tUStr() {
	if(name!=NULL) free((void *)name);
}
int tUStr::stream(tBBuf &buf,bool inv) {
	int spos = buf.tell();
	U16 ize = msize;
	if(inv) ize|=0xF000;
	buf.putU16(ize);
	if(!inv) {
		buf.write(name,msize);
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
		memcpy(name,buf.read(msize),msize);
	} else {
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
