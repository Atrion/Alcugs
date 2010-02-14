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

#include <alcnet.h>

////extra includes
#include "vaultserver.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	void tUnetVaultServer::onApplyConfig(void)
	{
		// check if we should clean the vault
		tConfig *cfg = alcGetMain()->config();
		tString var = cfg->getVar("daemon");
		bool daemon = (!var.isEmpty() && var.asByte()); // disabled per default
		var = cfg->getVar("vault.clean", "cmdline"); // this can only be enabled via cmdline
		bool clean = (!var.isEmpty() && var.asByte()); // disabled per default
		if (clean) {
			if (daemon) {
				alcGetMain()->err()->log("I will only clean the vault in interactive mode to give you more feedback\n");
				return;
			}
			var = cfg->getVar("vault.clean.ages");
			bool cleanAges = !var.isEmpty() && var.asByte(); // disabled per default
			vaultBackend.cleanVault(cleanAges);
			forcestop(); // don't let the server run, we started just for cleaning
			return;
		}
		// (re)load vault backend
		vaultBackend.applyConfig();
	}
	
	bool tUnetVaultServer::isValidAvatarName(const tString &avatar)
	{
		for (U32 i = 0; i < avatar.size(); ++i) {
			Byte c = avatar.getAt(i);
			if (!isprint(c) && !isalpha(c) && !alcIsAlpha(c)) return false;
			if (c == '\n' || c == '\t') return false;
		}
		return true;
	}
	
	int tUnetVaultServer::onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgCustomVaultAskPlayerList:
			{
				tmCustomVaultAskPlayerList askPlayerList(u);
				msg->data.get(askPlayerList);
				log->log("<RCV> [%d] %s\n", msg->sn, askPlayerList.str().c_str());
				
				vaultBackend.sendPlayerList(askPlayerList);
				
				return 1;
			}
			case NetMsgCustomVaultCreatePlayer:
			{
				tmCustomVaultCreatePlayer createPlayer(u);
				msg->data.get(createPlayer);
				log->log("<RCV> [%d] %s\n", msg->sn, createPlayer.str().c_str());
				
				Byte result = AUnspecifiedServerError;
				U32 ki = 0;
				int num = 0;
				if (createPlayer.accessLevel > AcMod) num = vaultBackend.getNumberOfPlayers(createPlayer.uid);
				if (num >= vaultBackend.getMaxPlayers()) result = AMaxNumberPerAccountReached;
				else if (createPlayer.avatar.size() < 3) result = ANameIsTooShort;
				else if (createPlayer.avatar.size() > 20) result = ANameIsTooLong;
				else if (createPlayer.friendName.size() > 0 || createPlayer.key.size() > 0) result = AInvitationNotFound;
				else if (!isValidAvatarName(createPlayer.avatar)) result = ANameIsNotAllowed;
				else {
					tString gender = createPlayer.gender.lower();
					if (gender != "male" && gender != "female" && createPlayer.accessLevel > AcCCR) {
						if (gender == "yeesha" || gender == "yeeshanoglow" || gender == "shuterland") createPlayer.gender = "Female";
						else createPlayer.gender = "Male";
					}
					ki = vaultBackend.createPlayer(createPlayer);
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
				log->log("<RCV> [%d] %s\n", msg->sn, deletePlayer.str().c_str());
				
				vaultBackend.deletePlayer(deletePlayer);
				
				return 1;
			}
			case NetMsgCustomVaultCheckKi:
			{
				tmCustomVaultCheckKi checkKi(u);
				msg->data.get(checkKi);
				log->log("<RCV> [%d] %s\n", msg->sn, checkKi.str().c_str());
				
				vaultBackend.checkKi(checkKi);
				
				return 1;
			}
			case NetMsgCustomVaultFindAge:
			{
				tmCustomVaultFindAge findAge(u);
				msg->data.get(findAge);
				log->log("<RCV> [%d] %s\n", msg->sn, findAge.str().c_str());
				
				tvAgeLinkStruct ageLink;
				findAge.data.rewind();
				findAge.data.get(ageLink);
				if (!findAge.data.eof()) throw txProtocolError(_WHERE("Got a NetMsgFindAge which is too long"));
				log->print(" %s\n", ageLink.str().c_str());
				
				if (!ageLink.ageInfo.hasGuid()) {
					if (!vaultBackend.setAgeGuid(&ageLink, findAge.ki)) {
						err->log("ERR: Request to link to unknown age %s - kicking player %d\n", ageLink.ageInfo.filename.c_str(), findAge.ki);
						tmPlayerTerminated term(u, findAge.ki, RKickedOff);
						send(term);
						return 1;
					}
				}
				
				tmCustomFindServer findServer(u, findAge, alcGetStrGuid(ageLink.ageInfo.guid), ageLink.ageInfo.filename);
				send(findServer);
				
				return 1;
			}
			case NetMsgCustomVaultPlayerStatus:
			{
				tmCustomVaultPlayerStatus status(u);
				msg->data.get(status);
				log->log("<RCV> [%d] %s\n", msg->sn, status.str().c_str());
				
				vaultBackend.updatePlayerStatus(status);
				
				return 1;
			}
			case NetMsgVault:
			case NetMsgVaultTask:
			{
				bool isTask = (msg->cmd == NetMsgVaultTask);
				tvMessage parsedMsg(isTask, /* 0 = non-TPOTS */0);
				
				// get the data out of the packet
				tmVault vaultMsg(u);
				msg->data.get(vaultMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, vaultMsg.str().c_str());
				if (!vaultMsg.hasFlags(plNetKi) || vaultMsg.ki == 0) throw txProtocolError(_WHERE("KI missing"));
				if (isTask && !vaultMsg.hasFlags(plNetX))  throw txProtocolError(_WHERE("X flag missing"));
				// parse the vault stuff
				vaultMsg.message.rewind();
				vaultMsg.message.get(parsedMsg);
				
				try {
					if (isTask) vaultBackend.processVaultTask(parsedMsg, u, vaultMsg.ki, vaultMsg.x);
					else vaultBackend.processVaultMsg(parsedMsg, u, vaultMsg.ki);
				}
				catch (txBase &t) { // don't kick the lobby/game server we are talking to but let it kick the client
					err->log("%s Recieved invalid vault message from player %d\n", u->str().c_str(), vaultMsg.ki);
					err->log(" Exception %s\n%s\n",t.what(),t.backtrace());
					tmPlayerTerminated term(u, vaultMsg.ki, RParseError);
					send(term);
				}
				
				return 1;
			}
		}
		
		return 0;
	}

} //end namespace alc

