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

#ifndef _MYSQL_HURU_H
#define _MYSQL_HURU_H
#define _MYSQL_HURU_H_ID "$Id$"

#include <mysql/mysql.h>
#include <mysql/errmsg.h>

#define mysql_check_error(x, y) sqlerror(fctname, x, y, __FILE__, __LINE__)

#ifdef TRUE
# undef TRUE
# undef FALSE
#endif

#define TRUE 1
#define FALSE 0

/* Struct 'mysqli', ALL executables should allocate one using allocSGBD,
 * saving connection informations in order to auto-reconnect to database */
typedef struct {
	MYSQL *mysql;	// to be allocated by initSGBD()
	MYSQL_RES *res;
	MYSQL_ROW row;
	my_ulonglong affrows;
	char *host;
	char *username;
	char *password;
	char *database;
	bool compress;
} MYSQLINFO;

MYSQLINFO *allocSGBD();
MYSQLINFO *dupSGBD(MYSQLINFO *);
void freeSGBD(MYSQLINFO *);
bool loginSGBD (MYSQLINFO *, char *,char *,char *,char *,bool);
void logoutSGBD (MYSQLINFO *);
int pingSGBD (MYSQLINFO *);
MYSQL_RES *mysql_request (MYSQLINFO *, char *, ...);
char *mysql_backslash_string(MYSQLINFO *, char *, char *);
bool mysql_end_request(MYSQLINFO *);
int sqlerror(char *, MYSQLINFO *, char *, char *, int);

#endif /* _MYSQL_HURU_H */

