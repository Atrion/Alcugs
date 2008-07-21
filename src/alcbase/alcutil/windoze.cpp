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
	The Alcugs H'uru server also works on prehistoric/inferior systems like windows.

	It's hard, dangerous and complicated to run a server service on an operating system
	that has never been designed for this purpose.

	This code was written mainly because not everybody has access to more than one machine,
	 and Uru currently don't runs very well under Linux.

	Note that several functions of the software are disabled in these versions
	because them are missing in the system. Windows programers are welcome to port
	any missing function or feature that is only available on other Operating Systems.

	This file contains hacks, workarounds, and fixes to attempt running this
	software under a hasecorp system.

	You know windows sucks and linux rocks.
 */

#define __U_WINDOZE_ID $Id$

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include "alcdebug.h"

#if defined(__WIN32__) 
//or defined(__CYGWIN__) - I'm not going to waste more time on this shit, if someone wants to fix it there are two options: A) Use Linux that is superior to Windows. B) Find the problem and fix it.
int gettimeofday(struct timeval *tv, struct timezone *tz) {

	static int firstrun=1;
	if(firstrun==1) {
		firstrun=0;
		DBG(5,"Calibrating clock..\n");
		for(int calibrator=0;calibrator<1000;calibrator++)
			gettimeofday(tv,tz);
		DBG(5,"Clock calibrated..\n");
	}

	static unsigned __int64 count=0;
	static unsigned __int64 frq=0;
	static double freq=0;
	static unsigned int lastSec=0,currentSec=0,sec=0;

	static unsigned __int64 usec=0,rusec=0,last_usec=0,offset=0;

	if(frq==0) { //only once
		QueryPerformanceFrequency((LARGE_INTEGER *) &frq);
		freq=frq/1000000;
		//currentSec=((unsigned int)time(NULL) % 60); //get the current second (first time)
	}

	QueryPerformanceCounter((LARGE_INTEGER *) &count);
	lastSec=currentSec; //set the last second
	currentSec=((unsigned int)time(NULL) % 60); //get the current second

	last_usec=usec; //set last microseconds counter
	usec = (unsigned __int64)(count/freq); //get current microseconds count
	rusec = (usec) % 1000000;
	usec += offset; //correct the offset

	sec = (usec/1000000) % 60;
	usec = (usec) % 1000000;

	if(lastSec<currentSec && last_usec<usec) {
		offset = 1000000 - rusec; //update the offset
		DBG(2,"offset is now %I64d\n",offset);
		usec=0;
	}

	DBG(3,"%i.%06I64d %I64d %I64d\n",sec,usec,count,frq);
	DBG(5,"currentSec is %i\n",currentSec);
	tv->tv_sec=currentSec;
	tv->tv_usec=usec;
	return 0;
}
#endif

#ifdef __WIN32__

// if you are a windows programer, please fill these functions with the missing code
// i'm not going to waste more time searching for an equivalent. It's not my fault
// that windows sucks a lot.

int getuid(void) { return 10000; }
int daemon(int a,int b) { return 0; }
unsigned int alarm(unsigned int sec) { return 0; }

//strsep has to be rewritten. sometimes it causes errors.

char *strsep(char **pcadena, const char *delim)
{
	if(**pcadena==0)
		return NULL;

	char * what=*pcadena;
	while(**pcadena!=0 && **pcadena!=*delim)
	{
		(*pcadena)++;
	}

	if(**pcadena==0)
	{
		**pcadena=0;
		(*pcadena)++;
	}
	else
		**pcadena=0;

	return what;
}

char *getpass( const char * prompt ) {
	// I though that windows had something for asking passwords in the console.
	puts(prompt);
	static char what[500];
	int str_len;
	fflush(0);
	fgets(what,500,stdin);
	str_len=strlen(what);
	if(what[str_len-1]=='\n') {
		what[str_len-1]='\0';
		str_len--;
	}
	return what;
}


#endif

