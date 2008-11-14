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
	virtual U32 tell()=0;
	//! Sets absolute offset
	virtual void set(U32 pos)=0;
	//! Write a buffer of size n
	virtual void write(const Byte * val,U32 n)=0;
	//virtual void write(SByte * val,U32 n) { this->write((Byte *)val,n); }
	//! Reads n bytes, if n=0 reads the entire buffer
	virtual Byte * read(U32 n=0)=0;
	//!gets buffer size
	virtual U32 size()=0;
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
	/** \throws txUnexpectedData if pattern don't matches buffer contents
	*/
	virtual void check(const Byte * what,U32 n);
	virtual void check(const char * what,U32 n) {
		this->check((Byte *)what,n);
	}
	virtual void putU16(U16 val);
	//NOTE: I have already tought about overloading a "put(U16 val)", and I don't want
	// it, mainly because it can be a little confusing when you are reading the code.
	virtual void putS16(S16 val);
	virtual void putU32(U32 val);
	virtual void putS32(S32 val);
	virtual void putByte(Byte val);
	virtual void putSByte(SByte val);
	virtual void putDouble(double val);
	virtual U16 getU16();
	virtual S16 getS16();
	virtual U32 getU32();
	virtual S32 getS32();
	virtual Byte getByte();
	virtual SByte getSByte();
	virtual double getDouble();
	virtual void copy(tBBuf &t);
	virtual SByte compare(tBBuf &t);
	/** \return True if the pointer is at the end of the stream */
	virtual bool eof() {
		//std::printf("%i>=%i?\n",this->tell(),this->size());
		return(this->tell()>=this->size());
	}
	/** \return the number of bytes remaining to the end of the stream */
	virtual int remaining() {
		return (this->size()-this->tell());
	}
	/** \return A pointer to a readonly location with the Ascii representation of the buffer */
	virtual Byte * hexToAscii();
	//Overlaoded Operators
	virtual void operator++(int) { this->seek(+1); }
	virtual void operator--(int) { this->seek(-1); }
	//virtual void operator+(tBaseType &t) { this->put(t); }
	virtual void operator+=(U32 n) { this->seek(+n); }
	virtual void operator-=(U32 n) { this->seek(-n); }
	
	virtual bool operator==(tBBuf &t) { return(!this->compare(t)); }
	virtual bool operator!=(tBBuf &t) { return(this->compare(t)); }
	virtual bool operator>(tBBuf &t) { return(this->compare(t)<0); }
	virtual bool operator<(tBBuf &t) { return(this->compare(t)>0); }
	virtual bool operator>=(tBBuf &t) { return(this->compare(t)<=0); }
	virtual bool operator<=(tBBuf &t) { return(this->compare(t)>=0); }
	virtual void operator=(tBBuf &t) { this->copy(t); }
	
	/** Puts an object into the buffer (streams the object) \return the amount of written bytes */
	inline void put(tBaseType &t) {
		t.stream(*this);
	}
	/** Gets an object form the buffer (stores object from stream) */
	inline void get(tBaseType &t) {
		t.store(*this);
	}
protected:
	//! Built-in initialization
	virtual void init();
	virtual void _pcopy(tBBuf &t);
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
	tMBuf(U32 size);
	virtual ~tMBuf();
	virtual U32 tell();
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n);
	virtual void write(const SByte * val,U32 n) { this->write((Byte *)val,n); }
	virtual void zeroend(); //!< makes sure there is a zero after the end of the buffer - this zero is NOT counted as part of the buffer!
	virtual Byte * read(U32 n=0);
	virtual void stream(tBBuf &buf);
	virtual void store(tBBuf &buf);
	virtual U32 size();
	virtual void clear();
	virtual void copy(tMBuf &t);
	virtual void operator=(tMBuf &t) { this->copy(t); }
	virtual Byte getAt(U32 pos);
	virtual void setAt(U32 pos,const char what);
	//virtual Byte operator[](U32 pos) { return(this->getAt(pos)); }
	//virtual void operator[](U32 pos,const char what) { setAt(pos,what); }
	virtual void setSize(U32 size) {
		msize=size;
	}
	virtual Byte *readAll(void) {
		rewind();
		return read();
	}
protected:
	virtual void init();
	virtual void onmodify();
	virtual void _pcopy(tMBuf &t);
	tRefBuf * buf;
	U32 off;
	U32 msize; //!< this is the part of the buffer that is actually used, while buf->size() is the currently available size
};

/** File buffer */
class tFBuf :public tBBuf {
public:
	tFBuf();
	virtual ~tFBuf();
	virtual U32 tell();
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n);
	virtual void write(const SByte * val,U32 n) { this->write((Byte *)val,n); }
	virtual Byte * read(U32 n=0);
	virtual void stream(tBBuf &buf);
	virtual void store(tBBuf &buf);
	virtual U32 size();
	virtual void close();
	virtual void open(const void * path,const void * mode="rb");
	virtual void flush();
	//virtual void copy(tFBuf &t);
	//virtual void operator=(tFBuf &t) { this->copy(t); }
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
	virtual U32 tell();
	virtual void set(U32 pos);
	virtual void write(const Byte * val,U32 n) {}
	virtual void write(const SByte * val,U32 n) {}
	virtual Byte * read(U32 n=0);
	virtual void stream(tBBuf &buf);
	virtual void store(tBBuf &buf) {}
	virtual U32 size();
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
	void uncompress(int iosize=0);
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
	tStrBuf(const void * k);
	tStrBuf(U32 size=200);
	tStrBuf(tBBuf &k);
	tStrBuf(tMBuf &k);
	tStrBuf(const tStrBuf &k);
	void init();
	void onmodify();
	virtual ~tStrBuf();
	virtual void rewind();
	S32 find(const char cat, bool reverse=false);
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
	tStrBuf & substring(U32 start,U32 len);
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
	void writeStr(const Byte * t);
	void writeStr(const SByte * t) { writeStr((const Byte *)t); }
	void writeStr(Byte val) { printf("%u",val); }
	void writeStr(SByte val) { printf("%i",val); }
	void writeStr(U16 val) { printf("%u",val); }
	void writeStr(S16 val) { printf("%i",val); }
	void writeStr(U32 val) { printf("%u",val); }
	void writeStr(S32 val) { printf("%i",val); }
	void writeStr(tStrBuf * val) { val->rewind(); write(val->read(),val->size()); }
	void writeStr(tStrBuf & val) { val.rewind(); write(val.read(),val.size()); }
	void writeStr(const tStrBuf & val) { writeStr((tStrBuf &)val); }
	void printf(const char * msg, ...);
	void nl() { writeStr("\n"); }
	U32 asU32();
	S32 asS32() { return (S32)asU32(); }
	U16 asU16() { return (U16)asU32(); }
	S16 asS16() { return (S16)asU32(); }
	Byte asByte() { return (Byte)asU32(); }
	SByte asSByte() { return (SByte)asU32(); }
	const Byte * c_str();
	virtual void copy(tStrBuf &t);
	virtual void copy(const char * str);
	virtual tStrBuf & operator=(tStrBuf &t) { this->copy(t); return *this; }
	virtual tStrBuf & operator=(const tStrBuf &t) { this->copy((tStrBuf &)t); return *this; }
	virtual tStrBuf & operator=(const char * str) { this->copy(str); return *this; }
	void setSeparator(char w) { sep=w; }
	virtual SByte compare(tStrBuf &t);
	virtual SByte compare(const char * str);
	virtual bool operator==(tStrBuf &t) { return(!this->compare(t)); }
	virtual bool operator!=(tStrBuf &t) { return(this->compare(t)); }
	virtual bool operator>(tStrBuf &t) { return(this->compare(t)<0); }
	virtual bool operator<(tStrBuf &t) { return(this->compare(t)>0); }
	virtual bool operator>=(tStrBuf &t) { return(this->compare(t)<=0); }
	virtual bool operator<=(tStrBuf &t) { return(this->compare(t)>=0); }
	virtual bool operator==(const char * str) { return(!this->compare(str)); }
	virtual bool operator!=(const char * str) { return(this->compare(str)); }
protected:
	virtual void _pcopy(tStrBuf &t);
private:
	U16 l,c;
	char sep;
	tStrBuf * shot;
	tStrBuf * cache_lower;
	Byte flags; //0x01 - original parsed string had quotes
	            //0x02 - string is NULL
};

tStrBuf operator+(const tStrBuf & str1, const tStrBuf & str2);
tStrBuf operator+(const void * str1, const tStrBuf & str2);
tStrBuf operator+(const tStrBuf & str1, const void * str2);

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
	void operator=(tTime &t) {
		seconds=t.seconds;
		microseconds=t.microseconds;
	}
	void now();
	double asDouble(char how='s');
	U32 asU32(char how='s');
	const Byte * str(Byte type=0x00);
	U32 seconds;
	U32 microseconds;
};

tTime operator+ (tTime &a,tTime &b);
tTime operator- (tTime &a,tTime &b);


} //End alc namespace

#endif
