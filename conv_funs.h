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

/*
  Conversion between different data types
*/

#ifndef __U_CONV_FUNS_H_
#define __U_CONV_FUNS_H_
#define __U_CONV_FUNS_H_ID $Id$

#include "data_types.h" //for Byte, and others

//creates a 0000000000000000 valid guid
const Byte * get_guid(Byte * guid);
//creates a 00000-0000-0000-0000-000000000000 valid guid
const Byte * create_str_guid(Byte * guid);
Byte * str_guid_to_hex(Byte * guid);

//returns a pointer to a formated time string
Byte * get_stime(U32 timestamp, U32 microseconds);

/*------------------------------------------
  Conversion functions for the
	 MD5-Champ challenge used in Uru
	 hex2ascii out must be 2*in
	 ascii2hex in must be 2*out
	 REMEMBER, SIZE of the smallest string!!!
-------------------------------------------*/
void hex2ascii2(Byte * out, Byte * in, int size);
void ascii2hex2(Byte * out, Byte * in, int size);

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
int decode_urustring(unsigned char* out, unsigned char* in, U16 max_size);
int encode_urustring(unsigned char* out, unsigned char* in, U16 size, Byte how);

char check_buf(Byte * buf, Byte car, int n);

//Strips out some characters that win32 doesn't like..
void str_filter(Byte * what);

#endif
