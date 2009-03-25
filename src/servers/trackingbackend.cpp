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
#include <alcnet.h>

////extra includes
#include <set>
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
		portStart = portEnd = 0;
		seqPrefix = 0;
		childs = new tNetSessionList;
	}
	
	//// tPlayer
	tPlayer::tPlayer(U32 ki)
	{
		this->ki = ki;
		this->sid = 0;
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
	tTrackingBackend::tTrackingBackend(tUnetBase *net, tNetSessionList *servers, char *host, U16 port)
	{
		log = lnull;
		guidGen = NULL;
		this->servers = servers;
		this->net = net;
		this->host = host;
		this->port = port;
		lastUpdate = 0;
		generateFakeGuid(fakeLobbyGuid);
		load();
	}
	
	tTrackingBackend::~tTrackingBackend(void)
	{
		unload();
		if (players.size())
			lerr->log("ERR: The backend is quitting, and there were still %d players online\n", players.size());
	}
	
	void tTrackingBackend::unload(void)
	{
		if (log != lnull) {
			delete log;
			log = lnull;
		}
		if (guidGen != NULL) {
			delete guidGen;
			guidGen = NULL;
		}
	}
	
	void tTrackingBackend::load(void)
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
		if (var.isNull()) strcpy(resettingAges, "Cleft,DniCityX2Finale,GreatZero,Kveer,Myst,Neighborhood02,Personal02,RestorationGuild,spyroom,Garden"); // see uru.conf.dist for explanation
		else strncpy(resettingAges, var.c_str(), 1023);
		
		var = cfg->getVar("track.html");
		statusHTML = (!var.isNull() && var.asByte());
		var = cfg->getVar("track.html.path");
		if (var.isNull()) statusHTML = false;
		else strncpy(statusHTMLFile, var.c_str(), 255);
		
		var = cfg->getVar("track.htmldbg");
		statusHTMLdbg = (!var.isNull() && var.asByte());
		var = cfg->getVar("track.htmldbg.path");
		if (var.isNull()) statusHTMLdbg = false;
		else strncpy(statusHTMLdbgFile, var.c_str(), 255);
		
		var = cfg->getVar("track.xml");
		statusXML = (!var.isNull() && var.asByte());
		var = cfg->getVar("track.xml.path");
		if (var.isNull()) statusXML = false;
		else strncpy(statusXMLFile, var.c_str(), 255);
		statusFileUpdate = true;
		
		if (guidGen == NULL)
			guidGen = new tGuidGen();
	}
	
	void tTrackingBackend::reload(void)
	{
		unload();
		load();
	}
	
	void tTrackingBackend::findServer(tmCustomFindServer &findServer)
	{
		statusFileUpdate = true;
		
		tPlayerList::iterator player = getPlayer(findServer.ki);
		if (player == players.end()) {
			log->log("ERR: Ignoring a NetMsgCustomFindServer for player with KI %d since I can't find that player\n", findServer.ki);
			return;
		}
		player->sid = findServer.sid;
		player->awaiting_x = findServer.x;
		player->ip = findServer.ip;
		player->port = findServer.port;
		log->log("Player %s[%s:%d] wants to link to %s (%s)\n", player->str(), alcGetStrIp(player->ip), player->port, findServer.age.c_str(), findServer.serverGuid.c_str());
		if (strcmp(findServer.serverGuid.c_str(), "0000000000000000") == 0) { // these are 16 zeroes
			if (!guidGen->generateGuid(player->awaiting_guid, findServer.age.c_str(), player->ki)) {
				log->log("ERR: Request to link to unknown age %s - kicking player %s\n", findServer.age.c_str(), player->str());
				tmPlayerTerminated term(player->u, player->ki, RKickedOff);
				net->send(term);
				return;
			}
			log->log(" Generated GUID: %s\n", alcGetStrGuid(player->awaiting_guid));
		}
		else
			alcAscii2Hex(player->awaiting_guid, findServer.serverGuid.c_str(), 8);
		// copy data to player
		player->status = RInRoute;
		strncpy(player->awaiting_age, findServer.age.c_str(), 199);
		player->waiting = true;
		// search for the game server the player needs
		tNetSession *server = NULL, *game = NULL;
		servers->rewind();
		while ((server = servers->getNext())) {
			if (server->data && memcmp(server->serverGuid, player->awaiting_guid, 8) == 0 && strncmp(server->name, player->awaiting_age, 199) == 0) {
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
				tTrackingData *data = dynamic_cast<tTrackingData*>(server->data);
				if (data && data->isLobby && (load < 0 || data->childs->getCount() < load)) {
					lobby = server;
					load = data->childs->getCount();
				}
			}
			if (!lobby) {
				log->log("ERR: There's no lobby I could use to spawn the server\n");
				return;
			}
			// search for free ports
			tTrackingData *data = static_cast<tTrackingData*>(lobby->data); // we already checked the type above
			int nPorts = data->portEnd - data->portStart + 1;
			bool *freePorts = (bool *)malloc(nPorts*sizeof(bool));
			if (freePorts == NULL) throw txNoMem(_WHERE("NoMem"));
			for (int i = 0; i < nPorts; ++i) freePorts[i] = true;
			data->childs->rewind();
			while ((server = data->childs->getNext()))
				freePorts[ntohs(server->getPort()) - data->portStart] = false; // this port is occupied
			int lowest;
			for (lowest = 0; lowest < nPorts; ++lowest) {
				if (freePorts[lowest]) break; // we found a free one
			}
			free(freePorts);
			if (lowest == nPorts) { // no free port on the lobby with the least childs
				log->log("ERR: No free port on lobby %s, can't spawn game server\n", lobby->str());
				return;
			}
			lowest += data->portStart;
			// ok, telling the lobby to fork
			bool loadState = doesAgeLoadState(player->awaiting_age);
			tmCustomForkServer forkServer(lobby, lowest, alcGetStrGuid(player->awaiting_guid), player->awaiting_age, loadState);
			net->send(forkServer);
			log->log("Spawning new game server %s (Server GUID: %s, port: %d) on %s ", player->awaiting_age, alcGetStrGuid(player->awaiting_guid), lowest, lobby->str());
			if (loadState) log->print("(loading age state)\n");
			else log->print("(not loading age state)\n");
		}
		else {
			// ok, we got it, let's tell the player about it
			serverFound(&*player, game);
		}
		log->flush();
	}
	
	void tTrackingBackend::notifyWaiting(tNetSession *server)
	{
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (!it->waiting) continue;
			if (memcmp(it->awaiting_guid, server->serverGuid, 8) != 0 || strncmp(it->awaiting_age, server->name, 199)) continue;
			// ok, this player is waiting for this age, let's tell him about it
			serverFound(&*it, server);
		}
	}
	
	void tTrackingBackend::serverFound(tPlayer *player, tNetSession *server)
	{
		tTrackingData *data = dynamic_cast<tTrackingData *>(server->data);
		if (!data) throw txUnet(_WHERE("server passed in tTrackingBackend::serverFound is not a game/lobby server"));
		// notify the server that a player will come
		tmCustomPlayerToCome playerToCome(server);
		net->send(playerToCome);
		// notifiy the player that it's server is available
		tmCustomServerFound found(player->u, player->ki, player->awaiting_x, player->sid, ntohs(server->getPort()), data->externalIp, alcGetStrGuid(server->serverGuid), server->name);
		net->send(found);
		log->log("Found age for player %s\n", player->str());
		// no longer waiting
		player->waiting = false;
	}
	
	tTrackingBackend::tPlayerList::iterator tTrackingBackend::getPlayer(U32 ki)
	{
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (it->ki == ki)
				return it;
		}
		return players.end();
	}
	
	void tTrackingBackend::updateServer(tNetSession *game, tmCustomSetGuid &setGuid)
	{
		statusFileUpdate = true;
		Byte serverGuid[8];
		alcAscii2Hex(serverGuid, setGuid.serverGuid.c_str(), 8);
		// search if another game server for that guid is already running. in that case, ignore this one
		tNetSession *server;
		while ((server = servers->getNext())) {
			if (server == game || !server->data) continue;
			if (memcmp(server->serverGuid, serverGuid, 8) == 0) {
				log->log("ERR: There already is a server for guid %s, kicking the new one %s\n", setGuid.serverGuid.c_str(), game->str());
				net->terminate(game); // this should usually result in the game server going down
				log->flush();
				return;
			}
		}
		
		memcpy(game->serverGuid, serverGuid, 8);
		strncpy(game->name, setGuid.age.c_str(), 199);
		
		
		if (game->data) return; // ignore the rest of the info if we already got it. IP and Port can't change.
		tTrackingData *data = new tTrackingData;
		if (setGuid.validSpawnPorts()) {
			data->isLobby = true;
			data->portStart = setGuid.spawnStart;
			data->portEnd = setGuid.spawnStop;
		}
		else {
			data->isLobby = false;
#ifdef ENABLE_UNET3
			if ( (game->proto == 1 || game->proto == 2) && ntohs(game->getPort()) == 5000 ) {
				data->isLobby = true;
				data->portStart = 5001;
				data->portEnd = 5002;
			}
#endif
		}
		strncpy(data->externalIp, setGuid.externalIp.c_str(), 99);
		if (!data->isLobby) { // let's look to which lobby this server belongs
			tNetSession *lobby = NULL;
			server = NULL;
			servers->rewind();
			while ((server = servers->getNext())) {
				tTrackingData *data = dynamic_cast<tTrackingData *>(server->data);
				if (data && data->isLobby && server->getIp() == game->getIp()) {
					lobby = server;
					break;
				}
			}
			if (lobby) { // we found the server's lobby
				static_cast<tTrackingData *>(server->data)->childs->add(game); // add the game server to the list of children of that lobby
				data->parent = lobby;
			}
			else
				log->log("ERR: Found game server %s without a Lobby belonging to it\n", game->str());
			
			// get the age's sequence prefix
			tAgeInfo *age = guidGen->getAge(game->name);
			if (!age) {
				log->log("ERR: Can\'t find the age file (%s) for game server %s - kicking it\n", game->name, game->str());
				net->terminate(game);
			}
			else data->seqPrefix = age->seqPrefix;
		}
		else // if it is a lobby
			generateFakeGuid(data->agentGuid); // create guid for UruVision
		game->data = data; // save the data
		log->log("Found server at %s\n", game->str());
		
		notifyWaiting(game);
		log->flush();
	}
	
	void tTrackingBackend::updatePlayer(tNetSession *game, tmCustomPlayerStatus &playerStatus)
	{
		if (!game->data)
			// throwing this error will terminate the connection to this server, which in term should result in the server going down
			//  (game servers should quit when they loose the connection to the tracking server)
			throw txProtocolError(_WHERE("server passed in tTrackingBackend::updatePlayer is not a game/lobby server"));
		statusFileUpdate = true;
		/* Flags:
		0: delete
		1: set invisible
		2: set visible
		3: set only buddies (unused!) */
		tPlayerList::iterator player = getPlayer(playerStatus.ki);
		if (playerStatus.playerFlag == 0) {
			if (player != players.end()) {
				log->log("Player %s quit\n", player->str());
				players.erase(player);
				statusFileUpdate = true;
			}
		}
		else if (playerStatus.playerFlag == 1 || playerStatus.playerFlag == 2) {
			if (player == players.end()) { // it doesn't exist, create it
				player = players.insert(players.end(), tPlayer(playerStatus.ki));
			}
			else { // if it already exists, check if the avi is already logged in elsewhere
				// to do so, we first check if the game server the player uses changed. if that's the case, and the player did not request to link, kick the old player
				if (player->u != game && player->status != RLeaving) {
					log->log("WARN: Kicking player %s at %s as it just logged in at %s\n", player->str(), player->u->str(), game->str());
					tmPlayerTerminated term(player->u, player->ki, RLoggedInElsewhere);
					net->send(term);
				}
			}
			// update the player's data
			player->sid = playerStatus.sid;
			player->u = game;
			player->flag = playerStatus.playerFlag;
			player->status = playerStatus.playerStatus;
			strcpy(player->avatar, playerStatus.avatar.c_str());
			strcpy(player->account, playerStatus.account.c_str());
			memcpy(player->uid, playerStatus.uid, 16);
			// no longer waiting
			player->waiting = false;
			if (player->status != RLeaving) {
				// when he's leaving to another age, keep this info (to be shown on status page)
				player->awaiting_age[0] = 0;
				memset(player->awaiting_guid, 0, 8);
				player->awaiting_x = 0;
			}
			log->log("Got status update for player %s: 0x%02X (%s)\n", player->str(), playerStatus.playerStatus,
					alcUnetGetReasonCode(playerStatus.playerStatus));
			statusFileUpdate = true;
		}
		else {
			log->log("ERR: Got unknown flag 0x%02X for player with KI %d\n", playerStatus.playerFlag, playerStatus.ki);
		}
		log->flush();
	}
	
	void tTrackingBackend::removeServer(tNetSession *game)
	{
		tTrackingData *data = dynamic_cast<tTrackingData *>(game->data);
		if (!data) return;
		statusFileUpdate = true;
		// remove all players which were still on this server
		tPlayerList::iterator it = players.begin();
		while (it != players.end()) {
			if (it->u == game) {
				log->log("WARN: Removing player %s as it was on a terminating server\n", it->str());
				it = players.erase(it);
				statusFileUpdate = true;
			}
			else
				++it; // we have to increment manually because above if block already increments
		}
		log->log("Server %s is leaving us\n", game->str());
		log->flush();
		// remove this server from the list of childs of its lobby/from the game server it is the lobby for
		if (data->isLobby) {
			// it's childs are lobbyless now
			tNetSession *server;
			data->childs->rewind();
			while ((server = data->childs->getNext())) {
				tTrackingData *subData = dynamic_cast<tTrackingData *>(server->data);
				if (!subData) throw txUnet(_WHERE("One child of the lobby I'm just deleting is not a game/lobby server"));
				subData->parent = NULL;
			}
		}
		else if (data->parent) {
			tTrackingData *parentData = dynamic_cast<tTrackingData *>(data->parent->data);
			if (!parentData) throw txUnet(_WHERE("The parent of the game server I'm just deleting is not a game/lobby server"));
			parentData->childs->remove(game);
		}
	}
	
	bool tTrackingBackend::doesAgeLoadState(const char *age)
	{
		if (!loadAgeState) return false;
		
		// local copy of resetting age list as strsep modifies it
		char ages[1024];
		strcpy(ages, resettingAges);
		
		char *buf = ages;
		char *p = strsep(&buf, ",");
		while (p != 0) {
			if (strcmp(p, age) == 0) return false;
			p = strsep(&buf, ",");
		}
		return true;
	}
	
	void tTrackingBackend::forwardMessage(tmCustomDirectedFwd &directedFwd)
	{
		// for each player, check if we can reach it, and save which game server we need to send the message to
		typedef std::set<tNetSession *> tSessionList;
		tTrackingData *data;
		tSessionList receivers; // to save where we already sent it
		tmCustomDirectedFwd::tRecList::iterator it = directedFwd.recipients.begin();
		while (it != directedFwd.recipients.end()) {
			bool removed = false;
			for (tPlayerList::iterator jt = players.begin(); jt != players.end(); ++jt) {
				if (*it != jt->ki) continue; // next one
				// ok, this player should get this message
				if (jt->u == directedFwd.getSession()) break; // don't send it back to where it comes from
				if (jt->status == RLeaving) break; // the player is not connected
				data = dynamic_cast<tTrackingData *>(jt->u->data);
				if (!data || data->isLobby) break; // don't send messages to the lobby
				// ok, let's go - check if we already sent the message there
				if (!receivers.count(jt->u)) {
					// no, we didn't - do it! and save that
					tmCustomDirectedFwd forwardedMsg(jt->u, directedFwd);
					net->send(forwardedMsg);
					receivers.insert(jt->u);
				}
				removed = true;
				it = directedFwd.recipients.erase(it); // since we already sent the message for this player we can remove it from our list
			}
			if (!removed) ++it; // go to next one if we did not yet do that
		}
		
		// check if we got all recipients - if not, tell the sender
		if (directedFwd.recipients.size()) {
			// format the text
			tStrBuf text("These players did not get the message (linking?): ");
			bool comma = false;
			for (it = directedFwd.recipients.begin(); it != directedFwd.recipients.end(); ++it) {
				if (comma) text.writeStr(", ");
				text.printf("%d", *it);
				comma = true;
			}
			// create the message
			tpKIMsg kiMsg(tUruObjectRef(), tStrBuf("Tracking Server"), 0, text);
			kiMsg.flags = 0x00004248;
			kiMsg.messageType = 0x0009; // private inter age chat
			// send message
			tmCustomDirectedFwd msg(directedFwd.getSession(), 0, &kiMsg);
			msg.recipients.push_back(directedFwd.ki);
			net->send(msg);
		}
	}
	
	void tTrackingBackend::generateFakeGuid(Byte *guid)
	{
		tMBuf buf;
		buf.putU16(0xFFFF);
		buf.putU32(random());
		buf.putByte(alcGetMicroseconds());
		buf.putByte(0x00);
		buf.rewind();
		memcpy(guid, buf.read(8), 8);
	}
	
	void tTrackingBackend::updateStatusFile(void)
	{
		if (!statusFileUpdate && lastUpdate > alcGetTime()-5*60) return; // update at least every 5 minutes
		
		if (statusHTML) printStatusHTML();
		if (statusHTMLdbg) printStatusHTML(true);
		if (statusXML) printStatusXML();
		statusFileUpdate = false;
		lastUpdate = alcGetTime();
	}
	
	void tTrackingBackend::printStatusHTML(bool dbg)
	{
		char *statusfile = dbg ? statusHTMLdbgFile : statusHTMLFile;
		char *filename = strrchr(statusfile, '/'); // get only the filename (for automatic reload)
		if (filename) ++filename; // it points to the slash, but we want to start after it
		else filename = statusfile; // no slash... it must be a relative filename then, containing no directory, so let's use that
		
		FILE *f = fopen(statusfile, "w");
		if (!f) {
			lerr->log("Can\'t open %s for writing - disabling HTML status page\n", statusfile);
			if (dbg) statusHTMLdbg = false;
			else statusHTML = false;
			return;
		}
		
		// header
		fprintf(f, "<html><head><title>Shard Status</title>\n");
		fprintf(f, "<meta http-equiv=\"refresh\" content=\"30;url=%s\" />\n", filename);
		fprintf(f, "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=ISO-8859-1\">");
		fprintf(f, "</head><body>\n");
		// player list
		fprintf(f, "<h2>Current Online Players</h2>\n");
		if (dbg)
			fprintf(f, "<b>Total population: %d</b><br /><br />\n", players.size());
		fprintf(f, "<table border=\"1\"><tr><th>Avatar (Account)</th><th>KI</th><th>Age Name</th><th>Age GUID</th><th>Status</th></tr>\n");
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (!dbg && it->flag != 2) continue;
			fprintf(f, "<tr><td>%s (%s)%s</td><td>%d</td><td>%s</td><td>%s</td>", it->avatar, it->account, it->flag == 2 ? "" : " [hidden]", it->ki, it->u->name, alcGetStrGuid(it->u->serverGuid));
			if (it->awaiting_age[0] != 0) // if the age he wants to is saved, print it
				fprintf(f, "<td>%s to %s</td>", alcUnetGetReasonCode(it->status), it->awaiting_age);
			else fprintf(f, "<td>%s</td>", alcUnetGetReasonCode(it->status));
			fprintf(f, "</tr>\n");
		}
		fprintf(f, "</table><br />\n");
		// server list
		if (dbg) {
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
				fprintf(f, "<tr><td>%s</td><td>%s</td><td>%s:%d</td><tr>\n", server->name, alcGetStrGuid(server->serverGuid), alcGetStrIp(server->getIp()), ntohs(server->getPort()));
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
		tNetSession *server;
		bool needFake = false;
		FILE *f = fopen(statusXMLFile, "w");
		if (!f) {
		  lerr->log("Can\'t open %s for writing - disabling XML status page\n", statusXMLFile);
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
						fprintf(f, "<Addr>%s</Addr>\n", host);
						fprintf(f, "<Port>%i</Port>\n", port);
						fprintf(f, "<Guid>0000000000000000</Guid>\n");
					fprintf(f, "</ServerInfo>\n");
				fprintf(f, "</Server>\n");
				fprintf(f, "<Agents>\n");
				servers->rewind();
				while ((server = servers->getNext())) {
					tTrackingData *data = dynamic_cast<tTrackingData *>(server->data);
					if (!data) continue;
					if (!data->isLobby && !data->parent) needFake = true;
					else if (data->isLobby) printLobbyXML(f, server, data);
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
					fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(lobby->getIp()));
					fprintf(f, "<Port>%d</Port>\n", ntohs(lobby->getPort()));
					data->agentGuid[7] = 0x00; // set last byte to 00 (to distinguish from Lobby)
					fprintf(f, "<Guid>%s00</Guid>\n", alcGetStrGuid(data->agentGuid));
				}
				else {
					fprintf(f, "<Addr>Fake Agent</Addr>\n");
					fprintf(f, "<Port>0</Port>\n");
					fprintf(f, "<Guid>%s00</Guid>\n", alcGetStrGuid(fakeLobbyGuid));
				}
			fprintf(f, "</ServerInfo>\n");
#if 0
			// not used by UruVision
			if (lobby) fprintf(f, "<ExternalAddr>%s</ExternalAddr>\n", data->externalIp);
			else       fprintf(f, "<ExternalAddr>Fake Agent</ExternalAddr>\n");
			fprintf(f, "<PlayerLimit>-1</PlayerLimit>\n");
			fprintf(f, "<GameLimit>-1</GameLimit>\n");
#endif
			// Lobby
			if (lobby && data) {
				fprintf(f, "<Lobby><Process>\n");
					fprintf(f, "<Server>\n");
						fprintf(f, "<ServerInfo>\n");
							fprintf(f, "<Name>Lobby</Name>\n");
							fprintf(f, "<Type>2</Type>\n");
							fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(lobby->getIp()));
							fprintf(f, "<Port>%d</Port>\n", ntohs(lobby->getPort()));
							data->agentGuid[7] = 0x02; // set last byte to 02 (to distinguish from Agent)
							fprintf(f, "<Guid>%s02</Guid>\n", alcGetStrGuid(data->agentGuid));
						fprintf(f, "</ServerInfo>\n");
					fprintf(f, "</Server>\n");
					printPlayersXML(f, lobby);
				fprintf(f, "</Process></Lobby>\n");
			}
			// Game Servers
			fprintf(f, "<Games>\n");
				tNetSession *server;
				if (lobby && data) { // the lobby's children
					data->childs->rewind();
					while ((server = data->childs->getNext())) {
						tTrackingData *subData = dynamic_cast<tTrackingData *>(server->data);
						if (!subData) continue;
						printGameXML(f, server, subData);
					}
				}
				else { // all game server without lobby
					servers->rewind();
					while ((server = servers->getNext())) {
						tTrackingData *subData = dynamic_cast<tTrackingData *>(server->data);
						if (!subData) continue;
						if (!subData->isLobby && !subData->parent) printGameXML(f, server, subData);
					}
				}
			fprintf(f, "</Games>\n");
		fprintf(f, "</Agent>\n");
	}
	
	void tTrackingBackend::printPlayersXML(FILE *f, tNetSession *server)
	{
		fprintf(f, "<Players>\n");
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (it->u != server || it->status != RActive) continue;
			printPlayerXML(f, &*it);
		}
		fprintf(f, "</Players>\n");
		fprintf(f, "<InRoutePlayers>\n");
		for (tPlayerList::iterator it = players.begin(); it != players.end(); ++it) {
			if (it->u != server || it->status == RActive) continue;
			printPlayerXML(f, &*it);
		}
		fprintf(f, "</InRoutePlayers>\n");
	}
	
	void tTrackingBackend::printPlayerXML(FILE *f, tPlayer *player)
	{
		fprintf(f, "<Player>\n");
			fprintf(f, "<AcctName>%s</AcctName>\n", player->account);
			fprintf(f, "<PlayerID>%i</PlayerID>\n", player->ki);
			fprintf(f, "<PlayerName>%s%s</PlayerName>\n", player->avatar, player->flag == 2 ? "" : " [hidden]");
			fprintf(f, "<AccountUUID>%s</AccountUUID>\n", alcGetStrUid(player->uid));
			if (player->awaiting_age[0] != 0)
				// if the age he wants to is saved, print it
				fprintf(f, "<State>%s to %s</State>\n", alcUnetGetReasonCode(player->status), player->awaiting_age);
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
						fprintf(f, "<Name>%s</Name>\n", game->name);
						fprintf(f, "<Type>3</Type>\n");
						fprintf(f, "<Addr>%s</Addr>\n", alcGetStrIp(game->getIp()));
						fprintf(f, "<Port>%i</Port>\n", ntohs(game->getPort()));
						fprintf(f, "<Guid>%s</Guid>\n", alcGetStrGuid(game->serverGuid));
					fprintf(f, "</ServerInfo>\n");
				fprintf(f, "</Server>\n");
				printPlayersXML(f, game);
			fprintf(f, "</Process>\n");
			fprintf(f, "<AgeLink>\n");
				fprintf(f, "<AgeInfo>\n");
					fprintf(f, "<AgeInstanceName>%s</AgeInstanceName>\n", game->name);
					fprintf(f, "<AgeInstanceGuid>%s</AgeInstanceGuid>\n", alcGetStrGuid(game->serverGuid));
					fprintf(f, "<AgeSequenceNumber>%d</AgeSequenceNumber>\n", data->seqPrefix);
				fprintf(f, "</AgeInfo>\n");
			fprintf(f, "</AgeLink>\n");
		fprintf(f, "</Game>\n");
	}

} //end namespace alc

