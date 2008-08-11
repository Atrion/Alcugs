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
	
	void tUnetLobbyServerBase::onLoadConfig(bool reload)
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
	
	void tUnetLobbyServerBase::setActivePlayer(tNetSession *u, U32 ki, const Byte *avatar)
	{
		tNetSession *client;
		smgr->rewind();
		while ((client = smgr->getNext())) {
			if (client->whoami == KClient && client->ki == ki) { // an age cannot host the same avatar multiple times
				if (u != client) // it could be the same session for which the active player is set twice for some reason
					terminate(client, RLoggedInElsewhere);
				else
					lerr->log("Active player is set twice for %s\n", u->str());
				break;
			}
		}
		
		if (whoami == KGame && avatar[0] == 0) // empty avatar names are not allowed in game server
			throw txProtocolError(_WHERE("Someone with KI %d is trying to set an empty avatar name, but I\'m a game server. Kick him.", ki));
	
		tNetSession *trackingServer = getSession(tracking), *vaultServer = getSession(vault);
		if (!trackingServer || !vaultServer) {
			err->log("ERR: I've got to set player %s active, but tracking or vault is unavailable.\n",
					u->str());
			return;
		}
		
		// tell vault and taracking
		tmCustomPlayerStatus trackingStatus(trackingServer, ki, u->sid, u->uid, u->name, avatar, 2 /* visible */, whoami == KGame ? RJoining : RActive);
		send(trackingStatus);
		tmCustomVaultPlayerStatus vaultStatus(vaultServer, ki, u->sid, alcGetStrGuid(serverGuid), serverName, 1 /* is online */, 0 /* don't increase online time now, do that on disconnect */);
		send(vaultStatus);
		
		// now, tell the client
		u->ki = ki;
		tmActivePlayerSet playerSet(u);
		send(playerSet);
	}
	
	void tUnetLobbyServerBase::terminate(tNetSession *u, Byte reason, bool destroyOnly)
	{
		if (u->whoami == KClient && u->ki != 0) { // if necessary, tell the others about it
			tNetSession *trackingServer = getSession(tracking), *vaultServer = getSession(vault);
			if (!trackingServer || !vaultServer) {
				err->log("ERR: I've got to update a player\'s (%s) status for the tracking and vault server, but one of them is unavailable.\n", u->str());
				return;
			}
			
			tmCustomPlayerStatus trackingStatus(trackingServer, u->ki, u->sid, u->uid, u->name, (Byte *)"", 0 /* delete */, RStopResponding);
			send(trackingStatus);
			
			Byte state = u->inRoute; // if he's in route, tell the vault he'd be online... this message just servers to update the online timer
			// we only tell the vault he's offline if he leaves without asking for another age before
			// this way, the vault can remove the vmgrs for this player when he crashes, without making problems when he just links
			tmCustomVaultPlayerStatus vaultStatus(vaultServer, u->ki, u->sid, (Byte *)"0000000000000000" /* these are 16 zeroes */, (Byte *)"", state, u->onlineTime());
			send(vaultStatus);
			
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
		tNetSessionIte ite = netConnect((char *)host.c_str(), port.asU16(), (proto == 0 || proto >= 3) ? 3 : 2, 0);
		tNetSession *session = getSession(ite);
		session->whoami = dst;
		session->proto = proto;
		
		// send hello
		tmAlive alive(session);
		send(alive);
		
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
		
		if (auth_gone && auth_gone+5 < alcGetTime()) { // if it went more than 5 sec ago, reconnect
			auth = reconnectPeer(KAuth);
			auth_gone = 0;
		}
		
		if (tracking_gone && tracking_gone+5 < alcGetTime()) { // if it went more than 5 sec ago, reconnect
			tracking = reconnectPeer(KTracking);
			tracking_gone = 0;
		}
		
		if (vault_gone && vault_gone+5 < alcGetTime()) { // if it went more than 5 sec ago, reconnect
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
					err->log("UNX: Max packet size of %s is not 1024, but %d, ignoring\n", u->str(), authHello.maxPacketSize);
					return 1;
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
#ifdef ENABLE_UNET2
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
					memcpy(client->uid, authResponse.uid, 16);
					client->whoami = KClient; // it's a real client now
					client->authenticated = 2; // the player is authenticated!
					client->accessLevel = authResponse.accessLevel;
					strcpy((char *)client->passwd, (char *)authResponse.passwd.c_str()); // passwd is needed for validating packets
					client->conn_timeout = 30; // 30sec, client should send an alive every 10sec
					tmAccountAutheticated accountAuth(client, authResponse.result, serverGuid);
					send(accountAuth);
					sec->log("%s successful login\n", client->str());
				}
				else {
					Byte zeroGuid[8]; // only send zero-filled GUIDs to non-authed players
					memset(zeroGuid, 0, 8);
					memset(client->uid, 0, 16);
					tmAccountAutheticated accountAuth(client, authResponse.result, zeroGuid);
					send(accountAuth);
					sec->log("%s failed login\n", client->str());
					terminate(client, RNotAuthenticated);
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
				
				if (u->accessLevel <= AcAdmin) {
					setActivePlayer(u, setPlayer.ki, setPlayer.avatar.c_str());
				}
				else {
					// ask the vault server about this KI
					tNetSession *vaultServer = getSession(vault);
					if (!vaultServer) {
						err->log("ERR: I've got the ask the vault to verify a KI, but it's unavailable. I'll have to kick the player.\n", u->str());
						// kick the player since we cant be sure he doesnt lie about the KI
						return -1; // parse error
					}
					u->delayMessages = true; // dont process any further messages till we verified the KI
					tmCustomVaultCheckKi checkKi(vaultServer, setPlayer.ki, u->getSid(), u->uid);
					send(checkKi);
				}
				
				return 1;
			}
			case NetMsgCustomVaultKiChecked:
			{
				if (u->whoami != KVault) {
					err->log("ERR: %s sent a NetMsgCustomVaultKiChecked but is not the vault server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomVaultKiChecked kiChecked(u);
				msg->data->get(kiChecked);
				log->log("<RCV> %s\n", kiChecked.str());
				
				// find the client's session
				tNetSession *client = smgr->get(kiChecked.x);
				// verify GUID and session state
				if (!client || client->getPeerType() != KClient || u->ki != 0 || memcmp(client->uid, kiChecked.uid, 16) != 0) {
					err->log("ERR: Got NetMsgCustomVaultKiChecked for player with UID %s but can't find his session.\n", alcGetStrUid(kiChecked.uid));
					return 1;
				}
				
				client->delayMessages = false; // KI is checked, so we can process messages
				if (kiChecked.status != 1) { // the avatar is NOT correct - kick the player
					terminate(client, RNotAuthenticated);
					return 1;
				}
				
				// it is correct, so tell everyone about it
				setActivePlayer(client, kiChecked.ki, kiChecked.avatar.c_str());
				
				return 1;
			}
			
			//// vault messages
			case NetMsgVault:
			case NetMsgVault2: // TPOTS
			{
				if (msg->cmd == NetMsgVault) u->tpots = 2; // it's not TPOTS
				else                         u->tpots = 1; // it is TPOTS
				
				// get the data out of the packet
				tmVault vaultMsg(u);
				msg->data->get(vaultMsg);
				log->log("<RCV> %s\n", vaultMsg.str());
				
				// prepare for parsing the message (actual parsing is only done when the packet is really forwarded
				tvMessage parsedMsg(/*isTask:*/false, u->tpots);
				vaultMsg.message.rewind();
				
				if (u->whoami == KVault) { // got it from the vault - send it to the client
					if (!vaultMsg.hasFlags(plNetKi) || vaultMsg.ki == 0) throw txProtocolError(_WHERE("KI missing"));
					
					vaultMsg.message.get(parsedMsg);
					
					tNetSession *client = smgr->find(vaultMsg.ki);
					if (!client || client->whoami != KClient) {
						lvault->print("<h2 style='color:red'>Packet for unknown client</h2>\n");
						parsedMsg.print(lvault, /*clientToServer:*/false, NULL, vaultLogShort);
						if ((parsedMsg.cmd == VOnlineState && parsedMsg.vmgr == vaultMsg.ki) || parsedMsg.cmd == VDisconnect)
							// this is most likely the message that this player went offline
							// (vault sends one to lobby after the client already left to connect to the game server)
							// so silently ignore it
							;
						else
							err->log("ERR: I've got a vault message to forward to player with KI %d but can\'t find it\'s session.\n", vaultMsg.ki);
						return 1;
					}
					parsedMsg.print(lvault, /*clientToServer:*/false, client, vaultLogShort);
					parsedMsg.tpots = client->tpots;
					tmVault vaultMsgFwd(client, vaultMsg.ki, &parsedMsg);
					send(vaultMsgFwd);
				}
				else { // got it from a client
					if (vaultMsg.hasFlags(plNetKi) && vaultMsg.ki != u->ki)
						throw txProtocolError(_WHERE("KI mismatch (%d != %d)", vaultMsg.ki, u->ki));
					if (u->whoami != KClient || u->ki == 0) { // KI is necessary to know where to route it
						err->log("ERR: %s sent a NetMsgVault but is not yet authed or did not set his KI. I\'ll kick him.\n", u->str());
						return -2; // hack attempt
					}
					// forward it to the vault server
					tNetSession *vaultServer = getSession(vault);
					if (!vaultServer) {
						err->log("ERR: I've got a vault message to forward to the vault server, but it's unavailable.\n", u->str());
						return 1;
					}
					vaultMsg.message.get(parsedMsg);
					parsedMsg.print(lvault, /*clientToServer:*/true, u, vaultLogShort);
					parsedMsg.tpots = vaultServer->tpots;
					tmVault vaultMsgFwd(vaultServer, u->ki, &parsedMsg);
					send(vaultMsgFwd);
				}
				
				return 1;
			}
			
			// messages for finding the server to link to
			case NetMsgFindAge:
			{
				if (u->whoami != KClient || u->ki == 0) {
					err->log("ERR: %s sent a NetMsgVault but is not yet authed or did not set his KI. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmFindAge findAge(u);
				msg->data->get(findAge);
				log->log("<RCV> %s\n", findAge.str());
				
				tvAgeLinkStruct ageLink;
				findAge.message.rewind();
				findAge.message.get(ageLink);
				if (!findAge.message.eof()) throw txProtocolError(_WHERE("Got a NetMsgFindAge which is too long"));
				log->print(" %s\n", ageLink.str());
				
				if (ageLink.linkingRule != KOriginalBook && ageLink.linkingRule != KOwnedBook)
					throw txProtocolError(_WHERE("Linking rule must be KOriginalBook or KOwnedBook but is 0x%02X", ageLink.linkingRule));
				if (ageLink.ccr)
					throw txProtocolError(_WHERE("Linking as CCR is not allowed"));
				
				// let's ask the tracking server
				tNetSession *trackingServer = getSession(tracking);
				if (!trackingServer) {
					err->log("ERR: I've got the ask the tracking server to find an age for me, but it's unavailable.\n");
					return 1;
				}
				
				tmCustomFindServer findServer(trackingServer, u->ki, u->getSid(), u->getIp(), u->getPort(), alcGetStrGuid(ageLink.ageInfo.guid), ageLink.ageInfo.filename.c_str());
				send(findServer);
				
				return 1;
			}
			case NetMsgCustomServerFound:
			{
				if (u->whoami != KTracking) {
					err->log("ERR: %s sent a NetMsgCustomServerFound but is not the tracking server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomServerFound serverFound(u);
				msg->data->get(serverFound);
				log->log("<RCV> %s\n", serverFound.str());
				
				// find the client
				tNetSession *client = smgr->get(serverFound.x);
				if (!client || client->whoami != KClient || client->ki != serverFound.ki) {
					err->log("ERR: I've got to tell player with KI %d about his game server, but can't find his session.\n", serverFound.ki);
					return 1;
				}
				Byte guid[8];
				alcAscii2Hex(guid, serverFound.serverGuid.c_str(), 8);
				tmFindAgeReply reply(client, serverFound.ipStr, serverFound.serverPort, serverFound.age, guid);
				send(reply);
				
				client->inRoute = true; // we told the palyer where to connect, now he'll leave... but don't tell vault and tracking, he'll soon come back!
				
				return 1;
			}
			
			// terminating a player
			case NetMsgPlayerTerminated:
			{
				if (u->whoami != KTracking && u->whoami != KVault) {
					err->log("ERR: %s sent a NetMsgPlayerTerminated but is neither tracking nor vault server. I\'ll kick him.\n", u->str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmPlayerTerminated playerTerminated(u);
				msg->data->get(playerTerminated);
				log->log("<RCV> %s\n", playerTerminated.str());
				
				tNetSession *client = smgr->find(playerTerminated.ki);
				if (!client || (client->whoami != KClient && client->whoami != 0)) {
					err->log("ERR: I've got to kick the player with KI %d but can\'t find it\'s session.\n", playerTerminated.ki);
					return 1;
				}
				
				terminate(client, playerTerminated.reason);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
