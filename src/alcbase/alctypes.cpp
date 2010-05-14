/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs Server Team                           *
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

static const U32 bufferOversize = 512; // number of Bytes to reserve in addition to the actual need

/* 
	basicbuffer
*/
//Uru is Little-Endian, so if we are on a Big-Endian machine, 
// all writes and reads to any network/file buffer must be
// correctly swapped.

//For these put* functions DO NOT change val to a reference (though I don't
// know why anyone would). If you do, the byte-swapping will hurt you!
void tBBuf::putU16(U16 val) {
	val = htole16(val);
	this->write(reinterpret_cast<Byte *>(&val),2);
}
void tBBuf::putS16(S16 val) {
	val = htole16(val);
	this->write(reinterpret_cast<Byte *>(&val),2);
}
void tBBuf::putU32(U32 val) {
	val = htole32(val);
	this->write(reinterpret_cast<Byte *>(&val),4);
}
void tBBuf::putS32(S32 val) {
	val = htole32(val);
	this->write(reinterpret_cast<Byte *>(&val),4);
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
	U32 *valAsArray = reinterpret_cast<U32 *>(&val);
	U32 lo = htole32(valAsArray[1]);
	U32 hi = htole32(valAsArray[0]);
	valAsArray[0] = lo;
	valAsArray[1] = hi;
#endif
	this->write(reinterpret_cast<Byte *>(&val),8);
}
void tBBuf::putFloat(float val) { // Does this work on big-endian?
	this->write(reinterpret_cast<Byte *>(&val),4);
}
U16 tBBuf::getU16() {
	U16 val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy(&val, this->read(2), 2);
#else
	val = *reinterpret_cast<U16 *>(this->read(2));
#endif
	return(letoh16(val));
}
S16 tBBuf::getS16() {
	S16 val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy(&val, this->read(2), 2);
#else
	val = *reinterpret_cast<S16 *>(this->read(2));
#endif
	return(letoh16(val));
}
U32 tBBuf::getU32() {
	U32 val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy(&val, this->read(4), 4);
#else
	val = *reinterpret_cast<U32 *>(this->read(4));
#endif
	return(letoh32(val));
}
S32 tBBuf::getS32() {
	S32 val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy(&val, this->read(4), 4);
#else
	val = *reinterpret_cast<S32 *>(this->read(4));
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
	memcpy(&val, this->read(8), 8);
#else
	val = *reinterpret_cast<double *>(this->read(8));
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
float tBBuf::getFloat() { // Does this work on big-endian?
	float val;
#if defined(NEED_STRICT_ALIGNMENT)
	memcpy(&val, this->read(4), 4);
#else
	val = *reinterpret_cast<float *>(this->read(4));
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
	this->set(res);
}
void tBBuf::check(const Byte * what,U32 n) {
	if(n && (size()-tell()<n || memcmp(what,read(n),n) != 0)) { // make sure we throw "unexpected data", not "out of range"
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
	buffer=static_cast<Byte *>(malloc(sizeof(Byte) * csize));
	if(buffer==NULL) throw txNoMem(_WHERE("NoMem"));
}
tRefBuf::~tRefBuf() {
	free(buffer);
}
void tRefBuf::resize(U32 newsize) {
	msize=newsize;
	Byte * b2 =static_cast<Byte *>(realloc(buffer,sizeof(Byte *) * newsize));
	if(b2==NULL) throw txNoMem(_WHERE("NoMem"));
	buffer=b2;
}
void tRefBuf::inc() { refs++; }
void tRefBuf::dec() {
	if(refs==0) throw txRefErr(_WHERE("RefErr %i-1",refs));
	refs--;
}
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
	this->write(t.read(t.size()), t.size());
	t.set(pos); // restore old pos
	set(pos);
}
tMBuf::tMBuf(const tMBuf &t) {
	DBG(9,"tMBuf(tMBuf)\n");
	buf = t.buf;
	if (buf != NULL) buf->inc();
	msize = t.msize;
	off=t.off;
}
tMBuf::tMBuf(const Byte *d, U32 s) {
	init();
	write(d, s);
}
tMBuf::tMBuf(U32 size) {
	init();
	buf = new tRefBuf(size);
	msize = size;
}
tMBuf::~tMBuf() {
	DBG(9,"~tMBuf()\n");
	clear();
}
void tMBuf::getUniqueBuffer(U32 newsize) {
	DBG(6, "cur size: %d, new size: %d\n", msize, newsize);
	if (buf != NULL && buf->getRefs() == 1) { // check if the buffer is large enough
		if (buf->size() < newsize) buf->resize(newsize+bufferOversize);
		return;
	}
	// make unique copy of us
	tRefBuf *newbuf = new tRefBuf(std::max(msize,newsize+bufferOversize)); // don't shrink buffer
	if (buf) { // copy old buffer if there is one
		memcpy(newbuf->buf(),buf->buf(),msize);
		buf->dec(); // decrement old buffer's reference
	}
	buf = newbuf; // and use the new one
}
void tMBuf::copy(const tMBuf &t) {
	DBG(9,"tMBuf::copy()\n");
	if (&t == this) return;
	clear();
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
void tMBuf::set(U32 pos) { 
	if(pos > msize) // allow pos == msize for EOF
		throw txOutOfRange(_WHERE("OutOfRange, must be 0<=%i<%i",pos,msize));
	off=pos;
}
Byte tMBuf::getAt(U32 pos) const {
	if(pos >= msize) {
		throw txOutOfRange(_WHERE("OutOfRange %i>%i",pos,msize));
	}
	return *(buf->buf()+pos);
}
void tMBuf::setAt(U32 pos,const Byte what) {
	if(pos >= msize) throw txOutOfRange(_WHERE("OutOfRange %i>%i",pos,msize));
	getUniqueBuffer(msize);
	*(buf->buf()+pos)=what;
}
void tMBuf::write(const Byte * val,U32 n) {
	if(n==0) return;
	getUniqueBuffer(off+n); // the buffer will never be shrunken
	memcpy(buf->buf()+off,val,n);
	off+=n;
	if(off>msize) msize=off;
}
const Byte * tMBuf::read(U32 n) {
	U32 oldpos=off;
	off+=n;
	if(off>msize || buf==NULL) { 
		DBG(8,"off:%u,msize:%u\n",off,msize);
		off-=n; 
		throw txOutOfRange(_WHERE("OutOfRange %i>%i",off+n,msize)); 
		//return NULL;
	}
	return buf->buf()+oldpos;
}
void tMBuf::stream(tBBuf &b) const {
	if (buf==NULL) return;
	b.write(buf->buf(),msize);
}
void tMBuf::clear() {
	off=0;
	msize=0;
	if(buf!=NULL) {
		buf->dec();
		if(buf->getRefs()<=0) {
			delete buf;
		}
		buf = NULL;
	}
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
	SByte out = memcmp(buf->buf(), t.buf->buf(), std::min(s1, s2));
	if (out != 0 || s1 == s2) return out;
	return (s1 < s2) ? -1 : 1;
}
void tMBuf::addNullTerminator(void) const
{
	tRefBuf *buf = const_cast<tRefBuf *>(this->buf); // yep, a hack - but we won't change the actual content
	if (msize == buf->size()) buf->resize(msize+200);
	*(buf->buf()+msize) = 0;
}
void tMBuf::cutEnd(U32 newSize)
{
	if (newSize > msize) throw txOutOfRange(_WHERE("cutEnd() can only decrease the size"));
	msize = newSize;
	if (off > newSize) end();
}
/* end tMBuf */

/* 
	File Buffer 
*/
tFBuf::tFBuf() {
	this->init();
}
tFBuf::tFBuf(const char *file,const char * mode) {
	this->init();
	this->open(file,mode);
}
tFBuf::~tFBuf() {
	this->close();
	free(xbuf);
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
	if(n==0) return;
	if(f==NULL) throw txNoFile(_WHERE("NoFile"));
	if(fwrite(val,n,1,f)!=1) throw txWriteErr(_WHERE("write error"));
}
const Byte * tFBuf::read(U32 n) {
	if(f==NULL) throw txNoFile(_WHERE("NoFile"));
	if(xsize<n) {
		free(xbuf);
		xbuf=static_cast<Byte *>(malloc(n));
		if(xbuf==NULL) throw txNoMem(_WHERE("NoMem"));
		xsize=n;
	}
	if (fread(xbuf,n,1,f) != 1) throw txReadErr(_WHERE("error while reading"));
	return xbuf;
}
void tFBuf::stream(tBBuf &b) const {
	// make sure the position in the file is the same after the read
	U32 pos = tell();
	fseek(f,0,SEEK_SET);
	if(xsize<msize) {
		free(xbuf);
		xbuf=static_cast<Byte *>(malloc(msize));
		if(xbuf==NULL) throw txNoMem(_WHERE("NoMem"));
		xsize=msize;
	}
	if (fread(xbuf,msize,1,f) != 1) throw txReadErr(_WHERE("error while reading"));
	b.write(xbuf,msize);
	fseek(f,pos,SEEK_SET);
}
U32 tFBuf::size() const {
	return msize;
}
void tFBuf::open(const char * path,const char * mode) {
	f=fopen(path,mode);
	if(f==NULL) {
		if(errno==EACCES || errno==EISDIR) throw txNoAccess(path);
		if(errno==ENOENT) throw txNotFound(path);
		throw txUnkErr("Unknown error code: %i\n",errno);
	}
	setCloseOnExec(fileno(f)); // close file when forking a game server
	// get size and seek back to beginning
	fseek(f,0,SEEK_END);
	msize=ftell(f);
	fseek(f,0,SEEK_SET);
}
void tFBuf::close() {
	if(f!=NULL) { fclose(f); }
	f=NULL;
	msize=0;
}
void tFBuf::flush() {
	if (f != NULL) fflush(f);
}
/* end File buffer */

/* static buffer */
tSBuf::tSBuf(const Byte * buf,U32 msize) {
	off=0;
	this->msize=msize;
	this->buf=buf;
}
void tSBuf::set(U32 pos) {
	if (pos >= msize) throw txOutOfRange(_WHERE("Cannot access pos %i, size %i\n",pos,msize));
	off=pos; 
}
const Byte * tSBuf::read(U32 n) {
	U32 oldpos=off;
	off+=n;
	if(off>msize) { 
		off-=n; 
		throw txOutOfRange(_WHERE("OutOfRange %i>%i",off+n,msize)); 
	}
	return buf+oldpos;
}
void tSBuf::stream(tBBuf &buf) const {
	buf.write(this->buf,msize);
}

/* end static buffer */

/* tZBuf */
void tZBuf::compress() {
	zlib::uLongf comp_size = zlib::compressBound(size());
	tZBuf newBuf(comp_size);
	int ret=zlib::compress(newBuf.volatileData(),&comp_size,data(),size());
	/* zlib docu for compress:
		int compress(Bytef *dest,   uLongf *destLen,
					const Bytef *source, uLong sourceLen)
		Compresses the source buffer into the destination buffer.  sourceLen is
		the byte length of the source buffer. Upon entry, destLen is the total
		size of the destination buffer, which must be at least the value returned
		by compressBound(sourceLen). Upon exit, destLen is the actual size of the
		compressed buffer.*/
	if(ret!=0) throw txBase(_WHERE("Something terrible happenened compressing the buffer"));
	copy(newBuf);
	cutEnd(comp_size);
	rewind();
}
void tZBuf::uncompress(U32 iosize) {
	zlib::uLongf comp_size=iosize;
	tZBuf newBuf(comp_size);
	int ret=zlib::uncompress(newBuf.volatileData(),&comp_size,data(),size());
	/* zlib docu for uncompress:
		int uncompress(Bytef *dest,   uLongf *destLen,
						const Bytef *source, uLong sourceLen)
		Decompresses the source buffer into the destination buffer.  sourceLen is
		the byte length of the source buffer. Upon entry, destLen is the total
		size of the destination buffer, which must be large enough to hold the
		entire uncompressed data. (The size of the uncompressed data must have
		been saved previously by the compressor and transmitted to the decompressor
		by some mechanism outside the scope of this compression library.)
		Upon exit, destLen is the actual size of the compressed buffer. */
	if(ret!=0) throw txBase(_WHERE("Something terrible happenened uncompressing the buffer"));
	if (iosize != comp_size)
		throw txUnexpectedData(_WHERE("tZBuf size mismatch: %i != %i", iosize, comp_size));
	copy(newBuf);
	rewind();
}
/* end tZBuf */

/* md5 buff */
void tMD5Buf::compute() {
	tMD5Buf newBuf(0x10);
	md5::MD5(data(),size(),newBuf.volatileData());
	copy(newBuf);
	rewind();
}
/* end md5 buf */

/* String buffer */
void tString::store(tBBuf &t)
{
	clear();
	U32 bufSize = t.getU16();
	if (bufSize & 0xF000) throw txUnexpectedData(_WHERE("This is an inverted string!"));
	if (bufSize)
		write(t.read(bufSize), bufSize);
}
void tString::stream(tBBuf &t) const
{
	t.putU16(size());
	tMBuf::stream(t); // just puts the bytes into the buffer
}
SByte tString::compare(const char * str) const {
	U32 s1 = size(), s2 = strlen(str);
	if (!s1 || !s2) {
		// needs special treatment as buf might be NULL
		if (!s1 && !s2) return 0; // both empty
		else if (s1) return 1; // we empty
		else return -1; // the other one empty
	}
	SByte out = memcmp(data(), str, std::min(s1, s2));
	if (out != 0 || s1 == s2) return out;
	return (s1 < s2) ? -1 : 1;
}
const char * tString::c_str() const {
	if (!size()) return "";
	addNullTerminator();
	return reinterpret_cast<const char *>(data());
}
S32 tString::find(const char cat, bool reverse) const {
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
S32 tString::find(const char *str) const {
	const char *c = strstr(c_str(), str);
	if (c == NULL) return -1;
	return (c-reinterpret_cast<const char *>(data()));
}
void tString::convertSlashesFromWinToUnix() {
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
void tString::convertSlashesFromUnixToWin() {
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
tString tString::stripped(Byte what,Byte how) const {
	tString aux;
	U32 i,max,start,end;
	start=0;
	i=0;
	max=size();
	end=max-1;
	Byte ctrl;
	if(how & 0x01) {
		while (start <= end) {
			ctrl=getAt(start);
			if (ctrl != what) break;
			++start;
		}
		// start now points to the first character to be copied
	}
	if(how & 0x02) { // if there's still anything left to strip from
		while (end >= start) {
			ctrl = getAt(end);
			if (ctrl != what || !end) break; // be acreful we don't count below 0
			--end;
		}
		// end now points to the last character to be copied
	}
	aux.write(data()+start, end+1-start);
	return aux;
}
tString tString::escape() const {
	tString shot;
	int i,max;
	Byte ctrl; 
	max=size();
	for(i=0; i<max; i++) {
		ctrl=getAt(i);
		if(ctrl=='\n') {
			shot.putByte('\\');
			shot.putByte('n');
		} else if(ctrl=='\r') {
			shot.putByte('\\');
			shot.putByte('r');
		} else if(ctrl=='"') {
			shot.putByte('\\');
			shot.putByte('"');
		} else if(ctrl=='\\') {
			shot.putByte('\\');
			shot.putByte('\\');
		} else {
			shot.putByte(ctrl);
		}
	}
	return shot;
}
tString tString::lower() const {
	tString shot(size());
	for(U32 i=0; i<size(); i++) {
		*(shot.volatileData()+i) = std::tolower(*(data()+i));
	}
	return shot;
}
tString tString::upper() const {
	tString shot(size());
	for(U32 i=0; i<size(); i++) {
		*(shot.volatileData()+i) = std::toupper(*(data()+i));
	}
	return shot;
}
tString tString::substring(U32 start,U32 len) const {
	if (len == 0) len = size()-start;
	tString shot;
	shot.write(data()+start,len);
	return shot;
}
bool tString::startsWith(const char * pat) const {
	try {
		return(substring(0,strlen(pat))==pat);
	}
	catch (txOutOfRange &e) {
		return false;
	}
}
bool tString::endsWith(const char * pat) const {
	try {
		return(substring(size()-strlen(pat),strlen(pat))==pat);
	}
	catch (txOutOfRange &e) {
		return false;
	}
}
tString tString::dirname() const {
	tString shot = stripped('/',0x02);
	int pos=shot.find('/',/*reverse*/true);
	
	if(pos==-1) {
		return ".";
	} else if(pos==0) {
		return "/";
	} else {
		return shot.substring(0,pos);
	}
}
tString tString::filename() const {
	int pos=find('/',/*reverse*/true);
	
	if(pos==-1) {
		return *this;
	} else {
		return substring(pos+1);
	}
}
void tString::writeStr(const char * t) {
	this->write(t,strlen(t));
}
void tString::printf(const char * msg, ...) {
	va_list ap;
	va_start(ap,msg);
	vprintf(msg,ap);
	va_end(ap);
}
void tString::vprintf(const char * msg, va_list ap)
{
	const int size = 2048;
	char buffer[size];
	
	if (vsnprintf(buffer,size,msg,ap) >= size) // "a return value of size or more means that the output was truncated"
		throw txWriteErr(_WHERE("String is too long (max. %d)", size));
	this->write(buffer,strlen(buffer));
}
void tString::printBoolean(bool val)
{
	if (val) writeStr("yes");
	else writeStr("no");
}
U32 tString::asU32() const {
	if(size()==0) return 0;
	return atoi(c_str());
}

tString tString::fromByte(Byte val)
{
	tString str;
	str.printf("%d", val);
	return str;
}

tString operator+(const tString & str1, const tString & str2) {
	tString out(str1);
	out.writeStr(str2);
	return out;
}
tString operator+(const char * str1, const tString & str2) {
	tString out(str1);
	out.writeStr(str2);
	return out;
}
tString operator+(const tString & str1, const char * str2) {
	tString out(str1);
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
			return (seconds * 1000000.0) + microseconds;
		case 'm':
			return (seconds * 1000.0) + (microseconds / 1000.0);
		case 's':
		default:
			return seconds + (microseconds / 1000000.0);
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
tString tTime::str(Byte type) const {
	if(type==0x00) {
		return alcGetStrTime(seconds,microseconds);
	} else {
		U32 minutes=seconds/60;
		U32 hours=minutes/60;
		minutes %= 60;

		U32 days=hours/24;
		hours %= 24;

		U32 weeks=days/7;
		days %= 7;

		tString sth;
		if(weeks==1) sth.printf("1 week, ");
		else if(weeks>1) sth.printf("%i weeks, ",weeks);
		if(days==1) sth.printf("1 day, ");
		else if(days>1) sth.printf("%i days, ",days);
		if(hours==1) sth.printf("1 hour, ");
		else if(hours>1) sth.printf("%i hours, ",hours);
		if(minutes==1) sth.printf("1 minute, ");
		else if(minutes>1) sth.printf("%i minutes, ",minutes);
		sth.printf("%.6f seconds.",seconds%60 + microseconds/1000000.0);
		return sth;
	}
}
void tTime::setToNow() {
	seconds=alcGetTime();
	microseconds=alcGetMicroseconds();
}

} //end namespace alc
