/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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
#define __U_ALCTYPES_ID "$Id$"

//#define _DBG_LEVEL_ 6

#include "alcugs.h"

#include <cerrno>
#include <cstdarg>


namespace zlib {
#include <zlib.h>
}

namespace md5 {
#include "alcutil/md5.h"
}

//alctypes already included in alcugs.h

#include "alcdebug.h"

namespace alc {

/* 
	basicbuffer
*/
tBBuf::tBBuf() {
	DBG(9,"tBBuf::tBBuf()\n");
}
tBBuf::~tBBuf() {
	DBG(9,"~tBBuf()\n");
}
//Uru is Little-Endian, so if we are on a Big-Endian machine, 
// all writes and reads to any network/file buffer must be
// correctly swapped.

//For these put* functions DO NOT change val to a reference (though I don't
// know why anyone would). If you do, the byte-swapping will hurt you!
void tBBuf::putU16(U16 val) {
	val = htole16(val);
	this->write((Byte *)&val,2);
}
void tBBuf::putS16(S16 val) {
	val = htole16(val);
	this->write((Byte *)&val,2);
}
void tBBuf::putU32(U32 val) {
	val = htole32(val);
	this->write((Byte *)&val,4);
}
void tBBuf::putS32(S32 val) {
	val = htole32(val);
	this->write((Byte *)&val,4);
}
void tBBuf::putByte(Byte val) {
	this->write(&val,1);
}
void tBBuf::putSByte(SByte val) {
	this->write(&val,1);
}
void tBBuf::putDouble(double val) {
#if defined(WORDS_BIGENDIAN)
	//ok for integers, but I don't trust it for doubles
	//val = htole32(val >> 32) | ((htole32(val & 0xffffffff)) << 32);
	//is there a less yucky way?
	U32 *valAsArray = (U32 *)&val;
	U32 lo = htole32(valAsArray[1]);
	U32 hi = htole32(valAsArray[0]);
	valAsArray[0] = lo;
	valAsArray[1] = hi;
#endif
	this->write((Byte *)&val,8);
}
void tBBuf::putFloat(float val) { // TODO: Does this work on big-endian?
	this->write((Byte *)&val,4);
}
U16 tBBuf::getU16() {
	U16 val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy((void *)&val, this->read(2), 2);
#else
	val = *(U16 *)(this->read(2));
#endif
	return(letoh16(val));
}
S16 tBBuf::getS16() {
	S16 val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy((void *)&val, this->read(2), 2);
#else
	val = *(S16 *)(this->read(2));
#endif
	return(letoh16(val));
}
U32 tBBuf::getU32() {
	U32 val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy((void *)&val, this->read(4), 4);
#else
	val = *(U32 *)(this->read(4));
#endif
	return(letoh32(val));
}
S32 tBBuf::getS32() {
	S32 val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy((void *)&val, this->read(4), 4);
#else
	val = *(S32 *)(this->read(4));
#endif
	return(letoh32(val));
}
Byte tBBuf::getByte() {
	return(*(this->read(1)));
}
SByte tBBuf::getSByte() {
	return(*(this->read(1)));
}
double tBBuf::getDouble() {
	double val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy((void *)&val, this->read(8), 8);
#else
	val = *(double *)(this->read(8));
#endif
#if defined(WORDS_BIGENDIAN)
	//ok for integers, but I don't trust it for doubles
	//val = letoh32(val >> 32) | ((letoh32(val & 0xffffffff)) << 32);
	//is there a less yucky way?
	U32 *valAsArray = (U32 *)&val;
	U32 lo = letoh32(valAsArray[1]);
	U32 hi = letoh32(valAsArray[0]);
	valAsArray[0] = lo;
	valAsArray[1] = hi;
#endif
	return(val);
}
float tBBuf::getFloat() { // TODO: Does this work on big-endian?
	float val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy((void *)&val, this->read(4), 4);
#else
	val = *(S32 *)(this->read(4));
#endif
	return(val);
}
void tBBuf::seek(int n,Byte flags) {
	int res;
	if(flags==SEEK_CUR) {
		res=(this->tell()+n);
	} else if(flags==SEEK_END) {
		res=(this->size()-n);
	} else if(flags==SEEK_SET) {
		res=n;
	} else {
		throw txUnkFlags(_WHERE("Unexpected Flags %02X",flags));
	}
	if(res<0) throw txOutOfRange(_WHERE("range is %i<0",res));
	this->set(res);
}
void tBBuf::check(const Byte * what,U32 n) {
	if(n && (size()-tell()<n || memcmp(what,read(n),n) != 0)) {
		throw txUnexpectedData(_WHERE("UnexpectedData"));
	}
}
/* end Basic buffer */

/* 
	tRefBuf 
*/
tRefBuf::tRefBuf(U32 csize) {
	refs=1;
	msize=csize;
	buf=(Byte *)malloc(sizeof(Byte) * csize);
	if(buf==NULL) 
	throw txNoMem(_WHERE("NoMem"));
}
tRefBuf::~tRefBuf() {
	free((void *)buf);
}
void tRefBuf::resize(U32 newsize) {
	msize=newsize;
	Byte * b2 =(Byte *)realloc(buf,sizeof(Byte *) * newsize);
	if(b2==NULL) throw txNoMem(_WHERE("NoMem"));
	buf=b2;
}
void tRefBuf::inc() { refs++; }
void tRefBuf::dec() {
	refs--;
	if(refs<0) throw txRefErr(_WHERE("RefErr %i",refs));
}
U32 tRefBuf::size() { return msize; }
/* end tRefBuf */

/*
	Memory Buffer
*/
tMBuf::tMBuf() {
	DBG(9,"tMBuf()\n");
	this->init();
}
tMBuf::tMBuf(tBBuf &t) {
	DBG(9,"tMBuf(tBBuf)\n");
	this->init();
	U32 pos=t.tell();
	t.rewind();
	this->write(t.read(), t.size());
	t.set(pos); // restore old pos
	set(pos);
}
tMBuf::tMBuf(U32 size) {
	DBG(9,"tMBuf(size:%i)\n",size);
	buf = new tRefBuf(size);
	msize=off=0;
}
tMBuf::tMBuf(const tMBuf &t) {
	DBG(9,"tMBuf(tMBuf)\n");
	buf = t.buf;
	if (buf != NULL) buf->inc();
	msize = t.msize;
	off=t.off;
}
tMBuf::~tMBuf() {
	DBG(9,"~tMBuf()\n");
	if(buf!=NULL) {
		buf->dec();
		if(buf->getRefs()<=0) {
			delete buf;
		}
	}
}
void tMBuf::onmodify() {
}
void tMBuf::copy(const tMBuf &t) {
	DBG(9,"tMBuf::copy()\n");
	if(buf!=NULL) {
		buf->dec();
		if(buf->getRefs()<=0) {
			delete buf;
		}
	}
	buf=t.buf;
	if(buf!=NULL) buf->inc();
	off=t.off;
	msize=t.msize;
}
void tMBuf::init() {
	DBG(9,"tMBuf::init()\n");
	buf=NULL;
	msize=off=0;
}
U32 tMBuf::tell() const { return off; }
void tMBuf::set(U32 pos) { 
	if(pos>msize) throw txOutOfRange(_WHERE("OutOfRange %i>%i",pos,msize));
	off=pos;
}
Byte tMBuf::getAt(U32 pos) const {
	if(pos>msize) {
		throw txOutOfRange(_WHERE("OutOfRange %i>%i",pos,msize));
	}
	return *(buf->buf+pos);
}
void tMBuf::setAt(U32 pos,const Byte what) {
	if(pos>msize) throw txOutOfRange(_WHERE("OutOfRange %i>%i",pos,msize));
	*(buf->buf+pos)=what;
	onmodify();
}
void tMBuf::write(const Byte * val,U32 n) {
	if(val==NULL) return;
	if(buf==NULL) buf = new tRefBuf(1024 + n);
	else if(buf->getRefs()>1) {
		buf->dec();
		Byte * oldbuf=buf->buf;
		U32 xsize=buf->size();
		buf = new tRefBuf((xsize)+1024+n);
		memcpy(buf->buf,oldbuf,xsize);
	}
	U32 oldoff=off;
	off+=n;
	U32 bsize=buf->size();
	if(off>bsize) {
		U32 newsize = ((off-bsize)>1024 ? off+1024 : bsize+1024);
		buf->resize(newsize);
	}
	memcpy(buf->buf+oldoff,val,n);
	if(off>msize) msize=off;
	onmodify();
}
const Byte * tMBuf::read(U32 n) {
	U32 pos=off;
	if(n==0) n=msize-off;
	if(n==0) return NULL;
	off+=n;
	if(off>msize || buf==NULL || buf->buf==NULL) { 
		DBG(8,"off:%u,msize:%u\n",off,msize); off-=n; 
		throw txOutOfRange(_WHERE("OutOfRange %i>%i",off+n,msize)); 
		//return NULL;
	}
	return buf->buf+pos;
}
void tMBuf::stream(tBBuf &b) const {
	if(buf==NULL || buf->buf==NULL) return;
	b.write(buf->buf,msize);
}
U32 tMBuf::size() const { return msize; }
void tMBuf::clear() {
	off=0;
	msize=0;
	onmodify();
}
char tMBuf::compare(const tMBuf &t) const {
	DBG(9,"tBBuf::compare()\n");
	U32 s1 = size(), s2 = t.size();
	if (!s1 || !s2) {
		// needs special treatment as buf might be NULL
		if (!s1 && !s2) return 0; // both empty
		else if (s1) return 1; // we empty
		else return -1; // the other one empty
	}
	SByte out = memcmp(buf->buf, t.buf->buf, std::min(s1, s2));
	if (out != 0 || s1 == s2) return out;
	return (s1 < s2) ? -1 : 1;
}
/* end tMBuf */

/* 
	File Buffer 
*/
tFBuf::tFBuf() {
	this->init();
}
tFBuf::~tFBuf() {
	this->close();
	free((void *)xbuf);
}
void tFBuf::init() {
	f=NULL;
	xsize=msize=0;
	xbuf=NULL;
}
U32 tFBuf::tell() const {
	DBG(9,"ftell()\n");
	if(f!=NULL) return ftell(f);
	return 0;
}
void tFBuf::set(U32 pos) {
	if(f==NULL || fseek(f,pos,SEEK_SET)<0) {
		throw txOutOfRange(_WHERE("OutOfRange"));
	}
}
void tFBuf::write(const Byte * val,U32 n) {
	DBG(9,"write(val:%s,n:%u)\n",val,n);
	if(val==NULL) return;
	if(f==NULL) throw txNoFile(_WHERE("NoFile"));
	U32 nn=fwrite(val,n,1,f);
	if(nn!=1) throw txWriteErr(_WHERE("write error, only wrote %u of 1",nn));
}
const Byte * tFBuf::read(U32 n) {
	if(n==0) n=this->size();
	if(f==NULL) throw txNoFile(_WHERE("NoFile"));
	if(xbuf==NULL) {
		xbuf=(Byte *)malloc(n);
		if(xbuf==NULL) throw txNoMem(_WHERE("NoMem"));
		xsize=n;
	}
	if(xsize<n) {
		xbuf=(Byte *)realloc((void *)xbuf,n);
		if(xbuf==NULL) throw txNoMem(_WHERE("NoMem"));
		xsize=n;
	}
	fread(xbuf,n,1,f);
	return xbuf;
}
void tFBuf::stream(tBBuf &b) const {
	// make sure the position in the file is the same after the read
	U32 pos = tell(), n = size();
	fseek(f,0,SEEK_SET);
	if(xbuf==NULL) {
		xbuf=(Byte *)malloc(n);
		if(xbuf==NULL) throw txNoMem(_WHERE("NoMem"));
		xsize=n;
	}
	if(xsize<n) {
		xbuf=(Byte *)realloc((void *)xbuf,n);
		if(xbuf==NULL) throw txNoMem(_WHERE("NoMem"));
		xsize=n;
	}
	fread(xbuf,n,1,f);
	b.write(xbuf,n);
	fseek(f,pos,SEEK_SET);
}
U32 tFBuf::size() const {
	if(msize==0) {
		if (f==NULL) return 0;
		int wtf=ftell(f);
		fseek(f,0,SEEK_END);
		msize=ftell(f);
		fseek(f,wtf,SEEK_SET);
	}
	DBG(9,"msize:%i\n",msize);
	return msize;
}
void tFBuf::open(const char * path,const char * mode) {
	f=fopen(path,mode);
	if(f==NULL) {
		if(errno==EACCES || errno==EISDIR) throw txNoAccess(path);
		if(errno==ENOENT) throw txNotFound(path);
		throw txUnkErr("Unknown error code: %i\n",errno);
	}
}
void tFBuf::close() {
	if(f!=NULL) { fclose(f); }
	f=NULL;
	msize=0;
}
void tFBuf::flush() {
	fflush(f);
}
/* end File buffer */

/* static buffer */
tSBuf::tSBuf(const Byte * buf,U32 msize) {
	off=0;
	this->msize=msize;
	this->buf=buf;
}
U32 tSBuf::tell() const { return off; }
void tSBuf::set(U32 pos) {
	if(pos>msize) throw txOutOfRange(_WHERE("Cannot access pos %i, size %i\n",pos,msize));
	off=pos; 
}
const Byte * tSBuf::read(U32 n) {
	const Byte * auxbuf=buf+off;
	if(n) {
		off+=n;
		if(off>msize) { off-=n; throw txOutOfRange(_WHERE("Cannot read pos %i,len:%i size %i\n",off,n,msize)); }
	}
	else off=msize;
	return auxbuf;
}
void tSBuf::stream(tBBuf &buf) const {
	buf.write(this->buf,msize);
}
U32 tSBuf::size() const { return msize; }

/* end static buffer */

/* tZBuf */
void tZBuf::compress() {
	tRefBuf * aux=this->buf;
	aux->dec();
	zlib::uLongf comp_size;
	comp_size=msize+(msize/10)+12;
	this->buf = new tRefBuf(comp_size);
	int ret=zlib::compress(this->buf->buf,&comp_size,aux->buf,msize);
	if(ret!=0) throw txBase(_WHERE("Something terrible happenened compressing the buffer"));
	msize=comp_size;
	if(aux->getRefs()<1) delete aux;
	this->buf->resize(msize);
	off=0;
}
void tZBuf::uncompress(int iosize) {
	tRefBuf * aux=this->buf;
	aux->dec();
	zlib::uLongf comp_size;
	/*if(iosize<=0) { comp_size=10*msize; } //yes, is very dirty
	else */comp_size=iosize;
	this->buf = new tRefBuf(comp_size);
	int ret=zlib::uncompress(this->buf->buf,&comp_size,aux->buf,msize);
	if(ret!=0) throw txBase(_WHERE("Something terrible happenened uncompressing the buffer"));
	msize=comp_size;
	if(aux->getRefs()<1) delete aux;
	this->buf->resize(msize);
	off=0;
	if ((zlib::uLong)iosize != comp_size)
		throw txUnexpectedData(_WHERE("tZBuf size mismatch: %i != %i", iosize, comp_size));
}
/* end tZBuf */

/* md5 buff */
void tMD5Buf::compute() {
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(0x10);
	md5::MD5((const unsigned char *)aux->buf,msize,(unsigned char *)this->buf->buf);
	msize=0x10;
	if(aux->getRefs()<1) delete aux;
	off=0;
}
/* end md5 buf */

/* String buffer */
tStrBuf::tStrBuf(const char * k) :tMBuf(200) {
	DBG(9,"ctor\n");
	init();
	writeStr(k);
	end();
}
tStrBuf::tStrBuf(U32 size) :tMBuf(size) { DBG(9,"ctor 2\n"); init(); }
tStrBuf::tStrBuf(tBBuf &k) :tMBuf(k) {
	DBG(9,"copy zero\n");
	init();
	if(size()) null = false;
}
tStrBuf::tStrBuf(const tMBuf &k) :tMBuf(k) { 
	DBG(9,"copy one\n");
	init(); 
	if(size()) null = false;
}
tStrBuf::tStrBuf(const tStrBuf &k) :tMBuf(k) { 
	DBG(9,"copy two\n");
	if(this==&k) return;
	init(); 
	l=k.l; 
	c=k.c;
	sep=k.sep;
	null=k.null;
}
tStrBuf::~tStrBuf() {
	delete shot;
	delete cache_lower;
}
void tStrBuf::init() {
	DBG(9,"tStrBuf::init\n");
	l=c=0;
	sep='=';
	shot=NULL;
	cache_lower=NULL;
	null = true;
}
void tStrBuf::onmodify() {
	DBG(7,"tStrBuf::onmodify()\n");
	tMBuf::onmodify();
	delete cache_lower;
	cache_lower=NULL;
	null = false;
}
void tStrBuf::copy(const tStrBuf &t) {
	DBG(9,"tStrBuf::copy()\n");
	if(this==&t) return;
	tMBuf::copy(t);
	l=t.l;
	c=t.c;
	sep=t.sep;
	null=t.null;
	delete cache_lower;
	cache_lower=NULL;
	delete shot;
	shot=NULL;
}
void tStrBuf::copy(const char * str) {
	DBG(2,"cpy\n");
	tStrBuf pat(str);
	copy(pat);
}
void tStrBuf::store(tBBuf &t)
{
	clear();
	U32 bufSize = t.getU16();
	if (bufSize & 0xF000) throw txUnexpectedData(_WHERE("This is an inverted string!"));
	if (bufSize)
		write(t.read(bufSize), bufSize);
}
void tStrBuf::stream(tBBuf &t) const
{
	t.putU16(msize);
	tMBuf::stream(t); // just puts the bytes into the buffer
}
SByte tStrBuf::compare(const char * str) const {
	U32 s1 = size(), s2 = strlen(str);
	if (!s1 || !s2) {
		// needs special treatment as buf might be NULL
		if (!s1 && !s2) return 0; // both empty
		else if (s1) return 1; // we empty
		else return -1; // the other one empty
	}
	SByte out = memcmp(buf->buf, str, std::min(s1, s2));
	if (out != 0 || s1 == s2) return out;
	return (s1 < s2) ? -1 : 1;
}
const char * tStrBuf::c_str() {
	DBG(2,"tStrBuf::c_str()\n");
	if(isNull() || msize == 0) {
		DBG(2,"is null: %d\n", msize);
		return "";
	}
	// add zero byte
	U32 bsize = buf->size();
	if(size()+1>bsize) {
		buf->resize(bsize+128);
	}
	buf->buf[size()] = 0;
	return (const char *)buf->buf;
}
const char * tStrBuf::c_str() const {
	DBG(2,"tStrBuf::c_str()\n");
	if(isNull() || msize == 0) {
		DBG(2,"is null: %d\n", msize);
		return "";
	}
	if (shot) delete shot;
	shot = new tStrBuf(*this);
	return shot->c_str();
}
void tStrBuf::onrewind() {
	tMBuf::onrewind();
	c=l=0;
}
U16 tStrBuf::getLineNum() {
	return l+1;
}
U16 tStrBuf::getColumnNum() {
	return c;
}
void tStrBuf::decreaseLineNum() {
	l--;
}
S32 tStrBuf::find(const char cat, bool reverse) const {
	int i,max;
	max=size();
	if(reverse) {
		for(i=max-1; i>=0; i--) {
			if(getAt(i)==cat) return i;
		}
	} else {
		for(i=0; i<max; i++) {
			if(getAt(i)==cat) return i;
		}
	}
	return -1;
}
S32 tStrBuf::find(const char *str) const {
	char *c = strstr(c_str(), str);
	if (c == NULL) return -1;
	return (c-(char *)buf->buf);
}
void tStrBuf::convertSlashesFromWinToUnix() {
#if defined(__WIN32__) or defined(__CYGWIN__)
	int i,max;
	max=size();
	for(i=0; i<max; i++) {
		if(getAt(i)=='\\') {
			setAt(i,'/');
		}
	}
#endif
//noop on linux
}
void tStrBuf::convertSlashesFromUnixToWin() {
#if defined(__WIN32__) or defined(__CYGWIN__)
	int i,max;
	max=size();
	for(i=0; i<max; i++) {
		if(getAt(i)=='/') {
			setAt(i,'\\');
		}
	}
#endif
//noop on linux
}
const tStrBuf & tStrBuf::strip(Byte what,Byte how) {
	tStrBuf aux;
	int i,max,start,end;
	start=0;
	i=0;
	max=size();
	end=max-1;
	Byte ctrl=what;
	if(how & 0x01) {
		while(i<max && ctrl==what) {
			ctrl=getAt(i++);
		}
		start=i-1;
		if(start<0) start=0;
	}
	if(how & 0x02) {
		ctrl=what;
		i=max-1;
		while(i>=0 && ctrl==what) {
			ctrl=getAt(i--);
		}
		end=i+1;
		if(end>=max) end=max-1;
	}
	for(i=start; i<=end; i++) {
		aux.putSByte(getAt(i));
	}
	copy(aux);
	return *this;
}
const tStrBuf & tStrBuf::escape() const {
	delete shot;
	shot = new tStrBuf(size()*3/2); // nothing inside this function will use shot, so we can initialize it now

	int i,max;
	Byte ctrl; 
	max=size();
	for(i=0; i<max; i++) {
		ctrl=getAt(i);
		if(ctrl=='\n') {
			shot->putByte('\\');
			shot->putByte('n');
		} else if(ctrl=='\r') {
			shot->putByte('\\');
			shot->putByte('r');
		} else if(ctrl=='"') {
			shot->putByte('\\');
			shot->putByte('"');
		} else if(ctrl=='\\') {
			shot->putByte('\\');
			shot->putByte('\\');
		} else {
			shot->putByte(ctrl);
		}
	}
	
	return *shot;
}

const tStrBuf & tStrBuf::lower() const {
	if(cache_lower!=NULL) {
		DBG(7,"cached...\n");
		return *cache_lower;
	}
	DBG(7,"non-cached %s...\n",c_str());
	
	int i,max;
	max=size();

	cache_lower = new tStrBuf(max);

	for(i=0; i<max; i++) {
		cache_lower->putByte(std::tolower(getAt(i)));
		DBG(7,"%i:%c\n",i,getAt(i));
	}
	return *cache_lower;
}

const tStrBuf & tStrBuf::upper() const {
	if (shot) delete shot;
	shot = new tStrBuf(200); // nothing inside this function will use shot, so we can initialize it now
	
	int i,max;
	max=size();
	for(i=0; i<max; i++) {
		shot->putByte(std::toupper(getAt(i)));
	}
	
	return *shot;
}


const tStrBuf & tStrBuf::substring(U32 start,U32 len) const {
	if (len == 0) len = remaining();
	if (shot) delete shot;
	shot = new tStrBuf(len); // nothing inside this function will use shot, so we can initialize it now
	shot->write(buf->buf+start,len);
	return *shot;
}
bool tStrBuf::startsWith(const char * pat) const {
	try {
		return(substring(0,strlen(pat))==pat);
	}
	catch (txOutOfRange &e) {
		return false;
	}
}
bool tStrBuf::endsWith(const char * pat) const {
	try {
		return(substring(size()-strlen(pat),strlen(pat))==pat);
	}
	catch (txOutOfRange &e) {
		return false;
	}
}
const tStrBuf & tStrBuf::dirname() const {
	if (shot) delete shot;
	shot = new tStrBuf(*this); // nothing inside this function will use shot, so we can initialize it now
	shot->strip('/',0x02);
	int pos=shot->find('/',1);
	
	if(pos==-1) {
		*shot = ".";
	} else if(pos==0) {
		*shot = "/";
	} else {
		*shot = shot->substring(0,pos);
	}
	return *shot;
}
const tStrBuf & tStrBuf::getLine(bool nl,bool slash) {
	Byte c=0;
	Byte slashm=0;
	tStrBuf out(255);

	while(!eof()) {
		c=getByte();
		if(!slash) {
			if(c=='\\') {
				if(slashm) {
					slashm=0;
					out.putByte('\\');
					out.putByte('\\');
				} else {
					slashm=1;
				}
			} else if(c=='\n') {
				if(!eof() && getByte()!='\r') seek(-1);
				this->l++;
				this->c=0;
				if(!slashm) {
					break;
				}
				slashm=0;
				c=' ';
			} else if(c=='\r') {
				if(!eof() && getByte()!='\n') seek(-1);
				this->l++;
				this->c=0;
				if(!slashm) {
					break;
				}
				slashm=0;
				c=' ';
			} else {
				if(slashm) {
					slashm=0;
					out.putByte('\\');
				}
				out.putByte(c);
			}
		} else {
			if(c=='\n') {
				if(!eof() && getByte()!='\r') seek(-1);
				this->l++;
				this->c=0;
				break;
			} else if(c=='\r') {
				if(!eof() && getByte()!='\n') seek(-1);
				this->l++;
				this->c=0;
				break;
			} else {
				out.putByte(c);
			}
		}
	}
	if(nl) {
		if(c=='\n' || c=='\r') {
			out.putByte('\n');
		}
	}
	
	// use shot only now as functions above might use it, too
	delete shot;
	shot=new tStrBuf(out);
	return *shot;
}
const tStrBuf & tStrBuf::getToken() {
	DBG(9,"tStrBuf::getToken()\n");
	Byte c;
	Byte slash=0;
	Byte quote=0;
	Byte mode=0;
	tStrBuf out(200);
	//out.hasQuotes(true);
	//assert(out.hasQuotes());
	while(!eof()) {
		c=getByte();
		this->c++;
		if(quote==0 && (c=='#' || c==';')) {
			if (out.size()) { // we already have something in out, dont attach the newline to it but make it the next token
				this->c--;
				seek(-1);
			} else {
				getLine();
				out.putByte('\n');
			}
			break;
		} else if(slash==1) {
			slash=0;
			if(quote==1 && (c=='n' || c=='r')) {
				if(c=='n') out.putByte('\n');
				else out.putByte('\r');
			} else if(c=='\n' || c=='\r') {
				if(c=='\n') {
					if(!eof() && getByte()!='\r') seek(-1);
					this->l++;
					this->c=0;
				} else {
					if(!eof() && getByte()!='\n') seek(-1);
					this->l++;
					this->c=0;
				}
			} else {
				if(quote==1) {
					out.putByte(c);
				} else {
					throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected '\\'\n",l,this->c));
				}
			}
		} else if(c=='\"') {
			if(quote==1) {
				quote=0;
				break;
			} else {
				quote=1;
			}
		} else if(c=='\n' || c=='\r') {
			if(mode==1 && quote==0) {
				seek(-1);
				c=0;
				break;
			} else {
				//out.putByte(c);
				out.putByte('\n');
				if(c=='\n') {
					if(!eof() && getByte()!='\r') seek(-1);
					//else out.putByte('\r');
					this->l++;
					this->c=0;
				} else {
					if(!eof() && getByte()!='\n') seek(-1);
					//else out.putByte('\n');
					this->l++;
					this->c=0;
				}
				if(quote==0) {
					break;
				}
			}
		} else if(c=='\\') {
			slash=1; 
		} else if(quote==0 && (c==' ' || c==sep || c==',' || isblank(c))) {
			if(mode==1) {
				if(c==sep || c==',') seek(-1);
				break;
			} else {
				if(c==sep || c==',') {
					out.putByte(c);
					break;
				}
			}
		} else if(isalpha(c) || isprint(c) || alcIsAlpha(c)) {
			out.putByte(c);
			mode=1;
		} else {
			throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected character '%c'\n",l,this->c,c));
		}
	}
	// use shot only now as functions above might use it, too
	delete shot;
	shot=new tStrBuf(out);
	return *shot;
}

void tStrBuf::writeStr(const char * t) {
	this->write(t,strlen(t));
}
void tStrBuf::printf(const char * msg, ...) {
	va_list ap;
	
	char buffer[1024];

	va_start(ap,msg);
	
	vsnprintf(buffer,1023,msg,ap);
	buffer[1023]='\0';
	this->write(buffer,strlen(buffer));
	
	va_end(ap);
}
void tStrBuf::printBoolean(bool val)
{
	if (val) writeStr("yes");
	else writeStr("no");
}
U32 tStrBuf::asU32() const {
	if(size()==0) return 0;
	return atoi(c_str());
}

tStrBuf operator+(const tStrBuf & str1, const tStrBuf & str2) {
	tStrBuf out;
	out.writeStr(str1);
	out.writeStr(str2);
	return out;
}
tStrBuf operator+(const char * str1, const tStrBuf & str2) {
	tStrBuf out;
	out.writeStr(str1);
	out.writeStr(str2);
	return out;
}
tStrBuf operator+(const tStrBuf & str1, const char * str2) {
	tStrBuf out;
	out.writeStr(str1);
	out.writeStr(str2);
	return out;
}


void tTime::store(tBBuf &t) {
	DBG(9,"Buffer status size:%i,offset:%i\n",t.size(),t.tell());
	seconds=t.getU32();
	microseconds=t.getU32();
}
void tTime::stream(tBBuf &t) const {
	t.putU32(seconds);
	t.putU32(microseconds);
}
SByte tTime::compare(const tTime &t) const {
	if(seconds==t.seconds) {
		if(microseconds==t.microseconds) return 0;
		if(microseconds<t.microseconds) return -1;
		return 1;
	}
	if(seconds<t.seconds) return -1;
	return 1;
}
tTime operator+(const tTime &a,const tTime &b) {
	tTime r;
	r.seconds=a.seconds + b.seconds;
	r.microseconds=a.microseconds + b.microseconds;
	if(r.microseconds>=1000000) {
		r.seconds++;
		r.microseconds = r.microseconds % 1000000;
	}
	return r;
}
tTime operator-(const tTime &a,const tTime &b) {
	tTime r;
	r.seconds=a.seconds - b.seconds;
	if(a.microseconds<b.microseconds) {
		r.microseconds=((a.microseconds+1000000) - b.microseconds);
		r.seconds--;
	} else {
		r.microseconds=a.microseconds - b.microseconds;
	}
	return r;
}
double tTime::asDouble(char how) const {
	switch(how) {
		case 'u':
			return ((double)seconds * 1000000) + (double)microseconds;
		case 'm':
			return ((double)seconds * 1000) + ((double)microseconds / 1000);
		case 's':
		default:
			return ((double)seconds + ((double)microseconds / 1000000));
	}
}
U32 tTime::asU32(char how) const {
	switch(how) {
		case 'u':
			return ((seconds %1000) * 1000000) + microseconds;
		case 'm':
			return ((seconds %1000000) * 1000) + (microseconds / 1000);
		case 's':
		default:
			return seconds;
	}
}
const char * tTime::str(Byte type) const {
	if(type==0x00) {
		return alcGetStrTime(seconds,microseconds);
	} else {
		static tStrBuf sth;
		double sseconds=(seconds % 60) + (((double)microseconds)/1000000);
		Byte minutes=(seconds/60)%60;
		Byte hours=(seconds/3600)%24;
		Byte days=(seconds/(3600*24))%30;
		Byte months=(seconds/(3600*24*30)); //%12;
		// a year is not exactly 12 months, so 361 days would show as 0 years, 0 months and 1 day...
		//Byte years=(seconds/(3600*24*365));
		//if(years==1) sth.printf("1 year, "); //I bet that nobody will get this uptime
		//else if(years>1) sth.printf("%i years, ",years);
		if(months==1) sth.printf("1 month, "); //This one, should be possible, the GoE shard has beatten it without problems
		else if(months>1) sth.printf("%i months, ",months);
		if(days==1) sth.printf("1 day, ");
		else if(days>1) sth.printf("%i days, ",days);
		if(hours==1) sth.printf("1 hour, ");
		else if(hours>1) sth.printf("%i hours, ",hours);
		if(minutes==1) sth.printf("1 minute, ");
		else if(minutes>1) sth.printf("%i minutes, ",minutes);
		sth.printf("%.6f seconds.",sseconds);
		return sth.c_str();
	}
}
void tTime::now() {
	seconds=alcGetTime();
	microseconds=alcGetMicroseconds();
}

} //end namespace alc
