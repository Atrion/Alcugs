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

//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "trackingbackend.h"

#include "trackingserver.h"
#include <netexception.h>
#include <alcmain.h>
#include <protocol/umsgbasic.h>
#include <urutypes/plmessage.h>

#include <ctime>
#include <set>
#include <cstring>
#include <cassert>

/* The process of finding an age for a player works as follows:

Find age request => set the "waiting" variables for the player => check if there is a game server for this age
	no game server yet => ask lobby to spawn one
	there is a game server => send player to come message (and save this ina  per game-server list of players waiting)
New game server says hello (because we told lobby to spawn it) => tell players we found this one
Game server replies "player can come" => tell player we found the server
A game server is going down => Check if there is anyone waiting for this server
	If yes => Argh, we needed this one! Tell lobby to spawn a new server (but wait a second for the server to go down properly) */

namespace alc {

	//// tTrackingData
	tTrackingData::tTrackingData()
	{
		isLobby = false;
		portStart = portEnd = 0;
	}
	
	//// tPlayer
	tPlayer::tPlayer(uint32_t ki)
	{
		this->ki = ki;
		this->sid = 0;
		flag = status = 0;
		waiting = false;
	}
	
	tString tPlayer::str(void) const
	{
		tString cnt;
		if (waiting)
			cnt.printf("[%s@%s][%d@@%s]", avatar.c_str(), account.c_str(), ki, awaiting_age.c_str());
		else if (*u)
			cnt.printf("[%s@%s][%d@%s]", avatar.c_str(), account.c_str(), ki, u->name.c_str());
		else
			cnt.printf("[%s@%s][%d]", avatar.c_str(), account.c_str(), ki);
		return cnt;
	}
	
	//// tTrackingBackend
	tTrackingBackend::tTrackingBackend(tUnetTrackingServer *net) : net(net)
	{
		generateFakeGuid(fakeLobbyGuid);
	}
	
	tTrackingBackend::~tTrackingBackend(void)
	{
		if (players.size())
			alcGetMain()->err()->log("ERR: The backend is quitting, and there were still %Zd players online\n", players.size());
	}
	
	void tTrackingBackend::applyConfig(void)
	{
		tConfig *cfg = alcGetMain()->config();
		
		tString var = cfg->getVar("tracking.log");
		if (var.isEmpty() || var.asInt()) { // logging enabled per default
			log.open("tracking.log");
			log.log("Tracking driver started\n\n");
			log.flush();
		}
		else log.close();
		
		var = cfg->getVar("track.html");
		statusHTML = (!var.isEmpty() && var.asInt());
		statusHTMLFile = cfg->getVar("track.html.path");
		if (statusHTMLFile.isEmpty()) statusHTML = false;
		
		var = cfg->getVar("track.htmldbg");
		statusHTMLdbg = (!var.isEmpty() && var.asInt());
		statusHTMLdbgFile = cfg->getVar("track.htmldbg.path");
		if (statusHTMLdbgFile.isEmpty()) statusHTMLdbg = false;
		
		var = cfg->getVar("track.xml");
		statusXML = (!var.isEmpty() && var.asInt());
		statusXMLFile = cfg->getVar("track.xml.path");
		if (statusXMLFile.isEmpty()) statusXML = false;
	}
	
	void tTrackingBackend::findServer(tmCustomFindServer &findServer)
	{
		assert(alcGetSelfThreadId() != alcGetMain()->threadId());
		
		tPlayerList::iterator player = getPlayer(findServer.ki);
		if (player == players.end()) {
			log.log("ERR: Ignoring a NetMsgCustomFindServer for player with KI %d since I can't find that player\n", findServer.ki);
			return;
		}
		player->sid = findServer.sid;
		player->awaiting_x = findServer.x;
		log.log("Player %s wants to link to %s (%s)\n", player->str().c_str(), findServer.age.c_str(), findServer.serverGuid.c_str());
		// copy data to player
		player->status = RInRoute;
		alcGetHexGuid(player->awaiting_guid, findServer.serverGuid);
		player->awaiting_age = findServer.age;
		player->waiting = true;
		// search for the game server the player needs
		tNetSession *game = NULL;
		{
			tReadLock lock(net->smgrMutex);
			for (tNetSessionMgr::tIterator it(net->smgr); it.next();) {
				if (it->data && memcmp(it->serverGuid, player->awaiting_guid, 8) == 0 && it->name == player->awaiting_age) {
					game = *it; // we found it
					break;
				}
			}
		}
		if (!game) {
			spawnServer(player->awaiting_age, player->awaiting_guid);
		}
		else {
			// ok, we got it, let's make sure it is running
			tTrackingData *data = dynamic_cast<tTrackingData*>(game->data);
			if (!data) throw txUnet(_WHERE("server found in tTrackingBackend::findServer is not a game/lobby server"));
			data->waitingPlayers.push_back(player->ki);
			tmCustomPlayerToCome playerToCome(game, player->ki);
			net->send(playerToCome);
		}
		log.flush();
		updateStatusFile();
	}
	
	void tTrackingBackend::playerCanCome(alc::tNetSession* game, uint32_t ki)
	{
		assert(alcGetSelfThreadId() != alcGetMain()->threadId());
		tTrackingData *data = dynamic_cast<tTrackingData*>(game->data);
		if (!data) throw txUnet(_WHERE("server passed in tTrackingBackend::playerCanCome is not a game/lobby server"));
		log.log("Game server %s tells us that player %d can join\n", game->str().c_str(), ki);
		log.flush();
		for (tTrackingData::tPlayerList::iterator it = data->waitingPlayers.begin(); it != data->waitingPlayers.end(); ++it) {
			if (*it == ki) {
				// it is indeed in the list
				tPlayerList::iterator player = getPlayer(ki);
				if (player != players.end())
					serverFound(&*player, game);
				else
					log.log("ERR: Game server %s told us that player %d can come, but the player no longer exists\n", game->str().c_str(), ki);
				data->waitingPlayers.erase(it);
				log.flush();
				return;
			}
		}
		// The player is not in the list of waiting players - weird
		log.log("ERR: Game server %s told us that player %d can come, but the player doesn't even wait for this server\n", game->str().c_str(), ki);
		log.flush();
	}
	
	void tTrackingBackend::spawnServer(const alc::tString& age, const uint8_t* guid, double delay)
	{
		// search for the lobby with the least load
		tNetSession *lobby = NULL;
		size_t load = -1; // biggest existing integer, any real load will be smaller
		{
			tReadLock lock(net->smgrMutex);
			for (tNetSessionMgr::tIterator it(net->smgr); it.next();) {
				tTrackingData *data = dynamic_cast<tTrackingData*>(it->data);
				if (data && data->isLobby && data->children.size() < load) {
					lobby = *it;
					load = data->children.size();
				}
			}
		}
		if (!lobby) {
			log.log("ERR: There's no lobby I could use to spawn the server\n");
			return;
		}
		// search for free ports
		tTrackingData *data = static_cast<tTrackingData*>(lobby->data); // we already checked the type above
		int nPorts = data->portEnd - data->portStart + 1;
		bool *freePorts = new bool[nPorts];
		for (int i = 0; i < nPorts; ++i) freePorts[i] = true;
		for (tTrackingData::tSessionList::iterator it = data->children.begin(); it != data->children.end(); ++it)
			freePorts[ntohs((*it)->getPort()) - data->portStart] = false; // this port is occupied
		int lowest;
		for (lowest = 0; lowest < nPorts; ++lowest) {
			if (freePorts[lowest]) break; // we found a free one
		}
		delete []freePorts;
		if (lowest == nPorts) { // no free port on the lobby with the least children
			log.log("ERR: No free port on lobby %s, can't spawn game server\n", lobby->str().c_str());
			return;
		}
		lowest += data->portStart;
		// ok, telling the lobby to fork
		tmCustomForkServer forkServer(lobby, lowest, alcGetStrGuid(guid), age);
		net->send(forkServer, delay);
		log.log("Spawning new game server %s (Server GUID: %s, port: %d) on %s\n", age.c_str(), alcGetStrGuid(guid).c_str(), lowest, lobby->str().c_str());
	}
	
	void tTrackingBackend::notifyWaiting(tNetSession *server)
	{
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (!it->waiting) continue;
			if (memcmp(it->awaiting_guid, server->serverGuid, 8) != 0 || server->name != it->awaiting_age) continue;
			// ok, this player is waiting for this age, let's tell him about it
			serverFound(&*it, server);
		}
	}
	
	void tTrackingBackend::serverFound(tPlayer *player, tNetSession *server)
	{
		tTrackingData *data = dynamic_cast<tTrackingData *>(server->data);
		if (!data) throw txUnet(_WHERE("server passed in tTrackingBackend::serverFound is not a game/lobby server"));
		// notifiy the player that it's server is available
		tmCustomServerFound found(*player->u, player->ki, player->awaiting_x, player->sid, ntohs(server->getPort()), data->externalIp, alcGetStrGuid(server->serverGuid), server->name);
		net->send(found);
		log.log("Found age for player %s\n", player->str().c_str());
		// no longer waiting
		player->waiting = false;
	}
	
	tTrackingBackend::tPlayerList::iterator tTrackingBackend::getPlayer(uint32_t ki)
	{
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (it->ki == ki)
				return it;
		}
		return players.end();
	}
	
	void tTrackingBackend::updateServer(tNetSession *game, tmCustomSetGuid &setGuid)
	{
		assert(alcGetSelfThreadId() != alcGetMain()->threadId());
		tReadLock lock(net->smgrMutex);
		uint8_t serverGuid[8];
		alcGetHexGuid(serverGuid, setGuid.serverGuid);
		bool isLobby = setGuid.validSpawnPorts();
		if (isLobby) {
			// Lobbies msut have an all-zero GUID
			uint8_t zeroGuid[8];
			memset(zeroGuid, 0, 8);
			if (memcmp(serverGuid, zeroGuid, 8))
				throw txProtocolError(_WHERE("Invalid non-zero lobby GUID %s", setGuid.serverGuid.c_str()));
		}
		else {
			// search if another game server for that guid is already running. in that case, ignore this one
			for (tNetSessionMgr::tIterator it(net->smgr); it.next();) {
				if (*it == game || !it->data) continue;
				if (memcmp(it->serverGuid, serverGuid, 8) == 0) {
					log.log("ERR: There already is a server for guid %s, kicking the new one %s\n", setGuid.serverGuid.c_str(), game->str().c_str());
					game->terminate(); // this should usually result in the game server going down
					log.flush();
					return;
				}
			}
		}
		
		{
			tWriteLock lock(game->pubDataMutex);
			memcpy(game->serverGuid, serverGuid, 8);
			game->name = setGuid.age;
		}
		
		
		if (game->data) {
			if (dynamic_cast<tTrackingData*>(game->data)->isLobby != isLobby)
				throw txProtocolError(_WHERE("%s changed from lobby to non-lobby or vice versa?!?", game->str().c_str()));
			return; // ignore the rest of the info if we already got it. IP and Port can't change.
		}
		tTrackingData *data = new tTrackingData;
		data->isLobby = isLobby;
		if (isLobby) {
			data->isLobby = true;
			data->portStart = setGuid.spawnStart;
			data->portEnd = setGuid.spawnStop;
		}
		data->externalIp = setGuid.externalIp;
		if (!data->isLobby) { // let's look to which lobby this server belongs
			tNetSession *lobby = NULL;
			{
				for (tNetSessionMgr::tIterator it(net->smgr); it.next();) {
					tTrackingData *data = dynamic_cast<tTrackingData *>(it->data);
					if (data && data->isLobby && it->getIp() == game->getIp()) {
						lobby = *it;
						break;
					}
				}
			}
			if (lobby) { // we found the server's lobby
				static_cast<tTrackingData *>(lobby->data)->children.push_back(tNetSessionRef(game)); // add the game server to the list of children of that lobby
				data->parent = lobby;
			}
			else
				log.log("ERR: Found game server %s without a Lobby belonging to it\n", game->str().c_str());
		}
		else // if it is a lobby
			generateFakeGuid(data->agentGuid); // create guid for UruVision
		game->data = data; // save the data
		log.log("Found server at %s\n", game->str().c_str());
		
		notifyWaiting(game);
		log.flush();
		updateStatusFile();
	}
	
	void tTrackingBackend::updatePlayer(tNetSession *game, tmCustomPlayerStatus &playerStatus)
	{
		assert(alcGetSelfThreadId() != alcGetMain()->threadId());
		if (!game->data)
			// throwing this error will terminate the connection to this server, which in term should result in the server going down
			throw txProtocolError(_WHERE("server passed in tTrackingBackend::updatePlayer is not a game/lobby server"));
		/* Flags:
		0: delete
		1: set invisible
		2: set visible
		3: set only buddies (unused!) */
		tPlayerList::iterator player = getPlayer(playerStatus.ki);
		if (playerStatus.playerFlag == 0) {
			if (player != players.end()) {
				log.log("Player %s quit\n", player->str().c_str());
				players.erase(player);
			}
		}
		else if (playerStatus.playerFlag == 1 || playerStatus.playerFlag == 2) {
			if (player == players.end()) { // it doesn't exist, create it
				player = players.insert(players.end(), tPlayer(playerStatus.ki));
			}
			else if (*player->u != game) { // if it already exists, check if the avi is already logged in elsewhere
				// ignore a RLeaving message from the old game server that we got after the RJoining from the new
				if (playerStatus.playerStatus == RLeaving) {
					log.log("WARN: Got RLeaving from %s, but player is already logged in at %s. Ignoring.\n",
							game->str().c_str(), player->u->str().c_str());
					log.flush();
					return;
				}
				// to do so, we first check if the game server the player uses changed. if that's the case, and the player did not request to link, kick the old player
				if (player->status != RLeaving) {
					log.log("WARN: Kicking player %s at %s as it just logged in at %s\n", player->str().c_str(),
							player->u->str().c_str(), game->str().c_str());
					tmPlayerTerminated term(*player->u, player->ki, RLoggedInElsewhere);
					net->send(term);
				}
			}
			// update the player's data
			player->sid = playerStatus.sid;
			player->u = game;
			player->flag = playerStatus.playerFlag;
			player->status = playerStatus.playerStatus;
			player->avatar = playerStatus.avatar;
			player->account = playerStatus.account;
			memcpy(player->uid, playerStatus.uid, 16);
			// no longer waiting
			player->waiting = false;
			if (player->status != RLeaving) {
				// when he's leaving to another age, keep this info (to be shown on status page)
				player->awaiting_age.clear();
				memset(player->awaiting_guid, 0, 8);
				player->awaiting_x = 0;
			}
			log.log("Got status update for player %s: 0x%02X (%s)\n", player->str().c_str(), playerStatus.playerStatus,
					alcUnetGetReasonCode(playerStatus.playerStatus));
		}
		else {
			log.log("ERR: Got unknown flag 0x%02X for player with KI %d\n", playerStatus.playerFlag, playerStatus.ki);
		}
		log.flush();
		updateStatusFile();
	}
	
	void tTrackingBackend::removeServer(tNetSession *game)
	{
		assert(alcGetSelfThreadId() != alcGetMain()->threadId());
		tTrackingData *data = dynamic_cast<tTrackingData *>(game->data);
		if (!data) return;
		// if players are waiting for this server, we have a problem - we need it! But we can't stop it from going down, so instead launch it again after a second
		if (data->waitingPlayers.size()) {
			log.log("I need to respawn %s\n", game->str().c_str());
			spawnServer(game->name, game->serverGuid, /*delay*/1.0);
			
		}
		// remove all players which were still on this server
		tPlayerList::iterator it = players.begin();
		while (it != players.end()) {
			if (*it->u == game) {
				log.log("WARN: Removing player %s as it was on a terminating server\n", it->str().c_str());
				it = players.erase(it);
			}
			else
				++it; // we have to increment manually because above if block already increments
		}
		log.log("Server %s is leaving us\n", game->str().c_str());
		// remove this server from the list of children of its lobby/from the game server it is the lobby for
		if (data->isLobby) {
			// it's children are lobbyless now
			for (tTrackingData::tSessionList::iterator it = data->children.begin(); it != data->children.end(); ++it) {
				tTrackingData *subData = dynamic_cast<tTrackingData *>((*it)->data);
				if (!subData) throw txUnet(_WHERE("One child of the lobby I'm just deleting is not a game/lobby server"));
				subData->parent = NULL;
			}
		}
		else if (*data->parent) {
			tTrackingData *parentData = dynamic_cast<tTrackingData *>(data->parent->data);
			if (!parentData) throw txUnet(_WHERE("The parent of the game server I'm just deleting is not a game/lobby server"));
			parentData->children.remove(tNetSessionRef(game));
		}
		delete game->data;
		game->data = NULL;
		// done
		log.flush();
		updateStatusFile();
	}
	
	void tTrackingBackend::forwardMessage(tmCustomDirectedFwd &directedFwd)
	{
		assert(alcGetSelfThreadId() != alcGetMain()->threadId());
		// for each player, check if we can reach it, and save which game server we need to send the message to
		tTrackingData *data;
		std::set<tNetSession*> receivers; // to save where we already sent it
		tmCustomDirectedFwd::tRecList::iterator it = directedFwd.recipients.begin();
		while (it != directedFwd.recipients.end()) {
			bool removed = false;
			for (tPlayerList::iterator jt = players.begin(); jt != players.end(); ++jt) {
				if (*it != jt->ki) continue; // next one
				// ok, this player should get this message
				if (*jt->u == directedFwd.getSession()) break; // don't send it back to where it comes from
				if (jt->status == RLeaving) break; // the player is not connected
				data = dynamic_cast<tTrackingData *>(jt->u->data);
				if (!data || data->isLobby) break; // don't send messages to the lobby
				// ok, let's go - check if we already sent the message there
				if (!receivers.count(*jt->u)) {
					// no, we didn't - do it! and save that
					tmCustomDirectedFwd forwardedMsg(*jt->u, directedFwd);
					net->send(forwardedMsg);
					receivers.insert(*jt->u);
				}
				removed = true;
				it = directedFwd.recipients.erase(it); // since we already sent the message for this player we can remove it from our list
			}
			if (!removed) ++it; // go to next one if we did not yet do that
		}
		
		// check if we got all recipients - if not, tell the sender
		if (directedFwd.recipients.size()) {
			// format the text
			tString text("These players did not get the message (linking?): ");
			bool comma = false;
			for (it = directedFwd.recipients.begin(); it != directedFwd.recipients.end(); ++it) {
				if (comma) text.writeStr(", ");
				text.printf("%d", *it);
				comma = true;
			}
			// create the message
			tpKIMsg kiMsg = tpKIMsg(tUruObjectRef(), tString("Tracking Server"), 0, text);
			kiMsg.flags = 0x00004248;
			kiMsg.messageType = 0x0009; // private inter age chat
			// send message
			tmCustomDirectedFwd msg(directedFwd.getSession(), 0, &kiMsg);
			msg.recipients.push_back(directedFwd.ki);
			net->send(msg);
		}
	}
	
	void tTrackingBackend::getPopulationCounts(tmPublicAgeList& ageList)
	{
		ageList.populations.clear();
		for (tmPublicAgeList::tAgeList::iterator it = ageList.ages.begin(); it != ageList.ages.end(); ++it) {
			// search for players on this age
			int count = 0;
			for (tPlayerList::iterator jt = players.begin(); jt != players.end(); ++jt) {
				if (memcmp(jt->u->serverGuid, it->guid, 8) == 0) {
					++count;
				}
			}
			ageList.populations.push_back(count);
		}
		net->send(ageList);
	}
	
	void tTrackingBackend::generateFakeGuid(uint8_t* guid)
	{
		tMBuf buf;
		buf.put16(0xFFFF);
		buf.put32(random());
		buf.put8(time(NULL));
		buf.put8(0x00);
		buf.rewind();
		memcpy(guid, buf.read(8), 8);
	}
	
	void tTrackingBackend::updateStatusFile(void)
	{
		tReadLock lock(net->smgrMutex);
		if (statusHTML) printStatusHTML(/*dbg*/false);
		if (statusHTMLdbg) printStatusHTML(/*dbg*/true);
		if (statusXML) printStatusXML();
	}
	
	void tTrackingBackend::printStatusHTML(bool dbg)
	{
		tString statusfile = dbg ? statusHTMLdbgFile : statusHTMLFile;
		
		FILE *f = fopen(statusfile.c_str(), "w");
		if (!f) {
			alcGetMain()->err()->log("Can\'t open %s for writing - disabling HTML status page\n", statusfile.c_str());
			if (dbg) statusHTMLdbg = false;
			else statusHTML = false;
			return;
		}
		
		// header
		fprintf(f, "<html><head><title>Shard Status</title>\n");
		fprintf(f, "<meta http-equiv=\"refresh\" content=\"30;url=%s\" />\n", statusfile.filename().c_str());
		fprintf(f, "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=ISO-8859-1\">");
		fprintf(f, "</head><body>\n");
		fprintf(f, "<h2>Current Online Players</h2>\n");
		if (dbg) {
			// player list (dbg)
			fprintf(f, "<b>Total population: %Zi</b><br /><br />\n", players.size());
			fprintf(f, "<table border=\"1\"><tr><th>Avatar (Account)</th><th>KI</th><th>Age Name</th><th>Age GUID</th><th>Status</th></tr>\n");
			for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
				fprintf(f, "<tr><td>%s (%s)%s</td><td>%d</td><td>%s</td><td>%s</td>", it->avatar.c_str(), it->account.c_str(), it->flag == 2 ? "" : " [hidden]", it->ki, it->u->name.c_str(), alcGetStrGuid(it->u->serverGuid).c_str());
				if (!it->awaiting_age.isEmpty()) // if the age he wants to is saved, print it
					fprintf(f, "<td>%s to %s</td>", alcUnetGetReasonCode(it->status), it->awaiting_age.c_str());
				else fprintf(f, "<td>%s</td>", alcUnetGetReasonCode(it->status));
				fprintf(f, "</tr>\n");
			}
			fprintf(f, "</table><br />\n");
			// server list
			fprintf(f,"<h2>Current Server Instances</h2>");
			fprintf(f, "<table border=\"1\"><tr><th>Age</th><th>GUID</th><th>IP and Port</th></tr>\n");
			for (tNetSessionMgr::tIterator it(net->smgr); it.next();) {
				if (it->isTerminated()) continue; // Don't print servers which we are currently disconnecting from
				if (!it->data) {
					fprintf(f, "<tr><td colspan=\"2\" style=\"color:red\">Unknown (not a game or lobby server)</td><td>%s:%d</td><tr>\n",
						alcGetStrIp(it->getIp()).c_str(), ntohs(it->getPort()));
					continue;
				}
				fprintf(f, "<tr><td>%s</td><td>%s</td><td>%s:%d</td><tr>\n", it->name.c_str(), alcGetStrGuid(it->serverGuid).c_str(), alcGetStrIp(it->getIp()).c_str(), ntohs(it->getPort()));
			}
			fprintf(f, "</table><br />\n");
		}
		else {
			// player list (normal)
			fprintf(f, "<table border=\"1\"><tr><th>Avatar</th><th>KI</th><th>Age Name</th></tr>\n");
			for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
				if (it->flag != 2) continue;
				fprintf(f, "<tr><td>%s</td><td>%d</td><td>%s</td></tr>", it->avatar.c_str(), it->ki, it->u->name.c_str());
			}
			fprintf(f, "</table><br />\n");
		}
		// footer
		char tmptime[26];
		time_t stamp = time(NULL);
		struct tm *tptr = gmtime(&stamp);
		strftime(tmptime, 25, "%d.%m.%Y, %H:%M:%S", tptr);
		fprintf(f, "<br />Last Update: %s<br />\n", tmptime);
		fprintf(f, "</body></html>\n");
		fclose(f);
	}
	
	void tTrackingBackend::printStatusXML(void)
	{
		bool needFake = false;
		FILE *f = fopen(statusXMLFile.c_str(), "w");
		if (!f) {
			alcGetMain()->err()->log("Can\'t open %s for writing - disabling XML status page\n", statusXMLFile.c_str());
			statusXML = false;
			return;
		}
		
		fprintf(f, "<?xml version='1.0' encoding='iso-8859-1'?>\n");
		fprintf(f, "<SystemView>\n");
			fprintf(f, "<Version>2.0</Version>\n");
			fprintf(f, "<Lookup>\n");
				fprintf(f, "<Server>\n");
					fprintf(f, "<ServerInfo>\n");
						fprintf(f, "<Name>Tracking</Name>\n");
						fprintf(f, "<Type>7</Type>\n");
						fprintf(f, "<Addr>%s</Addr>\n", net->getBindAddress().c_str());
						fprintf(f, "<Port>%i</Port>\n", net->getBindPort());
						fprintf(f, "<Guid>0000000000000000</Guid>\n");
					fprintf(f, "</ServerInfo>\n");
				fprintf(f, "</Server>\n");
				fprintf(f, "<Agents>\n");
				for (tNetSessionMgr::tIterator it(net->smgr); it.next();) {
					tTrackingData *data = dynamic_cast<tTrackingData *>(it->data);
					if (!data) continue;
					if (!data->isLobby && !*data->parent) needFake = true;
					else if (data->isLobby) printLobbyXML(f, *it, data);
				}
				if (needFake) printLobbyXML(f, NULL, NULL);
				fprintf(f, "</Agents>\n");
			fprintf(f, "</Lookup>\n");
		fprintf(f, "</SystemView>\n");
		fclose(f);
	}
	
	void tTrackingBackend::printLobbyXML(FILE *f, tNetSession *lobby, tTrackingData *data)
	{	// when lobby or data are NULL, we're printing the fake lobby
		fprintf(f, "<Agent>\n");
			// Agent
			fprintf(f, "<ServerInfo>\n");
				fprintf(f, "<Name>Agent</Name>\n");
				fprintf(f, "<Type>1</Type>\n");
				if (lobby && data) {
					fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(lobby->getIp()).c_str());
					fprintf(f, "<Port>%d</Port>\n", ntohs(lobby->getPort()));
					data->agentGuid[7] = 0x00; // set last byte to 00 (to distinguish from Lobby)
					fprintf(f, "<Guid>%s00</Guid>\n", alcGetStrGuid(data->agentGuid).c_str());
				}
				else {
					fprintf(f, "<Addr>Fake Agent</Addr>\n");
					fprintf(f, "<Port>0</Port>\n");
					fprintf(f, "<Guid>%s00</Guid>\n", alcGetStrGuid(fakeLobbyGuid).c_str());
				}
			fprintf(f, "</ServerInfo>\n");
			// Lobby
			if (lobby && data) {
				fprintf(f, "<Lobby><Process>\n");
					fprintf(f, "<Server>\n");
						fprintf(f, "<ServerInfo>\n");
							fprintf(f, "<Name>Lobby</Name>\n");
							fprintf(f, "<Type>2</Type>\n");
							fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(lobby->getIp()).c_str());
							fprintf(f, "<Port>%d</Port>\n", ntohs(lobby->getPort()));
							data->agentGuid[7] = 0x02; // set last byte to 02 (to distinguish from Agent)
							fprintf(f, "<Guid>%s02</Guid>\n", alcGetStrGuid(data->agentGuid).c_str());
						fprintf(f, "</ServerInfo>\n");
					fprintf(f, "</Server>\n");
					printPlayersXML(f, lobby);
				fprintf(f, "</Process></Lobby>\n");
			}
			// Game Servers
			fprintf(f, "<Games>\n");
				if (lobby && data) { // the lobby's children
					for (tTrackingData::tSessionList::iterator it = data->children.begin(); it != data->children.end(); ++it) {
						tTrackingData *subData = dynamic_cast<tTrackingData *>((*it)->data);
						if (!subData) continue;
						printGameXML(f, **it, subData);
					}
				}
				else { // all game server without lobby
					for (tNetSessionMgr::tIterator it(net->smgr); it.next();) {
						tTrackingData *subData = dynamic_cast<tTrackingData *>(it->data);
						if (!subData) continue;
						if (!subData->isLobby && !*subData->parent) printGameXML(f, *it, subData);
					}
				}
			fprintf(f, "</Games>\n");
		fprintf(f, "</Agent>\n");
	}
	
	void tTrackingBackend::printPlayersXML(FILE *f, tNetSession *server)
	{
		fprintf(f, "<Players>\n");
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (*it->u != server || it->status != RActive) continue;
			printPlayerXML(f, &*it);
		}
		fprintf(f, "</Players>\n");
		fprintf(f, "<InRoutePlayers>\n");
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (*it->u != server || it->status == RActive) continue;
			printPlayerXML(f, &*it);
		}
		fprintf(f, "</InRoutePlayers>\n");
	}
	
	void tTrackingBackend::printPlayerXML(FILE *f, tPlayer *player)
	{
		fprintf(f, "<Player>\n");
			fprintf(f, "<AcctName>%s</AcctName>\n", player->account.c_str());
			fprintf(f, "<PlayerID>%i</PlayerID>\n", player->ki);
			fprintf(f, "<PlayerName>%s%s</PlayerName>\n", player->avatar.c_str(), player->flag == 2 ? "" : " [hidden]");
			fprintf(f, "<AccountUUID>%s</AccountUUID>\n", alcGetStrUid(player->uid).c_str());
			if (!player->awaiting_age.isEmpty())
				// if the age he wants to is saved, print it
				fprintf(f, "<State>%s to %s</State>\n", alcUnetGetReasonCode(player->status), player->awaiting_age.c_str());
			else fprintf(f, "<State>%s</State>\n", alcUnetGetReasonCode(player->status));
		fprintf(f, "</Player>\n");
	}
	
	void tTrackingBackend::printGameXML(FILE *f, tNetSession *game, tTrackingData *data)
	{
		if (!data) return;
		fprintf(f, "<Game>\n");
			fprintf(f, "<Process>\n");
				fprintf(f, "<Server>\n");
					fprintf(f, "<ServerInfo>\n");
						fprintf(f, "<Name>%s</Name>\n", game->name.c_str());
						fprintf(f, "<Type>3</Type>\n");
						fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(game->getIp()).c_str());
						fprintf(f, "<Port>%i</Port>\n", ntohs(game->getPort()));
						fprintf(f, "<Guid>%s</Guid>\n", alcGetStrGuid(game->serverGuid).c_str());
					fprintf(f, "</ServerInfo>\n");
				fprintf(f, "</Server>\n");
				printPlayersXML(f, game);
			fprintf(f, "</Process>\n");
			fprintf(f, "<AgeLink>\n");
				fprintf(f, "<AgeInfo>\n");
					fprintf(f, "<AgeInstanceName>%s</AgeInstanceName>\n", game->name.c_str());
					fprintf(f, "<AgeInstanceGuid>%s</AgeInstanceGuid>\n", alcGetStrGuid(game->serverGuid).c_str());
				fprintf(f, "</AgeInfo>\n");
			fprintf(f, "</AgeLink>\n");
		fprintf(f, "</Game>\n");
	}

} //end namespace alc

