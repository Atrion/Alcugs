/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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
	Alcugs Configuration file.
	This depreceated file is going to disapear, when everything is correctly done by configure
*/

#ifndef __U_ALCCONFIG_OLD_H
#define __U_ALCCONFIG_OLD_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCCONFIG_OLD_H_ID "$Id$"

//! Enable debugging?
#define _DEBUG_

//! Allow to abort?
#define _DBG_ABORT_

//! Enable global debug level
#ifndef _DBG_LEVEL_
#define _DBG_LEVEL_ 0
#endif

/** Abort on Exception.
		If you enable this, the program will automatically abort when it launches and
		exception. Only useful to get Coredumps.
*/
//#define _TX_ABORT_

// Noise and latency debbuging tests
//#define _UNET_DBG_

// Debugging information attached to messages
#define _UNET_MSGDBG_

// Dump incoming/outgoing packages
//#define _DEBUG_PACKETS_

// Debug the ack stack
//#define _ACKSTACK_DBG_

// Enable dangerous testing options (must be defined, as 0 or as 1)
#define _DANGEROUS_TESTING 1

#endif
