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
#define __U_LOBBYSERVER_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "lobbyserver.h"

#include <netexception.h>
#include <protocol/trackingmsg.h>
#include <protocol/vaultmsg.h>
#include <protocol/lobbymsg.h>
#include <alcmain.h>

#include <unistd.h>
#include <cstring>

namespace alc {
	
	const char *alcNetName = "Lobby";
	tUnetServerBase *alcServerInstance(void) { return new tUnetLobbyServer(); }

	tUnetLobbyServer::tUnetLobbyServer(void) : tUnetLobbyServerBase(KLobby)
	{
		serverName = "Lobby";
	}
	
	void tUnetLobbyServer::onApplyConfig(void)
	{
		tUnetLobbyServerBase::onApplyConfig();
		tConfig *cfg = alcGetMain()->config();
		
		website = cfg->getVar("website");
		
		gameLogPath = cfg->getVar("game.log");
		if (gameLogPath.isEmpty()) throw txBase(_WHERE("game log directory is not defined"));
		
		gameConfig = cfg->getVar("game.config");
		if (gameConfig.isEmpty()) gameConfig = cfg->getVar("read_config", "cmdline");
		
		gameBin = cfg->getVar("game.bin");
		if (gameBin.isEmpty()) {
			gameBin = cfg->getVar("bin");
			if (gameBin.isEmpty()) throw txBase(_WHERE("game bin is not defined"));
			gameBin += "/uru_game";
		}
		
		tString var = cfg->getVar("load_on_demand");
		loadOnDemand = (var.isEmpty() || var.asUInt()); // on per default
	}
	
	int tUnetLobbyServer::onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetLobbyServerBase::onMsgRecieved(msg, u); // first let tUnetLobbyServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			//// messages regarding the player list
			case NetMsgRequestMyVaultPlayerList:
			{
				if (u->getPeerType() != KClient) {
					err->log("ERR: %s sent a NetMsgRequestMyVaultPlayerList but is not yet authed. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
			
				// get the packet
				tmRequestMyVaultPlayerList requestList(u);
				msg->data.get(requestList);
				log->log("<RCV> [%d] %s\n", msg->sn, requestList.str().c_str());
				
				// forward it to the vault server
				tmCustomVaultAskPlayerList askList(*vaultServer, requestList.x, u->getSid(), u->uid);
				send(askList);
				return 1;
			}
			case NetMsgCustomVaultPlayerList:
			{
				if (u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgCustomVaultPlayerList but is not the vault server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmCustomVaultPlayerList playerList(u);
				msg->data.get(playerList);
				log->log("<RCV> [%d] %s\n", msg->sn, playerList.str().c_str());
				
				// find the client's session
				tNetSessionRef client = sessionBySid(playerList.sid);
				// verify UID and session state
				if (!*client || client->getPeerType() != KClient || memcmp(client->uid, playerList.uid, 16) != 0) {
					err->log("ERR: Got NetMsgCustomVaultPlayerList for player with UID %s but can't find his session.\n", alcGetStrUid(playerList.uid).c_str());
					return 1;
				}
				
				// forward player list to client
				tmVaultPlayerList playerListClient(*client, playerList.x, playerList.numberPlayers, playerList.players, website);
				send(playerListClient);
				
				return 1;
			}
			
			//// messages regarding creating and deleting avatars
			case NetMsgCreatePlayer:
			{
				if (u->getPeerType() != KClient) {
					err->log("ERR: %s sent a NetMsgCreatePlayer but is not yet authed. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmCreatePlayer createPlayer(u);
				msg->data.get(createPlayer);
				log->log("<RCV> [%d] %s\n", msg->sn, createPlayer.str().c_str());
				
				// forward it to the vault server
				tmCustomVaultCreatePlayer vaultCreatePlayer(*vaultServer, createPlayer.x, u->getSid(), u->uid, u->getAccessLevel(), u->name, createPlayer.avatar, createPlayer.gender, createPlayer.friendName, createPlayer.key);
				send(vaultCreatePlayer);
				
				return 1;
			}
			case NetMsgCustomVaultPlayerCreated:
			{
				if (u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgCustomVaultPlayerList but is not the vault server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmCustomVaultPlayerCreated playerCreated(u);
				msg->data.get(playerCreated);
				log->log("<RCV> [%d] %s\n", msg->sn, playerCreated.str().c_str());
				
				// find the client's session
				tNetSessionRef client = sessionBySid(playerCreated.sid);
				// verify UID and session state
				if (!*client || client->getPeerType() != KClient || memcmp(client->uid, playerCreated.uid, 16) != 0) {
					err->log("ERR: Got NetMsgCustomVaultPlayerCreated for player with UID %s but can't find his session.\n", alcGetStrUid(playerCreated.uid).c_str());
					return 1;
				}
				
				// forward answer to client
				tmPlayerCreated playerCreatedClient(*client, playerCreated.ki, playerCreated.x, playerCreated.result);
				send(playerCreatedClient);
				
				return 1;
			}
			case NetMsgDeletePlayer:
			{
				if (u->getPeerType() != KClient) {
					err->log("ERR: %s sent a NetMsgDeletePlayer but is not yet authed. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the packet
				tmDeletePlayer deletePlayer(u);
				msg->data.get(deletePlayer);
				log->log("<RCV> [%d] %s\n", msg->sn, deletePlayer.str().c_str());
				
				// forward it to the vault server
				tmCustomVaultDeletePlayer vaultDeletePlayer(*vaultServer, deletePlayer.ki, deletePlayer.x, u->getSid(), u->uid, u->getAccessLevel());
				send(vaultDeletePlayer);
				
				return 1;
			}
			
			//// message for forking game servers
			case NetMsgCustomForkServer:
			{
				if (u->getPeerType() != KTracking) {
					err->log("ERR: %s sent a NetMsgCustomForkServer but is not the tracking server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomForkServer forkServer(u);
				msg->data.get(forkServer);
				log->log("<RCV> [%d] %s\n", msg->sn, forkServer.str().c_str());
				
				if (!loadOnDemand) {
					log->log("Ignoring fork request since I'm in manual mode\n");
					return 1;
				}
				
				// flush all log files before forking because otherwise they will be flushed by both parent and child and messages will be printed twice
				// Further assume that the vault subsystem was flushed already... (tvMessage::print does it right)
				log->flush(); err->flush(); sec->flush(); ack->flush();
				
				int pid = fork();
				if (pid == 0) {
					// This is the forked process. That means it is exactly the same as the lobby we just left.
					// We don't have to properly free each variable as this process will soon be completely
					//  replaced by the game server (this is was execlp does)
					
					// get the arguments for starting the server
					tString gameName = alcStrFiltered(forkServer.age);
					tString gameGuid = alcStrFiltered(forkServer.serverGuid);
					tString gameLog, gamePort;
					gameLog.printf("%s/%s/%s/", gameLogPath.c_str(), gameName.c_str(), gameGuid.c_str());
					gamePort.printf("%d", forkServer.forkPort);
					
					// if the server was put in daemon mode, the lobby would get the SIGCHILD immediately after starting, so it'd
					// be useless for debugging
					execlp(gameBin.c_str(), gameBin.c_str(),"-p",gamePort.c_str(),"-guid",gameGuid.c_str(),"-name",gameName.c_str(),
							"-log",gameLog.c_str(),"-c",gameConfig.c_str(),"-v","0",NULL);
					
					// if we come here, there was an error in the execlp call (but we're still in the game server process!)
					err->log("There was an error starting the game server %s (GUID: %s, Port: %s)\n", gameBin.c_str(), gameGuid.c_str(), gamePort.c_str());
					err->flush(); // make sure the error is actually printed
					exit(-1); // exit the game server process
				}
				// this is the parent process
				else if (pid < 0) {
					err->log("Can't fork game server\n");
				}
				else if (pid > 0) {
					log->log("Successfully forked game server (GUID: %s, Port: %d)\n", forkServer.serverGuid.c_str(), forkServer.forkPort);
				}
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc

