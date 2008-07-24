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
#include <alcnet.h>
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
	}
	
	void tUnetLobbyServer::onLoadConfig(bool reload)
	{
		tUnetLobbyServerBase::onLoadConfig(reload);
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("website");
		strncpy((char *)website, (char *)var.c_str(), 255);
		var = cfg->getVar("game.log");
		strncpy((char *)gameLogPath, (char *)var.c_str(), 255);
		var = cfg->getVar("game.config");
		strncpy((char *)gameConfig, (char *)var.c_str(), 255);
		var = cfg->getVar("bin");
		strncpy((char *)gameBinPath, (char *)var.c_str(), 255);
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
				tmCustomVaultAskPlayerList askList(vaultServer, u->getSid(), u->uid);
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
				if (!client || client->getPeerType() != KClient || memcmp(client->uid, playerList.uid, 16) != 0) {
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
				tmCustomVaultCreatePlayer vaultCreatePlayer(vaultServer, u->getSid(), u->uid, u->getAccessLevel(), u->name, createPlayer.avatar, createPlayer.gender, createPlayer.friendName, createPlayer.key);
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
				if (!client || client->getPeerType() != KClient || memcmp(client->uid, playerCreated.uid, 16) != 0) {
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
				tmCustomVaultDeletePlayer vaultDeletePlayer(vaultServer, deletePlayer.ki, u->getSid(), u->uid, u->getAccessLevel());
				send(vaultDeletePlayer);
				
				return 1;
			}
			
			//// message for forking game servers
			case NetMsgCustomForkServer:
			{
				if (u->getPeerType() != KTracking) {
					err->log("ERR: %s sent a NetMsgCustomVaultPlayerList but is not the tracking server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomForkServer forkServer(u);
				msg->data->get(forkServer);
				log->log("<RCV> %s\n", forkServer.str());
				
				// flush all log files before forking because otherwise they will be flushed by both parent and child and messages will be printed twice
				log->flush(); err->flush(); sec->flush();
				
				int pid = fork();
				if (pid == 0) {
					// This is the forked process. That means it is exactly the same as the lobby we just left.
					// We don't have to properly free each variable as this process will soon be completely
					//  replaced by the game server (this is was execlp does)
					// BUT we have to close each file and socket as otherwise they will stay opened till the game server exits
					
					// get the arguments for starting the server
					char gameName[128], gameGuid[32], gameLog[512], gameBin[512], gamePort[16];
					strncpy(gameName, (char *)forkServer.age.c_str(), 127);
					alcStrFilter(gameName);
					strncpy(gameGuid, (char *)forkServer.serverGuid.c_str(), 31);
					alcStrFilter(gameGuid);
					sprintf(gameLog, "%s/%s/%s/", gameLogPath, gameName, gameGuid);
					sprintf(gameBin, "%s/uru_game", gameBinPath);
					sprintf(gamePort, "%d", forkServer.forkPort);
					
					onUnloadConfig(); // will close the lobbyserverbase logs
					stopOp(); // will close the alcnet logs as well as the socket
					alcOnFork(); // will close the global logs
					
					if (forkServer.loadSDL)
						execlp(gameBin, gameBin,"-p",gamePort,"-guid",gameGuid,"-name",gameName,
								"-log",gameLog,"-c",gameConfig,"-v","0","-L",NULL);
					else
						execlp(gameBin,gameBin,"-p",gamePort,"-guid",gameGuid,"-name",gameName,
								"-log",gameLog,"-c",gameConfig,"-v","0",NULL);
					
					// if we come here, there was an error in the execlp call (but we're still in the game server process!)
					// weve already shut down the logs, so we have to get them up again
					alcLogInit();
					tLog *log = new tLog("fork_err.log", 2, DF_APPEND);
					log->log("There was an error starting the game server %s (GUID: %s, Port: %s)\n", gameBin, gamePort, gamePort);
					delete log;
					exit(-1); // exit the game server process
				}
				// this is the parent process
				else if (pid < 0) {
					lerr->log("Can't fork game server\n");
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

