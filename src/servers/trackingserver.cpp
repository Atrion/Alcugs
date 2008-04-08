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

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <unet.h>

////extra includes
#include <protocol/trackingmsg.h>
#include "trackingserver.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	const char * alcNetName="Tracking";
	Byte alcWhoami=KTracking;
	
	int tUnetTrackingServer::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(ev, msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgCustomSetGuid:
			{
				// get the data out of the packet
				tmCustomSetGuid setGuid(u);
				msg->data->get(setGuid);
				log->log("<RCV> %s\n", setGuid.str());
				
				// save the data for the session
				memcpy(u->guid, setGuid.guid, 8);
				strcpy((char *)u->name, (char *)setGuid.age.c_str());
				u->data = (void *)(new tTrackingData(u->getPort() == 5000)); // the peer is initialized, it will be regarded when searching for servers
				// FIXME: the criteria to determine whether it's a lobby or a game server is BAD
				
				return 1;
			}
			case NetMsgCustomPlayerStatus:
			{
				// get the data out of the packet
				tmCustomPlayerStatus playerStatus(u);
				msg->data->get(playerStatus);
				log->log("<RCV> %s\n", playerStatus.str());
				
				// update the player's data
				players->updatePlayer(playerStatus.ki, playerStatus.x);
				
				return 1;
			}
			case NetMsgCustomFindServer:
			{
				// get the data out of the packet
				tmCustomFindServer findServer(u);
				msg->data->get(findServer);
				log->log("<RCV> %s\n", findServer.str());
				
				tPlayer *player = players->getPlayer(findServer.ki);
				if (!player) {
					err->log("ERR: Ignoring a NetMsgCustomFindServer for player with KI %d since I can't find that player\n", findServer.ki);
					return 1;
				}
				player->x = findServer.x;
				players->findServer(player, findServer.guid, findServer.age.c_str(), smgr);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc

