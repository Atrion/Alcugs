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

#include <list>

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
		U16 portStart, portEnd;
		char externalIp[100]; //!< the external IP (the ones palyers should use to connect to this server)
		Byte agentGuid[8]; //!< set when isLobby = true, saves the fake guid for UruVision
		U32 seqPrefix;
	};
	
	class tPlayer {
	public:
		tPlayer(U32 ki);
		char *str(void);
		U32 ki; //!< player's ki number
		U32 sid; //!< player's sid in the lobby/game server
		Byte uid[16]; //!< the player's account uid (hex)
		char avatar[200]; //!< the avatar's name
		char account[200]; //!< the account the player is logged in with
		U16 flag; //!< the player's flag (see tTrackingBackend::updatePlayer)
		U16 status; //!< the player's status
		bool waiting; //!< true if the player is waiting for ServerFound, false if it isn't [only defined when waiting=true]
		U32 awaiting_x; //!< the X alue the player requested the age with
		Byte awaiting_guid[8]; //!< Age guid where the player wants to go (hex) [only defined when waiting=true]
		char awaiting_age[200]; //!< Age name where the player wants to go [only defined when waiting=true]
		tNetSession *u; //!< the lobby or game server the player is connected to
		U32 ip;
		U16 port;
	};
	
	class tTrackingBackend {
	public:
		tTrackingBackend(tUnetBase *net, tNetSessionList *servers, char *host, U16 port);
		~tTrackingBackend(void);
		void reload(void);
		
		void updateStatusFile(void);
		
		void updatePlayer(tNetSession *game, tmCustomPlayerStatus &playerStatus);
		
		void updateServer(tNetSession *game, tmCustomSetGuid &setGuid);
		void removeServer(tNetSession *game);
		void findServer(tmCustomFindServer &findServer);
		void forwardMessage(tmCustomDirectedFwd &directedFwd);
	private:
		void load(void);
		void unload(void);
		
		typedef std::list<tPlayer> tPlayerList;
		
		tPlayerList::iterator getPlayer(U32 ki);
		void notifyWaiting(tNetSession *server);
		void serverFound(tPlayer *player, tNetSession *server);
		bool doesAgeLoadState(const char *age);
		void printStatusHTML(bool dbg = false);
		void printStatusXML(void);
		void printLobbyXML(FILE *f, tNetSession *lobby, tTrackingData *data);
		void printPlayersXML(FILE *f, tNetSession *server);
		void printPlayerXML(FILE *f, tPlayer *player);
		void printGameXML(FILE *f, tNetSession *game, tTrackingData *data);
		void generateFakeGuid(Byte *guid); //!< generates a random 7 bytes fake guid for UruVision

		tPlayerList players;
		tUnetBase *net;
		tNetSessionList *servers;
		tLog *log;
		char *host;
		U16 port;
		Byte fakeLobbyGuid[8]; //!< saves the GUID for the fake lobby (for UruVision)
		
		tGuidGen *guidGen;
		char resettingAges[1024];
		bool loadAgeState;
		
		bool statusFileUpdate;
		bool statusHTML, statusHTMLdbg, statusXML;
		char statusHTMLFile[256], statusHTMLdbgFile[256], statusXMLFile[256];
		U32 lastUpdate;
	};

} //End alc namespace

#endif
