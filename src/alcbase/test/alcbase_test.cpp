/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs H'uru Server Team                     *
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

//#define _DBG_LEVEL_ 10

#define IN_ALC_PROGRAM
#define ALC_PROGRAM_ID "$Id$"

#include <alcugs.h>

#include <alcdebug.h>

using namespace alc;

void alcdebug_tests() {
	int i;
	for(i=0; i<100; i++) {
		DBG(i,"Debug Level %i\n",i);
	}
	std::cout << "where:" << _WHERE("hello").c_str() <<std::endl;
	
	FILE * f;
	f=fopen("this_file_does_not_exists_","rb");
	assert(f==NULL);
	//_DIE("testing die");
}

void and_another_one() {
	throw txBase("ouch, that hurts");
}

void my_other_func() {
	and_another_one();
}

void my_func() {
	my_other_func();
}

void alcexception_tests() {
	DBG(1,"Generating an exception...\n");
	try {
		throw txBase("ooops");
	} catch (txBase &t) {
		std::cout<< "Caught Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
	DBG(2,"Continuing...\n");
	
	try {
		throw txBase("generate a core",0,1);
	} catch (txBase &t) {
		std::cout<< "Caught Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}

	try {
		//throw txBase("generate a core and abort",1,1);
		my_func();
	} catch (txBase &t) {
		std::cout<< "Caught Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
}

void alctypes_mbuf() {
	//alctypes related tests
	tMBuf buf1;

	assert(buf1.tell()==0);
	try {
		buf1.set(2);
		throw txBase("Verification Test Failed - expected txOutOfRange",1);
	} catch( txOutOfRange &t) {
		//std::cout<< "Caught Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
	const char * a="Hello World";
	const SByte * b="Bye cruel world";
	buf1.write(a,strlen(a));
	buf1.write(b,strlen(b));
	//std::cout<< "tell():" << buf1.tell() << ",len:" << strlen(a) + strlen(b) << std::endl;
	assert(buf1.tell()==strlen(a) + strlen(b));
	buf1.set(1);
	assert(buf1.tell()==1);
	buf1.set(strlen(a) + strlen(b));
	assert(buf1.tell()==strlen(a) + strlen(b));
	SByte c=0;
	buf1.write(&c,1);
	buf1.set(0);
	assert(buf1.tell()==0);
	std::cout<<"-"<< buf1.read()<<"-" <<std::endl;
	buf1.rewind();
	assert(!strcmp(reinterpret_cast<const char *>(buf1.read()),"Hello WorldBye cruel world"));
	buf1.rewind();
	assert(buf1.tell()==0);
	buf1.end();
	assert(buf1.tell()==buf1.size());
	assert(buf1.eof());
	try {
		buf1.seek(1);
		throw txBase("expected txOutOfRange",1);
	} catch( txOutOfRange &t) {
		//std::cout<< "Caught Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
	buf1.seek(0,SEEK_SET);
	assert(buf1.tell()==0);
	buf1.seek(0,SEEK_END);
	assert(buf1.tell()==buf1.size());
	buf1.seek(-static_cast<S32>(buf1.size()));
	assert(buf1.tell()==0);
	dmalloc_verify(NULL);
	try {
		buf1.seek(-100);
		dmalloc_verify(NULL);
		throw txBase("expected txOutOfRange",1);
	} catch( txOutOfRange &t) {
		dmalloc_verify(NULL);
		//std::cout<< "Caught Exception " << t.what() << std::endl;
		dmalloc_verify(NULL);
		//std::cout<< t.backtrace() << std::endl;
		dmalloc_verify(NULL);
	}
	dmalloc_verify(NULL);
	buf1.check(a,strlen(a));
	buf1.check(b,strlen(b));
	buf1.check(&c,1);
	dmalloc_verify(NULL);
	try {
		dmalloc_verify(NULL);
		buf1.check(&c,1);
		dmalloc_verify(NULL);
		throw txBase("expected txUnexpectedData",1);
	} catch( txUnexpectedData &t) {
		dmalloc_verify(NULL);
		std::cout<< "Caught Exception " << t.what() << std::endl;
		dmalloc_verify(NULL);
		//std::cout<< t.backtrace() << std::endl;
	}
	dmalloc_verify(NULL);
	buf1.rewind();
	dmalloc_verify(NULL);
	try {
		buf1.check(&c,1);
		throw txBase("expected txUnexpectedData");
	} catch( txUnexpectedData &t) {
		//std::cout<< "Caught Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
#if defined(WORDS_BIGENDIAN)
	assert(letoh32(0x013478ab) == 0xab783401);
	assert(htole32(0x013478ab) == 0xab783401);
	assert(letoh16(0xdef0) == 0xf0de);
	assert(htole16(0xdef0) == 0xf0de);
#else
	assert(letoh32(0x013478ab) == 0x013478ab);
	assert(htole32(0x013478ab) == 0x013478ab);
	assert(letoh16(0xdef0) == 0xdef0);
	assert(htole16(0xdef0) == 0xdef0);
#endif
	U32 my=buf1.tell();
	buf1.putU16(23);
	buf1.putS16(-123);
	buf1.putU32(789);
	buf1.putS32(-399);
	buf1.putByte(255);
	buf1.putSByte(-2);
	buf1.set(my);
#if defined(WORDS_BIGENDIAN)
	const Byte * rawbuf=buf1.read();
	U32 tmp;
	memcpy(&tmp,rawbuf,2);
	assert(*((U16 *)&tmp)==(U16)letoh16((U16)23));
	memcpy(&tmp,rawbuf+2,2);
	assert(*((S16 *)&tmp)==(S16)letoh16((S16)-123));
	memcpy(&tmp,rawbuf+4,4);
	assert(*((U32 *)&tmp)==(U32)letoh32((U32)789));
	memcpy(&tmp,rawbuf+8,4);
	assert(*((S32 *)&tmp)==(S32)letoh32((S32)-399));
	rawbuf+=12;
	assert(*(rawbuf)==(Byte)255);
	rawbuf+=1;
	assert(*((rawbuf)==(SByte)-2);
	buf1.set(my);
#endif
	assert(buf1.getU16()==23);
	assert(buf1.getS16()==-123);
	assert(buf1.getU32()==789);
	assert(buf1.getS32()==-399);
	assert(buf1.getByte()==255);
	assert(buf1.getSByte()==-2);
	
	tMBuf buf2=buf1;
	
	assert(buf2.size()==buf1.size());
	assert(buf2.tell()==buf1.tell());
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
		//std::cout<< "Caught Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
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
	buf6=new tMBuf();
	U32 oldPos = buf1.tell();
	buf1.set(1);
	buf6->write(buf1.read(), buf1.size()-1);
	buf7=new tMBuf();
	buf1.set(5);
	buf7->write(buf1.read(10), 10);
	buf1.set(oldPos);
	
	
	
	assert(*buf5==buf1);
	assert(*buf5==buf3);
	assert(*buf5!=*buf4);
	assert(*buf5!=*buf6);
	assert(*buf5!=*buf7);
	assert(*buf4<*buf5);
	assert(*buf6<*buf5);
	assert(*buf7<*buf6);
	assert(*buf4<*buf7);

	buf1.rewind();
	buf3.rewind();
	buf6->rewind();
	buf7->rewind();

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
	
	// check for copying an empty mbuf
	tMBuf emptyBuf;
	tMBuf emptyBuf2(emptyBuf);
	
	// check writing from somehwere before the end over the end
	Byte bulk[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
	tMBuf writeTest;
	writeTest.write(bulk, 4);
	writeTest.seek(-2);
	writeTest.write(bulk, 8);
	assert(writeTest.tell() == 10);
	assert(writeTest.size() == 10);
	writeTest.rewind();
	assert(memcmp(writeTest.read(2), bulk, 2) == 0);
	assert(memcmp(writeTest.read(8), bulk, 8) == 0);
	assert(writeTest.eof());
	
	// test the get/put float functions
	tMBuf floatTest;
	float test = 0.467;
	floatTest.putFloat(test);
	floatTest.rewind();
	assert(floatTest.getFloat() == test);
	
	// check copy-on-write
	tMBuf cow1;
	cow1.write(bulk, 4);
	assert(cow1.getAt(1) == 0x02);
	tMBuf cow2(cow1);
	cow2.set(1);
	cow2.write(bulk, 2);
	assert(cow1.getAt(1) == 0x02);
	assert(cow2.getAt(1) == 0x01);
	cow2 = cow1;
	cow2.setAt(1, 0x01);
	assert(cow1.getAt(1) == 0x02);
	assert(cow2.getAt(1) == 0x01);
}

void alctypes_mbuf2() {
	tMBuf k1,k2;
	
	assert(k2==k1);
	k2=k1;
	assert(k2==k1);
	
	k1.putU32(123);
	assert(k2!=k1);
	k2=k1;
	assert(k2==k1);
}

void alctypes_fbuf() {
	tFBuf f1,f2;
	
	try {
		f1.open("one");
		throw txBase("expected txNotFound",1);
	} catch( txNotFound &t) {
		//std::cout<< "Caught Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
	
	f1.open("garbagedump.raw","wb");
	f1.write("a",1);
	f1.write("b",1);
	
	tMBuf b1;
	const char * a="Hello world";
	b1.write(a,strlen(a));
	b1.putByte(0);
	f1.put(b1);
	f1.write(a,strlen(a));
	f1.putByte(0);
	f1.flush();
	f1.close();
	
	f1.open("garbagedump.raw");
	f2.open("garbagedump.raw");
	assert(tMBuf(f1)==tMBuf(f2));
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
	assert(tMBuf(f1)==b1);
	
	tMBuf b2,b3;
	b2.put(f1);
	f1.rewind();
	b3.put(f1);
	assert(b2==b1);
	assert(b2==tMBuf(f1));
	assert(b2==b3);
	
	f1.close();
}

void alctypes_part2() {
	// tFBuf, tWDYSBuf, tAESBuf
	tMBuf b1;
	
	b1.putU32(5);
	const char * a="Hello";
	b1.write(a,strlen(a));
	char md5hash[16]={0x78, 0x86, 0x70, 0xf9, 0x99, 0xaa, 0x0b, 0x0c,
			  0xaf, 0x77, 0x89, 0x88, 0xbe, 0xfb, 0xe3, 0x1f};
	char WDYS[24]={'w', 'h', 'a', 't', 'd', 'o', 'y', 'o', 'u', 's', 'e', 'e',
		       0x09, 0x00, 0x00, 0x00, 0xaf, 0x1a, 0x9b, 0xd1, 0xe9, 0xf2, 0x6b, 0x5b}; // the remaining 8 bytes are OS-dependent

	tFBuf f1,f2;
	
	f1.open("out.raw","wb");
	f1.put(b1);
	f1.close();
	
	tMD5Buf m1;
	m1.put(b1);
	m1.compute();
	assert(!memcmp(m1.read(16),md5hash,16));
	
	f1.open("outmd5.raw","wb");
	f1.put(m1);
	f1.close();
	
	tWDYSBuf w1;
	w1.put(b1);
	w1.encrypt();
	assert(w1.size()==32);
	assert(!memcmp(w1.read(24),WDYS,24));
	
	f2.open("outwdys.raw","wb");
	f2.put(w1);
	f2.close();
	
	f1.open("outwdys.raw");
	tWDYSBuf w2;
	w2.put(f1);
	f1.close();
	
	assert(w2==w1);
	w2.decrypt();
	
	assert(w2.size()==9);
	assert(b1==w2);
	
	f2.open("out.raw");
	b1.clear();
	b1.put(f2);
	f2.close();
	
	assert(b1==w2);
	
	// AES stuff
	tAESBuf a1;
	a1.setM5Key();
	a1.put(b1);
	a1.encrypt();
	
	assert(a1.size() == 24);
	
	tAESBuf a2;
	a2.setM5Key();
	a2.put(a1);
	a2.decrypt();
	
	assert(a2==b1);
}

void alctypes_part3() {

	DBG(5,"opening current directory...\n");
	
	tDirectory mdir;
	mdir.open(".");
	tDirEntry * k;
	
	bool foundDocs = false, foundGame = false;
	
	while((k=mdir.getEntry())!=NULL) {
		if (k->isDir() && strcmp(k->name, "docs") == 0) foundDocs = true;
		else if (k->isFile() && strcmp(k->name, "alcugs_game") == 0) foundGame = true;
		//printf("%s - %u\n",k->name,k->type);
	}
	assert(foundDocs && foundGame);
}

void alctypes_part4() {
	DBG(5,"Helloooooo\n");
	tString aa("hola");
	assert(aa.eof());
	DBG(4,"-%s-\n",aa.c_str());
	assert(aa.eof());
	tString bb(aa);
	assert(bb.eof());
	DBG(4,"aa-%s-\n",aa.c_str());
	DBG(4,"bb-%s-\n",bb.c_str());
	tString cc=aa;
	assert(cc.eof());
	DBG(4,"aa-%s-\n",aa.c_str());
	DBG(4,"cc-%s-\n",cc.c_str());
	cc.writeStr("hola mundo");
	assert(cc.eof());
	assert(aa.eof());
	DBG(4,"aa-%s-\n",aa.c_str());
	DBG(4,"cc-%s-\n",cc.c_str());
	aa=cc;
	aa=bb;
	cc=aa;
	assert(bb==cc);
	cc=cc;
	assert(bb==cc);
	
	
	// check tString::len()
	tString test1;
	assert(test1.size() == 0);
	tString test2("");
	assert(test2.size() == 0);
	test2.writeStr("huhu");
	assert(test2.size() == 4);
	test2.c_str();
	test2.c_str();
	assert(test2.size() == 4);
	
	// find
	assert(tString("hiWorld").find("iW") == 1);
	assert(tString("hiWorldW").find("W") == 2);
	assert(tString("hiWorldW").find("Wdgdg") == -1);
	
	// comparsion
	assert(tString("aa") != "z");
	assert(tString("a") < tString("z"));
	assert(tString("a") <= "zz");
	assert(tString("a") <= "a");
	assert(!(tString("a") > "z"));
	assert(tString("zz") > "a");
	assert(tString("z") >= "aa");
	assert(tString("a") > "");
	assert(tString("") < tString("a"));
	
	// isEmpty
	tString emp;
	assert(emp.isEmpty());
	emp = "hi!";
	assert(!emp.isEmpty());
	emp.clear();
	assert(emp.isEmpty());
	
	// concat
	tString part1 = "hi world";
	tString part2 = " out there";
	assert(part1+part2 == "hi world out there");
	assert("ouch "+part1+" oops" == "ouch hi world oops");
	
	// substring
	assert(part1.substring(1, 2) == "i ");
	assert(part2.substring(4) == " there");
	part2 = part2.substring(3, 3);
	assert(part2 == "t t");
}

void alctypes_part5() {

	tString test("/path/to/something.txt");
	assert(test == test.c_str());

	char mychar;
	
	assert(test.getAt(0)=='/');
	assert(test.getAt(1)=='p');
	assert(test.getAt(3)=='t');
	assert(test.getAt(0)=='/');
	dmalloc_verify(NULL);
	try {
		dmalloc_verify(NULL);
		mychar=test.getAt(100);
		dmalloc_verify(NULL);
		throw txBase("Expected OutofRange");
		dmalloc_verify(NULL);
	} catch(txOutOfRange) {
		dmalloc_verify(NULL);
	}
	dmalloc_verify(NULL);
	
	// stripped
	tString due("///////");
	due = due.stripped('/');
	//printf("due:%s\n",due.c_str());
	assert(due=="");
	due="howhowhow ajfk fajf///////////////";
	tString blah = due = due.stripped('/');
	//printf("due:%s\n",due.c_str());
	assert(due==blah);
	assert(due=="howhowhow ajfk fajf");
	due="///////////////howhowhow ajfk fajf///////////////";
	due=due = due.stripped('/');
	assert(due=="howhowhow ajfk fajf");
	due="///////////////howhowhow ajfk fajf";
	due = due.stripped('/');
	assert(due=="howhowhow ajfk fajf");
	due="// /////////////howh//////////owhow/////// aj////////fk fajf////// /////////";
	due = due.stripped('/');
	assert(due==" /////////////howh//////////owhow/////// aj////////fk fajf////// ");
	due="/ /";
	due = due.stripped('/');
	assert(due==" ");
	due=" / ";
	due = due.stripped('/');
	assert(due==" / ");
	due="///////////////howhowhow ajfk fajf///////////////";
	due = due.stripped('/',0x01);
	//printf("due:%s\n",due.c_str());
	assert(due=="howhowhow ajfk fajf///////////////");
	due="///////////////howhowhow ajfk fajf///////////////";
	due = due.stripped('/',0x02);
	assert(due=="///////////////howhowhow ajfk fajf");
	//abort();
	dmalloc_verify(NULL);

	// dirname
	assert(test=="/path/to/something.txt");
	assert(test!="/path/to");
	dmalloc_verify(NULL);
	assert(test.dirname()=="/path/to");
	dmalloc_verify(NULL);
	test="/usr/lib";
	assert(test.dirname()=="/usr");
	test="/usr/";
	//printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="/");
	test="usr";
	assert(test.dirname()==".");
	test="/";
	assert(test.dirname()=="/");
	test=".";
	assert(test.dirname()==".");
	test="..";
	assert(test.dirname()==".");
	test="/usr/lib/";
	assert(test.dirname()=="/usr");
	test="usr/lib/";
	//printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="usr");
	test="usr/";
	assert(test.dirname()==".");
	test="usr/lib/kk/path/something";
	//printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="usr/lib/kk/path");
	test="../../../usr/lib/kk/path/something";
	//printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="../../../usr/lib/kk/path");
	test="/../../../usr/lib/kk/path/something";
	//printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="/../../../usr/lib/kk/path");
	dmalloc_verify(NULL);
	
	// stripped & dirname
	tString sth,sth2;
	sth="//some/thing//";
	sth2=sth.stripped('/');
	assert(sth2 == "some/thing");
	sth2=sth2.dirname();
	assert(sth2 == "some");
	sth2 = sth.dirname();
	printf("%s => %s\n", sth.c_str(), sth2.c_str());
	assert(sth2=="//some");
	assert(tString("ab").dirname() == ".");
	assert(strcmp(tString("/ab").dirname().c_str(), "/") == 0);
	
	tString key("hello[23]");
	U16 nr = alcParseKey(&key);
	assert(key == "hello");
	assert(nr == 23);
}

void alctypes_part7()
{
	// read ustr tests
	tString str;
	Byte ustr1[10] = {0x08, 0x00, 'H', 'i', ' ', 'W', 'o', 'r', 'l', 'd'};
	tMBuf b;
	b.write(ustr1, 10);
	b.rewind();
	b.get(str);
	assert(strcmp(str.c_str(), "Hi World") == 0);
	
	tUruString str2;
	Byte ustr2[17] = {0x0F, 0xF0, 0x8B, 0x97, 0x96, 0x8C, 0xDF, 0x96, 0x8C, 0xDF, 0x9E, 0xDF, 0x8B, 0x9A, 0x87, 0x8B, 0xDA};
	b.clear();
	b.write(ustr2, 17);
	b.rewind();
	b.get(str2);
	assert(strcmp(str2.c_str(), "this is a text%") == 0);
	
	Byte ustr5[5] = {0x03, 0x00, 0x8B, 0x00, 0x16};
	b.clear();
	b.write(ustr5, 5);
	b.rewind();
	b.get(str);
	str.rewind();
	tString check = alcHex2Ascii(str);
	assert(check == "8B0016");
	
	// write ustr tests
	str.clear();
	str.writeStr("Hallo");
	Byte ustr3[7] = {0x05, 0x00, 'H', 'a', 'l', 'l', 'o'};
	b.clear();
	b.put(str);
	b.rewind();
	assert(memcmp(b.read(7), ustr3, 7) == 0);
	
	str2.clear();
	str2.writeStr("Doh$");
	Byte ustr4[6] = {0x04, 0xF0, 0xBB, 0x90, 0x97, 0xDB};
	b.clear();
	b.put(str2);
	b.rewind();
	assert(memcmp(b.read(6), ustr4, 6) == 0);
	
	// assignment tests
	str = "Doh$";
	str2 = str;
	b.clear();
	b.put(str2);
	b.rewind();
	assert(memcmp(b.read(6), ustr4, 6) == 0);
	
	str2.clear();
	str2.writeStr("Hallo");
	str = str2;
	b.clear();
	b.put(str);
	b.rewind();
	assert(memcmp(b.read(7), ustr3, 7) == 0);
}

void alctypes_part8()
{
	tTime t1, t2;
	t1.microseconds = 423;
	t2.microseconds = 511;
	t1.seconds = t2.seconds = 0;
	assert(t1 < t2);
	assert(t1 <= t2);
	assert(t2 > t1);
	assert(t2 >= t1);
	assert(t1 != t2);
	t1.seconds = 1;
	assert(t1 > t2);
	t2 = t1;
	assert(t2 == t1);
	assert(t2 >= t1);
	assert(t2 <= t1);
	assert(!(t2 < t1));
	assert(!(t2 > t1));
}

void alctypes_tests() {
	alctypes_mbuf();
	alctypes_mbuf2();
	alctypes_fbuf();
	alctypes_part2();
	alctypes_part3();
	alctypes_part4();
	alctypes_part5();
	//alctypes_part6();
	alctypes_part7();
	alctypes_part8();
}

void alcfuncs_tests() {
	//// PageIDs
	// city
	assert(alcPageIdToNumber(6, 0x00000621) == 0);
	assert(alcPageIdToNumber(6, 0x00000627) == 6);
	assert(alcPageIdToNumber(6, 0x0000071F) == 254); // BultIn
	assert(alcPageIdToNumber(6, 0x00000720) == 255); // Textures
	assert(alcPageNumberToId(6, 0) == 0x00000621);
	assert(alcPageNumberToId(6, 6) == 0x00000627);
	assert(alcPageNumberToId(6, 254) == 0x0000071F);
	assert(alcPageNumberToId(6, 255) == 0x00000720);
	
	// Pahts (page number > 255)
	assert(alcPageIdToNumber(189, 0x0000BEC9) == 424);
	assert(alcPageNumberToId(189, 424) == 0x0000BEC9);
	
	//// Convenience
	// with lower-case
	Byte uid[16];
	alcGetHexUid(uid, "3F207Cb7-3D85-41F8-B6E2-FAFA9C36B999");
	assert(alcGetStrUid(uid) == tString("3F207CB7-3D85-41F8-B6E2-FAFA9C36B999"));
	try {
		alcGetHexUid(uid, "3F207"); // too short
		throw txBase(_WHERE("no exception?"));
	}
	catch (txUnexpectedData &) {}
	try {
		alcGetHexUid(uid, "3F207CB--3D85-41F8-B6E2-FAFA9C36B999"); // invalid form
		throw txBase(_WHERE("no exception?"));
	}
	catch (txUnexpectedData &) {}
	try {
		alcGetHexUid(uid, "3F207CB783D85-41F8-B6E2-FAFA9C36B999"); // invalid form
		throw txBase(_WHERE("no exception?"));
	}
	catch (txUnexpectedData &) {}
	
	// alcGetLoginInfo
	const char *a = "username@host:23", *b = "serv:er:12345";
	tString user, host;
	U16 port;
	
	assert(alcGetLoginInfo(a, &user, &host, &port));
	assert(host == "host");
	assert(user == "username");
	assert(port == 23);
	
	assert(alcGetLoginInfo(a+tString("15"), NULL, &host, &port));
	assert(host == "username@host");
	assert(port == 2315);
	
	assert(alcGetLoginInfo(b, NULL, &host, &port));
	assert(host == "serv:er");
	assert(port == 12345);
	
	assert(!alcGetLoginInfo(b, &user, &host, &port));
}

void alcparser_tests() {
	DBG(4,"Parser tests\n");
	
	tString a;
	a.writeStr("hello really, really big world\n\
I'm going to tell a small history about parsers,\n\
This story starts when someday\\\n_someone started to\
start touching the button that will\\\n one, tha ma la ka cha ta\
pa ka la cha sa la ma ka la ti ko su pi ko la mi na ca la pi la ta\
enia la le li lo lu sa se si so su ka ke ki ko ku pa pe pi po pu\n\
asdfghijklmnopqrstuvwxyz[23] = \"Tretze jutjes d'un jutjat menjen fetge d'un penjat\"\n\
\n\n\n\r\n		wow = !!wow = !!!!wow			lpeze		\n\
\n\
aklsjfas aslkfdjas fasklfdjeapuj3 0p89u3290hupivnp wapjkvdsa  9iiii0u3wrj\n\
\n\
Hello I'm Mr SEGFAULT, and today I'm going to annoy you.\n\
do you want somethin else?");

	tString b;
	b.writeStr("\
# This is not an autogenerated file\n\
key1 = \"val2\"\n\
 key2 = \"val3\"\n\
                           juas                 =            \"jo\"\n\
					ho_ho_ho = \"qaz\"\n\
   so \\\n = \\\n this_should_be_legal_ \n\
a=\"b\"\n\
kkkk\\\n\
 \"k\n\
a\"\n\
\n\
other \"val\"\n\
a = \"overrided\"\n\
\n\
\n\
problems = \"these are problems\"\n\
\n\
MProblem = \"this ' contains \\\" \\\\ some speical chars\"\n\
empty1\n\
empty2=\n\
empty3=""\n\
");

	a.rewind();
	b.rewind();
	//Ok, let's going to crash the system
	tString res;
	tStringTokenizer tokenizer(a);
	while(!tokenizer.eof()) {
		res=tokenizer.getLine();
		//printf("[a] %s\n",res.c_str());
	}
	tokenizer = tStringTokenizer(b);
	while(!tokenizer.eof()) {
		res=tokenizer.getLine();
		//printf("[b] %s\n",res.c_str());
	}
	tokenizer = tStringTokenizer(a);
	while(!tokenizer.eof()) {
		res=tokenizer.getToken();
		//printf("[a] %s\n",res.c_str());
	}
	tokenizer = tStringTokenizer(b);
	while(!tokenizer.eof()) {
		res=tokenizer.getToken();
		//printf("[b] %s\n",res.c_str());
	}
	//Ok, If we are here then seems that it worked - everybody is ok?
	
	// e = m * c^2
	// Now we should apply this formula everywhere, and we will be able to get flying cows in Relto as a  bonus relto page
	//config
	tConfig *cfg1 = new tConfig();
	tConfig *cfg2;
	
	cfg2 = new tConfig();

	tConfigKey * key1;
	key1 = cfg1->findKey("global");
	assert(key1==NULL);
	dmalloc_verify(NULL);
	key1 = cfg1->findKey("global",1); //<--BOUM
	dmalloc_verify(NULL);
	assert(key1!=NULL);
	dmalloc_verify(NULL);

	tConfigVal * val1;
	val1 = cfg1->findVar("kaka");
	assert(val1==NULL);
	dmalloc_verify(NULL);
	cfg1->setVar("1","kaka");
	dmalloc_verify(NULL);

	val1 = cfg1->findVar("kaka");
	assert(val1!=NULL);
	assert(val1->getName()=="kaka");

	delete cfg1;
	delete cfg2;
	dmalloc_verify(NULL);
	
	//parser (store and stream)
	bool found;
	tString out;
	b.rewind();
	tSimpleParser parser;
	parser.setConfig(cfg1 = new tConfig());
	parser.store(b);
	// re-parse
	parser.stream(out);
	assert(out.tell()>0);
	delete cfg1;
	parser.setConfig(cfg1 = new tConfig());
	parser.store(out);
	// check
	assert(cfg1->getVar("a") == "overrided");
	assert(cfg1->getVar("so") == "this_should_be_legal_");
	assert(cfg1->getVar("mProblem") == "this ' contains \" \\ some speical chars");
	assert(cfg1->getVar("kkkk") == "k\na");
	
	assert(cfg1->getVar("Empty1", &found).isEmpty());
	assert(found);
	assert(cfg1->getVar("EMPTY2", &found) == "");
	assert(found);
	assert(!cfg1->getVar("emptY3", &found).size());
	assert(found);
	
	// now let's test the XParser with override disabled
	b.clear();
	b.writeStr("A=b\na=\"overrided\"");
	b.rewind();
	tXParser xparser1(/*override*/false);
	xparser1.setConfig(cfg1);
	xparser1.store(b);
	assert(cfg1->getVar("SO") == "this_should_be_legal_"); // an old var from the first parser test
	assert(cfg1->getVar("a", "global", 0, 0) == "overrided"); // an old var from the first parser test
	assert(cfg1->getVar("a", "glObal", 0, 1) == "b");
	assert(cfg1->getVar("A", "GLOBAL", 0, 2) == "overrided");
	delete cfg1;
	
	// and XParser-specific stuff
	b.clear();
	b.writeStr("\
aname  =  a, b\n\
 [aseCtion]	\n\
anarR[0]=\"hi,ho\",hu\n\
  anArr[4]           =  \"another value\n\
which is more than a line long\"\n\
[global]\n\
empty=""\n\
");
	b.rewind();
	tXParser xparser2;
	xparser2.setConfig(cfg1 = new tConfig());
	xparser2.store(b);
	// re-parse
	out.clear();
	xparser2.stream(out);
	assert(out.tell()>0);
	printf("%s\n", out.c_str());
	delete cfg1;
	xparser2.setConfig(cfg1 = new tConfig());
	xparser2.store(out);
	// check
	assert(cfg1->getVar("aname") == "a");
	assert(!cfg1->getVar("aname", &found).isEmpty());
	assert(found);
	assert(cfg1->getVar("aname", "global", 1, 0) == "b");
	
	assert(cfg1->getVar("anarr", &found).isEmpty());
	assert(!found);
	assert(cfg1->getVar("anarr", "asection", 0, 0) == "hi,ho");
	assert(cfg1->getVar("anarr", "asection", 1, 0) == "hu");
	assert(cfg1->getVar("anarr", "asection", 0, 1, &found) == "");
	assert(!found);
	assert(cfg1->getVar("anarr", "asection", 0, 4) == "another value\nwhich is more than a line long");
	assert(cfg1->getVar("anarr", "asection", 1, 4, &found) == "");
	assert(!found);
	
	assert(cfg1->getVar("agfasgasdgf", &found).isEmpty());
	assert(!found);
	assert(!cfg1->getVar("EMpty", &found).size());
	assert(found);
	delete cfg1;
	
	// copy tests
	cfg1 = new tConfig();
	cfg1->setVar("value1", "name1");
	assert(cfg1->getVar("name1") == "value1");
	cfg1->copyKey("group", "global");
	assert(cfg1->getVar("name1", "group") == "value1");
	// Copy value from existing to non-existing
	cfg1->copyValue("name2", "name1");
	assert(cfg1->getVar("name2") == "value1");
	cfg1->copyValue("name2", "name1", "group", "global");
	assert(cfg1->getVar("name2", "group") == "value1");
	// Copy value from non-existing to existing
	cfg1->copyValue("name1", "name3");
	assert(cfg1->getVar("name1") == "value1");
	// Copy value from existing to existing
	cfg1->setVar("value2", "name2");
	cfg1->copyValue("name2", "name1");
	assert(cfg1->getVar("name2") == "value1");
	// from non-existing to existing
	cfg1->copyValue("name2", "nameWhichDoesNotExist");
	assert(cfg1->getVar("name2") == "value1");
	delete cfg1;
}

void libc_tests() {
	// check if snprintf guarantees a 0-terminated string? it should.
	char buf[10];
	snprintf(buf, sizeof(buf), "A text longer than 10 characters");
	assert(buf[8] == 'o');
	assert(sizeof(buf) == 10);
	assert(buf[9] == 0);
	// and what abut strncat? Yes, but you have to substract the last byte from the allowed size
	buf[5] = 0;
	buf[9] = 'x';
	strncat(buf, "these are mroe than 10 characters", sizeof(buf)-strlen(buf)-1);
	assert(buf[8] == 's');
	assert(buf[9] == 0);
	// strncpy is... BAD. It does not add the final 0.
	buf[9] = 'x';
	strncpy(buf, "and once again this text is longer", sizeof(buf)-1);
	assert(buf[8] == ' ');
	assert(buf[9] == 'x');
	// our alcStrncpy does it better
	buf[9] = 'x';
	alcStrncpy(buf, "we need a long text one final time", sizeof(buf)-1);
	assert(buf[8] == 'a');
	assert(buf[9] == 0);
	
}

void log_test() {

	tLog log1;
	tLog log2;
	tLog system;
	assert(log1.getDir() == "log/");

	DBG(5,"attempting to open the log file\n");

	log1.open("test");
	system.open("sys");
	assert(log1.getDir() == "log/");

	//tvLogConfig->syslog_enabled=0x01;

	system.log("Hello world\n");

	DBG(5,"attempting to print into the log file\n");

	log1.stamp();
	log1.print("This is a test\n");

	log1.rotate(true);

	log1.print("The test continues\n");

	log1.rotate(false);

	log2.open("subdir/blah",DF_HTML);
	assert(log2.getDir() == "log/subdir/");

	log1.print("I'm going to continue writing here\n");

	log1.rotate();
	log2.rotate();
	log2.rotate();
	log2.rotate();
	log2.rotate();
	log2.rotate();
	log2.rotate();
	log2.rotate();

	log2.print("Now, I'm generating an <b>html</b> file\n");

	log1.print("ABDCADFADSFASDFASFDDASFDASFASDFASFSAASDFASDFASDFASDFASDFASDFADFAS\n");
	log1.flush();

	log1.rotate();

	const char * kk="afjsakfljasfdalsdkfjskdlfjasdfjj3\
	8p94r37u9jujujoiasdfujasdofuasdfujlkasfdhasjfkl\
	asdfhasfpiuoeawhfe43w89piyehyhfwe";

	const char * kk3="hola whola ";

	log1.dumpbuf(reinterpret_cast<const Byte *>(kk),strlen(kk));
	log1.nl();

	log1.dumpbuf(reinterpret_cast<const Byte *>(kk3),strlen(kk3));
	log1.nl();

	log1.print("The test log ends _here_\n");
	
	dmalloc_verify(NULL);

	log1.close();
	log2.close();
	assert(log2.getDir() == "log/");
	system.close();
	
	alcGetMain()->std()->log("Hi here\n");
}

int main(void) {
	std::cout << std::endl << "Alcugs test suite - alcbase tests" <<std::endl;
	tAlcMain alcMain;
	try {
		alcdebug_tests();
		alcexception_tests();
		alctypes_tests();
		alcfuncs_tests();
		log_test();
		libc_tests();
		alcparser_tests();
		//alcShutdown();
		std::cout<< "Success!!" << std::endl;
	} catch (txBase &t) {
		std::cout<< "Caught Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
		return -1;
	} catch (...) {
		std::cout<< "Caught Unknown Exception" <<std::endl;
		return -1;
	}
	
	return 0;
}
