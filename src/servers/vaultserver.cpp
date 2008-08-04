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
				log->log("(ignored) <RCV> %s\n", status.str());
				
				vaultBackend->updatePlayerStatus(status);
				
				return 1;
			}
			case NetMsgVault:
			{
				// get the data out of the packet
				tmVault vaultMsg(u);
				msg->data->get(vaultMsg);
				log->log("<RCV> %s\n", vaultMsg.str());
				if (!vaultMsg.hasFlags(plNetKi) || vaultMsg.ki == 0) throw txProtocolError(_WHERE("KI missing"));
				
				// prepare for parsing the message (actual parsing is only done when the packet is really forwarded
				tvMessage parsedMsg(/*isTask:*/false, /* 0 = non-TPOTS */(Byte)0);
				vaultMsg.message.rewind();
				vaultMsg.message.get(parsedMsg);
				
				try {
					vaultBackend->processVaultMsg(parsedMsg, u, vaultMsg.ki);
				}
				catch (txProtocolError &t) { // don't kick the lobby/game server we are talking to but let it kick the client
					err->log("%s Recieved invalid vault message from player %d\n", u->str(), vaultMsg.ki);
					err->log(" Exception details: %s\n%s\n",t.what(),t.backtrace());
					tmPlayerTerminated term(u, vaultMsg.ki, RParseError);
					send(term);
				}
				
				return 1;
			}
		}
		
		return 0;
	}

} //end namespace alc

