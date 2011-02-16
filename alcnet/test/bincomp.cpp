/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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
 
/** Tiny helper application to check whether two binary files are equal */
#include <alcdefs.h>
#include <alcmain.h>
#include <alclicense.h>
#include <alcversion.h>
#include <alcexception.h>

#include <cstring>

using namespace alc;

void usage() {
	printf("Usage:\nbincomp file1 file2");
	fflush(0);
}

//main
int main(int argc, char * argv[])
{
	printf("This is the Alcugs binary compare helper\n\n");
	
	tAlcMain alcMain("BinComp");
	try {
	
	puts(alcVersionText());
	
	if(argc>1 && !strcmp(argv[1],"-l")) {
		puts(alcLicenseText());
		return 0;
	} else if((argc>1 && !strcmp(argv[1],"-h")) || argc != 3) {
		usage();
	}
	
	tFBuf f1(argv[1]), f2(argv[2]);
	if (f1.size() != f2.size()) return 1; // files are different
	
	const int maxChunk = 4*1024;
	while (!f1.eof()) {
		int size = f1.remaining();
		if (size > maxChunk) size = maxChunk;
		if (memcmp(f1.read(size), f2.read(size), size) != 0) return 1; // there was a difference here
	}
	if (!f2.eof()) return 1; // sizes are different (should not happen because we checked above, but who knows)
	return 0; // files are equal!

	
	} catch(txBase &t) {
		printf("Exception: %s\n",t.what());
		puts(t.backtrace());
	} catch(...) {
		printf("ERROR: Unknown exception...\n");
	}
	
  return 1; // if we got here, something went wrong
	
}

