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
#define __U_GAMESERVER_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "gameserver.h"

#include "sdl.h"
#include <netexception.h>
#include <protocol/gamemsg.h>
#include <protocol/trackingmsg.h>
#include <protocol/vaultmsg.h>
#include <protocol/vaultproto.h>
#include <urutypes/ageinfo.h>
#include <urutypes/plmessage.h>
#include <alcmain.h>

#include <cstring>

namespace alc {
	
	const char *alcNetName = "Game";
	tUnetServerBase *alcServerInstance(void) { return new tUnetGameServer(); }

	tUnetGameServer::tUnetGameServer(void) : tUnetLobbyServerBase(KGame)
	{
		// find out which age we are supposed to host
		tConfig *cfg = alcGetMain()->config();
		serverName = cfg->getVar("age_filename");
		if (serverName.size() < 2) throw txBase(_WHERE("an age name must be set"));
		tString var = cfg->getVar("age_guid");
		if (var.size() != 16) throw txBase(_WHERE("an age GUID must be set"));
		tMBuf guid = alcAscii2Hex(var);
		if (guid.size() != 8) throw txBase(_WHERE("invalid age GUID set"));
		memcpy(serverGuid, guid.data(), 8);
		
		// load age file and SDL Manager
		resetStateWhenEmpty = false;
		ageInfo = new tAgeInfo(cfg->getVar("age"), serverName, /*loadPages*/true);
		ageState = new tAgeStateManager(this, ageInfo);
		
		// make sure we quit if noone comes
		lastPlayerLeft = time(NULL);
	}
	
	bool tUnetGameServer::canPortBeUsed(uint16_t port) {
		return (port >= spawnStart && port <= spawnStop);
	}

	tUnetGameServer::~tUnetGameServer(void)
	{
		delete ageInfo;
		delete ageState;
	}
	
	void tUnetGameServer::onApplyConfig(void)
	{
		tUnetLobbyServerBase::onApplyConfig();
		if (allowedGames == tNetSession::UnknownGame) { // a game server can not deal with both clients at the same time
			log->log("WARN: According to your configuration, both UU and TPOTS clients are allowed, but the game server does not support that. Setting to TPOTS only.\n");
			allowedGames = tNetSession::POTSGame;
		}
		
		tConfig *cfg = alcGetMain()->config();
		tString var = cfg->getVar("game.persistent");
		if (!var.isEmpty() && var.asInt()) { // disabled per default
			lingerTime = 0;
		}
		else {
			var = cfg->getVar("game.linger_time");
			if (!var.isEmpty())
				lingerTime = var.asInt();
			else
				lingerTime = 120; // default
			if (lingerTime && lingerTime < 20)
				log->log("WARN: You set a linger time of only %d, which could result in the game server going down before the player even has the chance to join it - I recommend to set it to at least 20\n", lingerTime);
		}
		
		var = cfg->getVar("game.tmp.hacks.noreltoshare");
		noReltoShare = (!var.isEmpty() && var.asInt()); // disabled per default
		
		var = cfg->getVar("game.tmp.hacks.linkidle");
		linkingOutIdle = (var.isEmpty() || var.asInt()); // enabled per default
		
		var = cfg->getVar("game.serversidecommands");
		serverSideCommands = (var.isEmpty() || var.asInt()); // enabled per default
		
		shardIdentifier = cfg->getVar("shard.identifier"); // default: empty
		
		ageState->applyConfig();
	}
	
	void tUnetGameServer::onVaultMessageForward(tNetSession *u, tvMessage *msg)
	{
		// FIXME: This whole function is a bad hack - eventually, everything done here should be properly implemented elsewhere
		// first of all, we are only interested in VSaveNodes sent from client to vault
		if (u == *vaultServer) return; // we care only about messages from the client
		if (msg->task) { // it is a vault task
			if (msg->cmd != TRegisterOwnedAge) return; // we are only interested in these messages which are sent when an age is reset
			// now, find the age link struct
			tAgeLinkStruct *ageLink = NULL;
			for (tvMessage::tItemList::iterator it = msg->items.begin(); it != msg->items.end(); ++it) {
				if ((*it)->id != 11) continue;
				ageLink = (*it)->asAgeLink(); // we don't have to free it, tvMessage does that
				break; // got it
			}
			if (!ageLink)
				throw txProtocolError(_WHERE("A TRegisterOwnedAge without an AgeLinkStruct attached???"));
			// make sure we are actually talking about this age
			if (ageLink->ageInfo.filename != serverName || memcmp(serverGuid, ageLink->ageInfo.guid, 8) != 0)
				return; // this is another age!
			// ok, the player definitely wants to reset us, check if this is a public age
			if (ageLink->ageInfo.guid[1] == 0 && ageLink->ageInfo.guid[2] == 0 && ageLink->ageInfo.guid[3] == 0 && ageLink->ageInfo.guid[4] == 0) {
				// it's public (since no KI is set in the GUID)
				return;
			}
			// check there is someone else in here
			if (!checkIfOnlyPlayer(u)) { // there is someone else here, do not reset
				sendKIMessage("Sorry, but someone else is already in this age, so I can not reset it", u);
				return;
			}
			// ok, let's go :)
			ageState->clearAllStates();
		}
		else { // it is a vault command
			if (msg->cmd != VSaveNode) return; // we are only interested in VSaveNode messages
			// ok, now find the saved node
			tvNode *node = NULL;
			for (tvMessage::tItemList::iterator it = msg->items.begin(); it != msg->items.end(); ++it) {
				if ((*it)->id != 5) continue;
				node = (*it)->asNode(); // we don't have to free it, tvMessage does that
				break; // got it
			}
			if (!node)
				throw txProtocolError(_WHERE("A VSaveNode without a node attached???"));
			
			// now let's see what to do
			if (node->type == KSDLNode) {
				/* this is a very dirty fix for the bahro poles, but as long as the game server doesn't subscribe to the vault to get it's 
				own SDL node, we have to do it this way */
				if (node->blob1.isEmpty()) return; // don't bother parsing empty messages
				// got the node, and it is a SDL one... get the SDL binary stream
				ageState->saveSdlVaultMessage(node->blob1, u); // process it
			}
			/* NOTE: These checks are not mainly security checks but are necessary to update the age list when a player changes his name or hides.
			The KVNodeMgrPlayerNode check is meant to provided consistence in rejecting player name changes.
			But since these checks are in the game server and not in the lobbybase, you can still do everything as long as you are logged
			in through lobby. */
			else if (node->type == KPlayerInfoNode) {
				if (node->owner != u->ki)
					throw txProtocolError(_WHERE("changing a foreign player info node is not allowed"));
				if (u->getAccessLevel() > AcMod)
					throw txProtocolError(_WHERE("%s is not allowed to change his player info node", u->str().c_str()));
				
				tGameData *data = dynamic_cast<tGameData *>(u->data);
				if (!u->joined || !data) throw txProtocolError(_WHERE("Player data must be set when player node is changed"));
				if (node->flagB & MAgeName) { // the hidden/shown status changed
					bool isHidden = node->ageName.size();
					if (isHidden) log->log("Player %s just hid\n", u->str().c_str());
					else log->log("Player %s just unhid\n", u->str().c_str());
					// update member list
					data->isHidden = isHidden;
					bcastMemberUpdate(u, /*isJoined*/true);
				}
				if (node->flagB & MlStr64_1) { // avatar name changed
					log->log("%s is now called %s\n", u->str().c_str(), node->lStr1.c_str());
					// update member list
					u->avatar = node->lStr1;
					bcastMemberUpdate(u, /*isJoined*/true);
				}
				// update tracking server status
				tmCustomPlayerStatus trackingStatus(*trackingServer, u, data->isHidden ? 1 /* invisible */ : 2 /* visible */, RActive);
				send(trackingStatus);
			}
			else if (node->type == KVNodeMgrPlayerNode) {
				if (node->index != u->ki)
					throw txProtocolError(_WHERE("changing a foreign player mgr node is not allowed"));
				if (u->getAccessLevel() > AcMod)
					throw txProtocolError(_WHERE("%s is not allowed to change his player mgr node", u->str().c_str()));
			}
		}
	}
	
	bool tUnetGameServer::processGameMessage(tStreamedObject *msg, tNetSession *u, tUruObjectRef *receiver)
	{
		bool processed = false;
		tpMessage *subMsg = tpMessage::createFromStream(msg, u->gameType == tNetSession::UUGame, /*mustBeComplete*/false);
		if (receiver && subMsg->receivers.size()) *receiver = subMsg->receivers.at(0);
		// check for chat messages
		tpKIMsg *kiMsg = dynamic_cast<tpKIMsg*>(subMsg);
		if (serverSideCommands && kiMsg && kiMsg->messageType == 0 && kiMsg->text.startsWith("/!")) { // if it is a command
			processKICommand(kiMsg->text, u);
			processed = true;
		}
		delete subMsg;
		return processed;
	}
	
	void tUnetGameServer::processKICommand(const tString &text, tNetSession *u)
	{
		// process server-side commands
		if (text == "/!ping") sendKIMessage("You are still online :)", u);
		else if (text == "/!silentping") sendKIMessage("/!silentpong", u);
		else if (text == "/!getauthlevel") {
			tString text;
			text.printf("/!authlevel %d", u->getAccessLevel());
			sendKIMessage(text, u);
		}
		else if (text == "/!getshardidentifier") {
			tString text;
			text.printf("/!shardidentifier %s", shardIdentifier.c_str());
			sendKIMessage(text, u);
		}
		else if (text == "/!resetage") {
			if (ageInfo->seqPrefix <= 99) { // Cyan age
				sendKIMessage("You can not reset Cyan ages this way", u);
			}
			else if (!checkIfOnlyPlayer(u)) {
				sendKIMessage("You have to be the only player in an age to reset it", u);
			}
			else {
				sendKIMessage("Re-linking you to complete age reset...", u);
				sendKIMessage("/!relink", u);
				resetStateWhenEmpty = true;
			}
		}
#ifndef NDEBUG
		else if (text == "/!quit") {
			u->terminate();
		}
#endif
		else {
			tString error;
			error.printf("Unknown server-side command: \"%s\"", text.c_str());
			sendKIMessage(error, u);
		}
	}
	
	void tUnetGameServer::sendKIMessage(const tString &text, tNetSession *u)
	{
		tpKIMsg kiMsg = tpKIMsg(tUruObjectRef(), "Game Server", 0, text);
		kiMsg.flags = 0x00004248;
		kiMsg.messageType = 0x0001; // private age chat
		// send message
		tmGameMessageDirected msg(u, 0, &kiMsg);
		msg.recipients.push_back(u->ki);
		send(msg);
	}
	
	void tUnetGameServer::onPlayerAuthed(tNetSession */*u*/)
	{
		// a new player connected, so we are no longer alone
		// we have to do this when the player authenticates because it will download the age before joining, in which time the auto-kill could already stop the server
		tSpinLock lock(autoShutdownMutex);
		lastPlayerLeft = 0;
	}
	
	void tUnetGameServer::onConnectionClosing(alc::tNetSession* u, uint8_t reason)
	{
		if (u->ki != 0) { // if necessary, tell the others about it (don't rely on peer type, that will already be reset)
			time_t onlineTime = u->joined ? time(NULL)-u->joinedTime : 0;
			if (reason == RLeaving) { // the player is going on to another age, so he's not really offline
				// update online time
				tmCustomVaultPlayerStatus vaultStatus(*vaultServer, u->ki, alcGetStrGuid(serverGuid), serverName,
													  /* offline but will soon come back */ 2, onlineTime);
				send(vaultStatus);
			}
			else { // the player really went offline
				tmCustomVaultPlayerStatus vaultStatus(*vaultServer, u->ki, "0000000000000000" /* these are 16 zeroes */, "",
													  /* player is offline */0, onlineTime);
				send(vaultStatus);
			}
			
			// check if this was the last player
			bool lastPlayer = checkIfOnlyPlayer(u);
			{
				tSpinLock lock(autoShutdownMutex);
				lastPlayerLeft = lastPlayer ? time(NULL) : 0;
			}
			
			// remove leftovers of this player from the age state
			ageState->removePlayer(u);
			
			// remove player from player list if he is still on there
			if (u->data) {
				log->log("WARN: Player %s did not log off correctly\n", u->str().c_str());
				bcastMemberUpdate(u, /*isJoined*/false);
				delete u->data;
				u->data = NULL;
			}
			
			// make sure this player is on none of the pages' player lists
			for (tAgeInfo::tPageList::iterator it = ageInfo->pages.begin(); it != ageInfo->pages.end(); ++it)
				removePlayerFromPage(&it->second, u->ki);
			
			// this player is no longer joined
			{
				tWriteLock lock(u->pubDataMutex);
				u->joined = false;
			}
			
			if (lastPlayer && resetStateWhenEmpty) { // reset age state, this was the last player
				resetStateWhenEmpty = false;
				ageState->clearAllStates();
			}
		}
	
		tUnetLobbyServerBase::onConnectionClosing(u, reason); // forward event (this sets the KI to zero, so do it at the end)
	}
	
	void tUnetGameServer::fwdDirectedGameMsg(tmGameMessageDirected &msg)
	{
		// look for all recipients
		tmCustomDirectedFwd::tRecList::iterator it = msg.recipients.begin();
		while (it != msg.recipients.end()) {
			tNetSessionRef session = sessionByKi(*it);
			if (*session && session->ki) { // forward messages to all players which have their KI set - even if they are still linking
				if (session->ki != msg.ki) { // don't send it back to the sender
					tmGameMessageDirected fwdMsg(*session, msg);
					send(fwdMsg);
				}
				it = msg.recipients.erase(it); // next one
			}
			else
				++it; // next one
		}
	}
	
	void tUnetGameServer::bcastMemberUpdate(tNetSession *u, bool isJoined)
	{
		tGameData *data = dynamic_cast<tGameData *>(u->data);
		if (!data) return;
		tReadLock lock(smgrMutex);
		for (tNetSessionMgr::tIterator it(smgr); it.next();) {
			if (*it == u) continue;
			if (it->data) {
				tmMemberUpdate memberUpdate(*it, data->createInfo(u), isJoined);
				send(memberUpdate);
			}
		}
	}
	
	tmGameMessage tUnetGameServer::makePlayerIdle(alc::tNetSession* u, alc::tUruObject rec, int inputState)
	{
		// get the right object for the receiver
		if (rec.pageId == 0xFFFF0304) { // Yeesha
			rec.objType = 0x008F;
			rec.objName = "Avatar01";
		}
		else if (rec.pageId == 0xFFFF030B) { // YeeshaNoGlow
			rec.objType = 0x008F;
			rec.objName = "AvatarYeeshaNoGlow";
		}
		else { // everyone else
			rec.objType = 0x0095;
			rec.objName = "LODAvatar01";
		}
		if (inputState >= 0) {
			// create the plAvatarInputStateMsg
			tpAvatarInputStateMsg inputStateMsg = tpAvatarInputStateMsg(tUruObjectRef());
			inputStateMsg.receivers.push_back(rec);
			inputStateMsg.flags = 0x00008140;
			inputStateMsg.state = inputState;
			// create the message
			return tmGameMessage(u, u->ki, &inputStateMsg);
		}
		else {
			// create the plAvBrainGenericMsg
			tpAvBrainGenericMsg avBrainMsg = tpAvBrainGenericMsg(tUruObjectRef());
			avBrainMsg.receivers.push_back(rec);
			avBrainMsg.flags = 0x00000A40;
			// create the message
			return tmGameMessage(u, u->ki, &avBrainMsg);
		}
	}

	void tUnetGameServer::onIdle()
	{
		bool goDownIdle;
		{
			tSpinLock lock(autoShutdownMutex);
			goDownIdle = lingerTime && lastPlayerLeft && lastPlayerLeft + lingerTime < time(NULL);
		}
		if (goDownIdle) {
			log->log("The last player left more than %d sec ago, so I will go down.\n", lingerTime);
			stop(); // no player for some time, so go down
		}
		tUnetLobbyServerBase::onIdle();
	}
	
	bool tUnetGameServer::checkIfOnlyPlayer(tNetSession *u)
	{
		tReadLock lock(smgrMutex);
		for (tNetSessionMgr::tIterator it(smgr); it.next();) {
			if (it->isUruClient() && *it != u) {
				return false;
			}
		}
		return true;
	}
	
	void tUnetGameServer::removePlayerFromPage(alc::tPageInfo* page, uint32_t ki)
	{
		bool removed = page->removePlayer(ki); // remove player from list of players who loaded that age
		if (!removed) return; // player did not even load that age
		if (page->owner != ki) return; // he is not the owner, we are done
		// search for another owner
		if (page->players.size()) {
			uint32_t newOwner = *page->players.begin();
			tNetSessionRef session = sessionByKi(newOwner);
			if (!*session) {
				// very strange, the player is on the list but not connected anymore?
				throw txUnet(_WHERE("Player %d is on list of players who loaded page %s, but not connected anymore", newOwner, page->name.c_str()));
			}
			page->owner = newOwner;
			tmGroupOwner groupOwner(*session, page, true/*is owner*/);
			send(groupOwner);
		}
		else {
			// no player left for this page, no owner
			page->owner = 0;
		}
	}

	int tUnetGameServer::onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		if (linkingOutIdle && msg->cmd == NetMsgFindAge) {
			// he is going to leave soon - make sure everyone is idle for him
			tReadLock lock(smgrMutex);
			for (tNetSessionMgr::tIterator it(smgr); it.next();) {
				if (*it == u) continue;
				tGameData *data = dynamic_cast<tGameData *>(it->data);
				if (data) {
					// make sure noone else is sitting/has his KI open
					tmGameMessage msg(u, makePlayerIdle(*it, data->obj));
					send(msg);
					// make sure noone else is just running an animation
					tmGameMessage msgWalk(makePlayerIdle(u, data->obj, 1)); // let it walk forwards
					send(msgWalk, 0.2); // 200msecs after the sit/KI state message
					tmGameMessage msgStop(makePlayerIdle(u, data->obj, 0)); // let it walk forwards
					send(msgStop, 0.2+0.1); // 100msecs after the walk message (don't make this lower than 50msecs!)
				}
			}
		}
	
		int ret = tUnetLobbyServerBase::onMsgRecieved(msg, u); // first let tUnetLobbyServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			//// message for joining the game and for initializing the new client
			case NetMsgJoinReq:
			{
				if (!u->ki) {
					err->log("ERR: %s sent a NetMsgJoinReq but did not yet set his KI. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmJoinReq joinReq(u, msg);
				
				// the player is joined - tell tracking and (perhaps) vault
				tmCustomPlayerStatus trackingStatus(*trackingServer, u, 2 /* visible */, RActive);
				send(trackingStatus);
				tmCustomVaultPlayerStatus vaultStatus(*vaultServer, u->ki, alcGetStrGuid(serverGuid), serverName, 1 /* is online */, 0 /* don't increase online time now, do that on disconnect */);
				send(vaultStatus);
				
				// ok, tell the client he successfully joined
				{
					tWriteLock lock(u->pubDataMutex);
					u->joined = true;
					u->joinedTime = time(NULL);
				}
				const tStreamable *ageSDLState = ageState->getAgeState();
				if (ageSDLState) {
					tmJoinAck joinAck(u, joinReq.x, ageSDLState);
					send(joinAck);
				}
				else { // nothing found: send empty state
					tSdlState empty;
					tmJoinAck joinAck(u, joinReq.x, &empty);
					send(joinAck);
				}
				// log the join
				sec->log("%s joined\n", u->str().c_str());
				// now, it'll stat sending GameMessages
				
				return 1;
			}
			case NetMsgGameStateRequest:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgGameStateRequest but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmGameStateRequest gameStateRequest(u, msg);
				
				int n = 0;
				if (gameStateRequest.hasFlags(plNetStateReq1))
					n += ageState->sendClones(u); // only send clones for first state
				n += ageState->sendSdlStates(u, &gameStateRequest.pages);
				
				if (gameStateRequest.hasFlags(plNetStateReq1)) {
					// send this only when it's the initial request
					tmInitialAgeStateSent stateSent(u, n);
					send(stateSent);
				}
				
				return 1;
			}
			case NetMsgMembersListReq:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgMembersListReq but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmMembersListReq membersListReq(u, msg);
				
				// send members list
				tmMembersList list(u);
				list.members.reserve(smgr->getCount()); // avoid moving the member info structs
				tReadLock lock(smgrMutex);
				for (tNetSessionMgr::tIterator it(smgr); it.next();) {
					if (*it == u) continue;
					tGameData *data = dynamic_cast<tGameData *>(it->data);
					if (data) {
						list.members.push_back(data->createInfo(*it));
					}
				}
				send(list);
				
				// now the client should have finished loading the age
				u->setTimeout(authedTimeout);
				
				return 1;
			}
			case NetMsgPagingRoom:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgPagingRoom but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmPagingRoom pagingRoom(u, msg);
				
				tPageInfo *page = ageInfo->getPage(pagingRoom.pageId);
				if (!page)
					throw txProtocolError(_WHERE("Requested non-existing page %s (0x%08X)", pagingRoom.pageName.c_str(), pagingRoom.pageId));
				// fill in page information
				if (!page->pageId) {
					page->pageId = pagingRoom.pageId;
					page->pageType = pagingRoom.pageType;
				}
				
				// process message
				if (pagingRoom.isPageOut) {
					// player paged out, remove him as owner
					removePlayerFromPage(page, u->ki);
				}
				else if (!page->hasPlayer(u->ki)) {
					// paged in
					bool isOwner = false;
					page->players.push_back(u->ki); // this player loaded that page
					if (!page->owner) {
						isOwner = true;
						page->owner = u->ki; // this player is the new owner
					}
					tmGroupOwner groupOwner(u, page, isOwner);
					send(groupOwner);
				}
				return 1;
			}
			
			//// game messages
			case NetMsgGameMessage:
			{
				if (!u->joined) {
					log->log("WARN: %s sent a NetMsgGameMessage but did not yet join the game - ignore it.\n", u->str().c_str());
					// even the normal client sometimes does this, I don't know why, so just ignore this message
					return 2; // ignored
				}
				
				// get the data out of the packet
				tmGameMessage gameMsg(u, msg);
				
				// process contained plasma message
				if (!processGameMessage(&gameMsg.msgStream, u)) {
					// broadcast message
					bcastMessage(gameMsg);
				}
				
				return 1;
			}
			case NetMsgGameMessageDirected:
			{
				if (!u->ki) { // directed game messages are accepted from all players which have their KI set
					err->log("ERR: %s sent a NetMsgGameMessageDirected but did not yet set the KI. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmGameMessageDirected gameMsg(u, msg);
				
				// process contained plasma message
				tUruObjectRef receiver;
				if (processGameMessage(&gameMsg.msgStream, u, &receiver)) return 1;
				
				// Because sharing the Relto book causes everyone in the age to crash
				// out, throw out the initial message of the exchange. This is all we
				// can do without client-side PRP file updates.
				// To distinguish Relto book sharing from other book sharing, we check
				// the type of the page the PythonFileMod doing all this is on: The
				// Relto one is in a GUI page with type 0x0004, books you find in the
				// age have their PythonFileMod on a page with type 0x0000
				if (noReltoShare && gameMsg.msgStream.getType() == plNotifyMsg && receiver.hasObj && receiver.obj.pageType != 0x000) {
					log->log("INF: Throwing out relto book share notification from %s\n", u->str().c_str());
					sendKIMessage(tString("Ignoring the relto book share notification you just sent - it would crash people"), u);
					return 1;
				}
				
				// forward it
				fwdDirectedGameMsg(gameMsg);
				
				if (gameMsg.recipients.size()) { // we did not yet reach all recipients
					// this is a more reliable method to know whether to forward messages to tracking - the alternative would be the plNetDirected flag
					tmCustomDirectedFwd fwdMsg(*trackingServer, gameMsg);
					send(fwdMsg);
				}
				
				return 1;
			}
			case NetMsgCustomDirectedFwd:
			{
				if (u != *trackingServer) {
					err->log("ERR: %s sent a NetMsgCustomDirectedFwd but is not the tracking server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomDirectedFwd gameMsg(u, msg);
				
				// forward it
				fwdDirectedGameMsg(gameMsg);
				
				return 1;
			}
			
			//// SDL and age state messages
			case NetMsgSDLState:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgSDLState but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSDLState SDLState(u, msg);
				
				// save SDL state
				ageState->saveSdlState(SDLState.content, SDLState.obj);
				
				return 1;
			}
			case NetMsgSDLStateBCast:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgSDLStateBCast but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSDLStateBCast SDLStateBCast(u, msg);
				
				// save SDL state
				ageState->saveSdlState(SDLStateBCast.content, SDLStateBCast.obj);
				
				// broadcast message
				bcastMessage(SDLStateBCast);
				
				return 1;
			}
			case NetMsgLoadClone:
			{
				if (!u->joined) {
					log->log("WARN: %s sent a NetMsgLoadClone but did not yet join the game - ignore it.\n", u->str().c_str());
					// even the normal client sometimes does this, I don't know why, so just ignore this message
					return 2; // ignored
				}
				
				// get the data out of the packet
				tmLoadClone loadClone(u, msg);
				
				// parse contained plasma message
				tpLoadCloneMsg *loadCloneMsg = tpLoadCloneMsg::createFromStream(&loadClone.msgStream, u->gameType == tNetSession::UUGame);
				loadClone.checkSubMsg(loadCloneMsg);
				
				bool makeIdle = linkingOutIdle && loadClone.isPlayerAvatar && !loadClone.isLoad;
				if (makeIdle) {
					// he leaves - make him idle
					bcastMessage(makePlayerIdle(u, loadCloneMsg->clonedObj.obj));
					// It is too late to make others idle for him - if he linked out, that was already done when we got the NetMsgFindAge.
					// If he quits, he will do that too quickly.
				}
				
				// save clone in age state - this my delete the loadCloneMsg, so don't use it aferwards
				ageState->saveClone(loadCloneMsg);
				
				// broadcast message. A delay of less than 2700msecs is likely to cause crashes when the avatar just left the sitting state.
				bcastMessage(loadClone, /*delay*/ makeIdle ? 3 : 0 );
				
				// if it's an (un)load of the player's avatar, do the member list update
				if (loadClone.isPlayerAvatar) {
					if (!loadClone.isLoad && u->data) {
						bcastMemberUpdate(u, /*isJoined*/false);
						delete u->data;
						u->data = NULL;
					}
					else if (loadClone.isLoad && !u->data) {
						u->data = new tGameData(loadClone.obj);
						bcastMemberUpdate(u, /*isJoined*/true);
					}
				}
				
				return 1;
			}
			
			//// age management messages
			case NetMsgGetPublicAgeList:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgGetPublicAgeList but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				tmGetPublicAgeList getAgeList(u, msg);
				// forward to vault server
				send(tmGetPublicAgeList(*vaultServer, u->getSid(), getAgeList));
				return 1;
			}
			case NetMsgPublicAgeList:
				if (u == *vaultServer) {
					tmPublicAgeList ageList(u, msg);
					// now let tracking fill in the population counts
					send(tmPublicAgeList(*trackingServer, ageList));
					return 1;
				}
				else if (u == *trackingServer) {
					tmPublicAgeList ageList(u, msg);
					// it's ready, forward to client
					tNetSessionRef client = sessionBySid(ageList.sid);
					if (!*client || !client->isUruClient() || client->ki != ageList.ki) {
						err->log("ERR: I've got to tell player with KI %d about public ages, but can't find his session.\n", ageList.ki);
						return 1;
					}
					send(tmPublicAgeList(*client, ageList));
					return 1;
				}
				else {
					err->log("ERR: %s sent a NetMsgPublicAgeList but is not the vault or tracking server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
			case NetMsgCreatePublicAge:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgCreatePublicAge but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				tmCreatePublicAge createAge(u, msg);
				send(tmCreatePublicAge(*vaultServer, u->getSid(), createAge));
				return 1;
			}
			case NetMsgPublicAgeCreated:
			{
				if (u != *vaultServer) {
					err->log("ERR: %s sent a NetMsgPublicAgeCreated but is not the vault server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				tmPublicAgeCreated ageCreated(u, msg);
				// forward to client
				tNetSessionRef client = sessionBySid(ageCreated.sid);
				if (!*client || !client->isUruClient() || client->ki != ageCreated.ki) {
					err->log("ERR: I've got to tell player with KI %d about his created public age, but can't find his session.\n", ageCreated.ki);
					return 1;
				}
				send(tmPublicAgeCreated(*client, ageCreated));
				return 1;
			}
			case NetMsgRemovePublicAge:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgRemovePublicAge but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				tmRemovePublicAge removeAge(u, msg);
				send(tmRemovePublicAge(*vaultServer, u->getSid(), removeAge));
				return 1;
			}
			case NetMsgPublicAgeRemoved:
			{
				if (u != *vaultServer) {
					err->log("ERR: %s sent a NetMsgPublicAgeRemoved but is not the vault server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				tmPublicAgeRemoved ageRemoved(u, msg);
				// forward to client
				tNetSessionRef client = sessionBySid(ageRemoved.sid);
				if (!*client || !client->isUruClient() || client->ki != ageRemoved.ki) {
					err->log("ERR: I've got to tell player with KI %d about his created public age, but can't find his session.\n", ageRemoved.ki);
					return 1;
				}
				send(tmPublicAgeRemoved(*client, ageRemoved));
				return 1;
			}
			
			//// Server control messages
			case NetMsgCustomPlayerToCome:
			{
				if (u != *trackingServer) {
					err->log("ERR: %s sent a NetMsgCustomPlayerToCome but is not the tracking server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomPlayerToCome playerToCome(u, msg);
				
				{
					tSpinLock lock(autoShutdownMutex);
					if (lastPlayerLeft) {
						// stay up as if the last player left now, that should be long enough for the new player to join
						lastPlayerLeft = time(NULL);
					}
				}
				
				// Send the reply back
				send(tmCustomPlayerToCome(u, playerToCome.ki));
				
				return 1;
			}
			case NetMsgSetTimeout:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgSetTimeout but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSetTimeout setTimeout(u, msg);
				// let's ignore this - I don't think the client should tell the server what the timeout is, nor does 180 seconds seem a good choice to me
				return 1;
			}
			case NetMsgPython:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgPython but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				tmPython python(u, msg);
				// Just ignore it, server-side Python is not supported
				return 1;
			}
			
			//// unknown purpose messages
			case NetMsgTestAndSet:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgTestAndSet but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmTestAndSet testAndSet(u, msg);
				
				// if required, send a reply - this is simply copied from the old game server, don't ask me what it means
				if (testAndSet.isLockReq) {
					// build the game message
					tpServerReplyMsg serverReplyMsg = tpServerReplyMsg(tUruObjectRef()); // the parent is an empty object
					serverReplyMsg.receivers.push_back(tUruObjectRef(testAndSet.obj)); // add the sent object as receiver
					serverReplyMsg.flags = 0x00000800;
					serverReplyMsg.replyType = 1; // this means "affirm"
					// send message
					tmGameMessage msg(u, u->ki, &serverReplyMsg);
					send(msg);
				}
				
				return 1;
			}
			case NetMsgPlayerPage:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgPlayerPage but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmPlayerPage playerPage(u, msg);
				// And now we just have to find out why we should care that the avatar is paged in or out - this occurs only when the game is started or quit
				return 1;
			}
			case NetMsgRelevanceRegions:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgRelevanceRegions but did not yet join the game. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmRelevanceRegions relevanceRegions(u, msg);
				// I have no clue what this message is supposed to do, and things work without reacting to it
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
