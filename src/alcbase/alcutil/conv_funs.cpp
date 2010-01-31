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

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include "alcdebug.h"

namespace alc {

/**
  \brief Converts an hex guid to ascii
*/
const char * alcGetStrGuid(const Byte * guid) {
	if(guid==NULL) return "null";
	static char str_guid[17];
	alcHex2Ascii(str_guid,guid,8);
	return str_guid;
}

/**
  \brief Converts an hex uid to ascii.
*/
const char * alcGetStrUid(const Byte * guid) {
	if(guid==NULL) return "null";

	char str_guid[33];
	static char str_guid2[37];

	alcHex2Ascii(str_guid,guid,16);
	
	int off1=0;
	int off2=0;
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
	assert(off1 == 36 && off2 == 32);
	return str_guid2;
}

/**
  \brief Converts an Ascii uid to hex
*/
const Byte * alcGetHexUid(const char * passed_guid) {
	
	if (strlen(passed_guid) != 36) throw txUnexpectedData(_WHERE("An UID string must be 36 characters long"));

	static Byte hex_guid[16];
	char guid[36];

	for(U32 i=0; i<36; i++) {
		guid[i]=toupper(passed_guid[i]);
	}

	int off1=0;
	int off2=0;
	alcAscii2Hex(hex_guid+off1,guid+off2,4); //ID1
	off1+=4;
	off2+=8;
	if (guid[off2] != '-') throw txUnexpectedData(_WHERE("There must be a dash at position %d", off2));
	++off2;
	alcAscii2Hex(hex_guid+off1,guid+off2,2); //ID2
	off1+=2;
	off2+=4;
	if (guid[off2] != '-') throw txUnexpectedData(_WHERE("There must be a dash at position %d", off2));
	++off2;
	alcAscii2Hex(hex_guid+off1,guid+off2,2); //ID3
	off1+=2;
	off2+=4;
	if (guid[off2] != '-') throw txUnexpectedData(_WHERE("There must be a dash at position %d", off2));
	++off2;
	alcAscii2Hex(hex_guid+off1,guid+off2,2); //ID4
	off1+=2;
	off2+=4;
	if (guid[off2] != '-') throw txUnexpectedData(_WHERE("There must be a dash at position %d", off2));
	++off2;
	alcAscii2Hex(hex_guid+off1,guid+off2,6); //ID5
	off1+=6;
	off2+=12;
	assert(off1 == 16 && off2 == 36);

	return hex_guid; //In hex
}

/**
  \brief returns a pointer to a formated time string
*/
const char * alcGetStrTime(U32 timestamp, U32 microseconds) {
	static char btime[50];
	char tmptime[26];
	struct tm * tptr;
	time_t stamp = timestamp;

	tptr=gmtime(&stamp);
	strftime(tmptime,25,"%Y:%m:%d-%H:%M:%S",tptr);
	sprintf(btime,"%s.%06d",tmptime,microseconds);

	return btime;
}

const char * alcGetStrTime(double stamp, const char format) {
	U32 time,micros;
	U32 stampInt = static_cast<U32>(stamp);
	if(stamp!=0) {
		switch(format) {
			case 'u':
				micros = stampInt % 1000000;
				time = (stampInt/1000000);
				break;
			case 'm':
				micros = (stampInt*1000) % 1000000;
				time = (stampInt/1000);
				break;
			case 's':
			default:
				time = (stampInt);
				micros = ((stampInt-time)*1000000);
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
void alcHex2Ascii(char * out, const Byte * in, int size) {
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
*/
void alcAscii2Hex(Byte * out, const char * in, int size) {
	//humm I will write it if i need it :D
	int i;
	for(i=0; i<size; i++) {
		if ((in[2*i] < 0x41 || in[2*i] > 0x41+25) && (in[2*i] < 0x30 || in[2*i] > 0x30+9))
			throw txUnexpectedData(_WHERE("There is an invalid character in the data: %c", in[2*i]));
		if ((in[2*i+1] < 0x41 || in[2*i+1] > 0x41+25) && (in[2*i+1] < 0x30 || in[2*i+1] > 0x30+9))
			throw txUnexpectedData(_WHERE("There is an invalid character in the data: %c", in[2*i+1]));
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


/** \brief parses a "name[number]" kind of string, setting "t" to the name and returning the number */
U16 alcParseKey(tString *t) {
	int pos;
	pos=t->find('[');
	tString offset;
	if(pos==-1) return 0;
	if(!t->endsWith("]")) {
		throw txParseError(_WHERE("Parse error near %s, malformed var name.\n",t->c_str()));
	}
	offset=t->substring(pos,t->size()-pos);
	offset=offset.strip('[');
	offset=offset.strip(']');
	t->setSize(pos);
	return offset.asU16();
}

}
