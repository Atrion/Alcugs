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
//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "alctypes.h"

#include "alcexception.h"
namespace md5 {
#include "alcutil/md5.h"
}

#include <iostream>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cassert>
#include <sys/time.h>


namespace zlib {
#include <zlib.h>
}

namespace alc {

static const unsigned int bufferOversize = 512; // number of Bytes to reserve in addition to the actual need

/* 
	basicbuffer
*/
//Uru is Little-Endian, so if we are on a Big-Endian machine, 
// all writes and reads to any network/file buffer must be
// correctly swapped.

void tBBuf::putFloat(float val) { // Does this work on big-endian?
	this->write((&val),4);
}
void tBBuf::putDouble(double val) {
#if defined(WORDS_BIGENDIAN)
	//ok for integers, but I don't trust it for doubles
	//val = htole32(val >> 32) | ((htole32(val & 0xffffffff)) << 32);
	//is there a less yucky way?
	uint32_t *valAsArray = reinterpret_cast<uint32_t *>(&val);
	uint32_t lo = htole32(valAsArray[1]);
	uint32_t hi = htole32(valAsArray[0]);
	valAsArray[0] = lo;
	valAsArray[1] = hi;
#endif
	this->write((&val),8);
}

float tBBuf::getFloat() { // Does this work on big-endian?
	float val;
	memcpy(&val, this->read(4), 4);
	return(val);
}
double tBBuf::getDouble() {
	double val;
	memcpy(&val, this->read(8), 8);
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
void tBBuf::seek(ssize_t n, uint8_t flags) {
	size_t res;
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
void tBBuf::check(const void * what,size_t n) {
	if(n && (size()-tell()<n || memcmp(what,read(n),n) != 0)) { // make sure we throw "unexpected data", not "out of range"
		throw txUnexpectedData(_WHERE("UnexpectedData"));
	}
}
/* end Basic buffer */

/* 
	tRefBuf 
*/
tMBuf::tRefBuf::tRefBuf(size_t csize) {
	refs=1;
	msize=csize;
	buffer=malloc(csize);
	if(buffer==NULL) throw txNoMem(_WHERE("NoMem"));
}
tMBuf::tRefBuf::~tRefBuf() {
	free(buffer);
}
void tMBuf::tRefBuf::resize(size_t newsize) {
	msize=newsize;
	void * b2 =realloc(buffer,newsize);
	if(b2==NULL) throw txNoMem(_WHERE("NoMem"));
	buffer=b2;
}
void tMBuf::tRefBuf::inc() { tSpinLock lock(mutex); refs++; }
void tMBuf::tRefBuf::dec() {
	mutex.lock();
	--this->refs;
	if(refs==0) {
		mutex.unlock();
		delete this; // nobody wants us anymore :( but be sure not to hold the lock when deleting!
	}
	else
		mutex.unlock();
}
void tMBuf::tRefBuf::resizeLocked(size_t newsize)
{
	assert(newsize >= msize);
	tSpinLock lock(mutex);
	resize(newsize);
}
tMBuf::tRefBuf* tMBuf::tRefBuf::uniqueWithSize(size_t newsize)
{
	assert(newsize >= msize);
	tSpinLock lock(mutex);
	if (refs == 1) {
		// nobody else can access this, so holing the spinlock doesn't even harm
		if (newsize > msize) resize(newsize);
		return this; // easy case
	}
	// got to create a new one and copy the data, and decrease our own refcount
	tRefBuf *newBuf = new tRefBuf(newsize);
	memcpy(newBuf->buffer, buffer, msize);
	--this->refs;
	return newBuf;
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
	size_t pos=t.tell();
	t.rewind();
	this->write(t.read(t.size()), t.size());
	t.set(pos); // restore old pos
	set(pos);
}
tMBuf::tMBuf(const tMBuf &t) {
	DBG(9,"tMBuf(tMBuf)\n");
	assert(this != &t); // yes, it can happen that &t == this... if a class constructor initializes a value with itself
	init();
	copy(t);
}
tMBuf::tMBuf(const void *d, size_t s) {
	init();
	write(d, s);
}
tMBuf::tMBuf(size_t size) {
	init();
	buf = new tRefBuf(size);
	msize = size;
}
tMBuf::~tMBuf() {
	DBG(9,"~tMBuf()\n");
	clear();
}
void tMBuf::getUniqueBuffer(size_t newsize) {
	DBG(6, "cur size: %Zi, new size: %Zi\n", msize, newsize);
	if (!buf) {
		assert(msize == 0);
		buf = new tRefBuf(newsize+bufferOversize); // get us a new buffer
	}
	else {
		if (buf->size() < newsize) buf = buf->uniqueWithSize(newsize+bufferOversize);
		else buf = buf->uniqueWithSize(buf->size()); // don't shrink buffer
		
	}
	assert(buf->size() >= newsize);
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
void tMBuf::set(size_t pos) { 
	if(pos > msize) // allow pos == msize for EOF
		throw txOutOfRange(_WHERE("OutOfRange, must be 0<=%Zi<%Zi",pos,msize));
	off=pos;
}
uint8_t tMBuf::getAt(size_t pos) const {
	if(pos >= msize) {
		throw txOutOfRange(_WHERE("OutOfRange %Zi>%Zi",pos,msize));
	}
	return *(buf->buf()+pos);
}
void tMBuf::setAt(size_t pos,const uint8_t what) {
	if(pos >= msize) throw txOutOfRange(_WHERE("OutOfRange %Zi>%Zi",pos,msize));
	getUniqueBuffer(msize);
	*(buf->buf()+pos)=what;
}
void tMBuf::write(const void * val,size_t n) {
	if(n==0) return;
	getUniqueBuffer(off+n); // the buffer will never be shrunken
	memcpy(buf->buf()+off,val,n);
	off+=n;
	if(off>msize) msize=off;
}
const void * tMBuf::read(size_t n) {
	size_t oldpos=off;
	off+=n;
	if(off>msize || buf==NULL) { 
		DBG(8,"off:%Zu,msize:%Zu\n",off,msize);
		off-=n; 
		throw txOutOfRange(_WHERE("OutOfRange %Zi>%Zi",off+n,msize)); 
		//return NULL;
	}
	return buf->buf()+oldpos;
}
void tMBuf::stream(tBBuf &b) const {
	if (buf==NULL) return;
	b.write(buf->buf(),msize);
}
void tMBuf::store(tBBuf&)
{
	throw txBase(_WHERE("storing a tMBuf is not supported"));
}
void tMBuf::clear() {
	off=0;
	msize=0;
	if(buf!=NULL) {
		buf->dec(); // will delete if we are the last
		buf = NULL;
	}
}
int8_t tMBuf::compare(const tMBuf &t) const {
	DBG(9,"tBBuf::compare()\n");
	size_t s1 = size(), s2 = t.size();
	if (!s1 || !s2) {
		// needs special treatment as buf might be NULL
		if (!s1 && !s2) return 0; // both empty
		else if (s1) return 1; // we empty
		else return -1; // the other one empty
	}
	int8_t out = memcmp(buf->buf(), t.buf->buf(), std::min(s1, s2));
	if (out != 0 || s1 == s2) return out;
	return (s1 < s2) ? -1 : 1;
}
void tMBuf::addNullTerminator(void) const
{
	tRefBuf *buf = const_cast<tRefBuf *>(this->buf); // yep, a hack - but we won't change the actual content
	if (msize == buf->size()) buf->resizeLocked(msize+bufferOversize);
	*(buf->buf()+msize) = 0;
}
void tMBuf::cutEnd(size_t newSize)
{
	if (newSize > msize) throw txOutOfRange(_WHERE("cutEnd() can only decrease the size"));
	msize = newSize;
	if (off > newSize) end();
}
/* end tMBuf */

/* 
	File Buffer 
*/
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
size_t tFBuf::tell() const {
	DBG(9,"ftell()\n");
	if(f!=NULL) return ftell(f);
	return 0;
}
void tFBuf::set(size_t pos) {
	if(f==NULL || fseek(f,pos,SEEK_SET)<0) {
		throw txOutOfRange(_WHERE("OutOfRange"));
	}
}
void tFBuf::write(const void * val,size_t n) {
	if(n==0) return;
	if(f==NULL) throw txNoFile(_WHERE("NoFile"));
	if(fwrite(val,n,1,f)!=1) throw txWriteErr(_WHERE("write error"));
}
const void * tFBuf::read(size_t n) {
	if(f==NULL) throw txNoFile(_WHERE("NoFile"));
	if(xsize<n) {
		free(xbuf);
		xbuf=malloc(n);
		if(xbuf==NULL) throw txNoMem(_WHERE("NoMem"));
		xsize=n;
	}
	if (fread(xbuf,n,1,f) != 1) throw txReadErr(_WHERE("error while reading"));
	return xbuf;
}
void tFBuf::stream(tBBuf &b) const {
	// make sure the position in the file is the same after the read
	size_t pos = tell();
	fseek(f,0,SEEK_SET);
	if(xsize<msize) {
		free(xbuf);
		xbuf=malloc(msize);
		if(xbuf==NULL) throw txNoMem(_WHERE("NoMem"));
		xsize=msize;
	}
	if (fread(xbuf,msize,1,f) != 1) throw txReadErr(_WHERE("error while reading"));
	b.write(xbuf,msize);
	fseek(f,pos,SEEK_SET);
}
void tFBuf::store(tBBuf&)
{
	throw txBase(_WHERE("storing a tFBuf is not supported"));
}
size_t tFBuf::size() const {
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
/* end File buffer */

/* static buffer */
tSBuf::tSBuf(const void * buf,size_t msize) {
	off=0;
	this->msize=msize;
	this->buf=buf;
}
void tSBuf::set(size_t pos) {
	if (pos >= msize) throw txOutOfRange(_WHERE("Cannot access pos %Zi, size %Zi\n",pos,msize));
	off=pos; 
}
const void * tSBuf::read(size_t n) {
	size_t oldpos=off;
	off+=n;
	if(off>msize) { 
		off-=n; 
		throw txOutOfRange(_WHERE("OutOfRange %Zi>%Zi",off+n,msize)); 
	}
	return static_cast<const uint8_t*>(buf)+oldpos;
}
void tSBuf::stream(tBBuf &buf) const {
	buf.write(this->buf,msize);
}
void tSBuf::store(tBBuf&)
{
	throw txBase(_WHERE("storing a tSBuf is not supported"));
}
void tSBuf::write(const void*, size_t)
{
	throw txBase(_WHERE("writing to a tSBuf is not supported"));
}

/* end static buffer */

/* tZBuf */
void tZBuf::compress() {
	zlib::uLongf comp_size = zlib::compressBound(size());
	tZBuf newBuf(comp_size);
	int ret=zlib::compress(static_cast<zlib::Bytef*>(newBuf.volatileData()),&comp_size,
						   static_cast<const zlib::Bytef*>(data()),size());
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
void tZBuf::uncompress(size_t iosize) {
	zlib::uLongf comp_size=iosize;
	tZBuf newBuf(comp_size);
	int ret=zlib::uncompress(static_cast<zlib::Bytef*>(newBuf.volatileData()),&comp_size,
							 static_cast<const zlib::Bytef*>(data()),size());
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
		throw txUnexpectedData(_WHERE("tZBuf size mismatch: %Zi != %Zi", iosize, comp_size));
	copy(newBuf);
	rewind();
}
/* end tZBuf */

/* md5 buff */
void tMD5Buf::compute() {
	tMD5Buf newBuf(0x10);
	md5::MD5(static_cast<const unsigned char *>(data()),size(),static_cast<unsigned char *>(newBuf.volatileData()));
	copy(newBuf);
	rewind();
}
/* end md5 buf */

/* String buffer */
void tString::store(tBBuf &t)
{
	clear();
	uint16_t bufSize = t.get16();
	if (bufSize & 0xF000) throw txUnexpectedData(_WHERE("This is probably an inverted string!"));
	if (bufSize)
		write(t.read(bufSize), bufSize);
}
void tString::stream(tBBuf &t) const
{
	if (size() >= 0xF000) throw txUnexpectedData(_WHERE("Uru string is too long"));
	t.put16(size());
	tMBuf::stream(t); // just puts the bytes into the buffer
}
int8_t tString::compare(const char * str) const {
	size_t s1 = size(), s2 = strlen(str);
	if (!s1 || !s2) {
		// needs special treatment as buf might be NULL
		if (!s1 && !s2) return 0; // both empty
		else if (s1) return 1; // we empty
		else return -1; // the other one empty
	}
	int8_t out = memcmp(data(), str, std::min(s1, s2));
	if (out != 0 || s1 == s2) return out;
	return (s1 < s2) ? -1 : 1;
}
const char * tString::c_str() const {
	if (!size()) return "";
	addNullTerminator();
	return reinterpret_cast<const char *>(data());
}
size_t tString::find(const char cat, bool reverse) const {
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
	return npos;
}
size_t tString::find(const char *str) const {
	const uint8_t *c = reinterpret_cast<const uint8_t *>(strstr(c_str(), str));
	if (c == NULL) return npos;
	return (c-data());
}
tString tString::stripped(char what,uint8_t how) const {
	tString aux;
	size_t i,max,start,end;
	start=0;
	i=0;
	max=size();
	end=max-1;
	uint8_t ctrl;
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
	uint8_t ctrl; 
	max=size();
	for(i=0; i<max; i++) {
		ctrl=getAt(i);
		if(ctrl=='\n') {
			shot.putChar('\\');
			shot.putChar('n');
		} else if(ctrl=='\r') {
			shot.putChar('\\');
			shot.putChar('r');
		} else if(ctrl=='"') {
			shot.putChar('\\');
			shot.putChar('"');
		} else if(ctrl=='\\') {
			shot.putChar('\\');
			shot.putChar('\\');
		} else {
			shot.putChar(ctrl);
		}
	}
	return shot;
}
tString tString::lower() const {
	tString shot(size());
	for(size_t i=0; i<size(); i++) {
		*(shot.volatileData()+i) = std::tolower(*(data()+i));
	}
	return shot;
}
tString tString::upper() const {
	tString shot(size());
	for(size_t i=0; i<size(); i++) {
		*(shot.volatileData()+i) = std::toupper(*(data()+i));
	}
	return shot;
}
tString tString::substring(size_t start,size_t len) const {
	if (len == 0) len = size()-start;
	else if (start+len > size()) throw txOutOfRange(_WHERE("Reading %Zd Bytes from position %Zd on - but there are just %Zd Bytes left", len, start, size()-start));
	tString shot;
	shot.write(data()+start,len);
	return shot;
}
bool tString::startsWith(const char * pat) const {
	size_t len = strlen(pat);
	if (len == 0) return true; // substring() treats a 0 length special
	if (len > size()) return false;
	return(substring(0,len)==pat);
}
bool tString::endsWith(const char * pat) const {
	size_t len = strlen(pat);
	if (len == 0) return true; // substring() treats a 0 length special
	if (len > size()) return false;
	return(substring(size()-len,len)==pat);
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
int tString::asInt() const {
	return atoi(c_str());
}

tString tString::fromUInt(unsigned int val)
{
	tString str;
	str.printf("%d", val);
	return str;
}


tTime::tTime(double seconds) : seconds(seconds) // round down
{
	microseconds = (seconds - this->seconds)*1000*1000; // difference times 10^6 = usecs
}
void tTime::store(tBBuf &t) {
	seconds=t.get32();
	microseconds=t.get32();
}
void tTime::stream(tBBuf &t) const {
	t.put32(seconds);
	t.put32(microseconds);
}
int8_t tTime::compare(const tTime &t) const {
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
void tTime::setToNow()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	seconds=tv.tv_sec;
	microseconds=tv.tv_usec;
}
tString tTime::str(bool relative) const {
	if(!relative) {
		char tmptime[32];
		struct tm * tptr;

		tptr=gmtime(&seconds);
		strftime(tmptime,30,"%Y:%m:%d-%H:%M:%S",tptr);
		tString str = tmptime;
		str.printf(".%06d", microseconds);
		return str;
	} else {
		time_t minutes=seconds/60;
		time_t hours=minutes/60;
		minutes %= 60;

		time_t days=hours/24;
		hours %= 24;

		time_t weeks=days/7;
		days %= 7;

		tString sth;
		if(weeks==1) sth.printf("1 week, ");
		else if(weeks>1) sth.printf("%li weeks, ",weeks);
		if(days==1) sth.printf("1 day, ");
		else if(days>1) sth.printf("%li days, ",days);
		if(hours==1) sth.printf("1 hour, ");
		else if(hours>1) sth.printf("%li hours, ",hours);
		if(minutes==1) sth.printf("1 minute, ");
		else if(minutes>1) sth.printf("%li minutes, ",minutes);
		sth.printf("%.6f seconds.",seconds%60 + microseconds/1000000.0);
		return sth;
	}
}

} //end namespace alc
