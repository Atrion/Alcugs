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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#ifndef __MSVC__
#  include <unistd.h>
#endif

#include <mysql/mysql.h>

#include "stdebug.h"
#include "data_types.h"

#include "sql.h"

#include "debug.h"

//Init vars
void sql_init(st_sql * db) {
	memset(db,0,sizeof(st_sql));

	db->flags = SQL_LOG | SQL_LOGQ | SQL_CREATEDB | SQL_STAYCONN | SQL_CREATABL;

	db->timeout = 15*60; //15 minutes by default
}

//startup
void sql_start(st_sql * db) {
	log_init();
	//open log files
	if(db->flags & SQL_LOG) {
		if(f_uru==NULL || f_err==NULL) {
			log_openstdlogs();
		}
		if(db->log==NULL) {
			if(f_uru==NULL) {
				db->log=open_log(NULL,3,DF_STDOUT);
				f_uru=db->log;
			} else {
				db->log=f_uru;
			}
		}
		if(db->err==NULL) {
			if(f_err==NULL) {
				db->err=open_log(NULL,2,DF_STDERR);
				f_err=db->err;
			} else {
				db->err=f_err;
			}
		}
		if(db->sql==NULL && db->flags & SQL_LOGQ) {
			db->sql=open_log("sql.log",4,0);
		} else {
			db->sql=open_log(NULL,4,0);
		}
	}

	//some debbuging stuff
	if(db->flags & SQL_LOGQ) {
		plog(db->sql,"Mysql driver: %s\n",__U_SQL_DB_ID);
		print2log(db->sql,"flags: %04X,host:%s,port:%i,user:%s,dbname:%s,using passwd:",\
		db->flags,db->host,db->port,db->username,db->name);
		if(db->passwd!=NULL) { print2log(db->sql,"yes.\n"); } //%s\n",db->passwd); }
		else { print2log(db->sql,"no.\n\n"); }
		print2log(db->sql,"Mysql client: %s\n\n",mysql_get_client_info());
	}
}

void sql_free(st_sql * db) {
	DBG(5,"sgl_free..\n");
	if(db->conn!=NULL) {
		mysql_close(db->conn);
		db->conn=NULL;
		DBG(5,"connection to db closed...\n");
	}
}

//shutdown
void sql_shutdown(st_sql * db) {
	sql_free(db);
	if(db->log!=f_uru) {
		close_log(db->log);
		db->log=NULL;
	}
	if(db->err!=f_err) {
		close_log(db->err);
		db->err=NULL;
	}
	close_log(db->sql);
	db->sql=NULL;
}

void sql_error(st_sql * db,char * msg) {
	st_log * err=f_err;
	if(db->flags & SQL_LOGQ) {
		err=db->sql;
	}
	plog(err,"ERR: <%s> %u %s\n\n",msg,mysql_errno(db->conn),mysql_error(db->conn));
}

// Flags
// 0x01 directly to the db
int sql_connect(st_sql * db,char flag) {
	char * dbname=NULL;
	if(flag & 0x01) dbname=db->name;

	if(db->conn==NULL) {
		//init the object (if required)
		if ((db->conn = mysql_init(NULL)) == NULL)
			return(SQL_NOMEM); //failed
	}

	DBG(5,"connecting..\n");
	db->stamp=time(NULL);

	if(!mysql_real_connect(db->conn,(const char *)db->host,(const char *)db->username,\
		(const char *)db->passwd,(const char *)dbname,db->port,NULL,0)) {
		return SQL_ERR;
	}
	return SQL_OK;
}


int sql_query(st_sql * db,char * query,char * name) {
	int ret=SQL_OK;
	db->stamp=time(NULL);

	if(db->conn==NULL) {
		ret=sql_begin(db); // DANGER!! Possible infinite loop ading this line
		if(ret!=SQL_OK) return ret;
	}

	if(db->flags & SQL_LOGQ) {
		//plog(db->sql,"Query: <%s> %s\n",name,query);
		//yep, here was a buffer overflow, some querys, where bigger than 1024!
		plog(db->sql,"Query: <%s>",name);
		print2log(db->sql," %s\n",query);
	}

	if(mysql_query(db->conn,query)!=0) {
		ret=SQL_ERR;
		sql_error(db,name);
		if(mysql_errno(db->conn)==2013 || mysql_errno(db->conn)==2006) {
			plog(db->sql,"Reconnecting...\n");
			//just in case, close and open it again
			sql_free(db);
			sql_connect(db,0x01);
			if(mysql_query(db->conn,query)!=0) {
				sql_error(db,name);
			} else {
				ret=SQL_OK;
				lognl(db->sql);
			}
		}
	}
	return ret;
}

//Must be called at the beggining of all querys
int sql_begin(st_sql * db) {
	char query[400];
	int ret=SQL_OK;

	//check if we are already connected
	if(db->conn==NULL) {
		ret=sql_connect(db,0x01);
	}

	if(ret!=SQL_OK) {

		//check if database does not exists
		//  we are always disconnected if it fails, so we must
		//  open the connection again.
		if(mysql_errno(db->conn)==1049 && db->flags & SQL_CREATEDB) {
			//attempt to create it
			sql_free(db);
			ret=sql_connect(db,0x00);

			if(ret==SQL_OK) {
				sprintf(query,"CREATE DATABASE %s",db->name);
				ret=sql_query(db,query,_WHERE("creating db"));
				DBG(5,"num affected rows: %i\n",(int)mysql_affected_rows(db->conn));
				if(ret!=SQL_OK) {
					plog(f_err,"ERR: Cannot create the database, check the permissions\n");
					sql_free(db);
				} else {
					sprintf(query,"USE %s",db->name);
					ret=sql_query(db,query,_WHERE("selecting db"));
					DBG(5,"num affected rows: %i\n",(int)mysql_affected_rows(db->conn));
				}
			}
		}

		if(ret!=SQL_OK) {
			sql_error(db,_WHERE("connecting to db..."));
			plog(f_err,"ERR: It was no possible to connect to the database\n");
		}
	}
	return ret;
}


//Must be called at the end
void sql_end(st_sql * db) {
	logflush(db->sql);
	if(!(db->flags & SQL_STAYCONN)) {
		sql_free(db);
	}
}

//Must be called at least 1 time every 5 seconds (it will close the link, after some time)
void sql_idle(st_sql * db) {
	U32 now;
	now=(U32)time(NULL);

	if((db->flags & SQL_STAYCONN) && db->timeout!=0 && (now-db->stamp)>db->timeout) {
		sql_free(db);
	}
}

#if 0
// g++ sql.cpp -o sql_test stdebug.o debug.o -lmysqlclient
int main(int argc, char argv[]) {
	//connection
	st_sql db;

	sql_init(&db);
	//set here sql params
	db.host = "matrix";
	db.username = "uru";
	db.passwd = "";
	db.name = "uru_sql_driver_test";
	//sql.port=0;
	sql_start(&db);

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[1024];
	char en_query[512];

	int ret; //for store result codes

	int i,e;

	for(e=0; e<5; e++) {

		ret=sql_begin(&db);
		DBG(5,"SQL begin ret:%i\n",ret);

		sprintf(query,"SELECT * from test");
		ret=sql_query(&db,query,_WHERE("test call"));
		if(ret==SQL_OK) {

			result=mysql_store_result(db.conn); //store the query results
			if(result==NULL) {
				sql_error(&db,_WHERE("test"));
			} else {
				ret=mysql_num_rows(result);

				printf("%i rows\n",ret);
				for(i=0; i<ret; i++) {
					row=mysql_fetch_row(result);
					printf("%s,%s\n",row[0],row[1]);
				}
				mysql_free_result(result);
			}
		}

		sql_end(&db);

		for(i=0; i<15; i++) {
			sleep(2);
			sql_idle(&db);
		}
	}

	sql_shutdown(&db);

	log_shutdown();

	return ret;
}
#endif

#endif

