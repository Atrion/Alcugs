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
#define __U_NETSESSION_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "urunet/unet.h"

#include "alcdebug.h"

namespace alc {

/* Session */
tNetSession::tNetSession(tUnet * net) {
	DBG(5,"tNetSession()\n");
	this->net=net;
	init();
}
tNetSession::~tNetSession() {
	DBG(5,"~tNetSession()\n");
}
void tNetSession::init() {
	DBG(5,"init()\n");
	ip=0;
	port=0;
	sid=-1;
	validation=0;
	authenticated=0;
}
void tNetSession::processMsg(Byte * buf,int size) {
	DBG(5,"Message of %i bytes\n",size);
	//stamp
	time_sec=alcGetTime();
	time_usec=alcGetMicroseconds();
	
	int ret;
	
	ret=alcUruValidatePacket(buf,size,&validation,authenticated,passwd);
	
	if(ret!=0 && (ret!=1 || net->flags & UNET_ECRC)) {
		if(ret==1) net->err->log("ERR: Failed Validating a message!\n");
		else net->err->log("ERR: Non-Uru protocol packet recieved!\n");
	}
	
	#if _DBG_LEVEL_ > 2
	DBG(2,"RAW Packet follows: \n");
	net->log->dumpbuf(buf,size);
	net->log->nl();
	#endif
	
	
}
/* End session */

}


