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

#define _DBG_LEVEL_ 10
#define IN_ALC_PROGRAM
#define ALC_PROGRAM_ID "$Id$"

#include <alcugs.h>

#include <alcdebug.h>

using namespace alc;

void alcdebug_tests() {
	std::cout << "alcdebug_tests" <<std::endl;
	int i;
	for(i=0; i<100; i++) {
		DBG(i,"Debug Level %i\n",i);
	}
	std::cout << "where:" <<_WHERE("hello") <<std::endl;
	
	std::FILE * f;
	f=std::fopen("this_file_does_not_exists_","rb");
	assert(f==NULL);
	ERR(0,"testing dbg ERR call\n");
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
	std::cout << "alcexception_tests" <<std::endl;
	DBG(1,"Generating an exception...\n");
	try {
		throw txBase("ooops");
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
	DBG(2,"Continuing...\n");
	
	try {
		throw txBase("generate a core",0,1);
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}

	try {
		//throw txBase("generate a core and abort",1,1);
		my_func();
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
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
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
	const Byte * a=(Byte *)"Hello World";
	const SByte * b="Bye cruel world";
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
		//std::cout<< t.backtrace() << std::endl;
	}
	buf1.seek(0,SEEK_SET);
	assert(buf1.tell()==0);
	buf1.seek(0,SEEK_END);
	assert(buf1.tell()==buf1.size());
	buf1.seek(-(S32)buf1.size());
	assert(buf1.tell()==0);
	dmalloc_verify(NULL);
	try {
		buf1.seek(-100);
		dmalloc_verify(NULL);
		throw txBase("expected txOutOfRange",1);
	} catch( txOutOfRange &t) {
		dmalloc_verify(NULL);
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		dmalloc_verify(NULL);
		//std::cout<< t.backtrace() << std::endl;
		dmalloc_verify(NULL);
	}
	dmalloc_verify(NULL);
	buf1.check(a,strlen((char *)a));
	buf1.check(b,strlen(b));
	buf1.check(&c,1);
	dmalloc_verify(NULL);
	try {
		dmalloc_verify(NULL);
		buf1.check(&c,1);
		dmalloc_verify(NULL);
		throw txBase("expected txOutOfRange",1);
	} catch( txUnexpectedData &t) {
		dmalloc_verify(NULL);
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		dmalloc_verify(NULL);
		//std::cout<< t.backtrace() << std::endl;
	} catch( txOutOfRange &t) {
		dmalloc_verify(NULL);
		std::cout<< "Cauth Exception " << t.what() << std::endl;
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
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		//std::cout<< t.backtrace() << std::endl;
	}
#if defined(WORDS_BIGENDIAN)
	assert(letoh32(0x013478ab) == 0xab783401);
	assert(htole32(0x013478ab) == 0xab783401);
	assert(letoh16((U16)0xdef0) == (U16)0xf0de);
	assert(htole16((U16)0xdef0) == (U16)0xf0de);
#else
	assert(letoh32(0x013478ab) == 0x013478ab);
	assert(htole32(0x013478ab) == 0x013478ab);
	assert(letoh16((U16)0xdef0) == (U16)0xdef0);
	assert(htole16((U16)0xdef0) == (U16)0xdef0);
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
	Byte * rawbuf=buf1.read();
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
	assert(*((Byte *)rawbuf)==(Byte)255);
	rawbuf+=1;
	assert(*((SByte *)rawbuf)==(SByte)-2);
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
	buf6=new tMBuf(buf1,1);
	buf7=new tMBuf(buf1,5,10);
	
	
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
}

void alctypes_mbuf2() {
	tMBuf k1,k2;
	
	if(k1==k2) std::cout<<"eq"<<std::endl;
	else std::cout<<"!eq"<<std::endl;
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
		std::cout<< "Cauth Exception " << t.what() << std::endl;
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

void alctypes_part2() {
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
	
	assert(b1==w2);
	
	b1.putByte(0);
	
	f2.open("out.raw");
	b1.clear();
	f2.get(b1);
	f2.close();
	
	assert(b1==w2);
	
}

void alctypes_part3() {

	DBG(5,"opening current directory...\n");
	
	tDirectory mdir;
	mdir.open(".");
	tDirEntry * k;
	
	bool foundSrc = false, foundMake = false;
	
	while((k=mdir.getEntry())!=NULL) {
		if (k->type == 4 && strcmp(k->name, "src")) foundSrc = true;
		else if (k->type == 8 && strcmp(k->name, "Makefile.am")) foundMake = true;
		//printf("%s - %u\n",k->name,k->type);
	}
	assert(foundSrc && foundMake);
}

void alctypes_part4() {
	DBG(5,"Helloooooo\n");
	tStrBuf aa("hola");
	DBG(4,"-%s-\n",aa.c_str());
	assert(aa.eof());
	tStrBuf bb(aa);
	assert(bb.eof());
	DBG(4,"aa-%s-\n",aa.c_str());
	DBG(4,"bb-%s-\n",bb.c_str());
	tStrBuf cc=aa;
	assert(cc.eof());
	DBG(4,"aa-%s-\n",aa.c_str());
	DBG(4,"cc-%s-\n",cc.c_str());
	cc.writeStr("hola mundo");
	assert(cc.eof());
	assert(aa.eof());
	DBG(4,"aa-%s-\n",aa.c_str());
	DBG(4,"cc-%s-\n",cc.c_str());
	aa=cc;
	DBG(4,"aa-%s-\n",aa.c_str());
	DBG(4,"bb-%s-\n",bb.c_str());
	DBG(4,"cc-%s-\n",cc.c_str());
	aa=bb;
	cc=aa;
	DBG(4,"aa-%s-\n",aa.c_str());
	DBG(4,"bb-%s-\n",bb.c_str());
	DBG(4,"cc-%s-\n",cc.c_str());
	
	// check tStrBuf::len()
	tStrBuf test1;
	assert(test1.size() == 0);
	tStrBuf test2("");
	assert(test2.size() == 0);
	test2.writeStr("huhu");
	assert(test2.size() == 4);
	test2.c_str();
	test2.c_str();
	assert(test2.size() == 4);
}

void alctypes_part5() {

	tStrBuf test("/path/to/something.txt");
	printf("%s\n",test.c_str());

	char mychar;
	
	assert(test[0]=='/');
	assert(test[1]=='p');
	assert(test[3]=='t');
	assert(test[0]=='/');
	dmalloc_verify(NULL);
	try {
		dmalloc_verify(NULL);
		mychar=test[1000];
		dmalloc_verify(NULL);
		throw txBase("Expected OutofRange");
		dmalloc_verify(NULL);
	} catch(txOutOfRange) {
		dmalloc_verify(NULL);
		printf("passed.\n");
		dmalloc_verify(NULL);
	}
	dmalloc_verify(NULL);
	dmalloc_verify(NULL);

	assert(test=="/path/to/something.txt");
	assert(test!="/path/to");
	dmalloc_verify(NULL);
	printf("%s\n",test.dirname().c_str());
	assert(test.dirname()=="/path/to");
	printf("-%s-\n",test.dirname().c_str());
	dmalloc_verify(NULL);
	test="/usr/lib";
	assert(test.dirname()=="/usr");
	test="/usr/";
	printf("-%s-\n",test.dirname().c_str());
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
	printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="usr");
	test="usr/";
	assert(test.dirname()==".");
	test="usr/lib/kk/path/something";
	printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="usr/lib/kk/path");
	test="../../../usr/lib/kk/path/something";
	printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="../../../usr/lib/kk/path");
	test="/../../../usr/lib/kk/path/something";
	printf("-%s-\n",test.dirname().c_str());
	assert(test.dirname()=="/../../../usr/lib/kk/path");
	dmalloc_verify(NULL);
	
	tStrBuf due("/////////");
	due.strip('/');
	printf("due:%s\n",due.c_str());
	assert(due=="");
	due="howhowhow ajfk ñfajf///////////////";
	due.strip('/');
	printf("due:%s\n",due.c_str());
	assert(due=="howhowhow ajfk ñfajf");
	due="///////////////howhowhow ajfk ñfajf///////////////";
	due.strip('/');
	assert(due=="howhowhow ajfk ñfajf");
	due="///////////////howhowhow ajfk ñfajf";
	due.strip('/');
	assert(due=="howhowhow ajfk ñfajf");
	due="// /////////////howh//////////owhow/////// aj////////fk ñfajf////// /////////";
	due.strip('/');
	assert(due==" /////////////howh//////////owhow/////// aj////////fk ñfajf////// ");
	due="/ /";
	due.strip('/');
	assert(due==" ");
	due=" / ";
	due.strip('/');
	assert(due==" / ");
	due="///////////////howhowhow ajfk ñfajf///////////////";
	due.strip('/',0x01);
	printf("due:%s\n",due.c_str());
	assert(due=="howhowhow ajfk ñfajf///////////////");
	due="///////////////howhowhow ajfk ñfajf///////////////";
	due.strip('/',0x02);
	assert(due=="///////////////howhowhow ajfk ñfajf");
	//abort();
	dmalloc_verify(NULL);
}

void alctypes_part6() {

	tStrBuf sth,sth2;
	sth="//some/thing//";
	sth.strip('/');
	assert(sth == "some/thing");
	sth2=sth.dirname();
	sth=sth.dirname();
}

void alctypes_part7()
{
	// read ustr tests
	tUStr str;
	str.setVersion(1); // auto-detect
	Byte ustr1[10] = {0x08, 0x00, 'H', 'i', ' ', 'W', 'o', 'r', 'l', 'd'};
	tMBuf b;
	b.write(ustr1, 10);
	b.rewind();
	b.get(str);
	assert(strcmp((char *)str.c_str(), "Hi World") == 0);
	assert(str.getVersion() == 0);
	
	str.setVersion(1); // auto-detect
	Byte ustr2[17] = {0x0F, 0xF0, 0x8B, 0x97, 0x96, 0x8C, 0xDF, 0x96, 0x8C, 0xDF, 0x9E, 0xDF, 0x8B, 0x9A, 0x87, 0x8B, 0xDA};
	b.clear();
	b.write(ustr2, 17);
	b.rewind();
	b.get(str);
	assert(strcmp((char *)str.c_str(), "this is a text%") == 0);
	assert(str.getVersion() == 5);
	
	str.setVersion(0); // normal, but we don't use ASCII characters this time (check if a 0 in the string is a problem)
	Byte ustr5[5] = {0x03, 0x00, 0x8B, 0x00, 0x16};
	b.clear();
	b.write(ustr5, 5);
	b.rewind();
	b.get(str);
	Byte check[7];
	alcHex2Ascii(check, str.readAll(), 3);
	assert(strcmp((char *)check, "8B0016") == 0);
	
	// write ustr tests
	str.setVersion(0);
	str.clear();
	str.writeStr("Hallo");
	Byte ustr3[7] = {0x05, 0x00, 'H', 'a', 'l', 'l', 'o'};
	b.clear();
	b.put(str);
	b.rewind();
	assert(memcmp(b.read(7), ustr3, 7) == 0);
	
	str.setVersion(5);
	str.clear();
	str.writeStr("Doh$");
	Byte ustr4[6] = {0x04, 0xF0, 0xBB, 0x90, 0x97, 0xDB};
	b.clear();
	b.put(str);
	b.rewind();
	assert(memcmp(b.read(6), ustr4, 6) == 0);
	
	// assignment tests
	tUStr str2(str);
	b.clear();
	b.put(str2);
	b.rewind();
	assert(memcmp(b.read(6), ustr4, 6) == 0);
	
	str.setVersion(0);
	str.clear();
	str.writeStr("Hallo");
	str2 = str;
	b.clear();
	b.put(str2);
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
	alctypes_part6();
	alctypes_part7();
	alctypes_part8();
}

void alcparser_tests() {
	DBG(4,"Parser tests\n");
	int i=0;
	
	tStrBuf a;
	a.writeStr("hello really, really big world\n\
I'm going to tell a small history about parsers,\n\
This story starts when someday\\\n_someone started to\
start touching the button that will\\\n one, tha ma la ka cha ta\
pa ka la cha sa la ma ka la ti ko su pi ko la mi na ca la pi la ta\
enia la le li lo lu sa se si so su ka ke ki ko ku pa pe pi po pu\n\
asdfghijklmnñopqrstuvwxyz[23] = \"Tretze jutjes d'un jutjat menjen fetge d'un penjat\"\n\
\n\n\n\r\n		wow = !!wow = !!!!wow			lpeze		\n\
\n\
ñaklsjfas aslkñfdjas fañsklfdjeapuj3 0p89u3290hupivnp wapñjkvdsa  9iiii0u3wrj\n\
\n\
Hello I'm Mr SEGFAULT, and today I'm going to annoy you.\n\
do you want somethin else?");

	tStrBuf b;
	b.writeStr("\
# This is not an autogenerated file\n\
key1 = \"val2\"\n\
 key2 = \"val3\"\n\
                           juas                 =            \"jo\"\n\
					ho_ho_ho = \"qaz\"\n\
   so \\\n = \\\n \"this_should_be_legal\" \n\
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
mproblem = \"this ' contains \\\" \\\\ some speical chars\"\n\
");

	//Ok, let's going to crash the system
	
	a.rewind();
	b.rewind();
	
	tStrBuf res;
	
	DBG(6,"kkkkk\n");
#if 0
	i=0;
	while(!a.eof()) {
		res=a.getWord();
		printf("[a:%i] %s\n",i++,res.c_str());
	}
	i=0;
	while(!b.eof()) {
		res=b.getWord();
		printf("[b:%i] %s\n",i++,res.c_str());
	}
#endif
	printf("by lines now\n");
	a.rewind();
	b.rewind();
	i=0;
	while(!a.eof()) {
		res=a.getLine();
		//printf("[a:%i] %s\n",i++,res.c_str());
	}
	i=0;
	while(!b.eof()) {
		res=b.getLine();
		//printf("[b:%i] %s\n",i++,res.c_str());
	}
	printf("by tokens now\n");
	a.rewind();
	b.rewind();
	i=0;
	while(!a.eof()) {
		res=a.getToken();
		//printf("[a:%i] %s\n",i++,res.c_str());
	}
	i=0;
	while(!b.eof()) {
		res=b.getToken();
		//printf("[b:%i] %s\n",i++,res.c_str());
	}
	//Ok, If we are here then seems that it worked - everybody is ok?
	
	// e = m * c^2
	// Now we should apply this formula everywhere, and we will be able to get flying cows in Relto as a  bonus relto page
	//config
	tConfig cfg1;
	tConfig * cfg2;
	
	cfg2 = new tConfig();

	tConfigKey * key1;
	key1 = cfg1.findKey("global");
	assert(key1==NULL);
	dmalloc_verify(NULL);
	key1 = cfg1.findKey("global",1); //<--BOUM
	dmalloc_verify(NULL);
	assert(key1!=NULL);
	dmalloc_verify(NULL);

	tConfigVal * val1;
	val1 = cfg1.findVar("kaka");
	assert(val1==NULL);
	dmalloc_verify(NULL);
	cfg1.setVar("1","kaka");
	dmalloc_verify(NULL);

	val1 = cfg1.findVar("kaka");
	assert(val1!=NULL);
	//assert(!strcmp((const char *)val1->getName(),"kaka"));
	assert(val1->getName()=="kaka");

	delete cfg2;
	dmalloc_verify(NULL);
	
	//parser
	b.rewind();
	tSimpleParser parser;
	parser.setConfig(&cfg1);
	parser.store(b);
	tStrBuf out;
	U32 oldPos = out.tell();
	parser.stream(out);
	assert(out.tell()>oldPos);
	printf("original: ->%s<-\n",b.c_str());
	printf("generated: ->%s<-\n",out.c_str());
	assert(cfg1.getVar("a") == "overrided");
	assert(cfg1.getVar("so") == "this_should_be_legal");
	assert(cfg1.getVar("mproblem") == "this ' contains \" \\ some speical chars");
	assert(cfg1.getVar("kkkk") == "k\na");
	
	// now let's test the XParser
	b.clear();
	b.writeStr("\
aname  =  a, b\n\
 [asection]	\n\
anarr[0]=\"hi,ho\",hu\n\
  anarr[4]           =  \"another value\n\
which is more than a line long\"\n\
");
	b.rewind();
	tXParser xparser;
	xparser.setConfig(&cfg1);
	xparser.store(b);
	assert(cfg1.getVar("a") == "overrided"); // an old var
	assert(cfg1.getVar("aname") == "a");
	assert(cfg1.getVar("aname", "global", 1, 0) == "b");
	assert(cfg1.getVar("anarr").isNull());
	assert(cfg1.getVar("anarr", "asection", 0, 0) == "hi,ho");
	assert(cfg1.getVar("anarr", "asection", 1, 0) == "hu");
	assert(cfg1.getVar("anarr", "asection", 0, 4) == "another value\nwhich is more than a line long");
}

int log_test() {

	tLog log1;
	tLog log2;
	tLog system;

	DBG(5,"attempting to open the log file\n");

	log1.open("test",5,0);
	system.open("sys",5,DF_SYSLOG);

	//tvLogConfig->syslog_enabled=0x01;

	system.log("Hello world\n");
	system.logl(10,"This line will be not printed\n");
	system.logl(1,"This one yes\n");

	DBG(5,"attempting to print into the log file\n");

	log1.stamp();
	log1.print("This is a test\n");

	log1.rotate(true);

	log1.print("The test continues\n");

	log1.rotate(false);

	log2.open("maika/sordida",4,DF_HTML);

	log1.print("I'm going to continue writting here\n");

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

	const char * kk="afjsakflñjasfdñalsdkfjskdlfñjasdfjj3\
	8p94r37u9jujujoiasdfujasdofuasdfuñjlkasfdhasjfkl\
	8p94r37u9jujujoiasdfujasdofuasdfuñjlkasfdhasjfkl\
	8p94r37u9jujujoiasdfujasdofuasdfuñjlkasfdhasjfkl\
	ñasdfhasfpiuoeñawhfe43w89piyehyhfwe";

	const char * kk3="hola whola ";

	log1.dumpbuf((Byte *)kk,strlen(kk));
	log1.nl();

	log1.dumpbuf((Byte *)kk3,strlen(kk3));
	log1.nl();

	log1.print("The test ends _here_\n");
	
	dmalloc_verify(NULL);

	log1.close();
	log2.close();
	system.close();
	
	lstd->log("Hi here\n");

	return 0;
}

int main(int argc, char * argv[]) {
	std::cout << "Alcugs test suit" <<std::endl;

	try {
		alcdebug_tests();
		alcInit(argc,argv);
		alcexception_tests();
		alctypes_tests();
		log_test();
		alcparser_tests();
		//alcShutdown();
		std::cout<< "Success!!" << std::endl;
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
		return -1;
	} catch (...) {
		std::cout<< "Cauth Unknown Exception" <<std::endl;
		return -1;
	}
	
	return 0;
}
