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
	MySQL driver abstraction.

	TODO:
		- Separate db drivers in their own folder
		- Implement Postgress and filesystem drivers.

	BUG:
		- There is something not working very well in this code!

*/


#ifndef __U_SQL_DB_
#define __U_SQL_DB_
/* CVS tag - DON'T TOUCH*/
#define __U_SQL_DB_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <mysql/mysql.h>

#include "stdebug.h"
#include "data_types.h"

#include "sql.h"

/*--------------------------------------------
  Closes the mysql connection
---------------------------------------------*/
void close_mysql_connection(MYSQL * conn) {
	//free the object
	DBG(5,"mysql_close\n");
	if (conn) {
		mysql_close(conn);
		conn=NULL;
	} else {
		print2log(f_err,"ERR: MySQL session already closed\n");
	}
}

/*---------------------------------------------
  Starts the mysql connection
----------------------------------------------*/
MYSQL * make_mysql_connection() {

	MYSQL *conn;
	char query[400];

	//init the object
	if ((conn = mysql_init(NULL)) == NULL)
		return(NULL); //failed

	DBG(5,"connecting..\n");

	if(!mysql_real_connect (conn,
				(const char *)global_db_server,
				(const char *)global_db_username,
				(const char *)global_db_passwd,
				(const char *)NULL,0,NULL,0)) {
		print2log(f_err,"ERR: Failed connecting to the db!: %u %s\n",mysql_errno(conn),mysql_error(conn));
		logflush(f_err);
		close_mysql_connection(conn);
		return NULL;
	}

	//now USE global_db_name
	sprintf(query,"USE %s",global_db_name);
	print2log(f_vlt,"\nQuery %s\n",query);

	if(mysql_query(conn,query)!=0) {

		if(mysql_errno(conn)==1049) {

			sprintf(query,"CREATE DATABASE %s",global_db_name);
			print2log(f_vlt,"\nQuery %s\n",query);

			if(mysql_query(conn,query)!=0) {
				DBG(5,"error\n");
				print_mysql_error(conn);
				print2log(f_err,"ERR: Cannot create the database %s, check permissions\n",global_db_name);
				logflush(f_err);
				close_mysql_connection(conn);
				return NULL;
			}

			//now try to use the new created database
			sprintf(query,"USE %s",global_db_name);
			print2log(f_vlt,"\nQuery %s\n",query);

			if(mysql_query(conn,query)!=0) {
				DBG(5,"USE error\n");
				print_mysql_error(conn);
				print2log(f_err,"ERR: Cannot access to the database %s, check permissions\n",global_db_name);
				logflush(f_err);
				close_mysql_connection(conn);
				return NULL;
			}

		} else {
			DBG(5,"USE error\n");
			print_mysql_error(conn);
			print2log(f_err,"ERR: Cannot access to the database %s, check permissions\n",global_db_name);
			logflush(f_err);
			close_mysql_connection(conn);
			return NULL;
		}
	}

	return conn;
}

/*------------------------------------------------------------
  Displays any mysql error
------------------------------------------------------------*/
void print_mysql_error(MYSQL * conn) {
	if(conn!=NULL) {
		print2log(f_vlt,"\nErr: %u : %s\n",mysql_errno(conn),mysql_error(conn));
		logflush(f_vlt);
	}
}

#if 0
int create_mysql_database(char * name, MYSQL *conn) {

#if 0 // WARNING: already connected if calling function!!
	MYSQL *conn;

	conn = mysql_init(NULL);

	//init the object
	if(conn==NULL) return -1; //failed

	if(!mysql_real_connect (
	conn,
	(const char *)global_db_server,
	(const char *)global_db_username,
	(const char *)global_db_passwd,
	NULL,0,NULL,0)) {
		print2log(f_err,"ERR: Failed connecting to the db!: %u %s\n",mysql_errno(conn),mysql_error(conn));
		logflush(f_err);
		return -1;
	}
#endif
	int ret;

	char query[400];

	sprintf(query,"CREATE DATABASE %s",name);

	mysql_ping(conn);

	if(mysql_query(conn,query)!=0) {
		DBG(5,"error");
		print_mysql_error(conn);
		ret=-1;
	} else {
		ret=0;
	}

#if 0 // WARNING: already connected in calling function
	close_mysql_connection(conn);
#endif
	return ret;
}
#endif

#endif

