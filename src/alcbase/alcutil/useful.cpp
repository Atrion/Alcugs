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
const char * alcConsoleAsk() {
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

/**
	\brief "Transforms a  username#avie@host:port into hostname, username, avie and port"
*/
int alcGetLoginInfo(const char * argv,char * hostname,char * username,U16 * port,char * avie) {
	unsigned int i;

	int a=0,b=0,c=0;
	int q=0;

	char left[100]="";
	char mid[100]="";
	char right[100]="";
	char user[100]="";

	for(i=0; i<strlen(argv); i++) {

		if(argv[i]=='@') { q=1; }
		else if(argv[i]==':') { q=2; }
		else {

			switch (q) {
				case 0:
					left[a]=argv[i];
					a++;
					break;
				case 1:
					mid[b]=argv[i];
					b++;
					break;
				case 2:
					right[c]=argv[i];
					c++;
					break;
				default:
					return -1;
			}
		}

	}
	left[a]='\0';
	mid[b]='\0';
	right[c]='\0';

	switch (q) {
		case 0:
			alcStrncpy(hostname,left,99);
			break;
		case 1:
			alcStrncpy(user,left,99);
			alcStrncpy(hostname,mid,99);
			break;
		case 2:
			if(b!=0) {
				alcStrncpy(user,left,99);
				alcStrncpy(hostname,mid,99);
			} else {
				alcStrncpy(hostname,left,99);
			}
			*port=atoi(right);
			break;
	}

	//check for avie
	a=0; b=0; q=0;
	for(i=0; i<strlen(user); i++) {

		if(user[i]=='#') { q=1; }
		else {
			switch (q) {
				case 0:
					left[a]=user[i];
					a++;
					break;
				case 1:
					mid[b]=user[i];
					b++;
					break;
			}
		}
	}
	left[a]='\0';
	mid[b]='\0';

	switch (q) {
		case 0:
			if (username != 0)
				alcStrncpy(username,left,99);
			break;
		case 1:
			if (username != 0)
				alcStrncpy(username,left,99);
			if (avie != 0)
				alcStrncpy(avie,mid,99);
			break;
	}

	return 1;
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
  return(index("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZáàâä\
éèêëíìîïóòôöúùûüçÇñÑß", c)!=NULL); //<- Some characters are not visible on UTF systems, or under other codifications, so you may see garbage
#endif
}


}

#endif

