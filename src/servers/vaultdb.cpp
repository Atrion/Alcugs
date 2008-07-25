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

/* CVS tag - DON'T TOUCH*/
#define __U_VAULTDB_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>

////extra includes
#include "vaultdb.h"

#include "alcdebug.h"

namespace alc {

	const char *vault_table = "vault";
	const char *ref_vault_table = "ref_vault";

	////IMPLEMENTATION
	tVaultDB::tVaultDB(tLog *log)
	{
		sql = NULL;
		this->log = log;
	
		int version = getVersion();
		if (version < 2 || version > 3) throw txUnet(_WHERE("only vault DB version 2 and 3 are supported, not %d", version));
		log->log("Started VaultDB driver (%s) on a vault DB version %d\n", __U_VAULTDB_ID, version);
	}
	
	bool tVaultDB::prepare(void)
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
			// FIXME: create vault db if necessary, upgrade it if necessary
			return true;
		}
		// when we come here, it didn't work, so delete everything
		DBG(6, "deleting sql\n");
		delete sql;
		sql = NULL;
		return false;
	}
	
	int tVaultDB::getVersion(void)
	{
		if (!prepare()) throw txUnet(_WHERE("no access to DB"));
		
		char query[512];
		sprintf(query, "SHOW COLUMNS FROM vault LIKE 'torans'");
		sql->query(query, "Checking for torans column");
		MYSQL_RES *result = sql->storeResult();
		bool exists = mysql_num_rows(result);
		mysql_free_result(result);
		if (!exists) // the torans column does not exist so it's already version 3
			return 3;
		else { // query the version from the database
			int version = -1;
			sprintf(query, "SELECT v.torans FROM %s v WHERE v.type=6 LIMIT 1", vault_table); // only the root node has type 6
			sql->query(query, "Checking version number");
			MYSQL_RES *result = sql->storeResult();
			if (result == NULL) throw txUnet(_WHERE("couldnt check version number"));
			MYSQL_ROW row = mysql_fetch_row(result);
			if (row) version = atoi(row[0]);
			mysql_free_result(result);
			if (!row) throw txUnet("couldnt find root vault node");
			return version;
		}
	}

} //end namespace alc

