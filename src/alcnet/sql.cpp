/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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

/**
	URUNET 3+
*/

/* CVS tag - DON'T TOUCH*/
#define __U_SQL_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"

#include "alcdebug.h"

namespace alc {

tSQL::tSQL(const Byte *host, U16 port, const Byte *username, const Byte *password, const Byte *dbname, Byte flags, U32 timeout)
{
	this->flags = flags;
	this->timeout = timeout;
	sql = err = log = lnull;
	connection = NULL;
	stamp = 0;
	
	this->host = (Byte *)malloc( (strlen((char *)host)+1) * sizeof(Byte) );
	strcpy((char *)this->host, (char *)host);
	this->port = port;
	this->username = (Byte *)malloc( (strlen((char *)username)+1) * sizeof(Byte) );
	strcpy((char *)this->username, (char *)username);
	this->password = (Byte *)malloc( (strlen((char *)password)+1) * sizeof(Byte) );
	strcpy((char *)this->password, (char *)password);
	this->dbname = (Byte *)malloc( (strlen((char *)dbname)+1) * sizeof(Byte) );
	strcpy((char *)this->dbname, (char *)dbname);
	
	// initialize logging
	if (flags & SQL_LOG) {
		log = lstd;
		err = lerr;
		if (flags & SQL_LOGQ) {
			sql = new tLog("sql.log", 4, 0);
			sql->log("MySQL driver loaded\n");
			sql->print(" flags: %04X, host: %s, port: %d, user: %s, dbname: %s, using password: ", this->flags, this->host, this->port, this->username, this->dbname);
			if (password != NULL) { sql->print("yes (%s)\n", this->password); }
			else { sql->print("no\n"); }
			sql->print(" MySQL client: %s\n\n", mysql_get_client_info());
		}
	}
}

tSQL::~tSQL(void)
{
	disconnect();
	if (sql != lnull) delete sql;
	free(host);
	free(username);
	free(password);
	free(dbname);
}

void tSQL::printError(char *msg)
{
	err->log("ERR (MySQL): %s: %u %s\n", msg, mysql_errno(connection), mysql_error(connection));
}

bool tSQL::connect(bool openDatabase)
{
	if (connection == NULL) {
		// init the connection handle (if required)
		connection = mysql_init(NULL);
		if (connection == NULL)
			throw txNoMem(_WHERE("not enough memory to create MySQL handle"));
	}
	
	DBG(5, "connecting\n");
	stamp = time(NULL);
	return mysql_real_connect(connection, (const char *)host, (const char *)username,
		(const char *)password, openDatabase ? (const char *)dbname : NULL, port, NULL, 0);
}

void tSQL::disconnect(void)
{
	if (connection != NULL) {
		DBG(5, "disconnecting\n");
		mysql_close(connection);
		connection = NULL;
	}
}

bool tSQL::prepare(void)
{	// if the connection is already established or connecting works fine, do nothing
	if (connection) return true;
	if (connect(true)) return true;
	
	// if the problem is the missing database, establish the connection again (it was closed) and create it
	if(mysql_errno(connection) == 1049 && (flags & SQL_CREATEDB)) {
		disconnect();
		if (!connect(false)) return false; // not even that works, giving up
		
		char str[400];
		sprintf(str, "CREATE DATABASE %s", dbname);
		if (!query(str, "create database")) // if we can't create it, stop
			return false;
		sprintf(str,"USE %s", dbname);
		return query(str, "select database");
	}
	return false;
}

bool tSQL::query(char *str, char *desc)
{
	if (connection == NULL) {
		if (!prepare()) return false; // DANGER: possible endless loop (query calls prepare calls query...)
	}
	
	if (flags & SQL_LOGQ)
		sql->log("MySQL query (%s): %s\n", desc, str);
	
	stamp = time(NULL);
	if (!mysql_query(connection, str)) return true; // if everything worked fine, we're done

	// if there's an error, print it
	printError(desc);
	if (mysql_errno(connection) == 2013 || mysql_errno(connection) == 2006) { // reconnect and try again if the connection was lost
		sql->log("Reconnecting...\n");
		disconnect();
		connect(true);
		if (!mysql_query(connection, str)) return true; // it worked on the 2nd try
		// failed again...
		printError(desc);
	}
	return false;
}

void tSQL::checkTimeout(void)
{
	U32 now = time(NULL);
	if (!(flags & SQL_STAYCONN) && timeout > 0 && (now-stamp) > timeout)
		disconnect();
}

tSQL *tSQL::createFromConfig(void)
{
	tConfig *cfg = alcGetConfig();
	tStrBuf var;
	// read basic connection info
	tStrBuf host = cfg->getVar("db.host");
	if (host.isNull()) host.putByte(0); // there must be a final 0 so that the tSQL consutrctor can strcpy it
	U16 port = cfg->getVar("db.port").asU16();
	tStrBuf user = cfg->getVar("db.username");
	if (user.isNull()) user.putByte(0);
	tStrBuf password = cfg->getVar("db.passwd");
	if (password.isNull()) password.putByte(0);
	tStrBuf dbname = cfg->getVar("db.name");
	if (dbname.isNull()) dbname.putByte(0);
	
	// additional options
	U32 timeout = 15*60; // default is 15 minutes
	Byte flags = allFlags();
	var = cfg->getVar("db.log");
	if (!var.isNull() && !var.asByte())
		flags &= ~SQL_LOG; // disable logging
	var = cfg->getVar("db.sql.log");
	if (!var.isNull() && !var.asByte())
		flags &= ~SQL_LOGQ; // disable logging sql statements
	var = cfg->getVar("db.persinstent");
	if (!var.isNull() && !var.asByte())
		flags &= ~SQL_STAYCONN; // disable staying always connected
	var = cfg->getVar("db.timeout");
	if (!var.isNull())
		timeout = var.asU32();
	
	return new tSQL(host.c_str(), port, user.c_str(), password.c_str(), dbname.c_str(), flags, timeout);
}

}


