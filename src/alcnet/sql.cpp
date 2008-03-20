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

tSQL::tSQL(const Byte *host, U16 port, const Byte *username, const Byte *password, const Byte *dbname, Byte flags, U32 timeout)
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
		sql->log("MySQL driver (%s)\n", __U_SQL_ID);
		sql->print(" flags: %04X, host: %s, port: %d, user: %s, dbname: %s, using password: ", this->flags, this->host, this->port, this->username, this->dbname);
		if (password != NULL) { sql->print("yes (%s)\n", this->password); }
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


