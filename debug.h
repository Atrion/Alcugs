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

#ifndef __U_DEBUG_A_
#define __U_DEBUG_A_
/* CVS tag - DON'T TOUCH*/
#define __U_DEGUB_A_ID "$Id$"

#ifdef _DBG_LEVEL_
 #if (_DBG_LEVEL_ == 0)
  #ifdef _DEBUG_
   #undef _DEBUG_
  #endif
 #endif
#endif


#ifdef _DEBUG_

#ifndef _DBG_LEVEL_
#define _DBG_LEVEL_ 0
#endif

#define DBG(a,...)  if((a)<=_DBG_LEVEL_) { fprintf(stderr,"DBG%i:%s:%s:%i> ",_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__);\
fprintf(stderr, __VA_ARGS__); fflush(stderr); }

#define ERR(a,...) if((a)<=_DBG_LEVEL_) { fprintf(stderr,"DBG%i:%s:%s:%i> ",_DBG_LEVEL_,__FILE__,__FUNCTION__,__LINE__);\
fprintf(stderr, __VA_ARGS__); perror(""); fflush(stderr); }

#else

#ifdef _DBG_LEVEL_
#undef _DBG_LEVEL_
#define _DBG_LEVEL_ 0
#endif

#define DBG(a,...)

#define ERR(a,...)

#endif


#endif

