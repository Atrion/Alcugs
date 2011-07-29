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

/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGSERVER_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "trackingserver.h"

#include <alcmain.h>

namespace alc {
	
	const char *alcNetName = "Tracking";
	tUnetServerBase *alcServerInstance(void) { return new tUnetTrackingServer(); }

	int tUnetTrackingServer::onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgCustomSetGuid:
			{
				tmCustomSetGuid setGuid(u, msg);
				trackingBackend.updateServer(u, setGuid);
				return 1;
			}
			case NetMsgCustomPlayerStatus:
			{
				tmCustomPlayerStatus playerStatus(u, msg);
				trackingBackend.updatePlayer(u, playerStatus);
				return 1;
			}
			case NetMsgCustomFindServer:
			{
				tmCustomFindServer findServer(u, msg);
				trackingBackend.findServer(findServer);
				return 1;
			}
			case NetMsgCustomDirectedFwd:
			{
				tmCustomDirectedFwd directedFwd(u, msg);
				trackingBackend.forwardMessage(directedFwd);
				return 1;
			}
			case NetMsgCustomPlayerToCome:
			{
				tmCustomPlayerToCome playerToCome(u, msg);
				trackingBackend.playerCanCome(u, playerToCome.ki);
				return 1;
			}
			case NetMsgPublicAgeList:
			{
				tmPublicAgeList ageList(u, msg);
				trackingBackend.getPopulationCounts(ageList);
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
