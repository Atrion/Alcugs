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
#define __U_VAULTSUBSYS_ID "$Id$"
const char * _vault_driver_ver="1.1.1e";

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

#include "vaultsubsys.h"

#include "debug.h"


int vault_initialitzed=0;

st_log * f_vmgr=NULL;
st_log * f_vhtml=NULL;

char * system_addr=NULL;
U16 system_port=0;

//t_age_def * global_age_def=NULL; //age struct
//int global_age_def_n=0; //total number of ages

int init_vault_subsys() {
	int ret=0;
	if(vault_initialitzed) return 0;
	vault_initialitzed=1;

	DBG(5,"Initialiting vault subsystem...\n");

	char * aux=NULL;

	/*
	aux=cnf_getString("./","log_files_path","global",global_config);
	log_data_path=(char *)malloc(sizeof(char) * (strlen(aux)+1));
	strcpy(log_data_path,aux);
	*/

	aux=cnf_getString("localhost","bind","global",global_config);
	system_addr=(char *)malloc(sizeof(char) * (strlen(aux)+1));
	strcpy(system_addr,aux);

	system_port=cnf_getU16(5000,"port","global",global_config);

	if(cnf_getByte(1,"vault.log","global",global_config)==1) {
		f_vmgr=open_log("vmgr.log",2,DF_STDOUT);
	}
	if(cnf_getByte(1,"vault.html.log","global",global_config)==1) {
		f_vhtml=open_log("vmgr.html",2,DF_HTML);
	}

	/////
	/*
	print2log(f_uru,"Parsing AGE descriptors...\n");

	global_age_def_n=read_all_age_descriptors(f_vmgr,(char *)"./age/",&global_age_def);
	if(global_age_def_n<=0) {
		print2log(f_err,"FATAL, failed to read AGE descriptors from %s...\n","./age/");
		return -1;
	}
	*/
	////

	plog(f_vmgr,"INF: Vault subsystem v %s started\n",_vault_driver_ver);
	plog(f_vmgr,"Log Path: %s\n",stdebug_config->path);
	logflush(f_vmgr);

	return ret;
}

void stop_vault_subsys() {
	if(vault_initialitzed!=1) return;
	plog(f_vmgr,"INF: Vault subsystem stopped\n");
	if(system_addr!=NULL) free((void *)system_addr);
	vault_initialitzed=0;
	close_log(f_vmgr);
	close_log(f_vhtml);
	f_vmgr=NULL;
	f_vhtml=NULL;
}

//Needs to be done on SIGHUP to be able to apply any changes performed in the config files
void reload_vault_subsys() {
	plog(f_vmgr,"INF: Reloading vault subsystem...\n");
	stop_vault_subsys();
	init_vault_subsys();
}


