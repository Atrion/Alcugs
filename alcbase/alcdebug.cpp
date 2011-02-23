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
#include "alcdefs.h"
#include "alcdebug.h"

#include "alctypes.h"
#include "alcutil/alcthread.h"

#include <cstdarg>

namespace alc {
	void alcPrintDbgHeader(int lvl, const char * b, const char * c, int d) {
		fprintf(stderr,"DBG%i:%li:%s:%s:%i> ",lvl,alcGetSelfThreadId(),b,c,d);
	}
	
	tString alcDbgWhere(const char * b,const char * c,int d,const char * a,...) {
		va_list ap;
		tString str;
		va_start(ap,a);
		str.printf("%d:%s:%s:%i:",alcGetSelfThreadId(),b,c,d);
		str.vprintf(a,ap);
		va_end(ap);
		return str;
	}
	
	void alcDbgPrntAbort(const char * b,const char * c,int d,const char * a) {
		tString where = alcDbgWhere(b, c, d, a);
		fprintf(stderr,"ABORT: %s\n",where.c_str());
	}
	
}


