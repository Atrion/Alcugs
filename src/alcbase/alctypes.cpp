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

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include <cstdlib>
#include <cstring>

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

using namespace std;

/*
	Abstract base type class
*/
SByte tBaseType::compare(tBaseType &t) {
	DBG(9,"tBaseType::compare()\n");
	if(this->size()==t.size()) return 0;
	else if(this->size()<t.size()) return 1;
	else return -1;
}
void tBaseType::_pcopy(tBaseType &t) {
	DBG(9,"tBaseType::_pcopy()\n");
}
void tBaseType::copy(tBaseType &t) {
	DBG(9,"tBaseType::copy()\n");
	this->_pcopy(t);
	tMBuf * aux=new tMBuf();
	aux->put(t);
	aux->rewind();
	this->store(*aux);
	delete aux;
}
/* end base type */

/* 
	basicbuffer
*/
tBBuf::tBBuf() {
	DBG(9,"tBBuf::tBBuf()\n");
	this->init(); 
}
tBBuf::~tBBuf() {
	DBG(9,"~tBBuf()\n");
	if(gpbuf!=NULL) free((void *)gpbuf);
}
void tBBuf::init() {
	DBG(9,"tBBuf::init()\n");
	gpbuf=NULL;
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
	this->write((Byte *)&val,1);
}
void tBBuf::putSByte(SByte val) {
	this->write((Byte *)&val,1);
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
	return(*(Byte *)(this->read(1)));
}
SByte tBBuf::getSByte() {
	return(*(SByte *)(this->read(1)));
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
U32 tBBuf::put(tBaseType &t) {
	return(t.stream(*this));
}
void tBBuf::get(tBaseType &t) {
	t.store(*this);
}
void tBBuf::rewind() {
	this->set(0);
}
void tBBuf::end() {
	this->set(this->size());
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
void tBBuf::check(Byte * what,U32 n) {
	if(eof() || size()-tell()<n || memcmp(what,this->read(n),n)) {
		throw txUnexpectedData(_WHERE("UnexpectedData"));
	}
}
void tBBuf::_pcopy(tBBuf &t) {
	DBG(9,"tBBuf::_pcopy()\n");
	tBaseType::_pcopy(t);
}
void tBBuf::copy(tBBuf &t) {
	DBG(9,"tBBuf::copy()\n");
	this->_pcopy(t);
	this->rewind();
	t.rewind();
	this->write(t.read(),t.size());
}
char tBBuf::compare(tBBuf &t) {
	DBG(9,"tBBuf::compare()\n");
	char out;
	rewind();
	t.rewind();
	DBG(9,"%u==%u?\n",t.size(),size());
	if(t.size()==size()) {
		out=memcmp(read(),t.read(),size());
		rewind();
		t.rewind();
	} else {
		if(t.size()>size()) out=1;
		else out=-1;
	}
	return out;
}
Byte * tBBuf::hexToAscii() {
	if(gpbuf!=NULL) free((void *)gpbuf);
	Byte * out;
	out=(Byte *)malloc(sizeof(Byte) * ((2*size())+1));
	
	U32 pos;
	pos=tell();
	rewind();
	Byte * in;
	in=read();

	U32 i;
	for(i=0; i<size(); i++) {
		out[2*i]=  ((in[i] & 0xF0)>>4);
		out[2*i]= (out[2*i]<0x0A ? out[2*i]+0x30 : out[2*i]+(0x41-0x0A));
		out[(2*i)+1] = ((in[i] & 0x0F)<0x0A ? (in[i] & 0x0F)+0x30 : (in[i] & 0x0F)+(0x41-0x0A));
	}
	out[size()*2]='\0';
	
	set(pos);
	gpbuf=out;
	return gpbuf;	
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
	if(buf!=NULL) free((void *)buf);
}
void tRefBuf::resize(U32 newsize) {
	msize=newsize;
	Byte * b2 =(Byte *)realloc((void *)buf,sizeof(Byte *) * newsize);
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
tMBuf::tMBuf(tBBuf &t,U32 start,U32 len) {
	DBG(9,"tMBuf(tBBuf,start:%u,len:%u)\n",start,len);
	this->init();
	U32 pos=t.tell();
	t.rewind();
	if(start) t.read(start);
	if(!len) len=t.size()-start;
	this->write(t.read(),len);
	t.set(pos);
	set(pos);
}
tMBuf::tMBuf(U32 size) {
	DBG(9,"tMBuf(size:%i)\n",size);
	buf = new tRefBuf(size);
	mstart=msize=off=0;
}
tMBuf::tMBuf(tMBuf &t,U32 start,U32 len) {
	DBG(9,"tMBuf(tMBuf,start:%u,len:%u)\n",start,len);
	if((S32)t.msize-(S32)(start+t.mstart)<0) throw txOutOfRange(_WHERE("start:%i,size:%i",start,t.msize));
	buf = t.buf;
	buf->inc();
	if(len==0) {
		msize = t.msize-(t.mstart+start);
	} else {
		msize = len;
	}
	mstart=t.mstart+start;
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
void tMBuf::_pcopy(tMBuf &t) {
	DBG(9,"tMBuf::_pcopy()\n");
	tBBuf::_pcopy(t);
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
	mstart=t.mstart;
}
void tMBuf::copy(tMBuf &t) {
	DBG(9,"tMBuf::copy()\n");
	this->_pcopy(t);
}
void tMBuf::init() {
	DBG(9,"tMBuf::init()\n");
	buf=NULL;
	mstart=msize=off=0;
}
U32 tMBuf::tell() { return off; }
void tMBuf::set(U32 pos) { 
	if(pos>msize) throw txOutOfRange(_WHERE("OutOfRange %i>%i",pos,msize));
	off=pos;
}
Byte tMBuf::getAt(U32 pos) {
	if(pos>msize) {
		//std::printf("getAt %i \n%s\n",pos,hexToAscii());
		throw txOutOfRange(_WHERE("OutOfRange %i>%i",pos,msize));
	}
	return *(Byte *)(buf->buf+mstart+pos);
}
void tMBuf::setAt(U32 pos,const char what) {
	if(pos>msize) throw txOutOfRange(_WHERE("OutOfRange %i>%i",pos,msize));
	*(char *)(buf->buf+mstart+pos)=what;
}
void tMBuf::write(Byte * val,U32 n) {
	if(val==NULL) return;
	if(buf==NULL) buf = new tRefBuf(1024 + n);
	else if(buf->getRefs()>1) {
		buf->dec();
		Byte * oldbuf=buf->buf;
		U32 xsize=buf->size()-mstart;
		buf = new tRefBuf((xsize)+1024+n);
		memcpy(buf->buf,oldbuf+mstart,xsize);
		mstart=0;
	}
	U32 oldoff=off;
	off+=n;
	U32 bsize=buf->size();
	if(off>bsize) {
		U32 newsize = ((off-bsize)>1024 ? off+1024 : bsize+1024);
		buf->resize(newsize);
	}
	memcpy(buf->buf+oldoff,val,n);
	if(off>msize) msize+=n;
}
Byte * tMBuf::read(U32 n) {
	U32 pos=off+mstart;
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
int tMBuf::stream(tBBuf &b) {
	if(buf==NULL || buf->buf==NULL) return 0;
	b.write(buf->buf+mstart,msize);
	return msize;
}
void tMBuf::store(tBBuf &b) {
	//b.rewind();
	U32 baal=b.tell();
	this->write(b.read(),b.size()-baal);
}
U32 tMBuf::size() { return msize; }
void tMBuf::clear() {
	off=0;
	msize=0;
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
	if(xbuf!=NULL) free((void *)xbuf);
}
void tFBuf::init() {
	f=NULL;
	xsize=msize=0;
	xbuf=NULL;
}
U32 tFBuf::tell() {
	DBG(9,"ftell()\n");
	if(f!=NULL) return ftell(f);
	return 0; 
}
void tFBuf::set(U32 pos) {
	if(f==NULL || fseek(f,pos,SEEK_SET)<0) {
		throw txOutOfRange(_WHERE("OutOfRange"));
	}
}
void tFBuf::write(Byte * val,U32 n) {
	DBG(9,"write(val:%s,n:%u)\n",val,n);
	if(val==NULL) return;
	if(f==NULL) throw txNoFile(_WHERE("NoFile"));
	U32 nn=fwrite(val,n,1,f);
	if(nn!=1) throw txWriteErr(_WHERE("write error, only wrote %u of 1",nn));
}
Byte * tFBuf::read(U32 n) {
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
int tFBuf::stream(tBBuf &b) {
	this->size();
	b.write(this->read(),msize);
	return msize;
}
void tFBuf::store(tBBuf &b) {
	b.rewind();
	this->write(b.read(),b.size());
}
U32 tFBuf::size() {
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
void tFBuf::open(const void * path,const void * mode) {
	f=fopen((const char *)path,(const char *)mode);
	if(f==NULL) {
		if(errno==EACCES || errno==EISDIR) throw txNoAccess((const char *)path);
		if(errno==ENOENT) throw txNotFound((const char *)path);
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
tSBuf::tSBuf(Byte * buf,U32 msize) {
	off=0;
	this->msize=msize;
	this->buf=buf;
}
U32 tSBuf::tell() { return off; }
void tSBuf::set(U32 pos) {
	if(pos>msize) throw txOutOfRange(_WHERE("Cannot access pos %i, size %i\n",pos,msize));
	off=pos; 
}
Byte * tSBuf::read(U32 n) {
	Byte * auxbuf=buf+off;
	if(n) {
		off+=n;
		if(off>msize) { off-=n; throw txOutOfRange(_WHERE("Cannot read pos %i,len:%i size %i\n",off,n,msize)); }
	}
	else off=msize;
	return auxbuf;
}
int tSBuf::stream(tBBuf &buf) {
	buf.write(this->buf,msize);
	return msize;
}
U32 tSBuf::size() { return msize; }

/* end static buffer */

/* tZBuf */
void tZBuf::compress() {
	tRefBuf * aux=this->buf;
	aux->dec();
	int comp_size;
	comp_size=msize+(msize/10)+12;
	this->buf = new tRefBuf(comp_size);
	int ret=zlib::compress(this->buf->buf,(zlib::uLongf *)&comp_size,aux->buf+mstart,msize);
	if(ret!=0) throw txBase(_WHERE("Something terrible happenened compressing the buffer"));
	mstart=0;
	msize=comp_size;
	if(aux->getRefs()<1) delete aux;
	this->buf->resize(msize);
	off=0;
}
void tZBuf::uncompress(int iosize) {
	tRefBuf * aux=this->buf;
	aux->dec();
	int comp_size;
	if(iosize==0) { comp_size=10*msize; } //yes, is very dirty
	else comp_size=10*msize;
	this->buf = new tRefBuf(comp_size);
	int ret=zlib::uncompress(this->buf->buf,(zlib::uLongf *)&comp_size,aux->buf+mstart,msize);
	if(ret!=0) throw txBase(_WHERE("Something terrible happenened uncompressing the buffer"));
	mstart=0;
	msize=comp_size;
	if(aux->getRefs()<1) delete aux;
	this->buf->resize(msize);
	off=0;
}
/* end tZBuf */

/* md5 buff */
void tMD5Buf::compute() {
	tRefBuf * aux=this->buf;
	aux->dec();
	this->buf = new tRefBuf(0x10);
	md5::MD5((const unsigned char *)aux->buf+mstart,msize,(unsigned char *)this->buf->buf);
	mstart=0;
	msize=0x10;
	if(aux->getRefs()<1) delete aux;
	off=0;
}
/* end md5 buf */

/* String buffer */
tStrBuf::tStrBuf(const void * k) :tMBuf(200) {
	DBG(9,"ctor\n");
	init();
	writeStr((char *)k);
	rewind();
}
tStrBuf::tStrBuf(U32 size) :tMBuf(size) { init(); }
tStrBuf::tStrBuf(tBBuf &k,U32 start,U32 len) :tMBuf(k,start,len) {
	DBG(9,"copy zero\n");
	init();
	if(size()) isNull(false);
}
tStrBuf::tStrBuf(tMBuf &k,U32 start,U32 len) :tMBuf(k,start,len) { 
	DBG(9,"copy one\n");
	init(); 
	if(size()) isNull(false);
}
tStrBuf::tStrBuf(const tStrBuf &k,U32 start,U32 len) :tMBuf((tStrBuf &)k,start,len) { 
	DBG(9,"copy two\n");
	if(this==&k) return;
	init(); 
	l=k.l; 
	c=k.c;
	sep=k.sep;
	flags=k.flags;
	DBG(9,"flags are %02X\n",flags);
}
tStrBuf::~tStrBuf() {
	DBG(9,"dtor\n");
	if(bufstr!=NULL) free((void *)bufstr);
	if(shot!=NULL) delete shot;
}
void tStrBuf::init() {
	DBG(9,"init\n");
	bufstr=NULL;
	l=c=0;
	sep='=';
	shot=NULL;
	flags=0x02; //Null true by default
}
void tStrBuf::_pcopy(tStrBuf &t) {
	DBG(9,"tStrBuf::_pcopy()\n");
	if(this==&t) return;
	tMBuf::_pcopy(t);
	if(bufstr!=NULL) free((void *)bufstr);
	bufstr=NULL;
	l=t.l;
	c=t.c;
	sep=t.sep;
	flags=t.flags;
	//last thing
	if(shot!=NULL) delete shot;
	shot=NULL;
	DBG(9,"flags are %02X\n",flags);
}
void tStrBuf::copy(tStrBuf &t) {
	DBG(9,"tStrBuf::copy()\n");
	if(this==&t) return;
	this->_pcopy(t);
	DBG(9,"flags are %02X\n",flags);
}
void tStrBuf::copy(const void * str) {
	DBG(9,"cpy\n");
	tStrBuf pat(str);
	copy(pat);
}
SByte tStrBuf::compare(tStrBuf &t) {
	if(this==&t) return 0;
	rewind();
	t.rewind();
	U32 s = size();
	U32 s2 = t.size();
	//if(s>s2) s=s2;
	DBG(9,"sizes %i,%i\n",s,s2);
	if(s<s2) return 1;
	if(s>s2) return -1;
	return((SByte)strncmp((char *)read(),(char *)t.read(),s));
}
SByte tStrBuf::compare(const void * str) {
	DBG(9,"compare %s\n",str);
	tStrBuf pat(str);
	return(compare(pat));
}
const Byte * tStrBuf::c_str() {
	if(isNull()) {
		return NULL;
	}
	end();
	putByte(0);
	setSize(tell()-1);
	rewind();
	return read();
}
void tStrBuf::rewind() {
	tMBuf::rewind();
	c=l=0;
}
bool tStrBuf::hasQuotes() {
	DBG(9,"hasQuotes %02X\n",flags);
	return flags & 0x01;
}
void tStrBuf::hasQuotes(bool has) {
	if (has) {
		flags |= 0x01;
	} else {
		flags &= ~0x01;
	}
	DBG(9,"hasQuotes enable %02X\n",flags);
}
bool tStrBuf::isNull() {
	return flags & 0x02;
}
void tStrBuf::isNull(bool val) {
	DBG(9,"isNull %i\n",val);
	//throw txBase("s",1,1);
	if (val) {
		flags |= 0x02;
	} else {
		flags &= ~0x02;
	}
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
S32 tStrBuf::find(const char cat, bool reverse) {
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
tStrBuf & tStrBuf::strip(Byte what,Byte how) {
	tStrBuf aux;
	int i,max,start,end;
	start=0;
	i=0;
	max=size();
	end=max-1;
	Byte ctrl=what;
	if(how & 0x01) {
		while(i<max && ctrl==what) {
			//std::printf("get1 %i\n",i);
			ctrl=getAt(i++);
		}
		start=i-1;
		if(start<0) start=0;
	}
	if(how & 0x02) {
		ctrl=what;
		i=max-1;
		while(i>=0 && ctrl==what) {
			//std::printf("get2 %i\n",i);
			ctrl=getAt(i--);
		}
		end=i+1;
		if(end>=max) end=max-1;
	}
	for(i=start; i<=end; i++) {
		//std::printf("put %i\n",i);
		aux.putSByte(getAt(i));
	}
	aux.rewind();
	copy(aux);
	return *this;
}
tStrBuf & tStrBuf::escape() {
	tStrBuf * out;
	out = new tStrBuf(200);

	int i,max;
	Byte ctrl; 
	max=size();
	for(i=0; i<max; i++) {
		ctrl=getAt(i);
		if(ctrl=='\n') {
			out->putByte('\\');
			out->putByte('n');
		} else if(ctrl=='\r') {
			out->putByte('\\');
			out->putByte('r');
		} else if(ctrl=='"') {
			out->putByte('\\');
			out->putByte('"');
		} else if(ctrl=='\\') {
			out->putByte('\\');
			out->putByte('\\');
		} else {
			out->putByte(ctrl);
		}
	}
	
	if(shot!=NULL) delete shot;
	shot=out;
	return *out;
}

tStrBuf & tStrBuf::lower() {
	tStrBuf * out;
	out = new tStrBuf(200);
	
	int i,max;
	max=size();
	for(i=0; i<max; i++) {
		out->putByte(std::tolower(getAt(i)));
	}
	
	if(shot!=NULL) delete shot;
	shot=out;
	return *out;
}

tStrBuf & tStrBuf::upper() {
	tStrBuf * out;
	out = new tStrBuf(200);
	
	int i,max;
	max=size();
	for(i=0; i<max; i++) {
		out->putByte(std::toupper(getAt(i)));
	}
	
	if(shot!=NULL) delete shot;
	shot=out;
	return *out;
}


tStrBuf & tStrBuf::substring(U32 start,U32 len) {
	tStrBuf * out;
	out = new tStrBuf(200);

	set(start);
	out->write(read(len),len);
	//std::printf("wtf -%s-%s-\n",c_str(),out->c_str());

	if(shot!=NULL) delete shot;
	shot=out;
	return *out;
}
bool tStrBuf::startsWith(const void * pat) {
	return(substring(0,strlen((const char *)pat))==pat);
}
bool tStrBuf::endsWith(const void * pat) {
	return(substring(size()-strlen((const char *)pat),strlen((const char *)pat))==pat);
}
tStrBuf & tStrBuf::dirname() {
	tStrBuf * out;
	out = new tStrBuf(200);
	int pos;
	strip('/',0x02);
	pos=find('/',1);
	//std::printf("thepos %i\n",pos);
	
	if(pos==-1) {
		out->writeStr(".");
	} else if(pos==0) {
		out->writeStr("/");
	} else {
		out->writeStr(substring(0,pos));
	}

	if(shot!=NULL) delete shot;
	shot=out;
	return *out;
}
tStrBuf & tStrBuf::getLine(bool nl,bool slash) {
	DBG(9,"getLine()\n");
	Byte c=0;
	Byte slashm=0;
	tStrBuf * out;
	out = new tStrBuf(200);

	while(!eof()) {
		c=getByte();
		if(!slash) {
			if(c=='\\') {
				if(slashm) {
					slashm=0;
					out->putByte('\\');
					out->putByte('\\');
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
					out->putByte('\\');
				}
				out->putByte(c);
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
				out->putByte(c);
			}
		}
	}
	if(nl) {
		if(c=='\n' || c=='\r') {
			out->putByte('\n');
		}
	}
	if(shot!=NULL) delete shot;
	shot=out;
	return *out;
}

tStrBuf & tStrBuf::getWord(bool slash) {
	DBG(9,"getWord()\n");
	Byte cc=0,c=0;
	Byte slashm=0;
	tStrBuf * out;
	out = new tStrBuf(200);

	while(!eof()) {
		c=getByte();
		this->c++;
		if(!slash) {
			if(c=='\\') {
				if(slashm) {
					slashm=0;
					out->putByte('\\');
					out->putByte('\\');
				} else {
					slashm=1;
				}
			} else if(c=='\n') {
				if(!eof() && getByte()!='\r') seek(-1);
				this->l++;
				this->c=0;
				if(!slashm) {
					if(out->size()!=0) {
						seek(-1);
					}
					break;
				}
				slashm=0;
				c=0;
			} else if(c=='\r') {
				if(!eof() && getByte()!='\n') seek(-1);
				this->l++;
				this->c=0;
				if(!slashm) {
					if(out->size()!=0) {
						seek(-1);
					}
					break;
				}
				slashm=0;
				c=0;
			} else if(c==sep || c==' ' || isblank(c)) {
				while(!eof()) {
					cc=c;
					c=getByte();
					if(c==sep || c==' ' || isblank(c)) {
						this->c++;
					} else {
						c=cc;
						if(out->size()!=0) {
							seek(-2);
							this->c--;
						} else {
							seek(-1);
						}
						break;
					}
				}
				break;
			} else {
				if(slashm) {
					slashm=0;
					out->putByte('\\');
				}
				out->putByte(c);
			}
		} else { //slash is true
			if(c=='\n') {
				if(!eof() && getByte()!='\r') seek(-1);
				this->l++;
				if(out->size()!=0) {
					seek(-1);
				}
				break;
			} else if(c=='\r') {
				if(!eof() && getByte()!='\n') seek(-1);
				this->l++;
				if(out->size()!=0) {
					seek(-1);
				}
				break;
			} else if(c==sep || c==' ' || isblank(c)) {
				while(!eof()) {
					cc=c;
					c=getByte();
					if(c==sep || c==' ' || isblank(c)) {
						this->c++;
					} else {
						c=cc;
						if(out->size()!=0) {
							seek(-2);
							this->c--;
						} else {
							seek(-1);
						}
						break;
					}
				}
				break;
			} else {
				out->putByte(c);
			}
		}
	}
	if (out->size()==0) {
		if(c=='\n' || c=='\r') {
			out->putByte('\n');
		} else if(c==sep) {
			out->putByte(sep);
		} else if(c==' ' || isblank(c)) {
			out->putByte(' ');
		}
	}
	if(shot!=NULL) delete shot;
	shot=out;
	return *out;
}
tStrBuf & tStrBuf::getToken() {
	DBG(9,"tStrBuf::getToken()\n");
	Byte c;
	Byte slash=0;
	Byte quote=0;
	Byte mode=0;
	tStrBuf * out;
	out = new tStrBuf(200);
	//out->hasQuotes(true);
	//assert(out->hasQuotes());
	while(!eof()) {
		c=getByte();
		this->c++;
		if(quote==0 && (c=='#' || c==';')) {
			getLine();
			out->putByte('\n');
			break;
		} else if(slash==1) {
			slash=0;
			if(quote==1 && (c=='n' || c=='r')) {
				if(c=='n') out->putByte('\n');
				else out->putByte('\r');
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
					out->putByte(c);
				} else {
					throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected '\\'\n",l,this->c));
				}
			}
		} else if(c=='\"') {
			out->hasQuotes(true);
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
				//out->putByte(c);
				out->putByte('\n');
				if(c=='\n') {
					if(!eof() && getByte()!='\r') seek(-1);
					//else out->putByte('\r');
					this->l++;
					this->c=0;
				} else {
					if(!eof() && getByte()!='\n') seek(-1);
					//else out->putByte('\n');
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
					out->putByte(c);
					break;
				}
			}
		} else if(isalpha(c) || isprint(c) || alcIsAlpha(c)) {
			out->putByte(c);
			mode=1;
		} else {
			throw txParseError(_WHERE("Parse error at line %i, column %i, unexpected character '%c'\n",l,this->c,c));
		}
	}
	if(shot!=NULL) delete shot;
	shot=out;
	return *out;
}

void tStrBuf::writeStr(const Byte * t) {
	this->write((Byte *)t,strlen((const char *)t));
}
void tStrBuf::printf(const char * msg, ...) {
	va_list ap;
	
	char buffer[1024];

	va_start(ap,msg);
	       
	vsnprintf(buffer,1023,msg,ap);
	buffer[1023]='\0';
	this->write((Byte *)buffer,strlen(buffer));
	
	va_end(ap);
}
U32 tStrBuf::asU32() {
	rewind();
	DBG(9,"asU32 %s\n",c_str());
	if(size()==0) return 0;
	return atoi((char *)c_str());
}

tStrBuf operator+(const tStrBuf & str1, const tStrBuf & str2) {
	tStrBuf out;
	out.writeStr(str1);
	out.writeStr(str2);
	return out;
}
tStrBuf operator+(const void * str1, const tStrBuf & str2) {
	tStrBuf out;
	out.writeStr(str1);
	out.writeStr(str2);
	return out;
}
tStrBuf operator+(const tStrBuf & str1, const void * str2) {
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
int tTime::stream(tBBuf &t) {
	t.putU32(seconds);
	t.putU32(microseconds);
	return 8;
}
U32 tTime::size() { return 8; }
SByte tTime::compare(tTime &t) {
	if(seconds==t.seconds) {
		if(microseconds==t.microseconds) return 0;
		if(microseconds<t.microseconds) return 1;
		return -1;
	}
	if(seconds>t.seconds) return 1;
	return -1;
}
tTime operator+(tTime &a,tTime &b) {
	tTime r;
	r.seconds=a.seconds + b.seconds;
	r.microseconds=a.microseconds + b.microseconds;
	if(r.microseconds>=1000000) {
		r.seconds++;
		r.microseconds = r.microseconds % 1000000;
	}
	return r;
}
tTime operator-(tTime &a,tTime &b) {
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
double tTime::asDouble(char how) {
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
U32 tTime::asU32(char how) {
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
const Byte * tTime::str(Byte type) {
	if(type==0x00) {
		return alcGetStrTime(seconds,microseconds);
	} else {
		static tStrBuf sth;
		double sseconds=(seconds % 60) + (((double)microseconds)/1000000);
		Byte minutes=(seconds/60)%60;
		Byte hours=(seconds/3600)%24;
		Byte days=(seconds/(3600*24))%30;
		Byte months=(seconds/(3600*24*30))%12;
		Byte years=(seconds/(3600*24*365));
		if(years==1) sth.printf("1 year, "); //I bet that nobody will get this uptime
		else if(years>1) sth.printf("%i years, ",years);
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
