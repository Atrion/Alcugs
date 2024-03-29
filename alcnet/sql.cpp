/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "sql.h"

#include "netexception.h"
#include <alcmain.h>

#include <ctime>
#include <cstring>

namespace alc {

tSQL::tSQL(const tString &host, uint16_t port, const tString &username, const tString &password, const tString &dbname, uint8_t flags)
 :host(host), username(username), password(password), dbname(dbname),  port(port)
{
	this->flags = flags;
	connection = NULL;
	
	// initialize logging
	sql = new tLog;
	if (flags & SQL_LOG) {
		err = alcGetMain()->err();
		if (flags & SQL_LOGQ) {
			sql->open("sql.log");
			sql->log("MySQL driver loaded\n");
			sql->print(" flags: %04X, host: %s, port: %d, user: %s, dbname: %s, using password: ", this->flags, this->host.c_str(), this->port, this->username.c_str(), this->dbname.c_str());
			if (!password.isEmpty()) { sql->print("yes\n"); }
			else { sql->print("no\n"); }
			sql->print(" MySQL client: %s\n\n", mysql_get_client_info());
		}
	}
	else
		err = new tLog;
}

tSQL::~tSQL(void)
{
	disconnect();
	delete sql;
	if (err != alcGetMain()->err()) delete err;
}

void tSQL::printError(const tString &msg)
{
	err->log("ERR (MySQL): %s: %u %s\n", msg.c_str(), mysql_errno(connection), mysql_error(connection));
}

bool tSQL::connect(bool openDatabase)
{
	if (connection != NULL) {
		err->log("ERR: Connection already established when calling tSQL::connect\n");
		disconnect();
	}
	// init the connection handle (if required)
	DBG(6, "creating connection\n");
	connection = mysql_init(NULL);
	if (connection == NULL)
		throw txNoMem(_WHERE("not enough memory to create MySQL handle"));
	
	return mysql_real_connect(connection, host.c_str(), username.c_str(), password.isEmpty() ? NULL : password.c_str(), openDatabase ? dbname.c_str() : NULL, port, NULL, 0);
}

void tSQL::disconnect(void)
{
	if (connection != NULL) {
		DBG(6, "deleting connection\n");
		mysql_close(connection);
		connection = NULL;
	}
}

bool tSQL::prepare(void)
{	// if the connection is already established or connecting works fine, do nothing
	if (connection) return true;
	if (connect(true)) {
		return query("SET sql_mode=STRICT_ALL_TABLES", "set strict mode", false);
	}
	
	// if the problem is the missing database, establish the connection again (it was closed) and create it
	if(mysql_errno(connection) == 1049 && (flags & SQL_CREATEDB)) { // 1049 = Unknown database '%s'
		disconnect();
		if (!connect(false)) return false; // not even that works, giving up
		
		tString queryStr;
		queryStr.printf("CREATE DATABASE %s", dbname.c_str());
		if (!query(queryStr, "create database", false)) // if we can't create it, stop
			return false;
		
		queryStr.clear();
		queryStr.printf("USE %s", dbname.c_str());
		if (!query(queryStr, "select database", false))
			return false;
		return query("SET sql_mode=STRICT_ALL_TABLES", "set strict mode", false);
	}
	return false;
}

bool tSQL::query(const tString &str, const tString &desc, bool throwOnError)
{
	if (connection == NULL) {
		if (!prepare()) { // DANGER: possible endless loop (query calls prepare calls query...)
			if (throwOnError) throw txDatabaseError(_WHERE("No connection to DB for \"%s\"", desc.c_str()));
			return false;
		}
	}
	
	if (flags & SQL_LOGQ) {
		sql->log("SQL query (%s): ", desc.c_str());
		sql->print(str);
		sql->nl();
	}
	
	if (!mysql_query(connection, str.c_str())) return true; // if everything worked fine, we're done

	// if there's an error, it might be necessary to reconnect
	if (mysql_errno(connection) == 2013 || mysql_errno(connection) == 2006) { // 2013 = Lost connection to MySQL server during query, 2006 = MySQL server has gone away
		printError(desc+" (first attempt, trying again)");
		// reconnect and try again if the connection was lost
		sql->log("Reconnecting...\n");
		disconnect();
		connect(true);
		if (!mysql_query(connection, str.c_str())) return true; // it worked on the 2nd try
	}
	// failed irrecoverably... print the error
	printError(desc);
	if (throwOnError) throw txDatabaseError(_WHERE("SQL error while \"%s\"", desc.c_str()));
	return false;
}

tString tSQL::escape(const tMBuf &buf)
{
	if (connection == NULL) throw txDatabaseError(_WHERE("can't escape a string"));
	char *out = new char[2*buf.size()+1]; // according to mysql doc
	mysql_real_escape_string(connection, out, reinterpret_cast<const char *>(buf.data()), buf.size());
	tString res(out);
	delete []out;
	return res;
}

int tSQL::queryForNumber(const tString &str, const char *desc)
{
	query(str, desc);
	MYSQL_RES *result = storeResult();
	int num = mysql_num_rows(result);
	mysql_free_result(result);
	return num;
}

int tSQL::insertId(void)
{
	if (connection == NULL) throw txDatabaseError(_WHERE("can't get the inserted ID"));
	return mysql_insert_id(connection);
}

int tSQL::affectedRows(void)
{
	if (connection == NULL) throw txDatabaseError(_WHERE("can't get the affected rows"));
	return mysql_affected_rows(connection);
}

MYSQL_RES *tSQL::storeResult(void)
{
	if (connection == NULL) return NULL;
	MYSQL_RES *result = mysql_store_result(connection);
	if (result == NULL) throw new txDatabaseError(_WHERE("Could not fetch result"));
	return result;
}

tSQL *tSQL::createFromConfig(void)
{
	tConfig *cfg = alcGetMain()->config();
	tString var;
	// read basic connection info
	tString host = cfg->getVar("db.host");
	uint16_t port = cfg->getVar("db.port").asInt();
	tString user = cfg->getVar("db.username");
	tString password = cfg->getVar("db.passwd");
	tString dbname = cfg->getVar("db.name");
	
	// additional options
	uint8_t flags = allFlags();
	var = cfg->getVar("db.log");
	if (!var.isEmpty() && !var.asInt()) // on per default
		flags &= ~SQL_LOG; // disable logging
	var = cfg->getVar("db.sql.log");
	if (var.isEmpty() || !var.asInt()) // off per default
		flags &= ~SQL_LOGQ; // disable logging sql statements
	
	return new tSQL(host, port, user, password, dbname, flags);
}

}


