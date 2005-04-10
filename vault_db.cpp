/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004  The Alcugs H'uru Server Team                          *
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

#ifndef __U_VAULT_DB_
#define __U_VAULT_DB_
/* CVS tag - DON'T TOUCH*/
#define __U_VAULT_DB_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>

#include "data_types.h" //data types
#include "sql.h" //mysql calls
#include "vaultstrs.h" //vault structs
#include "conv_funs.h"
#include "stdebug.h"
#include "vault_tasks.h" //now we need it to perform vault migrations on initizalization
#include "vaultstrs.h"
#include "guid_gen.h"
#include "vnodes.h"

#include "vault_db.h"

const int vault_version=1; //Please only change on major vault format changes, and be sure that
//the plVaultMigrate has implementation for the required changes!!!

/*
// alter table vault add index (val)
// alter table vault drop index val
*/

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
  `data_size` int(11) NOT NULL default '0',\
  `data` longblob NOT NULL,\
  `data2_size` int(11) NOT NULL default '0',\
  `data2` longblob NOT NULL,\
  `unk13` int(11) NOT NULL default '0',\
  `unk14` int(11) NOT NULL default '0',\
  `unk15` int(11) NOT NULL default '0',\
  `unk16` int(11) NOT NULL default '0',\
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

/*-----------------------------------------------------------
  Query the database for the players list
	returns -1 on db error
------------------------------------------------------------*/
int plVaultQueryPlayerList(Byte * guid,st_vault_player ** pls) {
	//connection
	MYSQL *conn;

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];

	int ret; //for store result codes
	int i=0; //a iterator

	DBG(5,"setting *pls to NULL...\n");
	*pls=NULL;

	//DBG(5,"mysql_init....\n");
	//conn = mysql_init(NULL);

	DBG(6,"doing connection to the mysql server...\n");
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		DBG(6,"connection failed!\n");
		return -1;
	}
	DBG(6,"now start the query...\n");
	//query here the database

	sprintf(query,"Select v.idx, v.avie,v.distance\
 from %s v where v.uid='%s';",vault_table,create_str_guid(guid));

	print2log(f_vlt,"\nplVaultQueryPlayerList, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum!!

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
	} else {

		result = mysql_store_result(conn); //store the query results

		if(result==NULL) {
			print_mysql_error(conn);
		} else {
			//get the number of rows
			ret=mysql_num_rows(result);

			if(ret>0) {
				//allocate buffer
				*pls=(st_vault_player *)malloc(sizeof(st_vault_player)*ret);

				//work with the results
				for(i=0; i<ret; i++) {
					row=mysql_fetch_row(result);
					DBG(3,"%i, %s\n",atoi(row[0]),row[1]);
					(*pls)[i].ki=atoi(row[0]);
					strcpy((char *)(*pls)[i].avatar,row[1]);
					(*pls)[i].access_level=15; //atoi(row[2]);
					//(*pls)[i].flags=0; //atoi(row[3]);
					(*pls)[i].flags=atoi(row[2]);
				}
			}
			mysql_free_result(result);
		}
	}

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(3,"RETUNRNING FROM VAULT ret=%i\n",ret);

	return ret;
}

/*-----------------------------------------------------------
  Query the database for the number of players
	returns -1 on db error
------------------------------------------------------------*/
int plVaultGetNumberOfPlayers(Byte * guid) {
	//connection
	MYSQL *conn;

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];

	int ret; //for store result codes
	//U32 i=0; //a iterator

	//DBG(5,"mysql_init....\n");
	//conn = mysql_init(NULL);

	DBG(6,"doing connection to the mysql server...\n");
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		DBG(6,"connection failed!\n");
		return -1;
	}
	DBG(6,"now start the query...\n");
	//query here the database

	sprintf(query,"select count(v.idx)\
 from %s v where v.uid='%s';",vault_table,create_str_guid(guid));

	print2log(f_vlt,"\nplVaultQueryPlayerList, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum!!

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
	} else {

		result = mysql_store_result(conn); //store the query results

		if(result==NULL) {
			print_mysql_error(conn);
		} else {
			row=mysql_fetch_row(result);
			DBG(3,"res: %i\n",atoi(row[0]));
			ret=atoi(row[0]);
			mysql_free_result(result);
		}
	}

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(3,"RETUNRNING FROM VAULT ret=%i\n",ret);

	return ret;
}

/*-----------------------
  Node related code
----------------------*/

/*--------------------------------------------------------------------
  Creates a new node, and returns the id
	returns -1 on db error
	returns 0 if node already exists
	returns node_id if node has been succesfully created
--------------------------------------------------------------------*/
int plVaultCreateNode(t_vault_node * node) {
	//connection
	MYSQL *conn;

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
	query=(char *)malloc((sizeof(char) * query_size) +4000);
	if(query==NULL) { return -1; }
	DBG(7,"Nope malloc worked fine\n");

	en_query=(char *)malloc(sizeof(char) * query_size);
	if(en_query==NULL) { free((void *)query); return -1; }
	DBG(8,"I'm here at line-->>\n");

	//conn = mysql_init(NULL);

	DBG(8,"step 1\n");

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		free((void *)query);
		free((void *)en_query);
		return -1;
	}

	DBG(8,"step 2\n");

	//time hack
	struct timeval tv;
	if(node->timestamp==0) {
		time((time_t *)&node->timestamp);
		gettimeofday(&tv,NULL);
		node->microseconds=tv.tv_usec;
	}
	if(node->timestamp2==0) {
		time((time_t *)&node->timestamp2);
		gettimeofday(&tv,NULL);
		node->microseconds2=tv.tv_usec;
	}
	if(node->timestamp3==0) {
		time((time_t *)&node->timestamp3);
		gettimeofday(&tv,NULL);
		node->microseconds3=tv.tv_usec;
	}
	//end time hack

	//query here the database
	mysql_escape_string(en_query,(const char *)node->age_name,\
	strlen((const char *)node->age_name));

	DBG(8,"step 3\n");

	sprintf(query,"Insert Into %s (type,permissions,owner,unk1,\
timestamp,microseconds,id1,timestamp2,microseconds2,\
timestamp3,microseconds3,age_name,age_guid,torans,distance,elevation,\
unk5,id2,unk7,unk8,unk9,entry_name,sub_entry_name,owner_name,guid,\
str1,str2,avie,uid,entry_value,entry2,unk13,unk14,unk15,unk16,\
data_size,data2_size,data,data2) values('%i','%i','%i','%i','%i',\
'%i','%i','%i','%i','%i','%i','%s','",\
vault_table,node->type,node->permissions,node->owner,node->unk1,\
node->timestamp,node->microseconds,node->id1,\
node->timestamp2,node->microseconds2,\
node->timestamp3,node->microseconds3,en_query);

	hex2ascii2((Byte *)aux,node->age_guid,8);
	mysql_escape_string(en_query,aux,strlen(aux));
	strcat(query,en_query);
	sprintf(en_query,"','%i','%i','%i','%i','%i','%i','%i','%i','",\
node->torans,node->distance,node->elevation,node->unk5,node->id2,\
node->unk7,node->unk8,node->unk9);

	strcat(query,en_query);

	DBG(8,"step 5\n");

	mysql_escape_string(en_query,(const char *)node->entry_name,\
	strlen((const char *)node->entry_name));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->sub_entry_name,\
	strlen((const char *)node->sub_entry_name));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->owner_name,\
	strlen((const char *)node->owner_name));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->guid,\
	strlen((const char *)node->guid));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->str1,\
	strlen((const char *)node->str1));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->str2,\
	strlen((const char *)node->str2));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->avie,\
	strlen((const char *)node->avie));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->uid,\
	strlen((const char *)node->uid));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->entry_value,\
	strlen((const char *)node->entry_value));
	strcat(query,en_query);
	strcat(query,"','");

	mysql_escape_string(en_query,(const char *)node->entry2,\
	strlen((const char *)node->entry2));
	strcat(query,en_query);

	sprintf(en_query,"','%i','%i','%i','%i','%i','%i','",\
node->unk13,node->unk14,node->unk15,node->unk16,\
node->data_size,node->data2_size);
	strcat(query,en_query);

	//binary data here
	mysql_escape_string(en_query,(const char *)node->data,node->data_size);
	strcat(query,en_query);
	strcat(query,"','");
	mysql_escape_string(en_query,(const char *)node->data2,node->data2_size);
	strcat(query,en_query);
	strcat(query,"');");

	//end query creation
	print2log(f_vlt,"\nplVaultCreateNode, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum

	if(mysql_query(conn,query)!=0) {
		if(mysql_errno(conn)==1062) ret=0;
		else print_mysql_error(conn);
	} else {
		//now get the node id
		ret=mysql_insert_id(conn);
	}
	logflush(f_vlt);
	close_mysql_connection(conn);

	free((void *)query);
	free((void *)en_query);
	DBG(3,"RETUNRNING FROM CreateNode VAULT ret=%i\n",ret);

	return ret;
}

/*--------------------------------------------------------------------
  Updates a node, and returns the id
	returns -1 on db error
	returns 0 (not sure about this stuff)
	returns node_id if node has been succesfully created
	(important mask must be set!, only masked items will be updated!)
--------------------------------------------------------------------*/
int plVaultUpdateNode(t_vault_node * node) {
	//connection
	MYSQL *conn;

	char * query=NULL;
	char * en_query=NULL;

	int ret; //for store result codes
	int query_size;

	//allocate memory usage
	query_size=(node->data_size*2)+(node->data2_size*2)+1000;

	query=(char *)malloc(sizeof(char) * query_size+4000);
	if(query==NULL) { return -1; }

	en_query=(char *)malloc(sizeof(char) * query_size);
	if(en_query==NULL) { free((void *)query); return -1; }

	//conn = mysql_init(NULL);

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		free((void *)query);
		free((void *)en_query);
		return -1;
	}

	//time hack
	struct timeval tv;
	if(node->timestamp==0) {
		time((time_t *)&node->timestamp);
		gettimeofday(&tv,NULL);
		node->microseconds=tv.tv_usec;
	}
	//end time hack

	int coma=0;

	//query here the database
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
	if(node->data_size!=0 || (node->unkB & MData1)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"data_size='%i'",node->data_size);
		strcat(query,en_query);
		strcat(query,",data='");
		//binary data here
		mysql_escape_string(en_query,(const char *)node->data,node->data_size);
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}
	if(node->data2_size!=0 || (node->unkB & MData2)) {
		if(coma==1) { strcat(query,","); }
		sprintf(en_query,"data2_size='%i'",node->data2_size);
		strcat(query,en_query);
		strcat(query,",data2='");
		//binary data here
		mysql_escape_string(en_query,(const char *)node->data2,node->data2_size);
		strcat(query,en_query);
		strcat(query,"'");
		coma=1;
	}

	sprintf(en_query," WHERE n.idx='%i';",node->index);
	strcat(query,en_query);
	//end query creation

	print2log(f_vlt,"\nplVaultUpdateNode, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum

	if(mysql_query(conn,query)!=0) {
		if(mysql_errno(conn)==1062) ret=0;
		else print_mysql_error(conn);
	} else {
		//now get the node id
		//ret=mysql_insert_id(conn);
		ret=node->index;
	}
	logflush(f_vlt);
	close_mysql_connection(conn);

	free((void *)query);
	free((void *)en_query);

	return ret;
}

//------------------------------------------------------------------------
/** search for a node,
   returns the id of the node with the modify timestamp
   if it doesn't exist
	 flag=0 -> returns '0' does not exist
	 flag=1 -> creates the node, and returns the id of that node
	 -1 on db error
*/
int plVaultFindNode(t_vault_node * node, t_vault_manifest * mfs,Byte flag) {
	MYSQL *conn;	//connection

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	char en_query[1024];

	int ret; //for store result codes

	//conn = mysql_init(NULL);

	conn=make_mysql_connection();
	if(conn==NULL) {
		return -1;
	}

	//query here the database
	sprintf(query,"select n.idx,timestamp,microseconds from %s n where ",vault_table);

	int coma=0;

	if(node->index!=0) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"n.idx='%i'",node->index);
		strcat(query,en_query);
		coma=1;
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
	if(node->id1!=0 || (node->unkB & MId1)) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"id1='%i'",node->id1);
		strcat(query,en_query);
		coma=1;
	}
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
#if 0
	//binary data not allowed!!
	if(node->data_size!=0) {
		if(coma==1) { strcat(query,"and "); }
		sprintf(en_query,"data_size='%i'",node->data_size);
		strcat(query,en_query);
		strcat(query,",data='");

		//binary data here
		mysql_escape_string(en_query,(const char *)node->data,node->data_size);
		strcat(query,en_query);

		strcat(query,"'");
		coma=1;
	}
#endif
	strcat(query,";");
	//end query creation

	print2log(f_vlt,"\nplVaultFindNode, Query: %s\n",query);

	//default return value
	ret=-1; //db error

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
	} else {
		DBG(4,"before result\n");
		result = mysql_store_result(conn); //store the query results

		if(result==NULL) {
			print_mysql_error(conn);
		} else {
			//work with the results
			DBG(4,"before ret\n");
			ret=mysql_num_rows(result);
			DBG(4,"after ret %i\n",ret); fflush(0);
			print2log(f_vlt,"\nplVaultFindNode, num_rows %i\n",ret);
			if(ret>0 && (row=mysql_fetch_row(result))!=NULL) {
				DBG(5,"just there\n"); fflush(0);
				mfs->index=atoi(row[0]);
				//convert to correct format
				mfs->timestamp=atof(row[1]);// + ((atof(row[2]))/1000000); <---
				ret=mfs->index;
			} else if(flag==1) {
				//create the requested node
				DBG(4,"just here\n");
				struct timeval tv;
				time_t tstamp;
				time(&tstamp);
				gettimeofday(&tv,NULL);

				node->timestamp=tstamp;
				//node->microseconds=tv.tv_usec;

				node->microseconds=(tv.tv_usec/10)*10;

				ret=plVaultCreateNode(node);

				mfs->index=ret;
				mfs->timestamp=(double)tstamp;// + ((double)tv.tv_usec/1000000);
				//mfs->timestamp=(long double)tstamp + ((double)node->microseconds/100000); <---
			} else {
				mfs->index=0;
				mfs->timestamp=0;
				ret=0;
			}
		}
		mysql_free_result(result);
	}

	close_mysql_connection(conn);
	logflush(f_vlt);
	printf("RETUNRNING FROM VAULT FindNode ret=%i\n",ret);
	fflush(0);

	return ret;
}

/*-------------------------------
	Creates a vault reference
	returns -1 on db error
------------------------------*/
int plVaultCreateRef(U32 id1,U32 id2,U32 id3,U32 tstamp,U32 micros,Byte flag) {
	//connection
	MYSQL *conn;

	char query[2*1024];
	int ret; //for store result codes

	//conn = mysql_init(NULL);

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		return -1;
	}

	//time hack
	struct timeval tv;
	if(tstamp==0) {
		time((time_t *)&tstamp);
		gettimeofday(&tv,NULL);
		micros=tv.tv_usec;
	}
	//end time hack

	//query here the database
	sprintf(query,"Insert Into %s (id1,id2,id3,timestamp,microseconds,flag)\
values('%i','%i','%i','%i','%i','%i');",ref_vault_table,id1,id2,id3,tstamp,micros,flag);
	//end query creation

	print2log(f_vlt,"\nplVaultCreateRef, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
	} else {
		ret=1;
	}

	logflush(f_vlt);
	close_mysql_connection(conn);

	DBG(5,"Returning from VAULT plVaultCreateRef RET:%i\n",ret);

	return ret;
}

/*------------------------------------------------------------
  Removes a node (put father=0, son=id to remove a root node)
	with flag=1 to force deletion!
	with flag=2 search for the info nodes and delete them (only if they are at level 1)
	if flag=1 and the node is a MGR it will be deleted!
	\return -1 on db error, 0 if node doens't exist and >0 if success
	(if tree_force==1, deletes the tree, if not, it is not deleted)
-------------------------------------------------------------*/
int plVaultRemoveNodeRef2(U32 father, U32 son, Byte flag,Byte tree_force) {
	//connection
	MYSQL *conn;

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[2*1024];

	int ret; //for store result codes
	int count; //number of nodes
	int type; //the type of the node
	U32 * mfs; //manifest
	int i;
	int pending;
	int level; //the profundity

	int info_nodes=0;
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

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		return -1;
	}

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

		print2log(f_vlt,"\nplVaultRemoveNodeRef search, Query: %s\n",query);
		ret=-1;

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
		} else {
			result = mysql_store_result(conn); //store the query results
			if(result==NULL) {
				print_mysql_error(conn);
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
					print2log(f_vlt,"The selected node %i doesn't exist!\n",son);
				}
				mysql_free_result(result);
			}
		}

		if(ret<0) {
			//something went bad, very bad
		} else {
			ret=-1;
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

			print2log(f_vlt,"\nplVaultRemoveNodeRef delete ref(s), Query: %s\n",query);

			if(mysql_query(conn,query)!=0) {
				print_mysql_error(conn);
				ret=-1;
			} else {
				ret=1;
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

				print2log(f_vlt,"\nplVaultRemoveNodeRef delete node, Query: %s\n",query);

				if(mysql_query(conn,query)!=0) {
					print_mysql_error(conn);
					ret=-1;
				} else {
					ret=1;
				}

				if(ret<0) {
					//go to the next one
				} else {
					ret=-1;
					//get the manifest
					sprintf(query,"Select r.id3 from %s r where\
 r.id2=%i;",ref_vault_table,son);
					print2log(f_vlt,"\nplVaultRemoveNodeRef, Query manifest: %s\n",query);

					if(mysql_query(conn,query)!=0) {
						print_mysql_error(conn);
					} else {
						result = mysql_store_result(conn); //store the query results
						if(result==NULL) {
							print_mysql_error(conn);
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

	logflush(f_vlt);
	close_mysql_connection(conn);

	return ret;
}

//I'm too lazy
int plVaultRemoveNodeRef(U32 father, U32 son, Byte flag) {
	return(plVaultRemoveNodeRef2(father,son,flag,0));
}

/** Set the reference as a seen reference
*/
int plVaultSeetRefSeen(U32 father, U32 son,Byte flag) {
	//connection
	MYSQL *conn;

	char query[500];
	//char aux[300];

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		return -1;
	}

	int ret; //for store result codes
	ret=-1;

	//query here the database
	sprintf(query,"Update %s r SET flag='%i'\
 WHERE r.id2='%i' and r.id3='%i';",ref_vault_table,flag,father,son);

	print2log(f_vlt,"\nplVaultSetRefSeen, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum

	if(mysql_query(conn,query)!=0) {
		if(mysql_errno(conn)==1062) ret=0;
		else print_mysql_error(conn);
	} else {
		ret=1;
	}
	logflush(f_vlt);
	close_mysql_connection(conn);
	DBG(5,"RETUNRNING FROM VAULT SetSeen ret=%i\n",ret);
	return ret;
}

//!check that the player owns that avatar
int plVaultCheckKi(Byte * guid,U32 ki,Byte * avatar_name) {
	//connection
	MYSQL *conn;

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];

	int ret; //for store result codes
	//U32 i=0; //a iterator

	//conn = mysql_init(NULL);

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		return -1;
	}
	//query here the database

	sprintf(query,"Select v.avie from %s v\
 where v.uid = '%s' and v.idx = '%i';",vault_table,create_str_guid(guid),ki);

	print2log(f_vlt,"\nplVaultCheckKi, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum!!

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
	} else {
		result = mysql_store_result(conn); //store the query results

		if(result==NULL) {
			print_mysql_error(conn);
		} else {
			//work with the results
			ret=mysql_num_rows(result);
			if(ret>0) {
				row=mysql_fetch_row(result);
				strcpy((char *)avatar_name,row[0]);
			}
			mysql_free_result(result);
		}
	}

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(5,"RETUNRNING FROM VAULTCheckKi ret=%i\n",ret);
	return ret;
}

/** Query the database for nodes
   (num is the number of nodes, table is a list of nodes)
	 remember to destroy the node buffer!
	 (it tryes to return the exact size of the node stream data)
*/
int plVaultFetchNodes(t_vault_node ** node,int num,U32 * table) {
	MYSQL *conn;  //connection

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	unsigned long *lengths; //the len of each column
	char query[1024];
	int ret; //for store result codes
	int i;

	*node=(t_vault_node *)malloc(sizeof(t_vault_node) * num);
	bzero(*node,sizeof(t_vault_node) * num);
	for(i=0; i<num; i++) {
		(*node)[i].unkA=0x00000002;
		(*node)[i].unkB=0xFFFFFFFF;
		(*node)[i].unkC=0x00000007;
		(*node)[i].data=NULL;
		(*node)[i].data2=NULL;
	}
	//conn = mysql_init(NULL);

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		return -1;
	}
	ret=0;
	//query the db
	for(i=0; i<num; i++) {
		//query here the database
		sprintf(query,"Select v.type,permissions,owner,unk1,timestamp,microseconds,id1,\
timestamp2,microseconds2,timestamp3,microseconds3,age_name,age_guid,torans,distance,\
elevation,unk5,id2,unk7,unk8,unk9,entry_name,sub_entry_name,owner_name,guid,\
str1,str2,avie,uid,entry_value,entry2,data_size,data,data2_size,data2,\
unk13,unk14,unk15,unk16\
 from %s v where v.idx='%i';",vault_table,table[i]);

		print2log(f_vlt,"\nplVaultFetchNodes, Query: %s\n",query);
		logflush(f_vlt);

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
		} else {
			result = mysql_store_result(conn); //store the query results

			if(result==NULL) {
				print_mysql_error(conn);
			} else {
				//work with the results
				//get the number of rows
				//ret=mysql_num_rows(result);

				//allocate the struct
				//while((row=mysql_fetch_row(result))!=NULL) {
				if((row=mysql_fetch_row(result))!=NULL) {
					lengths = mysql_fetch_lengths(result); //get the size of each column
					ret+=4+4+4; //the separators
					(*node)[i].index=table[i];
					ret+=4;
					(*node)[i].type=(Byte)atoi(row[0]);
					ret++;
					(*node)[i].permissions=(U32)atoi(row[1]);
					ret+=4;
					(*node)[i].owner=(S32)atoi(row[2]);
					ret+=4;
					(*node)[i].unk1=(U32)atoi(row[3]);
					ret+=4;
					(*node)[i].timestamp=(U32)atoi(row[4]);
					ret+=4;
					(*node)[i].microseconds=(U32)atoi(row[5]) & 0x00000000;
					//(*node)[i].microseconds=(atoi(row[5])/10)*10;
					//(*node)[i].microseconds=0;
					ret+=4;
					(*node)[i].id1=(U32)atoi(row[6]);
					ret+=4;
					(*node)[i].timestamp2=(U32)atoi(row[7]);
					ret+=4;
					(*node)[i].microseconds2=(U32)atoi(row[8]);
					ret+=4;
					(*node)[i].timestamp3=(U32)atoi(row[9]);
					ret+=4;
					(*node)[i].microseconds3=(U32)atoi(row[10]);
					ret+=4;
					strcpy((char *)(*node)[i].age_name,row[11]);
					ret+=strlen(row[11])+2;
					//if(lengths[12]==8) {
					//	memcpy((*node)[i].age_guid,row[12],8);
					//} else {
						char aux_guid[9];
						ascii2hex2((Byte *)aux_guid,(Byte *)row[12],strlen(row[12]));
						memcpy((*node)[i].age_guid,aux_guid,8);
					//}
					ret+=8;
					(*node)[i].torans=(S32)atoi(row[13]);
					ret+=4;
					(*node)[i].distance=(S32)atoi(row[14]);
					ret+=4;
					(*node)[i].elevation=(S32)atoi(row[15]);
					ret+=4;
					(*node)[i].unk5=(U32)atoi(row[16]);
					ret+=4;
					(*node)[i].id2=(U32)atoi(row[17]);
					ret+=4;
					(*node)[i].unk7=(U32)atoi(row[18]);
					ret+=4;
					(*node)[i].unk8=(U32)atoi(row[19]);
					ret+=4;
					(*node)[i].unk9=(U32)atoi(row[20]);
					ret+=4;
					strcpy((char *)(*node)[i].entry_name,row[21]);
					ret+=strlen(row[21])+2;
					strcpy((char *)(*node)[i].sub_entry_name,row[22]);
					ret+=strlen(row[22])+2;
					strcpy((char *)(*node)[i].owner_name,row[23]);
					ret+=strlen(row[23])+2;
					strcpy((char *)(*node)[i].guid,row[24]);
					ret+=strlen(row[24])+2;
					strcpy((char *)(*node)[i].str1,row[25]);
					ret+=strlen(row[25])+2;
					strcpy((char *)(*node)[i].str2,row[26]);
					ret+=strlen(row[26])+2;
					strcpy((char *)(*node)[i].avie,row[27]);
					ret+=strlen(row[27])+2;
					strcpy((char *)(*node)[i].uid,row[28]);
					ret+=strlen(row[28])+2;
					strcpy((char *)(*node)[i].entry_value,row[29]);
					ret+=strlen(row[29])+2;
					strcpy((char *)(*node)[i].entry2,row[30]);
					ret+=strlen(row[30])+2;
					(*node)[i].data_size=lengths[32];
					ret+=4;
					ret+=lengths[32];
					if(lengths[32]>0) {
						(*node)[i].data=(Byte *)malloc(lengths[32] * sizeof(Byte));
						memcpy((*node)[i].data,row[32],lengths[32]);
					} else {
						(*node)[i].data=NULL;
					}
					(*node)[i].data2_size=lengths[34];
					ret+=4;
					ret+=lengths[34];
					if(lengths[34]>0) {
						(*node)[i].data2=(Byte *)malloc(lengths[34] * sizeof(Byte));
						memcpy((*node)[i].data2,row[34],lengths[34]);
					} else {
						(*node)[i].data2=NULL;
					}
					(*node)[i].unk13=(U32)atoi(row[35]);
					ret+=4;
					(*node)[i].unk14=(U32)atoi(row[36]);
					ret+=4;
					(*node)[i].unk15=(U32)atoi(row[37]);
					ret+=4;
					(*node)[i].unk16=(U32)atoi(row[38]);
					ret+=4;
				} else {
					//ummm, a blank node has 0x8F bytes
					ret+=0x8F;
				}
				DBG(5,"Now size is %i\n",ret);
			}
			mysql_free_result(result);
		}
	}

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(4,"RETUNRNING FROM VAULT fetchNodes ret=%i\n",ret);
	return ret;
}

/* Get the cross reference from an specific node
   (it has anti-loop protection) <-not implemented
   returns the number of references if success
	 returns 0 if fails
*/
int plVaultGetCrossRef(U32 id, t_vault_cross_ref ** ref) {
	//connection
	MYSQL *conn;

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[5*1024];

	int ret; //for store result codes
	int i=-1; //a iterator
	int e=0;
	int total=0; //total number of references

	*ref=NULL; //security

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		return -1;
	}
	//query here the database

	while(i<total) {
		if(i>=0) {
			id=(*ref)[i].id3; //next one
		}

		sprintf(query,"Select r.id1,r.id2,r.id3,r.timestamp,r.microseconds,r.flag\
 from %s r where r.id2='%i';",ref_vault_table,id);
		print2log(f_vlt,"\nplVaultGetCrossRef, Query: %s\n",query);

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
		} else {
			result = mysql_store_result(conn); //store the query results

			if(result==NULL) {
				print_mysql_error(conn);
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

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(4,"RETUNRNING FROM VAULT GetCrossRef ret=%i\n",ret);
	//fflush(0);

	return ret;
}


/** query the manifest
     all immediatly child nodes (it must get all, but well.. just testing..)
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetManifest(U32 id, t_vault_manifest ** mfs) {
	MYSQL *conn; //connection
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[5*1024];
	int ret; //for store result codes
	int i=0; //a iterator
	int total=0;
	int e;

	*mfs=NULL; //security

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		return -1;
	}
	//query here the database

	//get the first node
	sprintf(query,"Select n.idx,n.timestamp,n.microseconds\
 from %s n where n.idx='%i'",vault_table,id);

	print2log(f_vlt,"\nplVaultGetManifest, Query initial node: %s\n",query);

	DBG(4,"doing query...\n");
	ret=-1;
	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
	} else {
		result = mysql_store_result(conn); //store the query results
		if(result==NULL) {
			print_mysql_error(conn);
		} else {
			//work with the results
			//get the number of rows
			ret=mysql_num_rows(result);
			DBG(5,"query done, number of rows %i\n",ret);
			if(ret>0) {
				//allocate the struct
				DBG(5,"after a malloc...\n");
				*mfs=(t_vault_manifest *)malloc(ret*sizeof(t_vault_manifest));
				DBG(5,"before a malloc...\n");
				total+=ret;

				DBG(5,"fetch row...\n");
				row=mysql_fetch_row(result);
				(*mfs)[0].index=atoi(row[0]);
				//convert to correct format
				//(*mfs)[0].timestamp=((double)atof(row[1])) + (((double)(atof(row[2])))/(double)1000000);
				//(*mfs)[0].timestamp=((double)atof(row[1])) + (((double)((atoi(row[2])/10)*10))/(double)1000000);
				//(*mfs)[0].timestamp=(double)atof(row[1]);// + (((long double)(atoi(row[2]) & 0xFFFF0000))/(long double)1000000);
				(*mfs)[0].timestamp=(double)atof(row[1]) + (((long double)(atoi(row[2]) & 0x00000000)/(long double)1000000));
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
		print2log(f_vlt,"\nplVaultGetManifest, Query %i: %s\n",id,query);

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
		} else {
			result = mysql_store_result(conn); //store the query results
			if(result==NULL) {
				print_mysql_error(conn);
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

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(4,"RETUNRNING FROM VAULT getManifest ret=%i\n",ret);
	fflush(0);
	return ret;
}

/** query the manifest
     all immediatly child nodes (it must get all, but well.. just testing..)
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetMGRS(U32 id, int ** table) {
	MYSQL *conn; //connection
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[5*1024];
	int ret; //for store result codes
	int i=0; //a iterator
	int total=0;
	int e;

	int honorifieds=0; //number of MGR's found

	*table=NULL; //security
	int * table2=NULL;

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
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
		print2log(f_vlt,"\nplVaultGetMGRS, Query %i: %s\n",id,query);

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
		} else {
			result = mysql_store_result(conn); //store the query results
			if(result==NULL) {
				print_mysql_error(conn);
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

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(4,"RETUNRNING FROM VAULT getMGRS ret=%i\n",ret);
	fflush(0);
	return ret;
}

/** query for parent nodes
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetParentNodes(U32 id, int ** table) {
	MYSQL *conn; //connection
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[5*1024];
	int ret; //for store result codes
	int e;

	*table=NULL; //security

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		return -1;
	}

	DBG(5,"Getting all nodes id:%i\n",id);
	sprintf(query,"Select r.id2 from %s r where r.id3='%i';",ref_vault_table,id);
	print2log(f_vlt,"\nplVaultGetParentNodes, Query %i: %s\n",id,query);

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
	} else {
		result = mysql_store_result(conn); //store the query results
		if(result==NULL) {
			print_mysql_error(conn);
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

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(4,"RETUNRNING FROM VAULT getParentNodes ret=%i\n",ret);
	fflush(0);
	return ret;
}

/** Initialitzes the database
*/
int plVaultInitializeDB() {
	MYSQL *conn; 	//connection

	int ret; //for store result codes

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		DBG(6,"connection failed!\n");
		return -1;
	}
	//query here the database
	/*sprintf(query,"Select v.index,v.entry_name\
 from %s v where v.type=6;",vault_table);*/

	print2log(f_vlt,"\nplVaultInitializeDB, Query: %s\n %s\n",\
db_vault_table_init_script,db_ref_vault_table_init_script);

	//default value
	ret=-1; //incorrect query!!, hum!!
	if(mysql_query(conn,db_vault_table_init_script)!=0) {
		print_mysql_error(conn);
		ret=-1;
	} else {
		ret=0;
	}

	//if(ret>=0) {
		ret=-1; //incorrect query!!, hum!!
		if(mysql_query(conn,db_ref_vault_table_init_script)!=0) {
			print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
		}
	//}

	close_mysql_connection(conn);

	return ret;
}

int plVaultGetVersion() {
	MYSQL *conn; 	//connection
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	int ret; //for store result codes
	//int i; //a iterator

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		DBG(6,"connection failed!\n");
		return -1;
	}
	//query here the database
	sprintf(query,"Select v.torans\
 from %s v where v.type=6;",vault_table); //Only the vault node can be type 6

	print2log(f_vlt,"\nplVaultGetVersion, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum!!

	if(mysql_query(conn,query)!=0) {
		if(mysql_errno(conn)==1146) { //1062
			ret=plVaultInitializeDB();
			if(ret>=0) {
				ret=vault_version;
			}
		} else {
			print_mysql_error(conn);
		}
	} else {
		ret=0;
		result = mysql_store_result(conn); //store the query results

		if(result==NULL) {
			print_mysql_error(conn);
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
	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(3,"RETUNRNING FROM VAULT GETVERSION ret=%i\n",ret);
	return ret;
}

/** Gets the folder
 returns -1 on error 0 or >0 if query was succesfull
*/
int plVaultGetFolder() {
	MYSQL *conn; 	//connection
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	int ret; //for store result codes
	//int i; //a iterator

	int vault_id;

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		DBG(6,"connection failed!\n");
		return -1;
	}
	//query here the database
	sprintf(query,"Select v.idx,v.entry_name\
 from %s v where v.type=6;",vault_table);

	print2log(f_vlt,"\nplVaultGetFolder, Query: %s\n",query);

	//default value
	ret=-1; //incorrect query!!, hum!!
	strcpy(global_vault_folder_name,"");

	if(mysql_query(conn,query)!=0) {
		if(mysql_errno(conn)==1146) { //1062
			ret=plVaultInitializeDB();
			if(ret>=0) {
				ret=0;
			}
		} else {
			print_mysql_error(conn);
		}
	} else {
		ret=0;
		result = mysql_store_result(conn); //store the query results

		if(result==NULL) {
			print_mysql_error(conn);
		} else {
			//get the number of rows
			ret=mysql_num_rows(result);

			if(ret>0) {
				row=mysql_fetch_row(result);
				vault_id=atoi(row[0]);
				strcpy(global_vault_folder_name,row[1]);
				ret=1;
			}
			mysql_free_result(result);
		}
	}
	DBG(5,"Step 5\n");

	if(vault_id!=KVaultID) {
		if(strlen(global_vault_folder_name)!=16) {
			//bad folder name, create a new one
			sprintf(query,"Delete from %s where `idx`=%i;",\
	vault_table,KVaultID);
			print2log(f_vlt,"\nplVaultGetFolder, deletion Query: %s\n",query);

			if(mysql_query(conn,query)!=0) {
				print_mysql_error(conn);
				ret=-1;
			} else {
				ret=0;
			}
		}
	}

	if(ret==1) {
		if(strlen(global_vault_folder_name)!=16) {
			//bad folder name, create a new one
			sprintf(query,"Delete from %s where `idx`=%i;",\
	vault_table,KVaultID);
			print2log(f_vlt,"\nplVaultGetFolder, deletion Query: %s\n",query);

			if(mysql_query(conn,query)!=0) {
				print_mysql_error(conn);
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
		hex2ascii2((Byte *)global_vault_folder_name,(Byte *)folder,8);
		DBG(6,"Create folder step5\n");
		char build[200];
		sprintf(build,"%s Build: %s Id: %s",text2,BUILD,ID);
		DBG(6,"Create folder step6\n");
		char version[200];
		sprintf(version,"%s %s",SNAME,VERSION);
		DBG(6,"Create folder step6a\n");
		sprintf(query,"Insert into %s (`idx`,type,entry_name,entry_value,entry2,sub_entry_name,torans)\
 VALUES('%i','%i','%s','%s','%s','%s','%i');",\
 vault_table,KVaultID,6,global_vault_folder_name,text1,build,version,vault_version);
		DBG(6,"Create folder step7\n");

		print2log(f_vlt,"\nplVaultGetFolder, creation Query: %s\n",query);

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
		}
	}

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(3,"RETUNRNING FROM VAULT ret=%i\n",ret);

	return ret;
}


/** Gets the folder
 returns -1 on error 0 or >0 if query was succesfull
*/
int plVaultMigrate_from0to1() {
	MYSQL *conn; 	//connection
	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	int ret; //for store result codes

	//int vault_id;

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		DBG(6,"connection failed!\n");
		return -1;
	}

	//change from index to idx
	//query here the database
	sprintf(query,"alter table %s change `index` idx INT(11) NOT NULL AUTO_INCREMENT;",vault_table);

	print2log(f_vlt,"\nplVaultMigrate_from0to1 step1, Query: %s\n",query);

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
		ret=-1;
	} else {
		ret=0;
	}

	int i;

	if(ret>=0) {
		//now wipe all ages and links
		sprintf(query,"Select v.idx from %s v where v.type=3 or v.type=28 or v.type=33;",vault_table);

		print2log(f_vlt,"\nplVaultMigrate_from0to1 step2, Query: %s\n",query);

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
			result = mysql_store_result(conn); //store the query results

			if(result==NULL) {
				print_mysql_error(conn);
			} else {
				//get the number of rows
				ret=mysql_num_rows(result);

				for(i=0; i<ret; i++) {
					row=mysql_fetch_row(result);
					plVaultRemoveNodeRef2(0,atoi(row[0]),1,1);
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

		print2log(f_vlt,"\nplVaultMigrate_from0to1 step3, Query: %s\n",query);

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
			result = mysql_store_result(conn); //store the query results

			if(result==NULL) {
				print_mysql_error(conn);
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
					bzero(&ainfo,sizeof(t_AgeInfoStruct));
					bzero(&spoint,sizeof(t_SpawnPoint));
					Byte a_guid[17];

					strcpy((char *)spoint.title,"FerryTerminal");
					strcpy((char *)spoint.name,"LinkInPointFerry");

					city_id=plVaultFindNode(&n,&mfs,0);
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
						plVaultAddLinkingPoint(-1,ki,city_id,&spoint);
					}

					strcpy((char *)n.entry_name,"Neighborhood");
					bzero(&ainfo,sizeof(t_AgeInfoStruct));

					strcpy((char *)spoint.title,"Default");
					strcpy((char *)spoint.name,"LinkInPointDefault");

					hood_id=plVaultFindNode(&n,&mfs,0);
					if(hood_id<=0) {
						//then create it
						strcpy((char *)ainfo.filename,"Neighborhood");
						strcpy((char *)ainfo.instance_name,"Neighborhood");
						strcpy((char *)ainfo.user_name,(char *)global_neighborhood_name);
						strcpy((char *)ainfo.display_name,(char *)global_neighborhood_comment);
						generate_newguid(a_guid,(Byte *)"Neighborhood",0);
						ascii2hex2(ainfo.guid,a_guid,8);
						hood_id=plVaultCreateAge(&ainfo);
					}
					if(hood_id>0) {
						//Add the linking point
						plVaultAddLinkingPoint(-1,ki,hood_id,&spoint);
						plVaultAddOwnerToAge(-1,hood_id,ki);
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

		print2log(f_vlt,"\nplVaultMigrate_from0to1, Query: %s\n",query);

		if(mysql_query(conn,query)!=0) {
			print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
		}
	}

	close_mysql_connection(conn);
	logflush(f_vlt);
	DBG(3,"RETUNRNING FROM VAULT ret=%i\n",ret);

	return ret;
}


int plVaultMigrate(int ver) {

	int ret=0;

	switch(ver) {
		case 0:
			ret=plVaultMigrate_from0to1();
			if(ret<0) break;
		case 1:
			//do here stuff to migrate from 1 to 2
			//ret=plVaultMigrate_from1to2();
			//if(ret<0) break;
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


#endif
