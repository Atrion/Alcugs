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

/*
	MySQL driver for the Settings Database
*/

/* CVS tag - DON'T TOUCH*/
#define __U_SETTINGS_DB_ID "$Id: auth_db.cpp,v 1.6 2004/12/02 22:20:37 almlys Exp $"

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

#include "debug.h"

//Please only change on major vault format changes, and be sure that
//the plVaultMigrate has implementation for the required changes!!!
const int db_settings_version=1;

const char * settings_table = "settings";

const char * db_settings_table_init_script = "\
CREATE TABLE `settings` (\
  `name` varchar(200) NOT NULL default '',\
  `value` varchar(200) NOT NULL default '',\
  PRIMARY KEY  (`name`)\
) TYPE=MyISAM;";


int plVaultInitSettingsDB(st_sql * db) {
	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return -1; }

	if(sql_query(db,(char *)db_settings_table_init_script,_WHERE("InitSettingsDB"))!=SQL_OK) {
		ret=-1;
		plog(db->err,"ERR: Cannot initialize the default database tables\n");
	} else {
		ret=mysql_affected_rows(db->conn);
	}

	sql_end(db);
	return ret;
}

/**
	Get a param
*/
char * plVaultSettingsGetKey(char * what,st_sql * db) {

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	static char value[255];

	char query[1024];
	char en_query[512];

	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return NULL; }

	mysql_escape_string(en_query,(const char *)what,strlen((const char *)what));

	sprintf(query,"Select LCASE(value)\
from settings where name='%s';",en_query);

	ret=sql_query(db,query,_WHERE("SettingsGetKey"));

	if(ret!=SQL_OK) {
		if(mysql_errno(db->conn)==1146) {
			ret=plVaultInitSettingsDB(db);
		}
		if(ret>=0) { //try again
			ret=sql_query(db,query,_WHERE("SettingsGetKey try2"));
		} else {
			ret=-1;
		}
	}

	if(ret==SQL_OK) {
		result=mysql_store_result(db->conn); //store the query results
		if(result==NULL) {
			sql_error(db,_WHERE("SettingsGetKey"));
		} else {
			//ret=mysql_num_rows(result);
			//work with the results
			row=mysql_fetch_row(result);
			if(row==NULL) {
				ret=-1; //not found!
			} else {
				strcpy((char *)value,row[0]); //copy the returned value
			}
			mysql_free_result(result);
		}
	}

	sql_end(db);

	if(ret<0) return NULL;
	return (char *)value;
}

/** Add a param
*/
int plVaultSettingsAddKey(char * what,char * value,st_sql * db) {
	char query[1024];
	char en_query[512];
	char en_value[512];

	int ret; //for store result codes

	ret=sql_begin(db);
	if(ret!=SQL_OK) { return -1; }

	mysql_escape_string(en_query,(const char *)what,strlen((const char *)what));
	mysql_escape_string(en_value,(const char *)value,strlen((const char *)value));

	sprintf(query,"INSERT INTO settings(name,value)\
VALUES('%s','%s');",\
en_query,en_value);

	if(sql_query(db,query,_WHERE("Addkey"))!=SQL_OK) {
		ret=-1;
	} else {
		ret=mysql_affected_rows(db->conn);
	}

	sql_end(db);
	return ret;
}

//TODO add, SET key


