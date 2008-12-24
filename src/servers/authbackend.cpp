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

#include <alcugs.h>
#include <alcnet.h>

////extra includes
#include "authbackend.h"

#include <alcdebug.h>

namespace alc {
	
	// this only contains the columns needed for this auth server
	const char * authTableInitScript = "\
	CREATE TABLE `accounts` (\
		`uid` int unsigned NOT NULL auto_increment,\
		`guid` varchar(50) NOT NULL default '',\
		`name` varchar(50) NOT NULL default '',\
		`passwd` varchar(32) NOT NULL default '',\
		`a_level` tinyint unsigned NOT NULL default 25,\
		`last_login` timestamp NOT NULL default 0,\
		`last_ip` varchar(30) NOT NULL default '',\
		`attempts` tinyint unsigned NOT NULL default 0,\
		`last_attempt` timestamp NOT NULL default 0,\
		PRIMARY KEY  (`uid`),\
		UNIQUE KEY `guid` (`guid`),\
		UNIQUE KEY `name` (`name`)\
	) TYPE=MyISAM;";

	tAuthBackend::tAuthBackend(void)
	{
		log = lnull;
		sql = NULL;
	
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("auth.minalevel");
		if (var.isNull()) minAccess = 25;
		else minAccess = var.asU16();
		
		var = cfg->getVar("auth.att");
		if (var.isNull()) maxAttempts = 10;
		else maxAttempts = var.asU16();
		
		var = cfg->getVar("auth.distime");
		if (var.isNull()) disTime = 5*60;
		else disTime = var.asU16();
		
		var = cfg->getVar("auth.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			log = new tLog("auth.log", 4, 0);
		}

		prepare(); // initialize the database
	}
	
	tAuthBackend::~tAuthBackend(void)
	{
		if (sql) {
			DBG(5, "deleting SQL\n");
			delete sql;
		}
		if (log != lnull) delete log;
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
			// check if the auth table exists
			char query[100];
			sprintf(query, "SHOW TABLES LIKE 'accounts'");
			sql->query(query, "Looking for accounts table");
			MYSQL_RES *result = sql->storeResult();
			bool exists = mysql_num_rows(result);
			mysql_free_result(result);
			// if it doesn't exist, create it
			if (exists || sql->query(authTableInitScript, "Creating auth table", false)) {
				log->log("Auth driver successfully started (%s)\n minimal access level: %d, max attempts: %d, disabled time: %d\n\n",
						__U_AUTHBACKEND_ID, minAccess, maxAttempts, disTime);
				log->flush();
				return true;
			}
			lerr->log("ERR: Creating auth table failed\n");
		}
		else
			lerr->log("ERR: Connecting to the database failed\n");
		// when we come here, it didn't work, so delete everything
		DBG(6, "deleting sql\n");
		delete sql;
		sql = NULL;
		return false;
	}

	void tAuthBackend::calculateHash(Byte *login, Byte *passwd, Byte *challenge, Byte *hash)
	{
		tMD5Buf md5buffer;
		md5buffer.write(challenge, strlen((char *)challenge));
		md5buffer.write(login, strlen((char *)login));
		md5buffer.write(passwd, strlen((char *)passwd));
		md5buffer.compute();
		alcHex2Ascii(hash, md5buffer.read(16), 16);
	}
	
	int tAuthBackend::queryPlayer(Byte *login, Byte *passwd, Byte *guid, U32 *attempts, U32 *lastAttempt)
	{
		char query[1024];
		*attempts = *lastAttempt = passwd[0] = 0; // ensure there's a valid value in there
		strcpy((char *)guid, "00000000-0000-0000-0000-000000000000");
		
		// only query if we are connected properly
		if (!prepare()) {
			lerr->log("ERR: Can't start auth driver. Authenticate request will be rejected.\n");
			return AcNotRes;
		}
		
		// query the database
		sprintf(query,"SELECT UCASE(passwd), a_level, guid, attempts, UNIX_TIMESTAMP(last_attempt) FROM accounts WHERE name='%s' LIMIT 1", sql->escape((char *)login));
		sql->query(query, "Query player");
		
		// read the result
		MYSQL_RES *result = sql->storeResult();
		int ret = AcNotRes;
		if (result != NULL) {
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row == NULL) ret = -1; // player doesn't exist
			else { // read the columns
				strncpy((char *)passwd, row[0], 49); // passwd
				ret = atoi(row[1]); // a_level
				strncpy((char *)guid, row[2], 49); // guid
				*attempts = atoi(row[3]); // attempts
				*lastAttempt = atoi(row[4]);
			}
		}
		mysql_free_result(result);
		
		return ret;
	}

	void tAuthBackend::updatePlayer(Byte *guid, Byte *ip, U32 attempts, Byte updateStamps)
	{
		char query[2048], stamps[256], ip_escaped[50], guid_escaped[50];
		strncpy(ip_escaped, sql->escape((char *)ip), 49);
		strncpy(guid_escaped, sql->escape((char *)guid), 49);
		if (updateStamps == 1) // update only last attempt
			sprintf(stamps, ", last_attempt=NOW()");
		else if (updateStamps == 2) // update last attempt and last login
			sprintf(stamps, ", last_attempt=NOW(), last_login=NOW()");
		else // don't update any stamp
			stamps[0] = 0; // an empty string
		sprintf(query, "UPDATE accounts SET attempts='%d', last_ip='%s'%s WHERE guid='%s'", attempts, ip_escaped, stamps, guid_escaped);
		sql->query(query, "Update player");
	}

	int tAuthBackend::authenticatePlayer(tNetSession *u, Byte *login, Byte *challenge, Byte *hash, Byte release, Byte *ip, Byte *passwd,
			Byte *guid, Byte *accessLevel)
	{
		Byte correctHash[50];
		U32 attempts, lastAttempt;
		int queryResult = queryPlayer(login, passwd, guid, &attempts, &lastAttempt); // query password, access level and guid of this user
		
		log->log("AUTH: player %s (IP: %s, game server %s):\n ", login, ip, u->str());
		if (queryResult < 0) { // that means: player not found
			*accessLevel = AcNotActivated;
			log->print("Player not found\n");
			log->flush();
			return AInvalidUser;
		}
		else if (queryResult >= AcNotRes) { // that means: there was an error
			*accessLevel = AcNotRes;
			log->print("unspecified server error\n");
			log->flush();
			return AUnspecifiedServerError;
		}
		else { // we found a player, let's process it
			int authResult;
			Byte updateStamps = 1; // update only last attempt
			*accessLevel = queryResult;
			log->print("UID = %s, attempt %d/%d, access level = %d\n ", guid, attempts+1, maxAttempts, *accessLevel);
			
			if (*accessLevel >= minAccess) { // the account doesn't have enough access for this shard (accessLevel = minAccess is rejected as well, for backward compatability)
				log->print("access level is too big (must be lower than %d)\n", minAccess);
				authResult = AAccountDisabled;
			}
			// check number of attempts
			else if (attempts+1 >= maxAttempts && time(NULL)-lastAttempt < disTime) {
				log->print("too many attempts, login disabled for %d seconds (till %s)\n", disTime, alcGetStrTime(lastAttempt+disTime, 0));
				updateStamps = 0; // don't update the last attempt time when we're already dissing
				authResult = AAccountDisabled;
			}
			// check internal client (i.e. VaultManager)
			else if (release != TExtRel && !(release == TIntRel && *accessLevel <= AcCCR)) {
				log->print("unauthorized client\n");
				authResult = AAccountDisabled;
				++attempts;
			}
			else { // everythign seems fine... let's compare the password
				calculateHash(login, passwd, challenge, correctHash);
				if(strncmp((char *)hash, (char *)correctHash, 49) != 0) { // wrong password :(
					log->print("invalid password\n");
					authResult = AInvalidPasswd;
					++attempts;
				}
				else { // it's correct, the player is authenticated
					log->print("auth succeeded\n");
					authResult = AAuthSucceeded;
					updateStamps = 2; // both last attempt and last login should be updated
					attempts = 0;
					
				}
			}
			
			// ok, now all we have to do is updating the player's last login and attempts and return the result
			updatePlayer(guid, ip, attempts, updateStamps);
			log->flush();
			return authResult;
		}
	}

} //end namespace alc
