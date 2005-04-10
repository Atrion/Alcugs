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

#ifndef __U_USEFUL_
#define __U_USEFUL_
/* CVS tag - DON'T TOUCH*/
#define __U_USEFUL_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include "useful.h"

#include "debug.h"

/**
	Waits for user input, blocks until user has not entered something followed by return
*/
char * ask() {
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
	Transforms a  username#avie@host:port  into hostname, username, avie and port
*/
int get_host_info(char * argv,char * hostname,char * username,U16 * port,char * avie) {
	unsigned int i;

	int a=0,b=0,c=0;
	int q=0;

	char left[100]="";
	char mid[100]="";
	char right[100]="";

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
			strcpy(hostname,left);
			break;
		case 1:
			strcpy(username,left);
			strcpy(hostname,mid);
			break;
		case 2:
			if(b!=0) {
				strcpy(username,left);
				strcpy(hostname,mid);
			} else {
				strcpy(hostname,left);
			}
			*port=atoi(right);
			break;
	}

	//check for avie
	a=0; b=0; q=0;
	for(i=0; i<strlen(username); i++) {

		if(username[i]=='#') { q=1; }
		else {
			switch (q) {
				case 0:
					left[a]=username[i];
					a++;
					break;
				case 1:
					mid[b]=username[i];
					b++;
					break;
			}
		}
	}
	left[a]='\0';
	mid[b]='\0';

	switch (q) {
		case 0:
			strcpy(username,left);
			break;
		case 1:
			strcpy(username,left);
			strcpy(avie,mid);
			break;
	}

	return 1;
}

/**
	Quick way to get microseconds
*/
U32 get_microseconds() {
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return tv.tv_usec;
}

/** Gets the current time as double*/
double get_current_time() {
	return ((double)time(NULL) + ((double)get_microseconds() / 1000000));
}


#endif

