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
#define __U_VAULTSERVERSYS_ID "$Id$"
const char * _vault_server_driver_ver="1.1.1d";

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

#include "sql.h"

#include "config_parser.h"
#include "uru.h"

#include "vault_db.h"

#include "vaultsubsys.h"
#include "vserversys.h"

#include "debug.h"


int vault_server_initialitzed=0;
Byte max_players_per_account=4;

st_sql db;

char global_vault_folder_name[20];

char * global_private_ages=NULL;
int global_muinstance_mode=1;

t_age_def * global_age_def=NULL; //age struct
int global_age_def_n=0; //total number of ages

char * ghood_name=NULL;
char * ghood_desc=NULL;

char * gvinit_title=NULL;
char * gvinit_desc=NULL;

char * kgvinit_title="Shorah b'shehmtee";
char * kgvinit_desc="Shorah b'shehmtee, this Shard is Running the Alcugs server software.\nThanks for your support!\n\nWelcome to the new adventure, feel free to explore Er'cana or any other age. Be careful if you see new books, some explorers have found some Kortee'nea and other ancient technology in a secret room in Kirel DRC neighborhood, and they are starting to learn the art of writting.\n";

//parse age files
int vsys_parse_age_files() {
	print2log(f_vmgr,"Parsing AGE descriptors...\n");

	char * where;
	where = (char *)cnf_getString("./age/","age","global",global_config);

	//1st destroy if them already exist
	int i;
	for(i=0; i<global_age_def_n; i++) {
		destroy_age_def(&global_age_def[i]);
	}
	if(global_age_def!=NULL) {
		free((void *)global_age_def);
		global_age_def=NULL;
	}
	global_age_def=0;

	global_age_def_n=read_all_age_descriptors(f_vmgr,(char *)where,&global_age_def);
	if(global_age_def_n<=0) {
		print2log(f_vmgr,"FATAL, failed to read AGE descriptors from %s...\n",where);
		return -1;
	}
	return 0;
}

int init_vault_server_subsys() {
	int ret=0;
	if(vault_server_initialitzed) return 0;
	if(!vault_initialitzed) {
		plog(f_err,"ERR: Cannot start up vault server sub system\n");
		return -1;
	}

	vault_server_initialitzed=1;

	DBG(5,"Initialiting vault server subsystem...\n");

	char * tmp;
	global_muinstance_mode=cnf_getByte(global_muinstance_mode,"instance_mode","global",\
	global_config);
	tmp = (char *)cnf_getString("AvatarCustomization,Personal,Nexus,BahroCave","private_ages","global",global_config);
	global_private_ages=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(global_private_ages,tmp);

	tmp = (char *)cnf_getString("Alcugs","vault.hood.name","global",global_config);
	ghood_name=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(ghood_name,tmp);
	tmp = (char *)cnf_getString("Be careful with the giant Quab!","vault.hood.desc","global",global_config);
	ghood_desc=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(ghood_desc,tmp);

	tmp = (char *)cnf_getString(kgvinit_title,"vault.wipe.msg.title","global",global_config);
	gvinit_title=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(gvinit_title,tmp);
	tmp = (char *)cnf_getString(kgvinit_desc,"vault.wipe.msg","global",global_config);
	gvinit_desc=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(gvinit_desc,tmp);

	sql_init(&db);
	//set here database params
	tmp = (char *)cnf_getString("","db.host","global",global_config);
	db.host=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(db.host,tmp);
	tmp = cnf_getString("","db.username","global",global_config);
	db.username=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(db.username,tmp);
	tmp = cnf_getString("","db.passwd","global",global_config);
	db.passwd=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(db.passwd,tmp);
	tmp = cnf_getString("","db.name","global",global_config);
	db.name=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(db.name,tmp);
	db.port= cnf_getU16(0,"db.port","global",global_config);

	//set more params
	if(cnf_getByte(1,"db.log","global",global_config)==0) {
		db.flags &= ~SQL_LOG; //dissable logging
	}
	if(cnf_getByte(1,"db.sql.log","global",global_config)==0) {
		db.flags &= ~SQL_LOGQ; //dissable logging sql statements
	}
	if(cnf_getByte(1,"db.persistent","global",global_config)==0) {
		db.flags &= ~SQL_STAYCONN; //dissable Always stay connected
	}
	db.timeout=cnf_getU32(db.timeout,"db.timeout","global",global_config);

	//vault settings
	max_players_per_account=cnf_getU32(max_players_per_account,"vault.maxplayers",\
	"global",global_config);

	//start up the sql driver
	sql_start(&db);

	st_log * verr=f_vmgr;
	st_log * vstd=f_vmgr;
	if(f_vmgr==NULL) {
		verr=f_err;
		vstd=f_uru;
	}

	if(vsys_parse_age_files()!=0) {
		ret=-1;
		plog(verr,"ERR: Error parsing the age files!\n");
	}

	int ver;
	ver=plVaultGetVersion(&db);

	if(ret==0 && ver<vault_version && ver>=0) {
		plog(vstd,"INF: Your vault format is outdated, I'm going to try to migrate it to the new format.\n");
		if(plVaultMigrate(ver,&db)<0) {
			plog(verr,"WAR: Fatal, vault migration from version %i to version %i failed!!\n",ver,vault_version);
			ret=-1;
		}
	} //else //ok, error, non-existent, failed, etc..

	char * fold;
	fold=plVaultGetFolder(&db);

	if(fold==NULL || ret!=0) {
		print2log(verr,"FATAL, cannot negotiate the folder name, the mysql database may be down, or there is a corruption in the vault database, check your configuration settings!\n");
		ret=-1;
		stop_vault_server_subsys();
	} else {
		strcpy(global_vault_folder_name,fold);
	}

	return ret;
}

void stop_vault_server_subsys() {
	if(vault_server_initialitzed!=1) return;
	//This will work, only if you always call init_auth_driver after this ...
	if(db.host!=NULL) free((void *)db.host);
	if(db.username!=NULL) free((void *)db.username);
	if(db.passwd!=NULL) free((void *)db.passwd);
	if(db.name!=NULL) free((void *)db.name);
	if(global_private_ages!=NULL) free((void *)global_private_ages);
	if(gvinit_desc!=NULL) free((void *)gvinit_desc);
	if(gvinit_title!=NULL) free((void *)gvinit_title);
	if(ghood_name!=NULL) free((void *)ghood_name);
	if(ghood_desc!=NULL) free((void *)ghood_desc);
	db.host=NULL;
	db.username=NULL;
	db.passwd=NULL;
	db.name=NULL;
	global_private_ages=NULL;
	ghood_name=NULL;
	ghood_desc=NULL;
	gvinit_desc=NULL;
	gvinit_title=NULL;

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	int i;
	for(i=0; i<global_age_def_n; i++) {
		DBG(5,"destroying age %i of %i\n",i,global_age_def_n);
		DBG(5,"dmalloc_verify()\n");
		dmalloc_verify(NULL);
		destroy_age_def(&global_age_def[i]);
	}
	if(global_age_def!=NULL) {
		free((void *)global_age_def);
		global_age_def=NULL;
	}
	global_age_def=0;

	plog(f_vmgr,"INF: Vault server subsystem stopped\n");
	vault_server_initialitzed=0;
	sql_shutdown(&db);
}

//Needs to be done on SIGHUP to be able to apply any changes performed in the config files
void reload_vault_server_subsys() {
	plog(f_vmgr,"INF: Reloading vault server subsystem...\n");
	stop_vault_server_subsys();
	init_vault_server_subsys();
}
//Note, the vault subsystem can only be restarted through the admin alcugs console

//note, reload will kill all connected MGRS, we should use this one instead
void update_vault_server_subsys() {
	plog(f_vmgr,"INF: Updating vault server subsystem...\n");
	vsys_parse_age_files();
}

void vault_server_subsys_idle_operation() {
	// Do idle tasks here
	sql_idle(&db); //important
}
