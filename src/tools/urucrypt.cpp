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
#ifdef M5CRYPT
#define ALC_PROGRAM_NAME "M5Crypt"
#else
#define ALC_PROGRAM_NAME "UruCrypt"
#endif
#define ALC_PROGRAM_VERSION "2.1"

#include<alcugs.h>

#include<alcdebug.h>

using namespace alc;

void usage() {
#ifdef M5CRYPT
	const char * p="m5crypt";
#else
	const char * p="urucrypt";
#endif
	printf("Usage:\nFor decrypt: \n\
 %s d source.sdl destination.sdl.dec\n\
For encrypt:\n\
 %s e source.sdl.dec destination.sdl\n\
-l Shows license text\n",p,p);
	fflush(0);
}

//main
int main(int argc, char * argv[]) {

#ifdef M5CRYPT
	printf("This is the Myst 5 Rijndael encoder/decoder\n\n");
#else
	printf("This is the Uru \"whatdoyousee\" encoder/decoder\n\n");
#endif
	
	tAlcMain alcMain;
	try {
	
	puts(alcVersionText());
	
	if(argc>1 && !strcmp(argv[1],"-l")) {
		puts(alcLicenseText());
		return 0;
	} else if(argc>1 && !strcmp(argv[1],"-h")) {
		usage();
	}

	tFBuf f1;
#ifdef M5CRYPT
	tAESBuf w1;
	w1.setM5Key();
#else
	tWDYSBuf w1;
#endif
	
	char xap[1024];
	
	//new feature :)
	if(argc==2) {
		f1.open(argv[1]);
		w1.put(f1);
		f1.close();
		try {
			w1.decrypt();
			if(w1.size()>0) {
				alcStrncpy(xap,argv[1],1000);
				strcat(xap,".dec");
				f1.open(xap,"wb");
				f1.put(w1);
				f1.close();
			}
		} catch(txUnexpectedData &t) {
			w1.encrypt();
			if(w1.size()>0) {
				alcStrncpy(xap,argv[1],1000);
				strcat(xap,".cry"); //Jaffa CRE!!
				f1.open(xap,"wb");
				f1.put(w1);
				f1.close();
			}
		}
		return 0;
	}

  if(argc!=4) {
		usage();
		return -1;
  }
	
	char what;

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
		f1.open(argv[2]);
		w1.put(f1);
		f1.close();
		w1.decrypt();
		f1.open(argv[3],"wb");
		f1.put(w1);
		f1.close();
  } else if(what==2) {
		f1.open(argv[2]);
		w1.put(f1);
		f1.close();
		w1.encrypt();
		f1.open(argv[3],"wb");
		f1.put(w1);
		f1.close();
  }

  return 0;
	
	} catch(txBase &t) {
		printf("Exception: %s\n",t.what());
		puts(t.backtrace());
	} catch(...) {
		printf("ERROR: Unknown exception...\n");
	}
	
}

