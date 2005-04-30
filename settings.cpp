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

#define __U_SETTINGS_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <string.h>
#include <stdlib.h>

#include "config_parser.h"
#include "prot.h"

#include "settings.h"

#include "debug.h"

/*------------------------------------------
   Global configuration variables
-------------------------------------------*/

//It's really, really important to keep always backwards compatibility, the deletion of the bind and port configuration directives caused a lot of damage, at the same time that the installer has been broken due to this change. Please think before doing changes that could have consequences, old vars are still allowed, and will be deleted in the future, but never too quick.

/* The rules in the new configuration schema are simple:

	1) Variables out of context are put inside the [global] section
	2) read_file (included files), will always get out of the context
	3) The same variables can be present in the global, and per-server sections.
	4) The per-server vars will always override the global vars.

*/

/* Also we always want to send hostnames to the netcore, the netcore will handle the conversion, it's more nicer to see the hostnames in the netcore logs than plain ip addresses */

/* Override Rules:
	The last version of the servers, will fetch the different values from the [global] section.
	Some global values, like the port are overrided now by the command line arguments, but
	local values for the per server configuration will override the global values.
*/

#if 0
"afk_timeout"

/* quick note about access levels
 0 -> FULL access. Gamemasters, admins, vault managers.
 1-2 -> *unused*
 3 -> Basic Admin access. GameMasters, write permissions
 4 -> *unused*
 5 -> Debug, read access level, status access level
 6 -> *unused*
 7 -> CCR level / Gamemasters
 8-9 -> *unused*
 10 -> Moderator/Counsuleur level (they can ban, silence..., etc...)
 11-14 -> *unused*
 15 -> Player Level
 16-24 -> *unused*
 25 -> Account Not Activated (Dissabled)
 26-29 -> *unused*
 30 -> Banned
	//for a open public shard use 15
	//for a private shard, you can put 25. [Account Not activated]
	//If you are crazy, really crazy, use 0 [Everybody will have admin privs.]
	//If you hate everybody, you can put 30. [Banned]
*/
/*
	<-1 -> reserved
	-1 -> Undefined
	0 -> custom
	1 -> prime '10-02-2003 Branch - Built 10/9/2003 at 6:53 A'
	2 -> prime12 '10-24-2003 Branch - Built 11/12/2003 at 1:12 PM'
	3 -> live/UU 'Build37 Branch - Built 7/30/2004 at 2:38 PM'
	4 -> todni 'Exp1 Branch - Built 3/9/2004 at 11:39 AM'
	5 -> tpots 'Main Branch - Built 5/26/2004 at 5:19 PM'
	6 -> stable 'Alcugs stable client patch distro for tpots'
	7 -> testing 'Alcugs testing client patch distro for tpots'
	8 and above -> '3rd party client patch distros' (each group working on a client distro MUST use an unique DataSet id)
*/

#endif

/**
	Parse the configuration file
*/
int get_config(FILE * dsc, char * path, st_config ** cfg2,char flags) {

	int ret;
	//char n_servers=8;
	char * server[8]={"auth","tracking","vault","lobby","game","meta","data","admin"};
	                 // 0        1        2       3       4      5       6      7
	int srv=4;

	U16 port=0;
	char * logs=NULL;
	char * logss=NULL;

	if(flags==0x00) { //don't override directives, save some directives
		if(cnf_exists("port","global",*cfg2)==1) {
			port=cnf_getU16(0,"port","global",*cfg2);
		}
		if(cnf_exists("log_files_path","global",*cfg2)==1) {
			logs=(char *)cnf_getString("log/","log_files_path","global",*cfg2);
			logss=(char *)malloc((strlen((const char *)logs)+1)*sizeof(char));
			strcpy(logss,logs);
		}
	}

	ret=read_config(dsc,path,cfg2);

	if(flags==0x00) {
		if(port!=0) {
			cnf_setU16(port,"port","global",cfg2);
		}
		if(logss!=NULL) {
			cnf_add_key(logss,"log_files_path","global",cfg2);
			free((void *)logss);
		}
	}

	if(ret<=-3) return ret;

	//1st set defaults

	//get per server directives
	Byte whoami=0;
	whoami=cnf_getByte(KGame,"whoami","global",*cfg2);

	switch(whoami) {
		case KAuth:
			srv=0;
			break;
		case KVault:
			srv=2;
			break;
		case KTracking:
			srv=1;
			break;
		case KLobby:
			srv=3;
			break;
		case KGame:
			srv=4;
			break;
		case KMeta:
			srv=5;
			break;
		case KData:
			srv=6;
			break;
		case KAdmin:
			srv=7;
			break;
		default:
			srv=4;
			break;
	}
	//override globals by the correct server section
	cnf_copy("global",server[srv],cfg2);

	//set the silent var
	Byte silent;
	switch (cnf_getByte(3,"verbose_level","global",*cfg2)) {
		case 0:
			silent=3;
			break;
		case 1:
			silent=2;
			break;
		case 2:
			silent=1;
			break;
		default:
			silent=0;
	}
	cnf_setByte(silent,"d.silent","global",cfg2);

	//set some important vars
	cnf_copy_key("auth","bind","global","auth",cfg2);
	cnf_copy_key("vault","bind","global","vault",cfg2);
	cnf_copy_key("tracking","bind","global","tracking",cfg2);
	cnf_copy_key("auth.port","port","global","auth",cfg2);
	cnf_copy_key("vault.port","port","global","vault",cfg2);
	cnf_copy_key("tracking.port","port","global","tracking",cfg2);

	cnf_copy_key("auth","auth_server","global","global",cfg2);
	cnf_copy_key("vault","vault_server","global","global",cfg2);
	cnf_copy_key("tracking","tracking_server","global","global",cfg2);
	cnf_copy_key("auth.port","auth_server_port","global","global",cfg2);
	cnf_copy_key("vault.port","vault_server_port","global","global",cfg2);
	cnf_copy_key("tracking.port","tracking_server_port","global","global",cfg2);

	//check some things
	if(cnf_exists("allow_unknown_accounts","global",*cfg2) && \
		cnf_getByte(0,"allow_unknown_accounts","global",*cfg2)==1) {
		cnf_setByte(AcNotActivated,"default_access_level","global",cfg2);
	}

	cnf_copy_key("auth.reg","auto_register_account","global","global",cfg2);
	cnf_copy_key("auth.reg.db","auto_register_account","global","global",cfg2);

	if(cnf_exists("default_access_level","global",*cfg2) && \
		cnf_getU32(AcPlayer,"default_access_level","global",*cfg2)>=AcNotRes) {
		cnf_setByte(AcNotRes,"default_access_level","global",cfg2);
	}

	//database
	cnf_copy_key("db.host","db_server","global","global",cfg2);
	cnf_copy_key("db.name","db_name","global","global",cfg2);
	cnf_copy_key("db.username","db_username","global","global",cfg2);
	cnf_copy_key("db.passwd","db_passwd","global","global",cfg2);
	cnf_copy_key("db.port","db_port","global","global",cfg2);

	//unet
	cnf_copy_key("net.timeout","connection_timeout","global","global",cfg2);
	cnf_copy_key("net.maxconnections","max_clients","global","global",cfg2);
	cnf_copy_key("max_population","max_players","global","global",cfg2);

	if(cnf_exists("bandwidth","global",*cfg2)) {
		cnf_copy_key("net.up","bandwidth","global","global",cfg2);
		cnf_copy_key("net.down","bandwidth","global","global",cfg2);
	}

	if(cnf_exists("dissabled","global",*cfg2)) {
		cnf_copy_key("stop","dissabled","global","global",cfg2);
	}

	//shard public info
	cnf_copy_key("shard.name","shard_name","global","global",cfg2);
	cnf_copy_key("shard.website","shard_website","global","global",cfg2);
	cnf_copy_key("shard.desc","shard_description","global","global",cfg2);
	cnf_copy_key("shard.passwd","shard_password","global","global",cfg2);
	cnf_copy_key("shard.contact","shard_contact","global","global",cfg2);

	if(!cnf_exists("shard.name","global",*cfg2)) {
		cnf_add_key("Unnamed Alcugs shard","shard.name","global",cfg2);
	}
	if(!cnf_exists("shard.website","global",*cfg2)) {
		cnf_add_key("http://huru.almlys.dyns.net/unconfigured.php",\
		"shard.website","global",cfg2);
	}
	if(!cnf_exists("shard.desc","global",*cfg2)) {
		cnf_add_key("Generic unconfigured Alcugs shard",\
		"shard.desc","global",cfg2);
	}

	cnf_copy_key("meta","enable_metaserver","global","global",cfg2);

	cnf_copy_key("vault.hood.name","neighborhood_name","global","global",cfg2);
	cnf_copy_key("vault.hood.desc","neighborhood_comment","global","global",cfg2);

	if(!cnf_exists("dataset","global",*cfg2)) {
		cnf_add_key("-1","dataset","global",cfg2);
	}

	switch((S32)cnf_getU32((U32)-1,"dataset","global",*cfg2)) {
		case -1:
			cnf_add_key("undef","dataset.tag","global",cfg2);
			break;
		case 0:
			cnf_add_key("custom","dataset.tag","global",cfg2);
			break;
		case 1:
			cnf_add_key("prime","dataset.tag","global",cfg2);
			break;
		case 2:
			cnf_add_key("prime12","dataset.tag","global",cfg2);
			break;
		case 3:
			cnf_add_key("live","dataset.tag","global",cfg2);
			break;
		case 4:
			cnf_add_key("todni","dataset.tag","global",cfg2);
			break;
		case 5:
			cnf_add_key("tpots","dataset.tag","global",cfg2);
			break;
		case 6:
			cnf_add_key("stable","dataset.tag","global",cfg2);
			break;
		case 7:
			cnf_add_key("testing","dataset.tag","global",cfg2);
			break;
		case 8:
			cnf_add_key("hacking","dataset.tag","global",cfg2);
			break;
		default:
			cnf_copy_key("dataset.tag","dataset","global","global",cfg2);
			break;
	}

	return ret;
}

