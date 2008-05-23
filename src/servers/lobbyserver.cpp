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
#define __U_LOBBYSERVER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <unet.h>
#include <protocol/lobbymsg.h>
#include <protocol/vaultmsg.h>

////extra includes
#include "lobbyserver.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	const char * alcNetName="Lobby";
	Byte alcWhoami=KLobby;
	
	tUnetLobbyServer::tUnetLobbyServer(void) : tUnetLobbyServerBase()
	{
		strcpy((char*)name, alcNetName);
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("website");
		strncpy((char *)website, (char *)var.c_str(), 255);
		lstd->log("WARNING: The lobby server is not finished yet. So if it doesn\'t work, that's not even a bug.\n");
	}
	
	int tUnetLobbyServer::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetLobbyServerBase::onMsgRecieved(ev, msg, u); // first let tUnetLobbyServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgRequestMyVaultPlayerList:
			{
				// get the packet
				tmRequestMyVaultPlayerList requestList(u);
				msg->data->get(requestList);
				log->log("<RCV> %s\n", requestList.str());
				u->x = requestList.x; // save the X to reuse it when answering
				u->ki = requestList.ki;
				
				// forward it to the vault server
				// send authAsk to auth server
				tNetSession *vaultServer = getPeer(KVault);
				if (!vaultServer) {
					err->log("ERR: I've got to ask the vault server about player %s, but it's unavailable.\n", u->str());
					return 1;
				}
				tmCustomVaultAskPlayerList askList(vaultServer, u->getSid(), u->guid);
				vaultServer->send(askList);
				return 1;
			}
			case NetMsgCustomVaultPlayerList:
			{
				if (u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgCustomVaultPlayerList but is not the vault server\n", u->str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmCustomVaultPlayerList playerList(u);
				msg->data->get(playerList);
				log->log("<RCV> %s\n", playerList.str());
				
				// find the client's session
				tNetSession *client = smgr->get(playerList.x);
				// verify GUID and session state
				if (!client || client->getPeerType() != KClient || memcmp(client->guid, playerList.guid, 16) != 0) {
					err->log("ERR: Got CustomVaultPlayerList for player with GUID %s but can't find his session.\n", alcGetStrGuid(playerList.guid, 16));
					return 1;
				}
				
				// forward player list to client
				tmVaultPlayerList playerListClient(client, playerList.numberPlayers, playerList.players, website);
				client->send(playerListClient);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc

