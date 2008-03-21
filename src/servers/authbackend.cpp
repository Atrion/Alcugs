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
#include <unet.h>

////extra includes
#include "authbackend.h"

#include <alcdebug.h>

namespace alc {
	
	// this only contains the columns needed for this auth server
	const char * authTableInitScript = "\
	CREATE TABLE `accounts` (\
		`uid` int(10) unsigned NOT NULL auto_increment,\
		`guid` varchar(50) NOT NULL default '',\
		`name` varchar(50) NOT NULL default '',\
		`passwd` varchar(32) NOT NULL default '',\
		`a_level` tinyint(1) unsigned NOT NULL default '25',\
		`last_login` timestamp(14) NOT NULL,\
		`last_ip` varchar(30) NOT NULL default '',\
		`attempts` tinyint(1) unsigned NOT NULL default '0',\
		`last_attempt` timestamp(14) NOT NULL,\
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
		if (var.isNull()) maxAttempts = 6;
		else maxAttempts = var.asU16();
		
		var = cfg->getVar("auth.distime");
		if (var.isNull()) disTime = 5*60;
		else disTime = var.asU16();
		
		var = cfg->getVar("auth.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			log = new tLog("auth.log", 4, 0);
		}

		if (prepare())
			log->log("Auth driver successfully started (%s)\n minimal access level: %d, max attempts: %d, disabled time: %d\n\n",
					__U_AUTHBACKEND_ID, minAccess, maxAttempts, disTime);
		else { // initializing didn't work... will be tried again when actually needed
			delete sql;
			sql = NULL;
		}
	}
	
	bool tAuthBackend::prepare(void)
	{
		// establish database connection
		if (sql) {
			lerr->log("ERR: Database already opened when calling tAuthBackend::prepare()\n");
			delete sql;
		}
		sql = tSQL::createFromConfig();
		if (!sql->prepare()) return false;
		
		// check if the auth table exists
		char query[100];
		sprintf(query, "SELECT * FROM `accounts`");
		bool exists = sql->query(query, "Looking for accounts table");
		mysql_free_result(sql->storeResult()); // that's necessary, otherwise we get a "2014 Commands out of sync; you can't run this command now" error
		if (!exists) // it does not, so create it
			if (!sql->query(authTableInitScript, "creating auth table")) return false;
		
		return true;
	}

	void tAuthBackend::calculateHash(Byte *login, Byte *passwd, Byte *challenge, Byte *hash) {
		tMD5Buf md5buffer;
		md5buffer.write(challenge, strlen((char *)challenge));
		md5buffer.write(login, strlen((char *)login));
		md5buffer.write(passwd, strlen((char *)passwd));
		md5buffer.compute();
		alcHex2Ascii(hash, md5buffer.read(16), 16);
	}
	
	int tAuthBackend::queryPlayer(Byte *login, Byte *passwd, Byte *guid)
	{
		char query[1024];
		passwd[0] = 0; // ensure there's a valid value in there
		strcpy((char *)guid, "00000000-0000-0000-0000-000000000000");
		
		// only query if we are connected
		bool connected = false;
		if (sql == NULL) { // if the connection wasn't yet established, try to do that now
			connected = prepare();
		}
		else
			connected = sql->prepare(); // otherwise, ensure the connection is re-established in case of a timeout
		if (!connected) {
			lerr->log("Can't start auth driver. All authenticate requests will be rejected.\n");
			delete sql;
			sql = NULL;
			return AcNotRes;
		}
		
		// query the database
		sprintf(query,"SELECT UCASE(passwd), a_level, guid FROM `accounts` WHERE name='%s'", sql->escape((char *)login));
		if (!sql->query(query, "Query user name")) return AcNotRes;
		
		// read the result
		MYSQL_RES *result = sql->storeResult();
		int ret = AcNotRes;
		if (result != NULL) {
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row == NULL) ret = -1;
			else {
				strcpy((char *)passwd, row[0]); // passwd
				ret = atoi(row[1]); // a_level
				strcpy((char *)guid, row[2]); // guid
			}
		}
		mysql_free_result(result);
		
		return ret;
	}

	int tAuthBackend::authenticatePlayer(Byte *login, Byte *challenge, Byte *hash, Byte release, Byte *ip, Byte *passwd,
			Byte *guid, Byte *accessLevel)
	{
		Byte correctHash[50];
		int queryResult = queryPlayer(login, passwd, guid); // query password, access level and guid of this user
		
		log->log("AUTH: result for player %s: ", login);
		if (queryResult < 0) { // that means: player not found
			*accessLevel = AcNotActivated;
			log->print("Player not found\n");
			return AInvalidUser;
		}
		else if (queryResult >= AcNotRes) { // that means: there was an error
			*accessLevel = AcNotRes;
			log->print("unspecified server error\n");
			return AUnspecifiedServerError;
		}
		else { // we found a player, let's process it
			*accessLevel = queryResult;
			log->print("GUID = %s, access level = %d; ", guid, *accessLevel);
			
			if (*accessLevel >= minAccess) { // the account doesn't have enough access for this shard (accessLevel = minAccess is rejected as well, for backward compatability)
				log->print("access level is too big (must be max. %d)\n", minAccess);
				return AAccountDisabled;
			}
			
			calculateHash(login, passwd, challenge, correctHash);
			if(strcmp((char *)hash, (char *)correctHash) != 0) {
				log->print("invalid password\n");
				return AInvalidPasswd;
			}
			log->print("auth aucceeded\n");
			return AAuthSucceeded;
		}
	}

} //end namespace alc
