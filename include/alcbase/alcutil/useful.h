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

#ifndef __U_USEFUL_H
#define __U_USEFUL_H
/* CVS tag - DON'T TOUCH*/
#define __U_USEFUL_H_ID "$Id$"

namespace alc {

const char * alcConsoleAsk();
int alcGetLoginInfo(char * argv,char * hostname,char * username,U16 * port,char * avie);

U32 alcGetTime();
U32 alcGetMicroseconds();
double alcGetCurrentTime(const char format='s');
char alcIsAlpha(int c);

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

// allow us to use some string functions with arguments of type Byte *
inline char *strcpy(const void *to, const void *from)
{
	return strcpy((char *)to, (char *)from);
}
inline char *strncpy(const void *to, const void *from, size_t n)
{
	return strncpy((char *)to, (char *)from, n);
}
inline int strcmp(const void *a, const void *b)
{
	return strcmp((char *)a, (char *)b);
}
inline int strncmp(const void *a, const void *b, size_t n)
{
	return strncmp((char *)a, (char *)b, n);
}

#endif

