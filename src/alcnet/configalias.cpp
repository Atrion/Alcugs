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

	tConfig * cfg;
	tStrBuf val;
	cfg=alcGetConfig();
	
	// save the auth, tracking and vault host and port in global: [auth/tracking/vault] and global: [auth/tracking/vault].port
	// use bind and port in the corresponding section
	cfg->copyKey("auth","bind","global","auth");
	cfg->copyKey("vault","bind","global","vault");
	cfg->copyKey("tracking","bind","global","tracking");
	cfg->copyKey("auth.port","port","global","auth");
	cfg->copyKey("vault.port","port","global","vault");
	cfg->copyKey("tracking.port","port","global","tracking");
	// if available, prefer global: [auth/tracking/vault]_server and global: [auth/tracking/vault]_server_port
	cfg->copyKey("auth","auth_server","global","global");
	cfg->copyKey("vault","vault_server","global","global");
	cfg->copyKey("tracking","tracking_server","global","global");
	cfg->copyKey("auth.port","auth_server_port","global","global");
	cfg->copyKey("vault.port","vault_server_port","global","global");
	cfg->copyKey("tracking.port","tracking_server_port","global","global");

	val=cfg->getVar("bandwidth","global");
	if(!val.isNull()) {
		val=cfg->getVar("net.up","global");
		if(val.isNull()) {
			cfg->copyKey("net.up","bandwidth","global","global");
		}
		val=cfg->getVar("net.down","global");
		if(val.isNull()) {
			cfg->copyKey("net.down","bandwidth","global","global");
		}
	}
	
	// Everything below here is legacy settings support!

#if 0
	//check some things
	// unused (no CGAS support implemented or planned):
	val=cfg->getVar("allow_unknown_accounts","global");
	if(!val.isNull() && val.asByte()==0) {
		val="";
		val.printf("%i",AcNotActivated);
		cfg->setVar(val.c_str(),"default_access_level","global");
	}

	cfg->copyKey("auth.reg","auto_register_account","global","global");
	cfg->copyKey("auth.reg.db","auto_register_account","global","global");

	val=cfg->getVar("default_access_level","global");
	if(!val.isNull() && val.asU32()>=AcNotRes) {
		val="";
		val.printf("%i",AcPlayer);
		cfg->setVar(val.c_str(),"default_access_level","global");
	}
#endif

	//database
	cfg->copyKey("db.host","db_server","global","global");
	cfg->copyKey("db.name","db_name","global","global");
	cfg->copyKey("db.username","db_username","global","global");
	cfg->copyKey("db.passwd","db_passwd","global","global");
	cfg->copyKey("db.passwd","db_password","global","global");
	cfg->copyKey("db.passwd","db.password","global","global");
	cfg->copyKey("db.port","db_port","global","global");

	//unet
	cfg->copyKey("net.timeout","connection_timeout","global","global");
	cfg->copyKey("net.maxconnections","max_clients","global","global");
	//cfg->copyKey("max_population","max_players","global","global");

	val=cfg->getVar("disabled","global");
	if(!val.isNull()) {
		cfg->copyKey("stop","disabled","global","global");
	}

	//shard public info
	/*cfg->copyKey("shard.name","shard_name","global","global");
	cfg->copyKey("shard.website","shard_website","global","global");
	cfg->copyKey("shard.desc","shard_description","global","global");
	cfg->copyKey("shard.passwd","shard_password","global","global");
	cfg->copyKey("shard.passwd","shard.password","global","global");
	cfg->copyKey("shard.contact","shard_contact","global","global");

	val=cfg->getVar("shard.name","global");
	if(val.isNull()) {
		cfg->setVar("Unnamed Alcugs shard","shard.name","global");
	}
	val=cfg->getVar("shard.website","global"); // http://huru.almlys.dyns.net/unconfigured.php does no longer exist
	if(val.isNull()) {
		cfg->setVar("http://huru.almlys.dyns.net/unconfigured.php",\
		"shard.website","global");
	}
	val=cfg->getVar("shard.desc","global");
	if(val.isNull()) {
		cfg->setVar("Generic unconfigured Alcugs shard",\
		"shard.desc","global");
	}
	cfg->copyKey("meta","enable_metaserver","global","global");*/

	cfg->copyKey("vault.hood.name","neighborhood_name","global","global");
	cfg->copyKey("vault.hood.desc","neighborhood_comment","global","global");

	//protocol stuff
	//0-latest
	//1-unet2
	//2-unet3
	//3-unet3+
	val=cfg->getVar("auth.oldprotocol","global");
	if(!val.isNull()) {
		if(val.asByte()) {
			cfg->setVar("2","auth.protocol","global");
		}
	}
	val=cfg->getVar("vault.oldprotocol","global");
	if(!val.isNull()) {
		if(val.asByte()) {
			cfg->setVar("2","vault.protocol","global");
		}
	}
	val=cfg->getVar("tracking.oldprotocol","global");
	if(!val.isNull()) {
		if(val.asByte()) {
			cfg->setVar("2","tracking.protocol","global");
		}
	}

#if 0
	val=cfg->getVar("dataset","global");
	if(val.isNull()) {
		val="-1";
		cfg->setVar("-1","dataset","global");
	}
	
	switch(val.asS32()) {
		case -1:
			cfg->setVar("undef","dataset.tag","global");
			break;
		case 0:
			cfg->setVar("custom","dataset.tag","global");
			break;
		case 1:
			cfg->setVar("prime","dataset.tag","global");
			break;
		case 2:
			cfg->setVar("prime12","dataset.tag","global");
			break;
		case 3:
			cfg->setVar("live","dataset.tag","global");
			break;
		case 4:
			cfg->setVar("todni","dataset.tag","global");
			break;
		case 5:
			cfg->setVar("tpots","dataset.tag","global");
			break;
		case 6:
			cfg->setVar("stable","dataset.tag","global");
			break;
		case 7:
			cfg->setVar("testing","dataset.tag","global");
			break;
		case 8:
			cfg->setVar("hacking","dataset.tag","global");
			break;
		case 9:
			cfg->setVar("uu","dataset.tag","global");
			break;
		default:
			cfg->copyKey("dataset.tag","dataset","global","global");
			break;
	}
#endif

	if (cfg->getVar("game.tmp.hacks.resetting_ages", "global").isNull()) {
		cfg->copyKey("game.tmp.hacks.resetting_ages", "tracking.tmp.hacks.resetting_ages", "global", "global");
		cfg->copyKey("game.tmp.hacks.resetting_ages", "tracking.tmp.hacks.resetting_ages", "global", "tracking");
	}
}

} //end namespace alc

