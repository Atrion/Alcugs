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

/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGBACKEND_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <unet.h>

////extra includes
#include "trackingbackend.h"

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	//// tTrackingData
	tTrackingData::tTrackingData()
	{
		isLobby = false;
		parent = NULL;
		externalIp[0] = 0;
		port_start = 5001;
		port_end = 6000;
		childs = new tNetSessionList;
	}
	
	//// tPlayer
	tPlayer::tPlayer(U32 ki)
	{
		this->ki = ki;
		this->x = 0;
		account[0] = avatar[0] = 0;
		flag = status = 0;
		ip = port = 0;
		u = NULL;
		waiting = false;
	}
	
	char *tPlayer::str(void)
	{
		static char cnt[1024];
		if (waiting)
			sprintf(cnt, "[%s@%s][%d@@%s]", avatar, account, ki, awaiting_age);
		else if (u)
			sprintf(cnt, "[%s@%s][%d@%s]", avatar, account, ki, u->name);
		else
			sprintf(cnt, "[%s@%s][%d]", avatar, account, ki);
		return cnt;
	}
	
	//// tTrackingBackend
	tTrackingBackend::tTrackingBackend(tNetSessionList *servers, char *host, U16 port)
	{
		log = lnull;
		this->servers = servers;
		this->host = host;
		this->port = port;
		size = count = lastUpdate = 0;
		players = NULL;
		loadSettings();
		guidGen = new tGuidGen();
		generateFakeGuid(fakeLobbyGuid);
	}
	
	tTrackingBackend::~tTrackingBackend(void)
	{
		delete guidGen;
		if (players != NULL) {
			int num_deleted = 0;
			for (int i = 0; i < size; ++i) {
				if (players[i]) {
					delete players[i];
					++num_deleted;
				}
			}
			if (num_deleted > 0) lerr->log("ERR: The backend is quitting, and there were still %d players online\n", num_deleted);
			free((void *)players);
		}
		if (log != lnull) delete log;
	}
	
	void tTrackingBackend::reload(void)
	{
		if (log != lnull) {
			delete log;
			log = lnull;
		}
		delete guidGen;
		loadSettings();
		guidGen = new tGuidGen();
	}
	
	void tTrackingBackend::findServer(tmCustomFindServer &findServer)
	{
		statusFileUpdate = true;
		
		tPlayer *player = getPlayer(findServer.ki);
		if (!player) {
			lerr->log("ERR: Ignoring a NetMsgCustomFindServer for player with KI %d since I can't find that player\n", findServer.ki);
			return;
		}
		player->x = findServer.x;
		player->ip = findServer.ip;
		player->port = findServer.port;
		log->log("Player %s[%s:%d] wants to link to %s (%s)\n", player->str(), alcGetStrIp(player->ip), player->port, findServer.age.c_str(), alcGetStrGuid(findServer.guid, 8));
		Byte zeroguid[8];
		memset(zeroguid, 0, 8);
		if (memcmp(findServer.guid, zeroguid, 8) == 0) {
			if (!guidGen->generateGuid(player->awaiting_guid, findServer.age.c_str(), player->ki)) {
				log->log("WARN: Request to link to unknown age %s - kicking player %s\n", findServer.age.c_str(), player->str());
				tmPlayerTerminated term(player->u, player->ki, RKickedOff);
				player->u->send(term);
				return;
			}
			log->log(" Generated GUID: %s\n", alcGetStrGuid(player->awaiting_guid, 8));
		}
		else
			memcpy(player->awaiting_guid, findServer.guid, 8);
		// copy data to player
		player->status = RInRoute;
		strncpy((char *)player->awaiting_age, (char *)findServer.age.c_str(), 199);
		player->waiting = true;
		// search for the game server the player needs
		tNetSession *server = NULL, *game = NULL;
		servers->rewind();
		while ((server = servers->getNext())) {
			if (server->data && memcmp(server->guid, player->awaiting_guid, 8) == 0 && strncmp((char *)server->name, (char *)player->awaiting_age, 199) == 0) {
				game = server; // we found it
				break;
			}
		}
		if (!game) {
			// search for the lobby with the least load
			tNetSession *lobby = NULL;
			int load = -1;
			servers->rewind();
			while ((server = servers->getNext())) {
				if (!server->data) continue;
				tTrackingData *data = (tTrackingData*)server->data;
				if (data->isLobby && (load < 0 || ((tTrackingData*)server->data)->childs->getCount() < load)) {
					lobby = server;
					load = ((tTrackingData*)server->data)->childs->getCount();
				}
			}
			if (!lobby) {
				lerr->log("ERR: There's no lobby I could use to spawn the server\n");
				return;
			}
			// search for free ports
			tTrackingData *data = (tTrackingData*)lobby->data;
			int nPorts = data->port_end - data->port_start + 1;
			bool *freePorts = new bool[nPorts];
			for (int i = 0; i < nPorts; ++i) freePorts[i] = true;
			data->childs->rewind();
			while ((server = data->childs->getNext()))
				freePorts[ntohs(server->getPort()) - data->port_start] = false; // this port is occupied
			int lowest;
			for (lowest = 0; lowest < nPorts; ++lowest) {
				if (freePorts[lowest]) break; // we found a free one
			}
			lowest += data->port_start;
			delete[] freePorts;
			if (lowest == nPorts) { // no free port on the lobby with the least childs
				lerr->log("ERR: No free port on lobby %s, can't spawn game server\n", lobby->str());
				return;
			}
			// ok, telling the lobby to fork
			bool loadState = doesAgeLoadState(player->awaiting_age);
			tmCustomForkServer forkServer(lobby, player->ki, player->x, lowest, player->awaiting_guid, player->awaiting_age, loadState);
			lobby->send(forkServer);
			log->log("Spawning new game server %s (GUID: %s, port: %d) on %s ", player->awaiting_age, alcGetStrGuid(player->awaiting_guid, 8), lowest, lobby->str());
			if (loadState) log->print("(loading age state)\n");
			else log->print("(not loading age state)\n");
		}
		else {
			// ok, we got it, let's tell the player about it
			serverFound(player, game);
		}
		log->flush();
	}
	
	void tTrackingBackend::notifyWaiting(tNetSession *server)
	{
		for (int i = 0; i < size; ++i) {
			if (!players[i] || !players[i]->waiting) continue;
			if (memcmp(players[i]->awaiting_guid, server->guid, 8) != 0 || strncmp((char *)players[i]->awaiting_age, (char *)server->name, 199)) continue;
			// ok, this player is waiting for this age, let's tell him about it
			serverFound(players[i], server);
		}
	}
	
	void tTrackingBackend::serverFound(tPlayer *player, tNetSession *server)
	{
		assert(server->data != 0);
		// notifiy the player that it's server is available
		tTrackingData *data = (tTrackingData *)server->data;
		tmCustomServerFound found(player->u, player->ki, player->x, ntohs(server->getPort()), data->externalIp, server->guid, server->name);
		player->u->send(found);
		log->log("Found age for player %s\n", player->str());
		// no longer waiting
		player->waiting = false;
		player->awaiting_age[0] = 0;
		memset(player->awaiting_guid, 0, 8);
	}
	
	tPlayer *tTrackingBackend::getPlayer(U32 ki, int *nr)
	{
		for (int i = 0; i < size; ++i) {
			if (players[i] && players[i]->ki == ki) {
				if (nr) *nr = i;
				return players[i];
			}
		}
		if (nr) *nr = -1;
		return NULL;
	}
	
	void tTrackingBackend::updateServer(tNetSession *game, tmCustomSetGuid &setGuid)
	{
		statusFileUpdate = true;
		// search if another game server for that guid is already running. in that case, ignore this one
		tNetSession *server;
		while ((server = servers->getNext())) {
			if (server == game || !server->data) continue;
			if (memcmp(server->guid, setGuid.guid, 8) == 0) {
				lerr->log("There already is a server for guid %s, ignoring the new one %s\n", alcGetStrGuid(setGuid.guid, 8), game->str());
				return;
			}
		}
		
		memcpy(game->guid, setGuid.guid, 8);
		strncpy((char *)game->name, (char *)setGuid.age.c_str(), 199);
		
		tTrackingData *data = (tTrackingData *)game->data;
		if (data) return; // ignore the rest if the info if we already got it. IP and Port can't change.
		data = new tTrackingData;
		data->isLobby = (ntohs(game->getPort()) == 5000); // FIXME: the criteria to determine whether it's a lobby or a game server is BAD
		strncpy((char *)data->externalIp, (char *)setGuid.externalIp.c_str(), 99);
		if (!data->isLobby) { // let's look to which lobby this server belongs
			tNetSession *lobby = NULL;
			server = NULL;
			servers->rewind();
			while ((server = servers->getNext())) {
				if (server->data && ((tTrackingData *)server->data)->isLobby && server->getIp() == game->getIp()) {
					lobby = server;
					break;
				}
			}
			if (lobby) { // we found the server's lobby
				((tTrackingData *)server->data)->childs->add(game); // add the game server to the list of children of that lobby
				data->parent = lobby;
			}
			else
				log->log("WARN: Found game server %s without a Lobby belonging to it\n", game->str());
		}
		else
			generateFakeGuid(data->agentGuid); // create guid for UruVision
		game->data = data;
		log->log("Found server at %s\n", game->str());
		
		notifyWaiting(game);
		log->flush();
	}
	
	void tTrackingBackend::updatePlayer(tNetSession *game, tmCustomPlayerStatus &playerStatus)
	{
		statusFileUpdate = true;
		/* Flags:
		0: delete
		1: set invisible
		2: set visible
		3: set only buddies */
		int nr;
		tPlayer *player = getPlayer(playerStatus.ki, &nr);
		if (playerStatus.playerFlag == 0) {
			if (!player) return;
			log->log("Player %s quit\n", player->str());
			removePlayer(nr);
		}
		else if (playerStatus.playerFlag >= 1 && playerStatus.playerFlag <= 3) {
			if (!player) { // it doesn't exist, create it
				int slot = -1;
				for (int i = 0; i < size; ++i) {
					if (players[i] == NULL) {
						slot = i;
						break;
					}
				}
				if (slot < 0) {
					++size;
					players = (tPlayer **)realloc((void *)players, size*sizeof(tPlayer*));
					slot = size-1;
				}
				player = players[slot] = new tPlayer(playerStatus.ki);
				++count;
			}
			else { // if it already exists, check if the avi is already logged in elsewhere
				// to do so, we first check if the game server the players uses changed. if that's the case, and the player did not request to link, kick the old player
				if (player->u != game && player->status != RInRoute && player->status != RLeaving) {
					tmPlayerTerminated term(player->u, player->ki, RLoggedInElsewhere);
					player->u->send(term);
				}
			}
			// update the player's data
			player->x = playerStatus.x;
			player->u = game;
			player->flag = playerStatus.playerFlag;
			player->status = playerStatus.playerStatus;
			strcpy((char *)player->avatar, (char *)playerStatus.avatar.c_str());
			strcpy((char *)player->account, (char *)playerStatus.account.c_str());
			memcpy(player->uid, playerStatus.guid, 16);
			// no longer waiting
			player->waiting = false;
			player->awaiting_age[0] = 0;
			memset(player->awaiting_guid, 0, 8);
			log->log("Got status update for player %s: 0x%02X (%s)\n", player->str(), playerStatus.playerStatus,
					alcUnetGetReasonCode(playerStatus.playerStatus));
		}
		else {
			lerr->log("Got unknown flag 0x%02X from player with KI %d\n", playerStatus.playerFlag, playerStatus.ki);
			lerr->flush();
		}
		log->flush();
	}
	
	void tTrackingBackend::removePlayer(int player)
	{
		if (player >= size) return;
		statusFileUpdate = true;
		if (players[player] != NULL) {
			delete players[player];
			players[player] = NULL;
			--count;
		}
		
		int last = size-1; // find the last player
		while (last >= 0 && players[last] == NULL) --last;
		if (last < size-1) { // there are some NULLs at the end, shrink the array
			size=last+1;
			players=(tPlayer **)realloc(players, sizeof(tPlayer*) * size); // it's not a bug if we get NULL here - the size might be 0
		}
	}
	
	void tTrackingBackend::removeServer(tNetSession *game)
	{
		statusFileUpdate = true;
		// remove all players which were still on this server
		for (int i = 0; i < size; ++i) {
			if (players[i] && players[i]->u == game) {
				log->log("WARN: Removing player %s as it was on a terminating server\n", players[i]->str());
				removePlayer(i);
			}
		}
		if (game->data)
			log->log("Server %s is leaving us\n", game->str());
		log->flush();
		// remove this server from the list of childs of its lobby
		if (!game->data) return;
		tTrackingData *data = (tTrackingData *)game->data;
		if (data->isLobby) {
			// it's childs are lobbyless now
			tNetSession *server;
			data->childs->rewind();
			while ((server = data->childs->getNext())) {
				assert(server->data != 0); // all childs must have data
				((tTrackingData *)server->data)->parent = NULL;
			}
		}
		else if (data->parent) {
			assert(data->parent->data != 0); // the parent must have data
			tTrackingData *parent_data = (tTrackingData *)data->parent->data;
			parent_data->childs->remove(game);
		}
	}
	
	void tTrackingBackend::loadSettings(void)
	{
		tConfig *cfg = alcGetConfig();
		
		tStrBuf var = cfg->getVar("tracking.log");
		if (log == lnull && (var.isNull() || var.asByte())) { // logging enabled per default
			log = new tLog("tracking.log", 4, 0);
			log->log("Tracking driver started (%s)\n\n", __U_TRACKINGBACKEND_ID);
			log->flush();
		}
		
		var = cfg->getVar("tracking.tmp.hacks.agestate");
		loadAgeState = (var.isNull() || var.asByte());
		var = cfg->getVar("tracking.tmp.hacks.resetting_ages");
		if (var.isNull()) strcpy((char *)resettingAges, "Cleft,DniCityX2Finale,GreatZero,Kveer,Myst,Neighborhood02,Personal02,RestorationGuild,spyroom");
		else strncpy((char *)resettingAges, (char *)var.c_str(), 1023);
		
		var = cfg->getVar("track.html");
		statusHTML = (!var.isNull() && var.asByte());
		var = cfg->getVar("track.html.path");
		if (var.isNull()) statusHTML = false;
		else strncpy((char *)statusHTMLFile, (char *)var.c_str(), 255);
		var = cfg->getVar("track.xml");
		statusXML = (!var.isNull() && var.asByte());
		var = cfg->getVar("track.xml.path");
		if (var.isNull()) statusXML = false;
		else strncpy((char *)statusXMLFile, (char *)var.c_str(), 255);
		statusFileUpdate = true;
	}
	
	bool tTrackingBackend::doesAgeLoadState(const Byte *age)
	{
		if (!loadAgeState) return false;
		
		// local copy of resetting age list as strsep modifies it
		char ages[1024];
		strcpy(ages, (char *)resettingAges);
		
		char *buf = ages;
		char *p = strsep(&buf, ",");
		while (p != 0) {
			if (strcmp(p, (char *)age) == 0) return false;
			p = strsep(&buf, ",");
		}
		return true;
	}
	
	void tTrackingBackend::forwardMessage(tmCustomDirectedFwd &directedFwd)
	{
		// for each game server, look which clients of that server need this message. This way, only one message is sent per server.
		tNetSession *server;
		tTrackingData *data;
		bool sent;
		servers->rewind();
		while ((server = servers->getNext())) {
			if (server == directedFwd.getSession() || !server->data) continue; // don't send the message back to the server which sent it
			data = (tTrackingData *)server->data;
			if (data->isLobby) continue; // don't send messages to players in a lobby
			sent = false; // so far, the message was not sent to this server
			DBG(5, "looking for players on %s\n", server->str());
			for (int i = 0; i < size; ++i) {
				if (!players[i] || players[i]->u != server) continue; // search only for players on this server
				for (int j = 0; j < directedFwd.nRecipients; ++j) {
					if (directedFwd.recipients[j] == players[i]->ki) { // one of the recipients is on this server, so send the message
						tmCustomDirectedFwd forwardedMsg(server, directedFwd);
						server->send(forwardedMsg);
						sent = true;
						break;
					}
				}
				if (sent) break; // if we already sent the message here, its not necessary to go on searching for players
			}
		}
	}
	
	void tTrackingBackend::generateFakeGuid(Byte *guid)
	{
		*(U16 *)(guid)=0xFFFF;
		*(U32 *)(guid+2)=(U32)random();
		*(Byte *)(guid+6)=(Byte)alcGetMicroseconds();
	}
	
	void tTrackingBackend::updateStatusFile(void)
	{
		if (!statusFileUpdate && lastUpdate > alcGetTime()-5*60) return; // update at least every 5 minutes
		DBG(9, "Printing the online list to %s\n", statusHTMLFile);
		
		if (statusHTML) printStatusHTML();
		if (statusXML) printStatusXML();
		statusFileUpdate = false;
		lastUpdate = alcGetTime();
	}
	
	void tTrackingBackend::printStatusHTML(void)
	{
		FILE *f = fopen((char *)statusHTMLFile, "w");
		// header
		fprintf(f, "<html><head><title>Shard Status</title></head><body>\n");
		fprintf(f, "Last Update: %s<br />\n", alcGetStrTime());
		// player list
		fprintf(f, "<h2>Current Online Players</h2>\n");
		fprintf(f, "<b>Total population: %d</b><br /><br />\n", count);
		fprintf(f, "<table border=\"1\"><tr><th>Avie (Account)</th><th>KI</th><th>Age</th><th>GUID</th><th>Status</th></tr>\n");
		for (int i = 0; i < size; ++i) {
			if (!players[i]) continue;
			fprintf(f, "<tr><td>%s (%s)</td><td>%d</td><td>%s</td><td>%s</td>", players[i]->avatar, players[i]->account, players[i]->ki,
					players[i]->u->name, alcGetStrGuid(players[i]->u->guid, 8));
			if (players[i]->waiting && players[i]->status == RInRoute) fprintf(f, "<td>InRoute to %s</td></tr>\n", players[i]->awaiting_age);
			else fprintf(f, "<td>%s</td></tr>\n", alcUnetGetReasonCode(players[i]->status));
		}
		fprintf(f, "</table><br /><br />\n");
		// server list
		tNetSession *server;
		fprintf(f,"<h2>Current Server Instances</h2>");
		fprintf(f, "<table border=\"1\"><tr><th>Age</th><th>GUID</th><th>IP and Port</th></tr>\n");
		servers->rewind();
		while ((server = servers->getNext())) {
			if (!server->data) {
				fprintf(f, "<tr><td colspan=\"2\" style=\"color:red\">Unknown (not a game or lobby server)</td><td>%s:%d</td><tr>\n",
					alcGetStrIp(server->getIp()), ntohs(server->getPort()));
				continue;
			}
			fprintf(f, "<tr><td>%s</td><td>%s</td><td>%s:%d</td><tr>\n", server->name, alcGetStrGuid(server->guid, 8), alcGetStrIp(server->getIp()), ntohs(server->getPort()));
		}
		fprintf(f, "</table>\n");
		// footer
		fprintf(f, "</html>\n");
		fclose(f);
	}
	
	void tTrackingBackend::printStatusXML(void)
	{
		tNetSession *server;
		bool needFake = false;
		FILE *f = fopen((char *)statusXMLFile, "w");
		
		fprintf(f, "<?xml version='1.0'?>\n");
		fprintf(f, "<SystemView>\n");
			fprintf(f, "<Version>2.0</Version>\n");
			fprintf(f, "<Lookup>\n");
				fprintf(f, "<Server>\n");
					fprintf(f, "<ServerInfo>\n");
						fprintf(f, "<Name>Tracking</Name>\n");
						fprintf(f, "<Type>7</Type>\n");
						fprintf(f, "<Addr>%s</Addr>\n", host);
						fprintf(f, "<Port>%i</Port>\n", port);
						fprintf(f, "<Guid>0000000000000000</Guid>\n");
					fprintf(f, "</ServerInfo>\n");
				fprintf(f, "</Server>\n");
				fprintf(f, "<Agents>\n");
				servers->rewind();
				while ((server = servers->getNext())) {
					if (!server->data) continue;
					tTrackingData *data = (tTrackingData *)server->data;
					if (!data->isLobby && !data->parent) needFake = true;
					else if (data->isLobby) printLobbyXML(f, server);
				}
				if (needFake) printLobbyXML(f, NULL);
				fprintf(f, "</Agents>\n");
			fprintf(f, "</Lookup>\n");
		fprintf(f, "</SystemView>\n");
		fclose(f);
	}
	
	void tTrackingBackend::printLobbyXML(FILE *f, tNetSession *lobby)
	{	// when lobby is NULL, we're printing the fake lobby
		tTrackingData *data = lobby ? (tTrackingData *)lobby->data : NULL;
		fprintf(f, "<Agent>\n");
			// Agent
			fprintf(f, "<ServerInfo>\n");
				fprintf(f, "<Name>Agent</Name>\n");
				fprintf(f, "<Type>1</Type>\n");
				if (lobby) {
					fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(lobby->getIp()));
					fprintf(f, "<Port>%d</Port>\n", ntohs(lobby->getPort()));
					fprintf(f, "<Guid>%s00</Guid>\n", alcGetStrGuid(data->agentGuid, 7));
				}
				else {
					fprintf(f, "<Addr>Fake Agent</Addr>\n");
					fprintf(f, "<Port>0</Port>\n");
					fprintf(f, "<Guid>%s00</Guid>\n", alcGetStrGuid(fakeLobbyGuid, 7));
				}
			fprintf(f, "</ServerInfo>\n");
			if (lobby) fprintf(f, "<ExternalAddr>%s</ExternalAddr>\n", data->externalIp);
			else       fprintf(f, "<ExternalAddr>Fake Agent</ExternalAddr>\n");
			fprintf(f, "<PlayerLimit>-1</PlayerLimit>\n");
			fprintf(f, "<GameLimit>-1</GameLimit>\n");
			// Lobby
			fprintf(f, "<Lobby><Process>\n");
				fprintf(f, "<Server>\n");
					fprintf(f, "<ServerInfo>\n");
						fprintf(f, "<Name>Lobby</Name>\n");
						fprintf(f, "<Type>2</Type>\n");
						if (lobby) {
							fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(lobby->getIp()));
							fprintf(f, "<Port>%d</Port>\n", ntohs(lobby->getPort()));
							fprintf(f, "<Guid>%s02</Guid>\n", alcGetStrGuid(data->agentGuid, 7));
						}
						else {
							fprintf(f, "<Addr>Fake Agent</Addr>\n");
							fprintf(f, "<Port>0</Port>\n");
							fprintf(f, "<Guid>%s02</Guid>\n", alcGetStrGuid(fakeLobbyGuid, 7));
						}
					fprintf(f, "</ServerInfo>\n");
				fprintf(f, "</Server>\n");
				fprintf(f, "<Players>\n");
					if (lobby) printPlayersXML(f, lobby);
				fprintf(f, "</Players>\n");
			fprintf(f, "</Process></Lobby>\n");
			// Game Servers
			fprintf(f, "<Games>\n");
				tNetSession *server;
				if (data) { // the lobby's children
					data->childs->rewind();
					while ((server = data->childs->getNext())) printGameXML(f, server);
				}
				else { // all game server without lobby
					servers->rewind();
					while ((server = servers->getNext())) {
						if (!server->data) continue;
						tTrackingData *data = (tTrackingData *)server->data;
						if (!data->isLobby && !data->parent) printGameXML(f, server);
					}
				}
			fprintf(f, "</Games>\n");
		fprintf(f, "</Agent>\n");
	}
	
	void tTrackingBackend::printPlayersXML(FILE *f, tNetSession *server)
	{
		for (int i = 0; i < size; ++i) {
			if (!players[i] || players[i]->u != server) continue;
			fprintf(f, "<Player>\n");
				fprintf(f, "<AcctName>%s</AcctName>\n", players[i]->account);
				fprintf(f, "<PlayerID>%i</PlayerID>\n", players[i]->ki);
				fprintf(f, "<PlayerName>%s</PlayerName>\n", players[i]->avatar);
				fprintf(f, "<AccountUUID>%s</AccountUUID>\n", alcGetStrGuid(players[i]->uid, 16));
				fprintf(f, "<State>%s</State>\n", alcUnetGetReasonCode(players[i]->status));
			fprintf(f, "</Player>\n");
		}
	}
	
	void tTrackingBackend::printGameXML(FILE *f, tNetSession *game)
	{
		fprintf(f, "<Game>\n");
			fprintf(f, "<Process>\n");
				fprintf(f, "<Server>\n");
					fprintf(f, "<ServerInfo>\n");
						fprintf(f, "<Name>%s</Name>\n", game->name);
						fprintf(f, "<Type>3</Type>\n");
						fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(game->getIp()));
						fprintf(f, "<Port>%i</Port>\n", ntohs(game->getPort()));
						fprintf(f, "<Guid>%s</Guid>\n", alcGetStrGuid(game->guid, 8));
					fprintf(f, "</ServerInfo>\n");
				fprintf(f, "</Server>\n");
				fprintf(f, "<Players>\n");
					printPlayersXML(f, game);
				fprintf(f, "</Players>\n");
			fprintf(f, "</Process>\n");
			fprintf(f, "<AgeLink>\n");
				fprintf(f, "<AgeInfo>\n");
					fprintf(f, "<AgeInstanceName>%s</AgeInstanceName>\n", game->name);
					fprintf(f, "<AgeInstanceGuid>%s</AgeInstanceGuid>\n", alcGetStrGuid(game->guid, 8));
					fprintf(f, "<AgeSequenceNumber>0</AgeSequenceNumber>\n");
				fprintf(f, "</AgeInfo>\n");
			fprintf(f, "</AgeLink>\n");
		fprintf(f, "</Game>\n");
	}

} //end namespace alc

