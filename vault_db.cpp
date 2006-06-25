/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs H'uru Server Team                     *
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


// The vault

/*
	this will contain all related with using the vault, mysql access
*/

/* CVS tag - DON'T TOUCH*/
#define __U_VAULT_DB_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include <mysql/mysql.h>

#include "data_types.h" //data types
#include "sql.h" //mysql calls
#include "vaultstrs.h" //vault structs
#include "conv_funs.h"
#include "stdebug.h"
#include "useful.h"
#include "version.h"
#include "vaultstrs.h"
#include "urustructs.h"
#include "vnodes.h"

#include "urunet.h"
#include "vault_tasks.h" //now we need it to perform vault migrations on initizalization
#include "guid_gen.h"

#include "vault_db.h"

#include "debug.h"

const int vault_version=2; //Please only change on major vault format changes, and be sure that
//the plVaultMigrate has implementation for the required changes!!!
/* Version history:
		0 -> old very old format
		1 -> last version
		2 -> extremely experimental, adds DniCityX2Finale to allow end game sequence play
		//NOTE THIS WILL HAVE UNEXPECTED AND UNWANTED RESULTS ON NON TPOTS CLIENTS!!!
*/

extern char * ghood_desc;
extern char * ghood_name;
//We are not including vserversys.cpp for a really good reason, it's intentional, so please
// don't destroy this adding the include.

const char * vault_table = "vault";
const char * ref_vault_table = "ref_vault";

const char * text1 = "You must Never delete or edit this node!";
const char * text2 = "The H\\'uru alternative server project.";

const char * db_vault_table_init_script = "\
CREATE TABLE `vault` (\
  `idx` int(11) NOT NULL auto_increment,\
  `type` tinyint(1) unsigned NOT NULL default '0',\
  `permissions` int(11) NOT NULL default '0',\
  `owner` int(11) NOT NULL default '0',\
  `unk1` int(11) NOT NULL default '0',\
  `timestamp` int(11) NOT NULL default '0',\
  `microseconds` int(11) NOT NULL default '0',\
  `id1` int(11) NOT NULL default '0',\
  `timestamp2` int(11) NOT NULL default '0',\
  `microseconds2` int(11) NOT NULL default '0',\
  `timestamp3` int(11) NOT NULL default '0',\
  `microseconds3` int(11) NOT NULL default '0',\
  `age_name` varchar(255) NOT NULL default '',\
  `age_guid` varchar(16) NOT NULL default '',\
  `torans` int(11) NOT NULL default '0',\
  `distance` int(11) NOT NULL default '0',\
  `elevation` int(11) NOT NULL default '0',\
  `unk5` int(11) NOT NULL default '0',\
  `id2` int(11) NOT NULL default '0',\
  `unk7` int(11) NOT NULL default '0',\
  `unk8` int(11) NOT NULL default '0',\
  `unk9` int(11) NOT NULL default '0',\
  `entry_name` varchar(255) NOT NULL default '',\
  `sub_entry_name` varchar(255) NOT NULL default '',\
  `owner_name` varchar(255) NOT NULL default '',\
  `guid` varchar(255) NOT NULL default '',\
  `str1` varchar(255) NOT NULL default '',\
  `str2` varchar(255) NOT NULL default '',\
  `avie` varchar(255) NOT NULL default '',\
  `uid` varchar(255) NOT NULL default '',\
  `entry_value` varchar(255) NOT NULL default '',\
  `entry2` varchar(255) NOT NULL default '',\
  `data` longblob NOT NULL,\
  `data2` longblob NOT NULL,\
  `unk13` int(11) NOT NULL default '0',\
  `unk14` int(11) NOT NULL default '0',\
  PRIMARY KEY  (`idx`),\
  KEY `type` (`type`),\
  KEY `owner` (`owner`),\
  KEY `id1` (`id1`),\
  KEY `age_name` (`age_name`),\
  KEY `age_guid` (`age_guid`),\
  KEY `torans` (`torans`),\
  KEY `guid` (`guid`),\
  KEY `avie` (`avie`),\
  KEY `uid` (`uid`)\
) TYPE=MyISAM PACK_KEYS=0 AUTO_INCREMENT=20001 ;\
";

/* //removed from version 1
  `data_size` int(11) NOT NULL default '0',\
  `data2_size` int(11) NOT NULL default '0',\
  `unk15` int(11) NOT NULL default '0',\
  `unk16` int(11) NOT NULL default '0',\
*/

const char * db_ref_vault_table_init_script="\
CREATE TABLE `ref_vault` (\
  `id1` int(11) NOT NULL default '0',\
  `id2` int(11) NOT NULL default '0',\
  `id3` int(11) NOT NULL default '0',\
  `timestamp` int(11) NOT NULL default '0',\
  `microseconds` int(11) NOT NULL default '0',\
  `flag` tinyint(1) unsigned NOT NULL default '0',\
  PRIMARY KEY  (`id2`,`id3`),\
  KEY `id2` (`id2`),\
  KEY `id3` (`id3`)\
) TYPE=MyISAM;\
";

//Migration to version 2
/*
ALTER TABLE `vault` DROP `data_size`
ALTER TABLE `vault` DROP `data2_size`
ALTER TABLE `vault` DROP `unk15`
ALTER TABLE `vault` DROP `unk16`
*/

//Migration to version 3
/*
 There is a new table for the blobs. And unk13 a& unk14 are the index of the blob.
*/

/**
  Query the database for the players list
	returns -1 on db error
*/
int plVaultQueryPlayerList(Byte * guid,Byte ** data,int * size,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[1024];
	int i,ret,off=0; //for store result codes
	Byte * buf;

	*data=NULL;

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return -1; }

	sprintf(query,"Select v.idx, v.avie,v.distance\
 from %s v where v.uid='%s';",vault_table,create_str_guid(guid));

	DBG(4,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	ret=sql_query(db,query,_WHERE("plVaultQueryPlayerList"));

	if(ret!=SQL_OK) {
		//if(mysql_errno(db->conn)==1146 || mysql_errno(db->conn)==1054) {
			//ret=plVaultInitializeAuthDB(db,1); //vault initialitzation task TODO
		//}
		if(ret>=0) { //try again
			ret=sql_query(db,query,_WHERE("plVaultQueryPlayerList try2"));
		} else {
			ret=-1;
		}
	}

	DBG(4,"dmalloc_verify()\n");
	dmalloc_verify(NULL);


	if(ret==SQL_OK) {
		result=mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("plVaultQueryPlayerList inner"));
			ret=-1;
		} else {
			ret=mysql_num_rows(result);

			DBG(4,"dmalloc_verify()\n");
			dmalloc_verify(NULL);

			//work with the results
			if(ret>=0) {
	DBG(4,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

				*data=(Byte *)malloc(sizeof(Byte) * ((200*ret)+50));
				if(*data==NULL) ret=-1;
	DBG(4,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

				if(ret>=0) {
					buf=*data;
					*(U16 *)(buf+off)=ret; //number of players
					off+=2;
					for(i=0; i<ret; i++) {
						row=mysql_fetch_row(result);
						*(U32 *)(buf+off)=atoi(row[0]); //KI
						off+=4;
						off+=encode_urustring(buf+off,(Byte *)row[1],strlen(row[1]),0); //avie
						*(Byte *)(buf+off)=atoi(row[2]); //flags
						off++;
					}
					*size=off;
				}
			}
			mysql_free_result(result);
		}
	} else { ret=-1; }

	DBG(4,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	sql_end(db);
	return ret;
}

/**
  Query the database for the number of players
	returns -1 on db error
*/
int plVaultGetNumberOfPlayers(Byte * guid,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[1024];
	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return -1; }

	sprintf(query,"select count(v.idx)\
 from %s v where v.uid='%s';",vault_table,create_str_guid(guid));

	ret=sql_query(db,query,_WHERE("plVaultGetNumberOfPlayers"));

	if(ret==SQL_OK) {
		result=mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("plVaultGetNumberOfPlayers inner"));
			ret=-1;
		} else {

	//DBG(4,"dmalloc_verify()\n");
	//dmalloc_verify(NULL);

			//ret=mysql_num_rows(result);
			row=mysql_fetch_row(result);
			DBG(3,"res: %i\n",atoi(row[0]));
			ret=atoi(row[0]);
			mysql_free_result(result);
		}
	} else {
		ret=-1;
	}

	sql_end(db);
	return ret;
}

//!check that the player owns that avatar
int plVaultCheckKi(Byte * guid,U32 ki,Byte * avatar_name,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[1024];
	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return -1; }

	sprintf(query,"Select v.avie from %s v\
 where v.uid = '%s' and v.idx = '%i';",vault_table,create_str_guid(guid),ki);

	ret=sql_query(db,query,_WHERE("plVaultCheckKi"));

	if(ret==SQL_OK) {
		result=mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("plVaultCheckKi inner"));
			ret=-1;
		} else {
			ret=mysql_num_rows(result);
			if(ret>0) {
				row=mysql_fetch_row(result);
				strcpy((char *)avatar_name,row[0]);
			}
			mysql_free_result(result);
		}
	} else {
		ret=-1;
	}

	sql_end(db);
	return ret;
}

/*------------------------------
  Low Level - Node related code
------------------------------*/
/**
  Creates a new node, and returns the id
	returns -1 on db error
	returns 0 if node already exists
	returns node_id if node has been succesfully created
	(Important, non-masked fields will be ignored, and default values will be set
	instead).
*/
int plVaultCreateNode(t_vault_node * node,st_sql * db) {
	char * query=NULL;
	char * en_query=NULL;
	char aux[300];

	int ret; //for store result codes
	int query_size; //query size

	DBG(5,"plVaultCreateNode -->>\n");

	//allocate memory usage
	query_size=(node->data_size*2)+(node->data2_size*2)+1000;
	DBG(6,"query_size:%i\n",query_size);

	DBG(7,"I bet that is the malloc()\n");
	DBG(9,"%i\n",(sizeof(char) * query_size) +4000);
	query=(char *)malloc((sizeof(char) * (query_size +4000)));
	if(query==NULL) { return -1; }
	DBG(7,"Nope malloc worked fine\n");

	en_query=(char *)malloc(sizeof(char) * (query_size));
	if(en_query==NULL) { free((void *)query); return -1; }
	DBG(8,"I'm here at line-->>\n");

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		free((void *)query);
		free((void *)en_query);
		return -1;
	}
	DBG(8,"step 2\n");

	//time hack
	//if(node->timestamp==0) {
		node->timestamp=(U32)time(NULL);
	//}
	//if(node->timestamp2==0) {
		node->timestamp2=node->timestamp;
	//}
	//if(node->timestamp3==0) {
		node->timestamp3=node->timestamp;
	//}
	node->microseconds=0;
	node->microseconds2=node->microseconds;
	node->microseconds3=node->microseconds;
	//end time hack

	/***** Begin query construction, part 1 *****/
	sprintf(query,"Insert Into %s (type",vault_table);

	if(node->permissions!=0) { // || (node->unkB & MPerms)) {
		strcat(query,",permissions");
	}
	if(node->owner!=0) { // || (node->unkB & MOwner)) {
		strcat(query,",owner");
	}
	if(node->unk1!=0) { // || (node->unkB & MUnk1)) {
		strcat(query,",unk1");
	}
	if(node->timestamp!=0) { // || (node->unkB & MStamp1)) {
		strcat(query,",timestamp");
	}
	if(node->microseconds!=0) { // || (node->unkB & MStamp1)) {
		strcat(query,",microseconds");
	}
	if(node->id1!=0) { // || (node->unkB & MId1)) {
		strcat(query,",id1");
	}
	if(node->timestamp2!=0) { // || (node->unkB & MStamp2)) {
		strcat(query,",timestamp2");
	}
	if(node->microseconds2!=0) { // || (node->unkB & MStamp2)) {
		strcat(query,",microseconds2");
	}
	if(node->timestamp3!=0) { // || (node->unkB & MStamp3)) {
		strcat(query,",timestamp3");
	}
	if(node->microseconds3!=0) { // || (node->unkB & MStamp3)) {
		strcat(query,",microseconds3");
	}
	if(strlen((const char *)node->age_name)!=0) { // || (node->unkB & MAgeName)) {
		strcat(query,",age_name");
	}
	if(*(U32 *)node->age_guid!=0 || *(U32 *)(node->age_guid+4)!=0 || (node->unkB & MHexGuid)) {
		strcat(query,",age_guid");
	}
	if(node->torans!=0) { // || (node->unkB & MTorans)) {
		strcat(query,",torans");
	}
	if(node->distance!=0) { // || (node->unkB & MDistance)) {
		strcat(query,",distance");
	}
	if(node->elevation!=0) { // || (node->unkB & MElevation)) {
		strcat(query,",elevation");
	}
	if(node->unk5!=0) { // || (node->unkB & MUnk5)) {
		strcat(query,",unk5");
	}
	if(node->id2!=0) { // || (node->unkB & MId2)) {
		strcat(query,",id2");
	}
	if(node->unk7!=0) { // || (node->unkB & MUnk7)) {
		strcat(query,",unk7");
	}
	if(node->unk8!=0) { // || (node->unkB & MUnk8)) {
		strcat(query,",unk8");
	}
	if(node->unk9!=0) { // || (node->unkB & MUnk9)) {
		strcat(query,",unk9");
	}
	if(strlen((const char *)node->entry_name)!=0) { // || (node->unkB & MEntryName)) {
		strcat(query,",entry_name");
	}
	if(strlen((const char *)node->sub_entry_name)!=0) { // || (node->unkB & MSubEntry)) {
		strcat(query,",sub_entry_name");
	}
	if(strlen((const char *)node->owner_name)!=0) { // || (node->unkB & MOwnerName)) {
		strcat(query,",owner_name");
	}
	if(strlen((const char *)node->guid)!=0) { // || (node->unkB & MGuid)) {
		strcat(query,",guid");
	}
	if(strlen((const char *)node->str1)!=0) { // || (node->unkB & MStr1)) {
		strcat(query,",str1");
	}
	if(strlen((const char *)node->str2)!=0) { // || (node->unkB & MStr2)) {
		strcat(query,",str2");
	}
	if(strlen((const char *)node->avie)!=0) { // || (node->unkB & MAvie)) {
		strcat(query,",avie");
	}
	if(strlen((const char *)node->uid)!=0) { // || (node->unkB & MUid)) {
		strcat(query,",uid");
	}
	if(strlen((const char *)node->entry_value)!=0) { // || (node->unkB & MEValue)) {
		strcat(query,",entry_value");
	}
	if(strlen((const char *)node->entry2)!=0) { // || (node->unkB & MEntry2)) {
		strcat(query,",entry2");
	}
#if 0
	if(node->unk13!=0 || (node->unkB & MBlob1)) {}
	if(node->unk14!=0 || (node->unkB & MBlob1)) {}
	//unk15 & unk16 has been removed
#endif
	if(node->data_size!=0) { // || (node->unkB & MData1)) {
		strcat(query,",data");
	}
	if(node->data2_size!=0) { // || (node->unkB & MData2)) {
		strcat(query,",data2");
	}
	/***** end part1, begin part 2 *****/
	sprintf(aux,") values('%i'",node->type);
	strcat(query,aux);

	if(node->permissions!=0) { // || (node->unkB & MPerms)) {
		sprintf(aux,",'%i'",node->permissions);
		strcat(query,aux);
	}
	if(node->owner!=0) { // || (node->unkB & MOwner)) {
		sprintf(aux,",'%i'",node->owner);
		strcat(query,aux);
	}
	if(node->unk1!=0) { // || (node->unkB & MUnk1)) {
		sprintf(aux,",'%i'",node->unk1);
		strcat(query,aux);
	}
	if(node->timestamp!=0) { // || (node->unkB & MStamp1)) {
		sprintf(aux,",'%i'",node->timestamp);
		strcat(query,aux);
	}
	if(node->microseconds!=0) { // || (node->unkB & MStamp1)) {
		sprintf(aux,",'%i'",node->microseconds);
		strcat(query,aux);
	}
	if(node->id1!=0) { // || (node->unkB & MId1)) {
		sprintf(aux,",'%i'",node->id1);
		strcat(query,aux);
	}
	if(node->timestamp2!=0) { // || (node->unkB & MStamp2)) {
		sprintf(aux,",'%i'",node->timestamp2);
		strcat(query,aux);
	}
	if(node->microseconds2!=0) { // || (node->unkB & MStamp2)) {
		sprintf(aux,",'%i'",node->microseconds2);
		strcat(query,aux);
	}
	if(node->timestamp3!=0) { // || (node->unkB & MStamp3)) {
		sprintf(aux,",'%i'",node->timestamp3);
		strcat(query,aux);
	}
	if(node->microseconds3!=0) { // || (node->unkB & MStamp3)) {
		sprintf(aux,",'%i'",node->microseconds3);
		strcat(query,aux);
	}
	if(strlen((const char *)node->age_name)!=0) { // || (node->unkB & MAgeName)) {
		mysql_escape_string(en_query,(const char *)node->age_name,\
		strlen((const char *)node->age_name));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(*(U32 *)node->age_guid!=0 || *(U32 *)(node->age_guid+4)!=0 || (node->unkB & MHexGuid)) {
		hex2ascii2((Byte *)aux,node->age_guid,8);
		mysql_escape_string(en_query,aux,strlen(aux));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(node->torans!=0) { // || (node->unkB & MTorans)) {
		sprintf(aux,",'%i'",node->torans);
		strcat(query,aux);
	}
	if(node->distance!=0) { // || (node->unkB & MDistance)) {
		sprintf(aux,",'%i'",node->distance);
		strcat(query,aux);
	}
	if(node->elevation!=0) { // || (node->unkB & MElevation)) {
		sprintf(aux,",'%i'",node->elevation);
		strcat(query,aux);
	}
	if(node->unk5!=0) { // || (node->unkB & MUnk5)) {
		sprintf(aux,",'%i'",node->unk5);
		strcat(query,aux);
	}
	if(node->id2!=0) { // || (node->unkB & MId2)) {
		sprintf(aux,",'%i'",node->id2);
		strcat(query,aux);
	}
	if(node->unk7!=0) { // || (node->unkB & MUnk7)) {
		sprintf(aux,",'%i'",node->unk7);
		strcat(query,aux);
	}
	if(node->unk8!=0) { // || (node->unkB & MUnk8)) {
		sprintf(aux,",'%i'",node->unk8);
		strcat(query,aux);
	}
	if(node->unk9!=0) { // || (node->unkB & MUnk9)) {
		sprintf(aux,",'%i'",node->unk9);
		strcat(query,aux);
	}
	if(strlen((const char *)node->entry_name)!=0) { // || (node->unkB & MEntryName)) {
		mysql_escape_string(en_query,(const char *)node->entry_name,\
		strlen((const char *)node->entry_name));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->sub_entry_name)!=0) { // || (node->unkB & MSubEntry)) {
		mysql_escape_string(en_query,(const char *)node->sub_entry_name,\
		strlen((const char *)node->sub_entry_name));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->owner_name)!=0) { // || (node->unkB & MOwnerName)) {
		mysql_escape_string(en_query,(const char *)node->owner_name,\
		strlen((const char *)node->owner_name));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->guid)!=0) { // || (node->unkB & MGuid)) {
		mysql_escape_string(en_query,(const char *)node->guid,\
		strlen((const char *)node->guid));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->str1)!=0) { // || (node->unkB & MStr1)) {
		mysql_escape_string(en_query,(const char *)node->str1,\
		strlen((const char *)node->str1));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->str2)!=0) { // || (node->unkB & MStr2)) {
		mysql_escape_string(en_query,(const char *)node->str2,\
		strlen((const char *)node->str2));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->avie)!=0) { // || (node->unkB & MAvie)) {
		mysql_escape_string(en_query,(const char *)node->avie,\
		strlen((const char *)node->avie));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->uid)!=0) { // || (node->unkB & MUid)) {
		mysql_escape_string(en_query,(const char *)node->uid,\
		strlen((const char *)node->uid));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->entry_value)!=0) { // || (node->unkB & MEValue)) {
		mysql_escape_string(en_query,(const char *)node->entry_value,\
		strlen((const char *)node->entry_value));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(strlen((const char *)node->entry2)!=0) { // || (node->unkB & MEntry2)) {
		mysql_escape_string(en_query,(const char *)node->entry2,\
		strlen((const char *)node->entry2));
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
#if 0
	if(node->unk13!=0 || (node->unkB & MBlob1)) {}
	if(node->unk14!=0 || (node->unkB & MBlob1)) {}
	//unk15 & unk16 has been removed
#endif
	if(node->data_size!=0) {// || (node->unkB & MData1)) {
		mysql_escape_string(en_query,(const char *)node->data,node->data_size);
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	if(node->data2_size!=0) {// || (node->unkB & MData2)) {
		mysql_escape_string(en_query,(const char *)node->data2,node->data2_size);
		strcat(query,",'");
		strcat(query,en_query);
		strcat(query,"'");
	}
	strcat(query,");");
	//end query construction

	ret=sql_query(db,query,_WHERE("plVaultCreateNode"));

	if(ret==SQL_OK) {
		ret=mysql_insert_id(db->conn);
	} else if(mysql_errno(db->conn)==1062) {
		ret=0;
	} else {
		ret=-1;
	}

	free((void *)query);
	free((void *)en_query);

	sql_end(db);
	return ret;
}

/**
  Updates a node, and returns the id
	returns -1 on db error
	returns node_id if node has been succesfully created
	(important mask must be set!, only masked items will be updated!)
*/
int plVaultUpdateNode(t_vault_node * node,st_sql * db) {
	char * query=NULL;
	char * en_query=NULL;
	//char aux[300];

	int ret; //for store result codes
	int query_size; //query size

	//allocate memory usage
	query_size=(node->data_size*2)+(node->data2_size*2)+1000;
	query=(char *)malloc((sizeof(char) * (query_size +4000)));
	if(query==NULL) { return -1; }
	en_query=(char *)malloc(sizeof(char) * (query_size));
	if(en_query==NULL) { free((void *)query); return -1; }

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		free((void *)query);
		free((void *)en_query);
		return -1;
	}

	//time hack
	if(node->timestamp==0) {
		node->timestamp=(U32)time(NULL);
		node->microseconds=0; //node->microseconds;
	}
	//end time hack

	int coma=0;

	/***** Begin query construction *****/
	sprintf(query,"Update %s n SET ",vault_table);
	if(node->type!=0) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"type='%i'",node->type);
		strcat(query,en_query);
		coma=1;
	}
	if(node->permissions!=0 || (node->unkB & MPerms)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"permissions='%i'",node->permissions);
		strcat(query,en_query);
		coma=1;
	}
	if(node->owner!=0 || (node->unkB & MOwner)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"owner='%i'",node->owner);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk1!=0 || (node->unkB & MUnk1)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk1='%i'",node->unk1);
		strcat(query,en_query);
		coma=1;
	}
	if(node->timestamp!=0 || (node->unkB & MStamp1)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"timestamp='%i'",node->timestamp);
		strcat(query,en_query);
		coma=1;
	}
	if(node->microseconds!=0 || (node->unkB & MStamp1)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"microseconds='%i'",node->microseconds);
		strcat(query,en_query);
		coma=1;
	}
	if(node->id1!=0 || (node->unkB & MId1)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"id1='%i'",node->id1);
		strcat(query,en_query);
		coma=1;
	}
#if 0
	if(node->timestamp2!=0 || (node->unkB & MStamp2)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"timestamp2='%i'",node->timestamp2);
		strcat(query,en_query);
		coma=1;
	}
	if(node->microseconds2!=0 || (node->unkB & MStamp2)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"microseconds2='%i'",node->microseconds2);
		strcat(query,en_query);
		coma=1;
	}
	if(node->timestamp3!=0 || (node->unkB & MStamp3)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"timestamp3='%i'",node->timestamp3);
		strcat(query,en_query);
		coma=1;
	}
	if(node->microseconds3!=0 || (node->unkB & MStamp3)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"microseconds3='%i'",node->microseconds3);
		strcat(query,en_query);
		coma=1;
	}
#endif
	if(strlen((const char *)node->age_name)!=0 || (node->unkB & MAgeName)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"age_name='");
		mysql_escape_string(en_query,(const char *)node->age_name,\
strlen((const char *)node->age_name));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(*(U32 *)node->age_guid!=0 || *(U32 *)(node->age_guid+4)!=0 || (node->unkB & MHexGuid)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"age_guid='");
		char aux_guid[30];
		hex2ascii2((Byte *)aux_guid,node->age_guid,8);
		mysql_escape_string(en_query,aux_guid,strlen(aux_guid));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(node->torans!=0 || (node->unkB & MTorans)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"torans='%i'",node->torans);
		strcat(query,en_query);
		coma=1;
	}
	if(node->distance!=0 || (node->unkB & MDistance)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"distance='%i'",node->distance);
		strcat(query,en_query);
		coma=1;
	}
	if(node->elevation!=0 || (node->unkB & MElevation)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"elevation='%i'",node->elevation);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk5!=0 || (node->unkB & MUnk5)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk5='%i'",node->unk5);
		strcat(query,en_query);
		coma=1;
	}
	if(node->id2!=0 || (node->unkB & MId2)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"id2='%i'",node->id2);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk7!=0 || (node->unkB & MUnk7)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk7='%i'",node->unk7);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk8!=0 || (node->unkB & MUnk8)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk8='%i'",node->unk8);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk9!=0 || (node->unkB & MUnk9)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk9='%i'",node->unk9);
		strcat(query,en_query);
		coma=1;
	}
	if(strlen((const char *)node->entry_name)!=0 || (node->unkB & MEntryName)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"entry_name='");
		mysql_escape_string(en_query,(const char *)node->entry_name,\
strlen((const char *)node->entry_name));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->sub_entry_name)!=0 || (node->unkB & MSubEntry)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"sub_entry_name='");
		mysql_escape_string(en_query,(const char *)node->sub_entry_name,\
strlen((const char *)node->sub_entry_name));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->owner_name)!=0 || (node->unkB & MOwnerName)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"owner_name='");
		mysql_escape_string(en_query,(const char *)node->owner_name,\
strlen((const char *)node->owner_name));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->guid)!=0 || (node->unkB & MGuid)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"guid='");
		mysql_escape_string(en_query,(const char *)node->guid,\
strlen((const char *)node->guid));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->str1)!=0 || (node->unkB & MStr1)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"str1='");
		mysql_escape_string(en_query,(const char *)node->str1,\
strlen((const char *)node->str1));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->str2)!=0 || (node->unkB & MStr2)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"str2='");
		mysql_escape_string(en_query,(const char *)node->str2,\
strlen((const char *)node->str2));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->avie)!=0 || (node->unkB & MAvie)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"avie='");
		mysql_escape_string(en_query,(const char *)node->avie,\
strlen((const char *)node->avie));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->uid)!=0 || (node->unkB & MUid)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"uid='");
		mysql_escape_string(en_query,(const char *)node->uid,\
strlen((const char *)node->uid));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->entry_value)!=0 || (node->unkB & MEValue)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"entry_value='");
		mysql_escape_string(en_query,(const char *)node->entry_value,\
strlen((const char *)node->entry_value));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->entry2)!=0 || (node->unkB & MEntry2)) {
		if(coma==1) { strcat(query,","); }
		strcat(query,"entry2='");
		mysql_escape_string(en_query,(const char *)node->entry2,\
strlen((const char *)node->entry2));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
#if 0
	if(node->unk13!=0 || (node->unkB & MBlob1)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk13='%i'",node->unk13);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk14!=0 || (node->unkB & MBlob1)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk14='%i'",node->unk14);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk15!=0 || (node->unkB & MBlob2)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk15='%i'",node->unk15);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk16!=0 || (node->unkB & MBlob2)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"unk16='%i'",node->unk16);
		strcat(query,en_query);
		coma=1;
	}
#endif
	if(node->data_size!=0 || (node->unkB & MData1)) {
		if(coma==1) { strcat(query,","); }
		//sprintf(en_query,"data_size='%i'",node->data_size);
		//strcat(query,en_query); //","
		strcat(query,"data='");
		//binary data here
		if(node->data_size!=0) {
			mysql_escape_string(en_query,(const char *)node->data,node->data_size);
			strcat(query,en_query);
		}
		strcat(query,"'");
		coma=1;
	}
	if(node->data2_size!=0 || (node->unkB & MData2)) {
		if(coma==1) { strcat(query,","); }
		//sprintf(en_query,"data2_size='%i'",node->data2_size);
		//strcat(query,en_query); ///","
		strcat(query,"data2='");
		//binary data here
		if(node->data2_size!=0) {
			mysql_escape_string(en_query,(const char *)node->data2,node->data2_size);
			strcat(query,en_query);
		}
		strcat(query,"'");
		coma=1;
	}

	sprintf(en_query," WHERE n.idx='%i';",node->index);
	strcat(query,en_query);
	//end query creation

	ret=sql_query(db,query,_WHERE("plVaultUpdateNode"));

	if(ret==SQL_OK) {
		ret=node->index;
	} else {
		ret=-1;
	}

	free((void *)query);
	free((void *)en_query);

	sql_end(db);
	return ret;
}

/** search for a node,
   returns the id of the node with the last modify timestamp
   if it doesn't exist
	 flag=0 -> returns '0' does not exist
	 flag=1 -> creates the node, and returns the id of that node
	 -1 on db error
*/
int plVaultFindNode(t_vault_node * node, t_vault_manifest * mfs,Byte flag,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[1024];
	char en_query[1024];
	int ret; //for store result codes
	int coma=0;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	/***** Begin query construction *****/
	sprintf(query,"select n.idx,timestamp,microseconds from %s n where ",vault_table);

	if(node->index!=0) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"n.idx='%i'",node->index);
		strcat(query,en_query);
		coma=1;
		flag=0; //Cannot create a node that already exists!
	}
	if(node->type!=0) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"type='%i'",node->type);
		strcat(query,en_query);
		coma=1;
	}
	if(node->permissions!=0 || (node->unkB & MPerms)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"permissions='%i'",node->permissions);
		strcat(query,en_query);
		coma=1;
	}
	if(node->owner!=0 || (node->unkB & MOwner)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"owner='%i'",node->owner);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk1!=0 || (node->unkB & MUnk1)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk1='%i'",node->unk1);
		strcat(query,en_query);
		coma=1;
	}
#if 0
	if(node->timestamp!=0 || (node->unkB & MStamp1)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"timestamp='%i'",node->timestamp);
		strcat(query,en_query);
		coma=1;
	}
	if(node->microseconds!=0 || (node->unkB & MStamp1)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"microseconds='%i'",node->microseconds);
		strcat(query,en_query);
		coma=1;
	}
#endif
	if(node->id1!=0 || (node->unkB & MId1)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"id1='%i'",node->id1);
		strcat(query,en_query);
		coma=1;
	}
#if 0
	if(node->timestamp2!=0 || (node->unkB & MStamp2)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"timestamp2='%i'",node->timestamp2);
		strcat(query,en_query);
		coma=1;
	}
	if(node->microseconds2!=0 || (node->unkB & MStamp2)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"microseconds2='%i'",node->microseconds2);
		strcat(query,en_query);
		coma=1;
	}
	if(node->timestamp3!=0 || (node->unkB & MStamp3)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"timestamp3='%i'",node->timestamp3);
		strcat(query,en_query);
		coma=1;
	}
	if(node->microseconds3!=0 || (node->unkB & MStamp3)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"microseconds3='%i'",node->microseconds3);
		strcat(query,en_query);
		coma=1;
	}
#endif
	if(strlen((const char *)node->age_name)!=0 || (node->unkB & MAgeName)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"age_name='");
		mysql_escape_string(en_query,(const char *)node->age_name,\
strlen((const char *)node->age_name));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(*(U32 *)node->age_guid!=0 || *(U32 *)(node->age_guid+4)!=0 || (node->unkB & MHexGuid)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"age_guid='");
		char aux_guid[20];
		hex2ascii2((Byte *)aux_guid,node->age_guid,8);
		//mysql_escape_string(en_query,(const char *)node->age_guid,strlen(aux_guid));
		mysql_escape_string(en_query,aux_guid,strlen(aux_guid));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(node->torans!=0 || (node->unkB & MTorans)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"torans='%i'",node->torans);
		strcat(query,en_query);
		coma=1;
	}
	if(node->distance!=0 || (node->unkB & MDistance)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"distance='%i'",node->distance);
		strcat(query,en_query);
		coma=1;
	}
	if(node->elevation!=0 || (node->unkB & MElevation)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"elevation='%i'",node->elevation);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk5!=0 || (node->unkB & MUnk5)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk5='%i'",node->unk5);
		strcat(query,en_query);
		coma=1;
	}
	if(node->id2!=0 || (node->unkB & MId2)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"id2='%i'",node->id2);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk7!=0 || (node->unkB & MUnk7)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk7='%i'",node->unk7);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk8!=0 || (node->unkB & MUnk8)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk8='%i'",node->unk8);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk9!=0 || (node->unkB & MUnk9)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk9='%i'",node->unk9);
		strcat(query,en_query);
		coma=1;
	}
	if(strlen((const char *)node->entry_name)!=0 || (node->unkB & MEntryName)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"entry_name='");
		mysql_escape_string(en_query,(const char *)node->entry_name,\
strlen((const char *)node->entry_name));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->sub_entry_name)!=0 || (node->unkB & MSubEntry)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"sub_entry_name='");
		mysql_escape_string(en_query,(const char *)node->sub_entry_name,\
strlen((const char *)node->sub_entry_name));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->owner_name)!=0 || (node->unkB & MOwnerName)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"owner_name='");
		mysql_escape_string(en_query,(const char *)node->owner_name,\
strlen((const char *)node->owner_name));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->guid)!=0 || (node->unkB & MGuid)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"guid='");
		mysql_escape_string(en_query,(const char *)node->guid,\
strlen((const char *)node->guid));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->str1)!=0 || (node->unkB & MStr1)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"str1='");
		mysql_escape_string(en_query,(const char *)node->str1,\
strlen((const char *)node->str1));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->str2)!=0 || (node->unkB & MStr2)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"str2='");
		mysql_escape_string(en_query,(const char *)node->str2,\
strlen((const char *)node->str2));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->avie)!=0 || (node->unkB & MAvie)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"avie='");
		mysql_escape_string(en_query,(const char *)node->avie,\
strlen((const char *)node->avie));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->uid)!=0 || (node->unkB & MUid)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"uid='");
		mysql_escape_string(en_query,(const char *)node->uid,\
strlen((const char *)node->uid));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->entry_value)!=0 || (node->unkB & MEValue)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"entry_value='");
		mysql_escape_string(en_query,(const char *)node->entry_value,\
strlen((const char *)node->entry_value));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(strlen((const char *)node->entry2)!=0 || (node->unkB & MEntry2)) {
		if(coma==1) { strcat(query,"and "); }
		strcat(query,"entry2='");
		mysql_escape_string(en_query,(const char *)node->entry2,\
strlen((const char *)node->entry2));
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
#if 0
	if(node->unk13!=0) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk13='%i'",node->unk13);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk14!=0) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk14='%i'",node->unk14);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk15!=0) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk15='%i'",node->unk15);
		strcat(query,en_query);
		coma=1;
	}
	if(node->unk16!=0) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"unk16='%i'",node->unk16);
		strcat(query,en_query);
		coma=1;
	}
#endif
	strcat(query,";");
	//end query creation

	ret=sql_query(db,query,_WHERE("plVaultFindNode"));

	if(ret==SQL_OK) {
		result = mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("plVaultFindNode inner"));
			ret=-1;
		} else {
			ret=mysql_num_rows(result);
			print2log(db->sql,"num_rows %i\n",ret);

			if(ret>0 && (row=mysql_fetch_row(result))!=NULL) {
				//found the node
				mfs->index=atoi(row[0]);
				mfs->timestamp=atof(row[1]);// + ((atof(row[2]))/1000000); <---
				ret=mfs->index;
			} else if(flag==1) {
				//create the requested node
				node->timestamp=0;
				node->microseconds=0;
				ret=plVaultCreateNode(node,db);
				mfs->index=ret;
				mfs->timestamp=(double)node->timestamp;
			} else {
				mfs->index=0;
				mfs->timestamp=0;
				ret=0;
			}
		}
		mysql_free_result(result);
	} else {
		ret=-1;
	}

	sql_end(db);
	return ret;
}

/**
	Creates a vault reference
	returns -1 on db error
*/
int plVaultCreateRef(U32 id1,U32 id2,U32 id3,U32 tstamp,U32 micros,Byte flag,st_sql * db) {
	char query[1024];
	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	//time hack
	if(tstamp==0) {
		tstamp=(U32)time(NULL);
		micros=0; //get_microseconds();;
	}
	//end time hack

	//query here the database
	sprintf(query,"Insert Into %s (id1,id2,id3,timestamp,microseconds,flag)\
values('%i','%i','%i','%i','%i','%i');",ref_vault_table,id1,id2,id3,tstamp,micros,flag);
	//end query creation

	ret=sql_query(db,query,_WHERE("plVaultCreateRef"));

	if(ret==SQL_OK) {
		ret=0;
	} else {
		ret=-1;
	}

	sql_end(db);
	return ret;
}

/**
  Removes a node (put father=0, son=id to remove a root node)
	with flag=1 to force deletion!
	with flag=2 search for the info nodes and delete them (only if they are at level 1)
	if flag=1 and the node is a MGR it will be deleted!
	\return -1 on db error, 0 if node doens't exist and >0 if success
	(if tree_force==1, deletes the tree, if not, it is not deleted)
*/
int plVaultRemoveNodeRef2(U32 father, U32 son, Byte flag,Byte tree_force,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[1024];
	int ret; //for store result codes

	int count; //number of nodes
	int type; //the type of the node
	U32 * mfs; //manifest
	int i;
	int pending;
	int level; //the profundity

	int info_nodes=0;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

#if 0
	if(flag==2) {
		info_nodes=1;
		flag=1;
	}
#else
	if(flag==2) {
		flag=1;
	}
#endif

	//set initial values
	mfs=(U32 *)malloc(sizeof(U32) * (1) *3);
	i=0;
	pending=1;
	level=0;
	mfs[i]=son; //son
	mfs[i+1]=father; //father
	mfs[i+2]=level; //level

	//loop
	while(i<pending) {

		son=mfs[i];
		father=mfs[i+1];
		level=mfs[i+2];

		//1o contar el número de referencias que apuntan a son y obtener el tipo de nodo
		sprintf(query,"Select count(*),v.type from %s r,%s v\
	where r.id3=%i and v.idx=%i\
	group by v.type;",ref_vault_table,vault_table,son,son);

		ret=sql_query(db,query,_WHERE("plVaultRemoveNodeRef search"));

		if(ret==SQL_OK) {
			result = mysql_store_result(db->conn); //store the query results
			if(result==NULL) {
				sql_error(db,_WHERE("plVaultRemoveNodeRef search inner"));
				ret=-1;
			} else {
				//work with the results
				if((row=mysql_fetch_row(result))!=NULL) {
					count=atoi(row[0]); //now we have the number of parents
					type=atoi(row[1]); //now we have the type of the node
					ret=1;
				} else {
					count=0;
					type=0;
					ret=0;
					print2log(db->sql,"The selected node %i doesn't exist!\n",son);
				}
				mysql_free_result(result);
			}
		} else {
			ret=-1;
		}

		if(ret<0) {
			//something went bad, very bad
		} else {
			int what=0;
			//2o si es un MGR y flag es 0 no propagar borrado a los hijos
			// cualquier otro cas borrar siempre que sea huérfano.
			DBG(5,"type:%i,flag:%i,count:%i,info_nodes:%i,father:%i,son:%i,level:%i\n",type,flag,count,info_nodes,father,son,level);
			if((type<=7 && flag==0) || (count>1 && flag==0)) { // && (type!=23 || info_nodes==0))) { //el nodo no es huerfano count>1
				//only delete the reference and end
				sprintf(query,"Delete from %s where id2=%i and id3=%i;",ref_vault_table,father,son);
				what=1;
			} else { //el nodo es huerfano, or we want to delete it by force brute
				sprintf(query,"Delete from %s where id3=%i;",ref_vault_table,son);
				what=0;
			}
			flag=0;
			if(level>1) {
				info_nodes=0;
			}

			ret=sql_query(db,query,_WHERE("lVaultRemoveNodeRef delete ref(s)"));

			if(ret==SQL_OK) {
				ret=1;
			} else {
				ret=-1;
			}
			//Due to how bad was programed the URU client, we can't clean the tree :(
			//what=1; //disable the cleanning, let's the vault to grow and grow with more garbage!

			//fix
			if(tree_force!=1) {
				what=1;
			} //if tree_force flag is set, allow tree deletion

			if(what==1 || ret<0) {
				//go to the next one
			} else {
				ret=-1;

				//now delete the node
				sprintf(query,"Delete from %s where `idx`=%i;",vault_table,son);

				ret=sql_query(db,query,_WHERE("plVaultRemoveNodeRef delete node"));

				if(ret==SQL_OK) {
					ret=1;
				} else {
					ret=-1;
				}

				if(ret<0) {
					//go to the next one
				} else {
					ret=-1;
					//get the manifest
					sprintf(query,"Select r.id3 from %s r where\
 r.id2=%i;",ref_vault_table,son);
					ret=sql_query(db,query,_WHERE("plVaultRemoveNodeRef, Query manifest"));

					if(ret!=SQL_OK) {
						ret=-1;
					} else {
						result = mysql_store_result(db->conn); //store the query results
						if(result==NULL) {
							sql_error(db,_WHERE("plVaultRemoveNodeRef, Query manifest inner"));
							ret=-1;
						} else {
							//get the number of rows
							ret=mysql_num_rows(result);

							if(ret>0) {
								//allocate buffer
								mfs=(U32 *)realloc(mfs,sizeof(U32)*(ret+pending)*3);

								int e;
								//work with the results
								for(e=0; e<ret; e++) {
									row=mysql_fetch_row(result);
									DBG(3,"%i\n",atoi(row[0]));
									mfs[e+pending]=atoi(row[0]);
									mfs[e+pending+1]=son;
									mfs[e+pending+2]=level+1;
								}
								pending+=ret;
							}
							mysql_free_result(result);
						}
					} //end get manifest
				} //end delete node
			} //end delete ref
		} //end main search
		//father=son;
		i++;
	}
	if(ret<0) {
		ret=0;
	}
	//all done
	free((void *)mfs);

	sql_end(db);
	return ret;
}

int plVaultRemoveNodeRef(U32 father, U32 son, Byte flag,st_sql * db) {
	return(plVaultRemoveNodeRef2(father,son,flag,0,db));
}

/** Set the reference as a seen reference
*/
int plVaultSeetRefSeen(U32 father, U32 son,Byte flag,st_sql * db) {
	char query[1024];
	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	//query here the database
	sprintf(query,"Update %s r SET flag='%i'\
 WHERE r.id2='%i' and r.id3='%i';",ref_vault_table,flag,father,son);

	ret=sql_query(db,query,_WHERE("plVaultSetRefSeen"));

	if(ret==SQL_OK) {
		ret=0;
	} else {
		if(mysql_errno(db->conn)==1062) ret=0;
		else ret=-1;
	}

	sql_end(db);
	return ret;
}

/** Query the database for nodes
   (n_table is the number of nodes in table, table is a list of nodes)
	 remember to destroy the node buffer!
	 (it tryes to return an average size of the node stream data)
	 ret <0 is an error and should be checked!
*/
int plVaultFetchNodes(t_vault_node ** node,int * num,U32 * table,int n_table,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	unsigned long *lengths; //the len of each column
	char query[30*1024];
	char aux[300];
	int ret; //for store result codes
	int i;
	int coma=0;

	*node=0;
	*num=0;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	/*** 1st Create the query *****/
	sprintf(query,"Select v.idx,v.type,permissions,owner,unk1,timestamp,id1,\
timestamp2,timestamp3,age_name,age_guid,torans,distance,\
elevation,unk5,id2,unk7,unk8,unk9,entry_name,sub_entry_name,owner_name,guid,\
str1,str2,avie,uid,entry_value,entry2,data,data2\
 from %s v where v.idx IN(",vault_table);

	for(i=0; i<n_table; i++) {
		if(coma==1) { strcat(query,","); }
		sprintf(aux,"%i",table[i]);
		strcat(query,aux);
		coma=1;
	}
	strcat(query,");");

	ret=sql_query(db,query,_WHERE("plVaultFetchNodes"));

	if(ret!=SQL_OK) {
		ret=-1;
	} else {
		result = mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("plVaultFetchNodes inner"));
			ret=-1;
		} else {
			//work with the results
			//get the number of rows
			*num=mysql_num_rows(result);
			ret=0;

			*node=(t_vault_node *)malloc(sizeof(t_vault_node) * *num);
			memset((void *)*node,0,sizeof(t_vault_node) * *num);

			for(i=0; i<*num; i++) {
				row=mysql_fetch_row(result);
				//set base vals
				(*node)[i].unkA=0x00000002;
				(*node)[i].unkB=0xFFFFFFFF;
				(*node)[i].unkC=0x00000007;
				(*node)[i].data=NULL;
				(*node)[i].data2=NULL;
				//end set base vals
				//get data
				lengths = mysql_fetch_lengths(result); //get the size of each column
				ret+=4+4+4; //the separators
				(*node)[i].index=(U32)atoi(row[0]);
				ret+=4;
				(*node)[i].type=(Byte)atoi(row[1]);
				ret++;
				(*node)[i].permissions=(U32)atoi(row[2]);
				ret+=4;
				(*node)[i].owner=(S32)atoi(row[3]);
				ret+=4;
				(*node)[i].unk1=(U32)atoi(row[4]);
				ret+=4;
				(*node)[i].timestamp=(U32)atoi(row[5]);
				ret+=4;
				(*node)[i].microseconds=0;
				ret+=4;
				(*node)[i].id1=(U32)atoi(row[6]);
				ret+=4;
				(*node)[i].timestamp2=(U32)atoi(row[7]);
				ret+=4;
				(*node)[i].microseconds2=0;
				ret+=4;
				(*node)[i].timestamp3=(U32)atoi(row[8]);
				ret+=4;
				(*node)[i].microseconds3=0;
				ret+=4;
				strcpy((char *)(*node)[i].age_name,row[9]);
				ret+=strlen(row[9])+2;
				char aux_guid[9];
				memset(aux_guid,0,8);
				if(strlen(row[10])==16) {
					ascii2hex2((Byte *)aux_guid,(Byte *)row[10],8);
				}
				memcpy((*node)[i].age_guid,aux_guid,8);
				ret+=8;
				(*node)[i].torans=(S32)atoi(row[11]);
				ret+=4;
				(*node)[i].distance=(S32)atoi(row[12]);
				ret+=4;
				(*node)[i].elevation=(S32)atoi(row[13]);
				ret+=4;
				(*node)[i].unk5=(U32)atoi(row[14]);
				ret+=4;
				(*node)[i].id2=(U32)atoi(row[15]);
				ret+=4;
				(*node)[i].unk7=(U32)atoi(row[16]);
				ret+=4;
				(*node)[i].unk8=(U32)atoi(row[17]);
				ret+=4;
				(*node)[i].unk9=(U32)atoi(row[18]);
				ret+=4;
				strcpy((char *)(*node)[i].entry_name,row[19]);
				ret+=strlen(row[19])+2;
				strcpy((char *)(*node)[i].sub_entry_name,row[20]);
				ret+=strlen(row[20])+2;
				strcpy((char *)(*node)[i].owner_name,row[21]);
				ret+=strlen(row[21])+2;
				strcpy((char *)(*node)[i].guid,row[22]);
				ret+=strlen(row[22])+2;
				strcpy((char *)(*node)[i].str1,row[23]);
				ret+=strlen(row[23])+2;
				strcpy((char *)(*node)[i].str2,row[24]);
				ret+=strlen(row[24])+2;
				strcpy((char *)(*node)[i].avie,row[25]);
				ret+=strlen(row[25])+2;
				strcpy((char *)(*node)[i].uid,row[26]);
				ret+=strlen(row[26])+2;
				strcpy((char *)(*node)[i].entry_value,row[27]);
				ret+=strlen(row[27])+2;
				strcpy((char *)(*node)[i].entry2,row[28]);
				ret+=strlen(row[28])+2;
				(*node)[i].data_size=lengths[29];
				ret+=4;
				ret+=lengths[29];
				if(lengths[29]>0) {
					(*node)[i].data=(Byte *)malloc(lengths[29] * sizeof(Byte));
					memcpy((*node)[i].data,row[29],lengths[29]);
				} else {
					(*node)[i].data=NULL;
				}
				(*node)[i].data2_size=lengths[30];
				ret+=4;
				ret+=lengths[30];
				if(lengths[30]>0) {
					(*node)[i].data2=(Byte *)malloc(lengths[30] * sizeof(Byte));
					memcpy((*node)[i].data2,row[30],lengths[30]);
				} else {
					(*node)[i].data2=NULL;
				}
				(*node)[i].unk13=0; //(U32)atoi(row[35]);
				ret+=4;
				(*node)[i].unk14=0; //(U32)atoi(row[36]);
				ret+=4;
				(*node)[i].unk15=0; //(U32)atoi(row[37]);
				ret+=4;
				(*node)[i].unk16=0; //(U32)atoi(row[38]);
				ret+=4;
				//end getting data
				DBG(5,"Now size is ret:%i\n",ret);
			}
		}
		mysql_free_result(result);
	}

	sql_end(db);
	return ret;
}

/** Get the cross reference from an specific node
   (it has anti-loop protection) <-not implemented
   returns the number of references if success
	 returns 0 if fails
*/
int plVaultGetCrossRef(U32 id, t_vault_cross_ref ** ref,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[30*1024];
	int ret; //for store result codes
	int i=-1;
	int e=0;
	int total=0; //total number of references

	*ref=NULL; //security

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	//query here the database

	while(i<total) {
		if(i>=0) {
			id=(*ref)[i].id3; //next one
		}

		sprintf(query,"Select r.id1,r.id2,r.id3,r.timestamp,r.microseconds,r.flag\
 from %s r where r.id2='%i';",ref_vault_table,id);
		ret=sql_query(db,query,_WHERE("plVaultGetCrossRef"));

		if(ret!=SQL_OK) {
			ret=-1;
		} else {
			result = mysql_store_result(db->conn); //store the query results
			if(result==NULL) {
				sql_error(db,_WHERE("plVaultGetCrossRef inner"));
				ret=-1;
			} else {
				//work with the results
				//get the number of rows
				ret=mysql_num_rows(result);
				if(ret>0) {
					//allocate the struct
					if(i==-1) {
						*ref=(t_vault_cross_ref *)malloc((total+ret)*sizeof(t_vault_cross_ref));
					} else {
						*ref=(t_vault_cross_ref *)realloc((void *)*ref,\
(total+ret)*sizeof(t_vault_cross_ref));
					}
					for(e=0; e<ret; e++) {
						row=mysql_fetch_row(result);
						(*ref)[e+total].id1=atoi(row[0]);
						(*ref)[e+total].id2=atoi(row[1]);
						(*ref)[e+total].id3=atoi(row[2]);
						(*ref)[e+total].timestamp=atoi(row[3]);
						(*ref)[e+total].microseconds=atoi(row[4]);
						(*ref)[e+total].flag=(Byte)atoi(row[5]);
					}
					total+=ret;
				}
			}
			mysql_free_result(result);
		}
		i++;
	}
	ret=total;

	sql_end(db);
	return ret;
}

/** query the manifest
     all immediatly child nodes (it must get all, but well.. just testing..)
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetManifest(U32 id, t_vault_manifest ** mfs,int * n_mfs,t_vault_cross_ref ** ref,int * n_ref,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[30*1024];
	char aux[300];
	int ret; //for store result codes
	int i=0;
	int e=0;
	int j=0;
	int coma=0;

	*mfs=NULL; //security
	*ref=NULL;
	*n_mfs=0;
	*n_ref=0;

	t_vault_manifest * mfs_feed=NULL;
	int n_mfs_feed=0;
	t_vault_manifest * mfs_final=NULL;
	int n_mfs_final=0;
	t_vault_manifest * mfs_aux=NULL;
	int n_mfs_aux=0;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	//1st get the first node
	sprintf(query,"Select n.idx,n.timestamp\
 from %s n where n.idx='%i'",vault_table,id);

	ret=sql_query(db,query,_WHERE("plVaultGetManifest"));

	if(ret!=SQL_OK) {
		ret=-1;
	} else {
		result = mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("plVaultGetManifest inner"));
			ret=-1;
		} else {
			//work with the results
			//get the number of rows
			ret=mysql_num_rows(result);
			DBG(5,"query done, number of rows %i\n",ret);
			if(ret>0) {
				//allocate the struct
				DBG(5,"before a malloc...\n");
				mfs_feed=(t_vault_manifest *)malloc(1*sizeof(t_vault_manifest));
				DBG(5,"after a malloc...\n");

				DBG(5,"fetch row...\n");
				row=mysql_fetch_row(result);
				mfs_feed[0].index=atoi(row[0]);
				mfs_feed[0].timestamp=(double)atof(row[1]);
				DBG(5,"end fetch...\n");
				n_mfs_feed=1;

				//mfs_final=(t_vault_manifest *)malloc(1*sizeof(t_vault_manifest));
				//mfs_final[0].index=(*aux_mfs)[0].index;
				//mfs_final[0].timestamp=(*aux_mfs)[0].timestamp;
				//n_mfs_final=1;
			}

			//ret=i;
			mysql_free_result(result);
			DBG(5,"result freed..\n");
		}
	}
	if(ret<=0) {
		plog(db->sql,"ERR: Requested Manifest for non-existent node %i\n",id);
		//abort();
		return ret;
	}

	//now get all nodes
	while(n_mfs_feed>0) {
		n_mfs_aux=n_mfs_final;
		mfs_aux=mfs_final;
		mfs_final=(t_vault_manifest *)malloc(sizeof(t_vault_manifest) * (n_mfs_final+n_mfs_feed));
		n_mfs_final=0;

		coma=0;
		sprintf(query,"select n.idx,n.timestamp,r.id1,r.id2,r.timestamp,r.flag\
		from %s n join %s r on r.id3=n.idx where r.id2 IN(",vault_table,ref_vault_table);

		e=0; j=0; i=0;
		while(i<n_mfs_aux) {
		DBG(5,"i:%i/%i\n",i,n_mfs_aux);
			while(e<n_mfs_feed) {
			DBG(5,"e:%i/%i j:%i\n",e,n_mfs_feed,j);
				if(mfs_feed[e].index<mfs_aux[i].index) {
					if(j==0 || mfs_final[j-1].index!=mfs_feed[e].index) {
						mfs_final[j].index=mfs_feed[e].index;
						mfs_final[j].timestamp=mfs_feed[e].timestamp;
						if(coma==1) strcat(query,",");
						sprintf(aux,"%i",mfs_feed[e].index);
						strcat(query,aux);
						coma=1;
						j++;
					}
				} else if(mfs_feed[e].index>mfs_aux[i].index) {
					mfs_final[j].index=mfs_aux[i].index;
					mfs_final[j].timestamp=mfs_aux[i].timestamp;
					j++;
					i++; //get next aux node
					break; //but check that there is one
				} //else skip dups
				e++;
			}
			//i++; not here!
			if(!(e<n_mfs_feed)) while(i<n_mfs_aux) {
				DBG(5,"i:%i/%i j:%i\n",i,n_mfs_aux,j);
				mfs_final[j].index=mfs_aux[i].index;
				mfs_final[j].timestamp=mfs_aux[i].timestamp;
				j++;
				i++;
			}
		}
		while(e<n_mfs_feed) {
			DBG(5,"e:%i/%i j:%i\n",e,n_mfs_feed,j);
			if(j==0 || mfs_final[j-1].index!=mfs_feed[e].index) {
				mfs_final[j].index=mfs_feed[e].index;
				mfs_final[j].timestamp=mfs_feed[e].timestamp;
				if(coma==1) strcat(query,",");
				sprintf(aux,"%i",mfs_feed[e].index);
				strcat(query,aux);
				coma=1;
				j++;
			}
			e++;
		}
		n_mfs_final=j;

		if(mfs_aux!=NULL) {
			free((void *)mfs_aux);
			mfs_aux=NULL;
			n_mfs_aux=0;
		}
		if(mfs_feed!=NULL) {
			free((void *)mfs_feed);
			mfs_feed=NULL;
			n_mfs_feed=0;
		}
		strcat(query,") order by n.idx ASC");

		if(coma==1) { //be sure that there is something to query

			ret=sql_query(db,query,_WHERE("plVaultGetManifest"));

			if(ret!=SQL_OK) {
				ret=-1;
			} else {
				result = mysql_store_result(db->conn); //store the query results
				if(result==NULL) {
					sql_error(db,_WHERE("plVaultGetManifest inner"));
					ret=-1;
				} else {
					//work with the results
					//get the number of rows
					ret=mysql_num_rows(result);

					if(ret>0) {
						//allocate the struct
						mfs_feed=(t_vault_manifest *)malloc(sizeof(t_vault_manifest) * ret);
						n_mfs_feed=ret;
						*ref=(t_vault_cross_ref *)realloc((void *)*ref,sizeof(t_vault_cross_ref) * (*n_ref+ret));

						for(e=0; e<ret; e++) {
							row=mysql_fetch_row(result);
							mfs_feed[e].index=atoi(row[0]);
							mfs_feed[e].timestamp=(double)atof(row[1]);
							//and also get the references
							(*ref)[e+*n_ref].id1=atoi(row[2]);
							(*ref)[e+*n_ref].id2=atoi(row[3]);
							(*ref)[e+*n_ref].id3=atoi(row[0]);
							(*ref)[e+*n_ref].timestamp=atoi(row[4]);
							(*ref)[e+*n_ref].microseconds=0;
							(*ref)[e+*n_ref].flag=(Byte)atoi(row[5]);
						}
						*n_ref+=ret;
					}
				}
				mysql_free_result(result);
			}
		}
		ret=n_mfs_final;
		*mfs=mfs_final;
		*n_mfs=ret;
	}

	if(mfs_aux!=NULL) {
		free((void *)mfs_aux);
		mfs_aux=NULL;
		n_mfs_aux=0;
	}
	if(mfs_feed!=NULL) {
		free((void *)mfs_feed);
		mfs_feed=NULL;
		n_mfs_feed=0;
	}

	if(ret<=0) {
		plog(db->sql,"ERR: non-zero ret, error in fetching the player manifest!\n");
	}

	sql_end(db);
	return ret;
}

#if 0
/** query the manifest
     all immediatly child nodes (it must get all, but well.. just testing..)
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetManifest(U32 id, t_vault_manifest ** mfs,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[5*1024];
	int ret; //for store result codes
	int i=0;
	int e=0;
	int total=0; //total number of references

	*mfs=NULL; //security

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	//get the first node
	sprintf(query,"Select n.idx,n.timestamp,n.microseconds\
 from %s n where n.idx='%i'",vault_table,id);

	ret=sql_query(db,query,_WHERE("plVaultGetManifest"));

	if(ret!=SQL_OK) {
		ret=-1;
	} else {
		result = mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("plVaultGetManifest inner"));
			ret=-1;
		} else {
			//work with the results
			//get the number of rows
			ret=mysql_num_rows(result);
			DBG(5,"query done, number of rows %i\n",ret);
			if(ret>0) {
				//allocate the struct
				DBG(5,"before a malloc...\n");
				*mfs=(t_vault_manifest *)malloc(ret*sizeof(t_vault_manifest));
				DBG(5,"after a malloc...\n");
				total+=ret;

				DBG(5,"fetch row...\n");
				row=mysql_fetch_row(result);
				(*mfs)[0].index=atoi(row[0]);
				//convert to correct format
				(*mfs)[0].timestamp=(double)atof(row[1]);
				DBG(5,"end fetch...\n");
			}

			//ret=i;
			mysql_free_result(result);
			DBG(5,"result freed..\n");
		}
	}
	if(ret<=0) {
		return ret;
	}

	i=0;
	//now get all nodes
	while(i<total) {
		id=(*mfs)[i].index; //set the node id

		DBG(5,"Getting all nodes id:%i, i:%i, total:%i\n",id,i,total);
		sprintf(query,"Select n.idx,n.timestamp,n.microseconds\
 from %s n, %s r where r.id2='%i' and r.id3=n.idx;",\
 vault_table,ref_vault_table,id);
		ret=sql_query(db,query,_WHERE("plVaultGetManifest"));

		if(ret!=SQL_OK) {
			ret=-1;
		} else {
			result = mysql_store_result(db->conn); //store the query results
			if(result==NULL) {
				sql_error(db,_WHERE("plVaultGetManifest inner"));
				ret=-1;
			} else {
				//work with the results
				//get the number of rows
				ret=mysql_num_rows(result);

				if(ret>0) {
				//allocate the struct
					*mfs=(t_vault_manifest *)realloc((void *)*mfs,\
(ret+total)*sizeof(t_vault_manifest));

					for(e=0; e<ret; e++) {
						row=mysql_fetch_row(result);
						(*mfs)[e+total].index=atoi(row[0]);
						//convert to correct format
						//(*mfs)[e+total].timestamp=atof(row[1]) + ((atof(row[2]))/1000000);
						//(*mfs)[e+total].timestamp=(double)atof(row[1]) + (((long double)(atoi(row[2]))/(long double)1000000));
						(*mfs)[e+total].timestamp=(double)atoi(row[1]) + (((long double)(atoi(row[2]) & 0x00000000)/(long double)1000000));
					}
					total+=ret;
				}
			}
			mysql_free_result(result);
		}
		i++;
	}
	ret=total;

	sql_end(db);
	return ret;
}
#endif

#if 0
/** query the manifest
     all immediatly child nodes (it must get all, but well.. just testing..)
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetMGRS(U32 id, int ** table,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[5*1024];
	int ret; //for store result codes
	int i=0;
	int e=0;
	int total=0; //total number of references
	int honorifieds=0; //number of MGR's found

	*table=NULL; //security
	int * table2=NULL;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

//---- Cutre Soft presents.. --- TODO: Check if the NODE IS A MGR!!!!!!, well...
	honorifieds++;
	if(*table==NULL) {
		*table=(int *)malloc(sizeof(int) * honorifieds);
	} else {
		*table=(int *)realloc((void *)*table,honorifieds*sizeof(int));
	}
	(*table)[honorifieds-1]=id;//atoi(row[0]);
//---- Cutre Soft Inc. --- we create worlds...

	i=-1;
	//now get all nodes
	while(i<total) {
		if(i>=0) {
			id=table2[i]; //set the node id
		}

		DBG(5,"Getting all nodes id:%i, i:%i, total:%i\n",id,i,total);
		sprintf(query,"Select n.idx,n.type\
 from %s n, %s r where r.id3='%i' and r.id2=n.idx;",\
 vault_table,ref_vault_table,id);
		ret=sql_query(db,query,_WHERE("blah"));

		if(ret!=SQL_OK) {
			ret=-1;
		} else {
			result = mysql_store_result(db->conn); //store the query results
			if(result==NULL) {
				ret=-1;
			} else {
				//work with the results
				//get the number of rows
				ret=mysql_num_rows(result);

				if(ret>0) {
					//allocate the struct
					if(table2==NULL) {
						table2=(int *)malloc(sizeof(int) * ret);
					} else {
						table2=(int *)realloc((void *)table2,(ret+total)*sizeof(int));
					}

					for(e=0; e<ret; e++) {
						row=mysql_fetch_row(result);
						table2[e+total]=atoi(row[0]);
						//convert to correct format
						if(atoi(row[1])<=7) { //It's a MGR, it's has the honor to be listed
							honorifieds++;
							if(*table==NULL) {
								*table=(int *)malloc(sizeof(int) * honorifieds);
							} else {
								*table=(int *)realloc((void *)*table,honorifieds*sizeof(int));
							}
							(*table)[honorifieds-1]=atoi(row[0]);
						}
					}
					total+=ret;
				}
			}
			mysql_free_result(result);
		}
		i++;
	}
	ret=honorifieds;
	if(table2!=NULL) {
		free((void *)table2);
	}

	sql_end(db);
	return ret;
}
#endif

/** query the manifest
     all immediatly child nodes (it must get all, but well.. just testing..)
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetMGRS(U32 id, int ** table,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row
	char query[30*1024];
	char aux[300];
	int ret; //for store result codes
	int i=0;
	int e=0;
	int j=0;
	int coma=0;

	*table=NULL;
	int honorifieds=0;

	int * table_feed=NULL;
	int n_table_feed=0;
	int * table_final=NULL;
	int n_table_final=0;
	int * table_aux=NULL;
	int n_table_aux=0;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	//1st set the first node
	table_feed=(int *)malloc(1*sizeof(int));
	table_feed[0]=id;
	n_table_feed=1;

	//now get all nodes
	while(n_table_feed>0) {
		n_table_aux=n_table_final;
		table_aux=table_final;
		table_final=(int *)malloc(sizeof(int) * (n_table_final+n_table_feed));
		n_table_final=0;

		coma=0;
		sprintf(query,"select n.idx,n.type\
 from %s n join %s r on r.id2=n.idx where r.id3 IN(",vault_table,ref_vault_table);

		e=0; j=0; i=0;
		while(i<n_table_aux) {
		DBG(5,"i:%i/%i\n",i,n_table_aux);
			while(e<n_table_feed) {
			DBG(5,"e:%i/%i j:%i\n",e,n_table_feed,j);
				if(table_feed[e]<table_aux[i]) {
					if(j==0 || table_final[j-1]!=table_feed[e]) {
						table_final[j]=table_feed[e];
						if(coma==1) strcat(query,",");
						sprintf(aux,"%i",table_feed[e]);
						strcat(query,aux);
						coma=1;
						j++;
					}
				} else if(table_feed[e]>table_aux[i]) {
					table_final[j]=table_aux[i];
					j++;
					i++; //get next aux node
					break; //but check that there is one
				} //else skip dups
				e++;
			}
			//i++; not here!
			if(!(e<n_table_feed)) while(i<n_table_aux) {
				DBG(5,"i:%i/%i j:%i\n",i,n_table_aux,j);
				table_final[j]=table_aux[i];
				j++;
				i++;
			}
		}
		while(e<n_table_feed) {
			DBG(5,"e:%i/%i j:%i\n",e,n_table_feed,j);
			if(j==0 || table_final[j-1]!=table_feed[e]) {
				table_final[j]=table_feed[e];
				if(coma==1) strcat(query,",");
				sprintf(aux,"%i",table_feed[e]);
				strcat(query,aux);
				coma=1;
				j++;
			}
			e++;
		}
		n_table_final=j;

		if(table_aux!=NULL) {
			free((void *)table_aux);
			table_aux=NULL;
			n_table_aux=0;
		}
		if(table_feed!=NULL) {
			free((void *)table_feed);
			table_feed=NULL;
			n_table_feed=0;
		}
		strcat(query,") order by n.idx ASC");

		if(coma==1) { //be sure that there is something to query

			ret=sql_query(db,query,_WHERE("plVaultGetMGRS"));

			if(ret!=SQL_OK) {
				ret=-1;
			} else {
				result = mysql_store_result(db->conn); //store the query results
				if(result==NULL) {
					sql_error(db,_WHERE("plVaultGetMGRS inner"));
					ret=-1;
				} else {
					//work with the results
					//get the number of rows
					ret=mysql_num_rows(result);

					if(ret>0) {
						//allocate the struct
						table_feed=(int *)malloc(sizeof(int) * ret);
						n_table_feed=ret;

						for(e=0; e<ret; e++) {
							row=mysql_fetch_row(result);
							table_feed[e]=atoi(row[0]);
							if(atoi(row[1])<=7) { //It's a MGR, it's has the honor to be listed
								int aux,aux2;
								aux=atoi(row[0]);
								for(i=0; i<honorifieds; i++) {
									if((*table)[i]==aux) { break; }
									if((*table)[i]>aux) {
										aux2=aux;
										aux=(*table)[i];
										(*table)[i]=aux2;
									}
								}
								if(honorifieds==0 || (*table)[honorifieds-1]<aux) {
									honorifieds++;
									*table=(int *)realloc((void *)*table,honorifieds*sizeof(int));
									(*table)[honorifieds-1]=aux;
								}
							}
						} //O(n) //hmm
					}
				}
				mysql_free_result(result);
			}
		}
		ret=honorifieds;
	}

	if(table_aux!=NULL) {
		free((void *)table_aux);
		table_aux=NULL;
		n_table_aux=0;
	}
	if(table_feed!=NULL) {
		free((void *)table_feed);
		table_feed=NULL;
		n_table_feed=0;
	}
	if(table_final!=NULL) {
		free((void *)table_final);
		table_final=NULL;
		n_table_final=0;
	}

	if(ret<=0) {
		plog(db->sql,"ERR: non-zero ret, error in fetching the MGRS!\n");
	}

	sql_end(db);
	return ret;
}


/** query for parent nodes
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetParentNodes(U32 id, int ** table,st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[5*1024];
	int ret; //for store result codes
	int e;

	*table=NULL; //security

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	DBG(5,"Getting all nodes id:%i\n",id);
	sprintf(query,"Select r.id2 from %s r where r.id3='%i';",ref_vault_table,id);
	ret=sql_query(db,query,_WHERE("blah"));

	if(ret!=SQL_OK) {
		ret=-1;
	} else {
		result = mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			ret=-1;
		} else {
			//work with the results
			//get the number of rows
			ret=mysql_num_rows(result);

			if(ret>0) {
				//allocate the struct
				*table=(int *)malloc(sizeof(int) * ret);

				for(e=0; e<ret; e++) {
					row=mysql_fetch_row(result);
					(*table)[e]=atoi(row[0]);
				}
			}
			mysql_free_result(result);
		}
	}

	sql_end(db);
	return ret;
}

/** Initialitzes the database
*/
int plVaultInitializeDB(st_sql * db) {
	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	//default value
	ret=-1; //incorrect query!!, hum!!
	if(sql_query(db,(char *)db_vault_table_init_script,_WHERE("blah"))!=SQL_OK) {
		ret=-1;
	} else {
		ret=0;
	}

	//if(ret>=0) {
		ret=-1; //incorrect query!!, hum!!
		if(sql_query(db,(char *)db_ref_vault_table_init_script,_WHERE("blah"))!=SQL_OK) {
			//print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
		}
	//}

	sql_end(db);
	return ret;
}

int plVaultGetVersion(st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	int ret; //for store result codes
	//int i; //a iterator

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}
	//query here the database
	sprintf(query,"Select v.torans\
 from %s v where v.type=6;",vault_table); //Only the vault node can be type 6


	//default value
	ret=-1; //incorrect query!!, hum!!

	if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
		if(mysql_errno(db->conn)==1146) { //1062
			ret=plVaultInitializeDB(db);
			if(ret>=0) {
				ret=vault_version;
			}
		} else {
			//print_mysql_error(conn);
			ret=-1;
		}
	} else {
		ret=0;
		result = mysql_store_result(db->conn); //store the query results

		if(result==NULL) {
			//print_mysql_error(conn);
		} else {
			//get the number of rows
			ret=mysql_num_rows(result);

			if(ret>0) {
				row=mysql_fetch_row(result);
				ret=atoi(row[0]);
			} else {
				ret=-1;
			}
			mysql_free_result(result);
		}
	}

	sql_end(db);
	return ret;
}

/** Gets the folder
 returns NULL on error folder if query was succesfull
*/
char * plVaultGetFolder(st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	static char folderr[50];

	char query[1024];
	int ret; //for store result codes
	//int i; //a iterator

	int vault_id;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return NULL;
	}
	//query here the database
	sprintf(query,"Select v.idx,v.entry_name\
 from %s v where v.type=6;",vault_table);

	//default value
	ret=-1; //incorrect query!!, hum!!
	strcpy(folderr,"");

	if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
		if(mysql_errno(db->conn)==1146) { //1062
			ret=plVaultInitializeDB(db);
			if(ret>=0) {
				ret=0;
			}
		} else {
			//print_mysql_error(conn);
			ret=-1;
		}
	} else {
		ret=0;
		result = mysql_store_result(db->conn); //store the query results

		if(result==NULL) {
			//print_mysql_error(conn);
		} else {
			//get the number of rows
			ret=mysql_num_rows(result);

			if(ret>0) {
				row=mysql_fetch_row(result);
				vault_id=atoi(row[0]);
				strcpy(folderr,row[1]);
				//ret=1;
			}
			mysql_free_result(result);
		}
	}
	DBG(5,"Step 5\n");

	if(vault_id!=KVaultID) {
		if(strlen(folderr)!=16) {
			//bad folder name, create a new one
			sprintf(query,"Delete from %s where `idx`=%i;",\
	vault_table,KVaultID);

			if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
				//print_mysql_error(conn);
				ret=-1;
			} else {
				ret=0;
			}
		}
	}

	if(ret==1) {
		if(strlen(folderr)!=16) {
			//bad folder name, create a new one
			sprintf(query,"Delete from %s where `idx`=%i;",\
	vault_table,KVaultID);

			if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
				//print_mysql_error(conn);
				ret=-1;
			} else {
				ret=0;
			}
		}
	}
	DBG(6,"Step 6\n");

	if(ret==0) {
		//then create it
		char folder[9];

		DBG(6,"Create folder step1\n");
		*(Byte *)folder=0x0F;
		*(Byte *)(folder+1)=0x13;
		*(Byte *)(folder+2)=0x37;
		time((time_t *)(folder+3));
		srandom((*(int *)(folder+4))); //<- needs to get more entropy
		*(Byte *)(folder+7)=(Byte)(random()%0x100);
		DBG(6,"Create folder step4\n");
		hex2ascii2((Byte *)folderr,(Byte *)folder,8);
		DBG(6,"Create folder step5\n");
		char build[200];
		sprintf(build,"%s Build: %s Id: %s",text2,BUILD,ID);
		DBG(6,"Create folder step6\n");
		char version[200];
		sprintf(version,"%s %s",SNAME,VERSION);
		DBG(6,"Create folder step6a\n");
		sprintf(query,"Insert into %s (`idx`,type,entry_name,entry_value,entry2,sub_entry_name,torans)\
 VALUES('%i','%i','%s','%s','%s','%s','%i');",\
 vault_table,KVaultID,6,folderr,text1,build,version,vault_version);
		DBG(6,"Create folder step7\n");


		if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
			//print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
		}
	}


	sql_end(db);

	if(ret<0) return NULL;
	else return folderr;
}


/** Gets the folder
 returns -1 on error 0 or >0 if query was succesfull
*/
int plVaultMigrate_from0to1(st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	int ret; //for store result codes

	//int vault_id;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}
	//change from index to idx
	//query here the database
	sprintf(query,"alter table %s change `index` idx INT(11) NOT NULL AUTO_INCREMENT;",vault_table);

	//print2log(f_vlt,"\nplVaultMigrate_from0to1 step1, Query: %s\n",query);

	if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
		//print_mysql_error(conn);
		ret=-1;
	} else {
		ret=0;
	}

	int i;

	if(ret>=0) {
		//now wipe all ages and links
		sprintf(query,"Select v.idx from %s v where v.type=3 or v.type=28 or v.type=33;",vault_table);

//		print2log(f_vlt,"\nplVaultMigrate_from0to1 step2, Query: %s\n",query);

		if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
			//print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
			result = mysql_store_result(db->conn); //store the query results

			if(result==NULL) {
				//print_mysql_error(conn);
			} else {
				//get the number of rows
				ret=mysql_num_rows(result);

				for(i=0; i<ret; i++) {
					row=mysql_fetch_row(result);
					plVaultRemoveNodeRef2(0,atoi(row[0]),1,1,db);
				}
				mysql_free_result(result);
			}
		}
		DBG(5,"Step 5\n");

	}

	//now re-create the hood & the city
	if(ret>=0) {
		/*sprintf(query,"Select v.idx,v.owner from %s v,%s y\
 where y.type=2 and v.owner=y.index and v.type=22 and v.torans=33",vault_table,vault_table);*/
		sprintf(query,"Select v.idx from %s v where v.type=2",vault_table);

		//print2log(f_vlt,"\nplVaultMigrate_from0to1 step3, Query: %s\n",query);

		if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
			//print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
			result = mysql_store_result(db->conn); //store the query results

			if(result==NULL) {
				//print_mysql_error(conn);
			} else {
				//get the number of rows
				ret=mysql_num_rows(result);

				int ki;

				for(i=0; i<ret; i++) {
					row=mysql_fetch_row(result);

					ki=atoi(row[0]);

					//create all the garbage here
					t_vault_node n;
					t_vault_manifest mfs;
					init_node(&n);

					//Now link that player with Ae'gura and with the Hood
					n.type=KAgeInfoNode; // 33
					strcpy((char *)n.entry_name,"city");
					int city_id,hood_id;
					t_AgeInfoStruct ainfo;
					t_SpawnPoint spoint;
					memset(&ainfo,0,sizeof(t_AgeInfoStruct));
					memset(&spoint,0,sizeof(t_SpawnPoint));
					Byte a_guid[17];

					strcpy((char *)spoint.title,"FerryTerminal");
					strcpy((char *)spoint.name,"LinkInPointFerry");

					city_id=plVaultFindNode(&n,&mfs,0,db);
					if(city_id<=0) {
						//then create it
						strcpy((char *)ainfo.filename,"city");
						strcpy((char *)ainfo.instance_name,"Ae'gura");
						strcpy((char *)ainfo.user_name,"Ae'gura");
						strcpy((char *)ainfo.display_name,"Ae'gura");
						generate_newguid(a_guid,(Byte *)"city",0);
						ascii2hex2(ainfo.guid,a_guid,8);
						city_id=plVaultCreateAge(&ainfo);
					}
					if(city_id>0) {
						//Add the linking point
						plVaultAddLinkingPoint(NULL,ki,city_id,&spoint,0);
					}

					strcpy((char *)n.entry_name,"Neighborhood");
					memset(&ainfo,0,sizeof(t_AgeInfoStruct));

					strcpy((char *)spoint.title,"Default");
					strcpy((char *)spoint.name,"LinkInPointDefault");

					hood_id=plVaultFindNode(&n,&mfs,0,db);
					if(hood_id<=0) {
						//then create it
						strcpy((char *)ainfo.filename,"Neighborhood");
						strcpy((char *)ainfo.instance_name,"Neighborhood");
						strcpy((char *)ainfo.user_name,(char *)ghood_name);
						strcpy((char *)ainfo.display_name,(char *)ghood_desc);
						generate_newguid(a_guid,(Byte *)"Neighborhood",0);
						ascii2hex2(ainfo.guid,a_guid,8);
						hood_id=plVaultCreateAge(&ainfo);
					}
					if(hood_id>0) {
						//Add the linking point
						plVaultAddLinkingPoint(NULL,ki,hood_id,&spoint,0);
						plVaultAddOwnerToAge(NULL,hood_id,ki);
					}

					destroy_node(&n);

				}
				mysql_free_result(result);
			}
		}
		DBG(5,"Step 555\n");

	}

	if(ret>=0) {

		//query here the database
		char build[200];
		sprintf(build,"%s Build: %s Id: %s",text2,BUILD,ID);
		char version[200];
		sprintf(version,"%s %s",SNAME,VERSION);

		sprintf(query,"UPDATE %s SET `entry2` = '%s', `sub_entry_name` = '%s', `torans` = '%i'\
 where type=6;",vault_table,build,version,vault_version);

		//print2log(f_vlt,"\nplVaultMigrate_from0to1, Query: %s\n",query);

		if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
			//print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
		}
	}
	sql_end(db);
	return ret;
}

/**
	WARNING. THIS COULD HAVE UNEXPECTED RESULTS
*/
int plVaultMigrate_from1to2(st_sql * db) {
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	int ret; //for store result codes

	//int vault_id;
	int i;

	ret=sql_begin(db);
	if(ret!=SQL_OK) {
		return -1;
	}

	//now re-create the hood & the city
	sprintf(query,"Select v.idx from %s v where v.type=2",vault_table);

	if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
		ret=-1;
	} else {
		ret=0;
		result = mysql_store_result(db->conn); //store the query results

		if(result==NULL) {
			//print_mysql_error(conn);
		} else {
			//get the number of rows
			ret=mysql_num_rows(result);

			int ki;

			for(i=0; i<ret; i++) {
				row=mysql_fetch_row(result);

				ki=atoi(row[0]);

				//create all the garbage here
				t_vault_node n;
				t_vault_manifest mfs;
				init_node(&n);

				//Now link that player with Ae'gura and with the Hood
				n.type=KAgeInfoNode; // 33
				strcpy((char *)n.entry_name,"DniCityX2Finale");
				int city_id; //,hood_id;
				t_AgeInfoStruct ainfo;
				t_SpawnPoint spoint;
				memset(&ainfo,0,sizeof(t_AgeInfoStruct));
				memset(&spoint,0,sizeof(t_SpawnPoint));
				Byte a_guid[17];

				strcpy((char *)spoint.title,"Default");
				strcpy((char *)spoint.name,"LinkInPointDefault");

				city_id=plVaultFindNode(&n,&mfs,0,db);
				if(city_id<=0) {
					//then create it
					strcpy((char *)ainfo.filename,"DniCityX2Finale");
					strcpy((char *)ainfo.instance_name,"DniCityX2Finale");
					strcpy((char *)ainfo.user_name,"");
					strcpy((char *)ainfo.display_name,"");
					generate_newguid(a_guid,(Byte *)"city",0);
					ascii2hex2(ainfo.guid,a_guid,8);
					city_id=plVaultCreateAge(&ainfo);
				}
				if(city_id>0) {
					//Add the linking point
					plVaultAddLinkingPoint(NULL,ki,city_id,&spoint,0);
				}

				destroy_node(&n);

			}
			mysql_free_result(result);
		}
	}

	if(ret>=0) {
		sprintf(query,"UPDATE %s SET `timestamp` = '%i', `entry_value` = '1'\
 where type=29 and entry_name='Blah';",vault_table,(int)time(NULL));

		if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
			//print_mysql_error(conn);
		}
	}
	
		
	if(ret>=0) {

		//query here the database
		char build[200];
		sprintf(build,"%s Build: %s Id: %s",text2,BUILD,ID);
		char version[200];
		sprintf(version,"%s %s",SNAME,VERSION);

		sprintf(query,"UPDATE %s SET `entry2` = '%s', `sub_entry_name` = '%s', `torans` = '%i'\
 where type=6;",vault_table,build,version,vault_version);

		//print2log(f_vlt,"\nplVaultMigrate_from0to1, Query: %s\n",query);

		if(sql_query(db,query,_WHERE("blah"))!=SQL_OK) {
			//print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
		}
	}

	sql_end(db);
	return ret;
}


int plVaultMigrate(int ver,st_sql * db) {

	int ret=0;

	switch(ver) {
		case 0:
			ret=plVaultMigrate_from0to1(db);
			if(ret<0) break;
		case 1:
			//do here stuff to migrate from 1 to 2
			ret=plVaultMigrate_from1to2(db);
			if(ret<0) break;
		case 2:
			//do here stuff to migrate from 2 to 3
			//ret=plVaultMigrate_from2to3();
			//if(ret<0) break;
			break;
		default:
			ret=-1;
	}

	return ret;
}


