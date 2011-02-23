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

/**
	Useful helper functions
*/

#ifndef __U_USEFUL_H
#define __U_USEFUL_H
/* CVS tag - DON'T TOUCH*/
#define __U_USEFUL_H_ID "$Id$"

#include "alctypes.h"

namespace alc {
	
	class tString;
	class tMBuf;

	tString alcConsoleAsk();
	bool alcGetLoginInfo(tString argv,tString * username,tString * hostname,U16 * port); // username may be NULL, the rest not

	U32 alcGetTime();
	U32 alcGetMicroseconds();
	double alcGetCurrentTime(const char format='s');
	bool alcIsAlpha(char c);

	void setCloseOnExec(int fd);

	/**
			\returns a pointer to a formated time string
	*/
	tString alcGetStrTime(U32 timestamp, U32 microseconds);
	tString alcGetStrTime(double stamp=0, const char format='s');


	/** Gets an ascii string from hex values
	*/
	tString alcHex2Ascii(tMBuf in);

	/** Encodes in Hex, an hex string
	*/
	tMBuf alcAscii2Hex(tString in);


	// These are mostly special cases of the above to, for compatability with Byte arrays
	/** creates a 0000000000000000 valid guid
			\param guid A hex guid (8 characters)
			\return A str guid, twice as long as the hex guid
	*/
	tString alcGetStrGuid(const Byte * guid);

	/** encodes a 16-character GUID into a 8-byte hex
		\param out 8-byte array for the output
		\param in 16-byte string
	*/
	void alcGetHexGuid(Byte *out, tString in);

	/** creates a 00000000-0000-0000-0000-000000000000 valid guid
			\param guid A 16 bytes user id
			\return A 36 bytes str formated id
	*/
	tString alcGetStrUid(const Byte * guid);

	/** \param passed_guid A 36 bytes str user id
		\return A 16 bytes hex user id
	*/
	void alcGetHexUid(Byte *out, const tString &passed_guid);


	/**
		Strips out some characters that win32 doesn't like..
		(Overwrittes what)
	*/
	tString alcStrFiltered(tString what);

	/** parses a "name[number]" kind of string, setting "t" to the name and returning the number */
	U16 alcParseKey(tString *t);

	/** Convert an IP-address to a string */
	tString alcGetStrIp(U32 ip);


	/** convert pageIDs to pageNumbers and the other way around - wired, but whatever... */
	inline U16 alcPageIdToNumber(U32 seqPrefix, U32 pageId)
	{
		return pageId - (seqPrefix << 8) - 33;
	}
	inline U32 alcPageNumberToId(U32 seqPrefix, U16 number)
	{
		return (seqPrefix << 8) + 33 + number;
	}

	//These macros are the inverse of htonl() etc. They take little-endian
	// numbers and put them in host order or vice-versa. The htonl() macros
	// cannot be used because they are #defined to no-ops on big-endian machines.
	//Some OSes define htole macros.
	#if !defined(htole16)
		#if defined(WORDS_BIGENDIAN)
			#define htole16(x) \
				(((x) >> 8 & 0x00ff) | ((x) << 8 & 0xff00))
			#define htole32(x) \
				(((x) >> 24 & 0x000000ff) | ((x) >> 8 & 0x0000ff00) | \
				((x) << 8 & 0x00ff0000) | ((x) << 24 & 0xff000000))
		#else
			#define htole16(x) (x)
			#define htole32(x) (x)
		#endif
	#endif
	#if !defined(letoh16)
		#define letoh16(x) htole16(x)
		#define letoh32(x) htole32(x)
	#endif

}


#endif

