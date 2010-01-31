/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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

#ifndef __U_UNETLOBBYSERVERBASE_H
#define __U_UNETLOBBYSERVERBASE_H
/* CVS tag - DON'T TOUCH*/
#define __U_UNETLOBBYSERVERBASE_H_ID "$Id$"

#include "protocol/ext-protocol.h"

namespace alc {

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/

class tUnetLobbyServerBase : public tUnetServerBase {
public:
	tUnetLobbyServerBase(void);
	virtual void terminateAll();
	
	inline const Byte *getGuid() { return serverGuid; }
	inline const char *getName() { return serverName; }
protected:
	virtual void onStart(void);
	virtual void onIdle(bool idle);
	virtual void onConnectionClosed(tNetSession *u);
	virtual int onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u);
	virtual void forwardPing(tmPing &ping, tNetSession *u);
	virtual void terminate(tNetSession *u, Byte reason = 0, bool gotLeave = false);
	virtual void onUnloadConfig(void);
	virtual void onLoadConfig(void);

	virtual void additionalVaultProcessing(tNetSession */*u*/, tvMessage */*msg*/) {}
	virtual void playerAuthed(tNetSession */*u*/) {}
	
	tNetSession *getServer(Byte dst);

	Byte serverGuid[8]; //!< This system's guid (age guid) (in Hex)
	char serverName[200]; //!< The system/server name, normally the age filename

protected:
	U16 spawnStart, spawnStop;
	const U32 authedTimeout;

private:
	bool setActivePlayer(tNetSession *u, U32 ki, U32 x, const char *avatar);
	tNetSessionIte reconnectPeer(Byte dst); //!< establishes a connection to that service (remember to set the corresponding gone variable to 0)

	tNetSessionIte authIte, trackingIte, vaultIte;
	U32 auth_gone, tracking_gone, vault_gone; // saves when this server got disconnected. wait 10sec before trying to connect again

	tLog *lvault;
	bool vaultLogShort;

	bool allowUU;
	U32 loadingTimeout;
	char linkLog[512];
};
	
} //End alc namespace

#endif
