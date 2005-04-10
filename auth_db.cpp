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
	MySQL driver for the Auth server
	*public implementation*
*/


#ifndef __U_AUTH_DB_
#define __U_AUTH_DB_
/* CVS tag - DON'T TOUCH*/
#define __U_AUTH_DB_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>

#include <mysql/mysql.h>

#include "stdebug.h"
#include "data_types.h"
#include "prot.h"
#include "sql.h"

#include "auth_db.h"

const char * auth_table = "accounts";

const char * db_auth_table_init_script = "\
CREATE TABLE `accounts` (\
  `uid` int(10) unsigned NOT NULL auto_increment,\
  `name` varchar(50) NOT NULL default '',\
  `a_level` tinyint(1) unsigned NOT NULL default '15',\
	`last_login` DATETIME default '',\
  PRIMARY KEY  (`uid`),\
  UNIQUE KEY `name` (`name`)\
) TYPE=MyISAM;";

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

int plVaultInitializeAuthDB() {
	MYSQL *conn; 	//connection

	int ret; //for store result codes

	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the -1
		DBG(6,"connection failed!\n");
		return -1;
	}

	print2log(f_vlt,"\nplVaultInitializeDB, Query: %s\n",\
db_auth_table_init_script);

	//default value
	ret=-1; //incorrect query!!, hum!!
	if(mysql_query(conn,db_auth_table_init_script)!=0) {
		print_mysql_error(conn);
		ret=-1;
	} else {
		ret=0;
	}

	if(global_auto_register_account) {

		print2log(f_vlt,"\nplVaultInitializeDB, Query: %s\n",\
	db_auth_table_init_script_cont);

		//default value
		ret=-1; //incorrect query!!, hum!!
		if(mysql_query(conn,db_auth_table_init_script_cont)!=0) {
			print_mysql_error(conn);
			ret=-1;
		} else {
			ret=0;
		}

	}

	close_mysql_connection(conn);

	return ret;
}


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
