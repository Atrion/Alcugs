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

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	const char * alcNetName="Game";
	Byte alcWhoami=KGame;

	tUnetGameServer::tUnetGameServer(void) : tUnetLobbyServerBase()
	{
		lerr->log("WARNING: The game server is not finished yet. So if it doesn\'t work, that's not even a bug.\n");
		
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
	}

	tUnetGameServer::~tUnetGameServer(void)
	{
		delete ageInfo;
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

	int tUnetGameServer::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetLobbyServerBase::onMsgRecieved(ev, msg, u); // first let tUnetLobbyServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			//// message for joining the game
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
				
				// the player is joined - tell tracking
				tNetSession *trackingServer = getSession(tracking);
				if (!trackingServer) {
					err->log("ERR: Player %s is joining, but tracking is unavailable.\n", u->str());
					return 1;
				}
				tmCustomPlayerStatus status(trackingServer, u->ki, u->getSid(), u->uid, u->name, u->avatar, 2 /* visible */, RActive);
				send(status);
				
				// ok, tell the client he successfully joined
				u->joined = true;
				tmJoinAck joinAck(u, joinReq.x);
				if (joinReq.hasFlags(plNetP2P)) joinAck.setFlags(plNetFirewalled);
				send(joinAck);
				// now, it'll stat sending GameMessages
				
				return 1;
			}
			
			//// game messages
			case NetMsgGameMessage:
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgGameMessage but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmGameMessage gameMsg(u);
				msg->data->get(gameMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, gameMsg.str());
				
				// FIXME: do something
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
				
				// FIXME: do something
				return 1;
			}
			
			//// SDL and object state messages
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
				
				// FIXME: do something
				return 1;
			}
			case NetMsgPlayerPage: // FIXME: is this an SDL and avatar state message, or something related to pages?
			{
				if (!u->joined) {
					err->log("ERR: %s sent a NetMsgPlayerPage but did not yet join the game. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmPlayerPage playerPage(u);
				msg->data->get(playerPage);
				log->log("<RCV> [%d] %s\n", msg->sn, playerPage.str());
				
				// FIXME: do something
				return 1;
			}
			
			//// page and group owner messages
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
		}
		return 0;
	}

} //end namespace alc

