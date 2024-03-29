/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "unetserverbase.h"

#include "protocol/umsgbasic.h"


namespace alc {

	tUnetServerBase::tUnetServerBase(uint8_t whoami) : tUnetBase(whoami)
	{ }
	
	int tUnetServerBase::onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		switch(msg->cmd) {
			// answer to pings
			case NetMsgPing:
			{
				tmPing ping(u, msg);
				if (ping.destination == whoami || ping.destination == KBcast) { // if it's for us or for everyone, answer
					send(tmPing(u, ping));
				}
				else onForwardPing(ping, u); // if not, forward it (only implemented by lobby and game)
				return 1;
			}
		}
		return 0;
	}
} //end namespace alc

