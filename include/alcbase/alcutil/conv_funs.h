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

/*
  Conversion between different data types
*/

#ifndef __U_CONV_FUNS_H
#define __U_CONV_FUNS_H
#define __U_CONV_FUNS_H_ID $Id$

namespace alc {

/** creates a 0000000000000000 valid guid
		\param guid A hex guid
		\return A str guid, twice as long as the hex guid
*/
const char * alcGetStrGuid(const Byte * guid);

/** creates a 00000000-0000-0000-0000-000000000000 valid guid
		\param guid A 16 bytes user id
		\return A 36 bytes str formated id
*/
const char * alcGetStrUid(const Byte * guid);

/** \param passed_guid A 36 bytes str user id
	\return A 16 bytes hex user id
*/
const Byte * alcGetHexUid(const char * passed_guid);

/**
		\returns a pointer to a formated time string
*/
tString alcGetStrTime(U32 timestamp, U32 microseconds);
tString alcGetStrTime(double stamp=0, const char format='s');

/** Gets an ascii string from hex values
	\param out A buffer of at least 2*sizeof(in) bytes
	\param in Input buffer
	\param size size of the input buffer
*/
void alcHex2Ascii(char * out, const Byte * in, int size);

/** Encodes in Hex, an hex string
	\param out Output buffer of at least sizeof(in)/2 bytes
	\param in Input buffer, must be 2*sizeof(out)
	\param size size of the output buffer
*/
void alcAscii2Hex(Byte * out, const char * in, int size);


/**
	Strips out some characters that win32 doesn't like..
	(Overwrittes what)
*/
void alcStrFilter(char * what);

/** parses a "name[number]" kind of string, setting "t" to the name and returning the number */
U16 alcParseKey(tString *t);


/** convert pageIDs to pageNumbers and the other way around - wired, but whatever... */
inline U16 alcPageIdToNumber(U32 seqPrefix, U32 pageId)
{
	return pageId - (seqPrefix << 8) - 33;
}
inline U32 alcPageNumberToId(U32 seqPrefix, U16 number)
{
	return (seqPrefix << 8) + 33 + number;
}

}

#endif
