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

//#define _DBG_LEVEL_ 10

//Program vars
#define IN_ALC_PROGRAM
#define ALC_PROGRAM_ID "$Id$"

#include<alcugs.h>
#include<alcdebug.h>

using namespace alc;

void usage() {
	printf("Usage: [--version] [--cxxflags] [--cppflags] [--libs] [-l] [-V]\n");
	fflush(0);
}

//main
int main(int argc, char * argv[]) {
	alcInit(argc,argv,true);
	int i;
	if(argc<2) {
		usage();
		return -1;
	}
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) {
			usage(); return -1;
		} else if(!strcmp(argv[i],"-V")) {
			usage();
			puts(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-l")) {
			usage();
			puts(alcVersionTextShort());
			puts(alcLicenseText());
			return -1;
		} else if(!strcmp(argv[i],"--version")) {
			printf("%i.%i.%i\n",alcGetMaxVersion(),alcGetMinVersion(),alcGetRelVersion());
		} else if(!strcmp(argv[i],"--cxxflags")) {
			printf("%s\n",ALC_CXXFLAGS);
		} else if(!strcmp(argv[i],"--cppflags")) {
			printf("-I%s/alcugs %s\n",ALC_INCLUDEPATH,ALC_CPPFLAGS);
		} else if(!strcmp(argv[i],"--libs")) {
			printf("-L%s -lalcnet1 -lalcbase1 %s\n",ALC_LIBSPATH,ALC_LIBS);
		} else {
			usage();
			return -1;
		}
	}
	return 0;
}
