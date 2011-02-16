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
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGSERVER_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "trackingserver.h"

#include <alcmain.h>

namespace alc {
	
	const char *alcNetName = "Tracking";
	tUnetServerBase *alcServerInstance(void) { return new tUnetTrackingServer(); }

	////IMPLEMENTATION
	int tUnetTrackingServer::onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgCustomSetGuid:
			{
				// get the data out of the packet
				tmCustomSetGuid setGuid(u);
				msg->data.get(setGuid);
				log->log("<RCV> [%d] %s\n", msg->sn, setGuid.str().c_str());
				
				// save the data for the session
				trackingBackend.updateServer(u, setGuid);
				
				return 1;
			}
			case NetMsgCustomPlayerStatus:
			{
				// get the data out of the packet
				tmCustomPlayerStatus playerStatus(u);
				msg->data.get(playerStatus);
				log->log("<RCV> [%d] %s\n", msg->sn, playerStatus.str().c_str());
				
				// update the player's data
				trackingBackend.updatePlayer(u, playerStatus);
				
				return 1;
			}
			case NetMsgCustomFindServer:
			{
				// get the data out of the packet
				tmCustomFindServer findServer(u);
				msg->data.get(findServer);
				log->log("<RCV> [%d] %s\n", msg->sn, findServer.str().c_str());
				
				trackingBackend.findServer(findServer);
				
				return 1;
			}
			case NetMsgCustomDirectedFwd:
			{
				// get the data out of the packet
				tmCustomDirectedFwd directedFwd(u);
				msg->data.get(directedFwd);
				log->log("<RCV> [%d] %s\n", msg->sn, directedFwd.str().c_str());
				
				trackingBackend.forwardMessage(directedFwd);
				
				return 1;
			}
			case NetMsgCustomPlayerToCome:
			{
				// get the data out of the packet
				tmCustomPlayerToCome playerToCome(u);
				msg->data.get(playerToCome);
				log->log("<RCV> [%d] %s\n", msg->sn, playerToCome.str().c_str());
				
				trackingBackend.playerCanCome(u, playerToCome.ki);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
