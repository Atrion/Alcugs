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
#define __U_GAMESERVER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/msgparsers.h>

////extra includes
#include "gameserver.h"
#include "sdl.h"
#include "gamesubmsg.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	const char * alcNetName="Game";
	Byte alcWhoami=KGame;

	tUnetGameServer::tUnetGameServer(void) : tUnetLobbyServerBase()
	{
		lastPlayerLeft = 0;
		
		// find out which age we are supposed to host
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("age_filename");
		if (var.size() < 2) throw txBase(_WHERE("an age name must be set"));
		strncpy(serverName, var.c_str(), 199);
		var = cfg->getVar("age_guid");
		if (var.size() != 16) throw txBase(_WHERE("an age GUID must be set"));
		alcAscii2Hex(serverGuid, var.c_str(), 8);
		
		// load our age info
		tAgeInfoLoader ageInfoLoader(serverName, /*loadPages*/true);
		ageInfo = new tAgeInfo(*ageInfoLoader.getAge(serverName)); // get ourselves a copy of it
		
		// load SDL Manager
		ageState = new tAgeStateManager(this, ageInfo);
		
		// make sure we quit if noone comes
		lastPlayerLeft = alcGetTime();
	}
	
	bool tUnetGameServer::canPortBeUsed(U16 port) {
		return (port >= spawnStart && port <= spawnStop);
	}

	tUnetGameServer::~tUnetGameServer(void)
	{
		delete ageInfo;
		delete ageState;
	}
	
	void tUnetGameServer::onLoadConfig(void)
	{
		tUnetLobbyServerBase::onLoadConfig();
		
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("game.persistent");
		if (!var.isNull() && var.asByte()) { // disabled per default
			lingerTime = 0;
		}
		else {
			var = cfg->getVar("game.linger_time");
			if (!var.isNull())
				lingerTime = var.asU32();
			else
				lingerTime = 120; // default
			if (lingerTime && lingerTime < 20)
				log->log("WARN: You set a linger time of only %d, which could result in the game server going down before the player even has the chance to join it - I recommend to set it to at least 20\n", lingerTime);
		}
	}

	void tUnetGameServer::onConnectionClosed(tNetEvent *ev, tNetSession *u)
	{
		if (ev->sid == tracking && isRunning()) {
			err->log("ERR: I lost the connection to the tracking server, so I will go down\n");
			/* The game server should go down when it looses the connection to tracking. This way, you can easily
			   shut down all game servers. In addition, it won't get any new peers anyway without the tracking server */
			stop();
		}
		else
			tUnetLobbyServerBase::onConnectionClosed(ev, u);
	}
	
	void tUnetGameServer::additionalVaultProcessing(tNetSession *u, tvMessage *msg)
	{
		/* this is a very dirty fix for the bahro poles, but as long as the game server doesn't subscribe to the vault to get it's 
		own SDL node, we have to do it this way */
		
		if (u->getPeerType() == KVault) return; // we care only about messages from the client
		if (msg->task) return; // ignore vault tasks
		if (msg->cmd != VSaveNode) return; // we are only interested in VSaveNode messages
		
		// ok, now find the saved node
		tvNode *node = NULL;
		for (tvMessage::tItemList::iterator it = msg->items.begin(); it != msg->items.end(); ++it) {
			if ((*it)->id != 5) continue;
			node = (*it)->asNode(); // got it
			break;
		}
		if (!node)
			throw txProtocolError(_WHERE("A VSaveNode without a node attached???"));
		if (node->type != KSDLNode) return;
		if (!node->blob1Size) return; // don't bother parsing empty messages
		// got the node, and it is a SDL one... get the SDL binary stream
		tMBuf data(node->blob1Size);
		data.write(node->blob1, node->blob1Size);
		data.rewind();
		ageState->saveSdlVaultMessage(data, u); // process it
	}
	
	void tUnetGameServer::playerAuthed(tNetSession *u)
	{
		// a new player connected, so we are no longer alone
		// we have to do this when the player authenticates because it will download the age before joining, in which time the auto-kill could already stop the server
		lastPlayerLeft = 0;
	}
	
	void tUnetGameServer::terminate(tNetSession *u, Byte reason, bool destroyOnly)
	{
		if (u->getPeerType() == KClient && u->ki != 0) { // if necessary, tell the others about it
			tNetSession *vaultServer = getSession(vault);
			if (!vaultServer) {
				err->log("ERR: I've got to update a player\'s (%s) status for the vault server, but it is unavailable.\n", u->str());
			}
			else if (reason == RLeaving) { // the player is going on to another age, so he's not really offline
				if (!u->proto || u->proto >= 3) { // unet2 or unet3 vault servers would not understand a state of 2
					// update online time
					tmCustomVaultPlayerStatus vaultStatus(vaultServer, u->ki, alcGetStrGuid(serverGuid), serverName, /* offline but will soon come back */ 2, u->onlineTime());
					send(vaultStatus);
				}
			}
			else { // the player really went offline
				tmCustomVaultPlayerStatus vaultStatus(vaultServer, u->ki, "0000000000000000" /* these are 16 zeroes */, "", /* player is offline */0, u->onlineTime());
				send(vaultStatus);
			}
			
			// check if this was the last player
			bool playerFound = false;
			tNetSession *session;
			smgr->rewind();
			while ((session = smgr->getNext())) {
				if (session->getPeerType() == KClient && session != u) {
					playerFound = true;
					break;
				}
			}
			lastPlayerLeft = playerFound ? 0 : alcGetTime();
			
			// remove leftovers of this player from the age state
			ageState->removePlayer(u);
			
			// remove player from player list if he is still on there
			if (u->data) {
				log->log("WARN: Player %s did not log off correctly\n", u->str());
				bcastMemberUpdate(u, /*isJoined*/false);
				delete u->data;
				u->data = NULL;
			}
			
			// make sure this player is on none of the pages' player lists
			for (tAgeInfo::tPageList::iterator it = ageInfo->pages.begin(); it != ageInfo->pages.end(); ++it)
				removePlayerFromPage(&*it, u->ki);
			
			// this player is no longer joined
			u->joined = false;
		}
	
		tUnetLobbyServerBase::terminate(u, reason, destroyOnly); // do the lobbybase terminate procedure
	}
	
	Byte tUnetGameServer::fwdDirectedGameMsg(tmGameMessageDirected &msg)
	{
		// look for all recipients
		Byte nSent = 0;
		tNetSession *session;
		for (tmCustomDirectedFwd::tRecList::iterator it = msg.recipients.begin(); it != msg.recipients.end(); ++it) {
			// now search for that player
			smgr->rewind();
			while ((session = smgr->getNext())) {
				if (session->ki == *it) {
					if (session->joined && session->ki != msg.ki) {
						tmGameMessageDirected fwdMsg(session, msg);
						send(fwdMsg);
					}
					++nSent;
					break;
				}
			}
		}
		return nSent;
	}
	
	template <class T> void tUnetGameServer::bcastMessage(T &msg)
	{
		// broadcast message
		tNetSession *session;
		smgr->rewind();
		while ((session = smgr->getNext())) {
			if (session->joined && session->ki != msg.ki) {
				T fwdMsg(session, msg);
				send(fwdMsg);
			}
		}
	}
	
	void tUnetGameServer::bcastMemberUpdate(tNetSession *u, bool isJoined)
	{
		tNetSession *session;
		smgr->rewind();
		while ((session = smgr->getNext())) {
			if (session != u && session->data) {
				tmMemberUpdate memberUpdate(session, u, ((tGameData *)u->data)->obj, isJoined);
				send(memberUpdate);
			}
		}
	}

	void tUnetGameServer::onIdle(bool idle)
	{
		if (idle && lingerTime && lastPlayerLeft && lastPlayerLeft + lingerTime < alcGetTime()) {
			log->log("The last player left more than %d sec ago, so I will go down.\n", lingerTime);
			stop(); // no player for some time, so go down
		}
		tUnetLobbyServerBase::onIdle(idle);
	}
	
	void tUnetGameServer::removePlayerFromPage(tPageInfo *page, U32 ki)
	{
		tPageInfo::tPlayerList::iterator it = page->getPlayer(ki);
		if (it == page->players.end()) return; // player did not even load that age
		page->players.erase(it); // remove player from list of players who loaded that age
		if (page->owner != ki) return; // he is not the owner, we are done
		// search for another owner
		if (page->players.size()) {
			U32 newOwner = *page->players.begin();
			tNetSession *session = smgr->find(newOwner);
			if (!session) {
				// very strange, the player is on the list but not connected anymore?
				throw txUnet(_WHERE("Player %d is on list of players who loaded page %s, but not connected anymore", newOwner, page->name));
			}
			page->owner = newOwner;
			tmGroupOwner groupOwner(session, page, true/*is owner*/);
			send(groupOwner);
		}
		else {
			// no player left for this page, no owner
			page->owner = 0;
		}
	}

	int tUnetGameServer::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetLobbyServerBase::onMsgRecieved(ev, msg, u); // first let tUnetLobbyServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			//// message for joining the game and for initializing the new client
			case NetMsgJoinReq:
			{
				if (!u->ki) {
					err->log("ERR: %s sent a NetMsgJoinReq but did not yet set his KI. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmJoinReq joinReq(u);
				msg->data.get(joinReq);
				log->log("<RCV> [%d] %s\n", msg->sn, joinReq.str());
				
				// the player is joined - tell tracking and (perhaps) vault
				tNetSession *trackingServer = getSession(tracking), *vaultServer = getSession(vault);
				if (!trackingServer || !vaultServer) {
					err->log("ERR: Player %s is joining, but vault or tracking is unavailable.\n", u->str());
					return 1;
				}
				tmCustomPlayerStatus trackingStatus(trackingServer, u->ki, u->getSid(), u->uid, u->name, u->avatar, 2 /* visible */, RActive);
				send(trackingStatus);
				tmCustomVaultPlayerStatus vaultStatus(vaultServer, u->ki, alcGetStrGuid(serverGuid), serverName, 1 /* is online */, 0 /* don't increase online time now, do that on disconnect */);
				send(vaultStatus);
				
				// now that the player joined, age loading is done, so decrease timeout to 30sec
				u->setTimeout(30);
				
				// ok, tell the client he successfully joined
				u->joined = true;
				tmJoinAck joinAck(u, joinReq.x);
				ageState->writeAgeState(&joinAck.sdl);
				send(joinAck);
				// log the join
				sec->log("%s joined\n", u->str());
				// now, it'll stat sending GameMessages
				
				return 1;
			}
			case NetMsgGameStateRequest:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgGameStateRequest but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmGameStateRequest gameStateRequest(u);
				msg->data.get(gameStateRequest);
				log->log("<RCV> [%d] %s\n", msg->sn, gameStateRequest.str());
				
				int n = 0;
				if (!gameStateRequest.pages.size())
					n += ageState->sendClones(u); // only send clones if global state is requested
				n += ageState->sendSdlStates(u, &gameStateRequest.pages);
				
				if (!gameStateRequest.pages.size()) {
					// send this only when it's the initial request
					tmInitialAgeStateSent stateSent(u, n);
					send(stateSent);
				}
				
				return 1;
			}
			case NetMsgMembersListReq:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgMembersListReq but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmMembersListReq membersListReq(u);
				msg->data.get(membersListReq);
				log->log("<RCV> [%d] %s\n", msg->sn, membersListReq.str());
				
				// send members list
				tmMembersList list(u);
				list.members.reserve(smgr->getSize()); // avoid moving the member info structs
				tNetSession *session;
				smgr->rewind();
				while ((session = smgr->getNext())) {
					if (session != u && session->data) {
						list.members.push_back(tMemberInfo(session, ((tGameData *)session->data)->obj));
					}
				}
				send(list);
				
				return 1;
			}
			case NetMsgPagingRoom:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgPagingRoom but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmPagingRoom pagingRoom(u);
				msg->data.get(pagingRoom);
				log->log("<RCV> [%d] %s\n", msg->sn, pagingRoom.str());
				
				tPageInfo *page = ageInfo->getPage(pagingRoom.pageId);
				if (!page)
					throw txProtocolError(_WHERE("Requested non-existing page %s (0x%08X)", pagingRoom.pageName.c_str(), pagingRoom.pageId));
				// fill in page information
				if (!page->plasmaPageId) {
					page->plasmaPageId = pagingRoom.pageId;
					page->plasmaPageType = pagingRoom.pageType;
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
					err->log("ERR: %s sent a NetMsgGameMessage but did not yet join the game - ignore it.\n", u->str());
					// even the normal client sometimes does this, I don't know why, so just ignore this message
					msg->data.end(); // avoid a warning because the message was too long
					return 1;
				}
				
				// get the data out of the packet
				tmGameMessage gameMsg(u);
				msg->data.get(gameMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, gameMsg.str());
				
				// verify contained game message
				U16 msgType = gameMsg.getSubMsgType();
				if (msgType == 0x024E || msgType == 0x03AC) // 0x024E = plLoadCloneMsg, 0x03AC = plLoadAvatarMsg
					throw txProtocolError(_WHERE("Got game message with invalid sub message type ox%04X", msgType));
				
				// broadcast message
				bcastMessage(gameMsg);
				
				return 1;
			}
			case NetMsgGameMessageDirected:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgGameMessageDirected but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmGameMessageDirected gameMsg(u);
				msg->data.get(gameMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, gameMsg.str());
				
				// verify contained game message
				U16 msgType = gameMsg.getSubMsgType();
				if (msgType == 0x024E || msgType == 0x03AC) // 0x024E = plLoadCloneMsg, 0x03AC = plLoadAvatarMsg
					throw txProtocolError(_WHERE("Got game message with invalid sub message type ox%04X", msgType));
				
				// Because sharing the Relto book causes everyone in the age to crash
				// out, throw out the initial message of the exchange. This is all we
				// can do without client-side PRP file updates. Other sharing is fine
				// and I could write code to throw out *only* the Relto share, but we
				// don't need sharing the way things are usually set up right now, so
				// it's not worth the effort to me. (Also, sharing is not enabled for
				// books, just Bahro stones.)
				if (msgType == 0x02E8) { // plNotifyMsg
					log->log("INF: Throwing out book share notification from %s\n", u->str());
					return 1;
				}
				
				// forward it
				Byte nSent = fwdDirectedGameMsg(gameMsg);
				
				if (nSent < gameMsg.recipients.size()) { // we did not yet reach all recipients
					tNetSession *trackingServer = getSession(tracking);
					if (!trackingServer) {
						err->log("ERR: I've got to to forward a message through the tracking server, but it's unavailable.\n");
						return 1;
					}
					tmCustomDirectedFwd fwdMsg(trackingServer, gameMsg);
					send(fwdMsg);
				}
				
				return 1;
			}
			case NetMsgCustomDirectedFwd:
			{
				if (u->getPeerType() != KTracking) {
					err->log("ERR: %s sent a NetMsgCustomDirectedFwd but is not the tracking server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomDirectedFwd gameMsg(u);
				msg->data.get(gameMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, gameMsg.str());
				
				// forward it
				fwdDirectedGameMsg(gameMsg);
				
				return 1;
			}
			
			//// SDL and age state messages
			case NetMsgSDLState:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgSDLState but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSDLState SDLState(u);
				msg->data.get(SDLState);
				log->log("<RCV> [%d] %s\n", msg->sn, SDLState.str());
				
				// save SDL state
				ageState->saveSdlState(SDLState.sdl);
				
				// NetMsgSDLState sometimes has the bcast falg set and NetMsgSDLStateBCast sometimes doesn't.
				// I assume the message type is correct and the flag not.
				
				return 1;
			}
			case NetMsgSDLStateBCast:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgSDLStateBCast but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSDLStateBCast SDLStateBCast(u);
				msg->data.get(SDLStateBCast);
				log->log("<RCV> [%d] %s\n", msg->sn, SDLStateBCast.str());
				
				// save SDL state
				ageState->saveSdlState(SDLStateBCast.sdl);
				// FIXME: The old game server sets a restriction how many updates can be sent for a physical in a certain time
				
				// NetMsgSDLState sometimes has the bcast falg set and NetMsgSDLStateBCast sometimes doesn't.
				// I assume the message type is correct and the flag not.
				
				// broadcast message
				bcastMessage(SDLStateBCast);
				
				return 1;
			}
			case NetMsgLoadClone:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgLoadClone but did not yet join the game - ignore it.\n", u->str());
					// even the normal client sometimes does this, I don't know why, so just ignore this message
					msg->data.end(); // avoid a warning because the message was too long
					return 1;
				}
				
				// get the data out of the packet
				tmLoadClone loadClone(u);
				msg->data.get(loadClone);
				log->log("<RCV> [%d] %s\n", msg->sn, loadClone.str());
				
				// parse contained game message
				U16 msgType = loadClone.getSubMsgType();
				tLoadCloneMsg loadCloneMsg(msgType == 0x03AC); // the plLoadAvatarMsgs are treated a bit differently
				if (msgType == 0x024E || msgType == 0x03AC) { // 0x024E = plLoadCloneMsg, 0x03AC = plLoadAvatarMsg
					loadClone.message.get(loadCloneMsg);
					loadCloneMsg.checkNetMsg(loadClone);
				}
				else
					throw txProtocolError(_WHERE("The sub message of a NetMsgLoadClone must be of the type 0x024E (plLoadCloneMsg) or 0x03AC (plLoadAvatarMsg), not 0x%04X", msgType));
				
				// save clone in age state
				ageState->saveClone(loadCloneMsg);
				
				// broadcast message
				bcastMessage(loadClone);
				
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
			
			//// message to prevent the server from going down while someone joins
			case NetMsgCustomPlayerToCome:
			{
				if (u->getPeerType() != KTracking) {
					err->log("ERR: %s sent a NetMsgCustomPlayerToCome but is not the tracking server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomPlayerToCome playerToCome(u);
				msg->data.get(playerToCome);
				log->log("<RCV> [%d] %s\n", msg->sn, playerToCome.str());
				
				if (lastPlayerLeft) {
					// stay up as if the last player left now, that should be long enough for the new player to join
					lastPlayerLeft = alcGetTime();
				}
				
				return 1;
			}
			
			//// unknown purpose messages
			case NetMsgTestAndSet:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgTestAndSet but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmTestAndSet testAndSet(u);
				msg->data.get(testAndSet);
				log->log("<RCV> [%d] %s\n", msg->sn, testAndSet.str());
				
				// if required, send a reply - this is simply copied from the old game server, don't ask me what it means
				if (testAndSet.isLockReq) {
					tmGameMessage msg(u, u->ki);
					// build the game message
					msg.message.putU16(0x026A); // game message cmd: plServerReplyMsg
					msg.message.putByte(0);
					msg.message.putU32(1);
					msg.message.putByte(1);
					msg.message.put(testAndSet.obj);
					msg.message.putByte(1);
					msg.message.putU32(0);
					msg.message.putU32(0);
					msg.message.putU16(8);
					msg.message.putByte(0);
					msg.message.putU32(1);
					// send it
					send(msg);
				}
				
				return 1;
			}
			case NetMsgPlayerPage:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgPlayerPage but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmPlayerPage playerPage(u);
				msg->data.get(playerPage);
				log->log("<RCV> [%d] %s\n", msg->sn, playerPage.str());
				
				// This message is sent once when the client starts and links to the first age, and once again when the client quits and
				// completely leaves the game
				
				return 1;
			}
			case NetMsgRelevanceRegions:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgRelevanceRegions but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmRelevanceRegions relevanceRegions(u);
				msg->data.get(relevanceRegions);
				log->log("<RCV> [%d] %s\n", msg->sn, relevanceRegions.str());
				// I have no clue what this message is supposed to do, and things work without reacting to it
				return 1;
			}
			case NetMsgSetTimeout:
			case NetMsgSetTimeout2:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgSetTimeout but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSetTimeout setTimeout(u);
				msg->data.get(setTimeout);
				log->log("<RCV> [%d] %s\n", msg->sn, setTimeout.str());
				// I have no clue what this message is supposed to do (well, it obviously should somehow set the timeout, but neither
				// do I know how 0x43340000 should be 180sec nor is that a useful timeout). Things work without reacting to it.
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
