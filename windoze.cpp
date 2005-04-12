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

#define __U_WINDOZE_ID $Id: stdebug.h,v 1.2 2004/12/02 23:09:36 almlys Exp $

#ifdef __WIN32__

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>

#ifndef __MSVC__
#  include <sys/time.h>
#endif

#include "windoze.h"

#include "debug.h"


//TODO: This function is broken.
// Reason: Windows is crap, shit, garbage, and is not able to have the precision that any other decent OS like MAC or Linux has.
// Don't use Windows for things that you need extreme precission.
// If you are able to get a working function, in a way that uruping don't shows 0 microseconds, or negative values pinging to the localhost, you will be my hero.

//Update: Well, now seems to work, but there are also issues...

int gettimeofday(struct timeval *tv, struct timezone *tz) {

	//candiate 1
#if 0
	tv->tv_sec=0;
	tv->tv_usec=0;
#endif

	//candidate 2
#if 0
	SYSTEMTIME tmp;

	GetSystemTime(&tmp);

	tv->tv_sec  = tmp.wSecond;
	tv->tv_usec = tmp.wMilliseconds*1000;
#endif

	/* ARGGGHHH!!!  Not enough precission, windows is a big piece of shit... */

	//candidate 3
#if 0
	//Thanks to The.Modificator for the next piece of code.
	//But it don't has the resolution required for our purposes.
	ULARGE_INTEGER tmp;
	unsigned long long int test = 0x019db1ded53e8000LL;

	DBG(5,"test is %I64d, %016I64X\n",test,test);

	GetSystemTimeAsFileTime((FILETIME*)&tmp);

	if(tmp.QuadPart < test) {
		memset(tv,0, sizeof(timeval));
		return -1; //this should not happen... (well - at least when you don't use anything <1970 as your system date. :P )
	}

	DBG(5,"here tmp is %I64d, %016I64X...\n",tmp.QuadPart,tmp.QuadPart);
	tmp.QuadPart -= test;
	DBG(5,"here tmp is %I64d, %016I64X...\n",tmp.QuadPart,tmp.QuadPart);
	tmp.QuadPart = tmp.QuadPart/10;
	DBG(5,"here tmp is %I64d, %016I64X...\n",tmp.QuadPart,tmp.QuadPart);

	tv->tv_sec = ((tmp.QuadPart / 1000000) % 60); // <- SEGFAULT here
	DBG(0,"tv->tv_sec is %i\n",tv->tv_sec);

	DBG(0,"yep... tmp.QuadPart is %I64d\n",tmp.QuadPart);
	tv->tv_usec = ((tmp.QuadPart) % 1000000);
	DBG(0,"tv->tv_usec is %06i\n",tv->tv_usec);
	//
#endif

	//candidate 4 - The winer. Say thanks to Almlys for wasting a whole week figuring it out..
#if 1

//int i;

//for(i=0; i<5000; i++) {

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

	/*
	DBG(3," %I64d > %I64d?\n",last_usec,usec);
	if(last_usec>usec) {
		DBG(3," %i = %i ?\n",lastSec,currentSec);
		if(currentSec==lastSec) { //it's out of Sync...
			offset = last_usec - usec; //update the offset
			DBG(0,"offset is now %I64d\n",offset);
		} //else ok
	}
	*/

	if(lastSec<currentSec && last_usec<usec) {
		offset = 1000000 - rusec; //update the offset
		DBG(2,"offset is now %I64d\n",offset);
		usec=0;
	}

	DBG(3,"%i.%06I64d %I64d %I64d\n",sec,usec,count,frq);
	DBG(5,"currentSec is %i\n",currentSec);
	//usleep(10); <- windows starts to suck more and more again
	//int e;
	//for(e=0; e<20000000; e++);

	//tv->tv_sec=sec;
	tv->tv_sec=currentSec;
	tv->tv_usec=usec;

//}
	/*
	DBG(0,"tv->tv_sec is %i\n",tv->tv_sec);
	DBG(0,"tv->tv_usec is %06i\n",tv->tv_usec);
	*/

#endif
	//DWORD GetTickCount(void);
	//DBG(0,"let's see %i\n",GetTickCount()); //<-- that one also sucks

	return 0;
}

//TODO, if you are a windows programer, please fill these functions with the missing code
// i'm not going to waste more time searching for an equivalent. It's not my fault
// that windows sucks a lot.

int getuid(void) { return 10000; }
int daemon(int a,int b) { return 0; }
unsigned int alarm(unsigned int sec) { return 0; }

///TODO porting: strsep has to be rewritten. sometimes it causes errors.

char *strsep(char **pcadena, const char *delim) {
	char * what=*pcadena;
	while(**pcadena!=0 && **pcadena!=*delim) {
		*pcadena++;
	}
	if(**pcadena==0) return NULL;
	else return what;
}


#endif
