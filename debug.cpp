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

//for debugging

#ifndef __U_DEBUG_A_I_
#define __U_DEBUG_A_I_
/* CVS tag - DON'T TOUCH*/
#define __U_DEGUB_A_I_ID "$Id: debug.h,v 1.4 2004/12/02 22:31:46 almlys Exp $"

#include <stdio.h>

char * __dbg_where(const char * a,const char * b,const char * c,int d) {
	static char buffer[500];
	sprintf(buffer,"%s:%s:%i:%s",b,c,d,a);
	return (char *)buffer;
}

#ifdef _MALLOC_DBG_

#include <stdio.h>

FILE * fdbg;

#endif

#endif

