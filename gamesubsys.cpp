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
#define __U_GAMESUBSYS_ID "$Id$"
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

#include "gamesubsys.h"

#include "debug.h"

int game_initialitzed=0;

//Age descriptor
t_age_def * global_age_def=NULL; //age struct
int global_age_def_n=0; //total number of ages

#ifdef TEST_SDL
t_sdl_def * global_sdl_def=NULL;
int global_sdl_def_n=0;

t_sdl_head * global_sdl_bin=NULL; //!TODO: sdl
#endif

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


	/////////////////////////////////////////////////////////
	//age stuff

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


#ifdef TEST_SDL
	/////////////////////////////////////////////////////////
	//SDL stuff

	//1st destroy if it already exists
	if(global_sdl_def_n!=0) {
		destroy_sdl_def(global_sdl_def,global_sdl_def_n);
		global_sdl_def_n=0;
		if(global_sdl_def!=NULL) {
			free((void *)global_sdl_def);
			global_sdl_def=NULL;
		}
	}

	aux = (char *)cnf_getString("./sdl/","sdl","global",global_config);


	ret=read_sdl_files(f_sdl,aux,&global_sdl_def,&global_sdl_def_n);
	if(ret<0) {
		print2log(f_sdl,"FATAL, failed to read SDL descriptors...\n");
		ret=-1;
		stop_game_subsys();
		return ret;
	}

	int sdl_id=find_sdl_descriptor_by_name((Byte *)&net->name,global_sdl_def,global_sdl_def_n);

	if(sdl_id>=0)
	{
		global_sdl_bin=(t_sdl_head *)malloc(sizeof(t_sdl_head));

		strcpy((char *)&global_sdl_bin->name,net->name);
		global_sdl_bin->version=global_sdl_def[sdl_id].version;
		global_sdl_bin->object_present=0;
		//global_sdl_bin->o=0;

		sdl_fill_t_sdl_binary_by_sdl_id(&global_sdl_bin->bin,global_sdl_def,global_sdl_def_n,sdl_id);
	}
	else
	{
		global_sdl_bin=0;
	}
#endif //TEST_SDL

	plog(f_sdl,"INF: Game subsystem v %s started\n",_game_driver_ver);
	logflush(f_sdl);

	return ret;
}

void stop_game_subsys() {
	if(game_initialitzed!=1) return;
	plog(f_sdl,"INF: Game subsystem stopped\n");

#ifdef TEST_SDL

	if(global_sdl_bin!=0)
	{
		int sdl_id=find_sdl_descriptor((Byte *)&global_sdl_bin->name,global_sdl_bin->version,global_sdl_def,global_sdl_def_n);

		if(sdl_id>=0)
			sdl_free_t_sdl_binary(&global_sdl_bin->bin,global_sdl_def,global_sdl_def_n,sdl_id);
		free(global_sdl_bin);
	}

	//destroy the sdl descriptors
	if(global_sdl_def_n!=0) {
		destroy_sdl_def(global_sdl_def,global_sdl_def_n);
		global_sdl_def_n=0;
		if(global_sdl_def!=NULL) {
			free((void *)global_sdl_def);
			global_sdl_def=NULL;
		}
	}
#endif //TEST_SDL

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

