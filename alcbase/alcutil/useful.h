/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

#include <sys/types.h> // necessary on some systems to get ssize_t
#include <stdint.h>

namespace alc {
	
	class tString;
	class tMBuf;

	tString alcConsoleAsk();
	bool alcGetLoginInfo(tString argv,tString * username,tString * hostname,uint16_t * port); // username may be NULL, the rest not
	
	bool alcIsAlpha(char c);

	void setCloseOnExec(int fd);


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
	tString alcGetStrGuid(const void * guid);

	/** encodes a 16-character GUID into a 8-byte hex
		\param out 8-byte array for the output
		\param in 16-byte string
	*/
	void alcGetHexGuid(void *out, tString in);

	/** creates a 00000000-0000-0000-0000-000000000000 valid guid
			\param guid A 16 bytes user id
			\return A 36 bytes str formated id
	*/
	tString alcGetStrUid(const void * guid);

	/** \param passed_guid A 36 bytes str user id
		\return A 16 bytes hex user id
	*/
	void alcGetHexUid(void *out, const tString &passed_guid);


	/**
		Strips out some characters that win32 doesn't like.
	*/
	tString alcStrFiltered(tString what);

	/** parses a "name[number]" kind of string, setting "t" to the name and returning the number */
	unsigned int alcParseKey(tString *t);

	/** Convert an IP-address to a string */
	tString alcGetStrIp(uint32_t ip);

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
	
	// macro to make copying a class impossible
	#define FORBID_CLASS_COPY(name) private: \
		name(const name &); \
		void operator=(const name &);
	
	/** GNUC has the ability to check format strings that follow the syntax used in printf and others.
	Hide the differences between different compilers in this GNUC_FORMAT_CHECK macro.
	@param archetype one of: printf, scanf, strftime or strfmon
	@param string_index specifies which argument is the format string argument (starting from 1)
	@param first_to_check is the number of the first argument to check against the format string
	Copied from Wireshark sources. */
	#if __GNUC__ >= 2
		#define GNUC_FORMAT_CHECK(archetype, string_index, first_to_check) __attribute__((format (archetype, string_index, first_to_check)))
	#else
		#define GNUC_FORMAT_CHECK(archetype, string_index, first_to_check)
	#endif

}


#endif

