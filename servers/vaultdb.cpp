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

//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "vaultdb.h"

#include <sql.h>
#include <netexception.h>
#include <alcmain.h>
#include <alcversion.h>
#include <protocol/vaultproto.h>
#include <protocol/vaultmsg.h>

#include <cstdlib>
#include <ctime>

namespace alc {

	static const char *vaultTable = "vault";
	static const char *refVaultTable = "ref_vault";
	
	static const char *vault_table_init = "\
	CREATE TABLE `%s` (\
		`idx` int NOT NULL auto_increment ,\
		`type` tinyint unsigned NOT NULL default 0,\
		`permissions` int NOT NULL default 0,\
		`owner` int NOT NULL default 0,\
		`grp` int NOT NULL default 0,\
		`mod_time` double NOT NULL default 0,\
		`creator` int NOT NULL default 0,\
		`crt_time` timestamp NOT NULL default 0,\
		`age_time` timestamp NOT NULL default 0,\
		`age_name` varchar(255) NOT NULL default '',\
		`age_guid` varchar(16) NOT NULL default '',\
		`int_1` int NOT NULL default 0,\
		`int_2` int NOT NULL default 0,\
		`int_3` int NOT NULL default 0,\
		`int_4` int NOT NULL default 0,\
		`uint_1` int NOT NULL default 0,\
		`uint_2` int NOT NULL default 0,\
		`uint_3` int NOT NULL default 0,\
		`uint_4` int NOT NULL default 0,\
		`str_1` varchar(255) NOT NULL default '',\
		`str_2` varchar(255) NOT NULL default '',\
		`str_3` varchar(255) NOT NULL default '',\
		`str_4` varchar(255) NOT NULL default '',\
		`str_5` varchar(255) NOT NULL default '',\
		`str_6` varchar(255) NOT NULL default '',\
		`lstr_1` varchar(255) NOT NULL default '',\
		`lstr_2` varchar(255) NOT NULL default '',\
		`text_1` text,\
		`text_2` text,\
		`blob_1` longblob,\
		PRIMARY KEY ( `idx` ) ,\
		KEY `type` ( `type` ) ,\
		KEY `owner` ( `owner` ),\
		KEY `creator` ( `creator` ) ,\
		KEY `age_name` ( `age_name` ) ,\
		KEY `age_guid` ( `age_guid` ) ,\
		KEY `int_1` ( `int_1` ) ,\
		KEY `str_4` ( `str_4` ) ,\
		KEY `lstr_1` ( `lstr_1` ) ,\
		KEY `lstr_2` ( `lstr_2` )\
	) TYPE=MyISAM PACK_KEYS=0 AUTO_INCREMENT=%d;";
	// observation shows that varchar is not enough for the text fields - and setting "NOT NULL" on it makes it mandatory for inserts
	// "blob_1" has a length field of 4 byte, with "longblob" we can be sure the data will always fit
	
	static const char *ref_vault_table_init = "\
	CREATE TABLE `%s` (\
		`id1` int NOT NULL default 0,\
		`id2` int NOT NULL default 0,\
		`id3` int NOT NULL default 0,\
		`timestamp` timestamp NOT NULL default 0,\
		`microseconds` int NOT NULL default 0,\
		`flag` tinyint unsigned NOT NULL default 0,\
		PRIMARY KEY  (`id2`,`id3`),\
		KEY `id2` (`id2`),\
		KEY `id3` (`id3`)\
	) TYPE=MyISAM;";
	
	static const int vaultVersion=4; // only change on major vault format changes, and be sure that there is a migration (see tVaultDB::prepare)
		/* Version history:
			0 -> old very old format
			1 -> old unet3 format
			2 -> new unet3 format, adds DniCityX2Finale to allow end game sequence play
				NOTE THIS WILL HAVE UNEXPECTED AND UNWANTED RESULTS ON NON TPOTS CLIENTS!!!
				The vault must have at least this version for the unet3+ vault to be able to migrate it
			3 -> first unet3+ version, removes unused columns and renames the rest to the vault manager names, uses timestamp columns
			4 -> use text instead of varchar for the text columns - varchar is not long enough
		*/

	tVaultDB::tVaultDB(tLog *log)
	{
		this->log = log;
		
		// init SQL
		sql = tSQL::createFromConfig();
		if (!sql->prepare()) {
			// it didn't work, so delete everything
			delete sql;
			sql = NULL;
			throw txDatabaseError(_WHERE("Error connecting to vault DB"));
		}
		// check if table exists
		tString query;
		query.printf("SHOW TABLES LIKE '%s'", vaultTable);
		// if vault table doesn't exist, create it
		if (!sql->queryForNumber(query, "Prepare: Looking for vault table")) {
			query.clear();
			query.printf(vault_table_init, vaultTable, KVaultID);
			sql->query(query, "Prepare: Creating vault table");
			query.clear();
			query.printf(ref_vault_table_init, refVaultTable);
			sql->query(query, "Prepare: Creating ref vault table");
			// create the root folder
			tString folderName;
			folderName.put8(0x0F);
			folderName.put8(0x13);
			folderName.put8(0x37);
			folderName.put32(time(NULL));
			folderName.put8(random()%250);
			tString asciiFolderName = alcGetStrGuid(folderName.data());
			query.clear();
			query.printf("INSERT INTO %s (idx, type, int_1, str_1, str_2, text_1, text_2) VALUES ('%d', 6, '%d', '%s', '%s', 'You must never edit or delete this node!', '%s')",
							vaultTable, KVaultID, vaultVersion, asciiFolderName.c_str(), alcGetMain()->name().c_str(), alcVersionTextShort());
			sql->query(query, "Prepare: Creating vault folder");
		}
		else {
			int version = getVersion();
			if (version < 2 || version > vaultVersion) throw txDatabaseError(_WHERE("only vault DB version 2 to %d are supported, not %d", vaultVersion, version));
			if (version == 2) {
				log->log("Converting DB from version 2 to 3... \n");
				migrateVersion2to3();
				log->log("Done converting DB from version 2 to 3!\n");
				version = 3;
			}
			if (version == 3) {
				log->log("Converting DB from version 3 to 4... \n");
				migrateVersion3to4();
				log->log("Done converting DB from version 3 to 4!\n");
				version = 4;
			}
			if (version != vaultVersion)
				throw txDatabaseError(_WHERE("Migration function missing!"));
		}
		// done!
		log->log("Started VaultDB driver\n");
	}
	
	tVaultDB::~tVaultDB()
	 { delete sql; }
	
	int tVaultDB::getVersion(void)
	{
		// this is a private function, so the caller already did the prepare() check
		
		tString numberQuery, versionQuery;
		int version = 0;
		
		// find out how to query for version number
		numberQuery.printf("SHOW COLUMNS FROM %s LIKE 'torans'", vaultTable);
		if (sql->queryForNumber(numberQuery, "getVersion: Checking for torans column"))
			versionQuery.printf("SELECT torans FROM %s WHERE type=6 LIMIT 1", vaultTable); // only the root node has type 6
		else
			versionQuery.printf("SELECT int_1 FROM %s WHERE type=6 LIMIT 1", vaultTable); // only the root node has type 6
		// get version number
		sql->query(versionQuery, "getVersion: Checking version number");
		MYSQL_RES *result = sql->storeResult();
		MYSQL_ROW row = mysql_fetch_row(result);
		if (row) version = atoi(row[0]);
		mysql_free_result(result);
		if (!row || !version) throw txDatabaseError("couldn't find vault folder node");
		return version;
	}
	
	tString tVaultDB::getVaultFolderName(void)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		tString folder;
		
		tString query;
		query.printf("SELECT idx, str_1 FROM %s WHERE type='%d' LIMIT 1", vaultTable, KVNodeMgrServerNode);
		sql->query(query, "Getting vault folder name");
		
		MYSQL_RES *result = sql->storeResult();
		size_t number = mysql_num_rows(result);
		
		if (number == 0) throw txDatabaseError(_WHERE("could not find main vault folder"));
		MYSQL_ROW row = mysql_fetch_row(result);
		if (atoi(row[0]) != KVaultID || strlen(row[1]) != 16)
			throw txDatabaseError(_WHERE("invalid main vault folder found"));
		folder = row[1];
		mysql_free_result(result);
		return folder;
	}
	
	void tVaultDB::convertIntToTimestamp(const char *table, const char *intColumn, const char *timestampColumn)
	{
		// this is a private function, so the caller already did the prepare() check
		tString query;
		
		query.printf("ALTER TABLE %s ADD %s timestamp NOT NULL default 0 AFTER %s", table, timestampColumn, intColumn);
		sql->query(query, "converting int to timestamp (1/3)");
		
		query.clear();
		query.printf("UPDATE %s SET %s = FROM_UNIXTIME(%s)", table, timestampColumn, intColumn);
		sql->query(query, "converting int to timestamp (2/3)");
		
		query.clear();
		query.printf("ALTER TABLE %s DROP %s", table, intColumn);
		sql->query(query, "converting int to timestamp (3/3)");
	}
	
	void tVaultDB::convertIntToDouble(const char *table, const char *intColumn, const char *doubleColumn)
	{
		// this is a private function, so the caller already did the prepare() check
		tString query;
		
		query.printf("ALTER TABLE %s ADD %s double NOT NULL default 0 AFTER %s", table, doubleColumn, intColumn);
		sql->query(query, "converting int to double (1/3)");
		
		query.clear();
		query.printf("UPDATE %s SET %s = %s", table, doubleColumn, intColumn);
		sql->query(query, "converting int to double (2/3)");
		
		query.clear();
		query.printf("ALTER TABLE %s DROP %s", table, intColumn);
		sql->query(query, "converting int to double (3/3)");
	}
	
	void tVaultDB::migrateVersion2to3(void)
	{
		// this is a private function, so the caller already did the prepare() check
	
		tString query;
		/* From version 2 to 3, the layout of the tables changed, but the way the content is organized stayed the same.
		   On both tables, the timestamp columns were converted to type TIMESTAMP.
		   For the main vault table, the columns were renamed to the names tvNode uses (which are taken from Vault Manager) */
		
		// first, the ref_vault
		// rename old timestamp column and delete microseconds
		query.printf("ALTER TABLE %s CHANGE timestamp timestamp_old int NOT NULL default 0", refVaultTable);
		sql->query(query, "migrateVersion2to3: renaming old timestamp row");
		// and create new one
		convertIntToTimestamp(refVaultTable, "timestamp_old", "timestamp");
		
		// now, the main vault
		// drop unused columns
		query.clear();
		query.printf("ALTER TABLE %s DROP microseconds, DROP microseconds2, DROP microseconds3, DROP data2, DROP unk13, DROP unk14", vaultTable);
		sql->query(query, "migrateVersion2to3: remove unused columns");
		// rename int columns (besides timestamps)
		query.clear();
		query.printf("ALTER TABLE %s CHANGE unk1 grp int NOT NULL default 0,\n\
			CHANGE id1 creator int NOT NULL default 0, CHANGE torans int_1 int NOT NULL default 0,\n\
			CHANGE distance int_2 int NOT NULL default 0, CHANGE elevation int_3 int NOT NULL default 0,\n\
			CHANGE unk5 int_4 int NOT NULL default 0, CHANGE id2 uint_1 int NOT NULL default 0,\n\
			CHANGE unk7 uint_2 int NOT NULL default 0, CHANGE unk8 uint_3 int NOT NULL default 0,\n\
			CHANGE unk9 uint_4 int NOT NULL default 0", vaultTable);
		sql->query(query, "migrateVersion2to3: rename int columns");
		// rename string and blob columns
		query.clear();
		query.printf("ALTER TABLE %s CHANGE entry_name str_1 varchar(255) NOT NULL default '',\n\
			CHANGE sub_entry_name str_2 varchar(255) NOT NULL default '', CHANGE owner_name str_3 varchar(255) NOT NULL default '',\n\
			CHANGE guid str_4 varchar(255) NOT NULL default '', CHANGE str1 str_5 varchar(255) NOT NULL default '',\n\
			CHANGE str2 str_6 varchar(255) NOT NULL default '', CHANGE avie lstr_1 varchar(255) NOT NULL default '',\n\
			CHANGE uid lstr_2 varchar(255) NOT NULL default '', CHANGE entry_value text_1 varchar(255) NOT NULL default '',\n\
			CHANGE entry2 text_2 varchar(255) NOT NULL default '', CHANGE data blob_1 longblob NOT NULL", vaultTable);
		sql->query(query, "migrateVersion2to3: rename string and blob columns");
		// convert timestamp columns
		convertIntToDouble(vaultTable, "timestamp", "mod_time");
		convertIntToTimestamp(vaultTable, "timestamp2", "crt_time");
		convertIntToTimestamp(vaultTable, "timestamp3", "age_time");
		// update version number
		query.clear();
		query.printf("UPDATE %s SET int_1=3 WHERE type=6", vaultTable); // only the root node has type 6
		sql->query(query, "migrateVersion2to3: setting version number");
		// remove invalid references
		removeInvalidRefs();
	}
	
	void tVaultDB::migrateVersion3to4(void)
	{
		// this is a private function, so the caller already did the prepare() check
		
		tString query;
		// update the text columns
		query.printf("ALTER TABLE %s CHANGE text_1 text_1 text, CHANGE text_2 text_2 text, CHANGE blob_1 blob_1 longblob", vaultTable);
		sql->query(query, "migrateVersion3to4: altering table structure");
		// update version number
		query.clear();
		query.printf("UPDATE %s SET int_1=4 WHERE type=6", vaultTable); // only the root node has type 6
		sql->query(query, "migrateVersion3to4: setting version number");
	}
	
	int tVaultDB::getPlayerList(const uint8_t *uid, tmVaultPlayerList *list)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		if (list) list->avatars.clear(); // t may be NULL if we just check the number of players
		
		tString query;
		
		query.printf("SELECT idx, lstr_1, int_2 FROM %s WHERE type = '%d' and lstr_2 = '%s'", vaultTable, KVNodeMgrPlayerNode, alcGetStrUid(uid).c_str());
		sql->query(query, "getting player list");
		
		MYSQL_RES *result = sql->storeResult();
		size_t number = mysql_num_rows(result);
		
		if (list) {
			for (size_t i = 0; i < number; ++i) {
				MYSQL_ROW row = mysql_fetch_row(result);
				list->avatars.push_back(tmVaultPlayerList::tAvatar(atoi(row[0]), tString(row[1]), atoi(row[2]))); // KI, Avatar name, flags
			}
		}
		mysql_free_result(result);
		return number;
	}
	
	tString tVaultDB::checkKi(uint32_t ki, const uint8_t* uid, bool* ownAvatar)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		tString query;
		query.printf("SELECT lstr_1 FROM %s WHERE type = '%d' and lstr_2 = '%s' and idx='%d' LIMIT 1",
					 vaultTable, KVNodeMgrPlayerNode, alcGetStrUid(uid).c_str(), ki);
		sql->query(query, "checking ki");
		
		MYSQL_RES *result = sql->storeResult();
		size_t number = mysql_num_rows(result);
		
		tString avatar;
		if (number == 1) {
			MYSQL_ROW row = mysql_fetch_row(result);
			avatar = row[0];
		}
		*ownAvatar = (number == 1);
		mysql_free_result(result);
		return avatar;
	}
	
	tString tVaultDB::createNodeQuery(const tvNode &node, bool isUpdate)
	{
		tString query;
		bool comma = false;
		const char *commaStr = isUpdate ? "," : " and ";
		
		if (!isUpdate && node.flagB & MIndex) { // only insert this if it is a SELECT
			query.printf("idx='%d'", node.index);
			comma = true;
		}
		if (node.flagB & MType) {
			if (comma) query.writeStr(commaStr);
			query.printf("type='%d'", node.type);
			comma = true;
		}
		if (node.flagB & MPerms) {
			if (comma) query.writeStr(commaStr);
			query.printf("permissions='%d'", node.permissions);
			comma = true;
		}
		if (node.flagB & MOwner) {
			if (comma) query.writeStr(commaStr);
			query.printf("owner='%d'", node.owner);
			comma = true;
		}
		if (node.flagB & MGroup) {
			if (comma) query.writeStr(commaStr);
			query.printf("grp='%d'", node.group);
			comma = true;
		}
		if (node.flagB & MModTime) {
			if (comma) query.writeStr(commaStr);
			query.printf("mod_time=%f", node.modTime);
			comma = true;
		}
		if (node.flagB & MCreator) {
			if (comma) query.writeStr(commaStr);
			query.printf("creator='%d'", node.creator);
			comma = true;
		}
		if (node.flagB & MCrtTime) {
			if (comma) query.writeStr(commaStr);
			query.printf("crt_time=FROM_UNIXTIME(%d)", node.crtTime);
			comma = true;
		}
		if (node.flagB & MAgeTime) {
			if (comma) query.writeStr(commaStr);
			query.printf("age_time=FROM_UNIXTIME(%d)", node.ageTime);
			comma = true;
		}
		if (node.flagB & MAgeName) {
			if (comma) query.writeStr(commaStr);
			query.printf("age_name='%s'", sql->escape(node.ageName).c_str());
			comma = true;
		}
		if (node.flagB & MAgeGuid) {
			if (comma) query.writeStr(commaStr);
			query.printf("age_guid='%s'", sql->escape(alcGetStrGuid(node.ageGuid)).c_str());
			comma = true;
		}
		if (node.flagB & MInt32_1) {
			if (comma) query.writeStr(commaStr);
			query.printf("int_1='%d'", node.int1);
			comma = true;
		}
		if (node.flagB & MInt32_2) {
			if (comma) query.writeStr(commaStr);
			query.printf("int_2='%d'", node.int2);
			comma = true;
		}
		if (node.flagB & MInt32_3) {
			if (comma) query.writeStr(commaStr);
			query.printf("int_3='%d'", node.int3);
			comma = true;
		}
		if (node.flagB & MInt32_4) {
			if (comma) query.writeStr(commaStr);
			query.printf("int_4='%d'", node.int4);
			comma = true;
		}
		if (node.flagB & MUInt32_1) {
			if (comma) query.writeStr(commaStr);
			query.printf("uint_1='%d'", node.uInt1);
			comma = true;
		}
		if (node.flagB & MUInt32_2) {
			if (comma) query.writeStr(commaStr);
			query.printf("uint_2='%d'", node.uInt2);
			comma = true;
		}
		if (node.flagB & MUInt32_3) {
			if (comma) query.writeStr(commaStr);
			query.printf("uint_3='%d'", node.uInt3);
			comma = true;
		}
		if (node.flagB & MUInt32_4) {
			if (comma) query.writeStr(commaStr);
			query.printf("uint_4='%d'", node.uInt4);
			comma = true;
		}
		if (node.flagB & MStr64_1) {
			if (comma) query.writeStr(commaStr);
			query.printf("str_1='%s'", sql->escape(node.str1).c_str());
			comma = true;
		}
		if (node.flagB & MStr64_2) {
			if (comma) query.writeStr(commaStr);
			query.printf("str_2='%s'", sql->escape(node.str2).c_str());
			comma = true;
		}
		if (node.flagB & MStr64_3) {
			if (comma) query.writeStr(commaStr);
			query.printf("str_3='%s'", sql->escape(node.str3).c_str());
			comma = true;
		}
		if (node.flagB & MStr64_4) {
			if (comma) query.writeStr(commaStr);
			query.printf("str_4='%s'", sql->escape(node.str4).c_str());
			comma = true;
		}
		if (node.flagB & MStr64_5) {
			if (comma) query.writeStr(commaStr);
			query.printf("str_5='%s'", sql->escape(node.str5).c_str());
			comma = true;
		}
		if (node.flagB & MStr64_6) {
			if (comma) query.writeStr(commaStr);
			query.printf("str_6='%s'", sql->escape(node.str6).c_str());
			comma = true;
		}
		if (node.flagB & MlStr64_1) {
			if (comma) query.writeStr(commaStr);
			query.printf("lstr_1='%s'", sql->escape(node.lStr1).c_str());
			comma = true;
		}
		if (node.flagB & MlStr64_2) {
			if (comma) query.writeStr(commaStr);
			query.printf("lstr_2='%s'", sql->escape(node.lStr2).c_str());
			comma = true;
		}
		if (node.flagB & MText_1) {
			if (comma) query.writeStr(commaStr);
			query.printf("text_1='%s'", sql->escape(node.text1).c_str());
			comma = true;
		}
		if (node.flagB & MText_2) {
			if (comma) query.writeStr(commaStr);
			query.printf("text_2='%s'", sql->escape(node.text2).c_str());
			comma = true;
		}
		if (isUpdate && node.flagB & MBlob1) { // only insert this if it is an UPDATE
			if (comma) query.writeStr(commaStr);
			query.writeStr("blob_1='");
			if (node.blob1.size())
				query.writeStr(sql->escape(node.blob1));
			query.writeStr( "'");
			comma = true;
		}
		
		return query;
	}
	
	uint32_t tVaultDB::findNode(tvNode &node, bool create, tvManifest *mfs)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		tString query;
		// first, we have to create the query...
		query.printf("SELECT idx, mod_time FROM %s WHERE ", vaultTable);
		query.writeStr(createNodeQuery(node, /*isUpdate*/false));
		
		// now, let's execute it
		sql->query(query, "finding node");
		MYSQL_RES *result = sql->storeResult();
		size_t number = mysql_num_rows(result);
		if (number > 1) throw txDatabaseError(_WHERE("strange, I should NEVER have several results when asking for a node"));
		
		uint32_t id = 0;
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
	
	uint32_t tVaultDB::createNode(tvNode &node)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		if (!(node.flagB & MType)) throw txDatabaseError(_WHERE("type must be set for all new nodes"));
		
		tString query, values;
		
		// set current time
		node.flagB |= (MModTime | MCrtTime | MAgeTime);
		node.modTime = tTime::now().asDouble();
		node.crtTime = node.ageTime = time(NULL);
		
		query.printf("INSERT INTO %s (type", vaultTable);
		values.printf(") VALUES ('%d'", node.type);
		
		if (node.flagB & MPerms) {
			query.writeStr(",permissions");
			values.printf(",'%d'", node.permissions);
		}
		if (node.flagB & MOwner) {
			query.writeStr(",owner");
			values.printf(",'%d'", node.owner);
		}
		if (node.flagB & MGroup) {
			query.writeStr(",grp");
			values.printf(",'%d'", node.group);
		}
		if (node.flagB & MModTime) {
			query.writeStr(",mod_time");
			values.printf(",'%f'", node.modTime);
		}
		if (node.flagB & MCreator) {
			query.writeStr(",creator");
			values.printf(",'%d'", node.creator);
		}
		if (node.flagB & MCrtTime) {
			query.writeStr(",crt_time");
			values.printf(",FROM_UNIXTIME('%d')", node.crtTime);
		}
		if (node.flagB & MAgeTime) {
			query.writeStr(",age_time");
			values.printf(",FROM_UNIXTIME('%d')", node.ageTime);
		}
		if (node.flagB & MAgeName) {
			query.writeStr(",age_name");
			values.printf(",'%s'", sql->escape(node.ageName).c_str());
		}
		if (node.flagB & MAgeGuid) {
			query.writeStr(",age_guid");
			values.printf(",'%s'", sql->escape(alcGetStrGuid(node.ageGuid)).c_str());
		}
		if (node.flagB & MInt32_1) {
			query.writeStr(",int_1");
			values.printf(",'%d'", node.int1);
		}
		if (node.flagB & MInt32_2) {
			query.writeStr(",int_2");
			values.printf(",'%d'", node.int2);
		}
		if (node.flagB & MInt32_3) {
			query.writeStr(",int_3");
			values.printf(",'%d'", node.int3);
		}
		if (node.flagB & MInt32_4) {
			query.writeStr(",int_4");
			values.printf(",'%d'", node.int4);
		}
		if (node.flagB & MUInt32_1) {
			query.writeStr(",uint_1");
			values.printf(",'%d'", node.uInt1);
		}
		if (node.flagB & MUInt32_2) {
			query.writeStr(",uint_2");
			values.printf(",'%d'", node.uInt2);
		}
		if (node.flagB & MUInt32_3) {
			query.writeStr(",uint_3");
			values.printf(",'%d'", node.uInt3);
		}
		if (node.flagB & MUInt32_4) {
			query.writeStr(",uint_4");
			values.printf(",'%d'", node.uInt4);
		}
		if (node.flagB & MStr64_1) {
			query.writeStr(",str_1");
			values.printf(",'%s'", sql->escape(node.str1).c_str());
		}
		if (node.flagB & MStr64_2) {
			query.writeStr(",str_2");
			values.printf(",'%s'", sql->escape(node.str2).c_str());
		}
		if (node.flagB & MStr64_3) {
			query.writeStr(",str_3");
			values.printf(",'%s'", sql->escape(node.str3).c_str());
		}
		if (node.flagB & MStr64_4) {
			query.writeStr(",str_4");
			values.printf(",'%s'", sql->escape(node.str4).c_str());
		}
		if (node.flagB & MStr64_5) {
			query.writeStr(",str_5");
			values.printf(",'%s'", sql->escape(node.str5).c_str());
		}
		if (node.flagB & MStr64_6) {
			query.writeStr(",str_6");
			values.printf(",'%s'", sql->escape(node.str6).c_str());
		}
		if (node.flagB & MlStr64_1) {
			query.writeStr(",lstr_1");
			values.printf(",'%s'", sql->escape(node.lStr1).c_str());
		}
		if (node.flagB & MlStr64_2) {
			query.writeStr(",lstr_2");
			values.printf(",'%s'", sql->escape(node.lStr2).c_str());
		}
		if (node.flagB & MText_1) {
			query.writeStr(",text_1");
			values.printf(",'%s'", sql->escape(node.text1).c_str());
		}
		if (node.flagB & MText_2) {
			query.writeStr(",text_2");
			values.printf(",'%s'", sql->escape(node.text2).c_str());
		}
		if (node.flagB & MBlob1) {
			query.writeStr(",blob_1");
			values.writeStr(",'");
			if (node.blob1.size())
				values.writeStr(sql->escape(node.blob1));
			values.writeStr("'");
		}
		
		// Now compose the two parts
		query.writeStr(values);
		query.writeStr(")");
		sql->query(query, "Inserting new node");
		
		return sql->insertId();
	}
	
	uint32_t tVaultDB::createChildNode(uint32_t saver, uint32_t parent, alc::tvNode& node)
	{
		uint32_t nodeId = createNode(node);
		tvNodeRef ref(saver, parent, nodeId);
		addNodeRef(ref);
		return nodeId;
	}
	
	void tVaultDB::updateNode(tvNode &node)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		// set current time
		node.flagB |= MModTime;
		node.modTime = tTime::now().asDouble();
		
		// create the query
		tString query;
		query.printf("UPDATE %s SET ", vaultTable);
		query.writeStr(createNodeQuery(node, /*isUpdate*/true));
		query.printf(" WHERE idx='%d'", node.index);
		
		sql->query(query, "Updating vault node", true);
	}
	
	void tVaultDB::getManifest(uint32_t baseNode, alc::tvManifest*** mfs, size_t* nMfs, alc::tvNodeRef*** ref, size_t* nRef)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		*mfs = NULL;
		*nMfs = 0;
		*ref = NULL;
		*nRef = 0;
	
		tvManifest **feed = NULL, **aux = NULL, **final = NULL;
		size_t      nFeed = 0,     nAux = 0,     nFinal = 0;
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		bool comma;
		size_t auxIndex, feedIndex;
		
		// get base node
		query.printf("SELECT idx, mod_time FROM %s WHERE idx='%d' LIMIT 1", vaultTable, baseNode);
		sql->query(query, "getManifest: getting first node");
		
		result = sql->storeResult();
		size_t number = mysql_num_rows(result);
		if (number > 1) throw txDatabaseError(_WHERE("strange, I should NEVER have several results when asking for a node"));
		else if (number < 1) throw txDatabaseError(_WHERE("getManfiest: First node %d does not exist", baseNode));
		
		// save it in the feed
		row = mysql_fetch_row(result);
		feed = static_cast<tvManifest **>(malloc(sizeof(tvManifest *)));
		if (feed == NULL) throw txNoMem(_WHERE("NoMem"));
		feed[0] = new tvManifest(atoi(row[0]), atof(row[1]));
		nFeed = 1;
		mysql_free_result(result);
		
		// now, the big loop... as long as there is something in the feed, process it
		while (nFeed > 0) {
			// save the so-far final list
			aux = final;
			nAux = nFinal;
			// get space for the new final list - it will contain all elements of the so-far final list and the feed
			final = static_cast<tvManifest **>(malloc((nFinal+nFeed)*sizeof(tvManifest *)));
			if (final == NULL) throw txNoMem(_WHERE("NoMem"));
			nFinal = 0;
			
			query.clear();
			query.printf("SELECT n.idx, n.mod_time, r.id1, r.id2, UNIX_TIMESTAMP(r.timestamp), r.microseconds, r.flag FROM %s n JOIN %s r ON r.id3=n.idx WHERE r.id2 IN(", vaultTable, refVaultTable);
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
							if (comma) query.writeStr(",");
							query.printf("%d", feed[feedIndex]->id);
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
					if (comma) query.writeStr(",");
					query.printf("%d", feed[feedIndex]->id);
					comma = true;
				}
				else
					delete feed[feedIndex]; // duplicate
				++feedIndex; // got this one
			}
			
			// now we can free the aux and feed tables
			free(aux);
			aux = NULL;
			nAux = 0;
			free(feed);
			feed = NULL;
			nFeed = 0;
			
			// ok... now we can query (if there's anything)
			if (!comma) break;
			query.writeStr(") ORDER BY n.idx ASC");
			sql->query(query, "getManifest: getting child nodes");
			
			result = sql->storeResult();
			size_t number = mysql_num_rows(result);
				
			// the result goes into the feed table, and the refs are saved in their table as well
			feed = static_cast<tvManifest **>(malloc(number*sizeof(tvManifest *)));
			*ref = static_cast<tvNodeRef **>(realloc(*ref, (*nRef + number)*sizeof(tvNodeRef *)));
			if (feed == NULL || *ref == NULL) throw txNoMem(_WHERE("NoMem"));
			while (nFeed < number) {
				row = mysql_fetch_row(result);
				// save manifest
				uint32_t idx = atoi(row[0]);
				feed[nFeed] = new tvManifest(idx, atof(row[1]));
				++nFeed;
				// and reference
				(*ref)[*nRef] = new tvNodeRef(atoi(row[2]), atoi(row[3]), idx, atoi(row[4]), atoi(row[5]), atoi(row[6]));
				++(*nRef);
			}
			mysql_free_result(result);
		}
		
		// now we can free the aux and feed tables
		free(aux);
		aux = NULL;
		free(feed);
		feed = NULL;
		
		// the final list is what the caller gets
		*mfs = final;
		*nMfs = nFinal;
	}
	
	void tVaultDB::getMGRs(uint32_t baseNode, uint32_t** table, size_t* tableSize)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		*table = NULL;
		*tableSize = 0;
	
		uint32_t *feed = NULL, *aux = NULL, *final = NULL;
		size_t   nFeed = 0,    nAux = 0,    nFinal = 0;
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		bool comma;
		size_t auxIndex, feedIndex;
		
		// put base node in the feed
		feed = static_cast<uint32_t *>(malloc(sizeof(uint32_t)));
		if (feed == NULL) throw txNoMem(_WHERE("NoMem"));
		feed[0] = baseNode;
		nFeed = 1;
		
		// check if the node we're talking about is a MGR itself (it has to be added ti the list then)
		query.printf("SELECT type FROM %s WHERE idx='%d' LIMIT 1", vaultTable, baseNode);
		sql->query(query, "getMGRs: node type");
		result = sql->storeResult();
		size_t num = mysql_num_rows(result);
		if (num != 1) throw txDatabaseError(_WHERE("couldn't find base node %d", baseNode));
		
		row = mysql_fetch_row(result);
		int type = atoi(row[0]);
		mysql_free_result(result);
		if (type <= 7) {
			// it's a MGR, add it to the list
			*tableSize = 1;
			*table = static_cast<uint32_t *>(malloc(sizeof(uint32_t)));
			if (*table == NULL) throw txNoMem(_WHERE("NoMem"));
			(*table)[0] = baseNode;
		}
		
		// now, the big loop... as long as there is something in the feed, process it
		while (nFeed > 0) {
			// save the so-far final list
			aux = final;
			nAux = nFinal;
			// get space for the new final list - it will contain all elements of the so-far final list and the feed
			final = static_cast<uint32_t *>(malloc((nFinal+nFeed)*sizeof(uint32_t)));
			if (final == NULL) throw txNoMem(_WHERE("NoMem"));
			nFinal = 0;
			
			query.clear();
			query.printf("SELECT n.idx,n.type FROM %s n JOIN %s r ON r.id2=n.idx WHERE r.id3 IN(", vaultTable, refVaultTable);
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
							if (comma) query.writeStr(",");
							query.printf("%d", feed[feedIndex]);
							comma = true;
						}
						else
							{} // duplicate
					}
					else if (feed[feedIndex] > aux[auxIndex]) {
						// the current feed must be inserted into the final list after the current aux, so we can insert the current aux now
						final[nFinal] = aux[auxIndex];
						++nFinal;
						++auxIndex;
						break; // we have to check if this was the last aux node or there's something after it
					}
					else
						{} // duplicate
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
					if (comma) query.writeStr(",");
					query.printf("%d", feed[feedIndex]);
					comma = true;
				}
				else
					{} // duplicate
				++feedIndex; // got this one
			}
			DBG(9, "Completed the new lists\n");
			
			// now we can free the aux and feed tables
			free(aux);
			aux = NULL;
			nAux = 0;
			free(feed);
			feed = NULL;
			nFeed = 0;
			
			// ok... now we can query (if there's anything)
			if (!comma) break;
			query.writeStr(") ORDER BY n.idx ASC");
			sql->query(query, "getMGRs: getting parent nodes");
			
			result = sql->storeResult();
			size_t number = mysql_num_rows(result);
			DBG(9, "Got %zd parent nodes, adding them to table\n", number);
				
			// the result goes into the feed table
			feed = static_cast<uint32_t *>(malloc(number*sizeof(uint32_t)));
			if (feed == NULL) throw txNoMem(_WHERE("NoMem"));
			while (nFeed < number) {
				row = mysql_fetch_row(result);
				// save ID
				uint32_t idx = atoi(row[0]);
				feed[nFeed] = idx;
				++nFeed;
				if (atoi(row[1]) <= KVNodeMgrMAX) { // it's a MGR so lets save it - and keep the table in order!
					DBG(9, "%d is a MGR, adding it\n", idx);
					uint32_t insertVal = idx, tmp;
					for (size_t i = 0; i < *tableSize; ++i) {
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
						*table = static_cast<uint32_t *>(realloc(*table, (*tableSize)*sizeof(uint32_t)));
						if (*table == NULL) throw txNoMem(_WHERE("NoMem"));
						(*table)[*tableSize-1] = insertVal;
					}
				}
			}
			mysql_free_result(result);
		}
		DBG(9, "That was the last loop\n");
		
		// now we can free the aux and feed tables
		free(aux);
		aux = NULL;
		free(feed);
		feed = NULL;
		free(final);
		final = NULL;
	}
	
	void tVaultDB::fetchNodes(alc::tMBuf& buf, size_t tableSize, alc::tvNode*** nodes, size_t* nNodes)
	{
		if (!tableSize)
			throw txDatabaseError(_WHERE("There must be at least one node to fetch"));
		uint32_t *table = new uint32_t[tableSize];
		buf.rewind();
		for (size_t i = 0; i < tableSize; ++i) table[i] = buf.get32();
		fetchNodes(table, tableSize, nodes, nNodes);
		delete[] table;
	}
	
	void tVaultDB::fetchNodes(uint32_t* table, size_t tableSize, alc::tvNode*** nodes, size_t* nNodes)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		if (!tableSize)
			throw txDatabaseError(_WHERE("There must be at least one node to fetch"));
	
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		unsigned long *lengths; // the len of each column
		*nodes = NULL;
		*nNodes = 0;
		
		query.printf("SELECT idx, type, permissions, owner, grp, mod_time, creator, UNIX_TIMESTAMP(crt_time), UNIX_TIMESTAMP(age_time), age_name, age_guid, int_1, int_2, int_3, int_4, uint_1, uint_2, uint_3, uint_4, str_1, str_2, str_3, str_4, str_5, str_6, lstr_1, lstr_2, text_1, text_2, blob_1 FROM %s WHERE idx IN(", vaultTable);
		
		for (size_t i = 0; i < tableSize; ++i) {
			if (i > 0) query.writeStr(",");
			query.printf("%d", table[i]);
		}
		
		query.writeStr(")");
		sql->query(query, "fetching nodes");
		
		result = sql->storeResult();
		*nNodes = mysql_num_rows(result);
		
		*nodes = static_cast<tvNode **>(malloc((*nNodes)*sizeof(tvNode *)));
		if (*nodes == NULL) throw txNoMem(_WHERE("NoMem"));
		for (size_t i = 0; i < *nNodes; ++i) {
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result); //get the size of each column
			
			tvNode *node = new tvNode;
			node->flagB = 0xFFFFFFFF; // everything enabled
			node->flagC = 0x00000007; // everything enabled
			// fill in the row
			node->index = atoi(row[0]);
			node->type = atoi(row[1]);
			node->permissions = atoi(row[2]);
			node->owner = atoi(row[3]);
			node->group = atoi(row[4]);
			node->modTime = atof(row[5]);
			node->creator = atoi(row[6]);
			node->crtTime = atoi(row[7]);
			node->ageTime = atoi(row[8]);
			node->ageName = row[9];
			if (strlen(row[10]) == 16)
				alcGetHexGuid(node->ageGuid, tString(row[10]));
			else
				memset(node->ageGuid, 0, 8);
			node->int1 = atoi(row[11]);
			node->int2 = atoi(row[12]);
			node->int3 = atoi(row[13]);
			node->int4 = atoi(row[14]);
			node->uInt1 = atoi(row[15]);
			node->uInt2 = atoi(row[16]);
			node->uInt3 = atoi(row[17]);
			node->uInt4 = atoi(row[18]);
			node->str1 = row[19];
			node->str2 = row[20];
			node->str3 = row[21];
			node->str4 = row[22];
			node->str5 = row[23];
			node->str6 = row[24];
			node->lStr1 = row[25];
			node->lStr2 = row[26];
			if (lengths[27]) node->text1 = row[27];
			if (lengths[28]) node->text2 = row[28];
			node->blob1.write(row[29], lengths[29]);
			
			(*nodes)[i] = node;
		}
		mysql_free_result(result);
	}
	
	bool tVaultDB::checkNode(uint32_t node)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		tString query;
		query.printf("SELECT idx FROM %s WHERE idx='%d'", vaultTable, node);
		return (sql->queryForNumber(query, "checkNode") == 1);
	}
	
	bool tVaultDB::addNodeRef(tvNodeRef &ref)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		tString query;
		
		// first check if this ref already exists
		query.printf("SELECT id1 FROM %s WHERE id2='%d' AND id3='%d' LIMIT 1", refVaultTable, ref.parent, ref.child);
		bool exists = sql->queryForNumber(query, "addNodeRef: checking for ref");
		if (exists) return false;
		
		// set current time
		tTime t = tTime::now();
		ref.time = t.seconds;
		ref.microsec = t.microseconds;
		
		query.clear();
		query.printf("INSERT INTO %s (id1, id2, id3, timestamp, microseconds, flag) VALUES('%d', '%d', '%d', FROM_UNIXTIME('%d'), '%d', '%d')", refVaultTable, ref.saver, ref.parent, ref.child, ref.time, ref.microsec, ref.flags);
		sql->query(query, "Creating new node ref");
		return true;
	}
	
	void tVaultDB::removeNodeRef(uint32_t parent, uint32_t son, bool cautious)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
	
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		int numParent, type;
		size_t num;
		
		// get number of parent nodes
		query.printf("SELECT COUNT(*) FROM %s WHERE id3='%d'", refVaultTable, son);
		sql->query(query, "removeNodeRef: number of parent nodes");
		result = sql->storeResult();
		num = mysql_num_rows(result);
		if (num != 1) throw txDatabaseError(_WHERE("couldn't get number of parent nodes of %d", son));
		
		row = mysql_fetch_row(result);
		numParent = atoi(row[0]);
		mysql_free_result(result);
		
		// get node type
		query.clear();
		query.printf("SELECT type FROM %s WHERE idx='%d' LIMIT 1", vaultTable, son);
		sql->query(query, "removeNodeRef: node type");
		result = sql->storeResult();
		
		num = mysql_num_rows(result);
		if (num == 1) {
			row = mysql_fetch_row(result);
			type = atoi(row[0]);
		}
		else
			type = KInvalidNode; // don't fail when son of a ref we have to remove doesn't exist - this seems to happen sometimes
		mysql_free_result(result);
		
		bool safeType = cautious ? (type == KImageNode || type == KTextNoteNode || type == KChronicleNode
				|| type == KMarkerListNode || type == KMarkerNode) : (type > KVNodeMgrMAX);
		if (type != KInvalidNode && numParent <= 1 && safeType) {
			// there are no more references to this node, and it's a safe node to remove
			removeNodeTree(son, cautious);
		}
		else { // only remove this ref
			query.clear();
			query.printf("DELETE FROM %s WHERE id2='%d' AND id3='%d'", refVaultTable, parent, son);
			sql->query(query, "removeNodeRef: removing a node reference");
		}
	}
	
	void tVaultDB::removeNodeTree(uint32_t node, bool cautious)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		size_t num;
		
		// remove the node and all references to it
		query.printf("DELETE FROM %s WHERE idx='%d'", vaultTable, node);
		sql->query(query, "removeNodeTree: removing a node");
		query.clear();
		query.printf("DELETE FROM %s WHERE id3='%d'", refVaultTable, node);
		sql->query(query, "removeNodeTree: removing all references to a node");
		
		// get all references from this node and remove them
		// we do this using recursion since doing it with a loop would make things more complicated without saving database queries
		query.clear();
		query.printf("SELECT id3 FROM %s WHERE id2='%d'", refVaultTable, node);
		sql->query(query, "removeNodeTree: getting child nodes");
		result = sql->storeResult();
		num = mysql_num_rows(result);
		
		for (size_t i = 0; i < num; ++i) {
			row = mysql_fetch_row(result);
			removeNodeRef(node, atoi(row[0]), cautious);
		}
		mysql_free_result(result);
	}
	
	void tVaultDB::setSeen(uint32_t parent, uint32_t son, uint32_t seen)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		tString query;
		query.printf("UPDATE %s SET flag='%d' WHERE id2='%d' AND id3='%d'", refVaultTable, seen, parent, son);
		sql->query(query, "Updating seen flag");
	}
	
	void tVaultDB::getParentNodes(uint32_t node, uint32_t** table, size_t* tableSize)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		
		*table = NULL;
		*tableSize = 0;
		
		query.printf("SELECT id2 FROM %s WHERE id3='%d'", refVaultTable, node);
		sql->query(query, "getting parent nodes");
		result = sql->storeResult();
		*tableSize = mysql_num_rows(result);
		
		*table = static_cast<uint32_t *>(malloc((*tableSize)*sizeof(uint32_t)));
		if (*table == NULL) throw txNoMem(_WHERE("NoMem"));
		for (size_t i = 0; i < *tableSize; ++i) {
			row = mysql_fetch_row(result);
			(*table)[i] = atoi(row[0]);
		}
		mysql_free_result(result);
	}
	
	void tVaultDB::getReferences(uint32_t node, alc::tvNodeRef*** ref, size_t* nRef)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		
		*ref = NULL;
		*nRef = 0;
		
		size_t i = 0;
		while (true) { // condition is at the end
			query.clear();
			query.printf("SELECT id1, id2, id3, UNIX_TIMESTAMP(timestamp), microseconds, flag FROM %s WHERE id2='%d'", refVaultTable, node);
			sql->query(query, "getting references");
			result = sql->storeResult();
			size_t newSize = *nRef + mysql_num_rows(result);
			
			if (newSize > *nRef) { // found new references, add to input
				*ref = static_cast<tvNodeRef **>(realloc(*ref, newSize*sizeof(tvNodeRef *)));
				if (*ref == NULL) throw txNoMem(_WHERE("NoMem"));
				for (size_t j = *nRef; j < newSize; ++j) {
					row = mysql_fetch_row(result);
					uint32_t parent = atoi(row[1]), child = atoi(row[2]);
					// check for duplicate (i.e. anti-loop-protection)
					bool found = false;
					for (size_t k = 0; k < *nRef; ++k) { // searching the already existing nodes is enough
						if ((*ref)[k]->parent == parent && ((*ref)[k]->child == child)) {
							alcGetMain()->err()->log("tVaultDB::getReferences: loop in vault structure detected, found referece %d->%d twice\n", parent, child);
							found = true;
						}
					}
					if (!found) {
						(*ref)[*nRef] = new tvNodeRef(atoi(row[0]), parent, child, atoi(row[3]), atoi(row[4]), atoi(row[5]));
						++(*nRef);
					}
				}
			}
			mysql_free_result(result);
			
			if (i >= *nRef) break; // all references done
			node = (*ref)[i]->child; // this is the next one
			++i;
		}
	}
	
	void tVaultDB::getAgeInfos(uint32_t parent, tString ageName, tvNode ***nodes, size_t *size)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		*nodes = NULL;
		*size = 0;
		
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		
		// get age info nodes with this age
		query.printf("SELECT n.idx FROM %s r JOIN %s n ON n.idx = r.id3 WHERE r.id2 = '%d' and n.type = '%d' and n.str_1 = '%s'",
					 refVaultTable, vaultTable, parent, KAgeInfoNode, sql->escape(ageName).c_str());
		sql->query(query, "getAgeInfos: Finding ages");
		result = sql->storeResult();
		size_t n = mysql_num_rows(result);
		uint32_t *table = new uint32_t[n];
		for (size_t i = 0; i < n; ++i) {
			row = mysql_fetch_row(result);
			table[i] = atoi(row[0]);
		}
		mysql_free_result(result);
		
		// fetch node conents (fetching zero nodes is an error)
		if (n > 0)
			fetchNodes(table, n, nodes, size);
		delete[] table;
	}
	
	void tVaultDB::removeInvalidRefs(void)
	{
		// this is a private function, so the caller already did the prepare() check
		
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		int num;
		
		// find invalid references and remove them
		// first: invalid parent
		query.printf("SELECT r.id2, r.id3 FROM %s r LEFT JOIN %s n ON n.idx = r.id2 WHERE n.idx IS NULL", refVaultTable, vaultTable);
		sql->query(query, "tVaultDB::removeInvalidRefs: Finding references with invalid parent");
		result = sql->storeResult();
		
		num = mysql_num_rows(result);
		if (num > 0) {
			alcGetMain()->err()->log("WARNING: Found %d references with invalid parent - removing them...\n", num);
			for (int i = 0; i < num; ++i) {
				row = mysql_fetch_row(result);
				query.clear();
				query.printf("DELETE FROM %s WHERE id2='%s' AND id3='%s'", refVaultTable, row[0], row[1]);
				sql->query(query, "tVaultDB::removeInvalidRefs: Removing reference with invalid parent");
			}
		}
		mysql_free_result(result);
		
		// then: invalid son
		query.clear();
		query.printf("SELECT r.id2, r.id3 FROM %s r LEFT JOIN %s n ON n.idx = r.id3 WHERE n.idx IS NULL", refVaultTable, vaultTable);
		sql->query(query, "tVaultDB::removeInvalidRefs: Finding references with invalid son");
		result = sql->storeResult();
		
		num = mysql_num_rows(result);
		if (num > 0) {
			alcGetMain()->err()->log("WARNING: Found %d references with invalid son - removing them...\n", num);
			for (int i = 0; i < num; ++i) {
				row = mysql_fetch_row(result);
				query.clear();
				query.printf("DELETE FROM %s WHERE id2='%s' AND id3='%s'", refVaultTable, row[0], row[1]);
				sql->query(query, "tVaultDB::removeInvalidRefs: Removing reference with invalid son");
			}
		}
		mysql_free_result(result);
	}
	
	bool tVaultDB::isLostAge(int id)
	{
		// this is a private function, so the caller already did the prepare() check
		
		// look for age info node (which must be a direct child)
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		
		int ageInfo = 0;
		
		// check if there is an age info node
		query.printf("SELECT n.idx FROM %s n LEFT JOIN %s r ON n.idx = r.id3 WHERE r.id2 = '%d' AND n.type='%d' LIMIT 1", vaultTable, refVaultTable, id, KAgeInfoNode);
		sql->query(query, "tVaultDB::isLostAge: Looking for age info node");
		result = sql->storeResult();
		int num = mysql_num_rows(result);
		
		if (num == 1) {
			row = mysql_fetch_row(result);
			ageInfo = atoi(row[0]);
		}
		mysql_free_result(result);
		if (!num || !ageInfo) return true; // no age info node, the age is lost
		
		// check if the age info node is referenced from anywhere else
		query.clear();
		query.printf("SELECT id2 FROM %s WHERE id3 = '%d' LIMIT 2", refVaultTable, ageInfo);
		num = sql->queryForNumber(query, "tVaultDB::isLostAge: Looking for references to age info node");
		if (num < 1) throw txDatabaseError(_WHERE("First I found the node, then there's no reference to it? This can't happen"));
		return (num == 1); // if there's only one reference, the age is lost, otherwise, it isn't
	}
	
	void tVaultDB::clean(bool cleanAges)
	{
		if (!sql->prepare()) throw txDatabaseError(_WHERE("no access to DB"));
		
		tString query;
		MYSQL_RES *result;
		MYSQL_ROW row;
		
		removeInvalidRefs();
		
		// if enabled, remove lost ages
		// NOTE: Ages linked to with KBasicLink are not referenced anywhere, so they are removed by this
		if (cleanAges) {
			alcGetMain()->std()->log("Cleaning up: Looking for lost ages...\n");
		
			query.clear();
			query.printf("SELECT idx, str_1 FROM %s WHERE type = '%d'", vaultTable, KVNodeMgrAgeNode);
			sql->query(query, "tVaultDB::clean: Finding age MGRs");
			result = sql->storeResult();
			int num = mysql_num_rows(result);
			
			for (int i = 0; i < num; ++i) {
				row = mysql_fetch_row(result);
				int id = atoi(row[0]);
				if (isLostAge(id)) {
					alcGetMain()->std()->log("Age MGR %s is lost, removing it...\n", row[1]);
					removeNodeTree(id, /*cautious*/false);
				}
			}
			mysql_free_result(result);
		}
		
		// check for the admin node - if it does not exist, the AllPlayers folder has no parent and we can not remove lost nodes
		// (the vault server now creates it automatically when initializing a new vault, but older versions did not do that)
		tvNode node(MType);
		node.type = KVNodeMgrAdminNode;
		int adminNode = findNode(node);
		if (!adminNode) {
			alcGetMain()->err()->print("\n\nWARNING: You have no admin node in your vault, so I can't remove lost nodes as that would destroy your vault.\n");
			alcGetMain()->err()->print("Please log in once with the VaultManager (which will create that node) and try again.\n\n");
		}
		else {
			alcGetMain()->std()->log("Cleaning up: Looking for lost nodes...\n");
			// find lost nodes and remove them
			// a lost node is a node without a parent and which is not a mgr
			query.clear();
			query.printf("SELECT n.idx FROM %s n LEFT JOIN %s r ON n.idx = r.id3 "
				"WHERE r.id2 IS NULL AND n.type > 7", vaultTable, refVaultTable);
			sql->query(query, "tVaultDB::clean: Finding lost nodes");
			result = sql->storeResult();
			int num = mysql_num_rows(result);
			
			if (num > 0) {
				alcGetMain()->std()->log("Cleaning up: Removing %d lost nodes...\n", num);
				for (int i = 0; i < num; ++i) {
					row = mysql_fetch_row(result);
					removeNodeTree(atoi(row[0]), /*cautious*/false);
				}
			}
			mysql_free_result(result);
		}
		
		// optimize the tables
		alcGetMain()->std()->log("Cleaning up: Optimizing tables...\n");
		query.clear();
		query.printf("OPTIMIZE TABLE %s, %s", refVaultTable, vaultTable);
		sql->query(query, "Optimizing tables");
	}

} //end namespace alc
