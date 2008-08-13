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
	
	int tUnetVaultServer::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(ev, msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgCustomVaultAskPlayerList:
			{
				tmCustomVaultAskPlayerList askPlayerList(u);
				msg->data->get(askPlayerList);
				log->log("<RCV> %s\n", askPlayerList.str());
				
				vaultBackend->sendPlayerList(askPlayerList);
				
				return 1;
			}
			case NetMsgCustomVaultCreatePlayer:
			{
				tmCustomVaultCreatePlayer createPlayer(u);
				msg->data->get(createPlayer);
				log->log("<RCV> %s\n", createPlayer.str());
				
				Byte result = AUnspecifiedServerError;
				U32 ki = 0;
				int num = 0;
				if (createPlayer.accessLevel > AcCCR) num = vaultBackend->getNumberOfPlayers(createPlayer.uid);
				// FIXME: check for max. number of players
				if (createPlayer.avatar.len() < 3) result = ANameIsTooShort;
				else if (createPlayer.avatar.len() > 20) result = ANameIsTooLong;
				else if (createPlayer.friendName.len() > 0 || createPlayer.key.len() > 0) result = AInvitationNotFound;
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
				
				tmCustomVaultPlayerCreated playerCreated(u, ki, createPlayer.x, createPlayer.uid, result);
				send(playerCreated);
				
				return 1;
			}
			case NetMsgCustomVaultDeletePlayer:
			{
				tmCustomVaultDeletePlayer deletePlayer(u);
				msg->data->get(deletePlayer);
				log->log("<RCV> %s\n", deletePlayer.str());
				
				vaultBackend->deletePlayer(deletePlayer);
				
				return 1;
			}
			case NetMsgCustomVaultCheckKi:
			{
				tmCustomVaultCheckKi checkKi(u);
				msg->data->get(checkKi);
				log->log("<RCV> %s\n", checkKi.str());
				
				vaultBackend->checkKi(checkKi);
				
				return 1;
			}
			case NetMsgCustomVaultPlayerStatus:
			{
				tmCustomVaultPlayerStatus status(u);
				msg->data->get(status);
				log->log("<RCV> %s\n", status.str());
				
				vaultBackend->updatePlayerStatus(status);
				
				return 1;
			}
			case NetMsgVault:
			case NetMsgVaultTask:
			{
				bool isTask = (msg->cmd == NetMsgVaultTask);
				tvMessage parsedMsg(isTask, /* 0 = non-TPOTS */(Byte)0);
				U32 ki, x;
				
				if (isTask) {
					// get the data out of the packet
					tmVaultTask vaultTask(u);
					msg->data->get(vaultTask);
					log->log("<RCV> %s\n", vaultTask.str());
					if (!vaultTask.hasFlags(plNetKi) || vaultTask.ki == 0) throw txProtocolError(_WHERE("KI missing"));
					ki = vaultTask.ki;
					x = vaultTask.x;
					// parse the vault stuff
					vaultTask.message.rewind();
					vaultTask.message.get(parsedMsg);
				}
				else {
					// get the data out of the packet
					tmVault vaultMsg(u);
					msg->data->get(vaultMsg);
					log->log("<RCV> %s\n", vaultMsg.str());
					if (!vaultMsg.hasFlags(plNetKi) || vaultMsg.ki == 0) throw txProtocolError(_WHERE("KI missing"));
					ki = vaultMsg.ki;
					// parse the vault stuff
					vaultMsg.message.rewind();
					vaultMsg.message.get(parsedMsg);
				}
				
				try {
					if (isTask) vaultBackend->processVaultTask(parsedMsg, u, ki, x);
					else vaultBackend->processVaultMsg(parsedMsg, u, ki);
				}
				catch (txProtocolError &t) { // don't kick the lobby/game server we are talking to but let it kick the client
					err->log("%s Recieved invalid vault message from player %d\n", u->str(), ki);
					err->log(" Exception details: %s\n%s\n",t.what(),t.backtrace());
					tmPlayerTerminated term(u, ki, RParseError);
					send(term);
				}
				
				return 1;
			}
		}
		
		return 0;
	}

} //end namespace alc

