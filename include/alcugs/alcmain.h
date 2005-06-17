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
	Alcugs Lib Main code.
*/

#ifndef __U_ALCMAIN_H
#define __U_ALCMAIN_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCMAIN_H_ID "$Id$"

/* You need to define these vars in your app's */
extern const char * alcXSNAME;
extern const char * alcXBUILD;
extern const char * alcXVERSION;
extern const char * alcXID;

namespace alc {

/** Start Alcugs library 
		\param argc Number of args
		\param argv args
*/
void alcInit(int argc=0,char ** argv=NULL);
/** Stop Alcugs library */
void alcShutdown();

}

#endif
