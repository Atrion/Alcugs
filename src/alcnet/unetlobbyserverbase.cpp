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
#include "alcnet.h"
#include "protocol/msgparsers.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	tUnetLobbyServerBase::tUnetLobbyServerBase(void) : tUnetServerBase()
	{
		memset(serverGuid, 0, 8);
		serverName[0] = 0;
		auth_gone = tracking_gone = vault_gone = 0;
		lvault = lnull;
		vaultLogShort = false;
	}
	
	void tUnetLobbyServerBase::onLoadConfig(void)
	{
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("vault.html.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			if (lvault == lnull) lvault = new tLog("vault.html", 2, DF_HTML);
			var = cfg->getVar("vault.html.log.short");
			vaultLogShort = (!var.isNull() && var.asByte()); // per default, it's not short
		}
		var = cfg->getVar("allow_uu_clients");
		allowUU = (var.isNull() || var.asByte()); // per default, UU clients are allowed
		var = cfg->getVar("tmp.link_log");
		if (!var.isNull()) strncpy(linkLog, (char *)var.c_str(), 511);
		else linkLog[0] = 0;
	}
	
	void tUnetLobbyServerBase::onUnloadConfig()
	{
		if (lvault != lnull) {
			delete lvault;
			lvault = lnull;
		}
	}
	
	void tUnetLobbyServerBase::forwardPing(tmPing &ping, tNetSession *u)
	{
		if (u->getPeerType() == KAuth || u->getPeerType() == KTracking || u->getPeerType() == KVault) { // we got a ping reply from one of our servers, let's forward it to the client it came from
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
	
	bool tUnetLobbyServerBase::setActivePlayer(tNetSession *u, U32 ki, U32 x, const Byte *avatar)
	{
		tNetSession *client;
		smgr->rewind();
		while ((client = smgr->getNext())) {
			if (client->getPeerType() == KClient && client->ki == ki) { // an age cannot host the same avatar multiple times
				if (u != client) // it could be the same session for which the active player is set twice for some reason
					terminate(client, RLoggedInElsewhere);
				else
					lerr->log("Active player is set twice for %s\n", u->str());
				break;
			}
		}
		
		if (whoami == KGame && avatar[0] == 0) // empty avatar names are not allowed in game server
			throw txProtocolError(_WHERE("Someone with KI %d is trying to set an empty avatar name, but I\'m a game server. Kick him.", ki));
	
		tNetSession *trackingServer = getSession(tracking);
		if (!trackingServer) {
			err->log("ERR: I've got to set player %s active, but tracking is unavailable.\n", u->str());
			return false;
		}
		
		// save the data
		strncpy((char *)u->avatar, (char *)avatar, 199);
		u->ki = ki;
		
		// tell tracking
		tmCustomPlayerStatus trackingStatus(trackingServer, u->ki, u->getSid(), u->uid, u->name, u->avatar, 2 /* visible */, (u->release == TIntRel) ? RActive : RJoining); // show the VaultManager as active (it's the only IntRel we have)
		send(trackingStatus);
		
		// now, tell the client
		tmActivePlayerSet playerSet(u, x);
		send(playerSet);
		// and write to the logfile
		sec->log("%s player set\n", u->str());
		return true;
	}
	
	void tUnetLobbyServerBase::terminate(tNetSession *u, Byte reason, bool destroyOnly)
	{
		if (u->getPeerType() == KClient && u->ki != 0) { // if necessary, tell the others about it
			tNetSession *trackingServer = getSession(tracking);
			if (!trackingServer) {
				err->log("ERR: I've got to update a player\'s (%s) status for the tracking server, but it is unavailable.\n", u->str());
			}
			else {
				int state = (reason == RLeaving) ? 2 /* visible */ : 0 /* delete */; // if the player just goes on to another age, don't remove him from the list
				tmCustomPlayerStatus trackingStatus(trackingServer, u->ki, u->getSid(), u->uid, u->name, u->avatar, state, reason);
				send(trackingStatus);
			}
			u->ki = 0; // this avoids sending the messages twice
		}
	
		tUnetServerBase::terminate(u, reason, destroyOnly); // do the common terminate procedure
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
		
#ifndef ENABLE_UNET2
		if (!protocol.isNull() && protocol.asU32() == 1) {
			err->log("ERR: Unet2 protocol is requested for service %d (%s) but it is no longer supported\n", dst, alcUnetGetDestination(dst));
			return tNetSessionIte();
		}
#endif
		
		U32 proto = protocol.isNull() ? 0 : protocol.asU32();
		tNetSessionIte ite = netConnect((char *)host.c_str(), port.asU16(), (proto == 0 || proto >= 3) ? 3 : 2, 0, dst);
		tNetSession *session = getSession(ite);
		session->proto = proto;
		
		// sending a NetMsgAlive is not necessary, the netConnect will already start the negotiation process
		
		if (dst == KTracking) {
			tStrBuf var = cfg->getVar("public_address");
			if (var.isNull()) log->log("WARNING: No public address set, using bind address %s\n", bindaddr);
			tmCustomSetGuid setGuid(session, alcGetStrGuid(serverGuid), serverName, var.c_str());
			send(setGuid);
		}
		
		return ite;
	}
	
	void tUnetLobbyServerBase::onConnectionClosed(tNetEvent *ev, tNetSession */*u*/)
	{
		// if it was one of the servers, save the time it went (it will be reconnected later)
		if (ev->sid == auth) { auth_gone = alcGetTime(); auth = tNetSessionIte(); }
		else if (ev->sid == tracking) { tracking_gone = alcGetTime(); tracking = tNetSessionIte(); }
		else if (ev->sid == vault) { vault_gone = alcGetTime(); vault = tNetSessionIte(); }
	}
	
	void tUnetLobbyServerBase::onIdle(bool idle)
	{
		if (!isRunning()) return;
		
		U32 time = alcGetTime();
		if (auth_gone && auth_gone+5 < time) { // if it went more than 5 sec ago, reconnect
			auth = reconnectPeer(KAuth);
			auth_gone = 0;
		}
		
		if (tracking_gone && tracking_gone+5 < time) { // if it went more than 5 sec ago, reconnect
			tracking = reconnectPeer(KTracking);
			tracking_gone = 0;
		}
		
		if (vault_gone && vault_gone+5 < time) { // if it went more than 5 sec ago, reconnect
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
				if (u->getPeerType() != 0 || u->getAuthenticated() != 0) { // this is impossible
					err->log("ERR: %s player is already being authend and sent another AuthenticateHello, ignoring\n", u->str());
					return 2; // ignore, leave it unparsed
				}
				
				// get the data out of the packet
				tmAuthenticateHello authHello(u);
				msg->data->get(authHello);
				log->log("<RCV> [%d] %s\n", msg->sn, authHello.str());
				
				if (authHello.maxPacketSize != u->getMaxPacketSz()) {
					err->log("UNX: Max packet size of %s is not %d, but %d, ignoring\n", u->str(), u->getMaxPacketSz(), authHello.maxPacketSize);
					return 1; // it was already parsed, so we can return 1
				}
				
				// determine auth result
				int result = AAuthHello;
				if (u->max_version < 12) result = AProtocolNewer; // servers are newer
				else if (u->max_version > 12) result = AProtocolOlder; // servers are older
				else if (u->min_version > 7) result = AProtocolOlder; // servers are older
				else if (u->min_version != 6) {
					u->tpots = 2; // it's not TPOTS
					// block UU if we're told to do so
					if (!allowUU && u->min_version == 7) // it's UU, and we are told not to allow that, so tell him the servers are older
						result = AProtocolOlder;
				}
				
				// init the challenge to the MD5 of the current system time and other garbage
				tMD5Buf md5buffer;
				md5buffer.putU32(alcGetTime());
				md5buffer.putU32(alcGetMicroseconds());
				md5buffer.putU32(random());
				md5buffer.putU32(alcGetUptime().seconds);
				md5buffer.put(authHello.account);
				md5buffer.putU32(alcGetBornTime().seconds);
				md5buffer.compute();
		
				// save data in session
				strcpy((char *)u->name, (char *)authHello.account.c_str());
				memcpy(u->challenge, md5buffer.read(16), 16);
				u->release = authHello.release;
				
				// reply with AuthenticateChallenge
				tmAuthenticateChallenge authChallenge(u, authHello.x, result, u->challenge);
				send(authChallenge);
				u->challengeSent();
				
				return 1;
			}
			case NetMsgAuthenticateResponse:
			{
				if (u->getPeerType() != 0 || u->getAuthenticated() != 10) { // this is impossible
					err->log("ERR: %s player sent an AuthenticateResponse and he is already being authend or he didn\'t yet send an AuthenticateHello, ignoring\n", u->str());
					return 2; // ignore, leave it unparsed
				}
				
				// get the data out of the packet
				tmAuthenticateResponse authResponse(u);
				msg->data->get(authResponse);
				log->log("<RCV> [%d] %s\n", msg->sn, authResponse.str());
				
				// send authAsk to auth server
				tNetSession *authServer = getSession(auth);
				if (!authServer) {
					err->log("ERR: I've got to ask the auth server about player %s, but it's unavailable.\n", u->str());
					return 1;
				}
				tmCustomAuthAsk authAsk(authServer, authResponse.x, u->getSid(), u->getIp(), u->getPort(), u->name, u->challenge, authResponse.hash.readAll(), u->release);
				send(authAsk);
#ifdef ENABLE_UNET3
				// perhaps the server does not preserve the X
				u->x = authResponse.x;
#endif
				
				return 1;
			}
			case NetMsgCustomAuthResponse:
			{
				if (u->getPeerType() != KAuth) {
					err->log("ERR: %s sent a NetMsgCustomAuthResponse but is not the auth server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomAuthResponse authResponse(u);
				msg->data->get(authResponse);
				log->log("<RCV> [%d] %s\n", msg->sn, authResponse.str());
				
				// find the client's session
				tNetSession *client = smgr->get(authResponse.sid);
				if (u->proto != 1 && client) { // check if IP and Port are correct
					if (client->getIp() != authResponse.ip || client->getPort() != authResponse.port) {
						err->log("ERR: Got CustomAuthResponse for player %s but can't find his session.\n", authResponse.login.c_str());
						return 1;
					}
				}
				// verify account name and session state
				if (!client || client->getAuthenticated() != 10 || client->getPeerType() != 0 || strncmp((char *)client->name, (char *)authResponse.login.c_str(), 199) != 0) {
					err->log("ERR: Got CustomAuthResponse for player %s but can't find his session.\n", authResponse.login.c_str());
					return 1;
				}
#ifdef ENABLE_UNET3
				if (u->proto == 1 || u->proto == 2) // the server does not preserve the X
					authResponse.x = client->x;
#endif
				
				// send NetMsgAccountAuthenticated to client
				if (authResponse.result == AAuthSucceeded) {
					memcpy(client->uid, authResponse.uid, 16);
					client->setAuthData(authResponse.accessLevel, authResponse.passwd.c_str());
					
					tmAccountAutheticated accountAuth(client, authResponse.x, AAuthSucceeded, serverGuid);
					send(accountAuth);
					sec->log("%s successful login\n", client->str());
				}
				else {
					Byte zeroGuid[8]; // only send zero-filled GUIDs to non-authed players
					memset(zeroGuid, 0, 8);
					memset(client->uid, 0, 16);
					tmAccountAutheticated accountAuth(client, authResponse.x, authResponse.result, zeroGuid);
					send(accountAuth);
					sec->log("%s failed login\n", client->str());
					terminate(client, RNotAuthenticated);
				}
				return 1;
			}
			
			//// messages regarding setting the avatar
			case NetMsgSetMyActivePlayer:
			{
				if (u->getPeerType() != KClient) {
					err->log("ERR: %s sent a NetMsgSetMyActivePlayer but is not yet authed. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSetMyActivePlayer setPlayer(u);
				msg->data->get(setPlayer);
				log->log("<RCV> [%d] %s\n", msg->sn, setPlayer.str());
				
				if (u->ki != 0) {
					if (u->ki == setPlayer.ki) {
						log->log("%s set the same KI twice, ignoring the second try\n", u->str());
						return 1;
					}
					else {
						err->log("ERR: %s sent a NetMsgSetMyActivePlayer but already set a different KI. I\'ll kick him.\n", u->str());
						return -2; // hack attempt
					}
				}
				
				if (u->getAccessLevel() <= AcAdmin) {
					setActivePlayer(u, setPlayer.ki, setPlayer.x, setPlayer.avatar.c_str());
				}
				else {
					// ask the vault server about this KI
					tNetSession *vaultServer = getSession(vault);
					if (!vaultServer) {
						err->log("ERR: I've got the ask the vault to verify a KI, but it's unavailable. I'll have to kick the player.\n", u->str());
						// kick the player since we cant be sure he doesnt lie about the KI
						return -1; // parse error
					}
					u->setDelayMessages(true); // dont process any further messages till we verified the KI
					tmCustomVaultCheckKi checkKi(vaultServer, setPlayer.ki, setPlayer.x, u->getSid(), u->uid);
					send(checkKi);
#ifdef ENABLE_UNET3
					// perhaps the server does not preserve the X
					u->x = setPlayer.x;
#endif
				}
				
				return 1;
			}
			case NetMsgCustomVaultKiChecked:
			{
				if (u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgCustomVaultKiChecked but is not the vault server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomVaultKiChecked kiChecked(u);
				msg->data->get(kiChecked);
				log->log("<RCV> [%d] %s\n", msg->sn, kiChecked.str());
				
				// find the client's session
				tNetSession *client = smgr->get(kiChecked.sid);
				// verify GUID and session state
				if (!client || client->getPeerType() != KClient || u->ki != 0 || memcmp(client->uid, kiChecked.uid, 16) != 0) {
					err->log("ERR: Got NetMsgCustomVaultKiChecked for player with UID %s but can't find his session.\n", alcGetStrUid(kiChecked.uid));
					return 1;
				}
				
				client->setDelayMessages(false); // KI is checked, so we can process messages
				if (kiChecked.status != 1) { // the avatar is NOT correct - kick the player
					terminate(client, RNotAuthenticated);
					return 1;
				}
#ifdef ENABLE_UNET3
				if (u->proto == 1 || u->proto == 2) // the server does not preserve the X
					kiChecked.x = client->x;
#endif
				
				// it is correct, so tell everyone about it
				setActivePlayer(client, kiChecked.ki, kiChecked.x, kiChecked.avatar.c_str());
				
				return 1;
			}
			
			//// vault messages
			case NetMsgVault:
			case NetMsgVaultTask:
			case NetMsgVault2: // TPOTS
			{
				bool isTask = (msg->cmd == NetMsgVaultTask);
				if (msg->cmd == NetMsgVault)       u->tpots = 2; // it's not TPOTS
				else if (msg->cmd == NetMsgVault2) u->tpots = 1; // it is TPOTS
				
				// get the data out of the packet
				tmVault vaultMsg(u);
				msg->data->get(vaultMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, vaultMsg.str());
				if (isTask) {
					if (!vaultMsg.hasFlags(plNetX))  throw txProtocolError(_WHERE("X flag missing"));
				}
				else vaultMsg.x = 0; // make sure it is set
				
				// prepare for parsing the message (actual parsing is only done when the packet is really forwarded)
				tvMessage parsedMsg(/*isTask:*/isTask, u->tpots);
				vaultMsg.message.rewind();
				
				if (u->getPeerType() == KVault) { // got it from the vault - send it to the client
					if (!vaultMsg.hasFlags(plNetKi) || vaultMsg.ki == 0) throw txProtocolError(_WHERE("KI missing"));
					
					vaultMsg.message.get(parsedMsg);
					
					tNetSession *client = smgr->find(vaultMsg.ki);
					if (!client || client->getPeerType() != KClient) {
						lvault->print("<h2 style='color:red'>Packet for unknown client</h2>\n");
						parsedMsg.print(lvault, /*clientToServer:*/false, NULL, vaultLogShort);
						err->log("ERR: I've got a vault message to forward to player with KI %d but can\'t find it\'s session.\n", vaultMsg.ki);
						return 1;
					}
					parsedMsg.print(lvault, /*clientToServer:*/false, client, vaultLogShort);
					parsedMsg.tpots = client->tpots;
					tmVault vaultMsgFwd(client, vaultMsg.ki, vaultMsg.x, isTask, &parsedMsg);
					send(vaultMsgFwd);
				}
				else { // got it from a client
					if (!u->ki) { // KI is necessary to know where to route it
						err->log("ERR: %s sent a vault message but did not yet set his KI. I\'ll kick him.\n", u->str());
						return -2; // hack attempt
					}
					if (vaultMsg.hasFlags(plNetKi) && vaultMsg.ki != u->ki)
						throw txProtocolError(_WHERE("KI mismatch (%d != %d)", vaultMsg.ki, u->ki));
					// forward it to the vault server
					tNetSession *vaultServer = getSession(vault);
					if (!vaultServer) {
						err->log("ERR: I've got a vault message to forward to the vault server, but it's unavailable.\n", u->str());
						return 1;
					}
					vaultMsg.message.get(parsedMsg);
					parsedMsg.print(lvault, /*clientToServer:*/true, u, vaultLogShort);
					parsedMsg.tpots = vaultServer->tpots;
					tmVault vaultMsgFwd(vaultServer, u->ki, vaultMsg.x, isTask, &parsedMsg);
					send(vaultMsgFwd);
				}
				
				return 1;
			}
			
			// messages for finding the server to link to
			case NetMsgFindAge:
			{
				if (!u->ki) {
					err->log("ERR: %s sent a NetMsgFindAge but did not yet set his KI. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmFindAge findAge(u);
				msg->data->get(findAge);
				log->log("<RCV> [%d] %s\n", msg->sn, findAge.str());
				
				tvAgeLinkStruct ageLink;
				findAge.message.rewind();
				findAge.message.get(ageLink);
				if (!findAge.message.eof()) throw txProtocolError(_WHERE("Got a NetMsgFindAge which is too long"));
				log->print(" %s\n", ageLink.str());
				
				if (ageLink.linkingRule != KOriginalBook && ageLink.linkingRule != KOwnedBook && ageLink.linkingRule != KBasicLink && ageLink.linkingRule != KVisitBook)
					throw txProtocolError(_WHERE("Linking rule must be KOriginalBook, KOwnedBook, KVisitBook or KBasicLink but is 0x%02X", ageLink.linkingRule));
				if (ageLink.ccr)
					throw txProtocolError(_WHERE("Linking as CCR is not allowed"));
				
				// if asked to do so, log the linking
				if (linkLog[0]) {
					FILE *f = fopen(linkLog, "a");
					if (f) {
						fprintf(f, "Player %s links from %s to: %s\n", u->name, serverName, ageLink.str());
						fclose(f);
					}
				}
				
				// let's ask the tracking server
				tNetSession *trackingServer = getSession(tracking);
				if (!trackingServer) {
					err->log("ERR: I've got the ask the tracking server to find an age for me, but it's unavailable.\n");
					return 1;
				}
				
				tmCustomFindServer findServer(trackingServer, u->ki, findAge.x, u->getSid(), u->getIp(), u->getPort(), alcGetStrGuid(ageLink.ageInfo.guid), ageLink.ageInfo.filename.c_str());
				send(findServer);
#ifdef ENABLE_UNET3
				// perhaps the server does not preserve the X
				u->x = findAge.x;
#endif
				
				return 1;
			}
			case NetMsgCustomServerFound:
			{
				if (u->getPeerType() != KTracking) {
					err->log("ERR: %s sent a NetMsgCustomServerFound but is not the tracking server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomServerFound serverFound(u);
				msg->data->get(serverFound);
				log->log("<RCV> [%d] %s\n", msg->sn, serverFound.str());
				
				// find the client
				tNetSession *client = smgr->get(serverFound.sid);
				if (!client || client->getPeerType() != KClient || client->ki != serverFound.ki) {
					err->log("ERR: I've got to tell player with KI %d about his game server, but can't find his session.\n", serverFound.ki);
					return 1;
				}
				Byte guid[8];
				alcAscii2Hex(guid, serverFound.serverGuid.c_str(), 8);
				
#ifdef ENABLE_UNET3
				if (u->proto == 1 || u->proto == 2) // the server does not preserve the X
					serverFound.x = client->x;
#endif
				tmFindAgeReply reply(client, serverFound.x, serverFound.ipStr, serverFound.serverPort, serverFound.age, guid);
				send(reply);
				
				return 1;
			}
			
			// terminating a player
			case NetMsgPlayerTerminated:
			{
				if (u->getPeerType() != KTracking && u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgPlayerTerminated but is neither tracking nor vault server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmPlayerTerminated playerTerminated(u);
				msg->data->get(playerTerminated);
				log->log("<RCV> [%d] %s\n", msg->sn, playerTerminated.str());
				
				tNetSession *client = smgr->find(playerTerminated.ki);
				if (!client || (client->getPeerType() != KClient && client->getPeerType() != 0)) {
					err->log("ERR: I've got to kick the player with KI %d but can\'t find his session.\n", playerTerminated.ki);
					return 1;
				}
				
				terminate(client, playerTerminated.reason);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
