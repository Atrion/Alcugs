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
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_CONFIGALIAS_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "alcnet.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
void alcNetSetConfigAliases() {
	// This is called AFTER the current server's settings are copied to global!
	// Also note that, when copying a value and both source and destination exist, the source is used!

	tConfig * cfg;
	tStrBuf val;
	cfg=alcGetConfig();
	
	// save the auth, tracking and vault host and port in global: [auth/tracking/vault] and global: [auth/tracking/vault].port
	// use bind and port in the corresponding section
	cfg->copyValue("auth","bind","global","auth");
	cfg->copyValue("vault","bind","global","vault");
	cfg->copyValue("tracking","bind","global","tracking");
	cfg->copyValue("auth.port","port","global","auth");
	cfg->copyValue("vault.port","port","global","vault");
	cfg->copyValue("tracking.port","port","global","tracking");
	// if available, prefer global: [auth/tracking/vault]_server and global: [auth/tracking/vault]_server_port
	cfg->copyValue("auth","auth_server","global","global");
	cfg->copyValue("vault","vault_server","global","global");
	cfg->copyValue("tracking","tracking_server","global","global");
	cfg->copyValue("auth.port","auth_server_port","global","global");
	cfg->copyValue("vault.port","vault_server_port","global","global");
	cfg->copyValue("tracking.port","tracking_server_port","global","global");

	val=cfg->getVar("bandwidth","global");
	if(!val.isNull()) {
		val=cfg->getVar("net.up","global");
		if(val.isNull()) {
			cfg->copyValue("net.up","bandwidth","global","global");
		}
		val=cfg->getVar("net.down","global");
		if(val.isNull()) {
			cfg->copyValue("net.down","bandwidth","global","global");
		}
	}
	
	// Everything below here is legacy settings support!

	//database
	cfg->copyValue("db.host","db_server","global","global");
	cfg->copyValue("db.name","db_name","global","global");
	cfg->copyValue("db.username","db_username","global","global");
	cfg->copyValue("db.passwd","db_passwd","global","global");
	cfg->copyValue("db.passwd","db_password","global","global");
	cfg->copyValue("db.passwd","db.password","global","global");
	cfg->copyValue("db.port","db_port","global","global");

	//unet
	cfg->copyValue("net.timeout","connection_timeout","global","global");
	cfg->copyValue("net.maxconnections","max_clients","global","global");

	//vault
	cfg->copyValue("vault.hood.name","neighborhood_name","global","global");
	cfg->copyValue("vault.hood.desc","neighborhood_comment","global","global");

	//game
	if (cfg->getVar("game.tmp.hacks.resetting_ages", "global").isNull()) { // don't overwrite the new value if it exists
		cfg->copyValue("game.tmp.hacks.resetting_ages", "tracking.tmp.hacks.resetting_ages", "global", "global");
		cfg->copyValue("game.tmp.hacks.resetting_ages", "tracking.tmp.hacks.resetting_ages", "global", "tracking");
	}
}

} //end namespace alc

