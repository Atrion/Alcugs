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

#define _UruCrypt_ID "$Id$"
#define _MAYOR_ 1
#define _MINOR_ 3

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "files.h"
#include "license.h"


void usage() {
	printf("Usage:\nFor decrypt: \n\
 urucrypt d source.sdl destination.sdl.dec\n\
For encrypt:\n\
 urucrypt e source.sdl.dec destination.sdl\n");
	fflush(0);
}

void version() {
	printf("UruCrypt %i.%i build: %s\r\n",_MAYOR_,_MINOR_,_UruCrypt_ID);
	fflush(0);
}

//main
int main(int argc, char * argv[]) {
	char * buf; //the buffer
	long size;
	int what=0;
	char xap[500]; //xip & xap?

	buf=NULL; //avoid a SEGFAULT

	printf("This is the Uru \"whatdoyousee\" encoder/decoder\n\n");
	version();
	if(license_check(stdout,argc,argv)==1) {
		return -1;
	}
	//printf("%s",__uru_disclaimer_short);
	fflush(0);


	//new feature :)
	if(argc==2) {
		size=readWDYS(&buf,argv[1]);
		if(size==-2) {
			size=readfile(&buf,argv[1]);
			if(size>0) {
				strcpy(xap,argv[1]);
				strcat(xap,".cry"); //Jaffa CRE!!
				saveWDYS(buf,size,xap);
			}
		} else {
			if(size>0) {
				strcpy(xap,argv[1]);
				strcat(xap,".dec");
				savefile(buf,size,xap);
			}
		}
		return 0;
	}

  if(argc!=4) {
		usage();
		return -1;
  }

  if(!strcmp(argv[1],"d")) {
    what=1;
  } else if(!strcmp(argv[1],"e")) {
    what=2;
  } else {
    usage();
		return 1;
  }
  fflush(0);

	if(what==1) {
		size=readWDYS(&buf,argv[2]);
		savefile(buf,size,argv[3]);
  } else if(what==2) {
		size=readfile(&buf,argv[2]);
		saveWDYS(buf,size,argv[3]);
  }

	if(buf!=NULL) {
		free((void *)buf); //free the buffer
	}
  return 0;
}


