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

/** Alcugs debbuging interface (the old debug.h/debug.cpp)

	The next MACROS / functions are defined
	_WHERE(msg) returns filename,function,line and a message
	DBG(level,args...) debug message
	ERR(level,args...) error message, also calls perror()
	_DIE(msg) shows a message, and aborts


*/

#ifndef __U_ALCDEBUG_H
#define __U_ALCDEBUG_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCDEBUG_H_ID "$Id$"

#ifdef __cplusplus
extern "C" {
using namespace std
#endif

#ifdef _DBG_LEVEL_
 #if (_DBG_LEVEL_ == 0)
  #ifdef _DEBUG_
   #undef _DEBUG_
  #endif
 #endif
#endif //_DBG_LEVEL_


#ifdef _DEBUG_

#ifndef _DBG_LEVEL_
#define _DBG_LEVEL_ 0
#endif //_DBG_LEVEL_

#ifdef __MSVC__
//this looks like crap, i know... but it should work.

typedef void(*_DBGorERR_pointer)(int a, char *msg, ...);
//void _DBGorERR(int a,...);

#if (_MSC_VER <= 1200)
	//MSVC Version 6 or lower (doesn't support __FUNCTION__)


	_DBGorERR_pointer _DBG_before(int dbglvl, char *file, int line);
	_DBGorERR_pointer _ERR_before(int dbglvl, char *file, int line);

	#define DBG _DBG_before(_DBG_LEVEL_,__FILE__,__LINE__) 
	#define ERR _ERR_before(_DBG_LEVEL_,__FILE__,__LINE__) 

	#define _WHERE(a) __dbg_where(a,__FILE__,__LINE__)

	char * __dbg_where(const char * a,const char * b,int d);

#else
	//MSVC Version > 6 (should support __FUNCTION__)

	_DBGorERR_pointer _DBG_before(int dbglvl, char *file, char *function, int line);
	_DBGorERR_pointer _ERR_before(int dbglvl, char *file, char *function, int line);

	#define DBG _DBG_before(_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__) 
	#define ERR _ERR_before(_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__) 

	#define _WHERE(a) __dbg_where(a,__FILE__,__FUNCTION__,__LINE__)

	char * __dbg_where(const char * a,const char * b,const char * c,int d);

#endif //(_MSC_VER <= 1200)

#else
//no MSVC

#define DBG(a,...)  if((a)<=_DBG_LEVEL_) { fprintf(stderr,"DBG%i:%s:%s:%i> ",_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__);\
fprintf(stderr, __VA_ARGS__); fflush(stderr); }

#define ERR(a,...) if((a)<=_DBG_LEVEL_) { fprintf(stderr,"DBG%i:%s:%s:%i> ",_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__);\
fprintf(stderr, __VA_ARGS__); perror(""); fflush(stderr); }

#define _WHERE(a) __dbg_where(a,__FILE__,__FUNCTION__,__LINE__)

char * __dbg_where(const char * a,const char * b,const char * c,int d);

#endif //MSVC

//TODO, port to other platforms
#define _DIE(a) { DBG(0,"ABORT: %s",_WHERE(a)); abort(); }

#ifdef _MALLOC_DBG_

#include <stdio.h>

extern FILE * fdbg;

#define malloc(a) malloc(a); fdbg=fopen("mem.log","a"); fprintf(fdbg,"DBG%i:%s:%s:%i>malloc(%08X)\n",_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__,a); fclose(fdbg);
#define realloc(a,b) realloc(a,b); fdbg=fopen("mem.log","a"); fprintf(fdbg,"DBG%i:%s:%s:%i>realloc(%08X,%08X)\n",_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__,a,b); fclose(fdbg);
#define free(a) free(a); fdbg=fopen("mem.log","a"); fprintf(fdbg,"DBG%i:%s:%s:%i>free(%08X)\n",_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__,a); fclose(fdbg);

#else //_MALLOC_DBG_

#ifdef _DMALLOC_DBG_
#include "dmalloc/dmalloc.h"
#else //_DMALLOC_DBG_
#define dmalloc_verify(a)
#endif //_DMALLOC_DBG_


#endif //_MALLOC_DBG_


#else //_DEBUG_
//NO DEBUG

#ifdef _DBG_LEVEL_
#undef _DBG_LEVEL_
#define _DBG_LEVEL_ 0
#endif //_DBG_LEVEL_

#ifndef __MSVC__
#define DBG(a,...)
#define ERR(a,...)
#else
#  define DBG()
#  define ERR()
#  ifdef _MSC_VER
#    pragma warning(disable:4002) //disable warning "too many actual parameters for macro 'identifier'"
#  endif
#endif //__WIN32__

#define _WHERE(a) a

#define _DIE(a) { printf("ABORT: %s",a); abort(); }
//NOTE: _DIE must always stop the execution of the program, if not, unexpected results
// that could end on massive data lost could happen.

#define dmalloc_verify(a)

#endif //_DEBUG_

#ifdef __cplusplus
}
#endif

#endif //__U_DEBUG_A_

