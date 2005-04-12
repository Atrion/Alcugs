/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004  The Alcugs H'uru Server Team                          *
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

//This is the vault subsystem required to parse those vault messages

/* CVS tag - DON'T TOUCH*/
#define __U_LOBBYSUBSYS_ID "$Id$"
const char * _lobby_driver_ver="1.1.1a";

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "data_types.h"
#include "stdebug.h"
#include "prot.h"
#include "protocol.h"

#include "conv_funs.h"

#include "config_parser.h"
#include "uru.h"

//#include "ageparser.h"

#include "lobbysubsys.h"

#include "debug.h"

int lobby_initialitzed=0;

Byte load_on_demand=1;
char globby_game_log_path[500];
char globby_bin[200]; //the binary path
char globby_game_config[200]; //the config file


int init_lobby_subsys() {
	int ret=0;
	if(lobby_initialitzed) return 0;
	lobby_initialitzed=1;

	DBG(5,"Initialiting lobby subsystem...\n");

	char * aux=NULL;

	load_on_demand=cnf_getByte(load_on_demand,"load_on_demand","global",global_config);

	//set game server logging path
	aux=cnf_getString(stdebug_config->path,"game.log","global",global_config);
	strcpy(globby_game_log_path,aux);

	//the game config file
	aux=cnf_getString("uru.conf","read_config","global",global_config);
	aux=cnf_getString(aux,"game.config","global",global_config);
	strcpy(globby_game_config,aux);

#ifdef __WIN32__
	aux=cnf_getString("","bin","global",global_config);
	strcpy(globby_bin,aux);
#else
	aux=cnf_getString("./","bin","global",global_config);
	strcpy(globby_bin,aux);
	if((char)globby_bin[strlen((const char *)globby_bin)-1]!=(char)'/') {
		strcat(globby_bin,"/");
	}
#endif

	plog(f_uru,"INF: Lobby subsystem v %s started\n",_lobby_driver_ver);
	plog(f_uru,"Log Path: %s\n",stdebug_config->path);
	plog(f_uru,"Game Log Path: %s\n",globby_game_log_path);
	plog(f_uru,"Game config file: %s\n",globby_game_config);
	logflush(f_uru);

	return ret;
}

void stop_lobby_subsys() {
	if(lobby_initialitzed!=1) return;
	plog(f_uru,"INF: Lobby subsystem stopped\n");
	//if(system_addr!=NULL) free((void *)system_addr);
	lobby_initialitzed=0;
}

//Needs to be done on SIGHUP to be able to apply any changes performed in the config files
void reload_lobby_subsys() {
	plog(f_uru,"INF: Reloading lobby subsystem...\n");
	stop_lobby_subsys();
	init_lobby_subsys();
}


