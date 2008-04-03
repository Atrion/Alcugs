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
	tUnetLobbyServerBase::tUnetLobbyServerBase(void) : tUnetServerBase()
	{
		memset(guid, 0, 8);
		auth_gone = tracking_gone = vault_gone = 0;
	}
	
	void tUnetLobbyServerBase::onStart(void)
	{
		auth = reconnectPeer(KAuth);
		tracking = reconnectPeer(KTracking);
		vault = reconnectPeer(KVault);
	}
	
	tNetSessionIte tUnetLobbyServerBase::reconnectPeer(Byte dst)
	{
		tStrBuf host, port, protocol;
		tConfig *cfg = alcGetConfig();
		
		switch (dst) {
			case KAuth:
				host = cfg->getVar("auth","global");
				port = cfg->getVar("auth.port","global");
				protocol = cfg->getVar("auth.protocol","global");
				break;
			case KTracking:
				host = cfg->getVar("tracking","global");
				port = cfg->getVar("tracking.port","global");
				protocol = cfg->getVar("tracking.protocol","global");
				break;
			case KVault:
				host = cfg->getVar("vault","global");
				port = cfg->getVar("vault.port","global");
				protocol = cfg->getVar("vault.protocol","global");
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
		tNetSession *session = getSession(ite);
		session->whoami = dst;
		session->conn_timeout = 5*60; // 5minutes timeout for server. must be set there as well!
		if (!protocol.isNull())
			session->proto = protocol.asU32();
		
		// send hello
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
					err->log("UNEXPECTED: Max packet size of %s is not 1024, but %d, ignoring\n", u->str(), authHello.maxPacketSize);
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
				u->x = authHello.x;
				
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
				
				// verifiy if we're still talking about the same player
				if (authResponse.x != u->x) {
					err->log("ERR: %s X values of message and session don't match.\n", u->str());
					return 1;
				}
				
				// send authAsk to auth server
				tNetSession *authServer = getPeer(KAuth);
				if (!authServer) {
					err->log("ERR: I've got to ask the auth server about player %s, but it's unavailable.\n", u->account);
					return 1;
				}
				tmCustomAuthAsk authAsk(authServer, u->sid, u->ip, u->port, u->account, u->challenge, authResponse.hash.readAll(), u->release);
				authServer->send(authAsk);
				
				return 1;
			}
			case NetMsgCustomAuthResponse:
			{
				if (u->whoami != KAuth) {
					err->log("ERR: %s sent a NetMsgCustomAuthResponse but is not the auth server\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomAuthResponse authResponse(u);
				msg->data->get(authResponse);
				log->log("<RCV> %s\n", authResponse.str());
				
				// find the client's session
				tNetSession *client = NULL;
				tNetSessionIte ite(authResponse.ip, authResponse.port, authResponse.x);
				if (u->proto != 1) { // when we're using the new protocol, we're getting IP and Port, not only the sid
					client = getSession(ite);
				}
				else { // for the old protocol, we get only the sid, so let's hope it's still the right one
					client = smgr->getSession(authResponse.x);
					ite = client->getIte();
				}
				// verify account name and session state
				if (!client || client->authenticated != 10 || client->whoami != 0 || strcmp((char *)client->account, (char *)authResponse.login.c_str()) != 0) {
					err->log("ERR: Got CustomAuthResponse for player %s but can't find his session.\n", authResponse.login.c_str());
					return 1;
				}
				
				// send NetMsgAccountAuthenticated to client
				if (authResponse.result == AAuthSucceeded) {
					tmAccountAutheticated accountAuth(client, authResponse.guid, authResponse.result, guid);
					client->send(accountAuth);
					client->whoami = KClient; // it's a real client now
					client->authenticated = 2; // the player is authenticated!
					strcpy((char *)client->passwd, (char *)authResponse.passwd.c_str()); // passwd is needed for validating packets
					client->timeout = 30; // 30sec, client should send an alive every 10sec
				}
				else {
					Byte zeroGuid[16]; // only send zero-filled GUIDs to non-authed players
					memset(zeroGuid, 0, 16);
					tmAccountAutheticated accountAuth(client, zeroGuid, authResponse.result, zeroGuid);
					client->send(accountAuth);
					terminate(ite, RNotAuthenticated);
				}
				return 1;
			}
			case NetMsgRequestMyVaultPlayerList:
				tmMsgBase requestList(u);
				msg->data->get(requestList);
				log->log("<RCV> %s\n", requestList.str());
				return 1;
		}
		return 0;
	}

} //end namespace alc
