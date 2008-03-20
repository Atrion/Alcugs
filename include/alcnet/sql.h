/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs Server Team                           *
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
#define SQL_STAYCONN 0x08   // Always stay connected to the database (maintains a persistent link)
#define SQL_CREATABL 0x10   // Allow to create tables if they don't exist

namespace alc {

class tSQL {
public:
	tSQL(const Byte *host, U16 port, const Byte *username, const Byte *password, const Byte *dbname, Byte flags, U32 timeout);
	~tSQL(void)
	{
		_closelogs();
		free(host); free(username); free(password); free(dbname);
	}
	
	static Byte allFlags(void) { return SQL_LOG | SQL_LOGQ | SQL_CREATEDB | SQL_STAYCONN | SQL_CREATABL; }
	static tSQL *createFromConfig(void);
private:
	Byte flags;
	U32 timeout;
	tLog *sql, *err, *log;
	// connection info
	Byte *host, *username, *password, *dbname;
	U16 port;
	
	void _openlogs(void);
	void _closelogs(void);
};

}

#endif
