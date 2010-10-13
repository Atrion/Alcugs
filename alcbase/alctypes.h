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
/* CVS tag - DON'T TOUCH*/
#define __U_ALCTYPES_H_ID "$Id$"

namespace alc {

//basic types
typedef unsigned char Byte;
typedef char SByte;
typedef unsigned int U32;
typedef int S32;
typedef unsigned short int U16;
typedef short int S16;
//end basic types

//Forward
class tBBuf;
class tMBuf;

/** Base type */
class tBaseType {
public:
	//!(de)constructors
	inline virtual ~tBaseType() {} //GCC 4 wants this?
	//!stores from a buffer
	virtual void store(tBBuf &t)=0;
	//!streams to a buffer
	virtual void stream(tBBuf &t) const=0;
};
//end base type

/** Basic buffer */
class tBBuf : public tBaseType {
public:
	//!(de)constructors
	inline tBBuf() {}
	inline virtual ~tBBuf() {}
	
	//You must implement the next ones on derived buffer classes, plus the tBaseType ones.
	//! Gets absolute offset
	virtual U32 tell() const=0;
	//! Sets absolute offset
	virtual void set(U32 pos)=0;
	//! Write a buffer of size n
	virtual void write(const Byte * val,U32 n)=0;
	inline virtual void write(const SByte * val,U32 n) { this->write(reinterpret_cast<const Byte *>(val),n); }
	//! Reads n bytes, if n=0 reads the entire buffer
	virtual const Byte * read(U32 n)=0;
	inline const Byte *readAll(void) { return read(remaining()); } //!< reads all remaining Bytes
	//!gets buffer size
	virtual U32 size() const=0;
	
	// convenience functions
	//! Sets offset to 0
	inline void rewind() { set(0); }
	//! Sets offset ot End of the stream
	inline void end() { set(size()); }
	/** Relative seek
			\param n pos
			\param flags SEEK_CUR (default), SEEK_SET, SEEK_END
			\throws txOutOfRange
	*/
	void seek(int n,Byte flags=SEEK_CUR);
	
	/** Puts an object into the buffer (streams the object) */
	inline void put(const tBaseType &t) { t.stream(*this); }
	/** Gets an object from the buffer (stores object from stream) */
	inline void get(tBaseType &t) { t.store(*this); }
	
	/** \return True if the pointer is at the end of the stream */
	inline bool eof() const { return (this->tell() >= this->size()); }
	/** \return the number of bytes remaining to the end of the stream */
	inline U32 remaining() const { return (this->size() - this->tell()); }
	
	// Overlaoded Operators
	inline void operator++(int) { this->seek(+1); }
	inline void operator++() { this->seek(+1); }
	inline void operator--(int) { this->seek(-1); }
	inline void operator--() { this->seek(-1); }
	inline void operator+=(U32 n) { this->seek(+n); }
	inline void operator-=(U32 n) { this->seek(-n); }
	
	// put and get functions
	//NOTE: I have already thought about overloading a "put(U16 val)", and I don't want
	// it, mainly because it can be a little confusing when you are reading the code.
	void putU16(U16 val);
	void putS16(S16 val);
	void putU32(U32 val);
	void putS32(S32 val);
	void putByte(Byte val);
	void putSByte(SByte val);
	void putDouble(double val);
	void putFloat(float val);
	U16 getU16();
	S16 getS16();
	U32 getU32();
	S32 getS32();
	Byte getByte();
	SByte getSByte();
	double getDouble();
	float getFloat();
	
	// useful functions
	void check(const Byte * what,U32 n); //!< \throws txUnexpectedData if pattern don't matches buffer contents
	inline void check(const SByte * what,U32 n) {
		check(reinterpret_cast<const Byte *>(what), n);
	}
};

/** Buffer with reference control */
class tRefBuf {
public:
	tRefBuf(U32 csize=1024);
	~tRefBuf();
	void resize(U32 newsize);
	void inc();
	void dec();
	inline U32 size() { return msize; }
	inline U32 getRefs() { return refs; }
	inline Byte *buf() { return buffer; }
	inline const Byte *buf() const { return buffer; }
private:
	U32 refs;
	U32 msize;
	Byte *buffer;
};

/** memory based buffer with reference control */
class tMBuf : public tBBuf {
public:
	tMBuf();
	tMBuf(const tMBuf &t);
	tMBuf(const Byte *d, U32 s);
	explicit tMBuf(tBBuf &t);
	virtual ~tMBuf();
	
	// implement interface
	virtual void stream(tBBuf &buf) const;
	virtual void store(tBBuf &/*buf*/) {}
	inline virtual U32 tell() const { return off; }
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n);
	inline virtual void write(const SByte * val,U32 n) { this->write(reinterpret_cast<const Byte *>(val),n); }
	virtual const Byte * read(U32 n);
	inline virtual U32 size() const { return msize; }
	
	// assignment
	inline const tMBuf &operator=(const tMBuf &t) { this->copy(t); return *this; }
	
	// comparison
	inline bool operator==(const tMBuf &t) { return(!this->compare(t)); }
	inline bool operator!=(const tMBuf &t) { return(this->compare(t)); }
	inline bool operator>(const tMBuf &t) { return(this->compare(t)>0); }
	inline bool operator<(const tMBuf &t) { return(this->compare(t)<0); }
	inline bool operator>=(const tMBuf &t) { return(this->compare(t)>=0); }
	inline bool operator<=(const tMBuf &t) { return(this->compare(t)<=0); }
	
	// useful functions
	void clear();
	Byte getAt(U32 pos) const;
	void setAt(U32 pos,const Byte what);
	inline const Byte *data() const { return msize ? buf->buf() : NULL; }
	inline bool isEmpty(void) const { return !msize; }
	void cutEnd(U32 newSize); //!< reduces the size of the buffer to newSize by cutting of the last bytes
protected:
	//! creates a buffer of the given size, with undefined content
	explicit tMBuf(U32 size);
	//! write-access to the buffer for sublcasses. Use with caution!
	inline Byte *volatileData() { return msize ? buf->buf() : NULL; }
	//! assignment
	void copy(const tMBuf &t);
	//! comparison
	SByte compare(const tMBuf &t) const;
	//! helper for tString::c_str - use only if msize is at least 1!
	void addNullTerminator(void) const;
private:
	void init();
	void getUniqueBuffer(U32 newsize);
	
	// data
	tRefBuf *buf;
	U32 off;
	U32 msize; //!< this is the part of the buffer that is actually used, while buf->size() is the currently available size
};

/** File buffer */
class tFBuf : public tBBuf {
public:
	tFBuf();
	explicit tFBuf(const char *file,const char * mode="rb");
	virtual ~tFBuf();
	
	// implement interface
	virtual U32 tell() const;
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n);
	inline virtual void write(const SByte * val,U32 n) { this->write(reinterpret_cast<const Byte *>(val),n); }
	virtual const Byte * read(U32 n);
	virtual void stream(tBBuf &buf) const;
	virtual void store(tBBuf &/*buf*/) {}
	virtual U32 size() const;
	
	// additional functions
	void close();
	void open(const char * path,const char * mode="rb");
	void flush();
private:
	void init();
	FILE * f;
	U32 msize;
	mutable Byte *xbuf;
	mutable U32 xsize; // size of the xbuf

	FORBID_CLASS_COPY(tFBuf)
};

/** Static buffer, operatoring on external data */
class tSBuf : public tBBuf {
public:
	explicit tSBuf(const Byte * buf,U32 msize);
	
	// implement interface
	inline virtual U32 tell() const { return off; }
	virtual void set(U32 pos);
	virtual void write(const Byte * /*val*/,U32 /*n*/) {}
	inline virtual void write(const SByte * /*val*/,U32 /*n*/) { }
	virtual const Byte * read(U32 n);
	virtual void stream(tBBuf &buf) const;
	virtual void store(tBBuf &/*buf*/) {}
	inline virtual U32 size() const { return msize; }
private:
	const Byte * buf;
	U32 off;
	U32 msize;
};

/** Zlib buffer */
class tZBuf : public tMBuf {
public:
	tZBuf() :tMBuf() {}
	tZBuf(const tMBuf &k) :tMBuf(k) {}
	void compress();
	void uncompress(U32 iosize);
protected:
	tZBuf(U32 size) : tMBuf(size) {}
};

/** Md5 buffer */
class tMD5Buf : public tMBuf {
public:
	tMD5Buf() :tMBuf() {}
	tMD5Buf(tMBuf &k) :tMBuf(k) { this->compute(); }
	void compute();
protected:
	tMD5Buf(U32 size) : tMBuf(size) {}
};

/** String buffer */
class tString : public tMBuf {
public:
	inline tString() : tMBuf() {}
	inline tString(const char * k) : tMBuf() { writeStr(k); }
	inline explicit tString(tBBuf &k) : tMBuf(k) {}
	inline explicit tString(const tMBuf &k) : tMBuf(k) {}
	inline virtual ~tString() {}
	
	// interface
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	
	// find functions
	S32 find(const char cat, bool reverse=false) const;
	S32 find(const char *str) const;
	inline S32 find(const tString &str) const { return find(str.c_str()); }
	
	// string manipulation
	tString escape() const;
	tString lower() const;
	tString upper() const;
	tString substring(U32 start,U32 len=0) const;
	tString dirname() const;
	tString filename() const;
	/** \brief strips the character from the beginning (when how=1 or 3) and/or from the end (how=2 or 3) of the string */
	tString stripped(Byte what,Byte how=0x03) const;
	
	// functions writing strings
	void writeStr(const char * t);
	void writeStr(const tString & val) { write(val.c_str(),val.size()); }
	void printf(const char * msg, ...);
	void vprintf(const char * msg, va_list ap);
	void printBoolean(const char *desc, bool val) { writeStr(desc); printBoolean(val); }
	void printBoolean(bool val);
	void nl() { writeStr("\n"); }
	
	// functions to convert the string to a number
	U32 asU32() const;
	inline S32 asS32() const { return static_cast<S32>(asU32()); }
	inline U16 asU16() const { return static_cast<U16>(asU32()); }
	inline S16 asS16() const { return static_cast<S16>(asU32()); }
	inline Byte asByte() const { return static_cast<Byte>(asU32()); }
	inline SByte asSByte() const { return static_cast<SByte>(asU32()); }
	
	// Functions checking something
	inline bool isNewline(void) const
	{
		return compare("\n") == 0 || compare("\r") == 0 || compare("\n\r") == 0 || compare("\r\n") == 0;
	}
	bool startsWith(const char * pat) const;
	bool endsWith(const char * pat) const;
	
	// C compatbility
	const char * c_str() const;
	
	// path functions
	void convertSlashesFromWinToUnix();
	void convertSlashesFromUnixToWin();
	
	// assignment
	inline const tString & operator=(const tString &t) { copy(t); return *this; }
	inline const tString & operator=(const char * str) { copy(str); return *this; }

	// operator +=
	inline const tString &operator+=(const tString &t) { end(); writeStr(t); return *this; }
	inline const tString &operator+=(const char *str) { end(); writeStr(str); return *this; }
	
	// comparison
	inline bool operator==(const tString &t) const { return(!compare(t)); }
	inline bool operator==(const char * str) const { return(!compare(str)); }
	inline bool operator!=(const tString &t) const { return(compare(t)); }
	inline bool operator!=(const char * str) const { return(compare(str)); }
	inline bool operator>(const tString &t) const { return(compare(t)>0); }
	inline bool operator>(const char *t) const { return(compare(t)>0); }
	inline bool operator<(const tString &t) const { return(compare(t)<0); }
	inline bool operator<(const char *t) const { return(compare(t)<0); }
	inline bool operator>=(const tString &t) const { return(compare(t)>=0); }
	inline bool operator>=(const char *t) const { return(compare(t)>=0); }
	inline bool operator<=(const tString &t) const { return(compare(t)<=0); }
	inline bool operator<=(const char *t) const { return(compare(t)<=0); }

	// static creation functions
	static tString fromByte(Byte val);
protected:
	//! assignment
	inline void copy(const char * str) { tMBuf::copy(tString(str)); }
	inline void copy(const tString &t) { tMBuf::copy(t); }
	//! creates a buffer of the given size, with undefined content
	inline explicit tString(U32 size) : tMBuf(size) {}
private:
	//! comparison
	inline SByte compare(const tString &t) const { return tMBuf::compare(t); }
	SByte compare(const char * str) const;
};

tString operator+(const tString & str1, const tString & str2);
tString operator+(const char * str1, const tString & str2);
tString operator+(const tString & str1, const char * str2);

/** Time */
class tTime : public tBaseType {
public:
	tTime(void) : tBaseType() { seconds = microseconds = 0; }
	
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
	inline const tTime &operator=(const tTime &t) {
		seconds=t.seconds;
		microseconds=t.microseconds;
		return *this;
	}
	
	// convenience functions
	void setToNow();
	double asDouble(char how='s') const;
	U32 asU32(char how='s') const;
	tString str(Byte type=0x00) const;
	
	// data
	U32 seconds;
	U32 microseconds;
private:
	//! Comparison
	SByte compare(const tTime &t) const;
};

tTime operator+ (const tTime &a,const tTime &b);
tTime operator- (const tTime &a,const tTime &b);


} //End alc namespace

#endif