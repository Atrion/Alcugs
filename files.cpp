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

#ifndef _FILE_S_
#define _FILE_S_
//YOU MUST NEVER TOUCH A CVS TAG!
#define _FILE_S_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

//enable/disable the debug
//#define _File_DBG

#include <stdio.h>
#include <string.h>

//Compile Xtea support?
#define _WDYS_SUPPORT

#ifdef _WDYS_SUPPORT

#include "whatdoyousee.h"

#endif

#include "files.h"

#include "debug.h"

//! Return the size of a file
long int get_file_size(char * path) {
	//first get the file size
	struct stat file_stats;
	if(stat(path,&file_stats)!=0) {
		perror("ERR");
		return -1;
	}
	return file_stats.st_size;
}

#ifdef _WDYS_SUPPORT
/**Read an encoded file
    remember to free up the buffer after using it!
		 buf: the buffer that must be a pointer to a char
		 path: it must be the file that you are going to open
		\return The size if all went well, <0 if something wrong happened
*/
int readWDYS(char ** buf, char * path) {

	FILE * f;
	int off=0;
	char out[13];
	int size;

	f=fopen(path,"rb");
	if(f==NULL) { fprintf(stderr,"ERR: An error ocurred trying to read %s\n",path); return -1; }

#ifdef _File_DBG
	printf("Reading %s ",path);
#endif
	fread(out,sizeof(char)*12,1,f);
	out[12]='\0';
	if(strcmp(out,"whatdoyousee")) {
		//fprintf(stderr,"ERR: This is not an Uru \"whatdoyousee\" file, %s\n",out);
	fclose(f);
	return -2;
	}

	fread(&size,sizeof(int),1,f);

#ifdef _File_DBG
	printf("size %i bytes ..",size);
#endif

	*buf=(char *)malloc(sizeof(char)*(size+18));
	if(*buf==NULL) {
		fprintf(stderr,"ERR: Not enough free memory!\n");
		return -1;
	}

	while(!feof(f) && off<size) {
		fread(*buf+off,sizeof(char)*8,1,f);
		decodeQuad((unsigned int *)(*buf + off),(unsigned int *)(*buf + off + 4));
#ifdef _File_DBG
		if(off%800==0) { printf("."); fflush(0); }
#endif
		off+=8;
	}
	fclose(f);
#ifdef _File_DBG
	printf(" decoded %i bytes\n",off);
#endif
	return size;
}

/** Save an encoded file
	\param buf the buffer with the data (buffer size must be 8*n where n is a Natural)
	 size the data size
	 path where do you want to store the file?
	\return the size of the stored file if all went well, elsewhere <0
*/
int saveWDYS(char * buf, int size, char* path) {

	int i;
	FILE * f;

	f=fopen(path,"wb");
	if(f==NULL) { fprintf(stderr,"ERR: An error ocurred trying to write %s\n",path); return -1; }

#ifdef _File_DBG
	printf("Writing %s ",path);
#endif
	fwrite("whatdoyousee",sizeof(char)*12,1,f);
#ifdef _File_DBG
	printf("size %i bytes ..",size);
#endif
	fwrite(&size,sizeof(int),1,f);

	for (i=0; i<size; i+=8) {
		encodeQuad((unsigned int *)(buf + i),(unsigned int *)(buf+i+4));
		fwrite(buf+i,sizeof(char)*8,1,f);
#ifdef _File_DBG
		if(i%800==0) { printf("."); fflush(0); }
#endif
	}
	printf(" encoded %i bytes\n",i);

	fclose(f);
	return i;
}

#endif

/**Read a Normal file
	Remember to free up the memory!
	\param buf a pointer to a char
	 path the source file
	\return size if all is OK, elsewhere <0
*/
int readfile(char ** buf, char * path) {

	FILE * f;
	int ret=0;
	long size;

	f=fopen(path,"rb");
	if(f==NULL) { fprintf(stderr,"ERR: An error ocurred trying to read %s\n",path); return -1; }

#ifdef _File_DBG
	printf("Reading %s ",path);
#endif
	fseek(f,0,SEEK_END);
	size=ftell(f);
	fseek(f,0,SEEK_SET);

#ifdef _File_DBG
	printf(" %ld bytes ..",size);
#endif
	*buf=(char *)malloc(sizeof(char)*(size+18));
	if(*buf==NULL) {
		fprintf(stderr,"ERR: Not enough free memory!\n");
		return -1;
	}

	while(!feof(f)) {
		(*buf)[ret]=(char)fgetc(f);
#ifdef _File_DBG
		if(ret%(8*800)==0) { printf("."); fflush(0); }
#endif
		ret++;
	}
	ret--;
	fclose(f);

#ifdef _File_DBG
	printf(" read %i bytes\n",ret);
#endif

	//while(ret%8!=0) { ret++; m_buffer[ret]='\0'; printf("add-"); }

	return ret;
}


/*Save a normal file
	\param buf the buffer
	 path the source file
	\return size if all is OK, elsewhere <0
*/
int savefile(char * buf, int size, char* path) {

	int i;
	FILE * f;

	f=fopen(path,"wb");
	if(f==NULL) { fprintf(stderr,"ERR: An error ocurred trying to write %s\n",path); return -1; }

#ifdef _File_DBG
	printf("Writing %s size %i ..",path,size);
#endif

	for (i=0; i<size; i++) {
		fputc(buf[i],f);
#ifdef _File_DBG
		if(i%(8*800)==0) { printf("."); fflush(0); }
#endif
	}
#ifdef _File_DBG
	printf(" wrote %i bytes\n",i);
#endif

	fclose(f);
	return i;
}


#endif

