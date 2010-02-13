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
	
	You can choose to provide the global Alcugs information yourself with #define ALC_PROGRAM_VARS
	
*/

#ifndef __U_ALCUGS_H_
#define __U_ALCUGS_H_
#define __U_ALCUGS_H_ID "$Id$"

#include "alcconfig.h"
#include "alcxversion.h"

#ifdef VERSION
#undef VERSION
#endif
//#define VERSION alcSTR_VER

#ifndef IN_ALC_LIBRARY
	#ifdef IN_ALC_PROGRAM
		#ifndef ALC_PROGRAM_VARS
			#define ALC_PROGRAM_VARS
			#ifndef ALC_PROGRAM_ID
				#define ALC_PROGRAM_ID __U_ALCUGS_H_ID
			#endif
			#ifndef ALC_PROGRAM_NAME
				#define ALC_PROGRAM_NAME "Alcugs"
			#endif
			#ifndef ALC_PROGRAM_VERSION
				#define ALC_PROGRAM_VERSION alcSTR_VER
			#endif
			const char * alcID = ALC_PROGRAM_ID;
			const char * alcBUILD =  alcBUILD_TIME;
			const char * alcNAME = ALC_PROGRAM_NAME;
			const char * alcVERSION = ALC_PROGRAM_VERSION;
		#endif
	#endif
#endif

//std includes
#include <iostream>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <cassert>

//system includes

#if defined(__WIN32__) or defined(__CYGWIN__)
#include "alcutil/windoze.h"
#endif

#include <dirent.h>

//alcugs includes
#include "alctypes.h"
#include "alcexception.h"
#include "alcversion.h"
#include "alclicense.h"

#include "alcutil/alccfgtypes.h"
#include "alcutil/alcthread.h"
#include "alcutil/alcos.h"
#include "alcutil/conv_funs.h"
#include "alcutil/useful.h"
#include "alcutil/alclog.h"
#include "alcutil/alcparser.h"

#include "urutypes/uruconsts.h"
#include "urutypes/urubasetypes.h"
#include "urutypes/ageinfo.h"
#include "urutypes/plbase.h"
#include "urutypes/plmessage.h"

#include "alcmain.h"

//#include "alcutil/alcpkg.h"
//#include "urutypes/pypkg.h"

#endif
