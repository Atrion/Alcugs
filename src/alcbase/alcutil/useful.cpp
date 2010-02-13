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

#ifndef __U_USEFUL_
#define __U_USEFUL_
/* CVS tag - DON'T TOUCH*/
#define __U_USEFUL_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include <sys/time.h>

#include "alcdebug.h"

namespace alc {

/**
	Waits for user input, blocks until user has not entered something followed by return
*/
tString alcConsoleAsk() {
	char what[501];
	fflush(0);
	fgets(what,500,stdin);
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
char alcIsAlpha(int c) {
#ifdef __WIN32__
	return(IsCharAlpha(c));
#else
  return(index("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ·‡‚‰\
ÈËÍÎÌÏÓÔÛÚÙˆ˙˘˚¸Á«Ò—ﬂ", c)!=NULL); //<- Some characters are not visible on UTF systems, or under other codifications, so you may see garbage
#endif
}


}

#endif

