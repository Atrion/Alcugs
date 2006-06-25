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

//for debugging
/*
	Note this file will be overwritten by config.h files, automatically generated
	by configure.
*/

#ifndef __U_CONFIG_
#define __U_CONFIG_
/* CVS tag - DON'T TOUCH*/
#define __U_CONFIG_ID "$Id$"

//! Enable debugging?
#define _DEBUG_

//! Enable malloc debugging?
//#define _MALLOC_DBG_

//! Enable dmalloc debugging?
//#define _DMALLOC_DBG_

//! Allow to abort?
#define _DBG_ABORT_

//! Enable global debug level
#ifndef _DBG_LEVEL_
#define _DBG_LEVEL_ 0
#endif

#endif