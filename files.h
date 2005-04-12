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

#ifndef _FILE_S_H_
#define _FILE_S_H_
//YOU MUST NEVER TOUCH A CVS TAG!
#define _FILE_S_H_ID "$Id$"

//Compile Xtea support?
#define _WDYS_SUPPORT

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#ifndef __MSVC__
#  include <unistd.h>
#endif

//! Returns the size of a file
long int get_file_size(char * path);


#ifdef _WDYS_SUPPORT
/**Read an encoded file
    remember to free up the buffer after using it!
		 buf: the buffer that must be a pointer to a char
		 path: it must be the file that you are going to open
		\return The size if all went well, <0 if something wrong happened
*/
int readWDYS(char ** buf, char * path);

/** Save an encoded file
	\param buf the buffer with the data (buffer size must be 8*n where n is a Natural)
	 size the data size
	 path where do you want to store the file?
	\return the size of the stored file if all went well, elsewhere <0
*/
int saveWDYS(char * buf, int size, char* path);

#endif

/**Read a Normal file
	Remember to free up the memory!
	\param buf a pointer to a char
	 path the source file
	\return size if all is OK, elsewhere <0
*/
int readfile(char ** buf, char * path);

/*Save a normal file
	\param buf the buffer
	 path the source file
	\return size if all is OK, elsewhere <0
*/
int savefile(char * buf, int size, char* path);


#endif

