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
	Useful helper functions
*/

/* CVS tag - DON'T TOUCH*/
#define __U_USEFUL_ID "$Id$"
//#define _DBG_LEVEL_ 10#include "alcdefs.h"
#include "alcdefs.h"
#include "useful.h"

#include "alcexception.h"

#include <ctime>
#include <cstring>
#include <sys/time.h>
#include <fcntl.h>
#include <cassert>
#include <arpa/inet.h>

namespace alc {

/**
	Waits for user input, blocks until user has not entered something followed by return
*/
tString alcConsoleAsk() {
	char what[501];
	fflush(0);
	if (!fgets(what,500,stdin)) throw txReadErr(_WHERE("Error requesting user input"));
	int str_len=strlen(what);
	if(what[str_len-1]=='\n') {
		what[str_len-1]='\0';
	}
	return tString(what);
}

/**
	\brief "Transforms a  username@host:port into username, hostname and port"
*/
bool alcGetLoginInfo(tString argv,tString * username,tString * hostname,U16 *port)
{
	if (username) { // don't look for username if its not requested
		int at = argv.find('@');
		if (at >= 0) {
			*username = argv.substring(0, at);
			argv = argv.substring(at+1);
		}
		else
			return false;
	}

	// now look for host and port
	int colon = argv.find(':', /*reverse*/true);
	if (colon >= 0) {
		*hostname = argv.substring(0, colon);
		argv = argv.substring(colon+1);
		*port = argv.asU16();
	}
	else return false;
	return true;
}

/**
	Quick way to get microseconds
*/
U32 alcGetMicroseconds() {
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return tv.tv_usec;
}

U32 alcGetTime() {
	return time(NULL);
}

/** Gets the current time as double*/
double alcGetCurrentTime(const char format) {
	switch(format) {
		case 'u':
			return (time(NULL) * 1000000.0) + alcGetMicroseconds();
		case 'm':
			return (time(NULL) * 1000.0) + (alcGetMicroseconds() / 1000.0);
		case 's':
		default:
			return time(NULL) + (alcGetMicroseconds() / 1000000.0);
	}
}


/** Handle nicely foreign languages. setlocale() is not sufficient on systems which didn't installed all locales,
 so we better use our own function. Feel free to add other accents according to your language.
 
 NOTE: I think that setlocale is going to be the way used in the future. The user will need to install
	and set the prefered locales.
*/
bool alcIsAlpha(char c) {
#ifdef __WIN32__
	return(IsCharAlpha(c));
#else
	return(index("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ·‡‚‰\
ÈËÍÎÌÏÓÔÛÚÙˆ˙˘˚¸Á«Ò—ﬂ", c)!=NULL); //<- Some characters are not visible on UTF systems, or under other codifications, so you may see garbage
#endif
}

void setCloseOnExec(int fd)
{
	int flags;
	flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		throw txBase(_WHERE("Error reading flags of FD"));
	flags |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, flags) == -1)
		throw txBase(_WHERE("Error writing flags of FD"));
}

/** creates a 0000000000000000 valid guid
	\param guid A hex guid (8 characters)
	\return A str guid, twice as long as the hex guid
*/
tString alcGetStrGuid(const Byte * guid) {
	return alcHex2Ascii(tMBuf(guid, 8));
}

/** encodes a 16-character GUID into a 8-byte hex
	\param out 8-byte array for the output
	\param in 16-byte string
*/
void alcGetHexGuid(Byte *out, tString in) {
	if (in.size() != 16)
		throw txUnexpectedData(_WHERE("A GUID string must be 16 characters long"));
	tMBuf tmp = alcAscii2Hex(in);
	assert(tmp.size() == 8);
	memcpy(out, tmp.data(), 8);
}

/**
  \brief Converts an hex uid to ascii.
*/
tString alcGetStrUid(const Byte * guid) {
	if(guid==NULL) return "null";

	tString str_guid = alcHex2Ascii(tMBuf(guid,16)), str_guid2;
	str_guid.rewind();
	
	str_guid2.write(str_guid.read(8),8); //ID1
	str_guid2 += "-";
	str_guid2.write(str_guid.read(4),4); //ID2
	str_guid2 += "-";
	str_guid2.write(str_guid.read(4),4); //ID3
	str_guid2 += "-";
	str_guid2.write(str_guid.read(4),4); //ID4
	str_guid2 += "-";
	str_guid2.write(str_guid.read(12),12); //ID5
	assert(str_guid2.size() == 36 && str_guid.tell() == 32);
	return str_guid2;
}

/**
  \brief Converts an Ascii uid to hex
*/
void alcGetHexUid(Byte *out, const tString &passed_guid) {
	if (passed_guid.size() != 36) throw txUnexpectedData(_WHERE("An UID string must be 36 characters long"));
	if (passed_guid.getAt(8) != '-' || passed_guid.getAt(13) != '-' || passed_guid.getAt(18) != '-' || passed_guid.getAt(23) != '-')
		throw txUnexpectedData(_WHERE("There must be dashes between the UID parts"));

	tString guid = passed_guid.substring(0, 8) + passed_guid.substring(9, 4) + passed_guid.substring(14, 4) + passed_guid.substring(19, 4) + passed_guid.substring(24, 12);
	guid = guid.upper();
	assert(guid.size() == 32);
	tMBuf tmp = alcAscii2Hex(guid);
	assert(tmp.size() == 16);
	memcpy(out, tmp.data(), 16);
}

/**
  \brief returns a pointer to a formated time string
*/
tString alcGetStrTime(U32 timestamp, U32 microseconds) {
	char tmptime[26];
	struct tm * tptr;
	time_t stamp = timestamp;

	tptr=gmtime(&stamp);
	strftime(tmptime,25,"%Y:%m:%d-%H:%M:%S",tptr);
	tString str = tmptime;
	str.printf(".%06d", microseconds);
	return str;
}

tString alcGetStrTime(double stamp, const char format) {
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
*/
tString alcHex2Ascii(tMBuf in) {
	tString str;
	for(U32 n=0; n<in.size(); n++) {
		Byte i = in.getAt(n);
		Byte b = ((i & 0xF0)>>4);
		str.putByte(b<0x0A ? b+0x30 : b+(0x41-0x0A));
		str.putByte((i & 0x0F)<0x0A ? (i & 0x0F)+0x30 : (i & 0x0F)+(0x41-0x0A));
	}
	return str;
}

/**
  \brief Converts an ASCII string to hex data
*/
tMBuf alcAscii2Hex(tString in) {
	tMBuf buf;
	for(U32 n=0; n<in.size()/2; n++) {
		Byte i1 = in.getAt(2*n), i2 = in.getAt(2*n+1);
		if ((i1 < 0x41 || i1 > 0x41+25) && (i1 < 0x30 || i1 > 0x30+9))
			throw txUnexpectedData(_WHERE("There is an invalid character in the data: %c", i1));
		if ((i2 < 0x41 || i2 > 0x41+25) && (i2 < 0x30 || i2 > 0x30+9))
			throw txUnexpectedData(_WHERE("There is an invalid character in the data: %c", i2));
		Byte b =  ((i1)<0x3A ? (i1 - 0x30) : (i1 - (0x41-0x0A)));
		buf.putByte((0x10 * b) + ((i2)<0x3A ? (i2 - 0x30) : (i2 - (0x41-0x0A))));
	}
	return buf;
}

/**
	\brief Strips out some characters that win32 doesn't like in file names
*/
tString alcStrFiltered(tString what) {
	tString result;
	for (U32 i = 0; i < what.size(); ++i) {
		Byte c = what.getAt(i);
		if(c!='<' && c!='>' && c!=':' && c!='#' && c!='\\' && c!='/' && c!='*' && c!='?' && c!='"' && c!='\'' && c!='|') {
			result.putByte(c);
		}
	}
	return result;
}


/** \brief parses a "name[number]" kind of string, setting "t" to the name and returning the number */
U16 alcParseKey(tString *t) {
	DBG(9, "alcParseKey() for %s\n", t->c_str());
	int pos;
	pos=t->find('[');
	tString offset;
	if(pos==-1) return 0;
	if(!t->endsWith("]")) {
		throw txParseError(_WHERE("Parse error near %s, malformed var name.\n",t->c_str()));
	}
	offset=t->substring(pos+1,t->size()-pos-1);
	*t = t->substring(0, pos);
	DBG(9, "   result: %s[%s]\n", t->c_str(), offset.c_str());
	return offset.asU16();
}



/** gets the ip address string of a host ip in network byte order
*/
tString alcGetStrIp(U32 ip) {
	in_addr cip;
	cip.s_addr=ip;
	return tString(inet_ntoa(cip));
}


}

