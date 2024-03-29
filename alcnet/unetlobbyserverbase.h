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

#ifndef __U_UNETLOBBYSERVERBASE_H
#define __U_UNETLOBBYSERVERBASE_H

#include "netsessionmgr.h"
#include "unetserverbase.h"

namespace alc {
	
	class tvMessage;

class tUnetLobbyServerBase : public tUnetServerBase {
public:
	tUnetLobbyServerBase(uint8_t whoami);
	
	const uint8_t *getGuid() { return serverGuid; }
	const tString &getName() { return serverName; }
protected:
	virtual void onStart(void);
	virtual int onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u);
	virtual void onForwardPing(tmPing &ping, tNetSession *u);
	virtual void onConnectionClosing(tNetSession *u, uint8_t reason);
	virtual void onApplyConfig(void);

	/** This event is triggered when a vault message is forwarded to or from the vault server */
	virtual void onVaultMessageForward(tNetSession */*u*/, tvMessage */*msg*/) {}
	
	/** This event is triggered when a player authenticated itself against the server */
	virtual void onPlayerAuthed(tNetSession */*u*/) {}

	uint8_t serverGuid[8]; //!< This system's guid (age guid) (in Hex)
	tString serverName; //!< The system/server name, normally the age filename

protected:
	uint16_t spawnStart, spawnStop;
	const time_t authedTimeout;
	
	tNetSessionRef authServer, trackingServer, vaultServer; //!< only accessed in worker, and never NULL (will immediately re-connect on disconnect)
	tNetSession::tGameType allowedGames; //!< controls which kinds of clients are allowed to log in: UU, TPOTS or both
private:
	bool setActivePlayer(tNetSession *u, uint32_t ki, uint32_t x, const tString &avatar);
	tNetSessionRef connectPeer(uint8_t dst); //!< establishes a connection to that service

	tLog lvault;
	bool vaultLogShort;

	time_t loadingTimeout;
	tString linkLog;
};
	
} //End alc namespace

#endif
