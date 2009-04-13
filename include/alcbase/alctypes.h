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
	virtual ~tBaseType() {} //GCC 4 wants this?
	//!stores from a buffer
	virtual void store(tBBuf &t)=0;
	//!streams to a buffer
	virtual void stream(tBBuf &t)=0;
};
//end base type

/** Basic buffer */
class tBBuf :public tBaseType {
public:
	//You must implement the next ones on derived buffer classes, plus the tBaseType ones.
	//!(de)constructors
	tBBuf();
	virtual ~tBBuf();
	//! Gets absolute offset
	virtual U32 tell() const=0;
	//! Sets absolute offset
	virtual void set(U32 pos)=0;
	//! Write a buffer of size n
	virtual void write(const Byte * val,U32 n)=0;
	inline virtual void write(const SByte * val,U32 n) { this->write((Byte *)val,n); }
	//! Reads n bytes, if n=0 reads the entire buffer
	virtual const Byte * read(U32 n=0)=0;
	//!gets buffer size
	virtual U32 size() const=0;
	
	//! Sets offset to 0
	virtual void rewind();
	//! Sets offset ot End of the stream
	virtual void end();
	/** Relative seek
			\param n pos
			\param flags SEEK_CUR (default), SEEK_SET, SEEK_END
			\throws txOutOfRange
	*/
	virtual void seek(int n,Byte flags=SEEK_CUR);
	/** Puts an object into the buffer (streams the object) \return the amount of written bytes */
	inline void put(tBaseType &t) {
		t.stream(*this);
	}
	/** Gets an object from the buffer (stores object from stream) */
	inline void get(tBaseType &t) {
		t.store(*this);
	}
	
	/** \return True if the pointer is at the end of the stream */
	virtual bool eof() const {
		return(this->tell()>=this->size());
	}
	/** \return the number of bytes remaining to the end of the stream */
	virtual int remaining() const {
		return (this->size()-this->tell());
	}
	
	// Overlaoded Operators
	virtual void operator++(int) { this->seek(+1); }
	virtual void operator++() { this->seek(+1); }
	virtual void operator--(int) { this->seek(-1); }
	virtual void operator--() { this->seek(-1); }
	virtual void operator+=(U32 n) { this->seek(+n); }
	virtual void operator-=(U32 n) { this->seek(-n); }
	
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
	void check(const char * what,U32 n) {
		this->check((Byte *)what,n);
	}
private:
	//! Built-in initialization
	virtual void init();
};

/** Buffer with reference control */
class tRefBuf {
public:
	tRefBuf(U32 csize=1024);
	~tRefBuf();
	void resize(U32 newsize);
	void inc();
	void dec();
	U32 size();
	U32 getRefs() { return refs; }
	void zero() {
		memset(buf,0,msize);
	}
	
	Byte * buf;
private:
	U32 refs;
	U32 msize;
};

/** memory based buffer */
class tMBuf :public tBBuf {
public:
	tMBuf();
	tMBuf(const tMBuf &t);
	tMBuf(tBBuf &t);
	explicit tMBuf(U32 size);
	virtual ~tMBuf();
	
	// implement interface
	virtual void stream(tBBuf &buf);
	virtual void store(tBBuf &buf) {}
	virtual U32 tell() const;
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n);
	inline virtual void write(const SByte * val,U32 n) { this->write((Byte *)val,n); }
	virtual const Byte * read(U32 n=0);
	virtual U32 size() const;
	
	// assignment
	virtual const tMBuf &operator=(const tMBuf &t) { this->copy(t); return *this; }
	
	// comparison
	virtual bool operator==(const tMBuf &t) { return(!this->compare(t)); }
	virtual bool operator!=(const tMBuf &t) { return(this->compare(t)); }
	virtual bool operator>(const tMBuf &t) { return(this->compare(t)>0); }
	virtual bool operator<(const tMBuf &t) { return(this->compare(t)<0); }
	virtual bool operator>=(const tMBuf &t) { return(this->compare(t)>=0); }
	virtual bool operator<=(const tMBuf &t) { return(this->compare(t)<=0); }
	
	// useful functions
	void clear();
	Byte getAt(U32 pos) const;
	void setAt(U32 pos,const Byte what);
	void setSize(U32 size) {
		msize=size;
	}
protected:
	//! assignment
	virtual void copy(const tMBuf &t);
	//! called when content is modified
	virtual void onmodify();
	//! comparison
	virtual SByte compare(const tMBuf &t) const;
	
	// data
	tRefBuf * buf;
	U32 off;
	U32 msize; //!< this is the part of the buffer that is actually used, while buf->size() is the currently available size
private:
	virtual void init();
};

/** File buffer */
class tFBuf :public tBBuf {
public:
	tFBuf();
	virtual ~tFBuf();
	virtual U32 tell() const;
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n);
	inline virtual void write(const SByte * val,U32 n) { this->write((Byte *)val,n); }
	virtual const Byte * read(U32 n=0);
	virtual void stream(tBBuf &buf);
	virtual void store(tBBuf &buf) {}
	virtual U32 size() const;
	virtual void close();
	virtual void open(const char * path,const char * mode="rb");
	virtual void flush();
protected:
	virtual void init();
private:
	FILE * f;
	U32 xsize;
	mutable U32 msize;
	Byte * xbuf;
};

/** Static buffer */
class tSBuf :public tBBuf {
public:
	explicit tSBuf(Byte * buf,U32 msize);
	virtual U32 tell() const;
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n) {}
	inline virtual void write(const SByte * val,U32 n) { this->write((Byte *)val,n); }
	virtual const Byte * read(U32 n=0);
	virtual void stream(tBBuf &buf);
	virtual void store(tBBuf &buf) {}
	virtual U32 size() const;
private:
	Byte * buf;
	U32 off;
	U32 msize;
};

/** Zlib buffer */
class tZBuf :public tMBuf {
public:
	tZBuf() :tMBuf() {}
	tZBuf(tMBuf &k) :tMBuf(k) {}
	void compress();
	void uncompress(int iosize);
};

/** Md5 buffer */
class tMD5Buf :public tMBuf {
public:
	tMD5Buf() :tMBuf() {}
	tMD5Buf(tMBuf &k) :tMBuf(k) { this->compute(); }
	void compute();
};

/** String buffer */
class tStrBuf :public tMBuf {
public:
	tStrBuf(const char * k);
	explicit tStrBuf(U32 size=200);
	explicit tStrBuf(tBBuf &k);
	explicit tStrBuf(const tMBuf &k);
	tStrBuf(const tStrBuf &k);
	virtual ~tStrBuf();
	
	// interface
	virtual void rewind();
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t);
	
	// string functions
	S32 find(const char cat, bool reverse=false) const;
	S32 find(const char *str) const;
	S32 find(const tStrBuf &str) const { return find(str.c_str()); }
	/** \brief strips the character from the beginning (when how=1 or 3) and/or from the end (how=2 or 3) of the string */
	tStrBuf & strip(Byte what,Byte how=0x03);
	tStrBuf & escape() const;
	tStrBuf & lower() const;
	tStrBuf & upper() const;
	tStrBuf & substring(U32 start,U32 len=0) const;
	const tStrBuf & dirname() const;
	bool startsWith(const char * pat) const;
	bool endsWith(const char * pat) const;
	void writeStr(const char * t);
	void writeStr(tStrBuf * val) { val->rewind(); write(val->read(),val->size()); }
	void writeStr(tStrBuf & val) { val.rewind(); write(val.read(),val.size()); }
	void writeStr(const tStrBuf & val) { writeStr((tStrBuf &)val); } // ugly, casting a const away...
	void printf(const char * msg, ...);
	inline void printBoolean(const char *desc, bool val) { writeStr(desc); printBoolean(val); }
	void printBoolean(bool val);
	inline void nl() { writeStr("\n"); }
	U32 asU32() const;
	S32 asS32() const { return (S32)asU32(); }
	U16 asU16() const { return (U16)asU32(); }
	S16 asS16() const { return (S16)asU32(); }
	Byte asByte() const { return (Byte)asU32(); }
	SByte asSByte() const { return (SByte)asU32(); }
	const char * c_str();
	const char * c_str() const;
	inline bool isNewline(void) const
	{
		return compare("\n") == 0 || compare("\r") == 0 || compare("\n\r") == 0 || compare("\r\n") == 0;
	}
	
	// parse functions
	void decreaseLineNum();
	U16 getLineNum();
	U16 getColumnNum();
	void setSeparator(char w) { sep=w; }
	/** \brief returns a line
			\param nl If true, it will also append the \\n if it's present
			\param slash If false, a \\n followed by an slash will be ignored
			\return A tStrBuf object
	*/
	const tStrBuf & getLine(bool nl=false,bool slash=false);
	/** \brief returns a token (newline, key, value, separator - but not a space)
			\return A tStBuf object
	*/
	tStrBuf & getToken();
	
	// helpful functions
	void convertSlashesFromWinToUnix();
	void convertSlashesFromUnixToWin();
	inline bool isNull() const { return null; }
	
	// assignment
	virtual const tStrBuf & operator=(const tStrBuf &t) { copy(t); return *this; }
	virtual const tStrBuf & operator=(const char * str) { copy(str); return *this; }
	
	// comparison
	virtual bool operator==(const tStrBuf &t) const { return(!this->compare(t)); }
	virtual bool operator==(const char * str) const { return(!this->compare(str)); }
	virtual bool operator!=(const tStrBuf &t) const { return(this->compare(t)); }
	virtual bool operator!=(const char * str) const { return(this->compare(str)); }
	virtual bool operator>(const tStrBuf &t) const { return(this->compare(t)>0); }
	virtual bool operator>(const char *t) const { return(this->compare(t)>0); }
	virtual bool operator<(const tStrBuf &t) const { return(this->compare(t)<0); }
	virtual bool operator<(const char *t) const { return(this->compare(t)<0); }
	virtual bool operator>=(const tStrBuf &t) const { return(this->compare(t)>=0); }
	virtual bool operator>=(const char *t) const { return(this->compare(t)>=0); }
	virtual bool operator<=(const tStrBuf &t) const { return(this->compare(t)<=0); }
	virtual bool operator<=(const char *t) const { return(this->compare(t)<=0); }
protected:
	virtual void onmodify();
	// assignment
	virtual void copy(const char * str);
	virtual void copy(const tStrBuf &t);
private:
	U16 l,c;
	char sep;
	mutable tStrBuf * shot;
	mutable tStrBuf * cache_lower;
	bool null;

	virtual void init();
	// comparison
	inline virtual SByte compare(const tStrBuf &t) const { return tMBuf::compare(t); }
	virtual SByte compare(const char * str) const;
};

tStrBuf operator+(const tStrBuf & str1, const tStrBuf & str2);
tStrBuf operator+(const char * str1, const tStrBuf & str2);
tStrBuf operator+(const tStrBuf & str1, const char * str2);

/** Time */
class tTime :public tBaseType {
public:
	tTime(void) : tBaseType() { seconds = microseconds = 0; }
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t);
	virtual U32 size();
	virtual bool operator==(tTime &t) { return(seconds==t.seconds && microseconds==t.microseconds); }
	virtual bool operator!=(tTime &t) { return(seconds!=t.seconds || microseconds!=t.microseconds); }
	virtual bool operator>(tTime &t) { return(this->compare(t)>0); }
	virtual bool operator<(tTime &t) { return(this->compare(t)<0); }
	virtual bool operator>=(tTime &t) { return(this->compare(t)>=0); }
	virtual bool operator<=(tTime &t) { return(this->compare(t)<=0); }
	const tTime &operator=(const tTime &t) {
		seconds=t.seconds;
		microseconds=t.microseconds;
		return *this;
	}
	void now();
	double asDouble(char how='s');
	U32 asU32(char how='s');
	const char * str(Byte type=0x00);
	U32 seconds;
	U32 microseconds;
private:
	virtual SByte compare(tTime &t);
};

tTime operator+ (tTime &a,tTime &b);
tTime operator- (tTime &a,tTime &b);


} //End alc namespace

#endif
