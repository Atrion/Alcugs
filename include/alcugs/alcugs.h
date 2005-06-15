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

/**
	You should include this file at the begginging of any program that uses the
	Alcugs API.
	
	If you have plans to use the Alcugs debuging interface, you should include
	alcdebug.h at the end of your include list.
	
	So, your code will look something like this.
	
	//other includes
	
	#include <alcugs.h>
	
	//Other alcugs includes
	
	//other includes
	
	#include <alcdebug.h>
	
	//You can't include nothing here - It may cause problems.
	
	Also, you should not include alcdebug.h inside any other header file.
	
	Note: If you are going to install these on your system, they must reside in their
	own alcugs directory. For example: "/usr/include/alcugs/" or "/usr/local/include/alcugs".
	Remember to pass the -I/usr/include/alcugs parameter to your compiler.
	
*/

#ifndef __U_ALCUGS_H_
#define __U_ALCUGS_H_
#define __U_ALCUGS_H_ID "$Id$"

#if defined(HAVE_CONFIG_H) and defined(HAVE_WINCONFIG_H)
#error You can only use config.h, or winconfig.h, but not both
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_WINCONFIG_H
#include "winconfig.h"
#endif

#include "alcconfig.h"

//std includes
#include <iostream>
#include <cstdio>

//system includes

#ifdef __WIN32__
#include "alcutil/windoze.h"
#endif

namespace std {       
#include <sys/types.h>
#include <dirent.h>
}

//alcugs includes
#include "alcexception.h"
#include "alctypes.h"

#include "urutypes/urubasetypes.h"

#include "alcutil/alcos.h"


#endif
