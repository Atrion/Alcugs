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

/* CVS tag - DON'T TOUCH*/
#define __U_UNETLOBBYSERVERBASE_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "unetlobbyserverbase.h"

#include "protocol/umsgbasic.h"
#include "protocol/trackingmsg.h"
#include "protocol/lobbybasemsg.h"
#include "protocol/authmsg.h"
#include "protocol/vaultmsg.h"
#include "protocol/vaultproto.h"
#include "netexception.h"
#include <alcmain.h>

#include <cstring>


namespace alc {

	tUnetLobbyServerBase::tUnetLobbyServerBase(uint8_t whoami) : tUnetServerBase(whoami), authedTimeout(30) /* 30seconds for authenticated clients */
	{
		memset(serverGuid, 0, 8);
		vaultLogShort = false;
	}
	
	void tUnetLobbyServerBase::onApplyConfig(void)
	{
		tConfig *cfg = alcGetMain()->config();
		tString var = cfg->getVar("vault.html.log");
		if (var.isEmpty() || var.asUInt()) { // logging enabled per default
			lvault.open("vault.html", DF_HTML);
			var = cfg->getVar("vault.html.log.short");
			vaultLogShort = (var.isEmpty() || var.asUInt()); // per default, it *is* short
		}
		else
			lvault.open(DF_HTML);
		
		var = cfg->getVar("allow_uu_clients");
		allowUU = (var.isEmpty() || var.asUInt()); // per default, UU clients are allowed
		linkLog = cfg->getVar("tmp.link_log");
		
		var = cfg->getVar("spawn.start");
		if (var.isEmpty()) spawnStart = 5001;
		else spawnStart = var.asUInt();
		
		var = cfg->getVar("spawn.stop");
		if (var.isEmpty()) spawnStop = 6000;
		else spawnStop = var.asUInt();
		
		if (spawnStop < spawnStart) {
			log->log("WARN: spawnStop (%d) lower than spawnStart (%d), setting both to %d\n", spawnStop, spawnStart, spawnStart);
			spawnStop = spawnStart;
		}
		
		var = cfg->getVar("net.timeout.loading");
		if (var.isEmpty()) loadingTimeout = 90;
		else loadingTimeout = var.asUInt();
	}
	
	void tUnetLobbyServerBase::onForwardPing(tmPing &ping, tNetSession *u)
	{
		if (u->getPeerType() == KAuth || u->getPeerType() == KTracking || u->getPeerType() == KVault) { // we got a ping reply from one of our servers, let's forward it to the client it came from
			if (!ping.hasFlags(plNetSid)) throw txProtocolError(_WHERE("SID flag missing"));
			tNetSessionRef client = sessionBySid(ping.sid);
			if (*client) {
				tmPing pingFwd(*client, ping);
				pingFwd.unsetRouteInfo();
				send(pingFwd);
			}
		}
		else { // ok, let's forward the ping to the right server
			tNetSession *server = NULL;
			switch (ping.destination) {
				case KAuth: server = *authServer; break;
				case KTracking: server = *trackingServer; break;
				case KVault: server = *vaultServer; break;
				default:
					err->log("ERR: Connection to unknown service %d requested by ping\n", ping.destination);
					return;
			}
			tmPing pingFwd(server, ping);
			pingFwd.setRouteInfo(u);
			send(pingFwd);
		}
	}
	
	bool tUnetLobbyServerBase::setActivePlayer(tNetSession *u, uint32_t ki, uint32_t x, const tString &avatar)
	{
		// worker thread only
		tNetSessionRef client = sessionByKi(ki);
		if (*client && client->getPeerType() == KClient) { // an age cannot host the same avatar multiple times
			if (u != *client) // it could be the same session for which the active player is set twice for some reason
				terminate(*client, RLoggedInElsewhere);
			else
				err->log("Active player is set twice for %s\n", u->str().c_str());
		}
		
		if (whoami == KGame && avatar.isEmpty()) // empty avatar names are not allowed in game server
			throw txProtocolError(_WHERE("Someone with KI %d is trying to set an empty avatar name, but I\'m a game server. Kick him.", ki));
		
		if (!*trackingServer) {
			err->log("ERR: I've got to set player %s active, but tracking is unavailable.\n", u->str().c_str());
			return false;
		}
		
		// save the data
		{
			tWriteLock lock(u->pubDataMutex); // don't hold this later on, it'd block the str() function!
			u->avatar = avatar;
			u->ki = ki;
		}
		
		// tell tracking
		tmCustomPlayerStatus trackingStatus(*trackingServer, u, 2 /* visible */, (u->buildType == TIntRel) ? RActive : RJoining); // show the VaultManager as active (it's the only IntRel we have)
		send(trackingStatus);
		
		// now, tell the client
		tmActivePlayerSet playerSet(u, x);
		send(playerSet);
		// and write to the logfile
		sec->log("%s player set\n", u->str().c_str());
		return true;
	}
	
	void tUnetLobbyServerBase::onNewConnection(tNetSession* u)
	{
		tMutexLock lock(serversMutex);
		switch (u->getPeerType()) {
			case KAuth: authServer = u; return;
			case KTracking: trackingServer = u; return;
			case KVault: vaultServer = u; return;
		}
	}

	void tUnetLobbyServerBase::onConnectionClosing(tNetSession *u, uint8_t reason)
	{
		{
			tMutexLock lock(serversMutex);
			// if it was one of our servers, save the time it went (it will be reconnected later)
			if (*authServer == u) {
				authServer = NULL;
				return;
			}
			else if (*trackingServer == u) {
				if (whoami == KGame && isRunning()) {
					err->log("ERR: I lost the connection to the tracking server, so I will go down\n");
					/* The game server should go down when it looses the connection to tracking. This way, you can easily
					shut down all game servers. In addition, it won't get any new peers anyway without the tracking server */
					stop();
				}
				else {
					trackingServer = NULL;
				}
				return;
			}
			else if (*vaultServer == u) {
				vaultServer = NULL;
				return;
			}
		}
		// If it was a client, tell the others it left
		if (u->getPeerType() == KClient && u->ki != 0) {
			if (!*trackingServer) {
				err->log("ERR: I've got to update a player\'s (%s) status for the tracking server, but it is unavailable.\n", u->str().c_str());
			}
			else if (reason != RLoggedInElsewhere) { // if the player went somewhere else, don't remove him from tracking
				int state = (reason == RLeaving) ? 2 /* visible */ : 0 /* delete */; // if the player just goes on to another age, don't remove him from the list
				tmCustomPlayerStatus trackingStatus(*trackingServer, u, state, reason);
				send(trackingStatus);
			}
			tWriteLock lock(u->pubDataMutex);
			u->ki = 0; // this avoids sending the messages twice
		}
	}
	
	void tUnetLobbyServerBase::onStart(void)
	{
		reconnectPeer(KAuth);
		reconnectPeer(KTracking);
		reconnectPeer(KVault);
	}
	
	void tUnetLobbyServerBase::reconnectPeer(uint8_t dst)
	{
		tString host, port;
		tConfig *cfg = alcGetMain()->config();
		
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
				return;
		}
		if (host.isEmpty() || port.isEmpty()) {
			err->log("ERR: Hostname or port for service %d (%s) is missing\n", dst, alcUnetGetDestination(dst));
			return;
		}
		
		tNetSessionRef u = netConnect(host.c_str(), port.asUInt(), 3 /* Alcugs upgraded protocol */, 0, dst);
		
		// sending a NetMsgAlive is not necessary, the netConnect will already start the negotiation process
		
		if (dst == KTracking) {
			tString var = cfg->getVar("public_address");
			if (var.isEmpty()) log->log("WARNING: No public address set, using bind address %s\n", bindaddr.c_str());
			tReadLock lock(u->pubDataMutex); // we are in main thread
			tmCustomSetGuid setGuid(*u, alcGetStrGuid(serverGuid), serverName, var, whoami == KGame ? 0 : spawnStart, whoami == KGame ? 0 : spawnStop);
			send(setGuid);
		}
	}
	
	void tUnetLobbyServerBase::onIdle()
	{
		if (!isRunning()) return;
		
		tMutexLock lock(serversMutex);
		// make sure we have a connection to the servers
		if (!*authServer) {
			reconnectPeer(KAuth);
		}
		if (!*trackingServer) {
			reconnectPeer(KTracking);
		}
		if (!*vaultServer) {
			reconnectPeer(KVault);
		}
	}
	
	int tUnetLobbyServerBase::onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			//// messages regarding authentication
			case NetMsgAuthenticateHello:
			{
				if (u->getPeerType() != 0 || u->getAuthenticated() != 0) { // this is impossible
					err->log("ERR: %s player is already being authend and sent another AuthenticateHello, ignoring\n", u->str().c_str());
					return 2; // ignore, leave it unparsed
				}
				
				// get the data out of the packet
				tmAuthenticateHello authHello(u);
				msg->data.get(authHello);
				log->log("<RCV> [%d] %s\n", msg->sn, authHello.str().c_str());
				
				if (authHello.maxPacketSize != u->getMaxPacketSz()) {
					throw txUnexpectedData(_WHERE("UNX: Max packet size of %s is not %d, but %d, ignoring\n", u->str().c_str(), u->getMaxPacketSz(), authHello.maxPacketSize));
				}
				
				// determine auth result
				int result = AAuthHello;
				{
					tWriteLock lock(u->pubDataMutex);
					if (u->max_version < 12) result = AProtocolNewer; // servers are newer
					else if (u->max_version > 12) result = AProtocolOlder; // servers are older
					else if (u->min_version > 7) result = AProtocolOlder; // servers are older
					else if (u->min_version != 6) u->tpots = 2; // it's not TPOTS
					else if (u->min_version == 6) u->tpots = 1; // it *is* TPOTS
					// block UU if we're told to do so
					if (!allowUU && u->tpots == 2) // it's UU, and we are told not to allow that, so tell him the servers are older
						result = AProtocolOlder;
					
					// init the challenge to the MD5 of the current system time and other garbage
					tTime t = tTime::now();
					tMD5Buf md5buffer;
					md5buffer.put32(t.seconds);
					md5buffer.put32(t.microseconds);
					md5buffer.put32(random());
					md5buffer.put32(alcGetMain()->upTime().seconds);
					md5buffer.put(authHello.account);
					md5buffer.put32(alcGetMain()->bornTime().seconds);
					md5buffer.compute();
			
					// save data in session
					u->name = authHello.account;
					memcpy(u->challenge, md5buffer.read(16), 16);
					u->buildType = authHello.release;
				}
				
				// reply with AuthenticateChallenge´
				tmAuthenticateChallenge authChallenge(u, authHello.x, result, u->challenge);
				send(authChallenge);
				u->challengeSent();
				
				return 1;
			}
			case NetMsgAuthenticateResponse:
			{
				if (u->getPeerType() != 0 || u->getAuthenticated() != 10) { // this is impossible
					err->log("ERR: %s player sent an AuthenticateResponse and he is already being authend or he didn\'t yet send an AuthenticateHello, ignoring\n", u->str().c_str());
					return 2; // ignore, leave it unparsed
				}
				
				// get the data out of the packet
				tmAuthenticateResponse authResponse(u);
				msg->data.get(authResponse);
				log->log("<RCV> [%d] %s\n", msg->sn, authResponse.str().c_str());
				
				// send authAsk to auth server
				if (!*authServer) {
					err->log("ERR: I've got to ask the auth server about player %s, but it's unavailable.\n", u->str().c_str());
					return 1;
				}
				tmCustomAuthAsk authAsk(*authServer, authResponse.x, u->getSid(), u->getIp(), u->name, u->challenge, authResponse.hash.data(), u->buildType);
				send(authAsk);
				
				return 1;
			}
			case NetMsgCustomAuthResponse:
			{
				if (u->getPeerType() != KAuth) {
					err->log("ERR: %s sent a NetMsgCustomAuthResponse but is not the auth server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomAuthResponse authResponse(u);
				msg->data.get(authResponse);
				log->log("<RCV> [%d] %s\n", msg->sn, authResponse.str().c_str());
				
				// find the client's session
				tNetSessionRef client = sessionBySid(authResponse.sid);
				// verify account name and session state
				if (!*client || client->getAuthenticated() != 10 || client->getPeerType() != 0 || client->name != authResponse.login) {
					err->log("ERR: Got CustomAuthResponse for player %s but can't find his session.\n", authResponse.login.c_str());
					return 1;
				}
				
				// send NetMsgAccountAuthenticated to client
				if (authResponse.result == AAuthSucceeded) {
					{
						tWriteLock lock(client->pubDataMutex);
						memcpy(client->uid, authResponse.uid, 16);
						client->setAuthData(authResponse.accessLevel, authResponse.passwd);
						client->setTimeout(loadingTimeout); // use higher timeout - the client might be in the lobby (waiting for the user to work with the GUI) or loading an age
					}
					
					tmAccountAutheticated accountAuth(*client, authResponse.x, AAuthSucceeded, serverGuid);
					send(accountAuth);
					sec->log("%s successful login\n", client->str().c_str());
					onPlayerAuthed(*client);
				}
				else {
					uint8_t zeroGuid[8]; // only send zero-filled GUIDs to non-authed players
					memset(zeroGuid, 0, 8);
					tmAccountAutheticated accountAuth(*client, authResponse.x, authResponse.result, zeroGuid);
					send(accountAuth);
					sec->log("%s failed login\n", client->str().c_str());
					terminate(*client, RNotAuthenticated);
				}
				return 1;
			}
			
			//// messages regarding setting the avatar
			case NetMsgSetMyActivePlayer:
			{
				if (u->getPeerType() != KClient) {
					err->log("ERR: %s sent a NetMsgSetMyActivePlayer but is not yet authed. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmSetMyActivePlayer setPlayer(u);
				msg->data.get(setPlayer);
				log->log("<RCV> [%d] %s\n", msg->sn, setPlayer.str().c_str());
				
				if (u->ki != 0) {
					if (u->ki == setPlayer.ki) {
						log->log("%s set the same KI twice, ignoring the second try\n", u->str().c_str());
						return 1;
					}
					else {
						err->log("ERR: %s sent a NetMsgSetMyActivePlayer but already set a different KI. I\'ll kick him.\n", u->str().c_str());
						return -2; // hack attempt
					}
				}
				
				if (u->getAccessLevel() <= AcAdmin) {
					setActivePlayer(u, setPlayer.ki, setPlayer.x, setPlayer.avatar); // dont hold the public data lock when calling this!
				}
				else {
					// ask the vault server about this KI
					if (!*vaultServer) {
						err->log("ERR: I've got the ask the vault to verify a KI, but it's unavailable. I'll have to kick the player.\n", u->str().c_str());
						// kick the player since we cant be sure he doesnt lie about the KI
						return -1; // parse error
					}
					u->setRejectMessages(true); // dont process any further messages till we verified the KI
					tmCustomVaultCheckKi checkKi(*vaultServer, setPlayer.ki, setPlayer.x, u->getSid(), u->uid);
					send(checkKi);
				}
				
				return 1;
			}
			case NetMsgCustomVaultKiChecked:
			{
				if (u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgCustomVaultKiChecked but is not the vault server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomVaultKiChecked kiChecked(u);
				msg->data.get(kiChecked);
				log->log("<RCV> [%d] %s\n", msg->sn, kiChecked.str().c_str());
				
				// find the client's session
				tNetSessionRef client = sessionBySid(kiChecked.sid);
				// verify GUID and session state
				if (!*client || client->getPeerType() != KClient || u->ki != 0 || memcmp(client->uid, kiChecked.uid, 16) != 0) {
					err->log("ERR: Got NetMsgCustomVaultKiChecked for player with UID %s but can't find his session.\n", alcGetStrUid(kiChecked.uid).c_str());
					return 1;
				}
				
				client->setRejectMessages(false); // KI is checked, so we can process messages
				if (kiChecked.status != 1) { // the avatar is NOT correct - kick the player
					terminate(*client, RNotAuthenticated);
					return 1;
				}
				
				// it is correct, so tell everyone about it
				setActivePlayer(*client, kiChecked.ki, kiChecked.x, kiChecked.avatar);
				
				return 1;
			}
			
			//// vault messages
			case NetMsgVault:
			case NetMsgVaultTask:
			{
				bool isTask = (msg->cmd == NetMsgVaultTask);
				
				// get the data out of the packet
				tmVault vaultMsg(u);
				msg->data.get(vaultMsg);
				log->log("<RCV> [%d] %s\n", msg->sn, vaultMsg.str().c_str());
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
					
					tNetSessionRef client = sessionByKi(vaultMsg.ki);
					if (!*client || client->getPeerType() != KClient) {
						lvault.print("<h2 style='color:red'>Packet for unknown client</h2>\n");
						parsedMsg.print(&lvault, /*clientToServer:*/false, NULL, vaultLogShort);
						log->log("WARN: I've got a vault message to forward to player with KI %d but can\'t find the session.\n", vaultMsg.ki);
						return 1;
					}
					parsedMsg.print(&lvault, /*clientToServer:*/false, *client, vaultLogShort);
					// do additional processing
					onVaultMessageForward(u, &parsedMsg);
					// send it on to client
					parsedMsg.tpots = client->tpots;
					tmVault vaultMsgFwd(*client, vaultMsg.ki, vaultMsg.x, isTask, &parsedMsg);
					send(vaultMsgFwd);
				}
				else { // got it from a client
					if (!u->ki) { // KI is necessary to know where to route it
						err->log("ERR: %s sent a vault message but did not yet set his KI. I\'ll kick him.\n", u->str().c_str());
						return -2; // hack attempt
					}
					if (vaultMsg.hasFlags(plNetKi) && vaultMsg.ki != u->ki)
						throw txProtocolError(_WHERE("KI mismatch (%d != %d)", vaultMsg.ki, u->ki));
					// forward it to the vault server
					if (!*vaultServer) {
						err->log("ERR: I've got a vault message to forward to the vault server, but it's unavailable.\n", u->str().c_str());
						return 1;
					}
					vaultMsg.message.get(parsedMsg);
					parsedMsg.print(&lvault, /*clientToServer:*/true, u, vaultLogShort);
					// do additional processing
					onVaultMessageForward(u, &parsedMsg);
					// send it on to vault
					parsedMsg.tpots = vaultServer->tpots; // be sure to normalize to POTS format
					tmVault vaultMsgFwd(*vaultServer, u->ki, vaultMsg.x, isTask, &parsedMsg);
					send(vaultMsgFwd);
				}
				
				return 1;
			}
			
			// messages for finding the server to link to
			case NetMsgFindAge:
			{
				if (!u->ki) {
					err->log("ERR: %s sent a NetMsgFindAge but did not yet set his KI. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmFindAge findAge(u);
				msg->data.get(findAge);
				log->log("<RCV> [%d] %s\n", msg->sn, findAge.str().c_str());
				
				tvAgeLinkStruct ageLink;
				findAge.message.rewind();
				findAge.message.get(ageLink);
				if (!findAge.message.eof()) throw txProtocolError(_WHERE("Got a NetMsgFindAge which is too long"));
				log->print(" %s\n", ageLink.str().c_str());
				
				if (ageLink.linkingRule != KOriginalBook && ageLink.linkingRule != KOwnedBook && ageLink.linkingRule != KBasicLink && ageLink.linkingRule != KVisitBook && ageLink.linkingRule != KSubAgeBook)
					throw txProtocolError(_WHERE("Linking rule must be KSubAgeBook, KOriginalBook, KOwnedBook, KVisitBook or KBasicLink but is 0x%02X", ageLink.linkingRule));
				if (ageLink.ccr)
					throw txProtocolError(_WHERE("Linking as CCR is not allowed"));
				
				// if asked to do so, log the linking
				if (!linkLog.isEmpty()) {
					FILE *f = fopen(linkLog.c_str(), "a");
					if (f) {
						fprintf(f, "Player %s links from %s to: %s\n", u->name.c_str(), serverName.c_str(), ageLink.str().c_str());
						fclose(f);
					}
				}
				
				// Let's ask vault
				if (!*vaultServer) {
					err->log("ERR: I've got the ask the vault server to check an age for me, but it's unavailable.\n");
					return 1;
				}
				tmCustomVaultFindAge vaultFindAge(*vaultServer, u->ki, findAge.x, u->getSid(), findAge.message);
				send(vaultFindAge);
				
				return 1;
			}
			case NetMsgCustomFindServer:
			{
				if (u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgCustomFindServer but is not the vault server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomFindServer findServer(u);
				msg->data.get(findServer);
				log->log("<RCV> [%d] %s\n", msg->sn, findServer.str().c_str());
			
				// let's ask the tracking server
				if (!*trackingServer) {
					err->log("ERR: I've got the ask the tracking server to find an age for me, but it's unavailable.\n");
					return 1;
				}
				
				tmCustomFindServer trackingFindServer(*trackingServer, findServer);
				send(trackingFindServer);
				
				return 1;
			}
			case NetMsgCustomServerFound:
			{
				if (u->getPeerType() != KTracking) {
					err->log("ERR: %s sent a NetMsgCustomServerFound but is not the tracking server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmCustomServerFound serverFound(u);
				msg->data.get(serverFound);
				log->log("<RCV> [%d] %s\n", msg->sn, serverFound.str().c_str());
				
				// find the client
				tNetSessionRef client = sessionBySid(serverFound.sid);
				if (!*client || client->getPeerType() != KClient || client->ki != serverFound.ki) {
					err->log("ERR: I've got to tell player with KI %d about his game server, but can't find his session.\n", serverFound.ki);
					return 1;
				}
				
				uint8_t guid[8];
				alcGetHexGuid(guid, serverFound.serverGuid);
				tmFindAgeReply reply(*client, serverFound.x, serverFound.ipStr, serverFound.serverPort, serverFound.age, guid);
				send(reply);
				
				return 1;
			}
			
			// terminating a player
			case NetMsgPlayerTerminated:
			{
				if (u->getPeerType() != KTracking && u->getPeerType() != KVault) {
					err->log("ERR: %s sent a NetMsgPlayerTerminated but is neither tracking nor vault server. I\'ll kick him.\n", u->str().c_str());
					return -2; // hack attempt
				}
				
				// get the data out of the packet
				tmPlayerTerminated playerTerminated(u);
				msg->data.get(playerTerminated);
				log->log("<RCV> [%d] %s\n", msg->sn, playerTerminated.str().c_str());
				
				tNetSessionRef client = sessionByKi(playerTerminated.ki);
				if (!*client || (client->getPeerType() != KClient && client->getPeerType() != 0)) {
					err->log("ERR: I've got to kick the player with KI %d but can\'t find his session.\n", playerTerminated.ki);
					return 1;
				}
				
				terminate(*client, playerTerminated.reason);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
