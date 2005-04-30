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

/* CVS tag - DON'T TOUCH*/
#define __U_VERSION_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <sys/stat.h>

#include "license.h"
#include "protocol.h"
#include "stdebug.h"

#include "debug.h"

extern const char * ID;
extern const char * BUILD;
extern const char * SNAME;
extern const char * VERSION;

const char * __uru_head ="\nAlcugs H'uru Server project\n\n";

/**
Prints version info to the specific log file
*/
void version_info(st_log * f_dsc) {
	print2log(f_dsc,"%s",__uru_disclaimer_short);
	print2log(f_dsc,"%s",__uru_head);
	print2log(f_dsc,"\n%s.  Build %s - Version %s\nId: %s\n",SNAME,BUILD,VERSION,ID);
	print2log(f_dsc,"Uru protocol %s min version %s max version supported\n",__U_PROTOCOL_VMIN,__U_PROTOCOL_VMAX);
	print2log(f_dsc,"Unet %s Protocol %i.%i\n",__U_PROTOCOL_V,unet_max_version,unet_min_version);
	logflush(f_dsc);
}

void short_version_info(st_log * f_dsc) {
	print2log(f_dsc,"%s.  Build %s - Version %s\nId: %s\n",SNAME,BUILD,VERSION,ID);
	print2log(f_dsc,"Uru protocol %s min version %s max version supported\n",__U_PROTOCOL_VMIN,__U_PROTOCOL_VMAX);
	print2log(f_dsc,"Unet %s Protocol %i.%i\n",__U_PROTOCOL_V,unet_max_version,unet_min_version);
	logflush(f_dsc);
}


/**
Prints version info to a file descriptor
*/
void version(FILE * f_dsc) {
	fprintf(f_dsc,"%s",__uru_disclaimer_short);
	fprintf(f_dsc,"%s",__uru_head);
	fprintf(f_dsc,"\n%s.  Build %s - Version %s\nId: %s\n",SNAME,BUILD,VERSION,ID);
	fprintf(f_dsc,"Uru protocol %s min version %s max version supported\n",__U_PROTOCOL_VMIN,__U_PROTOCOL_VMAX);
	fprintf(f_dsc,"Unet %s Protocol %i.%i\n",__U_PROTOCOL_V,unet_max_version,unet_min_version);
	fflush(f_dsc);
}

