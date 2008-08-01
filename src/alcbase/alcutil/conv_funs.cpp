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

#define __U_CONV_FUNS_ID $Id$

#define _DBG_LEVEL_ 5

#include "alcugs.h"

#include "alcdebug.h"

namespace alc {

/**
  \brief Converts an hex guid to ascii
*/
const Byte * alcGetStrGuid(const Byte * guid) {
	if(guid==NULL) return (const Byte *)"null";
	static Byte str_guid[17];
	alcHex2Ascii(str_guid,guid,8);
	return str_guid;
}

/**
  \brief Converts an hex uid to ascii.
*/
const Byte * alcGetStrUid(const Byte * guid) {
	int off1=0;
	int off2=0;
	if(guid==NULL) return (const Byte *)"null";

	Byte str_guid[33];
	static Byte str_guid2[50];

	alcHex2Ascii(str_guid,guid,16);
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
  \brief Converts an Ascii uid to hex
*/
const Byte * alcGetHexUid(Byte * guid) {
	int off1=0;
	int off2=0;

	static Byte str_guid[50];

	int i;
	for(i=0; i<(int)strlen((const char *)guid); i++) {
		guid[i]=toupper(guid[i]);
	}

	alcAscii2Hex(str_guid+off1,guid+off2,4); //ID1
	off1+=4;
	off2+=9;
	alcAscii2Hex(str_guid+off1,guid+off2,4); //ID2
	off1+=2;
	off2+=5;
	alcAscii2Hex(str_guid+off1,guid+off2,4); //ID3
	off1+=2;
	off2+=5;
	alcAscii2Hex(str_guid+off1,guid+off2,4); //ID4
	off1+=2;
	off2+=5;
	alcAscii2Hex(str_guid+off1,guid+off2,6); //ID5
	off1+=6;
	off2+=12;

	return (const Byte *)str_guid; //In hex
}

/**
  \brief returns a pointer to a formated time string
*/
const Byte * alcGetStrTime(U32 timestamp, U32 microseconds) {
	static Byte btime[50];
	Byte tmptime[25];
	struct tm * tptr;
	time_t stamp = (time_t)timestamp;

	tptr=gmtime(&stamp);
	strftime((char *)tmptime,25,"%Y:%m:%d-%H:%M:%S",tptr);
	sprintf((char *)btime,"%s.%06d",(char *)tmptime,microseconds);

	return btime;
}

const Byte * alcGetStrTime(double stamp, const char format) {
	U32 time,micros;
	if(stamp!=0) {
		switch(format) {
			case 'u':
				micros = (U32)stamp % 1000000;
				time = (U32)(stamp/1000000);
				break;
			case 'm':
				micros = (U32)(stamp*1000) % 1000000;
				time = (U32)(stamp/1000);
				break;
			case 's':
			default:
				time = (U32)(stamp);
				micros = (U32)((stamp-time)*1000000);
				DBG(5, "%f = %d . %d\n", stamp, time, micros);
				break;
		}
	} else {
		time=alcGetTime();
		micros=alcGetMicroseconds();
	}
	return alcGetStrTime(time,micros);
}

/**
  \brief Converts hex data to an ASCII string
  \param out pointer to the output buffer (must be 2*size)
  \param in pointer to the input data
  \param size size of the input data
*/
void alcHex2Ascii(Byte * out, const Byte * in, int size) {
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
void alcAscii2Hex(Byte * out, const Byte * in, int size) {
	//humm I will write it if i need it :D
	int i;
	for(i=0; i<size; i++) {
		out[i]=  ((in[2*i])<0x3A ? (in[2*i] - 0x30) : (in[2*i] - (0x41-0x0A)));
		out[i]= (0x10 * out[i]) + ((in[(2*i)+1])<0x3A ? (in[(2*i)+1] - 0x30) : (in[(2*i)+1] - (0x41-0x0A)));
	}
}

/**
	\brief Strips out some characters that win32 doesn't like in file names
*/
void alcStrFilter(char * what) {
	int i=0,e=0;
	while(what[i]!=0) {
		if(what[i]!='<' && what[i]!='>' && what[i]!=':' && what[i]!='#' && what[i]!='\\' && what[i]!='/' && what[i]!='*' && what[i]!='?' && what[i]!='"' && what[i]!='\'' && what[i]!='|') {
			what[e]=what[i];
			e++; i++;
		} else {
			i++;
		}
	}
	what[e]='\0';
}


}
