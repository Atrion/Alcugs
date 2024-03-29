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

#ifndef __U_ALCTYPES_H
#define __U_ALCTYPES_H

#include "alcutil/useful.h"

#include <cstdio>
#include <cstring>

namespace alc {

// string/character not found
const size_t npos = static_cast<size_t>(-1);

//Forward
class tBBuf;
class tMBuf;

/** Base type */
class tBaseType {
public: virtual ~tBaseType() {}
};

/** Put any type into a base type */
template <typename T> class tContainer : public tBaseType {
public:
	tContainer(T data) : data(data) {}
	T getData() { return data; }
private:
	T data;
};

/** Basic streamable type */
class tStreamable : public tBaseType {
public:
	//!stores from a buffer
	virtual void store(tBBuf &t)=0;
	//!streams to a buffer
	virtual void stream(tBBuf &t) const=0;
};

/** Basic buffer */
class tBBuf : public tStreamable {
public:
	//You must implement the next ones on derived buffer classes, plus the tBaseType ones.
	//! Gets absolute offset
	virtual size_t tell() const=0;
	//! Sets absolute offset
	virtual void set(size_t pos)=0;
	//! Write a buffer of size n
	virtual void write(const void * val,size_t n)=0;
	//! Reads n bytes, if n=0 reads the entire buffer
	virtual const void * read(size_t n)=0;
	const void *readAll(void) { return read(remaining()); } //!< reads all remaining Bytes
	//!gets buffer size
	virtual size_t size() const=0;
	
	// convenience functions
	//! Sets offset to 0
	void rewind() { set(0); }
	//! Sets offset ot End of the stream
	void end() { set(size()); }
	/** Relative seek
			\param n pos
			\param flags SEEK_CUR (default), SEEK_SET, SEEK_END
			\throws txOutOfRange
	*/
	void seek(ssize_t n,uint8_t flags=SEEK_CUR);
	
	/** Puts an object into the buffer (streams the object) */
	void put(const tStreamable &t) { t.stream(*this); }
	/** Gets an object from the buffer (stores object from stream) */
	void get(tStreamable &t) { t.store(*this); }
	
	/** \return True if the pointer is at the end of the stream */
	bool eof() const { return (this->tell() >= this->size()); }
	/** \return the number of bytes remaining to the end of the stream */
	size_t remaining() const { return (this->size() - this->tell()); }
	
	// Overlaoded Operators
	void operator++(int) { this->seek(+1); }
	void operator++() { this->seek(+1); }
	void operator--(int) { this->seek(-1); }
	void operator--() { this->seek(-1); }
	void operator+=(ssize_t n) { this->seek(+n); }
	void operator-=(ssize_t n) { this->seek(-n); }
	
	// put and get functions
	//NOTE: I have already thought about overloading a "put(U16 val)", and I don't want
	// it, mainly because it can be a little confusing when you are reading the code.
	void put8(uint8_t val) {
		this->write(&val,1);
	}
	void put16(uint16_t val) {
		val = htole16(val);
		this->write(&val,2);
	}
	void put32(uint32_t val) {
		val = htole32(val);
		this->write(&val,4);
	}
	void putFloat(float val);
	void putDouble(double val);
	
	uint8_t get8() {
		return *static_cast<const uint8_t*>(this->read(1));
	}
	uint16_t get16() {
		uint16_t val;
		memcpy(&val, this->read(2), 2);
		return(letoh16(val));
	}
	uint32_t get32() {
		uint32_t val;
		memcpy(&val, this->read(4), 4);
		return(letoh32(val));
	}
	float getFloat();
	double getDouble();
	
	// useful functions
	void check(const void * what,size_t n); //!< \throws txUnexpectedData if pattern don't matches buffer contents
};

/** memory based buffer with reference control */
class tMBuf : public tBBuf {
public:
	tMBuf();
	tMBuf(const tMBuf &t);
	tMBuf(const void *d, size_t s);
	explicit tMBuf(tBBuf &t);
	virtual ~tMBuf();
	
	// implement interface
	virtual void stream(tBBuf &buf) const;
	virtual void store(tBBuf &buf);
	virtual size_t tell() const { return off; }
	virtual void set(size_t pos);
	virtual void write(const void* val,size_t n);
	virtual const void * read(size_t n);
	virtual size_t size() const { return msize; }
	
	// assignment
	const tMBuf &operator=(const tMBuf &t) { this->copy(t); return *this; }
	
	// comparison
	bool operator==(const tMBuf &t) { return(!this->compare(t)); }
	bool operator!=(const tMBuf &t) { return(this->compare(t)); }
	bool operator>(const tMBuf &t) { return(this->compare(t)>0); }
	bool operator<(const tMBuf &t) { return(this->compare(t)<0); }
	bool operator>=(const tMBuf &t) { return(this->compare(t)>=0); }
	bool operator<=(const tMBuf &t) { return(this->compare(t)<=0); }
	
	// useful functions
	void clear();
	uint8_t getAt(size_t pos) const;
	void setAt(size_t pos,const uint8_t what);
	const uint8_t *data() const { return msize ? buf->buf() : NULL; }
	bool isEmpty(void) const { return !msize; }
	void cutEnd(size_t newSize); //!< reduces the size of the buffer to newSize by cutting of the last bytes
protected:
	//! creates a buffer of the given size, with undefined content
	explicit tMBuf(size_t size);
	//! write-access to the buffer for sublcasses. Use with caution!
	uint8_t *volatileData() { return msize ? buf->buf() : NULL; }
	//! assignment
	void copy(const tMBuf &t);
	//! comparison
	int compare(const tMBuf &t) const;
	//! helper for tString::c_str - use only if msize is at least 1!
	void addNullTerminator(void) const;
private:
	/** Buffer with reference control */
	class tRefBuf {
	public:
		tRefBuf(size_t csize=1024);
		~tRefBuf();
		void inc();
		void dec();
		size_t size() { return msize; }
		uint8_t *buf() { return static_cast<uint8_t*>(buffer); }
		const uint8_t *buf() const { return static_cast<const uint8_t*>(buffer); }
		tRefBuf *uniqueWithSize(size_t newsize); //!< returns a pointer to a ref buf with the same conent as this one, but not shared with anyone else. The parameter says how large we need the buffer (doing a resize of necessary)
	private:
		void resize(size_t newsize);
		
		unsigned int refs;
		size_t msize;
		void *buffer;
	};
	
	void init();
	void getUniqueBuffer(size_t newsize);
	
	// data
	tRefBuf *buf;
	size_t off;
	size_t msize; //!< this is the part of the buffer that is actually used, while buf->size() is the currently available size
};

/** File buffer */
class tFBuf : public tBBuf {
public:
	tFBuf() { init(); }
	explicit tFBuf(const char *file,const char * mode="rb");
	virtual ~tFBuf();
	
	// implement interface
	virtual size_t tell() const;
	virtual void set(size_t pos);
	virtual void write(const void * val,size_t n);
	virtual const void * read(size_t n);
	virtual void stream(tBBuf &buf) const;
	virtual void store(tBBuf &buf);
	virtual size_t size() const;
	
	// additional functions
	void close();
	void open(const char * path,const char * mode="rb");
	void flush() { if (f != NULL) fflush(f); }
private:
	void init();
	FILE * f;
	size_t msize;
	mutable void *xbuf;
	mutable size_t xsize; // size of the xbuf

	FORBID_CLASS_COPY(tFBuf)
};

/** Static buffer, operatoring on external data */
class tSBuf : public tBBuf {
public:
	explicit tSBuf(const void * buf,size_t msize);
	
	// implement interface
	virtual size_t tell() const { return off; }
	virtual void set(size_t pos);
	virtual void write(const void * val,size_t n);
	virtual const void * read(size_t n);
	virtual void stream(tBBuf &buf) const;
	virtual void store(tBBuf &buf);
	virtual size_t size() const { return msize; }
private:
	const void * buf;
	size_t off;
	size_t msize;
};

/** Zlib buffer */
class tZBuf : public tMBuf {
public:
	tZBuf() : tMBuf() {}
	tZBuf(const tMBuf &k) : tMBuf(k) {}
	void compress();
	void uncompress(size_t iosize);
protected:
	tZBuf(size_t size) : tMBuf(size) {}
};

/** Md5 buffer */
class tMD5Buf : public tMBuf {
public:
	tMD5Buf() :tMBuf() {}
	tMD5Buf(tMBuf &k) :tMBuf(k) { this->compute(); }
	void compute();
protected:
	tMD5Buf(size_t size) : tMBuf(size) {}
};

/** String buffer */
class tString : public tMBuf {
public:
	tString() : tMBuf() {}
	tString(const char * k) : tMBuf() { writeStr(k); }
	explicit tString(tBBuf &k) : tMBuf(k) {}
	explicit tString(const tMBuf &k) : tMBuf(k) {}
	virtual ~tString() {}
	
	// interface
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	
	// find functions
	size_t find(const char cat, bool reverse=false) const;
	size_t find(const char *str) const;
	size_t find(const tString &str) const { return find(str.c_str()); } // find is using strstr, so we definitely need a c_str
	
	// string manipulation
	tString escape() const;
	tString lower() const;
	tString upper() const;
	tString substring(size_t start,size_t len=0) const;
	tString dirname() const;
	tString filename() const;
	/** \brief strips the character from the beginning (when how=1 or 3) and/or from the end (how=2 or 3) of the string */
	tString stripped(char what,uint8_t how=0x03) const;
	
	// functions writing strings and characters
	void writeStr(const char * t);
	void writeStr(const tString & val) { write(val.data(),val.size()); }
	void printf(const char * msg, ...) GNUC_FORMAT_CHECK(printf, 2, 3);
	void vprintf(const char * msg, va_list ap);
	void printBoolean(const char *desc, bool val) { writeStr(desc); printBoolean(val); }
	void printBoolean(bool val);
	void nl() { writeStr("\n"); }
	void putChar(char c) { put8(static_cast<uint8_t>(c)); }
	char getChar(void) { return static_cast<char>(get8()); }
	
	// functions to convert the string to a number
	int asInt() const;
	
	// Functions checking something
	bool isNewline(void) const
	{
		return compare("\n") == 0 || compare("\r") == 0 || compare("\n\r") == 0 || compare("\r\n") == 0;
	}
	bool startsWith(const char * pat) const;
	bool endsWith(const char * pat) const;
	
	// C compatbility
	const char * c_str() const;
	
	// assignment
	const tString & operator=(const tString &t) { copy(t); return *this; }
	const tString & operator=(const char * str) { copy(str); return *this; }

	// operator +=
	const tString &operator+=(const tString &t) { end(); writeStr(t); return *this; }
	const tString &operator+=(const char *str) { end(); writeStr(str); return *this; }
	
	// comparison
	bool operator==(const tString &t) const { return(!compare(t)); }
	bool operator==(const char * str) const { return(!compare(str)); }
	bool operator!=(const tString &t) const { return(compare(t)); }
	bool operator!=(const char * str) const { return(compare(str)); }
	bool operator>(const tString &t) const { return(compare(t)>0); }
	bool operator>(const char *t) const { return(compare(t)>0); }
	bool operator<(const tString &t) const { return(compare(t)<0); }
	bool operator<(const char *t) const { return(compare(t)<0); }
	bool operator>=(const tString &t) const { return(compare(t)>=0); }
	bool operator>=(const char *t) const { return(compare(t)>=0); }
	bool operator<=(const tString &t) const { return(compare(t)<=0); }
	bool operator<=(const char *t) const { return(compare(t)<=0); }

	// static creation functions
	static tString fromUInt(unsigned int val);
protected:
	//! assignment
	void copy(const char * str) { tMBuf::copy(tString(str)); }
	void copy(const tString &t) { tMBuf::copy(t); }
	//! creates a buffer of the given size, with undefined content
	explicit tString(size_t size) : tMBuf(size) {}
private:
	//! comparison
	int compare(const tString &t) const { return tMBuf::compare(t); }
	int compare(const char * str) const;
};

inline tString operator+(const tString & str1, const tString & str2) {
	tString out(str1);
	out.writeStr(str2);
	return out;
}
inline tString operator+(const char * str1, const tString & str2) {
	tString out(str1);
	out.writeStr(str2);
	return out;
}
inline tString operator+(const tString & str1, const char * str2) {
	tString out(str1);
	out.writeStr(str2);
	return out;
}

/** Time */
class tTime : public tStreamable {
public:
	tTime(void) : seconds(0), microseconds(0) {}
	tTime(time_t seconds) : seconds(seconds), microseconds(0) {}
	tTime(uint32_t seconds) : seconds(seconds), microseconds(0) {}
	tTime(time_t seconds, unsigned int microseconds) : seconds(seconds), microseconds(microseconds) {}
	tTime(double seconds);
	static tTime now() {
		tTime t;
		t.setToNow();
		return t;
	}
	
	// Interface
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	
	// Comparison
	bool operator==(const tTime &t) const { return(seconds==t.seconds && microseconds==t.microseconds); }
	bool operator!=(const tTime &t) const { return(seconds!=t.seconds || microseconds!=t.microseconds); }
	bool operator>(const tTime &t) const { return(this->compare(t)>0); }
	bool operator<(const tTime &t) const { return(this->compare(t)<0); }
	bool operator>=(const tTime &t) const { return(this->compare(t)>=0); }
	bool operator<=(const tTime &t) const { return(this->compare(t)<=0); }
	
	// assignment
	const tTime &operator=(const tTime &t) {
		seconds=t.seconds;
		microseconds=t.microseconds;
		return *this;
	}
	
	// convenience functions
	void setToNow();
	double asDouble() const { return static_cast<double>(seconds) + (static_cast<double>(microseconds) / 1000000.0); }
	tString str(bool relative = false) const;
	bool isNull() const { return seconds == 0 && microseconds == 0; }
	
	// data
	time_t seconds;
	unsigned int microseconds;
private:
	//! Comparison
	int8_t compare(const tTime &t) const;
};

tTime operator+ (const tTime &a,const tTime &b);
tTime operator- (const tTime &a,const tTime &b);

} //End alc namespace

#endif
