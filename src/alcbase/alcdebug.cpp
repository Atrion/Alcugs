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

/** for debugging */

/* CVS tag - DON'T TOUCH*/
#define __U_DEGUB_ID "$Id$"

#ifdef __cplusplus
#include <cstdio>
#include <cstring>
extern "C" {
#else
#include <stdio.h>
#include <string.h>
#endif

#include <stdarg.h>

#ifdef __MSVC__
	//this looks like crap, i know...

	#include <stddef.h>
	#include <stdarg.h>

	typedef void(*_DBGorERR_pointer)(int a, char *msg, ...);

	int g_dbglvl;
	char *g_file;
	#if (_MSC_VER > 1200)
		char *g_function; //only needed if MSVC Version > 6
	#endif
	int g_line;
	int dbg_or_err; //0=>DBG, 1=>ERR

	#if (_MSC_VER <= 1200)
		//MSVC Version 6 or lower (doesn't support __FUNCTION__)

		void _DBGorERR(int a, char *text, ...)
		{
			if(a<=g_dbglvl)
			{
				va_list args;
				va_start(args, text);
				fprintf(stderr,dbg_or_err?"ERR":"DBG");
				fprintf(stderr,"%i:%s:%i> ",/*g_dbglvl*/ a,g_file,g_line);
				vfprintf(stderr, text, args);
				fflush(stderr);
				va_end(args);
			}
		}

		_DBGorERR_pointer _DBG_before(int dbglvl, char *file, int line)
		{
			g_dbglvl=dbglvl;
			g_file=file;
			g_line=line;
			dbg_or_err = 0;
			return &_DBGorERR;
		}

		_DBGorERR_pointer _ERR_before(int dbglvl, char *file, int line)
		{
			g_dbglvl=dbglvl;
			g_file=file;
			g_line=line;
			dbg_or_err = 1;
			return &_DBGorERR;
		}

		char * __dbg_where(const char * a,const char * b,int d) {
			static char buffer[500];
			sprintf(buffer,"%s:%i:%s",b,d,a);
			return (char *)buffer;
		}

	#else
		//MSVC Version > 6 (should support __FUNCTION__)

		void _DBGorERR(int a, char *text, ...)
		{
			if(a<=g_dbglvl)
			{
				va_list b;
				va_start(b,text);
				fprintf(stderr,dbg_or_err?"ERR":"DBG");
				fprintf(stderr,"%i:%s:%s:%i> ",/*g_dbglvl*/ a,g_file,g_function,g_line);
				vfprintf(stderr, text, b);
				fflush(stderr);
				va_end(b);
			}
		}

		_DBGorERR_pointer _DBG_before(int dbglvl, char *file, char *function, int line)
		{
			g_dbglvl=dbglvl;
			g_file=file;
			g_function=function;
			g_line=line;
			dbg_or_err = 0;
			return &_DBGorERR;
		}

		_DBGorERR_pointer _ERR_before(int dbglvl, char *file, char *function, int line)
		{
			g_dbglvl=dbglvl;
			g_file=file;
			g_function=function;
			g_line=line;
			dbg_or_err = 1;
			return &_DBGorERR;
		}

		char * __dbg_where(const char * a,const char * b,const char * c,int d) {
			static char buffer[500];
			sprintf(buffer,"%s:%s:%i:%s",b,c,d,a);
			return (char *)buffer;
		}

	#endif //(_MSC_VER <= 1200)

#else
	//no MSVC

	char * __dbg_where(const char * b,const char * c,int d,const char * a,...) {
		va_list ap;
		static char buffer[2*1024];
		
		va_start(ap,a);
		sprintf(buffer,"%s:%s:%i:",b,c,d);
		vsnprintf(buffer+strlen(buffer),sizeof(buffer) - (strlen(buffer)+1),a,ap);
		va_end(ap);
		return (char *)buffer;
	}

#endif

#ifdef __cplusplus
}
#endif

