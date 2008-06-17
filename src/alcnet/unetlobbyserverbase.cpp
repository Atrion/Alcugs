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
#include "protocol/msgparsers.h"
#include "protocol/vaultrouter.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	tUnetLobbyServerBase::tUnetLobbyServerBase(void) : tUnetServerBase()
	{
		memset(guid, 0, 8);
		name[0] = 0;
		auth_gone = tracking_gone = vault_gone = 0;
	}
	
	void tUnetLobbyServerBase::forwardPing(tmPing &ping, tNetSession *u)
	{
		if (u->whoami == KAuth || u->whoami == KTracking || u->whoami == KVault) { // we got a ping reply from one of our servers, let's forward it to the client it came from
			if (!ping.hasFlags(plNetIP)) throw txProtocolError(_WHERE("IP flag missing"));
			tNetSessionIte ite(ping.ip, ping.port, ping.hasFlags(plNetSid) ? ping.sid : -1);
			tNetSession *client = getSession(ite);
			if (client) {
				tmPing pingFwd(client, ping);
				pingFwd.unsetRouteInfo();
				send(pingFwd);
			}
		}
		else { // ok, let's forward the ping to the right server
			tNetSession *server = 0;
			switch (ping.destination) {
				case KAuth: server = getSession(auth); break;
				case KTracking: server = getSession(tracking); break;
				case KVault: server = getSession(vault); break;
				default:
					err->log("ERR: Connection to unknown service %d requested by ping\n", ping.destination);
			}
			if (server) {
				tmPing pingFwd(server, ping);
				pingFwd.setRouteInfo(u->getIte());
				send(pingFwd);
			}
		}
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
		
#ifndef _UNET2_SUPPORT
		if (!protocol.isNull() && protocol.asU32() == 1) {
			err->log("ERR: Unet2 protocol is requested for service %d (%s) but it is no longer supported\n", dst, alcUnetGetDestination(dst));
			return tNetSessionIte();
		}
#endif
		
		tNetSessionIte ite = netConnect((char *)host.c_str(), port.asU16(), 2, 0);
		tNetSession *session = getSession(ite);
		session->whoami = dst;
		if (!protocol.isNull())
			session->proto = protocol.asU32();
		
		// send hello
		tmAlive alive(session);
		send(alive);
		
		if (dst == KTracking) {
			tStrBuf var = cfg->getVar("public_address");
			if (var.isNull()) log->log("WARNING: No public address set, using bind address %s\n", bindaddr);
			tmCustomSetGuid setGuid(session, guid, name, var.c_str());
			send(setGuid);
		}
		
		return ite;
	}
	
	void tUnetLobbyServerBase::onConnectionClosed(tNetEvent *ev, tNetSession */*u*/)
	{	// if it was one of the servers, save the time it went (it will be reconnected 10sec later)
		if (ev->sid == auth) { auth_gone = alcGetTime(); auth = tNetSessionIte(); }
		else if (ev->sid == tracking) { tracking_gone = alcGetTime(); tracking = tNetSessionIte(); }
		else if (ev->sid == vault) { vault_gone = alcGetTime(); vault = tNetSessionIte(); }
	}
	
	void tUnetLobbyServerBase::onIdle(bool idle)
	{
		if (!isRunning()) return;
		
		if (auth_gone && auth_gone+10 < alcGetTime()) { // if it went more than 10sec ago, reconnect
			auth = reconnectPeer(KAuth);
			auth_gone = 0;
		}
		
		if (tracking_gone && tracking_gone+10 < alcGetTime()) { // if it went more than 10sec ago, reconnect
			tracking = reconnectPeer(KTracking);
			tracking_gone = 0;
		}
		
		if (vault_gone && vault_gone+10 < alcGetTime()) { // if it went more than 10sec ago, reconnect
			vault = reconnectPeer(KVault);
			vault_gone = 0;
		}
	}
	
	int tUnetLobbyServerBase::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(ev, msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			//// messages regarding authentication
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
				strcpy((char *)u->name, (char *)authHello.account.c_str());
				memcpy(u->challenge, md5buffer.read(16), 16);
				u->release = authHello.release;
				u->x = authHello.x;
				u->ki = authHello.ki;
				
				// reply with AuthenticateChallenge
				tmAuthenticateChallenge authChallenge(u, result, u->challenge);
				u->authenticated = 10; // the challenge was sent
				send(authChallenge);
				
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
				u->x = authResponse.x;
				u->ki = authResponse.ki;
				
				// send authAsk to auth server
				tNetSession *authServer = getSession(auth);
				if (!authServer) {
					err->log("ERR: I've got to ask the auth server about player %s, but it's unavailable.\n", u->str());
					return 1;
				}
				tmCustomAuthAsk authAsk(authServer, u->sid, u->ip, u->port, u->name, u->challenge, authResponse.hash.readAll(), u->release);
				send(authAsk);
				
				return 1;
			}
			case NetMsgCustomAuthResponse:
			{
				if (u->whoami != KAuth) {
					err->log("ERR: %s sent a NetMsgCustomAuthResponse but is not the auth server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomAuthResponse authResponse(u);
				msg->data->get(authResponse);
				log->log("<RCV> %s\n", authResponse.str());
				
				// find the client's session
				tNetSessionIte ite(authResponse.ip, authResponse.port, authResponse.x);
#ifdef _UNET2_SUPPORT
				tNetSession *client = NULL;
				if (u->proto != 1) { // when we're using the new protocol, we're getting IP and Port, not only the sid
					client = getSession(ite);
				}
				else { // for the old protocol, we get only the sid, so let's hope it's still the right one
					client = smgr->get(authResponse.x);
					ite = client->getIte();
				}
#else
				tNetSession *client = getSession(ite);
#endif
				// verify account name and session state
				if (!client || client->authenticated != 10 || client->whoami != 0 || strncmp((char *)client->name, (char *)authResponse.login.c_str(), 199) != 0) {
					err->log("ERR: Got CustomAuthResponse for player %s but can't find his session.\n", authResponse.login.c_str());
					return 1;
				}
				
				// send NetMsgAccountAuthenticated to client
				if (authResponse.result == AAuthSucceeded) {
					memcpy(client->guid, authResponse.guid, 16);
					client->whoami = KClient; // it's a real client now
					client->authenticated = 2; // the player is authenticated!
					client->accessLevel = authResponse.accessLevel;
					strcpy((char *)client->passwd, (char *)authResponse.passwd.c_str()); // passwd is needed for validating packets
					client->conn_timeout = 30; // 30sec, client should send an alive every 10sec
					tmAccountAutheticated accountAuth(client, authResponse.result, guid);
					send(accountAuth);
				}
				else {
					Byte zeroGuid[8]; // only send zero-filled GUIDs to non-authed players
					memset(zeroGuid, 0, 8);
					memset(client->guid, 0, 16);
					tmAccountAutheticated accountAuth(client, authResponse.result, zeroGuid);
					send(accountAuth);
					terminate(ite, RNotAuthenticated);
				}
				return 1;
			}
			
			//// messages regarding setting the avatar
			case NetMsgSetMyActivePlayer:
			{
				if (u->whoami != KClient) {
					err->log("ERR: %s sent a NetMsgSetMyActivePlayer but is not yet authed. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSetMyActivePlayer setPlayer(u);
				msg->data->get(setPlayer);
				log->log("<RCV> %s\n", setPlayer.str());
				
				if (u->accessLevel <= AcAdmin)
					u->ki = setPlayer.ki;
				// FIXME: do more here
				
				return 1;
			}
			
			//// vault messages
			case NetMsgVault:
			{
				// get the data out of the packet
				tmVault vaultMsg(u);
				msg->data->get(vaultMsg);
				log->log("<RCV> %s\n", vaultMsg.str());
				
				// parse the message
				tvMessage parsedMsg(/*isTask:*/false);
				vaultMsg.message.rewind();
				vaultMsg.message.get(parsedMsg);
				
				if (u->whoami == KVault) { // got it from the vault - send it to the client
					if (!vaultMsg.hasFlags(plNetKi) || vaultMsg.ki == 0) throw txProtocolError(_WHERE("KI missing"));
					tNetSession *client = smgr->find(vaultMsg.ki);
					if (!client) {
						err->log("ERR: I've got a vault message to forward to player with KI %d but can\'t find it\'s session.\n", vaultMsg.ki);
						return 1;
					}
					tmVault vaultMsgFwd(client, vaultMsg.ki, &parsedMsg);
					send(vaultMsgFwd);
				}
				else { // got it from a client
					if (u->whoami != KClient || u->ki == 0) {
						err->log("ERR: %s sent a NetMsgVault but is not yet authed or did not set his ki. I\'ll kick him.\n", u->str());
						return -2; // hack attempt
					}
					// forward it to the vault server
					tNetSession *vaultServer = getSession(vault);
					if (!vaultServer) {
						err->log("ERR: I've got a vault message to forward to the vault server, but it's unavailable.\n", u->str());
						return 1;
					}
					tmVault vaultMsgFwd(vaultServer, u->ki, &parsedMsg);
					send(vaultMsgFwd);
				}
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
