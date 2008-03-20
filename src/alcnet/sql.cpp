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

/**
	URUNET 3+
*/

/* CVS tag - DON'T TOUCH*/
#define __U_SQL_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"

#include <mysql/mysql.h>

#include "alcdebug.h"

namespace alc {

tSQL::tSQL(Byte *host, U16 Port, Byte *username, Byte *password, Byte *dbname, Byte flags, U32 timeout)
{
	this->flags = flags;
	this->timeout = timeout;
	sql = err = log = lnull;
	
	this->host = (Byte *)malloc( (strlen((char *)host)+1) * sizeof(Byte) );
	strcpy((char *)this->host, (char *)host);
	this->port = port;
	this->username = (Byte *)malloc( (strlen((char *)username)+1) * sizeof(Byte) );
	strcpy((char *)this->username, (char *)username);
	this->password = (Byte *)malloc( (strlen((char *)password)+1) * sizeof(Byte) );
	strcpy((char *)this->password, (char *)password);
	this->dbname = (Byte *)malloc( (strlen((char *)dbname)+1) * sizeof(Byte) );
	strcpy((char *)this->dbname, (char *)dbname);
	
	_openlogs();
	if (flags & SQL_LOGQ) { // write MySQL connection debug info
		sql->log("MySQL driver: %s (warning: git doesn\'t update this so it might be outdated)\n", __U_SQL_ID);
		sql->print(" flags: %04X, host: %s, port: %i, user: %s, dbname: %s, using password: ", flags, host, port, username, dbname);
		if (*password != NULL) { sql->print("yes\n"); }
		else { sql->print("no\n"); }
		sql->print(" MySQL client: %s\n\n", mysql_get_client_info());
	}
}

void tSQL::_openlogs(void)
{
	if(flags & SQL_LOG) {
		log = lstd;
		err = lerr;
		if (sql == lnull && (flags & SQL_LOGQ)) {
			sql = new tLog();
			sql->open("sql.log", 4, 0);
		}
	}
}

void tSQL::_closelogs(void)
{
	if (sql != lnull) {
		sql->close();
		delete sql;
		sql = lnull;
	}
}

}


