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
	DBG(5,"tBaseType::compare()\n");
	if(this->size()==t.size()) return 0;
	else if(this->size()<t.size()) return 1;
	else return -1;
}
void tBaseType::_pcopy(tBaseType &t) {
	DBG(5,"tBaseType::_pcopy()\n");
}
void tBaseType::copy(tBaseType &t) {
	DBG(5,"tBaseType::copy()\n");
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
	DBG(5,"tBBuf::tBBuf()\n");
	this->init(); 
}
tBBuf::~tBBuf() {
	DBG(5,"~tBBuf()\n");
	if(gpbuf!=NULL) free((void *)gpbuf);
}
void tBBuf::init() {
	DBG(5,"tBBuf::init()\n");
	gpbuf=NULL;
}
void tBBuf::putU16(U16 val) {
	this->write((Byte *)&val,2);
	//TODO, fix endianess here.
	//Uru is Little-Endian, so if we are on a Big-Ending machine, 
	// all writes and reads to any network/file buffer must be
	// correctly swaped.
}
void tBBuf::putS16(S16 val) {
	this->write((Byte *)&val,2);
}
void tBBuf::putU32(U32 val) {
	this->write((Byte *)&val,4);
}
void tBBuf::putS32(S32 val) {
	this->write((Byte *)&val,4);
}
void tBBuf::putByte(Byte val) {
	dmalloc_verify(NULL);
	this->write((Byte *)&val,1);
	dmalloc_verify(NULL);
}
void tBBuf::putSByte(SByte val) {
	this->write((Byte *)&val,1);
}
void tBBuf::putDouble(double val) {
	this->write((Byte *)&val,8);
}
U16 tBBuf::getU16() {
	return(*(U16 *)(this->read(2)));
}
S16 tBBuf::getS16() {
	return(*(S16 *)(this->read(2)));
}
U32 tBBuf::getU32() {
	return(*(U32 *)(this->read(4)));
}
S32 tBBuf::getS32() {
	return(*(S32 *)(this->read(4)));
}
Byte tBBuf::getByte() {
	return(*(Byte *)(this->read(1)));
}
SByte tBBuf::getSByte() {
	return(*(SByte *)(this->read(1)));
}
double tBBuf::getDouble() {
	return(*(double *)(this->read(8)));
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
	DBG(5,"tBBuf::_pcopy()\n");
	tBaseType::_pcopy(t);
}
void tBBuf::copy(tBBuf &t) {
	DBG(5,"tBBuf::copy()\n");
	this->_pcopy(t);
	this->rewind();
	t.rewind();
	this->write(t.read(),t.size());
}
char tBBuf::compare(tBBuf &t) {
	DBG(5,"tBBuf::compare()\n");
	char out;
	rewind();
	t.rewind();
	DBG(5,"%u==%u?\n",t.size(),size());
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
	DBG(5,"tMBuf()\n");
	this->init();
}
tMBuf::tMBuf(tBBuf &t,U32 start,U32 len) {
	DBG(5,"tMBuf(tBBuf,start:%u,len:%u)\n",start,len);
	this->init();
	t.rewind();
	if(start) t.read(start);
	if(!len) len=t.size()-start;
	this->write(t.read(),len);
}
tMBuf::tMBuf(U32 size) {
	DBG(5,"tMBuf(size:%i)\n",size);
	buf = new tRefBuf(size);
	mstart=msize=off=0;
}
tMBuf::tMBuf(tMBuf &t,U32 start,U32 len) {
	DBG(5,"tMBuf(tMBuf,start:%u,len:%u)\n",start,len);
	if((S32)t.msize-(S32)start<0) throw txOutOfRange(_WHERE("start:%i,size:%i",start,t.msize));
	buf = t.buf;
	buf->inc();
	if(len==0) {
		msize = t.msize-start;
	} else {
		msize = len;
	}
	mstart=start;
	off=t.off;
}
tMBuf::~tMBuf() {
	DBG(5,"~tMBuf()\n");
	if(buf!=NULL) {
		buf->dec();
		if(buf->getRefs()<=0) {
			delete buf;
		}
	}
}
void tMBuf::_pcopy(tMBuf &t) {
	DBG(5,"tMBuf::_pcopy()\n");
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
	DBG(5,"tMBuf::copy()\n");
	this->_pcopy(t);
}
void tMBuf::init() {
	DBG(5,"tMBuf::init()\n");
	buf=NULL;
	mstart=msize=off=0;
}
U32 tMBuf::tell() { return off; }
void tMBuf::set(U32 pos) { 
	if(pos>msize) throw txOutOfRange(_WHERE("OutOfRange %i>%i",off,msize));
	off=pos;
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
	dmalloc_verify(NULL);
	U32 pos=off+mstart;
	if(n==0) n=msize-off;
	if(n==0) return NULL;
	off+=n;
	if(off>msize || buf==NULL || buf->buf==NULL) { 
		DBG(5,"off:%u,msize:%u\n",off,msize); off-=n; 
		//throw txOutOfRange(_WHERE("OutOfRange %i>%i",off+n,msize)); 
		return NULL;
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
	if(f!=NULL) return ftell(f);
	return 0; 
}
void tFBuf::set(U32 pos) {
	if(f==NULL || fseek(f,pos,SEEK_SET)<0) {
		throw txOutOfRange(_WHERE("OutOfRange"));
	}
}
void tFBuf::write(Byte * val,U32 n) {
	DBG(5,"write(val:%s,n:%u)\n",val,n);
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
	return msize; 
}
void tFBuf::open(const char * path,const char * mode) {
	f=fopen(path,mode);
	if(f==NULL) {
		if(errno==EACCES || errno==EISDIR) throw txNoAccess((char *)path);
		if(errno==ENOENT) throw txNotFound((char *)path);
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
	init();
	writeStr((char *)k);
	rewind();
}
tStrBuf::tStrBuf(U32 size) :tMBuf(size) { init(); }
tStrBuf::tStrBuf(tBBuf &k,U32 start,U32 len) :tMBuf(k,start,len) {
	DBG(5,"copy zero\n");
	init(); 
}
tStrBuf::tStrBuf(tMBuf &k,U32 start,U32 len) :tMBuf(k,start,len) { 
	DBG(5,"copy one\n");
	init(); 
}
tStrBuf::tStrBuf(tStrBuf &k,U32 start,U32 len) :tMBuf(k,start,len) { 
	DBG(5,"copy two\n");
	init(); 
	l=k.l; 
	c=k.c; 
}
tStrBuf::~tStrBuf() {
	if(bufstr!=NULL) free((void *)bufstr);
	if(shot!=NULL) delete shot;
}
void tStrBuf::init() {
	bufstr=NULL;
	l=c=0;
	sep='=';
	shot=NULL;
	flags=0x00;
}
void tStrBuf::_pcopy(tStrBuf &t) {
	DBG(5,"tStrBuf::_pcopy()\n");
	tMBuf::_pcopy(t);
	bufstr=NULL;
	l=t.l;
	c=t.c;
	shot=NULL;
	sep=t.sep;
	flags=t.flags;
}
void tStrBuf::copy(tStrBuf &t) {
	DBG(5,"tStrBuf::copy()\n");
	this->_pcopy(t);
}
void tStrBuf::copy(const void * str) {
	tStrBuf pat(str);
	copy(pat);
}
SByte tStrBuf::compare(tStrBuf &t) {
	rewind();
	t.rewind();
	U32 s = size();
	U32 s2 = t.size();
	if(s>s2) s=s2;
	return((SByte)strncmp((char *)read(),(char *)t.read(),s));
}
SByte tStrBuf::compare(const void * str) {
	tStrBuf pat(str);
	return(compare(pat));
}
Byte * tStrBuf::c_str() {
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
	return flags & 0x01;
}
void tStrBuf::hasQuotes(bool has) {
	if (has) {
		flags |= 0x01;
	} else {
		flags &= ~0x01;
	}
}
U16 tStrBuf::getLineNum() {
	return l;
}
U16 tStrBuf::getColumnNum() {
	return c;
}
tStrBuf & tStrBuf::getLine(bool nl,bool slash) {
	DBG(5,"getLine()\n");
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
	DBG(5,"getWord()\n");
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
	DBG(5,"tStrBuf::getToken()\n");
	Byte c;
	Byte slash=0;
	Byte quote=0;
	Byte mode=0;
	tStrBuf * out;
	out = new tStrBuf(200);
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
			out->hasQuotes(1);
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
				out->putByte(c);
				if(c=='\n') {
					if(!eof() && getByte()!='\r') seek(-1);
					else out->putByte('\r');
					this->l++;
					this->c=0;
				} else {
					if(!eof() && getByte()!='\n') seek(-1);
					else out->putByte('\n');
					this->l++;
					this->c=0;
				}
				if(quote==0) {
					break;
				}
			}
		} else if(c=='\\') {
			slash=1; 
		} else if(quote==0 && (c==' ' || c==sep || isblank(c))) {
			if(mode==1) {
				if(c==sep) seek(-1);
				break;
			} else {
				if(c==sep) {
					out->putByte(sep);
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
	return atoi((char *)read());
}

void tTime::store(tBBuf &t) {
	DBG(5,"Buffer status size:%i,offset:%i\n",t.size(),t.tell());
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
	return r;
}
tTime operator-(tTime &a,tTime &b) {
	tTime r;
	r.seconds=a.seconds - b.seconds;
	r.microseconds=a.microseconds - b.microseconds;
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
const Byte * tTime::str() {
	return alcGetStrTime(seconds,microseconds);
}

} //end namespace alc
