/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004  The Alcugs H'uru Server Team                          *
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

#ifndef __U_CONV_FUNS_
#define __U_CONV_FUNS_
#define __U_CONV_FUNS_ID $Id$

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

#include "data_types.h" //for Byte, and others
#include "conv_funs.h"

#include "debug.h"

/**
  \brief Converts an hex guid to ascii
*/
const Byte * get_guid(Byte * guid) {

	if(guid==NULL) return (const Byte *)"null";

	static Byte str_guid[17];
	hex2ascii2(str_guid,guid,8);
	return str_guid;
}

/**
  \brief Converts an hex uid to ascii.
*/
const Byte * create_str_guid(Byte * guid) {
	int off1=0;
	int off2=0;

	if(guid==NULL) return (const Byte *)"null";

	Byte str_guid[33];
	static Byte str_guid2[50];

	hex2ascii2(str_guid,guid,16);
	DBG(6,"the guid is %s\n",str_guid);
	//abort();
	memcpy(str_guid2+off1,str_guid+off2,8); //ID1
	off1+=8;
	off2+=8;
	memcpy(str_guid2+off1,"-",1);
	off1++;
	memcpy(str_guid2+off1,str_guid+off2,4); //ID2
	off1+=4;
	off2+=4;
	memcpy(str_guid2+off1,"-",1);
	off1++;
	memcpy(str_guid2+off1,str_guid+off2,4); //ID3
	off1+=4;
	off2+=4;
	memcpy(str_guid2+off1,"-",1);
	off1++;
	memcpy(str_guid2+off1,str_guid+off2,4); //ID4
	off1+=4;
	off2+=4;
	memcpy(str_guid2+off1,"-",1);
	off1++;
	memcpy(str_guid2+off1,str_guid+off2,12); //ID5
	off1+=12;
	off2+=12;
	str_guid2[off1]='\0';
	return str_guid2;
}

/**
  \brief Converts an Ascii guid to hex
*/

Byte * str_guid_to_hex(Byte * guid) {
	int off1=0;
	int off2=0;

	static Byte str_guid[50];

	int i;
	for(i=0; i<(int)strlen((const char *)guid); i++) {
		guid[i]=toupper(guid[i]);
	}

	ascii2hex2(str_guid+off1,guid+off2,4); //ID1
	off1+=4;
	off2+=9;
	ascii2hex2(str_guid+off1,guid+off2,4); //ID2
	off1+=2;
	off2+=5;
	ascii2hex2(str_guid+off1,guid+off2,4); //ID3
	off1+=2;
	off2+=5;
	ascii2hex2(str_guid+off1,guid+off2,4); //ID4
	off1+=2;
	off2+=5;
	ascii2hex2(str_guid+off1,guid+off2,6); //ID5
	off1+=6;
	off2+=12;

	#if 0
	Byte kk[100];
	hex2ascii2(kk,str_guid,16);
	printf("-->%s<--",guid);
	printf("-->%s<--",kk);
	abort();
	#endif

	return str_guid; //In hex
}

/**
  \brief returns a pointer to a formated time string
*/
Byte * get_stime(U32 timestamp, U32 microseconds) {
	static Byte btime[50];
	Byte tmptime[25];

	struct tm * tptr;

	tptr=gmtime((const time_t *)&timestamp);

	strftime((char *)tmptime,25,"(%Y:%m:%d:%H:%M:%S",tptr);
	//free(tptr);

	sprintf((char *)btime,"%s.%06d) ",(char *)tmptime,microseconds);

	return btime;
}

/**
  \brief Converts hex data to an ASCII string
  \param out pointer to the output buffer (must be 2*size)
  \param in pointer to the input data
  \param size size of the input data
*/
void hex2ascii2(Byte * out, Byte * in, int size) {
	int i;
	for(i=0; i<size; i++) {
		out[2*i]=  ((in[i] & 0xF0)>>4);
		out[2*i]= (out[2*i]<0x0A ? out[2*i]+0x30 : out[2*i]+(0x41-0x0A));
		out[(2*i)+1] = ((in[i] & 0x0F)<0x0A ? (in[i] & 0x0F)+0x30 : (in[i] & 0x0F)+(0x41-0x0A));
	}
	out[size*2]='\0';
}


/**
  \brief Converts an ASCII string to hex data
  \param out pointer to the output buffer
  \param in pointer to the input data
  \param size size of the input data (must be 2*size)
*/
void ascii2hex2(Byte * out, Byte * in, int size) {
	//humm I will write it if i need it :D
	int i;
	for(i=0; i<size; i++) {
		out[i]=  ((in[2*i])<0x3A ? (in[2*i] - 0x30) : (in[2*i] - (0x41-0x0A)));
		out[i]= (0x10 * out[i]) + ((in[(2*i)+1])<0x3A ? (in[(2*i)+1] - 0x30) : (in[(2*i)+1] - (0x41-0x0A)));
	}
}

/**
  \brief Decodes the specific UruString associated to a buffer.
  \note The result will be put on out, and it must have the required
	size to host it!!!
  \return The size of the decoded string is returned, so be sure to add 2
	to continue moving throught the buffer!!!
*/
int decode_urustring(unsigned char* out, unsigned char* in, U16 max_size) {
	U16 size;
	Byte how;
	int i;

	size=*(U16 *)(in) & 0x0FFF;
	how=(*(Byte *)(in+1) & 0xF0);

	if(size<max_size) max_size=size;

	//print2log(f_uru,"--->HURUSTRING SIZE:%02X - how:%02X<-----\n",size,how);
	//fprintf(stderr,"--->HURUSTRING SIZE:%02X - how:%02X<-----\n",size,how);

	for(i=0; i<max_size; i++) {
		if(how!=0x00) { out[i]=~in[i+2]; }
		else { out[i]=in[i+2]; }
	}
	out[i]='\0'; //be sure that is correctly ended!

	return size;
}

/**
  \brief Encodes the specific UruString associated to a buffer.
  \note The result will be put on out, and it must have the required
	size to host it!!!
  \param how
	0x00 -> Normal string
    0x01 -> Invert Bits string
  \return The size of the encoded string with its "header" is returned.
*/
int encode_urustring(unsigned char* out, unsigned char* in, U16 size, Byte how) {
	int i;
	if(how==0x01) { how=0xF0; }

	*(U16 *)(out)=(size & 0x0FFF);
	*(Byte *)(out+1)=(how & 0xF0);

	for(i=0; i<size; i++) {
		if(how!=0x00) { out[i+2]=~in[i]; }
		else { out[i+2]=in[i]; }
	}

	return size+2;
}

/*
int decode_urustring32(unsigned char* out, unsigned char* in, U32 max_size) {
	U32 size;
	Byte how;
	U32 i;

	size=*(U32 *)(in) & 0x0FFFFFFF;
	how=(*(Byte *)(in+3) & 0xF0);
	//how=(Byte)(*(U32 *)(in) & 0xF0000000);

	if(size<max_size) max_size=size;

	//print2log(f_uru,"--->HURUSTRING SIZE:%02X - how:%02X<-----\n",size,how);

	for(i=0; i<max_size; i++) {
		if(how!=0x00) { out[i]=~in[i+4]; }
		else { out[i]=in[i+4]; }
	}
	out[i]='\0'; //be sure that is correctly ended!

	return size;
}

int encode_urustring32(unsigned char* out, unsigned char* in, U32 size, Byte how) {
	U32 i;
	if(how==0x01) { how=0x0F; }

	*(U32 *)(out)=(size & 0x0FFFFFFF);
	*(Byte *)(out+3)=(how & 0xF0);

	// | (how & 0xF0000000);

	for(i=0; i<size; i++) {
		if(how!=0x00) { out[i+4]=~in[i]; }
		else { out[i+4]=in[i]; }
	}

	return size+4;
}
*/

/** \brief Check if the char X is present n bytes in the buffer
	\return Returns true if is the case, false in any other case
*/
char check_buf(Byte * buf, Byte car, int n) {
	int i;
	for(i=0; i<n; i++) {
		if(buf[i]!=car) return 0; //false
	}
	return 1; //true
}

/**
	\brief Strips out some characters that win32 doesn't like in file names
*/
void str_filter(Byte * what) {
	int i=0,e=0;
	while(what[i]!=0) {
		if(what[i]!='<' && what[i]!='>' && what[i]!=':' && what[i]!='#' && what[i]!='\\' && what[i]!='/' && what[i]!='*' && what[i]!='?' && what[i]!='"' && what[i]!='|') {
			what[e]=what[i];
			e++; i++;
		} else {
			i++;
		}
	}
	what[e]='\0';
}

#endif

