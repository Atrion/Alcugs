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
#define __U_GAMESUBSYS_ID "$Id: psauthmsg_internal.cpp,v 1.1 2004/10/24 11:20:27 almlys Exp $"
const char * _game_driver_ver="1.1.1a";

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

#include "ageparser.h"
//#include "sdlparser.h" //SDL byte code parser

#include "gamesubsys.h"

#include "debug.h"

int game_initialitzed=0;

//Age descriptor
t_age_def * global_age_def=NULL; //age struct
int global_age_def_n=0; //total number of ages

//full sdl headers only
//t_sdl_def * ghsdl=NULL; //sdl Headers struct
int ghsdl_n=0; //number of global sdl records

st_log * f_sdl=NULL;

#if 0
	print2log(f_uru,"Parsing SDL files...\n");
	if(read_sdl_files((char *)global_sdl_folder,&global_sdl,&global_sdl_n)<0) {
		print2log(f_err,"FATAL, failed to parse the sdl files...\n");
		return -1;
	}
#endif

int init_game_subsys(st_unet * net) {
	int ret=0;
	if(game_initialitzed) return 0;
	game_initialitzed=1;

	DBG(5,"Initialiting game subsystem...\n");

	if(cnf_getByte(1,"sdl.log","global",global_config)==1) {
		f_sdl=open_log("sdl.log",2,DF_STDOUT);
	}

	print2log(f_sdl,"Parsing AGE %s descriptor...\n",net->name);

	//1st destroy if it already exists
	if(global_age_def_n!=0) {
		global_age_def_n=0;
		destroy_age_def(global_age_def);
		if(global_age_def!=NULL) {
			free((void *)global_age_def);
			global_age_def=NULL;
		}
	}

	char * aux;
	aux = (char *)cnf_getString("./age/","age","global",global_config);

	char desc[200];
	strcpy(desc,aux);
	strcat(desc,"/");
	strcat(desc,net->name);
	strcat(desc,".age");

	global_age_def_n=1;
	ret=read_age_descriptor(f_sdl,desc,&global_age_def);
	if(ret<0) {
		print2log(f_sdl,"FATAL, failed to read AGE descriptors from %s...\n",desc);
		ret=-1;
		stop_game_subsys();
		return ret;
	}
	dump_age_descriptor(f_sdl,*global_age_def);

	plog(f_sdl,"INF: Game subsystem v %s started\n",_game_driver_ver);
	logflush(f_sdl);

	return ret;
}

void stop_game_subsys() {
	if(game_initialitzed!=1) return;
	plog(f_sdl,"INF: Game subsystem stopped\n");

	//destroy the age descriptor
	if(global_age_def_n!=0) {
		global_age_def_n=0;
		destroy_age_def(global_age_def);
		if(global_age_def!=NULL) {
			free((void *)global_age_def);
			global_age_def=NULL;
		}
	}

	close_log(f_sdl);
	f_sdl=NULL;
	game_initialitzed=0;
}

//Needs to be done on SIGHUP to be able to apply any changes performed in the config files
void reload_game_subsys(st_unet * net) {
	plog(f_sdl,"INF: Reloading Game subsystem...\n");
	stop_game_subsys();
	init_game_subsys(net);
}

void update_game_subsys() {
	//
}

