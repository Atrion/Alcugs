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

/* Don't touch - NEVER */
#define __U_MYSQL_ID = "$Id$";
/* */

#include "useful.h" //useful generic functions
#include "debug.h" //for debugging
#include "data_types.h" //for data types used in this file
#include "config_parser.h" //for globals
#include "stdebug.h" //for debugging
#include "protocol.h" //all protocol specs
#include "urunet.h" //network functions
#include "mysql.h"
#include "log.h" // log functions

extern MYSQLINFO mysqli;

char * connectCmd     = "CONNECT",
     * commitCmd      = "COMMIT WORK",
     * rollBackCmd    = "ROLLBACK WORK",
     * releaseCmd     = "COMMIT WORK RELEASE",
     * prepareCmd     = "PREPARE FROM",
     * declareCmd     = "DECLARE CURSOR FOR",
     * openCmd        = "OPEN <cursor>",
     * closeCmd       = "CLOSE <cursor>",
     * updateCmd      = "UPDATE",
     * fetchCmd       = "FETCH INTO",
     * insertCmd      = "INSERT INTO",
     * deleteCmd      = "DELETE",
     * selectCmd      = "SELECT",
     * lockCmd        = "LOCK",
     * eofCmd         = "EOF",
     * selectdbCmd    = "SELECT DB",
     * closedbCmd     = "CLOSE",
     * queryCmd       = "QUERY",
     * storeresultCmd = "STORE RESULT",
     * fetchrowCmd    = "FETCH ROW",
     * grantCmd       = "GRANT";

/*
 * Database allocation
 */
MYSQLINFO *
allocSGBD()
{
  MYSQLINFO *mysqli;

  if (!(mysqli = (MYSQLINFO *)malloc(sizeof(MYSQLINFO))))
		Log(L_ALL, NULL, "ERR: Couldn't allocate memory for MYSQLINFO");
  if (!(mysqli->mysql = (MYSQL *)malloc(sizeof(MYSQL))))
		Log(L_ALL, NULL, "ERR: Couldn't allocate memory for mysqli->mysql");
	return(mysqli);
}

MYSQLINFO *
dupSGBD(MYSQLINFO *sock)
{
  MYSQLINFO *mysqli;

  if (sock->mysql == NULL)
		return(NULL);
  if (!(mysqli = (MYSQLINFO *)malloc(sizeof(MYSQLINFO))))
		Log(L_ALL, NULL, "ERR: Couldn't allocate memory for MYSQLINFO");
  mysqli->mysql = sock->mysql;
	return(mysqli);
}

void
freeSGBD(MYSQLINFO *mysqli)
{
  free(mysqli->mysql);
	free(mysqli);
}

/*
 * Database connection
 */
bool
loginSGBD (MYSQLINFO *mysqli, char *host,char *username,char *password,char *database,bool compress)
{
	static char *fctname = "loginSGBD";

	mysql_init(mysqli->mysql);
	if (compress)
		mysql_options(mysqli->mysql, MYSQL_OPT_COMPRESS, NULL);
	if (!mysql_real_connect(mysqli->mysql, host, username, password, database, 0, NULL,0)) {
		mysql_check_error(mysqli, connectCmd);
		return(FALSE);
	}
	Log(L_ALL, NULL, "%s logged to %s (host:%s)", username, database, host ? host : "localhost");
	mysqli->host     = (host ? strdup(host) : NULL);
	mysqli->username = (username ? strdup(username): NULL);
	mysqli->password = (password ? strdup(password): NULL);
	mysqli->database = (database ? strdup(database): NULL);
	mysqli->compress = compress;
	mysqli->res      = NULL;
	return(TRUE);
}

/*
 * Database disconnection
 */
void
logoutSGBD (MYSQLINFO *mysqli)
{
  Log(L_ALL, NULL, "%s disconnected", mysqli->mysql->user);
  mysql_close(mysqli->mysql);
}

/*
 * Database ping
 */
int
pingSGBD (MYSQLINFO *mysqli)
{
	int result = (mysql_ping(mysqli->mysql) == 0);
	int i;

	if (!result) {
		for (i=1; (i<=10) && !result; i++) {
			Log(L_ALL, NULL, "try=%ld sleeping 10 seconds before trying to reconnect", i);
			sleep(10);
			if (loginSGBD(mysqli, mysqli->host, mysqli->username, mysqli->password, mysqli->database, mysqli->compress))
			  result = (mysql_ping(mysqli->mysql) == 0);
		}
		if (result)
			Log(L_ALL, NULL, "Reconnection OK");
		else
			Log(L_ALL, NULL, "Couldn't reconnect");
	}
	return(result);
}

char *
mysql_backslash_string(MYSQLINFO *mysqli, char *to, char *from)
{
  unsigned long len=strlen(from);
 
  if (!to)
    to = (char *)malloc(len*2+1);
	if (to) {
		mysql_real_escape_string(mysqli->mysql, to, from, len);
		return(to);
	} else return(NULL);
}

MYSQL_RES *
mysql_request (MYSQLINFO *mysqli, char *queryfmt, ...)
{
  va_list args;
  static char query[4096];
  char *fctname = "mysql_request";

  va_start(args, queryfmt);
  vsprintf(query, queryfmt, args);
  va_end(args);

  mysqli->res = NULL;

  if (mysql_query(mysqli->mysql, query)) {
    /* Retry command if reconnection succeded */
    if (!mysql_check_error(mysqli, queryCmd)) {
      if (mysql_query(mysqli->mysql, query)) {
        mysql_check_error(mysqli, queryCmd);
        return(NULL);
      }
    } else
      return(NULL);
  }
  if (strstr(query, "SELECT") || strstr(query, "SHOW") || strstr(query, "DESCRIBE") || strstr(query, "EXPLAIN")) {
    if (!(mysqli->res = mysql_store_result(mysqli->mysql))) {
      mysql_check_error(mysqli, storeresultCmd);
      return(NULL);
    }
  }
  if ((mysqli->affrows = mysql_affected_rows(mysqli->mysql)) == (my_ulonglong)-1) {
    mysql_check_error(mysqli, storeresultCmd);
    return(NULL);
  }
  return(mysqli->res);
}

bool
mysql_end_request(MYSQLINFO *mysqli)
     /* if res was NULL then FALSE
        if error occured then FALSE
        if ok then TRUE
      */
{
  char *fctname = "mysql_end_request";
  bool ret=FALSE;

  if (mysqli->res) {   /* if mysql_query don't show up results (ex:UPDATE), then res=NULL from mysql_request */
    if (!mysql_eof(mysqli->res))
      mysql_check_error(mysqli, eofCmd);
    else {
      mysql_free_result(mysqli->res);
			mysqli->res = NULL;
      ret=TRUE;
    }
  }
  return(ret);
}

/*
 * Database error manipulation
 * In fact we use mysql_check_error() macro which converts into sqlerror() call
 */
int sqlerror (char *function, MYSQLINFO *mysqli, char *cmd, char *srcfile, int srcline)
{
	const char *explain;
	int errnum;
	char message[256];

	if ((errnum = mysql_errno(mysqli->mysql)) != 0) {
		if (function)
			sprintf(message, "MYSQL ERROR in %s at line %d, fct %s", srcfile, srcline, function);
		else
			sprintf(message, "MYSQL ERROR in %s at line %d, fct unknown", srcfile, srcline);
		switch (errnum) {
			case CR_CONNECTION_ERROR:
			case CR_SERVER_GONE_ERROR:
			case CR_SERVER_LOST:
				Log(L_ALL, NULL, "** DATABASE connection lost");
				if (pingSGBD(mysqli)) {
					explain = "Reconnection to database succeded, retrying command";
					errnum = 0;
				} else
					explain = "Reconnection to database FAILED";
				break;
			default:
				explain = (errnum > 2000 ? "Unknown error" : ""); break;
		}
		if (errnum)
			Log(L_ALL, NULL, "%s\n\t%s\n\t%s (%ld)", message, mysql_error(mysqli->mysql), explain, errnum);
		return(errnum);
	} else return(0);
}


