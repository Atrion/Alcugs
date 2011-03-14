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

#ifndef __U_TRACKINGBACKEND_H
#define __U_TRACKINGBACKEND_H
/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGBACKEND_H_ID "$Id$"

#include <protocol/trackingmsg.h>
#include <netsession.h>
#include <alcutil/alclog.h>

#include <list>

namespace alc {
	class tUnetTrackingServer;
	class tNetSessionList;
	
	class tTrackingData : public tBaseType {
	public:
		typedef std::list<uint32_t> tPlayerList;
	
		tTrackingData(void);
		bool isLobby;
		tNetSession *parent; //!< saves the lobby of a game server, is NULL for lobbys
		std::list<tNetSession *> children;
		uint16_t portStart, portEnd;
		tString externalIp; //!< the external IP (the ones palyers should use to connect to this server)
		uint8_t agentGuid[8]; //!< set when isLobby = true, saves the fake guid for UruVision
		tPlayerList waitingPlayers;
	};
	
	class tPlayer {
	public:
		tPlayer(uint32_t ki);
		tString str(void) const;
		uint32_t ki; //!< player's ki number
		uint32_t sid; //!< player's sid in the lobby/game server
		uint8_t uid[16]; //!< the player's account uid (hex)
		tString avatar; //!< the avatar's name
		tString account; //!< the account the player is logged in with
		uint16_t flag; //!< the player's flag (see tTrackingBackend::updatePlayer)
		uint16_t status; //!< the player's status
		bool waiting; //!< true if the player is waiting for ServerFound, false if it isn't [only defined when waiting=true]
		uint32_t awaiting_x; //!< the X alue the player requested the age with
		uint8_t awaiting_guid[8]; //!< Age guid where the player wants to go (hex) [only defined when waiting=true]
		tString awaiting_age; //!< Age name where the player wants to go [only defined when waiting=true]
		tNetSession *u; //!< the lobby or game server the player is connected to
	};
	
	class tTrackingBackend {
	public:
		tTrackingBackend(tUnetTrackingServer *net, const tString &host, uint16_t port);
		~tTrackingBackend(void);
		void applyConfig(void);
		
		void updateStatusFile(void);
		
		void updatePlayer(tNetSession *game, tmCustomPlayerStatus &playerStatus);
		
		void updateServer(tNetSession *game, tmCustomSetGuid &setGuid);
		void removeServer(tNetSession *game);
		void findServer(tmCustomFindServer &findServer);
		void forwardMessage(tmCustomDirectedFwd &directedFwd);
		void playerCanCome(tNetSession *game, uint32_t ki);
	private:
		
		typedef std::list<tPlayer> tPlayerList;
		
		tPlayerList::iterator getPlayer(uint32_t ki);
		void spawnServer(const tString &age, const uint8_t *guid, uint32_t delay = 0);
		void notifyWaiting(tNetSession *server);
		void serverFound(tPlayer *player, tNetSession *server);
		void printStatusHTML(bool dbg = false);
		void printStatusXML(void);
		void printLobbyXML(FILE *f, tNetSession *lobby, tTrackingData *data);
		void printPlayersXML(FILE *f, tNetSession *server);
		void printPlayerXML(FILE *f, tPlayer *player);
		void printGameXML(FILE *f, tNetSession *game, tTrackingData *data);
		void generateFakeGuid(uint8_t *guid); //!< generates a random 7 bytes fake guid for UruVision

		tPlayerList players;
		tUnetTrackingServer *net;
		tLog log;
		tString host;
		uint16_t port;
		uint8_t fakeLobbyGuid[8]; //!< saves the GUID for the fake lobby (for UruVision)
		
		bool statusFileUpdate;
		bool statusHTML, statusHTMLdbg, statusXML;
		tString statusHTMLFile, statusHTMLdbgFile, statusXMLFile;
		time_t lastUpdate;
	};

} //End alc namespace

#endif
