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
	virtual ~tBaseType() {} //GCC 4 wants this?
	//tBaseType() {}
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
	//! Gets absolute offset
	virtual U32 tell() const=0;
	//! Sets absolute offset
	virtual void set(U32 pos)=0;
	//! Write a buffer of size n
	virtual void write(const Byte * val,U32 n)=0;
	inline virtual void write(const SByte * val,U32 n) { this->write((Byte *)val,n); }
	//! Reads n bytes, if n=0 reads the entire buffer
	virtual Byte * read(U32 n=0)=0;
	//!gets buffer size
	virtual U32 size() const=0;
	//!gets average buffer size (faster than size())
	virtual U32 avgSize() { return this->size(); }
	
	tBBuf();
	virtual ~tBBuf();
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
	
	virtual void copy(tBBuf &t);
	virtual SByte compare(tBBuf &t);
	/** \return True if the pointer is at the end of the stream */
	virtual bool eof() {
		return(this->tell()>=this->size());
	}
	/** \return the number of bytes remaining to the end of the stream */
	virtual int remaining() {
		return (this->size()-this->tell());
	}
	/** \return A pointer to a readonly location with the Ascii representation of the buffer */
	virtual const Byte * hexToAscii();
	//Overlaoded Operators
	virtual void operator++(int) { this->seek(+1); }
	virtual void operator--(int) { this->seek(-1); }
	virtual void operator+=(U32 n) { this->seek(+n); }
	virtual void operator-=(U32 n) { this->seek(-n); }
	
	virtual bool operator==(tBBuf &t) { return(!this->compare(t)); }
	virtual bool operator!=(tBBuf &t) { return(this->compare(t)); }
	virtual bool operator>(tBBuf &t) { return(this->compare(t)<0); }
	virtual bool operator<(tBBuf &t) { return(this->compare(t)>0); }
	virtual bool operator>=(tBBuf &t) { return(this->compare(t)<=0); }
	virtual bool operator<=(tBBuf &t) { return(this->compare(t)>=0); }
	virtual const tBBuf &operator=(tBBuf &t) { this->copy(t); return *this; }
	
	// non-virtual functions
	/** \throws txUnexpectedData if pattern don't matches buffer contents
	*/
	void check(const Byte * what,U32 n);
	void check(const char * what,U32 n) {
		this->check((Byte *)what,n);
	}
	
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
protected:
	//! Built-in initialization
	virtual void init();
	virtual void _pcopy(const tBBuf &t);
private:
	Byte * gpbuf;
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
	Byte * buf;
	U32 getRefs() { return refs; }
	void zero() {
		memset(buf,0,msize);
	}
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
	virtual U32 tell() const;
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n);
	inline virtual void write(const SByte * val,U32 n) { this->write((Byte *)val,n); }
	virtual Byte * read(U32 n=0);
	virtual void stream(tBBuf &buf);
	virtual void store(tBBuf &buf);
	virtual U32 size() const;
	virtual void clear();
	virtual void copy(const tMBuf &t);
	virtual const tMBuf &operator=(const tMBuf &t) { this->copy(t); return *this; }

	Byte getAt(U32 pos) const;
	void setAt(U32 pos,const Byte what);
	void setSize(U32 size) {
		msize=size;
	}
protected:
	void zeroend(); //!< makes sure there is a zero after the end of the buffer - this zero is NOT counted as part of the buffer size!

	virtual void onmodify();
	virtual void _pcopy(const tMBuf &t);
	
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
	virtual Byte * read(U32 n=0);
	virtual void stream(tBBuf &buf);
	virtual void store(tBBuf &buf);
	virtual U32 size() const;
	virtual U32 size();
	virtual void close();
	virtual void open(const char * path,const char * mode="rb");
	virtual void flush();
	//virtual void copy(tFBuf &t);
protected:
	virtual void init();
	//virtual void _pcopy(tFBuf &t);
private:
	FILE * f;
	U32 msize,xsize;
	Byte * xbuf;
};

/** Static buffer */
class tSBuf :public tBBuf {
public:
	tSBuf(Byte * buf,U32 msize);
	virtual U32 tell() const;
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n) {}
	virtual Byte * read(U32 n=0);
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
	explicit tStrBuf(const char * k);
	explicit tStrBuf(U32 size=200);
	tStrBuf(tBBuf &k);
	tStrBuf(const tMBuf &k);
	tStrBuf(const tStrBuf &k);
	virtual void init();
	virtual void onmodify();
	virtual ~tStrBuf();
	virtual void rewind();
	S32 find(const char cat, bool reverse=false);
	S32 find(const char *str);
	S32 find(tStrBuf &str) { return find(str.c_str()); }
	void decreaseLineNum();
	
	/** \brief returns a line
			\param nl If true, it will also append the \\n if it's present
			\param slash If false, a \\n followed by an slash will be ignored
			\return A tStrBuf object
	*/
	tStrBuf & getLine(bool nl=false,bool slash=false);
#if 0
	/** \brief returns a word, a newline, or a separator
			\param slash If false, a \\n followed by an slash will be ignored
			\return A tStrBuf object
	*/
	tStrBuf & getWord(bool slash=false);
#endif
	/** \brief returns a token (newline, key, value, separator - but not a space)
			\return A tStBuf object
	*/
	tStrBuf & getToken();
	void convertSlashesFromWinToUnix();
	void convertSlashesFromUnixToWin();
	
	/** \brief strips the character from the beginning (when how=1 or 3) and/or from the end (how=2 or 3) of the string */
	tStrBuf & strip(Byte what,Byte how=0x03);
	tStrBuf & escape();
	tStrBuf & lower();
	tStrBuf & upper();
	tStrBuf & substring(U32 start,U32 len=0);
	tStrBuf & dirname();
	bool startsWith(const char * pat);
	bool endsWith(const char * pat);
	void hasQuotes(bool has);
	
	/** \brief returs true if original source had quotes
			\return bool
	*/
	bool hasQuotes();
	bool isNull();
	void isNull(bool val);
	U16 getLineNum();
	U16 getColumnNum();
	virtual void write(const Byte * val,U32 n) {
		tMBuf::write(val,n);
		isNull(false);
	}
	virtual void write(const SByte * val,U32 n) {
		tMBuf::write(val,n);
		isNull(false);
	}
	void writeStr(const char * t);
	void writeStr(tStrBuf * val) { val->rewind(); write(val->read(),val->size()); }
	void writeStr(tStrBuf & val) { val.rewind(); write(val.read(),val.size()); }
	void writeStr(const tStrBuf & val) { writeStr((tStrBuf &)val); } // ugly, casting a const away...
	void printf(const char * msg, ...);
	inline void printBoolean(const char *desc, bool val) { writeStr(desc); printBoolean(val); }
	void printBoolean(bool val);
	inline void nl() { writeStr("\n"); }
	U32 asU32();
	S32 asS32() { return (S32)asU32(); }
	U16 asU16() { return (U16)asU32(); }
	S16 asS16() { return (S16)asU32(); }
	Byte asByte() { return (Byte)asU32(); }
	SByte asSByte() { return (SByte)asU32(); }
	const char * c_str();
	virtual void copy(const char * str);
	virtual void copy(const tStrBuf &t);
	virtual const tStrBuf & operator=(const tStrBuf &t) { copy(t); return *this; }
	virtual const tStrBuf & operator=(const char * str) { copy(str); return *this; }
	void setSeparator(char w) { sep=w; }
	virtual SByte compare(const tStrBuf &t);
	virtual SByte compare(const char * str);
	virtual bool operator==(const tStrBuf &t) { return(!this->compare(t)); }
	virtual bool operator!=(const tStrBuf &t) { return(this->compare(t)); }
	virtual bool operator>(const tStrBuf &t) { return(this->compare(t)<0); }
	virtual bool operator<(const tStrBuf &t) { return(this->compare(t)>0); }
	virtual bool operator>=(const tStrBuf &t) { return(this->compare(t)<=0); }
	virtual bool operator<=(const tStrBuf &t) { return(this->compare(t)>=0); }
	virtual bool operator==(const char * str) { return(!this->compare(str)); }
	virtual bool operator!=(const char * str) { return(this->compare(str)); }
	inline bool isNewline(void)
	{
		return compare("\n") == 0 || compare("\r") == 0 || compare("\n\r") == 0 || compare("\r\n") == 0;
	}
	const Byte *readAll(void) const {
		static const Byte null = {0};
		return buf ? buf->buf : null;
	}
protected:
	virtual void _pcopy(const tStrBuf &t);
private:
	U16 l,c;
	char sep;
	tStrBuf * shot;
	tStrBuf * cache_lower;
	Byte flags; //0x01 - original parsed string had quotes
	            //0x02 - string is NULL
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
	virtual SByte compare(tTime &t);
	virtual bool operator==(tTime &t) { return(seconds==t.seconds && microseconds==t.microseconds); }
	virtual bool operator!=(tTime &t) { return(seconds!=t.seconds || microseconds!=t.microseconds); }
	virtual bool operator>(tTime &t) { return(this->compare(t)<0); }
	virtual bool operator<(tTime &t) { return(this->compare(t)>0); }
	virtual bool operator>=(tTime &t) { return(this->compare(t)<=0); }
	virtual bool operator<=(tTime &t) { return(this->compare(t)>=0); }
	const tTime &operator=(tTime &t) {
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
};

tTime operator+ (tTime &a,tTime &b);
tTime operator- (tTime &a,tTime &b);


} //End alc namespace

#endif
