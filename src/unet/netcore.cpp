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
	URUNET 3+
*/

/* CVS tag - DON'T TOUCH*/
#define __U_NETCORE_ID "$Id$"

#define _DBG_LEVEL_ 7

#include "alcugs.h"
#include "urunet/unet.h"

#include "alcdebug.h"

namespace alc {

tUnetBase::tUnetBase(char * lhost,U16 lport) :tUnet(lhost,lport) {
	state_running=false;
}

tUnetBase::~tUnetBase() {
	stop(5);
}

void tUnetBase::stop(Byte timeout) {
	stop_timeout=timeout;
	state_running=false;
}

//Blocks
void tUnetBase::run() {
	while(state_running==1) {
		Recv();
		
		tNetEvent * evt;
		while((evt=getEvent())) {
			lstd->log("Event id %i from host [%i]%s:%i\n",evt->id,evt->sid.sid,alcGetStrIp(evt->sid.ip),ntohs(evt->sid.port));
			delete evt;
		}

	}
}




} //end namespace



