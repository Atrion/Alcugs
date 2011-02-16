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

/** Alcugs debbuging interface (the old debug.h/debug.cpp)

	The next MACROS / functions are defined
	_WHERE(msg) returns filename,function,line and a message
	DBG(level,args...) debug message
	DBGM(level,args...) debug message (without line number)
	_DIE(msg) shows a message, and aborts

*/

#ifndef __U_ALCDEBUG_H
#define __U_ALCDEBUG_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCDEBUG_H_ID "$Id$"

namespace alc {
	class tString;
	
	// functions doing the actual work
	void alcPrintDbgHeader(int lvl, const char * b, const char * c, int d);
	tString alcDbgWhere(const char * b,const char * c,int d,const char * a,...);
	void alcDbgPrntAbort(const char * b,const char * c,int d,const char * a);
}

#ifndef NDEBUG
	// debugging enabled

	// make sure _DBG_LEVEL_ is set
	#ifndef _DBG_LEVEL_
		#define _DBG_LEVEL_ 0
	#endif //_DBG_LEVEL_

	// make DBG macros
	#define DBG(a,...)  if((a)<=_DBG_LEVEL_) { alcPrintDbgHeader(a,__FILE__,__FUNCTION__,__LINE__);\
	fprintf(stderr, __VA_ARGS__); fflush(stderr); }

	#define DBGM(a,...)  if((a)<=_DBG_LEVEL_) { fprintf(stderr, __VA_ARGS__); fflush(stderr); }

#else //NDEBUG

	// force _DBG_LEVEL_ to be 0 (for those preprocessor conditions checking this that are spread in the files)
	#ifdef _DBG_LEVEL_
		#undef _DBG_LEVEL_
	#endif //_DBG_LEVEL_
	#define _DBG_LEVEL_ 0

	// make DBG macros NO-OPs
	#define DBG(a,...)
	#define DBGM(a,...)

#endif //NDEBUG

// stuff which is always the same (with and without debug)
#define _WHERE(...) alcDbgWhere(__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)

#define _DIE(a) { alcDbgPrntAbort(__FILE__, __FUNCTION__, __LINE__, a); abort(); }
//NOTE: _DIE must always stop the execution of the program, if not, unexpected results
// that could end on massive data loss could happen.
	

#endif //__U_ALCDEBUG_H
