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
#include "protocol/lobbymsg.h"
#include "protocol/authmsg.h"

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
	
	int tUnetLobbyServerBase::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(ev, msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgAuthenticateHello:
			{
				if (u->whoami != 0 || u->authenticated != 0) { // this is impossible
					err->log("ERR: %s player is already being authend and sent another AuthenticateHello, ignoring\n", u->str());
					return 1;
				}
				
				// get the data out of the packet
				tmAuthenticateHello authHello(u);
				msg->data->get(authHello);
				log->log("<RCV> %s\n", authHello.str());
				
				if (authHello.maxPacketSize != 1024) {
					unx->log("UNEXPECTED: Max packet size of %s is not 1024, but %d, ignoring\n", u->str(), authHello.maxPacketSize);
					return 1;
				}
				
				// determine auth result
				int result = AAuthHello;
				if (u->max_version < 12) result = AProtocolNewer; // servers are newer
				else if (u->max_version > 12) result = AProtocolOlder; // servers are newer
				else if (u->min_version > 7) result = AProtocolOlder; // servers are newer
				else if (u->min_version != 6) u->tpots = 2; // it's not TPOTS
				
				// init the challenge to the MD5 of the current system time and other garbage
				tMD5Buf md5buffer;
				md5buffer.putU32(alcGetTime());
				md5buffer.putU32(alcGetMicroseconds());
				srandom(alcGetTime());
				md5buffer.putU32(random());
				md5buffer.putU32(alcGetUptime().seconds);
				md5buffer.put(authHello.account);
				md5buffer.putU32(alcGetBornTime().seconds);
				md5buffer.compute();
		
				// save data in session
				strcpy((char *)u->account, (char *)authHello.account.c_str());
				memcpy(u->challenge, md5buffer.read(16), 16);
				u->release = authHello.release;
				
				// reply with AuthenticateChallenge
				tmAuthenticateChallenge authChallenge(u, result, u->challenge, authHello);
				u->send(authChallenge);
				u->authenticated = 10; // the challenge was sent
				
				return 1;
			}
			case NetMsgAuthenticateResponse:
			{
				if (u->whoami != 0 || u->authenticated != 10) { // this is impossible
					err->log("ERR: %s player sent an AuthenticateResponse and he is already being authend or he didn\'t yet send an AuthenticateHello, ignoring\n", u->str());
					return 1;
				}
				
				// get the data out of the packet
				tmAuthenticateResponse authResponse(u);
				msg->data->get(authResponse);
				log->log("<RCV> %s\n", authResponse.str());
				
				// send authAsk to auth server
				tNetSession *s = getPeer(KAuth);
				if (!s) return 1;
				DBG(1, "%s %s\n", alcGetStrGuid(u->challenge, 16), alcGetStrGuid(authResponse.hash.readAll(), 16));
				tmCustomAuthAsk authAsk(s, u->ip, u->port, u->account, u->challenge, authResponse.hash.readAll(), u->release);
				s->send(authAsk);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
