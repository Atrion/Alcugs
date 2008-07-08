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
#include <protocol/msgparsers.h>

////extra includes
#include "lobbyserver.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	const char * alcNetName="Lobby";
	Byte alcWhoami=KLobby;
	
	tUnetLobbyServer::tUnetLobbyServer(void) : tUnetLobbyServerBase()
	{
		strcpy((char*)serverName, alcNetName);
		loadSettings();
		lstd->log("WARNING: The lobby server is not finished yet. So if it doesn\'t work, that's not even a bug.\n");
	}
	
	void tUnetLobbyServer::loadSettings(void)
	{
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("website");
		strncpy((char *)website, (char *)var.c_str(), 255);
	}
	
	int tUnetLobbyServer::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetLobbyServerBase::onMsgRecieved(ev, msg, u); // first let tUnetLobbyServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			//// messages regarding the player list
			case NetMsgRequestMyVaultPlayerList:
			{
				if (u->getPeerType() != KClient) {
					err->log("ERR: %s sent a NetMsgRequestMyVaultPlayerList but is not yet authed. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
			
				// get the packet
				tmRequestMyVaultPlayerList requestList(u);
				msg->data->get(requestList);
				log->log("<RCV> %s\n", requestList.str());
				
				// forward it to the vault server
				tNetSession *vaultServer = getSession(vault);
				if (!vaultServer) {
					err->log("ERR: I've got to ask the vault server about player %s, but it's unavailable.\n", u->str());
					return 1;
				}
				tmCustomVaultAskPlayerList askList(vaultServer, u->getSid(), u->guid);
				send(askList);
				return 1;
			}
			case NetMsgCustomVaultPlayerList:
			{
				if (u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgCustomVaultPlayerList but is not the vault server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmCustomVaultPlayerList playerList(u);
				msg->data->get(playerList);
				log->log("<RCV> %s\n", playerList.str());
				
				// find the client's session
				tNetSession *client = smgr->get(playerList.x);
				// verify UID and session state
				if (!client || client->getPeerType() != KClient || memcmp(client->guid, playerList.uid, 16) != 0) {
					err->log("ERR: Got NetMsgCustomVaultPlayerList for player with UID %s but can't find his session.\n", alcGetStrUid(playerList.uid));
					return 1;
				}
				
				// forward player list to client
				tmVaultPlayerList playerListClient(client, playerList.numberPlayers, playerList.players, website);
				send(playerListClient);
				
				return 1;
			}
			
			//// messages regarding creating and deleting avatars
			case NetMsgCreatePlayer:
			{
				if (u->getPeerType() != KClient) {
					err->log("ERR: %s sent a NetMsgCreatePlayer but is not yet authed. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmCreatePlayer createPlayer(u);
				msg->data->get(createPlayer);
				log->log("<RCV> %s\n", createPlayer.str());
				
				// forward it to the vault server
				tNetSession *vaultServer = getSession(vault);
				if (!vaultServer) {
					err->log("ERR: I've got to ask the vault server to create a player, but it's unavailable.\n", u->str());
					return 1;
				}
				tmCustomVaultCreatePlayer vaultCreatePlayer(vaultServer, createPlayer, u->getSid(), u->guid, u->getAccessLevel(), u->name);
				send(vaultCreatePlayer);
				
				return 1;
			}
			case NetMsgCustomVaultPlayerCreated:
			{
				if (u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgCustomVaultPlayerList but is not the vault server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmCustomVaultPlayerCreated playerCreated(u);
				msg->data->get(playerCreated);
				log->log("<RCV> %s\n", playerCreated.str());
				
				// find the client's session
				tNetSession *client = smgr->get(playerCreated.x);
				// verify UID and session state
				if (!client || client->getPeerType() != KClient || memcmp(client->guid, playerCreated.uid, 16) != 0) {
					err->log("ERR: Got NetMsgCustomVaultPlayerCreated for player with UID %s but can't find his session.\n", alcGetStrUid(playerCreated.uid));
					return 1;
				}
				
				// forward answer to client
				tmPlayerCreated playerCreatedClient(client, playerCreated.ki, playerCreated.result);
				send(playerCreatedClient);
				
				return 1;
			}
			case NetMsgDeletePlayer:
			{
				if (u->getPeerType() != KClient) {
					err->log("ERR: %s sent a NetMsgDeletePlayer but is not yet authed. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmDeletePlayer deletePlayer(u);
				msg->data->get(deletePlayer);
				log->log("<RCV> %s\n", deletePlayer.str());
				
				// forward it to the vault server
				tNetSession *vaultServer = getSession(vault);
				if (!vaultServer) {
					err->log("ERR: I've got to ask the vault server to delete a player, but it's unavailable.\n", u->str());
					return 1;
				}
				tmCustomVaultDeletePlayer vaultDeletePlayer(vaultServer, u->getSid(), deletePlayer.ki, u->guid, u->getAccessLevel());
				send(vaultDeletePlayer);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc

