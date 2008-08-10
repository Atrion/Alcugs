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
		
		prepare();
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
			// FIXME: create vault db if necessary
			int version = getVersion();
			if (version < 2 || version > 3) throw txDatabaseError(_WHERE("only vault DB version 2 and 3 are supported, not %d", version));
			if (version == 2) {
				log->log("Converting DB from version 2 to 3... \n");
				migrateVersion2to3();
				log->log("Done converting DB from version 2 to 3!\n");
			}
			log->log("Started VaultDB driver (%s) on a vault DB version %d\n", __U_VAULTDB_ID, version);
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
		sql->query(query, "getVersion: Checking for torans column");
		
		MYSQL_RES *result = sql->storeResult();
		bool exists = mysql_num_rows(result);
		mysql_free_result(result);
		if (exists)
			sprintf(query, "SELECT torans FROM %s WHERE type=6 LIMIT 1", vaultTable); // only the root node has type 6
		else
			sprintf(query, "SELECT int_1 FROM %s WHERE type=6 LIMIT 1", vaultTable); // only the root node has type 6
		sql->query(query, "getVersion: Checking version number");
		result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt check version number"));
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row) version = atoi(row[0]);
		mysql_free_result(result);
		if (!row) throw txDatabaseError("couldnt find root vault node");
		return version;
	}
	
	void tVaultDB::getVaultFolderName(Byte *folder)
	{
		folder[0] = 0; // empty it
		
		char query[512];
		sprintf(query, "SELECT idx, str_1 FROM %s WHERE type=6 LIMIT 1", vaultTable);
		sql->query(query, "Getting vault folder name");
		
		MYSQL_RES *result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt query player list"));
		int number = mysql_num_rows(result);
		
		if (number == 0) throw txDatabaseError(_WHERE("creating the main vault folder is not yet supported"));
		MYSQL_ROW row = mysql_fetch_row(result);
		if (atoi(row[0]) != KVaultID || strlen(row[1]) != 16)
			throw txDatabaseError(_WHERE("invalid main vault folder found (or there's none at all"));
		strcpy((char *)folder, row[1]);
		mysql_free_result(result);
	}
	
	void tVaultDB::convertIntToTimestamp(const char *table, const char *intColumn, const char *timestampColumn)
	{
		char query[512];
		
		sprintf(query, "ALTER TABLE %s ADD %s timestamp NOT NULL default 0 AFTER %s", table, timestampColumn, intColumn);
		sql->query(query, "converting int to timestamp (1/3)");
		
		sprintf(query, "UPDATE %s SET %s = FROM_UNIXTIME(%s)", table, timestampColumn, intColumn);
		sql->query(query, "converting int to timestamp (2/3)");
		
		sprintf(query, "ALTER TABLE %s DROP %s", table, intColumn);
		sql->query(query, "converting int to timestamp (3/3)");
	}
	
	void tVaultDB::migrateVersion2to3(void)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		char query[2048];
		/* From version 2 to 3, the layout of the tables changed, but the way the content is organized stayed the same.
		   On both tables, the timestamp columns were converted to type TIMESTAMP.
		   For the main vault table, the columns were renamed to the names tvNode uses (which are taked from Vault Manager) */
		
		// first, the ref_vault
		// rename old timestamp column and delete microseconds
		sprintf(query, "ALTER TABLE %s DROP microseconds, CHANGE timestamp timestamp_old int NOT NULL default 0", refVaultTable);
		sql->query(query, "migrateVersion2to3: renaming old timestamp row");
		// and create new one
		convertIntToTimestamp(refVaultTable, "timestamp_old", "timestamp");
		
		// now, the main vault
		// drop unused columns
		sprintf(query, "ALTER TABLE %s DROP microseconds, DROP microseconds2, DROP microseconds3, DROP data2, DROP unk13, DROP unk14", vaultTable);
		sql->query(query, "migrateVersion2to3: remove unused columns");
		// rename int columns (besides timestamps)
		sprintf(query, "ALTER TABLE %s CHANGE unk1 grp int NOT NULL default 0,\n\
			CHANGE id1 creator int NOT NULL default 0, CHANGE torans int_1 int NOT NULL default 0,\n\
			CHANGE distance int_2 int NOT NULL default 0, CHANGE elevation int_3 int NOT NULL default 0,\n\
			CHANGE unk5 int_4 int NOT NULL default 0, CHANGE id2 uint_1 int NOT NULL default 0,\n\
			CHANGE unk7 uint_2 int NOT NULL default 0, CHANGE unk8 uint_3 int NOT NULL default 0,\n\
			CHANGE unk9 uint_4 int NOT NULL default 0", vaultTable);
		sql->query(query, "migrateVersion2to3: rename int columns");
		// rename string and blob columns
		sprintf(query, "ALTER TABLE %s CHANGE entry_name str_1 varchar(255) NOT NULL default '',\n\
			CHANGE sub_entry_name str_2 varchar(255) NOT NULL default '', CHANGE owner_name str_3 varchar(255) NOT NULL default '',\n\
			CHANGE guid str_4 varchar(255) NOT NULL default '', CHANGE str1 str_5 varchar(255) NOT NULL default '',\n\
			CHANGE str2 str_6 varchar(255) NOT NULL default '', CHANGE avie lstr_1 varchar(255) NOT NULL default '',\n\
			CHANGE uid lstr_2 varchar(255) NOT NULL default '', CHANGE entry_value text_1 varchar(255) NOT NULL default '',\n\
			CHANGE entry2 text_2 varchar(255) NOT NULL default '', CHANGE data blob_1 longblob NOT NULL", vaultTable);
		sql->query(query, "migrateVersion2to3: rename string and blob columns");
		// convert timestamp columns
		convertIntToTimestamp(vaultTable, "timestamp", "mod_time");
		convertIntToTimestamp(vaultTable, "timestamp2", "crt_time");
		convertIntToTimestamp(vaultTable, "timestamp3", "age_time");
		// update version number
		sprintf(query, "UPDATE %s SET int_1=3 WHERE type=6", vaultTable); // only the root node has type 6
		sql->query(query, "migrateVersion2to3: setting version number");
	}
	
	int tVaultDB::getPlayerList(tMBuf &t, const Byte *uid)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
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
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		char query[1024];
		avatar[0] = 0; // first emtpy the string
		
		sprintf(query, "SELECT lstr_1 FROM %s WHERE lstr_2 = '%s' and idx='%d' LIMIT 1", vaultTable, alcGetStrUid(uid), ki);
		sql->query(query, "checking ki");
		
		MYSQL_RES *result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt check ki"));
		int number = mysql_num_rows(result);
		
		if (number == 1) {
			MYSQL_ROW row = mysql_fetch_row(result);
			strncpy((char*)avatar, row[0], 255);
		}
		mysql_free_result(result);
		return (number == 1) ? 1 : 0;
	}
	
	void tVaultDB::createNodeQuery(char *query, tvNode &node, bool isUpdate)
	{
		char *escapedData = NULL;
		char cond[1024];
		bool comma = false;
		const char *commaStr = isUpdate ? "," : " and ";
		
		if (isUpdate) {
			escapedData = (char *)malloc((node.blob1Size*2 + 2048)*sizeof(char));
			escapedData[0] = 0; // empty it
		}
		
		if (!isUpdate && node.flagB & MIndex) { // only insert this if it is a SELECT
			if (comma) strcat(query, commaStr);
			sprintf(cond, "idx='%d'", node.index);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MType) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "type='%d'", node.type);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MPerms) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "permissions='%d'", node.permissions);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MOwner) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "owner='%d'", node.owner);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MGroup) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "grp='%d'", node.group);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MModTime) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "mod_time=FROM_UNIXTIME(%d)", node.modTime);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MCreator) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "creator='%d'", node.creator);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MCrtTime) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "crt_time=FROM_UNIXTIME(%d)", node.crtTime);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MModTime) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "age_time=FROM_UNIXTIME(%d)", node.ageTime);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MAgeName) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "age_name='%s'", sql->escape((char *)node.ageName.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MAgeGuid) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "age_guid='%s'", sql->escape((char *)alcGetStrGuid(node.ageGuid)));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MInt32_1) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "int_1='%d'", node.int1);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MInt32_2) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "int_2='%d'", node.int2);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MInt32_3) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "int_3='%d'", node.int3);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MInt32_4) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "int_4='%d'", node.int4);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MUInt32_1) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "uint_1='%d'", node.uInt1);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MUInt32_2) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "uint_2='%d'", node.uInt2);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MUInt32_3) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "uint_3='%d'", node.uInt3);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MUInt32_4) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "uint_4='%d'", node.uInt4);
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_1) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "str_1='%s'", sql->escape((char *)node.str1.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_2) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "str_2='%s'", sql->escape((char *)node.str2.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_3) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "str_3='%s'", sql->escape((char *)node.str3.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_4) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "str_4='%s'", sql->escape((char *)node.str4.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_5) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "str_5='%s'", sql->escape((char *)node.str5.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MStr64_6) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "str_6='%s'", sql->escape((char *)node.str6.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MlStr64_1) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "lstr_1='%s'", sql->escape((char *)node.lStr1.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MlStr64_2) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "lstr_2='%s'", sql->escape((char *)node.lStr2.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MText_1) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "text_1='%s'", sql->escape((char *)node.text1.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (node.flagB & MText_2) {
			if (comma) strcat(query, commaStr);
			sprintf(cond, "text_2='%s'", sql->escape((char *)node.text2.c_str()));
			strcat(query, cond);
			comma = true;
		}
		if (isUpdate && node.flagB & MBlob1) { // only insert this if it is an UPDATE
			if (comma) strcat(query, commaStr);
			strcat(query, "blob_1='");
			if (node.blob1Size) {
				strcat(query, sql->escape(escapedData, (char *)node.blob1, node.blob1Size));
			}
			strcat(query, "'");
			comma = true;
		}
		
		if (escapedData) free(escapedData);
	}
	
	U32 tVaultDB::findNode(tvNode &node, tvManifest *mfs, bool create)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		char query[4096];
		// first, we have to create the query...
		sprintf(query, "SELECT idx, UNIX_TIMESTAMP(mod_time) FROM %s WHERE ", vaultTable);
		createNodeQuery(query, node, /*isUpdate*/false);
		
		// now, let's execute it
		sql->query(query, "finding node");
		MYSQL_RES *result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt check ki"));
		int number = mysql_num_rows(result);
		if (number > 1) throw txDatabaseError(_WHERE("strange, I should NEVER have several results when asking for a node"));
		
		U32 id = 0;
		if (number == 1) {
			MYSQL_ROW row = mysql_fetch_row(result);
			id = atoi(row[0]);
			if (mfs) {
				mfs->id = id;
				mfs->time = atoi(row[1]);
			}
		}
		else if (create) {
			id = createNode(node);
			if (mfs) {
				mfs->id = id;
				mfs->time = node.modTime;
			}
		}
		else if (mfs) {
			mfs->id = 0;
			mfs->time = 0;
		}
		
		mysql_free_result(result);
		
		return id;
	}
	
	U32 tVaultDB::createNode(tvNode &node)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		if (!(node.flagB & MType)) throw txDatabaseError(_WHERE("type must be set for all new nodes"));
		
		int size = node.blob1Size*2 + 4096;
		char *query = (char *)malloc((size+4096)*sizeof(char));
		char *values = (char *)malloc(size*sizeof(char));
		char *helpStr = (char *)malloc(size*sizeof(char));
		
		// time fix
		node.flagB |= (MModTime | MCrtTime | MAgeTime);
		node.crtTime = node.ageTime = node.modTime = alcGetTime();
		
		sprintf(query, "INSERT INTO %s (type", vaultTable);
		sprintf(values, ") VALUES ('%d'", node.type);
		
		if (node.flagB & MPerms) {
			strcat(query, ",permissions");
			sprintf(helpStr, ",'%d'", node.permissions);
			strcat(values, helpStr);
		}
		if (node.flagB & MOwner) {
			strcat(query, ",owner");
			sprintf(helpStr, ",'%d'", node.owner);
			strcat(values, helpStr);
		}
		if (node.flagB & MGroup) {
			strcat(query, ",grp");
			sprintf(helpStr, ",'%d'", node.group);
			strcat(values, helpStr);
		}
		if (node.flagB & MModTime) {
			strcat(query, ",mod_time");
			sprintf(helpStr, ",FROM_UNIXTIME('%d')", node.modTime);
			strcat(values, helpStr);
		}
		if (node.flagB & MCreator) {
			strcat(query, ",creator");
			sprintf(helpStr, ",'%d'", node.creator);
			strcat(values, helpStr);
		}
		if (node.flagB & MCrtTime) {
			strcat(query, ",crt_time");
			sprintf(helpStr, ",FROM_UNIXTIME('%d')", node.crtTime);
			strcat(values, helpStr);
		}
		if (node.flagB & MAgeTime) {
			strcat(query, ",age_time");
			sprintf(helpStr, ",FROM_UNIXTIME('%d')", node.ageTime);
			strcat(values, helpStr);
		}
		if (node.flagB & MAgeName) {
			strcat(query, ",age_name");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.ageName.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MAgeGuid) {
			strcat(query, ",age_guid");
			sprintf(helpStr, ",'%s'", sql->escape((char *)alcGetStrGuid(node.ageGuid)));
			strcat(values, helpStr);
		}
		if (node.flagB & MInt32_1) {
			strcat(query, ",int_1");
			sprintf(helpStr, ",'%d'", node.int1);
			strcat(values, helpStr);
		}
		if (node.flagB & MInt32_2) {
			strcat(query, ",int_2");
			sprintf(helpStr, ",'%d'", node.int2);
			strcat(values, helpStr);
		}
		if (node.flagB & MInt32_3) {
			strcat(query, ",int_3");
			sprintf(helpStr, ",'%d'", node.int3);
			strcat(values, helpStr);
		}
		if (node.flagB & MInt32_4) {
			strcat(query, ",int_4");
			sprintf(helpStr, ",'%d'", node.int4);
			strcat(values, helpStr);
		}
		if (node.flagB & MUInt32_1) {
			strcat(query, ",uint_1");
			sprintf(helpStr, ",'%d'", node.uInt1);
			strcat(values, helpStr);
		}
		if (node.flagB & MUInt32_2) {
			strcat(query, ",uint_2");
			sprintf(helpStr, ",'%d'", node.uInt2);
			strcat(values, helpStr);
		}
		if (node.flagB & MUInt32_3) {
			strcat(query, ",uint_3");
			sprintf(helpStr, ",'%d'", node.uInt3);
			strcat(values, helpStr);
		}
		if (node.flagB & MUInt32_4) {
			strcat(query, ",uint_4");
			sprintf(helpStr, ",'%d'", node.uInt4);
			strcat(values, helpStr);
		}
		if (node.flagB & MStr64_1) {
			strcat(query, ",str_1");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.str1.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MStr64_2) {
			strcat(query, ",str_2");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.str2.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MStr64_3) {
			strcat(query, ",str_3");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.str3.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MStr64_4) {
			strcat(query, ",str_4");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.str4.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MStr64_5) {
			strcat(query, ",str_5");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.str5.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MStr64_6) {
			strcat(query, ",str_6");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.str6.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MlStr64_1) {
			strcat(query, ",lstr_1");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.lStr1.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MlStr64_2) {
			strcat(query, ",lstr_2");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.lStr2.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MText_1) {
			strcat(query, ",text_1");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.text1.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MText_2) {
			strcat(query, ",text_2");
			sprintf(helpStr, ",'%s'", sql->escape((char *)node.text2.c_str()));
			strcat(values, helpStr);
		}
		if (node.flagB & MBlob1) {
			strcat(query, ",blob_1");
			strcat(values, ",'");
			if (node.blob1Size) {
				strcat(values, sql->escape(helpStr, (char *)node.blob1, node.blob1Size));
			}
			strcat(values, "'");
		}
		
		strcat(query, values);
		strcat(query, ")");
		sql->query(query, "Inserting new node");
		
		free((void *)query);
		free((void *)values);
		free((void *)helpStr);
		
		return sql->insertId();
	}
	
	void tVaultDB::updateNode(tvNode &node)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		// time fix
		node.flagB |= MModTime;
		node.modTime = alcGetTime();
		
		// create the query
		char *query = (char *)malloc((node.blob1Size*2 + 4048)*sizeof(char)), cond[64];
		sprintf(query, "UPDATE %s SET ", vaultTable);
		createNodeQuery(query, node, /*isUpdate*/true);
		sprintf(cond, " WHERE idx='%d'", node.index);
		strcat(query, cond);
		
		sql->query(query, "Updating vault node", true);
		
		free(query);
	}
	
	void tVaultDB::getManifest(U32 baseNode, tvManifest ***mfs, int *nMfs, tvNodeRef ***ref, int *nRef)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		*mfs = NULL;
		*nMfs = 0;
		*ref = NULL;
		*nRef = 0;
	
		tvManifest **feed = NULL, **aux = NULL, **final = NULL;
		int          nFeed = 0,     nAux = 0,     nFinal = 0;
		char query[2048], cond[32];
		MYSQL_RES *result;
		MYSQL_ROW row;
		bool comma;
		int auxIndex, feedIndex;
		
		// get base node
		sprintf(query, "SELECT idx, UNIX_TIMESTAMP(mod_time) FROM %s WHERE idx='%d'", vaultTable, baseNode);
		sql->query(query, "getManifest: getting first node");
		
		result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt get first node"));
		int number = mysql_num_rows(result);
		if (number > 1) throw txDatabaseError(_WHERE("strange, I should NEVER have several results when asking for a node"));
		else if (number < 1) throw txDatabaseError(_WHERE("getManfiest: First node %d does not exist", baseNode));
		
		// save it in the feed
		row = mysql_fetch_row(result);
		feed = (tvManifest **)malloc(sizeof(tvManifest *));
		feed[0] = new tvManifest(atoi(row[0]), atoi(row[1]));
		nFeed = 1;
		mysql_free_result(result);
		
		// now, the big loop... as long as there is something in the feed, process it
		while (nFeed > 0) {
			// save the so-far final list
			aux = final;
			nAux = nFinal;
			// get space for the new final list - it will contain all elements of the so-far final list and the feed
			final = (tvManifest **)malloc((nFinal+nFeed)*sizeof(tvManifest *));
			nFinal = 0;
			
			sprintf(query, "SELECT n.idx, UNIX_TIMESTAMP(n.mod_time), r.id1, r.id2, UNIX_TIMESTAMP(r.timestamp), r.flag FROM %s n JOIN %s r ON r.id3=n.idx WHERE r.id2 IN(", vaultTable, refVaultTable);
			comma = false;
			
			// our task is now to (a) merge the (both sorted) lists aux and feed into a (sorted) final list and (b) add all the node ids
			//  from the feed list to the query
			feedIndex = 0;
			auxIndex = 0;
			while (auxIndex < nAux) {
				while (feedIndex < nFeed) {
					// if the current feed node is lower than the current aux one, it has to be inserted now
					if (feed[feedIndex]->id < aux[auxIndex]->id) {
						if (nFinal == 0 || final[nFinal-1]->id != feed[feedIndex]->id) { // avoid duplicates
							final[nFinal] = feed[feedIndex];
							++nFinal;
							// add it to the query
							if (comma) strcat(query, ",");
							sprintf(cond, "%d", feed[feedIndex]->id);
							strcat(query, cond);
							comma = true;
						}
						else
							delete feed[feedIndex]; // duplicate
					}
					else if (feed[feedIndex]->id > aux[auxIndex]->id) {
						// the current feed must be inserted into the final list after the current aux, so we can insert the current aux now
						final[nFinal] = aux[auxIndex];
						++nFinal;
						++auxIndex;
						break; // we have to check if this was the last aux node or there's something after it
					}
					else
						delete feed[feedIndex]; // duplicate
					++feedIndex; // got this one
				}
				if (feedIndex >= nFeed) {
					// we got all the feed nodes, so the rest of the aux nodes comes now
					while(auxIndex < nAux) {
						final[nFinal] = aux[auxIndex];
						++nFinal;
						++auxIndex;
					}
					break; // got all of them
				}
			}
			
			// now we got all aux nodes, let's see if there are some feeds remaining
			while (feedIndex < nFeed) {
				if (nFinal == 0 || final[nFinal-1]->id != feed[feedIndex]->id) { // avoid duplicates
					final[nFinal] = feed[feedIndex];
					++nFinal;
					// add it to the query
					if (comma) strcat(query, ",");
					sprintf(cond, "%d", feed[feedIndex]->id);
					strcat(query, cond);
					comma = true;
				}
				else
					delete feed[feedIndex]; // duplicate
				++feedIndex; // got this one
			}
			
			// now we can free the aux and feed tables
			if (aux != NULL) {
				free((void *)aux);
				aux = NULL;
			}
			nAux = 0;
			if (feed != NULL) {
				free((void *)feed);
				feed = NULL;
			}
			nFeed = 0;
			
			// ok... now we can query (if there's anything)
			if (!comma) break;
			strcat(query, ") ORDER BY n.idx ASC");
			sql->query(query, "getManifest: getting child nodes");
			
			result = sql->storeResult();
			if (result == NULL) throw txDatabaseError(_WHERE("couldnt get nodes"));
			int number = mysql_num_rows(result);
				
			// the result goes into the feed table, and the refs are saved in their table as well
			feed = (tvManifest **)malloc(number*sizeof(tvManifest *));
			*ref = (tvNodeRef **)realloc((void *)*ref, (*nRef + number)*sizeof(tvNodeRef *));
			while (nFeed < number) {
				row = mysql_fetch_row(result);
				// save manifest
				U32 idx = atoi(row[0]);
				feed[nFeed] = new tvManifest(idx, atoi(row[1]));
				++nFeed;
				// and reference
				(*ref)[*nRef] = new tvNodeRef(atoi(row[2]), atoi(row[3]), idx, atoi(row[4]), (Byte)atoi(row[5]));
				++(*nRef);
			}
			mysql_free_result(result);
		}
		
		// now we can free the aux and feed tables
		if (aux != NULL) {
			free((void *)aux);
			aux = NULL;
		}
		if (feed != NULL) {
			free((void *)feed);
			feed = NULL;
		}
		
		// the final list is what the caller gets
		*mfs = final;
		*nMfs = nFinal;
	}
	
	void tVaultDB::getMGRs(U32 baseNode, U32 **table, U32 *tableSize)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		*table = NULL;
		*tableSize = 0;
	
		U32 *feed = NULL, *aux = NULL, *final = NULL;
		U32  nFeed = 0,    nAux = 0,    nFinal = 0;
		char query[2048], cond[32];
		MYSQL_RES *result;
		MYSQL_ROW row;
		bool comma;
		U32 auxIndex, feedIndex;
		
		// put base node in the feed
		feed = (U32 *)malloc(sizeof(U32));
		feed[0] = baseNode;
		nFeed = 1;
		
		// now, the big loop... as long as there is something in the feed, process it
		while (nFeed > 0) {
			// save the so-far final list
			aux = final;
			nAux = nFinal;
			// get space for the new final list - it will contain all elements of the so-far final list and the feed
			final = (U32 *)malloc((nFinal+nFeed)*sizeof(U32));
			nFinal = 0;
			
			sprintf(query, "SELECT n.idx,n.type FROM %s n JOIN %s r ON r.id2=n.idx WHERE r.id3 IN(", vaultTable, refVaultTable);
			comma = false;
			
			// our task is now to (a) merge the (both sorted) lists aux and feed into a (sorted) final list and (b) add all the node ids
			//  from the feed list to the query
			feedIndex = 0;
			auxIndex = 0;
			while (auxIndex < nAux) {
				while (feedIndex < nFeed) {
					// if the current feed node is lower than the current aux one, it has to be inserted now
					if (feed[feedIndex] < aux[auxIndex]) {
						if (nFinal == 0 || final[nFinal-1] != feed[feedIndex]) { // avoid duplicates
							final[nFinal] = feed[feedIndex];
							++nFinal;
							// add it to the query
							if (comma) strcat(query, ",");
							sprintf(cond, "%d", feed[feedIndex]);
							strcat(query, cond);
							comma = true;
						}
						else
							; // duplicate
					}
					else if (feed[feedIndex] > aux[auxIndex]) {
						// the current feed must be inserted into the final list after the current aux, so we can insert the current aux now
						final[nFinal] = aux[auxIndex];
						++nFinal;
						++auxIndex;
						break; // we have to check if this was the last aux node or there's something after it
					}
					else
						; // duplicate
					++feedIndex; // got this one
				}
				if (feedIndex >= nFeed) {
					// we got all the feed nodes, so the rest of the aux nodes comes now
					while(auxIndex < nAux) {
						final[nFinal] = aux[auxIndex];
						++nFinal;
						++auxIndex;
					}
					break; // got all of them
				}
			}
			
			// now we got all aux nodes, let's see if there are some feeds remaining
			while (feedIndex < nFeed) {
				if (nFinal == 0 || final[nFinal-1] != feed[feedIndex]) { // avoid duplicates
					final[nFinal] = feed[feedIndex];
					++nFinal;
					// add it to the query
					if (comma) strcat(query, ",");
					sprintf(cond, "%d", feed[feedIndex]);
					strcat(query, cond);
					comma = true;
				}
				else
					; // duplicate
				++feedIndex; // got this one
			}
			
			// now we can free the aux and feed tables
			if (aux != NULL) {
				free((void *)aux);
				aux = NULL;
			}
			nAux = 0;
			if (feed != NULL) {
				free((void *)feed);
				feed = NULL;
			}
			nFeed = 0;
			
			// ok... now we can query (if there's anything)
			if (!comma) break;
			strcat(query, ") ORDER BY n.idx ASC");
			sql->query(query, "getMGRs: getting child nodes");
			
			result = sql->storeResult();
			if (result == NULL) throw txDatabaseError(_WHERE("couldnt get nodes"));
			U32 number = mysql_num_rows(result);
				
			// the result goes into the feed table
			feed = (U32 *)malloc(number*sizeof(U32));
			while (nFeed < number) {
				row = mysql_fetch_row(result);
				// save ID
				U32 idx = atoi(row[0]);
				feed[nFeed] = idx;
				++nFeed;
				if (atoi(row[1]) <= 7) { // it's a MGR so lets save it - and keep the table in order!
					U32 insertVal = idx, tmp;
					for (U32 i = 0; i < *tableSize; ++i) {
						if ((*table)[i] == insertVal) break;
						if ((*table)[i] > insertVal) {
							// put the insertVal in the current place and save the current one to be inserted later
							tmp = insertVal;
							insertVal = (*table)[i];
							(*table)[i] = tmp;
						}
					}
					if (*tableSize == 0 || (*table)[*tableSize-1] < insertVal) { // if a value still has to be inserted, grow the table
						++(*tableSize);
						*table = (U32 *)realloc((void *)(*table), (*tableSize)*sizeof(U32));
						(*table)[*tableSize-1] = insertVal;
					}
				}
			}
			mysql_free_result(result);
		}
		
		// now we can free the aux and feed tables
		if (aux != NULL) {
			free((void *)aux);
			aux = NULL;
		}
		if (feed != NULL) {
			free((void *)feed);
			feed = NULL;
		}
		if (final != NULL) {
			free((void *)final);
			final = NULL;
		}
	}
	
	void tVaultDB::fetchNodes(tMBuf &buf, int tableSize, tvNode ***nodes, int *nNodes)
	{
		U32 *table = (U32 *)malloc(tableSize*sizeof(U32));
		buf.rewind();
		for (int i = 0; i < tableSize; ++i) table[i] = buf.getU32();
		fetchNodes(table, tableSize, nodes, nNodes);
		free((void *)table);
	}
	
	void tVaultDB::fetchNodes(U32 *table, int tableSize, tvNode ***nodes, int *nNodes)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		char query[32*1024], cond[32];
		MYSQL_RES *result;
		MYSQL_ROW row;
		unsigned long *lengths; // the len of each column
		*nodes = NULL;
		*nNodes = 0;
		
		sprintf(query, "SELECT idx, type, permissions, owner, grp, UNIX_TIMESTAMP(mod_time), creator, UNIX_TIMESTAMP(crt_time), UNIX_TIMESTAMP(age_time), age_name, age_guid, int_1, int_2, int_3, int_4, uint_1, uint_2, uint_3, uint_4, str_1, str_2, str_3, str_4, str_5, str_6, lstr_1, lstr_2, text_1, text_2, blob_1 FROM %s WHERE idx IN(", vaultTable);
		
		for (int i = 0; i < tableSize; ++i) {
			if (i > 0) strcat(query, ",");
			sprintf(cond, "%d", table[i]);
			strcat(query, cond);
		}
		
		strcat(query, ")");
		sql->query(query, "fetching nodes");
		
		result = sql->storeResult();
		if (result == NULL) throw txDatabaseError(_WHERE("couldnt fetch nodes"));
		*nNodes = mysql_num_rows(result);
		
		if (*nNodes == 0) return;
		*nodes = (tvNode **)malloc((*nNodes)*sizeof(tvNode *));
		for (int i = 0; i < *nNodes; ++i) {
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result); //get the size of each column
			
			tvNode *node = new tvNode;
			node->flagA = 0x00000002;
			node->flagB = 0xFFFFFFFF; // everything enabled
			node->flagC = 0x00000007; // everything enabled
			// fill in the row
			node->index = atoi(row[0]);
			node->type = atoi(row[1]);
			node->permissions = atoi(row[2]);
			node->owner = atoi(row[3]);
			node->group = atoi(row[4]);
			node->modTime = atoi(row[5]);
			node->creator = atoi(row[6]);
			node->crtTime = atoi(row[7]);
			node->ageTime = atoi(row[8]);
			node->ageName.writeStr(row[9]);
			alcAscii2Hex(node->ageGuid, (Byte *)row[10], 8);
			node->int1 = atoi(row[11]);
			node->int2 = atoi(row[12]);
			node->int3 = atoi(row[13]);
			node->int4 = atoi(row[14]);
			node->uInt1 = atoi(row[15]);
			node->uInt2 = atoi(row[16]);
			node->uInt3 = atoi(row[17]);
			node->uInt4 = atoi(row[18]);
			node->str1.writeStr(row[19]);
			node->str2.writeStr(row[20]);
			node->str3.writeStr(row[21]);
			node->str4.writeStr(row[22]);
			node->str5.writeStr(row[23]);
			node->str6.writeStr(row[24]);
			node->lStr1.writeStr(row[25]);
			node->lStr2.writeStr(row[26]);
			node->text1.writeStr(row[27]);
			node->text2.writeStr(row[28]);
			node->blob1Size = lengths[29];
			if (node->blob1Size > 0) {
				node->blob1 = (Byte *)malloc(sizeof(Byte)*node->blob1Size);
				memcpy(node->blob1, row[29], node->blob1Size);
			}
			
			(*nodes)[i] = node;
		}
		mysql_free_result(result);
	}
	
	bool tVaultDB::addNodeRef(tvNodeRef &ref)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		char query[1024];
	
		// time fix
		ref.time = alcGetTime();
		
		sprintf(query, "INSERT INTO %s (id1, id2, id3, timestamp, flag) VALUES('%d', '%d', '%d', FROM_UNIXTIME('%d'), '%d')", refVaultTable, ref.saver, ref.parent, ref.child, ref.time, ref.flags);
		try {
			sql->query(query, "Creating new node ref");
			return true;
		}
		catch (txDatabaseError &error) {
			// FIXME: find a better way to avoid an error when a duplicate is added
			return false;
		}
	}
	
	void tVaultDB::removeNodeRef(U32 parent, U32 son, bool cautious)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		char query[2048];
		MYSQL_RES *result;
		MYSQL_ROW row;
		int numParent, type, num;
		
		// get number of parent nodes
		sprintf(query, "SELECT COUNT(*) FROM %s WHERE id3='%d'", refVaultTable, son);
		sql->query(query, "removeNodeRef: number of parent nodes");
		result = sql->storeResult();
		
		if (result == NULL) throw txDatabaseError(_WHERE("couldn't get number of parent nodes"));
		num = mysql_num_rows(result);
		if (num != 1) throw txDatabaseError(_WHERE("couldn't get number of parent nodes"));
		
		row = mysql_fetch_row(result);
		numParent = atoi(row[0]);
		mysql_free_result(result);
		
		// get node type
		sprintf(query, "SELECT type FROM %s WHERE idx='%d'", vaultTable, son);
		sql->query(query, "removeNodeRef: node type");
		result = sql->storeResult();
		
		if (result == NULL) throw txDatabaseError(_WHERE("couldn't get node type"));
		num = mysql_num_rows(result);
		if (num != 1) throw txDatabaseError(_WHERE("couldn't get node type"));
		
		row = mysql_fetch_row(result);
		type = atoi(row[0]);
		mysql_free_result(result);
		
		bool safeType = cautious ? (type == KImageNode || type == KTextNoteNode || type == KChronicleNode) : type > 7;
		if (numParent <= 1 && safeType) {
			// there are no more references to this node, and it's a safe node to remove
			removeNodeTree(son, cautious);
			// FIXME: what about marker games?
		}
		else { // only remove this ref
			sprintf(query, "DELETE FROM %s WHERE id2='%d' AND id3='%d'", refVaultTable, parent, son);
			sql->query(query, "removeNodeRef: removing a node reference");
		}
	}
	
	void tVaultDB::removeNodeTree(U32 node, bool cautious)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		char query[2048];
		MYSQL_RES *result;
		MYSQL_ROW row;
		int num;
		
		// remove the node and all references to it
		sprintf(query, "DELETE FROM %s WHERE idx='%d'", vaultTable, node);
		sql->query(query, "removeNodeTree: removing a node");
		sprintf(query, "DELETE FROM %s WHERE id3='%d'", refVaultTable, node);
		sql->query(query, "removeNodeTree: removing all references to a node");
		
		// get all references from this node and remove them
		// we do this using recursion since doing it with a loop would make thigns more complicated without saving database queries
		sprintf(query, "SELECT id3 FROM %s WHERE id2='%d'", refVaultTable, node);
		sql->query(query, "removeNodeTree: getting child nodes");
		result = sql->storeResult();
		
		if (result == NULL) throw txDatabaseError(_WHERE("couldn't get child nodes"));
		num = mysql_num_rows(result);
		
		for (int i = 0; i < num; ++i) {
			row = mysql_fetch_row(result);
			removeNodeRef(node, atoi(row[0]), cautious);
		}
		mysql_free_result(result);
	}
	
	void tVaultDB::setSeen(U32 parent, U32 son, Byte seen)
	{
		if (!prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		char query[512];
		sprintf(query, "UPDATE %s SET flag='%d' WHERE id2='%d' AND id3='%d'", refVaultTable, seen, parent, seen);
		sql->query(query, "Updating seen flag");
	}

} //end namespace alc

