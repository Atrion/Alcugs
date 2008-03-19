/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_UNETSERVERBASE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	int tUnetServerBase::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret=0;

		tmPing ping;
	
		switch(msg->cmd) {
			// answer to pings
			case NetMsgPing:
				ping.setSource(u);
				msg->data->get(ping);
				log->log("Ping from %s:%i x=%i dest=%i %s time=%0.3f ms .... pong....\n",\
					alcGetStrIp(ev->sid.ip),ntohs(ev->sid.port),ping.x,ping.destination,\
					alcUnetGetDestination(ping.destination),ping.mtime*1000);
				ping.setReply();
				u->send(ping);
				ret=1;
				break;
			default:
				break;
		}
		return ret;
	}

} //end namespace alc

