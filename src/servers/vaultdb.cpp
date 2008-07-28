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

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>

////extra includes
#include "vaultdb.h"

#include "alcdebug.h"

namespace alc {

	const char *vaultTable = "vault";
	const char *refVaultTable = "ref_vault";

	////IMPLEMENTATION
	tVaultDB::tVaultDB(tLog *log)
	{
		sql = NULL;
		this->log = log;
	
		int version = getVersion();
		if (version < 2 || version > 3) throw txDatabaseError(_WHERE("only vault DB version 2 and 3 are supported, not %d", version));
		log->log("Started VaultDB driver (%s) on a vault DB version %d\n", __U_VAULTDB_ID, version);
		if (version == 2) {
			log->log("Converting DB from version 2 to 3... \n");
			migrateVersion2to3();
			log->log("Done converting DB from version 2 to 3!\n");
		}
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
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		char query[512];
		int version;
		sprintf(query, "SHOW COLUMNS FROM %s LIKE 'torans'", vaultTable);
		sql->query(query, "Checking for torans column");
		MYSQL_RES *result = sql->storeResult();
		bool exists = mysql_num_rows(result);
		mysql_free_result(result);
		if (exists)
			sprintf(query, "SELECT torans FROM %s WHERE type=6 LIMIT 1", vaultTable); // only the root node has type 6
		else
			sprintf(query, "SELECT int_1 FROM %s WHERE type=6 LIMIT 1", vaultTable); // only the root node has type 6
		sql->query(query, "Checking version number");
		result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt check version number"));
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row) version = atoi(row[0]);
		mysql_free_result(result);
		if (!row) throw txDatabaseError("couldnt find root vault node");
		return version;
	}
	
	void tVaultDB::convertIntToTimestamp(const char *table, const char *intColumn, const char *timestampColumn)
	{
		char query[512];
		
		sprintf(query, "ALTER TABLE %s ADD %s timestamp NOT NULL default 0 AFTER %s", table, timestampColumn, intColumn);
		sql->query(query, "converting int to timestamp (1/3)");
		
		sprintf(query, "UPDATE %s SET %s = FROM_UNIXTIME(%s)+0", table, timestampColumn, intColumn);
		sql->query(query, "converting int to timestamp (2/3)");
		
		sprintf(query, "ALTER TABLE %s DROP %s", table, intColumn);
		sql->query(query, "converting int to timestamp (3/3)");
	}
	
	void tVaultDB::migrateVersion2to3(void)
	{
		char query[2048];
		/* From version 2 to 3, the layout of the tables changed, but the way the content is organized stayed the same.
		   On both tables, the timestamp columns were converted to type TIMESTAMP.
		   For the main vault table, the columns were renamed to the names tvNode uses (which are taked from Vault Manager) */
		
		// first, the ref_vault
		// rename old timestamp column
		sprintf(query, "ALTER TABLE %s CHANGE timestamp timestamp_old int NOT NULL default 0", refVaultTable);
		sql->query(query, "renaming old timestamp row");
		// and create new one
		convertIntToTimestamp(refVaultTable, "timestamp_old", "timestamp");
		
		// now, the main vault
		// drop unused columns
		sprintf(query, "ALTER TABLE %s DROP microseconds2, DROP microseconds3, DROP data2, DROP unk13, DROP unk14", vaultTable);
		sql->query(query, "remove unused columns");
		// rename int columns (besides timestamps)
		sprintf(query, "ALTER TABLE %s CHANGE unk1 grp int NOT NULL default 0, CHANGE microseconds mod_microsec int NOT NULL default 0,\n\
			CHANGE id1 creator int NOT NULL default 0, CHANGE torans int_1 int NOT NULL default 0,\n\
			CHANGE distance int_2 int NOT NULL default 0, CHANGE elevation int_3 int NOT NULL default 0,\n\
			CHANGE unk5 int_4 int NOT NULL default 0, CHANGE id2 uint_1 int NOT NULL default 0,\n\
			CHANGE unk7 uint_2 int NOT NULL default 0, CHANGE unk8 uint_3 int NOT NULL default 0,\n\
			CHANGE unk9 uint_4 int NOT NULL default 0", vaultTable);
		sql->query(query, "rename int columns");
		// rename string and blob columns
		sprintf(query, "ALTER TABLE %s CHANGE entry_name str_1 varchar(255) NOT NULL default '',\n\
			CHANGE sub_entry_name str_2 varchar(255) NOT NULL default '', CHANGE owner_name str_3 varchar(255) NOT NULL default '',\n\
			CHANGE guid str_4 varchar(255) NOT NULL default '', CHANGE str1 str_5 varchar(255) NOT NULL default '',\n\
			CHANGE str2 str_6 varchar(255) NOT NULL default '', CHANGE avie lstr_1 varchar(255) NOT NULL default '',\n\
			CHANGE uid lstr_2 varchar(255) NOT NULL default '', CHANGE entry_value text_1 varchar(255) NOT NULL default '',\n\
			CHANGE entry2 text_2 varchar(255) NOT NULL default '', CHANGE data blob_1 longblob NOT NULL", vaultTable);
		sql->query(query, "rename string and blob columns");
		// convert timestamp columns
		convertIntToTimestamp(vaultTable, "timestamp", "mod_time");
		convertIntToTimestamp(vaultTable, "timestamp2", "crt_time");
		convertIntToTimestamp(vaultTable, "timestamp3", "age_time");
		// update version number
		sprintf(query, "UPDATE %s SET int_1=3 WHERE type=6", vaultTable); // only the root node has type 6
		sql->query(query, "setting version number");
	}
	
	int tVaultDB::getPlayerList(tMBuf &t, const Byte *uid)
	{
		char query[1024];
		t.clear();
		
		sprintf(query, "SELECT idx, lstr_1, int_2 FROM %s WHERE lstr_2 = '%s'", vaultTable, alcGetStrUid(uid));
		sql->query(query, "getting player list");
		
		MYSQL_RES *result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt query player list"));
		int number = mysql_num_rows(result);
		
		for (int i = 0; i < number; ++i) {
			MYSQL_ROW row = mysql_fetch_row(result);
			t.putU32(atoi(row[0])); // KI
			tUStr avatar(0); // normal UruString
			avatar.writeStr(row[1]);
			t.put(avatar);
			t.putByte(atoi(row[2])); // flags
		}
		mysql_free_result(result);
		return number;
	}
	
	int tVaultDB::checkKi(U32 ki, const Byte *uid, Byte *avatar)
	{
		char query[1024];
		avatar[0] = 0; // first emtpy the string
		
		sprintf(query, "SELECT lstr_1 FROM %s WHERE lstr_2 = '%s' and idx='%d'", vaultTable, alcGetStrUid(uid), ki);
		sql->query(query, "checking ki");
		
		MYSQL_RES *result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt check ki"));
		int number = mysql_num_rows(result);
		if (number > 1) throw txDatabaseError(_WHERE("Database inconsistency: KI %d exists more than once", ki));
		
		if (number == 1) {
			MYSQL_ROW row = mysql_fetch_row(result);
			strncpy((char*)avatar, row[0], 255);
		}
		mysql_free_result(result);
		return (number >= 1) ? 1 : 0;
	}
	
	U32 tVaultDB::findNode(tvNode &node, bool create)
	{
		char query[4096], cond[1024];
		bool comma = false;
		// first, we have to create the query...
		sprintf(query, "SELECT idx FROM %s WHERE ", vaultTable);
		
		if (node.flagB & MIndex) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "idx='%d'", node.index);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MType) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "type='%d'", node.type);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MPerms) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "permissions='%d'", node.permissions);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MOwner) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "owner='%d'", node.owner);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MGroup) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "grp='%d'", node.group);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MCreator) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "creator='%d'", node.creator);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MAgeName) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "age_name='%s'", sql->escape((char *)node.ageName.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MAgeGuid) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "age_guid='%s'", sql->escape((char *)alcGetStrGuid(node.ageGuid)));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MInt32_1) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "int_1='%d'", node.int1);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MInt32_2) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "int_2='%d'", node.int2);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MInt32_3) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "int_3='%d'", node.int3);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MInt32_4) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "int_4='%d'", node.int4);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MUInt32_1) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "uint_1='%d'", node.uInt1);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MUInt32_2) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "uint_2='%d'", node.uInt2);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MUInt32_3) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "uint_3='%d'", node.uInt3);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MUInt32_4) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "uint_4='%d'", node.uInt4);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_1) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "str_1='%s'", sql->escape((char *)node.str1.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_2) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "str_2='%s'", sql->escape((char *)node.str2.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_3) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "str_3='%s'", sql->escape((char *)node.str3.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_4) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "str_4='%s'", sql->escape((char *)node.str4.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_5) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "str_5='%s'", sql->escape((char *)node.str5.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_6) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "str_6='%s'", sql->escape((char *)node.str6.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MlStr64_1) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "lstr_1='%s'", sql->escape((char *)node.lStr1.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MlStr64_2) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "lstr_2='%s'", sql->escape((char *)node.lStr2.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MText_1) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "text_1='%s'", sql->escape((char *)node.text1.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MText_2) {
			if (comma) strcat(query, " and ");
			sprintf(cond, "text_2='%s'", sql->escape((char *)node.text2.c_str()));
			strcat(query, cond);
			comma = true;
		}
		
		strcat(query, " LIMIT 1");
		sql->query(query, "finding node");
		
		MYSQL_RES *result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt check ki"));
		int number = mysql_num_rows(result);
		if (number > 1) throw txDatabaseError(_WHERE("strange, I should NEVER have several results when asking for a node"));
		
		U32 id = 0;
		if (number == 1) {
			MYSQL_ROW row = mysql_fetch_row(result);
			id = atoi(row[0]);
		}
		else if (create) id = createNode(node);
		
		mysql_free_result(result);
		
		return id;
	}
	
	U32 tVaultDB::createNode(tvNode &node)
	{
		throw txUnet(_WHERE("FIXME: creating nodes not yet implemented"));
	}

} //end namespace alc

