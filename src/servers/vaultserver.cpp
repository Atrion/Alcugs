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
#define __U_VAULTSERVER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/vaultmsg.h>
#include <protocol/vaultproto.h>

////extra includes
#include "vaultserver.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	const char * alcNetName="Vault";
	Byte alcWhoami=KVault;
	
	void tUnetVaultServer::onLoadConfig(void)
	{
		// check if we should clean the vault
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("daemon");
		bool daemon = (!var.isNull() && var.asByte()); // disabled per default
		var = cfg->getVar("vault.clean", "cmdline"); // this can only be enabled via cmdline
		bool clean = (!var.isNull() && var.asByte()); // disabled per default
		if (clean) {
			if (daemon) {
				lerr->log("I will only clean the vault in interactive mode to give you more feedback\n");
				return;
			}
			var = cfg->getVar("vault.clean.ages");
			bool cleanAges = !var.isNull() && var.asByte(); // disabled per default
			vaultBackend->cleanVault(cleanAges);
			forcestop(); // don't let the server run, we started just for cleaning
		}
	}
	
	bool tUnetVaultServer::isValidAvatarName(tUStr &avatar)
	{
		for (U32 i = 0; i < avatar.size(); ++i) {
			Byte c = avatar.getAt(i);
			if (!isprint(c) && !isalpha(c) && !alcIsAlpha(c)) return false;
			if (c == '\n' || c == '\t') return false;
		}
		return true;
	}
	
	int tUnetVaultServer::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(ev, msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgCustomVaultAskPlayerList:
			{
				tmCustomVaultAskPlayerList askPlayerList(u);
				msg->data.get(askPlayerList);
				log->log("<RCV> [%d] %s\n", msg->sn, askPlayerList.str());
				
				vaultBackend->sendPlayerList(askPlayerList);
				
				return 1;
			}
			case NetMsgCustomVaultCreatePlayer:
			{
				tmCustomVaultCreatePlayer createPlayer(u);
				msg->data.get(createPlayer);
				log->log("<RCV> [%d] %s\n", msg->sn, createPlayer.str());
				
				Byte result = AUnspecifiedServerError;
				U32 ki = 0;
				int num = 0;
				if (createPlayer.accessLevel > AcCCR) num = vaultBackend->getNumberOfPlayers(createPlayer.uid);
				if (num >= vaultBackend->getMaxPlayers()) result = AMaxNumberPerAccountReached;
				else if (createPlayer.avatar.size() < 3) result = ANameIsTooShort;
				else if (createPlayer.avatar.size() > 20) result = ANameIsTooLong;
				else if (createPlayer.friendName.size() > 0 || createPlayer.key.size() > 0) result = AInvitationNotFound;
				else if (!isValidAvatarName(createPlayer.avatar)) result = ANameIsNotAllowed;
				else {
					tStrBuf gender = createPlayer.gender.lower();
					if (gender != "male" && gender != "female" && createPlayer.accessLevel > AcCCR) {
						if (gender == "yeesha" || gender == "yeeshanoglow" || gender == "shuterland") createPlayer.gender = "Female";
						else createPlayer.gender = "Male";
					}
					ki = vaultBackend->createPlayer(createPlayer);
					if (ki == 0) result = ANameIsAlreadyInUse;
					else result = AOK;
				}
				
				tmCustomVaultPlayerCreated playerCreated(u, ki, createPlayer.x, createPlayer.sid, createPlayer.uid, result);
				send(playerCreated);
				
				return 1;
			}
			case NetMsgCustomVaultDeletePlayer:
			{
				tmCustomVaultDeletePlayer deletePlayer(u);
				msg->data.get(deletePlayer);
				log->log("<RCV> [%d] %s\n", msg->sn, deletePlayer.str());
				
				vaultBackend->deletePlayer(deletePlayer);
				
				return 1;
			}
			case NetMsgCustomVaultCheckKi:
			{
				tmCustomVaultCheckKi checkKi(u);
				msg->data.get(checkKi);
				log->log("<RCV> [%d] %s\n", msg->sn, checkKi.str());
				
				vaultBackend->checkKi(checkKi);
				
				return 1;
			}
			case NetMsgCustomVaultPlayerStatus:
			{
				tmCustomVaultPlayerStatus status(u);
				msg->data.get(status);
				log->log("<RCV> [%d] %s\n", msg->sn, status.str());
				
				vaultBackend->updatePlayerStatus(status);
				
				return 1;
			}
			case NetMsgVault:
			case NetMsgVaultTask:
			{
				bool isTask = (msg->cmd == NetMsgVaultTask);
				tvMessage parsedMsg(isTask, /* 0 = non-TPOTS */(Byte)0);
				
				// get the data out of the packet
				tmVault vaultMsg(u);
				msg->data.get(vaultMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, vaultMsg.str());
				if (!vaultMsg.hasFlags(plNetKi) || vaultMsg.ki == 0) throw txProtocolError(_WHERE("KI missing"));
				if (isTask && !vaultMsg.hasFlags(plNetX))  throw txProtocolError(_WHERE("X flag missing"));
				// parse the vault stuff
				vaultMsg.message.rewind();
				vaultMsg.message.get(parsedMsg);
				
				try {
					if (isTask) vaultBackend->processVaultTask(parsedMsg, u, vaultMsg.ki, vaultMsg.x);
					else vaultBackend->processVaultMsg(parsedMsg, u, vaultMsg.ki);
				}
				catch (txBase &t) { // don't kick the lobby/game server we are talking to but let it kick the client
					err->log("%s Recieved invalid vault message from player %d\n", u->str(), vaultMsg.ki);
					err->log(" Exception details: %s\n",t.what());
					tmPlayerTerminated term(u, vaultMsg.ki, RParseError);
					send(term);
				}
				
				return 1;
			}
		}
		
		return 0;
	}

} //end namespace alc
