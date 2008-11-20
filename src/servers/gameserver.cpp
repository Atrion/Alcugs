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

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	const char * alcNetName="Game";
	Byte alcWhoami=KGame;

	tUnetGameServer::tUnetGameServer(void) : tUnetLobbyServerBase()
	{
		lerr->log("WARNING: The game server is not finished yet. So if it doesn\'t work, that's not even a bug.\n");
		
		lastPlayerLeft = 0;
		
		// find out which age we are supposed to host
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("age_filename");
		if (var.size() < 2) throw txBase(_WHERE("an age name must be set"));
		strncpy((char *)serverName, (char *)var.c_str(), 199);
		var = cfg->getVar("age_guid");
		if (var.size() != 16) throw txBase(_WHERE("an age GUID must be set"));
		alcAscii2Hex(serverGuid, var.c_str(), 8);
		
		// load our age info
		tAgeInfoLoader ageInfoLoader(serverName, /*loadPages*/true);
		ageInfo = new tAgeInfo(*ageInfoLoader.getAge(serverName)); // get ourselves a copy of it
		
		// load SDL Manager
		ageState = new tAgeStateManager(this);
	}

	tUnetGameServer::~tUnetGameServer(void)
	{
		delete ageInfo;
		delete ageState;
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
				tmCustomVaultPlayerStatus vaultStatus(vaultServer, u->ki, (Byte *)"0000000000000000" /* these are 16 zeroes */, (Byte *)"", /* player is offline */0, u->onlineTime());
				send(vaultStatus);
			}
			
			// check if this was the last player
			bool playerFound = false;
			tNetSession *session;
			smgr->rewind();
			while ((session = smgr->getNext())) {
				if (session->joined && session != u) {
					playerFound = true;
					break;
				}
			}
			lastPlayerLeft = playerFound ? 0 : alcGetTime();
			
			// remove leftovers of this player from the age state
			ageState->removePlayer(u->ki);
			
			// remove player from player list if he is still on there
			if (u->data) {
				bcastMemberUpdate(u, /*isJoined*/false);
				delete u->data;
				u->data = NULL;
			}
			
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
		for (int i = 0; i < msg.nRecipients; ++i) {
			U32 recip = msg.recipients[i];
			// now search for that player
			smgr->rewind();
			while ((session = smgr->getNext())) {
				if (session->ki == recip) {
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
		if (lastPlayerLeft && lastPlayerLeft + 120 < alcGetTime()) {
			log->log("The last player left more than 120sec ago, so I will go down.\n");
			stop(); // no player for 120sec, so go down. FIXME: Make time configurable
		}
		tUnetLobbyServerBase::onIdle(idle);
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
				msg->data->get(joinReq);
				log->log("<RCV> [%d] %s\n", msg->sn, joinReq.str());
				
				// the player is joined - tell tracking and vault
				tNetSession *trackingServer = getSession(tracking), *vaultServer = getSession(vault);
				if (!trackingServer || !vaultServer) {
					err->log("ERR: Player %s is joining, but vault or tracking is unavailable.\n", u->str());
					return 1;
				}
				tmCustomPlayerStatus trackingStatus(trackingServer, u->ki, u->getSid(), u->uid, u->name, u->avatar, 2 /* visible */, RActive);
				send(trackingStatus);
				tmCustomVaultPlayerStatus vaultStatus(vaultServer, u->ki, alcGetStrGuid(serverGuid), serverName, 1 /* is online */, 0 /* don't increase online time now, do that on disconnect */);
				send(vaultStatus);
				
				// ok, tell the client he successfully joined
				u->joined = true;
				tmJoinAck joinAck(u, joinReq.x);
				ageState->writeAgeState(&joinAck.sdl);
				send(joinAck);
				// log the join
				sec->log("%s joined\n", u->str());
				// now, it'll stat sending GameMessages
				
				// a new player joined, so we are no longer alone
				lastPlayerLeft = 0;
				
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
				msg->data->get(gameStateRequest);
				log->log("<RCV> [%d] %s\n", msg->sn, gameStateRequest.str());
				
				int n = ageState->sendClones(u);
				n += ageState->sendSdlStates(u); // FIXME: take requested pages into account
				
				tmInitialAgeStateSent stateSent(u, n);
				send(stateSent);
				
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
				msg->data->get(membersListReq);
				log->log("<RCV> [%d] %s\n", msg->sn, membersListReq.str());
				
				// FIXME: send members list
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
				msg->data->get(pagingRoom);
				log->log("<RCV> [%d] %s\n", msg->sn, pagingRoom.str());
				
				// FIXME: do something
				return 1;
			}
			
			//// game messages
			case NetMsgGameMessage:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgGameMessage but did not yet join the game - ignore it.\n", u->str());
					// even the normal client sometimes does this, I donÄt know why, so just ignore this message
					msg->data->end(); // avoid a warning because the message was too long
					return 1;
				}
				
				// get the data out of the packet
				tmGameMessage gameMsg(u);
				msg->data->get(gameMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, gameMsg.str());
				
				// broadcast message
				tNetSession *session;
				smgr->rewind();
				while ((session = smgr->getNext())) {
					if (session->joined && session->ki != gameMsg.ki) {
						tmGameMessage fwdMsg(session, gameMsg);
						send(fwdMsg);
					}
				}
				
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
				msg->data->get(gameMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, gameMsg.str());
				
				// Because sharing the Relto book causes everyone in the age to crash
				// out, throw out the initial message of the exchange. This is all we
				// can do without client-side PRP file updates. Other sharing is fine
				// and I could write code to throw out *only* the Relto share, but we
				// don't need sharing the way things are usually set up right now, so
				// it's not worth the effort to me. (Also, sharing is not enabled for
				// books, just Bahro stones.)
				if (gameMsg.getSubMsgType() == 0x02E8) { // plNotifyMsg
					log->log("INF: Throwing out book share notification from %s\n", u->str());
					return 1;
				}
				
				// forward it
				Byte nSent = fwdDirectedGameMsg(gameMsg);
				
				if (nSent < gameMsg.nRecipients) { // we did not yet reach all recipients
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
				msg->data->get(gameMsg);
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
				msg->data->get(SDLState);
				log->log("<RCV> [%d] %s\n", msg->sn, SDLState.str());
				
				// save SDL state
				ageState->saveSdlState(SDLState.obj, SDLState.sdl);
				
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
				msg->data->get(SDLStateBCast);
				log->log("<RCV> [%d] %s\n", msg->sn, SDLStateBCast.str());
				
				// save SDL state
				ageState->saveSdlState(SDLStateBCast.obj, SDLStateBCast.sdl);
				// FIXME: The old game server sets a restriction how many updates can be sent for a physical in a certain time
				
				// NetMsgSDLState sometimes has the bcast falg set and NetMsgSDLStateBCast sometimes doesn't.
				// I assume the message type is correct and the flag not.
				
				// broadcast message
				tNetSession *session;
				smgr->rewind();
				while ((session = smgr->getNext())) {
					if (session->joined && session->ki != SDLStateBCast.ki) {
						tmSDLStateBCast fwdMsg(session, SDLStateBCast);
						send(fwdMsg);
					}
				}
				return 1;
			}
			case NetMsgLoadClone:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgLoadClone but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmLoadClone loadClone(u);
				msg->data->get(loadClone);
				log->log("<RCV> [%d] %s\n", msg->sn, loadClone.str());
				
				ageState->saveClone(loadClone);
				
				// broadcast message
				tNetSession *session;
				smgr->rewind();
				while ((session = smgr->getNext())) {
					if (session->joined && session->ki != loadClone.ki) {
						tmLoadClone fwdMsg(session, loadClone);
						send(fwdMsg);
					}
				}
				
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
			
			//// unknown purpose messages
			case NetMsgTestAndSet:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgTestAndSet but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmTestAndSet testAndSet(u);
				msg->data->get(testAndSet);
				log->log("<RCV> [%d] %s\n", msg->sn, testAndSet.str());
				
				// if required, send a reply - this is simply copied from the old game server, don't ask me what it means
				if (testAndSet.lockReq) {
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
				msg->data->get(playerPage);
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
				msg->data->get(relevanceRegions);
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
				msg->data->get(setTimeout);
				log->log("<RCV> [%d] %s\n", msg->sn, setTimeout.str());
				// I have no clue what this message is supposed to do (well, it obviously should somehow set the timeout, but neither
				// do I know how 0x43340000 should be 180sec nor is that a useful timeout). Things work without reacting to it.
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc

