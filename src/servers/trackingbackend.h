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
		Byte ip[100]; //!< the external IP (the ones palyers should use to connect to this server)
		Byte agentGuid[7]; //!< set when isLobby = true, saves the fake guid for UruVision
	};
	
	class tPlayer {
	public:
		tPlayer(U32 ki);
		char *str(void);
		U32 ki; //!< player's ki number
		U32 x; //!< player's X value
		Byte uid[16]; //!< the player's account uid
		Byte avatar[200]; //!< the avatar's name
		Byte account[200]; //!< the account the player is logged in with
		U16 flag; //!< the player's flag (see tTrackingBackend::updatePlayer)
		U16 status; //!< the player's status
		bool waiting; //!< true if the player is waiting for ServerFound, false if it isn't [only defined when waiting=true]
		Byte awaiting_guid[8]; //!< Age guid where the player wants to go (hex) [only defined when waiting=true]
		Byte awaiting_age[200]; //!< Age name where the player wants to go
		tNetSession *u; //!< the lobby or game server the player is connected to
	};
	
	class tTrackingBackend {
	public:
		tTrackingBackend(tNetSessionList *servers, char *host, U16 port);
		~tTrackingBackend(void);
		void reload(void);
		void updateStatusFile(void);
		
		void updatePlayer(tNetSession *game, tmCustomPlayerStatus &playerStatus);
		tPlayer *getPlayer(U32 ki, int *nr = NULL);
		
		void updateServer(tNetSession *game, tmCustomSetGuid &setGuid);
		void removeServer(tNetSession *game);
		void findServer(tPlayer *player, Byte *guid, const Byte *name);
		void forwardMessage(tmCustomDirectedFwd &directedFwd);
	private:
		void notifyWaiting(tNetSession *server);
		void serverFound(tPlayer *player, tNetSession *server);
		void loadSettings(void);
		void removePlayer(int player);
		bool doesAgeLoadState(const Byte *age);
		void printStatusHTML(void);
		void printStatusXML(void);
		void printLobbyXML(FILE *f, tNetSession *lobby);
		void printPlayersXML(FILE *f, tNetSession *server);
		void printGameXML(FILE *f, tNetSession *game);
		void generateFakeGuid(Byte *guid); //!< generates a random 7 bytes fake guid for UruVision
	
		int size, count;
		tPlayer **players;
		tNetSessionList *servers;
		tLog *log;
		char *host;
		U16 port;
		tGuidGen *guidGen;
		Byte fakeLobbyGuid[7]; //!< saves the GUID for the fake lobby (for UruVision)
		
		Byte resettingAges[1024];
		bool loadAgeState;
		
		bool statusFileUpdate;
		bool statusHTML, statusXML;
		Byte statusHTMLFile[256], statusXMLFile[256];
		U32 lastUpdate;
	};

} //End alc namespace

#endif
