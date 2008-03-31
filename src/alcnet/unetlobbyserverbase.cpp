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

/* CVS tag - DON'T TOUCH*/
#define __U_UNETLOBBYSERVERBASE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"
#include "unetlobbyserverbase.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	void tUnetLobbyServerBase::onStart(void)
	{
		auth_gone = tracking_gone = vault_gone = 0;
		auth = reconnectPeer(KAuth);
		tracking = reconnectPeer(KTracking);
		vault = reconnectPeer(KVault);
	}
	
	tNetSessionIte tUnetLobbyServerBase::reconnectPeer(Byte dst)
	{
		tStrBuf host, port;
		tConfig *cfg = alcGetConfig();
		
		switch (dst) {
			case KAuth:
				host = cfg->getVar("auth","global");
				port = cfg->getVar("auth.port","global");
				break;
			case KTracking:
				host = cfg->getVar("tracking","global");
				port = cfg->getVar("tracking.port","global");
				break;
			case KVault:
				host = cfg->getVar("vault","global");
				port = cfg->getVar("vault.port","global");
				break;
			default:
				err->log("ERR: Connection to unknown service %d requested\n", dst);
				return tNetSessionIte();
		}
		if (host.isNull() || port.isNull()) {
			err->log("ERR: Hostname or port for service %d (%s) is missing\n", dst, alcUnetGetDestination(dst));
			return tNetSessionIte();
		}
		
		tNetSessionIte ite = netConnect((char *)host.c_str(), port.asU16(), 2, 0);
		
		// send hello
		tNetSession *session = getSession(ite);
		tmAlive alive(session);
		session->send(alive);
		
		return ite;
	}
	
	tNetSession *tUnetLobbyServerBase::getPeer(Byte dst)
	{
		tNetSession *s = NULL;
		switch (dst) {
			case KAuth:
				s = getSession(auth);
				if (s) return s;
				auth = reconnectPeer(KAuth);
				auth_gone = 0;
				return getSession(auth);
				break;
			case KTracking:
				s = getSession(tracking);
				if (s) return s;
				tracking = reconnectPeer(KTracking);
				tracking_gone = 0;
				return getSession(tracking);
				break;
			case KVault:
				s = getSession(vault);
				if (s) return s;
				vault = reconnectPeer(KVault);
				vault_gone = 0;
				return getSession(vault);
				break;
			default:
				err->log("ERR: Connection to unknown service %d requested\n", dst);
				return NULL;
		}
	}
	
	void tUnetLobbyServerBase::onConnectionClosed(tNetEvent *ev, tNetSession */*u*/)
	{	// if it was one of the servers, save the time it went (it will be reconnected 10sec later)
		if (ev->sid == auth) auth_gone = alcGetTime();
		else if (ev->sid == tracking) tracking_gone = alcGetTime();
		else if (ev->sid == vault) vault_gone = alcGetTime();
	}
	
	void tUnetLobbyServerBase::onIdle(bool idle)
	{
		if (!isRunning()) return;
		
		if (auth_gone && auth_gone+10 < alcGetTime()) { // if it went more than 10sec ago, reconnect
			auth = reconnectPeer(KAuth);
			auth_gone = 0;
			DBG(5, "reconnecting auth\n");
		}
		
		if (tracking_gone && tracking_gone+10 < alcGetTime()) { // if it went more than 10sec ago, reconnect
			tracking = reconnectPeer(KTracking);
			tracking_gone = 0;
			DBG(5, "reconnecting tracking\n");
		}
		
		if (vault_gone && vault_gone+10 < alcGetTime()) { // if it went more than 10sec ago, reconnect
			vault = reconnectPeer(KVault);
			vault_gone = 0;
			DBG(5, "reconnecting vault\n");
		}
	}

} //end namespace alc
