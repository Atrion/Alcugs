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

/* Packet injector and mangler */

/*

Reads a packet from the filesystem, and injects it to the client in raw mode.

It also rewrites all needed headers before sending it.

 --*/

#ifndef __U_INJECTOR_
#define __U_INJECTOR_
/* CVS tag - DON'T TOUCH*/
#define __U_INJECTOR_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "injector.h"

/*------------------------------------------
  Loads the packet into the buffer
	Warning with the SEGFAULT's
	and returns the size of the loaded buffer
-------------------------------------------*/
int load_packet(Byte * buf, const char * packet) {
	const char * f_path="raw_src/";
	FILE * f;
	char * path;
	int i=0;
	char mychar;

	path = (char *)malloc((strlen(packet) + strlen(f_path)+1)*sizeof(char));
	strcpy(path,(const char *)f_path);
	strcat(path,packet);

	f=fopen(path,"rb");
	free(path);
	if(f==NULL) {
		fprintf(f_err,"ERR: Fatal - Cannot Open %s to read \n",path);
		perror("returned error");
		return 0;
	}

	while(!feof(f)) {
		mychar=fgetc(f);
		if(!feof(f)) {
			buf[i]=mychar;
			i++;
		}
	}

	fclose(f);
	return i;
}

/*---------------------------------------------------------
	Stores a packet in the filesystem
---------------------------------------------------------*/

int store_packet(Byte * buf, int n, const char * packet) {
	const char * f_path="raw_wrt/";
	FILE * f;
	char * path;
	int i=0;
	//char mychar;

	path = (char *)malloc((strlen(packet) + strlen(f_path)+1)*sizeof(char));
	strcpy(path,(const char *)f_path);
	strcat(path,packet);

	f=fopen(path,"wb");
	free(path);
	if(f==NULL) {
		fprintf(f_err,"ERR: Fatal - Cannot Open %s to write \n",path);
		perror("returned error");
		return 0;
	}

	for(i=0; i<n; i++) {
		fputc(buf[i],f);
	}

	fclose(f);
	return n;
}


#endif

