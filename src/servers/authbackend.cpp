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
#define __U_AUTHBACKEND_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcnet.h>

////extra includes
#include "authbackend.h"

#include <alcdebug.h>

namespace alc {
	
	// this only contains the columns needed for this auth server
	static const char * authTableInitScript = "\
	CREATE TABLE `accounts` (\
		`uid` int unsigned NOT NULL auto_increment,\
		`guid` varchar(50) NOT NULL default '',\
		`name` varchar(50) NOT NULL default '',\
		`passwd` varchar(32) NOT NULL default '',\
		`a_level` tinyint unsigned NOT NULL default 15,\
		`last_login` timestamp NOT NULL default 0,\
		`last_ip` varchar(30) NOT NULL default '',\
		`attempts` tinyint unsigned NOT NULL default 0,\
		`last_attempt` timestamp NOT NULL default 0,\
		`cgas_cache_time` timestamp NOT NULL default 0,\
		PRIMARY KEY  (`uid`),\
		UNIQUE KEY `guid` (`guid`),\
		UNIQUE KEY `name` (`name`)\
	) TYPE=MyISAM;";

	tAuthBackend::tAuthBackend(void)
	{
		sql = NULL;
	
		tConfig *cfg = alcGetMain()->config();
		tString var = cfg->getVar("auth.minalevel");
		if (var.isEmpty()) minAccess = AcNotActivated;
		else minAccess = var.asU16();
		
		var = cfg->getVar("auth.att");
		if (var.isEmpty()) maxAttempts = 10;
		else maxAttempts = var.asU16();
		
		var = cfg->getVar("auth.distime");
		if (var.isEmpty()) disTime = 5*60;
		else disTime = var.asU16();
		
		var = cfg->getVar("auth.log");
		if (var.isEmpty() || var.asByte()) { // logging enabled per default
			log.open("auth.log");
		}
		
		cgasServer = cfg->getVar("auth.cgas.server"); // CGAS will be enabled if this one is not empty (aka set)
		cgasPath = cfg->getVar("auth.cgas.path");
		
		var = cfg->getVar("auth.cgas.port");
		if (var.isEmpty()) cgasPort = 80;
		else cgasPort = var.asU16();
		
		var = cfg->getVar("auth.cgas.default_access");
		if (var.isEmpty()) cgasDefaultAccess = 15;
		else cgasDefaultAccess = var.asU32();
		
		var = cfg->getVar("auth.cgas.max_cache_time");
		if (var.isEmpty()) cgasMaxCacheTime = 60*60*24; // 24 hours
		else cgasMaxCacheTime = var.asU32();

		prepare(); // initialize the database
	}
	
	tAuthBackend::~tAuthBackend(void)
	{
		if (sql) {
			DBG(5, "deleting SQL\n");
			delete sql;
		}
	}
	
	bool tAuthBackend::prepare(void)
	{
		// establish database connection
		if (sql) { // the connection is already established?
			if (sql->prepare()) // when we're still connected or recoonnection works, everything is fine
				return true;
			// otherwise, delete the connection and try again
			DBG(6, "deleting sql\n");
			delete sql;
		}
		DBG(6, "creating sql\n");
		sql = tSQL::createFromConfig();
		if (sql->prepare()) {
			// if auth table doesn't exist, create it
			if (!sql->queryForNumber("SHOW TABLES LIKE 'accounts'", "Looking for accounts table")) {
				sql->query(authTableInitScript, "Creating auth table");
			}
			// if it does, check if an upgrade of the structure is necessary
			else {
				if (!sql->queryForNumber("SHOW COLUMNS FROM accounts LIKE 'cgas_cache_time'", "Check for cgas_cache_time column")) {
					sql->query("ALTER TABLE accounts ADD cgas_cache_time timestamp NOT NULL default 0", "Adding cgas_cache_time column");
					sql->query("UPDATE accounts SET cgas_cache_time=0", "Set cgas_cache_time to 0"); // fill with 0 time, sicne the column must not be NULL
				}
			}
			
			log.log("Auth driver successfully started (%s)\n minimal access level: %d, max attempts: %d, disabled time: %d\n\n",
					__U_AUTHBACKEND_ID, minAccess, maxAttempts, disTime);
			log.flush();
			return true; // everything worked fine :)
		}
		else
			alcGetMain()->err()->log("ERR: Connecting to the database failed\n");
		// when we come here, it didn't work, so delete everything
		DBG(6, "deleting sql\n");
		delete sql;
		sql = NULL;
		return false;
	}

	tString tAuthBackend::calculateHash(const tString &login, const tString &passwd, const tString &challenge)
	{
		tMD5Buf md5buffer;
		md5buffer.write(challenge.data(), challenge.size());
		md5buffer.write(login.data(), login.size());
		md5buffer.write(passwd.data(), passwd.size());
		md5buffer.compute();
		return alcHex2Ascii(md5buffer);
	}
	
	tString tAuthBackend::sendCgasRequest(const tString &login, const tString &challenge, const tString &hash)
	{
		tString message;
		message.printf("GET %s HTTP/1.1\r\n"
					   "Host: %s\r\n"
					   "Connection: close\r\n"
					   "User-Agent: AlcugsHTTP\r\n"
					   "X-plLogin: %s\r\n"
					   "X-plHash: %s\r\n"
					   "X-plChallenge: %s\r\n\r\n",
					   cgasPath.c_str(), cgasServer.c_str(), login.c_str(), hash.c_str(), challenge.c_str());
		
		// The server (destination) struct
		struct sockaddr_in server; // server address struct
		memset(&server,0,sizeof(server)); //Delete it (zero it)
		server.sin_family = AF_INET; //The family
		// Set host an port
		struct hostent *host = gethostbyname(cgasServer.c_str());
		if(host==NULL) {
			log.log("Error resolving CGAS address: %s\n", cgasServer.c_str());
			return tString();
		}
		server.sin_addr.s_addr=*reinterpret_cast<U32 *>(host->h_addr_list[0]);
		server.sin_port=htons(cgasPort); //The port
		
		// Create a TCP socket
		int sock; //the socket
		if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0) {
			log.log("Error creating socket for CGAS\n");
			return tString();
		}

		// Start connection to the auth gateway
		if(connect(sock,reinterpret_cast<struct sockaddr *>(&server),sizeof(server))<0) {
			log.log("Error connecting to CGAS\n");
			return tString();
		}
		
		//Send the msg to the server
		if (send(sock, message.c_str(), message.size(), 0) != message.size()) {
			log.log("Error sending data to CGAS\n");
			close(sock);
			return tString();
		}
		
		//Recieve the response
		const int bufferSize = 2048;
		int receivedSize;
		Byte response[bufferSize];
		if((receivedSize = recv(sock, response, bufferSize, 0)) <= 0) {
			log.log("Error receiving data from CGAS\n");
			close(sock);
			return tString();
		}
		
		// close socket and be done
		close(sock);
		tString result;
		result.write(response, receivedSize);
		return result;
	}
	
	tAuthBackend::tQueryResult tAuthBackend::parseCgasResponse(const tString &response, tString *, tString *)
	{
		// Tokenize the response
		tStringTokenizer s = tString(response);
		while (!s.eof()) {
			log.log("Next token: %s\n", s.getToken().c_str());
		}
		return kError;
	}
	
	tAuthBackend::tQueryResult tAuthBackend::queryCgas(const tString &login, const tString &challenge, const tString &hash, bool hasCache, tString *passwd, tString *guid, Byte *accessLevel)
	{
		// send reuqest and parse reponse
		tString response = sendCgasRequest(login, challenge, hash);
		tQueryResult replyStatus = response.isEmpty() ? kError : parseCgasResponse(response, passwd, guid);
		if (replyStatus == kError) return kError;
		if (replyStatus == kNotFound) {
			if (hasCache) {
				// we can actually use what was queried, but the password is wrong, whatever we saved
				*accessLevel = AcNotRes;
				return kSuccess;
			}
			return kNotFound;
		}
		// okay, the server said the login is correct, put that into the cache
		tString query;
		if (hasCache) {
			query.printf("UPDATE accounts SET passwd='%s', cgas_cache_time=NOW() WHERE name='%s' AND guid='%s'", passwd->c_str(), login.c_str(), guid->c_str());
			sql->query(query, "Updating CGAS cache");
			if (sql->affectedRows() != 1) {
				log.log("ERROR: No player with name %s and GUID %s, even though I found it in the cache earlier - potential GUID change on CGAS", login.c_str(), guid->c_str());
				return kError;
			}
			// use the access level that was queried earlier
		}
		else {
			query.printf("INSERT INTO accounts (name, guid, passwd, a_level, cgas_cache_time) VALUES ('%s', '%s', '%s', '%d', NOW())", login.c_str(), guid->c_str(), passwd->c_str(), cgasDefaultAccess);
			sql->query(query, "Insert CGAS reply into cache");
			*accessLevel = cgasDefaultAccess;
		}
		return kSuccess;
	}
	
	tAuthBackend::tQueryResult tAuthBackend::queryPlayer(const tString &login, tString *passwd, tString *guid, U32 *attempts, U32 *lastAttempt, Byte *accessLevel)
	{
		tString query;
		*attempts = *lastAttempt = 0; // ensure there's a valid value in there
		*accessLevel = AcNotRes;
		*passwd = "";
		*guid = "00000000-0000-0000-0000-000000000000";
		
		// only query if we are connected properly
		if (!prepare()) {
			alcGetMain()->err()->log("ERR: Can't start auth driver. Authenticate request will be rejected.\n");
			return kError;
		}
		
		// query the database
		query.printf("SELECT UCASE(passwd), a_level, guid, attempts, UNIX_TIMESTAMP(last_attempt), UNIX_TIMESTAMP(cgas_cache_time) FROM accounts WHERE name='%s' LIMIT 1", sql->escape(login).c_str());
		sql->query(query, "Query player");
		
		// read the result
		MYSQL_RES *result = sql->storeResult();
		tQueryResult ret = kNotFound;
		if (result != NULL) {
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row != NULL)  { // read the columns
				// player found :)
				*passwd = row[0]; // passwd
				*accessLevel = atoi(row[1]); // a_level
				*guid = row[2]; // guid
				*attempts = atoi(row[3]); // attempts
				*lastAttempt = atoi(row[4]); // last_attempt
				U32 cgasCacheTime = atoi(row[5]); // cgas_cache_time
				if (!cgasServer.isEmpty() && cgasCacheTime < time(NULL)-cgasMaxCacheTime) {
					// the cache is too old, we can't use it - but still read the stuff to handle invalid login attempts
					ret = kCacheTooOld;
				}
				else {
					ret = kSuccess;
				}
			}
		}
		mysql_free_result(result);
		
		return ret;
	}

	void tAuthBackend::updatePlayer(const tString &guid, const tString &ip, U32 attempts, Byte updateStamps)
	{
		tString query;
		query.printf("UPDATE accounts SET attempts='%d', last_ip='%s'", attempts, sql->escape(ip).c_str());
		if (updateStamps == 1) // update only last attempt
			query.printf(", last_attempt=NOW()");
		else if (updateStamps == 2) // update last attempt and last login
			query.printf(", last_attempt=NOW(), last_login=NOW()");
		else {} // don't update any stamp
		query.printf(" WHERE guid='%s'", sql->escape(guid).c_str());
		sql->query(query, "Update player");
	}

	int tAuthBackend::authenticatePlayer(tNetSession *u, const tString &login, const tString &challenge, const tString &hash, Byte release, const tString &ip, tString *passwd, Byte *hexUid, Byte *accessLevel)
	{
		U32 attempts, lastAttempt;
		tString guid;
		tQueryResult queryResult = queryPlayer(login, passwd, &guid, &attempts, &lastAttempt, accessLevel); // query password, access level and guid of this user
		if (!cgasServer.isEmpty() && queryResult >= kNotFound) // query CGAS if it knows more (kNotFound or kCacheTooOld)
			queryResult = queryCgas(login, challenge, hash, queryResult == kCacheTooOld, passwd, &guid, accessLevel);
		alcGetHexUid(hexUid, guid);
		
		log.log("AUTH: player %s (IP: %s, game server %s):\n ", login.c_str(), ip.c_str(), u->str().c_str());
		if (queryResult == kNotFound || queryResult == kCacheTooOld) {
			log.print("Player not found\n");
			log.flush();
			return AInvalidUser;
		}
		else if (queryResult == kError) {
			log.print("unspecified server error\n");
			log.flush();
			return AUnspecifiedServerError;
		}
		else { // we found a player, let's process it
			int authResult;
			Byte updateStamps = 1; // update only last attempt
			log.print("UID = %s, attempt %d/%d, access level = %d\n ", guid.c_str(), attempts+1, maxAttempts, *accessLevel);
			
			if (*accessLevel >= minAccess) { // the account doesn't have enough access for this shard (accessLevel = minAccess is rejected as well, for backward compatability)
				log.print("access level is too big (must be lower than %d)\n", minAccess);
				authResult = AAccountDisabled;
			}
			// check number of attempts
			else if (attempts+1 >= maxAttempts && time(NULL)-lastAttempt < disTime) {
				log.print("too many attempts, login disabled for %d seconds (till %s)\n", disTime, alcGetStrTime(lastAttempt+disTime, 0).c_str());
				updateStamps = 0; // don't update the last attempt time when we're already dissing
				authResult = AAccountDisabled;
			}
			// check internal client (i.e. VaultManager)
			else if (release != TExtRel && !(release == TIntRel && *accessLevel <= AcCCR)) {
				log.print("unauthorized client\n");
				authResult = AAccountDisabled;
				++attempts;
			}
			else { // everythign seems fine... let's compare the password
				if(calculateHash(login, *passwd, challenge) != hash) { // wrong password :(
					log.print("invalid password\n");
					authResult = AInvalidPasswd;
					++attempts;
				}
				else { // it's correct, the player is authenticated
					log.print("auth succeeded\n");
					authResult = AAuthSucceeded;
					updateStamps = 2; // both last attempt and last login should be updated
					attempts = 0;
					
				}
			}
			
			// ok, now all we have to do is updating the player's last login and attempts and return the result
			updatePlayer(guid, ip, attempts, updateStamps);
			log.flush();
			return authResult;
		}
	}

} //end namespace alc
