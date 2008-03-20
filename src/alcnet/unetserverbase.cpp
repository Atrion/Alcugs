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
	tUnetServerBase::tUnetServerBase(void) : tUnetBase()
	{
		whoami = alcWhoami; // the server should know who it is
	}
	
	int tUnetServerBase::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret=0;

		tmPing ping;
		tmAlive alive;
	
		switch(msg->cmd) {
			// answer to pings
			case NetMsgPing:
				ret = 1;
				ping.setSource(u);
				msg->data->get(ping);
				log->log("<RCV> %s\n",ping.str());
				if (ping.destination == whoami || ping.destination == KBcast) { // if it's for us or for everyone, answer
					ping.setReply();
					u->send(ping);
				}
				else if (whoami == KLobby || whoami == KGame) { // TODO: lobby and game server should forward the pings to their destination
				
				}
				break;
			case NetMsgAlive:
				ret = 1;
				msg->data->get(alive);
				log->log("<RCV> %s\n",alive.str());
				break;
		}
		return ret;
	}

} //end namespace alc

