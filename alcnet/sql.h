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

#ifndef __U_SQL_H
#define __U_SQL_H
/* CVS tag - DON'T TOUCH*/
#define __U_SQL_H_ID "$Id$"


//flags
#define SQL_LOG      0x01   // Enable logging
#define SQL_LOGQ     0x02   // Log sql querys
#define SQL_CREATEDB 0x04   // Allow db creation if not exists?
#define SQL_CREATABL 0x10   // Allow to create tables if they don't exist

#include <alctypes.h>

#include <mysql/mysql.h>

namespace alc {
	
	class tLog;

class tSQL {
public:
	tSQL(const tString &host, uint16_t port, const tString &username, const tString &password, const tString &dbname, uint8_t flags, time_t timeout);
	~tSQL(void);
	
	bool prepare(void); //!< this must be called before each query. it establishes the connection and creates the database if necessary \return true on success, false on error
	bool query(const tString &str, const char *desc, bool throwOnError = true); //!< query the database \return true on success, false on error
	int queryForNumber(const tString &str, const char *desc); //!< query the database (must be a SELECT or SHOW) \return number of resulting rows
	void checkTimeout(void); //!< closes the connection on timeout
	int insertId(void);
	int affectedRows(void);
	
	tString escape(const char *str); //!< escapes the given string
	tString escape(const tString &str) { return escape(str.c_str()); }
	tString escape(const tMBuf &buf); //!< escapes the given data
	MYSQL_RES *storeResult(void);
	
	static uint8_t allFlags(void) { return SQL_LOG | SQL_LOGQ | SQL_CREATEDB | SQL_CREATABL; }
	static tSQL *createFromConfig(void);
private:
	void printError(const char *msg); //!< print the last MySQL error (with the given desctiption) to the error protocol
	
	uint8_t flags;
	time_t timeout, stamp;
	tLog *sql, *err;
	// connection info
	tString host, username, password, dbname;
	uint16_t port;
	
	MYSQL *connection;
	
	bool connect(bool openDatabase); //!< connect to the database \return true on success, false on error
	void disconnect(void); //!< disconnect from the database
};

}

#endif
