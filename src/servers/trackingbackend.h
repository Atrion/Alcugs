/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Project Server Team                   *
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

#ifndef __U_TRACKINGBACKEND_H
#define __U_TRACKINGBACKEND_H
/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGBACKEND_H_ID "$Id$"

#include <protocol/trackingmsg.h>
#include "guidgen.h"

namespace alc {

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/
	
		
	class tTrackingData : public tNetSessionData {
	public:
		tTrackingData(void);
		virtual ~tTrackingData(void) { delete childs; }
		bool isLobby;
		tNetSession *parent; //!< saves the lobby of a game server, is NULL for lobbys
		tNetSessionList *childs;
		U16 port_start, port_end;
		Byte ip[50]; //!< the external IP (the ones palyers should use to connect to this server)
	};
	
	class tPlayer {
	public:
		tPlayer(U32 ki) { this->ki = ki; this->x = 0; memset(guid, 0, 8); age_name[0] = 0; u = NULL; waiting = false; }
		char *str(void);
		U32 ki; //!< player's ki number
		U32 x; //!< player's X value
		Byte guid[8]; //!< Age guid where the player is / wants to go (hex)
		Byte age_name[200]; //!< Age name where the player is / wants to go
		bool waiting; //!< true if the player is waiting for ServerFound, false if it isn't
		tNetSession *u; //!< the lobby or game server the player is connected to
#if 0
		Byte uid[17];
	//link with _home_ server peer
	int sid; //the unique session identifier
	U32 ip; //the server session, where this player is
	U16 port; //the server session, where this player is
	//end link
	Byte flag; // 0-> delete, 1-> set invisible, 2-> set visible, 3-> set only buddies
	Byte status; //RStopResponding 0x00, 0x01 ok, RInroute 0x16, RArriving 0x17, RJoining 0x18, RLeaving 0x19, RQuitting 0x1A
	Byte avie[200];
	Byte login[200];
	U32 client_ip; //clients ip address
	U16 client_port; //clients port address
#endif
	};
	
	class tTrackingBackend {
	public:
		tTrackingBackend(tNetSessionList *servers);
		~tTrackingBackend(void);
		void reload(void);
		
		void updatePlayer(tNetSession *game, tmCustomPlayerStatus &playerStatus);
		tPlayer *getPlayer(U32 ki);
		
		void updateServer(tNetSession *game, tmCustomSetGuid &setGuid);
		void removeServer(tNetSession *game);
		void findServer(tPlayer *player, Byte *guid, const Byte *name);
	private:
		void notifyWaiting(tNetSession *server);
		void serverFound(tPlayer *player, tNetSession *server);
		void loadSettings(void);
		void removePlayer(int player);
		bool doesAgeLoadState(const Byte *age);
	
		int size;
		tPlayer **players;
		tNetSessionList *servers;
		tLog *log;
		tGuidGen *guidGen;
		
		Byte resettingAges[1024];
		bool loadAgeState;
	};

} //End alc namespace

#endif
