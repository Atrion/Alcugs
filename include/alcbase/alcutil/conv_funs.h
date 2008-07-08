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
const Byte * alcGetStrGuid(const Byte * guid);
/** creates a 00000000-0000-0000-0000-000000000000 valid guid
		\param uid A 16 bytes user id
		\return A 36 bytes str formated id
*/
const Byte * alcGetStrUid(Byte * guid);
/** \param uid A 36 bytes str user id
		\return A 16 bytes hex user id
*/
const Byte * alcGetHexUid(Byte * guid);
/**
		\returns a pointer to a formated time string
*/
const Byte * alcGetStrTime(U32 timestamp, U32 microseconds);
const Byte * alcGetStrTime(double stamp=0, const char format='s');

/** Gets an ascii string from hex values
	\param out A buffer of at least 2*sizeof(in) bytes
	\param in Input buffer
	\param size size of the input buffer
*/
void alcHex2Ascii(Byte * out, const Byte * in, int size);
/** Encodes in Hex, an hex string
	\param out Output buffer of at least sizeof(in)/2 bytes
	\param in Input buffer, must be 2*sizeof(out)
	\param size size of the output buffer
*/
void alcAscii2Hex(Byte * out, const Byte * in, int size);

/*------------------------------------------------------------
  De/Encodes the specific UruString associated to a buffer.
	The result will be put on out, and it must have the required
	size to host it!!!
	The size of the string is returned, so be sure to add 2
	to continue moving throught the buffer!!!
	 how
	  0x00 -> Normal string
		0x01 -> Invert Bits string
-------------------------------------------------------------*/
/** Decode an urutring
		\param out A buffer, with a size of max_size
		\param in Input buffer
		\param max_size Maxium size that the output buffer can host
		\return The string size (remember to add 2 bytes)
*/
int alcDecodeUStr(unsigned char* out, unsigned char* in, U16 max_size);
/** Encode an urutring
		\param out A buffer, with a size of size+2
		\param in Input buffer
		\param size Number of bytes to be coded
		\param how 0x00 normal, 0x01 Inverted
		\return The out buffer size, that it's size+2
*/
int alcEncodeUStr(unsigned char* out, unsigned char* in, U16 size, Byte how);

/**
		Checks buffer for Byte
*/
char alcCheckBuf(Byte * buf, Byte car, int n);
/**
	Strips out some characters that win32 doesn't like..
	(Overwrittes what)
*/
void alcStrFilter(Byte * what);

}

#endif
