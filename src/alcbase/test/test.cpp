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
/* $Id$ */

#define _DBG_LEVEL_ 10

#include <alcugs.h>

#include <alcdebug.h>

using namespace alc;

void and_another_one() {
	throw txBase("ouch, that hurts");
}

void my_other_func() {
	and_another_one();
}

void my_func() {
	my_other_func();
}

void do_tests() {
	DBG(1,"Generating an exception...\n");
	try {
		throw txBase("ooops");
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}
	DBG(2,"Continuing...\n");
	
	try {
		throw txBase("generate a core",0,1);
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}

	try {
		//throw txBase("generate a core and abort",1,1);
		my_func();
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}

}

void ok_the_real_ones() {
	//alctypes related tests
	tMBuf buf1;

	assert(buf1.tell()==0);
	try {
		buf1.set(2);
		throw txBase("Verification Test Failed - expected txOutOfRange",1);
	} catch( txOutOfRange &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}
	Byte * a=(Byte *)"Hello World";
	SByte * b="Bye cruel world";
	buf1.write(a,(U32)strlen((char *)a));
	buf1.write(b,(U32)strlen(b));
	std::cout<< "tell():" << buf1.tell() << ",len:" << strlen((char *)a) + strlen(b) << std::endl;
	assert(buf1.tell()==strlen((char *)a) + strlen(b));
	buf1.set(1);
	assert(buf1.tell()==1);
	buf1.set(strlen((char *)a) + strlen(b));
	assert(buf1.tell()==strlen((char *)a) + strlen(b));
	SByte c=0;
	buf1.write(&c,1);
	buf1.set(0);
	assert(buf1.tell()==0);
	std::cout<<"-"<< buf1.read()<<"-" <<std::endl;
	buf1.set(0);
	assert(!strcmp((char *)buf1.read(),"Hello WorldBye cruel world"));
	buf1.rewind();
	assert(buf1.tell()==0);
	buf1.end();
	assert(buf1.tell()==buf1.size());
	assert(buf1.size()==buf1.avgSize());
	assert(buf1.eof());
	try {
		buf1.seek(1);
		throw txBase("expected txOutOfRange",1);
	} catch( txOutOfRange &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}
	buf1.seek(0,SEEK_SET);
	assert(buf1.tell()==0);
	buf1.seek(0,SEEK_END);
	assert(buf1.tell()==buf1.size());
	buf1.seek(-(S32)buf1.size());
	assert(buf1.tell()==0);
	buf1.check(a,strlen((char *)a));
	buf1.check(b,strlen(b));
	buf1.check(&c,1);
	try {
		buf1.check(&c,1);
		throw txBase("expected txOutOfRange",1);
	} catch( txOutOfRange &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}
	buf1.rewind();
	try {
		buf1.check(&c,1);
		throw txBase("expected txUnexpectedData");
	} catch( txUnexpectedData &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}
	U32 my=buf1.tell();
	buf1.putU16(23);
	buf1.putS16(-123);
	buf1.putU32(789);
	buf1.putS32(-399);
	buf1.putByte(255);
	buf1.putSByte(-2);
	buf1.set(my);
	assert(buf1.getU16()==23);
	assert(buf1.getS16()==-123);
	assert(buf1.getU32()==789);
	assert(buf1.getS32()==-399);
	assert(buf1.getByte()==255);
	assert(buf1.getSByte()==-2);
	
	tMBuf buf2=buf1;
	
	assert(buf2.size()==buf1.size());
	assert(buf2.tell()==buf1.tell());
	assert(!buf1.compare(buf2));
	assert(buf1==buf2);
	buf1.end();
	buf1.put(buf2);
	assert(buf2!=buf1);
	assert(buf2<buf1);
	assert((buf1>=buf2));
	assert(!(buf2>buf1));
	assert(!(buf1<=buf2));
	buf1.end();
		
	try {
		buf1++;
		throw txBase("expected txOutOfRange",1);
	} catch( txOutOfRange &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}
	buf1--;
	buf1++;
	
	tMBuf buf3;

	buf3=buf2;
	buf3=buf1;
	assert(buf1==buf3);
	buf3=buf2;
	assert(buf3==buf2);
	
	buf3=buf1;

	tMBuf * buf4,* buf5, * buf6, *buf7;
	buf4=new tMBuf();
	buf5=new tMBuf(buf3);
	buf6=new tMBuf(buf1,1);
	buf7=new tMBuf(buf1,5,10);
	
	
	assert(*buf5==buf1);
	assert(*buf5==buf3);
	assert(*buf5!=*buf4);
	assert(*buf4<*buf5);
	assert(*buf6<*buf5);
	assert(*buf7<*buf6);
	assert(*buf4<*buf7);

	buf1.rewind();
	buf3.rewind();

	buf1.seek(1);
	buf3.seek(5);
	buf6->check(buf1.read(),buf6->size());
	buf7->check(buf3.read(),buf7->size());
	
	assert(buf7->size()==10);
	assert(buf6->size()==buf1.size()-1);
	
	buf6->putU32(123456);
	buf7->putU32(654321);
	buf6->rewind();
	buf7->rewind();

	buf1.rewind();
	buf3.rewind();

	buf1.seek(1);
	buf3.seek(5);
	buf6->check(buf1.read(),buf6->size()-4);
	buf7->check(buf3.read(),buf7->size()-4);
	
	assert(buf7->size()==14);
	assert(buf6->size()==buf1.size()+3);

	assert(buf6->getU32()==123456);
	assert(buf7->getU32()==654321);
	
	delete buf4;
	delete buf5;
	delete buf6;
	delete buf7;
}

void punto_y_aparte() {
	tMBuf k1,k2;
	
	if(k1==k2) std::cout<<"eq"<<endl;
	else std::cout<<"!eq"<<endl;
	k2=k1;
	assert(k2==k1);
	
	k1.putU32(123);
	k2=k1;
	assert(k2==k1);
}

void filebuffer() {

	dmalloc_verify(NULL);

	tFBuf f1,f2;
	
	try {
		f1.open("one");
		throw txBase("expected txNotFound",1);
	} catch( txNotFound &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
	}
	
	f1.open("garbagedump.raw","wb");
	f1.write("a",1);
	f1.write("b",1);
	
	tMBuf b1;
	char * a="Hello world";
	b1.write(a,strlen(a));
	b1.putByte(0);
	f1.put(b1);
	f1.write(a,strlen(a));
	f1.putByte(0);
	f1.flush();
	f1.close();
	
	f1.open("garbagedump.raw");
	f2.open("garbagedump.raw");
	assert(f1==f2);
	f1.close();
	f2.close();
	
	f2.open("garbagedump2.raw","wb");
	f2.put(b1);
	f2.putS32(-1);
	f2.putU32(5);
	f2.close();
	
	b1.putS32(-1);
	b1.putU32(5);
	
	f1.open("garbagedump2.raw");
	assert(f1==b1);
	
	tMBuf b2,b3;
	b2.put(f1);
	f1.rewind();
	f1.get(b3);
	assert(b2==b1);
	assert(b2==f1);
	assert(b2==b3);
	
	f1.close();
	
}

void part2() {
	tMBuf b1;
	
	b1.putU32(5);
	char * a="Hello";
	b1.write(a,strlen(a));

	tFBuf f1,f2;
	
	f1.open("out.raw","wb");
	f1.put(b1);
	f1.close();
	
	tMD5Buf m1;
	m1.put(b1);
	m1.compute();
	
	f1.open("outmd5.raw","wb");
	f1.put(m1);
	f1.close();
	
	tWDYSBuf w1;
	w1.put(b1);
	w1.encrypt();
	
	f2.open("outwdys.raw","wb");
	f2.put(w1);
	f2.close();
	
	f1.open("outwdys.raw");
	tWDYSBuf w2;
	w2.put(f1);
	f1.close();
	
	assert(w2==w1);
	w2.decrypt();
	
	assert(b1==w2);
	
	b1.putByte(0);
	
	f2.open("out.raw");
	b1.clear();
	f2.get(b1);
	f2.close();
	
	assert(b1==w2);
	
}

void part3() {

	DBG(5,"opening current directory...\n");
	
	tDirectory mdir;
	mdir.open(".");
	tDirEntry * k;
	
	while((k=mdir.getEntry())!=NULL) {
		printf("%s - %u\n",k->name,k->type);
	}

}


int main(int argc, char * argv[]) {
	std::cout << "Alcugs test suit" <<std::endl;

	DBG(4,"Starting testing suit...\n");

	try {
		do_tests();
		ok_the_real_ones();
		punto_y_aparte();
		filebuffer();
		part2();
		part3();
		std::cout<< "Success!!" << std::endl;
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
		return -1;
	} catch (...) {
		std::cout<< "Cauth Unknown Exception" <<std::endl;
		return -1;
	}
	
}
