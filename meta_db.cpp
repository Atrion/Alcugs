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
	MySQL driver for the Meta server
	*public implementation*
*/


#ifndef __U_META_DB_
#define __U_META_DB_
/* CVS tag - DON'T TOUCH*/
#define __U_META_DB_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>

#include <mysql/mysql.h>

#include "stdebug.h"
#include "data_types.h"
#include "sql.h"

#include "meta_db.h"

const char * meta_table = "servers";


/*-----------------------------------------------------------
Inserts a server to the list
------------------------------------------------------------*/
int plMetaAddServer(Byte * ip,Byte * address,Byte * name,Byte * desc,Byte * contact,\
Byte * site,U32 dset,Byte * passwd) {
	//connection
	MYSQL *conn;

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[4096];
	char ip2[512];
	char address2[512];
	char name2[512];
	char desc2[512];
	char contact2[512];
	char site2[512];
	char passwd2[512];

	int ret=0; //for store result codes

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the AcNotRes
		print2log(f_err,"ERR: Failed connection to the database");
		return -1;
	}
	//query here the database

	mysql_escape_string(ip2,(const char *)ip,strlen((const char *)ip));
	mysql_escape_string(address2,(const char *)address,strlen((const char *)address));
	mysql_escape_string(name2,(const char *)name,strlen((const char *)name));
	mysql_escape_string(desc2,(const char *)desc,strlen((const char *)desc));
	mysql_escape_string(contact2,(const char *)contact,strlen((const char *)contact));
	mysql_escape_string(site2,(const char *)site,strlen((const char *)site));
	mysql_escape_string(passwd2,(const char *)passwd,strlen((const char *)passwd));

	sprintf(query,"Select id \
from servers where name='%s' and (passwd='%s' or \
lastup<DATE_SUB(NOW(),INTERVAL 2 DAY))",name2,passwd2);

	print2log(f_vlt,"\nplMetaAddServer find %s, Query: %s\n",name,query);

	//default value
	ret=0;

	if(mysql_query(conn,query)!=0) {
		ret=-1;
	}

	if(ret==0) {

		result = mysql_store_result(conn); //store the query results

		if(result==NULL) {
			print_mysql_error(conn);
			ret=-1;
		} else {
			//work with the results
			row=mysql_fetch_row(result);
			if(row==NULL) {
				ret=0; //not found!
			} else {
				ret=atoi(row[0]); //the peer id
			}
			mysql_free_result(result);
		}
	}

	if(ret>=0) { //OK
		if(ret==0) {

			//clean
			sprintf(query,"delete from servers where ip='%s' or address='%s'",ip2,address2);

			print2log(f_vlt,"\nplMetaAddServer delete %s, Query: %s\n",name,query);

			if(mysql_query(conn,query)!=0) {
				print_mysql_error(conn);
			}

			//create a new one

			sprintf(query,"insert into servers (ip,address,name,`desc`,contact,website,\
			dataset,passwd,lastupdate,lastup,status) \
			values ('%s','%s','%s','%s','%s','%s',%i,'%s',NOW(),NOW(),1);",\
			ip2,address2,name2,desc2,contact2,site2,dset,passwd2);

			print2log(f_vlt,"\nplMetaAddServer insert %s, Query: %s\n",name,query);

			if(mysql_query(conn,query)!=0) {
				print_mysql_error(conn);
			}

		} else {
			//update the old one
			sprintf(query,"update servers set ip='%s',address='%s',\
			`desc`='%s',contact='%s',website='%s',dataset='%i',passwd='%s',\
			lastupdate=NOW(),lastup=NOW(),status=1,population=0 \
			where `id`=%i",ip2,address2,desc2,contact2,site2,dset,passwd2,ret);

			print2log(f_vlt,"\nplMetaAddServer update %s, Query: %s\n",name,query);

			if(mysql_query(conn,query)!=0) {
				print_mysql_error(conn);
			}

		}
	}


	logflush(f_vlt);
	close_mysql_connection(conn);

	DBG(5,"Returning from plMetaAddServer val: %i\n",ret);
	return ret;
}

int plMetaUpdateServer(int id,Byte * ip,int status,double ping,int pop) {
	//connection
	MYSQL *conn;

	//MYSQL_RES *result; //the results
	//MYSQL_ROW row; //a mysql row

	char query[4096];
	char ip2[512];

	mysql_escape_string(ip2,(const char *)ip,strlen((const char *)ip));

	int ret=0; //for store result codes

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the AcNotRes
		print2log(f_err,"ERR: Failed connection to the database");
		return -1;
	}
	//query here the database

	mysql_escape_string(ip2,(const char *)ip,strlen((const char *)ip));

	if(status==1) {

		sprintf(query,"update servers set ip='%s',\
		lastupdate=NOW(),lastup=NOW(),status=1,population=%i,ping=%f \
		where `id`=%i",ip2,pop,ping,id);

	} else if(status==0) {

		sprintf(query,"update servers set ip='%s',\
		lastupdate=NOW(),status=0,population=0 \
		where `id`=%i",ip2,id);

	} else {

		sprintf(query,"update servers set ip='%s',\
		lastupdate=NOW(),lastup=0,status=0,population=0 \
		where `id`=%i",ip2,id);

	}

	print2log(f_vlt,"\nplMetaUpdateServer update, Query: %s\n",query);

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
		ret=-1;
	}

	logflush(f_vlt);
	close_mysql_connection(conn);

	DBG(5,"Returning from plMetaUpdateServer val: %i\n",ret);
	return ret;
}


int plMetaGetServers(t_meta_servers ** servers) {
	//connection
	MYSQL *conn;

	MYSQL_RES *result; //the results
	MYSQL_ROW row; //a mysql row

	char query[4096];

	int i,ret=0; //for store result codes

	//conn = mysql_init(NULL);
	conn=make_mysql_connection();
	if(conn==NULL) {
		//if the database is dead, return the AcNotRes
		print2log(f_err,"ERR: Failed connection to the database");
		return -1;
	}
	//query here the database

	sprintf(query,"update servers set status=0,population=0 where \
	lastup<DATE_SUB(NOW(),INTERVAL 36 MINUTE)");

	print2log(f_vlt,"\nplMetaGetServers, Query delete: %s\n",query);

	if(mysql_query(conn,query)!=0) {
		print_mysql_error(conn);
	}

	sprintf(query,"Select `id`,address from servers where status=1 and \
	lastup>DATE_SUB(NOW(),INTERVAL 36 MINUTE) and \
	lastup<DATE_SUB(NOW(),INTERVAL 34 MINUTE)");

	print2log(f_vlt,"\nplMetaGetServers, Query: %s\n",query);

	//default value
	ret=0;

	if(mysql_query(conn,query)!=0) {
		ret=-1;
	}

	if(ret==0) {

		result = mysql_store_result(conn); //store the query results

		if(result==NULL) {
			print_mysql_error(conn);
			ret=-1;
		} else {
			ret=mysql_num_rows(result);
			//work with the results

			*servers=(t_meta_servers *)malloc(sizeof(t_meta_servers) * ret+1);

			for(i=0; i<ret; i++) {
				row=mysql_fetch_row(result);
				(*servers)->id=atoi(row[0]);
				strcpy((char *)((*servers)->address),row[1]);
			}

			mysql_free_result(result);
		}
	}


	logflush(f_vlt);
	close_mysql_connection(conn);

	DBG(5,"Returning from plMetaGetServers val: %i\n",ret);
	return ret;
}

#endif
