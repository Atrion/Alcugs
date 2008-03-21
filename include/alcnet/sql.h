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

#include <mysql/mysql.h>

//flags
#define SQL_LOG      0x01   // Enable logging
#define SQL_LOGQ     0x02   // Log sql querys
#define SQL_CREATEDB 0x04   // Allow db creation if not exists?
#define SQL_STAYCONN 0x08   // Always stay connected to the database (maintains a persistent link)
#define SQL_CREATABL 0x10   // Allow to create tables if they don't exist

namespace alc {

class tSQL {
public:
	tSQL(const Byte *host, U16 port, const Byte *username, const Byte *password, const Byte *dbname, Byte flags, U32 timeout);
	~tSQL(void);
	
	bool prepare(void); //!< this must be called before each query. it establishes the connection and creates the database if necessary \return true on success, false on error
	void printError(const char *msg); //!< print the last MySQL error (with the given desctiption) to the error protocol
	bool query(const char *str, const char *desc); //!< query the database \return true on success, false on error
	void checkTimeout(void); //!< closes the connection on timeout
	inline int affectedRows(void) {
		return (connection == NULL) ? -1 : mysql_affected_rows(connection);
	}
	char *escape(char *str); //!< escapes the given string and returns the point to a static array. max string length is 511
	MYSQL_RES *storeResult(void);
	
	static Byte allFlags(void) { return SQL_LOG | SQL_LOGQ | SQL_CREATEDB | SQL_STAYCONN | SQL_CREATABL; }
	static tSQL *createFromConfig(void);
private:
	Byte flags;
	U32 timeout, stamp;
	tLog *sql, *err;
	// connection info
	Byte *host, *username, *password, *dbname;
	U16 port;
	
	MYSQL *connection;
	
	bool connect(bool openDatabase); //!< connect to the database \return true on success, false on error
	void disconnect(void); //!< disconnect from the database
};

}

#endif
