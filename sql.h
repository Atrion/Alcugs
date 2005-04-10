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
	(At current time only MySQL is supported)
*/


#ifndef __U_SQL_DB_H_
#define __U_SQL_DB_H_
/* CVS tag - DON'T TOUCH*/
#define __U_SQL_DB_H_ID "$Id$"

#ifdef __WIN32__
#include "windoze.h"
#endif

#include <mysql/mysql.h>

#include "data_types.h"
#include "stdebug.h"

//error codes
#define SQL_OK     0 //the requested operation was succesfully performed
#define SQL_ERR   -1 //An error ocurred
#define SQL_NOMEM -2 //Not enough memory to allocate a struct


//flags
#define SQL_LOG      0x01   //Enable logging
#define SQL_LOGQ     0x02   //Log sql querys
#define SQL_CREATEDB 0x04   //Allow db creation if not exists?
#define SQL_STAYCONN 0x08   //Always stay connected to the database (maintains a persistent link)
#define SQL_CREATABL 0x10   //Allow to create tables if them doesn't exist

typedef struct {
	MYSQL * conn; //<! Mysql connection handler
	Byte flags; //<! Flags
	char * host; //<! Database host
	U16 port; //<! Db port
	char * username; //<! Db username
	char * passwd; //<! Db password
	char * name; //<! Database name
	U32 timeout; //<! Connection timeout
	U32 stamp; //<! Time of the last query

	st_log * sql; //<! SQL logging subsystem
	st_log * err; //<! Error logging subsystem
	st_log * log; //<! Standard logging sybsystem

} st_sql;


void sql_init(st_sql * db);
void sql_start(st_sql * db);
void sql_idle(st_sql * db);
void sql_shutdown(st_sql * db);

int sql_begin(st_sql * db);
int sql_query(st_sql * db,char * query,char * name);
void sql_error(st_sql * db,char * msg);
void sql_end(st_sql * db);

#endif

