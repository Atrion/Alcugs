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

#ifndef __U_SETTINGS_H_
#define __U_SETTINGS_H_
#define __U_SETTINGS_H_ID "$Id: tmp_config.h,v 1.2 2004/12/02 22:31:46 almlys Exp $"

#if 0

#include <openssl/md5.h> //for MD5

#include "data_types.h" //for Byte, and others
#include "conv_funs.h" //for conversion functions
#include "vaultstrs.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

extern Byte * global_config_alt; //pointer to the alternate config file
extern Byte global_sdl_folder[512]; //where are the sdl files?
extern Byte global_age_folder[512]; //where are the age files?

extern Byte global_max_version;
extern Byte global_min_version;

extern int global_max_players_per_account;
extern Byte global_allow_multiple_login;

extern Byte global_url[300];
//extern Byte global_private_ip_address[30]; *unused*
extern Byte global_public_ip_address[100];
extern Byte global_private_network_mask[30];
extern Byte global_daemon_mode; //daemon mode?
extern Byte global_bind_hostname[100];
extern int global_port;
extern int global_connection_timeout;
extern int global_afk_timeout;
extern int global_auto_shutdown;
extern U32 global_bandwidth;
//mysql database access <-> that will be changed to vault server access credentials
extern Byte global_db_server[100]; //hostname/IP address of the vault server
extern U16 global_db_port; //default port, or unix socket
extern Byte global_db_username[100];
extern Byte global_db_passwd[100];
extern Byte global_db_name[100];
//limits
extern int global_the_bar;
extern int global_client_count;
extern int global_max_clients;
extern int global_bypass_access_level;
//age
extern Byte global_age_filename[100];
extern Byte global_age_guid[0x20];
extern st_uru_age global_age;
//bcast
extern int global_broadcast; //enable broadcast?
//vault
extern char global_vault_folder_name[20];
//bd
extern U32 global_bandwidth;
//logs
extern int silent;
extern Byte global_verbose_level;
extern Byte global_log_files_path[512];
extern Byte global_log_timestamp;
extern Byte global_log_ip; //0 log ip globaly disabled/1 enabled

extern Byte global_auth_hostname[100];
extern Byte global_vault_hostname[100];
extern Byte global_tracking_hostname[100];

extern U16 global_auth_port;
extern U16 global_vault_port;
extern U16 global_tracking_port;

extern U16 global_min_port;
extern U16 global_max_port;
extern Byte global_fork;

extern Byte global_neighborhood_comment[255];
extern Byte global_neighborhood_name[255];
extern Byte global_welcome_title[STR_MAX_SIZE+1];
extern Byte global_welcome_text[1000];

extern Byte global_default_access_level;

extern Byte global_metaserver_address[100];
extern U16 global_metaserver_port;

extern Byte global_enable_metaserver;

extern Byte global_shard_name[100];
extern Byte global_shard_website[100];
extern Byte global_shard_description[255];
extern Byte global_shard_passwd[100];
extern Byte global_shard_contact[100];

extern int global_dataset;
extern Byte global_dataset_tag[50];

extern Byte global_private_ages[1024];
extern Byte global_muinstance_mode;

extern Byte global_binary_path[255];

extern Byte global_auto_register_account;
extern Byte global_allow_unknown_accounts;

#endif

/**
	Parse the configuration file
	Returns 2 on fatal error
	Returns 1 on non-fatal error
	Retruns 0 if success
	flags 0x00 don't override, 0x01 override some params
*/
int get_config(FILE * dsc, char * path, st_config ** cfg2,char flags);


#endif
