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
	URUNET 3
*/

/* CVS tag - DON'T TOUCH*/
#define __U_NETLOG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcnet.h"

#ifndef __WIN32__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <alcdebug.h>

namespace alc {

/** gets the ip address string of a host ip in network byte order
*/
const char * alcGetStrIp(U32 ip) {
	// valgrind gives an error here about some malloc within inet_ntoa still being reachable. However I've got no idea what to do about that.
	in_addr cip;
	static char mip[16]; // FIXME
	cip.s_addr=ip;
	alcStrncpy(mip, inet_ntoa(cip), sizeof(mip)-1);
	//print2log(f_uru,"DBGDBGDBG:<<<----->>>>%s:%08X\n",mip,ip);
	return mip;
}

}


