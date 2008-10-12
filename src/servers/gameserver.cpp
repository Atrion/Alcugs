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
#define __U_GAMESERVER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/msgparsers.h>

////extra includes
#include "gameserver.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	const char * alcNetName="Game";
	Byte alcWhoami=KGame;
	
	tUnetGameServer::tUnetGameServer(void) : tUnetLobbyServerBase()
	{
		ageInfo = NULL;
		lerr->log("WARNING: The game server is not finished yet. So if it doesn\'t work, that's not even a bug.\n");
	}
	
	tUnetGameServer::~tUnetGameServer(void)
	{
		if (ageInfo) delete ageInfo;
	}
	
	void tUnetGameServer::onStart(void)
	{
		// find out which age we are supposed to host
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("age_filename");
		if (var.size() < 2) throw txBase(_WHERE("an age name must be set"));
		strncpy((char *)serverName, (char *)var.c_str(), 199);
		var = cfg->getVar("age_guid");
		if (var.size() != 16) throw txBase(_WHERE("an age GUID must be set"));
		alcAscii2Hex(serverGuid, var.c_str(), 8);
		
		// load our age info
		tAgeInfoLoader ageInfoLoader(serverName);
		ageInfo = new tAgeInfo(*ageInfoLoader.getAge(serverName)); // get ourselves a copy of it
		
		tUnetLobbyServerBase::onStart();
	}

	void tUnetGameServer::onConnectionClosed(tNetEvent *ev, tNetSession *u)
	{
		if (ev->sid == tracking && isRunning()) {
			err->log("ERR: I lost the connection to the tracking server, so I will go down\n");
			/* The game server should go down when it looses the connection to tracking. This way, you can easily
			   shut down all game servers. In addition, it won't get any new peers anyway without the tracking server */
			stop();
		}
		else
			tUnetLobbyServerBase::onConnectionClosed(ev, u);
	}

} //end namespace alc

