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

/*
	MySQL driver for the Auth server
*/

/* CVS tag - DON'T TOUCH*/
#define __U_AUTH_DB_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include <mysql/mysql.h>

#include "stdebug.h"
#include "data_types.h"
#include "prot.h"
#include "sql.h"

#include "settings_db.h"

#include "auth_db.h"

#include "debug.h"

const int db_auth_version=1;

const char * auth_table = "accounts";

//auth table script
const char * db_auth_table_init_script = "\
CREATE TABLE `accounts` (\
  `uid` int(10) unsigned NOT NULL auto_increment,\
  `guid` varchar(50) NOT NULL default '',\
  `name` varchar(50) NOT NULL default '',\
  `a_level` tinyint(1) unsigned NOT NULL default '15',\
  `last_login` timestamp(14) NOT NULL,\
  `last_ip` varchar(30) NOT NULL default '',\
  `attempts` tinyint(1) unsigned NOT NULL default '0',\
  `last_attempt` timestamp(14) NOT NULL,\
  PRIMARY KEY  (`uid`),\
  UNIQUE KEY `guid` (`guid`),\
  UNIQUE KEY `name` (`name`)\
) TYPE=MyISAM;";


#if 0
//not all versions of mysql allow that
const char * db_auth_table_init_script_cont = "\
CREATE TABLE lastlogs (\
  inc bigint NOT NULL auto_increment,\
  name VARCHAR(50) NOT NULL default '',\
	login DATETIME NOT NULL default '',\
	logout DATETIME NOT NULL default '',\
	KEY name (name),\
	UNIQUE KEY inc (inc)\
) TYPE=MyISAM;\
";
#endif

/**
  Query the database for an specified username
	get the passwd and
	Then return the user access_level
	returns -1 if it was no possible to find a user
*/
int plVaultQueryUserName(Byte * login, U32 * attempt, U32 * att,st_sql * db) {

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	char en_query[512];

	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return AcNotRes; }

	mysql_escape_string(en_query,(const char *)login,strlen((const char *)login));

	sprintf(query,"Select a_level,UNIX_TIMESTAMP(last_attempt),attempts \
from accounts where name='%s';",en_query);

	ret=sql_query(db,query,_WHERE("QueryUserName"));

	if(ret!=SQL_OK) {
		if(mysql_errno(db->conn)==1146 || mysql_errno(db->conn)==1054) {
			ret=plVaultInitializeAuthDB(db,1);
		}
		if(ret>=0) { //try again
			ret=sql_query(db,query,_WHERE("QueryUserName try2"));
		} else {
			ret=AcNotRes;
		}
	}

	if(ret==SQL_OK) {
		result=mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("test"));
		} else {
			//ret=mysql_num_rows(result);
			//work with the results
			row=mysql_fetch_row(result);
			if(row==NULL) {
				ret=-1; //not found!
			} else {
				ret=atoi(row[0]); //the access_level
				*attempt=atoi(row[1]);
				*att=atoi(row[2]);
			}
			mysql_free_result(result);
		}
	}
	sql_end(db);
	return ret;
}

/** A
*/
int plVaultAddUser(Byte * login,Byte * guid,Byte * ip,Byte a_level,st_sql * db) {
	char query[1024];
	char en_query[512];
	char en_guid[60];

	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return -1; }

	mysql_escape_string(en_query,(const char *)login,strlen((const char *)login));
	mysql_escape_string(en_guid,(const char *)guid,strlen((const char *)guid));

	sprintf(query,"INSERT INTO accounts(name,guid,last_ip,\
attempts,last_login,last_attempt,a_level)\
VALUES('%s','%s','%s',0,NOW(),NOW(),%i);",\
en_query,en_guid,ip,a_level);

	if(sql_query(db,query,_WHERE("AddUser"))!=SQL_OK) {
		ret=-1;
	} else {
		ret=mysql_affected_rows(db->conn);
	}

	sql_end(db);
	return ret;
}

/**
 this one should update the user, ehmm.., ah, the number of tryes, I din't remember that
*/
int plVaultUpdateUser(Byte * login,Byte * guid,Byte * ip,int n_tryes,st_sql * db) {
	char query[1024];
	char en_query[512];
	char en_guid[60];

	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return -1; }

	mysql_escape_string(en_query,(const char *)login,strlen((const char *)login));
	mysql_escape_string(en_guid,(const char *)guid,strlen((const char *)guid));

	sprintf(query,"UPDATE accounts SET attempts='%i',last_ip='%s',\
last_attempt=NOW(),guid='%s'\
WHERE name='%s';",\
n_tryes,ip,en_guid,en_query);

	if(sql_query(db,query,_WHERE("UpdateUser"))!=SQL_OK) {
		ret=-1;
	} else {
		ret=mysql_affected_rows(db->conn);
	}

	sql_end(db);
	return ret;
}


int plVaultInitializeAuthDB(st_sql * db,char force) {
	int ret; //for store result codes
	char * key=NULL;
	char work[1024];
	int newone=0;

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return -1; }

	//check internal
	key=plVaultSettingsGetKey("auth_db_internal",db);
	if(key!=NULL) {
		plog(db->err,"ERR: Database version mismatch!\n");
		ret=-1;
	} else {

		key=plVaultSettingsGetKey("auth_db_version",db);
		if(key==NULL) {
			sprintf(work,"%i",db_auth_version);
			ret=plVaultSettingsAddKey("auth_db_version",work,db);
			if(ret<=0) {
				plog(db->err,"ERR: Cannot add key \"auth_db_version\"!\n");
				return -1;
			}
			key=work;
			newone=1;
		}

		ret=atoi(key);

		if(ret==db_auth_version && newone!=1 && force!=1) {
			ret=0; //OK!
		} else if(ret<db_auth_version) {
			plog(db->err,"FixMe: Implement version upgrade from %i to %i\n",ret,db_auth_version);
			ret=-1;
		} else if(ret>db_auth_version) {
			plog(db->err,"FATAL: Databases are too new for this version of the servers!!! - Maxium supported version %i, versus found version %i\n",db_auth_version,ret);
			ret=-1;
		} else {
			//then create it
			if(sql_query(db,(char *)db_auth_table_init_script,_WHERE("InitAuthDB"))!=SQL_OK) {
				ret=-1;
				if(mysql_errno(db->conn)==1050) {
					//OK, attempt to migrate the table from the current unstable and broken state,
					//  to the normal state that *we* expect to find in future versions of the servers.
					ret=sql_query(db,(char *)"ALTER TABLE `accounts` CHANGE `last_login` `last_login` TIMESTAMP(14);",_WHERE("migration from nothing to 1 step 1"));
					if(ret!=SQL_OK && mysql_errno(db->conn)==1054) {
						ret=sql_query(db,(char *)"ALTER TABLE `accounts` ADD `last_login` TIMESTAMP(14) NOT NULL;",_WHERE("migration from nothing to 1 step 1 bis"));
					}
					ret=sql_query(db,(char *)"ALTER TABLE `accounts` ADD `guid` VARCHAR(50) NOT NULL AFTER `uid`;",_WHERE("migration from nothing to 1 step 2"));
					ret=sql_query(db,(char *)"ALTER TABLE `accounts` ADD `last_ip` VARCHAR(30) NOT NULL;",_WHERE("migration from nothing to 1 step 3"));
					ret=sql_query(db,(char *)"ALTER TABLE `accounts` ADD `attempts` tinyint(1) unsigned NOT NULL default '0';",_WHERE("migration from nothing to 1 step 4"));
					ret=sql_query(db,(char *)"ALTER TABLE `accounts` ADD `last_attempt` timestamp(14) NOT NULL;",_WHERE("migration from nothing to 1 step 5"));
					ret=sql_query(db,(char *)"ALTER TABLE `accounts` ADD UNIQUE(`guid`);",_WHERE("migration from nothing to 1 step 6"));
					//well we hope that everything worked well...
					ret=0;
				}
				plog(db->err,"ERR: Cannot initialize the default database tables\n");
			} else {
				ret=mysql_affected_rows(db->conn);
			}
		}
	}
	sql_end(db);
	return ret;
}


#if 0

/*-----------------------------------------------------------
  Query the database for an specified username
	get the passwd and
	Then return the user access_level
	returns -1 if it was no possible to find a user
------------------------------------------------------------*/
int plVaultQueryUserName(Byte * login) {
	//connection
	MYSQL *conn;

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	char en_query[512];

	int ret; //for store result codes

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the AcNotRes
		print2log(f_err,"ERR: Failed connection to the database");
		return AcNotRes;
	}
	//query here the database

	mysql_escape_string(en_query,(const char *)login,strlen((const char *)login));

	sprintf(query,"Select a_level \
from accounts where name='%s'",en_query);

	print2log(f_vlt,"\nplVaultQueryUserName %s, Query: %s\n",login,query);

	//default value
	ret=0;

	if(mysql_query(conn,query)!=0) {

		if(mysql_errno(conn)==1146) { //1062
			ret=plVaultInitializeAuthDB();
			if(ret>=0) {
				ret=0;
				if(mysql_query(conn,query)!=0) {
					print_mysql_error(conn);
					ret=AcNotRes;
				}
			} else {
				ret=AcNotRes;
			}
		} else {
			print_mysql_error(conn);
			ret=AcNotRes;
		}
	}

	if(ret==0) {

		ret=AcNotRes;

		result = mysql_store_result(conn); //store the query results

		DBG(5,"ret:%i\n",ret);
		if(result==NULL) {
			print_mysql_error(conn);
			ret=AcNotRes;
		} else {
			//work with the results
			DBG(5,"ret:%i\n",ret);
			row=mysql_fetch_row(result);
			if(row==NULL) {
				ret=-1; //not found!
			} else {
				ret=atoi(row[0]); //the access_level
			}
			DBG(5,"ret:%i\n",ret);
			mysql_free_result(result);
		}
	}
	logflush(f_vlt);
	close_mysql_connection(conn);

	DBG(5,"Returning from plVaultQueryUserName val: %i\n",ret);
	return ret;
}


int plVaultStoreUserInfo(Byte * login, int exists, int dologin)
{
	//connection
	MYSQL *conn;

	char query[1024];
	char en_query[512];

	int ret=1; //default is ok

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the AcNotRes
		print2log(f_err,"ERR: Failed connection to the database");
		return(-1);
	}

	mysql_escape_string(en_query,(const char *)login,strlen((const char *)login));

  // Update last_login fiels in accounts table
  if (exists) {
		sprintf(query,"UPDATE accounts SET last_login=NOW() WHERE name='%s'",
						en_query);
	} else 
		sprintf(query,"INSERT INTO accounts (name,a_level,last_login) VALUES('%s',%d,NOW())",
						en_query,global_default_access_level);

	if (mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
		ret=0;
	} else {
	  DBG(5,"User %s inserted/updated in accounts table\n", login);
	  // update or insert *latest* user entry in lastlogs table
	  if (exists && !dologin)
			sprintf(query,"UPDATE lastlogs SET logout=NOW() WHERE name='%s' ORDER BY inc DESC LIMIT 1",
							en_query);
		else
	  	sprintf(query,"INSERT INTO lastlogs (name,login) VALUES('%s',NOW())",
							en_query);
	  if(mysql_query(conn,query)!=0) {
		  print_mysql_error(conn);
			ret=0;
		}
	  DBG(5,"ret=%d", ret);
	}
	close_mysql_connection(conn);
	return(ret);
}
#endif
