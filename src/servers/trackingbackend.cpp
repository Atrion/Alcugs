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
		ip[0] = 0;
		port_start = 5001;
		port_end = 6000;
		childs = new tNetSessionList;
	}
	
	//// tPlayer
	char *tPlayer::str(void)
	{
		static char cnt[1024];
		if (age_name[0] != 0)
			sprintf(cnt, "[%d@%s]", ki, age_name);
		else
			sprintf(cnt, "[%d]", ki);
		return cnt;
	}
	
	//// tTrackingBackend
	tTrackingBackend::tTrackingBackend(tNetSessionList *servers)
	{
		log = lnull;
		this->servers = servers;
		size = 0;
		players = NULL;
		loadSettings();
		guidGen = new tGuidGen();
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
	
	void tTrackingBackend::findServer(tPlayer *player, Byte *guid, const Byte *name)
	{
		log->log("Player %s wants to link to %s (%s)\n", player->str(), name, alcGetStrGuid(guid, 8));
		Byte zeroguid[8];
		memset(zeroguid, 0, 8);
		if (memcmp(guid, zeroguid, 8) == 0) {
			if (!guidGen->generateGuid(guid, name, player->ki)) {
				// TODO: kick the player?
				log->log(" Can\'t generate a GUID for this unknown age, ignoring\n");
				lerr->log("Unable to generate GUID for age %s\n", name);
				return;
			}
			log->log(" Generated GUID: %s\n", alcGetStrGuid(guid, 8));
		}
		memcpy(player->guid, guid, 8);
		strncpy((char *)player->age_name, (char *)name, 199);
		tNetSession *server = NULL, *game = NULL;
		servers->rewind();
		while ((server = servers->getNext())) {
			if (server->data && memcmp(server->guid, guid, 8) == 0 && strncmp((char *)server->name, (char *)name, 199) == 0) {
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
				freePorts[ntohs(server->getPort()) - data->port_start] = false;
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
			tmCustomForkServer forkServer(lobby, player->ki, player->x, lowest, guid, name, false); // FIXME: atm it never loads the SDL state
			lobby->send(forkServer);
			player->waiting = true;
			log->log("Spawning new game server %s (GUID: %s, port: %d) on %s\n", name, alcGetStrGuid(guid, 8), lowest, lobby->str());
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
			if (memcmp(players[i]->guid, server->guid, 8) != 0 || strncmp((char *)players[i]->age_name, (char *)server->name, 199)) continue;
			// ok, this player is waiting for this age, let's tell him about it
			serverFound(players[i], server);
		}
	}
	
	void tTrackingBackend::serverFound(tPlayer *player, tNetSession *server)
	{
		assert(server->data != 0);
		if (!player->u) {
			lerr->log("ERR: Found age for player %s, but I don't know how to contact him\n", player->str());
			return;
		}
		tTrackingData *data = (tTrackingData *)server->data;
		tmCustomServerFound found(player->u, player->ki, player->x, ntohs(server->getPort()), data->ip, server->guid, server->name);
		player->u->send(found);
		player->waiting = false;
		log->log("Found age for player %s\n", player->str());
	}
	
	tPlayer *tTrackingBackend::getPlayer(U32 ki)
	{
		for (int i = 0; i < size; ++i) {
			if (players[i] && players[i]->ki == ki) return players[i];
		}
		return NULL;
	}
	
	void tTrackingBackend::updateServer(tNetSession *game, tmCustomSetGuid &setGuid)
	{
		memcpy(game->guid, setGuid.guid, 8);
		strncpy((char *)game->name, (char *)setGuid.age.c_str(), 199);
		
		tTrackingData *data = (tTrackingData *)game->data;
		if (data) return; // ignore the rest if the info if we already got it. IP and Port can't change.
		data = new tTrackingData;
		data->isLobby = (ntohs(game->getPort()) == 5000); // FIXME: the criteria to determine whether it's a lobby or a game server is BAD
		strncpy((char *)data->ip, (char *)setGuid.ip_str.c_str(), 49);
		if (!data->isLobby) { // let's look to which lobby this server belongs
			tNetSession *server = NULL, *lobby = NULL;
			servers->rewind();
			while ((server = servers->getNext())) {
				if (server->data && server->getIP() == game->getIP() && ((tTrackingData *)server->data)->isLobby) {
					lobby = server;
					break;
				}
			}
			if (lobby) {
				((tTrackingData *)server->data)->childs->add(game); // add the game server to the list of children of that lobby
				data->parent = lobby;
			}
			else
				lerr->log("ERR: Found game server %s without a Lobby belonging to it\n", game->str());
		}
		game->data = data;
		log->log("Found server at %s\n", game->str());
		
		notifyWaiting(game);
		log->flush();
	}
	
	void tTrackingBackend::updatePlayer(tNetSession *game, tmCustomPlayerStatus &playerStatus)
	{
		tPlayer *player = getPlayer(playerStatus.ki);
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
		}
		player->x = playerStatus.x;
		player->u = game;
		// TODO: log something useful here
	}
	
	void tTrackingBackend::removePlayer(int player)
	{
		if (player >= size) return;
		if (players[player] != NULL) {
			delete players[player];
			players[player] = NULL;
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
		// remove all players which were still on this server
		int num_removed = 0;
		for (int i = 0; i < size; ++i) {
			if (players[i] && players[i]->u == game) {
				removePlayer(i);
				++num_removed;
			}
		}
		if (num_removed > 0) log->log("WARN: Server %s is quitting though it still had %d players\n", game->str(), num_removed);
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
	}

} //end namespace alc

