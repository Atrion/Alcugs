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

/* This file is going to remain as a temporany file until the new netcore is
   prepared for public usage */

/**********************************************
  For parsing the configuration URU files
*************************************************/

#ifndef __U_TMP_CONFIG_
#define __U_TMP_CONFIG_
#define __U_TMP_CONFIG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "config_parser.h"
#include "tmp_config.h"

#include "uru.h"
#include "prot.h"

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

#define _CNF_VARS 46

//internal only
int n_cnf_vars=_CNF_VARS;
char *cnf_vars[_CNF_VARS]={\
"bind",\
"port",\
"verbose_level",\
"daemon",\
"log_files_path",\
"connection_timeout",\
"afk_timeout",\
"db_server",\
"db_name",\
"db_username",\
"db_passwd",\
"db_port",\
"max_players",\
"bandwidth",\
"age_filename",\
"age_guid",\
"broadcast",\
"public_address",\
"private_mask",\
"auth_server",\
"vault_server",\
"tracking_server",\
"website",\
"auth_server_port",\
"vault_server_port",\
"tracking_server_port",\
"load_on_demand",\
"sdl",\
"age",\
"neighborhood_name",\
"neighborhood_comment",\
"welcome_title",\
"welcome_text",\
"shard_name",\
"shard_website",\
"shard_description",\
"shard_password",\
"shard_contact",\
"dataset",\
"enable_metaserver",\
"default_access_level",\
"private_ages",\
"instance_mode",\
"bin",\
"allow_unknown_accounts",\
"auto_register_account"\
};

//internal only
Byte * global_config_alt=NULL; //pointer to the alternate config file

//public
Byte global_sdl_folder[512]="sdl/"; //where are the sdl files?
Byte global_age_folder[512]="age/"; //where are the age files?

int silent=0; //be silent yes/no (print or no uru_log messages to the stdout)
				// 0-> print all (I say, ALL!!)
				// 1-> print only uru.log messages and error.log messages
				// 2-> silence only the stdout
				// 3-> total silence, don't say nothing, be totally quiet

Byte global_url[300]= "http://huru.almlys.dyns.net/unconfigured.php";

Byte global_max_version=12;
Byte global_min_version=6;

int global_max_players_per_account=4;

Byte global_allow_multiple_login=1;

//external - public
Byte global_public_ip_address[100] = "127.0.0.1"; //hostnames are also allowed
Byte global_private_network_mask[30] = "255.255.255.0";

Byte global_daemon_mode = 0; //daemon mode?

Byte global_verbose_level = 3; //verbose level
Byte global_log_timestamp = 1; //0 timestamps globaly disabled/1 enabled
Byte global_log_ip = 1; //0 log ip globaly disabled/1 enabled

Byte global_bind_hostname[100] = "0.0.0.0";
int global_port = 5000;

Byte global_log_files_path[512] = "log/"; //512 is considerable, I will change it for a pointer and a malloc in a future release to allow biggest paths. :). But WHO has a 256 sized path?.

int global_connection_timeout=5; //Disconnect client after X minutes, if we don't know nothing about it.
int global_afk_timeout=15; //DISCONNECT player session, if he/she is more than X minutes inactive.
int global_auto_shutdown=30; //Automatic emergency shutdown countdown, invoqued by the system

/*server bandwidth in bps.
examples:
 0x40000000 -> 1Gbps  (Run Uru here, and you will fly!!)
 0x06400000 -> 100Mbps
 0x05000000 -> 10Mbps
 0x00120000 -> 1.1Mbps
 0x00080000 -> 512Kbps
 0x00050000 -> 320Kbps
 0x00040000 -> 256Kbps
 0x00030000 -> 192Kbps
 0x00020000 -> 128Kbps
 0x00010000 -> 64Kbps
*/
U32 global_bandwidth=0x06400000;

//mysql database access <-> that will be changed to vault server access credentials
Byte global_db_server[100]; //hostname/IP address of the vault server
U16 global_db_port = 0; //default port, or unix socket
Byte global_db_username[100] = "vault"; //vault username
Byte global_db_passwd[100] = ""; //vault passwd
Byte global_db_name[100] = "vault"; //database name

//limits
//int global_max_players = 20; //maxium number of players/connections allowed
int global_the_bar=0;
int global_client_count=1;
int global_max_clients=200; //there is known issue that will be fixed. :/
//int global_reserved_admin_slots = 5; //admin reserved slots
int global_bypass_access_level = 7; //this level

Byte global_age_filename[100] = "AvatarCustomization";
Byte global_age_guid[0x20] = {0,1,0,0,0,0,0,0};

st_uru_age global_age;

int global_broadcast = 1; //enable broadcast?

Byte global_auth_hostname[100]="127.0.0.1";
Byte global_vault_hostname[100]="127.0.0.1";
Byte global_tracking_hostname[100]="127.0.0.1";

U16 global_auth_port= 2010;
U16 global_vault_port = 2012;
U16 global_tracking_port = 2011;

//define the port range that will be used (the first one must be the same as the lobby)
U16 global_min_port = 5000;
U16 global_max_port = 6000;

Byte global_fork = 1; //Automatically fork game servers on demand?

Byte global_default_access_level = 15;
	//for a open public shard use 15
	//for a private shard, you can put 25. [Account Not activated]
	//If you are crazy, really crazy, use 0 [Everybody will have admin privs.]
	//If you hate everybody, you can put 30. [Banned]

	//WARNING, values bigger than 30 can cause lot's of problems.

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

*/

//Metaserver
// Please note that *everybody* should be using the same metaserver
// also this is temporany, If I get too many traffic and nobody offers an alternate
// host to permanently run the metaserver 24 hours/365 days, then I will write a
// http gateway and it will be hosted at sourceforge.
Byte global_metaserver_address[100]="metaserver.uru3.almlys.dyns.net";
//Byte global_metaserver_address[100]="metaserver.uru2.almlys.dyns.net";
U16 global_metaserver_port=8000;

Byte global_enable_metaserver=0; //1-connect to metaserver, 0-private shard

//params
Byte global_shard_name[100]="Unnamed H'uru shard";
Byte global_shard_website[100]="http://huru.almlys.dyns.net/unconfigured.php";
Byte global_shard_description[255]="Generic unconfigured H'uru shard";
Byte global_shard_passwd[100]="123";
Byte global_shard_contact[100]="mailto:nowhere AT mysuperserver DOT tld";

/*
 Note: The difference between shard_website and website, is that you can set different
 sites one for the client launcher, and the other one for the metaserver
*/

//Data set
int global_dataset=-1; //undefined
//allowed values for dataset
/*
	<-1 -> reserved
	-1 -> Undefined
	0 -> custom
	1 -> prime '10-02-2003 Branch - Built 10/9/2003 at 6:53 A'
	2 -> prime12 '10-24-2003 Branch - Built 11/12/2003 at 1:12 PM'
	3 -> live/UU 'Build37 Branch - Built 7/30/2004 at 2:38 PM'
	4 -> todni 'Exp1 Branch - Built 3/9/2004 at 11:39 AM'
	5 -> tpots 'Main Branch - Built 5/26/2004 at 5:19 PM'
	6 -> tpots2 'Alcugs H'uru official client patch distro for tpots'
	7 and above -> '3rd party client patch distros' (each group working on a client distro MUST use an unique DataSet id)

*/
Byte global_dataset_tag[50]="undef";
//Allowed tags: custom, prime, prime12,live,todni,tpots,tpots2.
//Note: live tag is also for UU, since there isn't any difference between both clients.

Byte global_private_ages[1024]="AvatarCustomization,Personal,Nexus";
Byte global_muinstance_mode=1; //0 use the new standard single instance mode
                               //1 enable multiple instance mode
                               //2 use the old hardcoded standard (to avoid doing a wipe to the vault)

//** vault stuff default vars **
//default hood
Byte global_neighborhood_name[255]="H'uru";
Byte global_neighborhood_comment[255]="Be carefull with the giant Quab";
Byte global_welcome_title[STR_MAX_SIZE+1] = "Welcome to the Alcugs H'uru server";
Byte global_welcome_text[1000] = "Congratulations!, this Shard is Running the Alcugs H'uru alternate server software.\nThanks for your support!\n\nWelcome to the new adventure, feel free to explore Er'cana or any other age. Be careful if you see an strange book, since some explorers have found some blank books and other old D'ni technology in a secret room in Kirel DRC neighborhood age, and they are starting to learn the art of writting.\n";
//Default folder if for some strange, rare, unknow, and unexpected reason the
// unique guid generator failed to crete the folder
char global_vault_folder_name[20]="000A000000000000";

Byte global_binary_path[255]="./";

Byte global_auto_register_account=0; //default is to enable auto-registration into accouts table
Byte global_allow_unknown_accounts=1; //default is to enable unknown clients from accounts table

st_config * cfg=NULL;


/*---------------------------------------------------
	Parse the configuration file
----------------------------------------------------*/
int get_config(FILE * dsc, char * path) {

	int ret;
	char * server[6]={"auth","tracking","vault","lobby","game","meta"};
	                 // 0        1        2       3       4      5
	int srv=4;

	ret=read_config(dsc,path,&cfg,cnf_vars,n_cnf_vars);

	if(ret<=-3) return ret;

	//get globals

	//basic - required
	global_daemon_mode=cnf_getByte(global_daemon_mode,"daemon","global",cfg);
	global_verbose_level=cnf_getByte(global_verbose_level,"verbose_level","global",cfg);
	global_port=cnf_getU16(global_port,"port","global",cfg);
	strcpy((char *)global_bind_hostname,\
	(char *)cnf_getString(global_bind_hostname,"bind","global",cfg));
	strcpy((char *)global_log_files_path,\
	(char *)cnf_getString(global_log_files_path,"log_files_path","global",cfg));
	global_connection_timeout=cnf_getU32(global_connection_timeout,\
	"connection_timeout","global",cfg);
	global_afk_timeout=cnf_getU32(global_afk_timeout,"afk_timeout","global",cfg);
	global_max_clients=cnf_getU32(global_max_clients,"max_players","global",cfg);
	global_bandwidth=1024*cnf_getU32(global_bandwidth/1024,"bandwidth","global",cfg);

	DBG(5,"%s - %i\n",global_bind_hostname,global_port);

	//database
	strcpy((char *)global_db_server,\
	(char *)cnf_getString(global_db_server,"db_server","global",cfg));
	strcpy((char *)global_db_name,\
	(char *)cnf_getString(global_db_name,"db_name","global",cfg));
	strcpy((char *)global_db_username,\
	(char *)cnf_getString(global_db_username,"db_username","global",cfg));
	strcpy((char *)global_db_passwd,\
	(char *)cnf_getString(global_db_passwd,"db_passwd","global",cfg));
	global_db_port=cnf_getU16(global_db_port,"db_port","global",cfg);

	//client files
	strcpy((char *)global_sdl_folder,\
	(char *)cnf_getString(global_sdl_folder,"sdl","global",cfg));
	strcpy((char *)global_age_folder,\
	(char *)cnf_getString(global_age_folder,"age","global",cfg));
	global_dataset=cnf_getU32(global_dataset,"dataset","global",cfg);


	//public address & LAN mask
	strcpy((char *)global_public_ip_address,\
	(char *)cnf_getString(global_public_ip_address,"public_address","global",cfg));
	strcpy((char *)global_private_network_mask,\
	(char *)cnf_getString(global_private_network_mask,"private_mask","global",cfg));

	//Shard info
	strcpy((char *)global_url,\
	(char *)cnf_getString(global_url,"website","global",cfg));
	strcpy((char *)global_shard_name,\
	(char *)cnf_getString(global_shard_name,"shard_name","global",cfg));
	strcpy((char *)global_shard_website,\
	(char *)cnf_getString(global_shard_website,"shard_website","global",cfg));
	strcpy((char *)global_shard_description,\
	(char *)cnf_getString(global_shard_description,"shard_description","global",cfg));
	strcpy((char *)global_shard_passwd,\
	(char *)cnf_getString(global_shard_passwd,"shard_password","global",cfg));
	strcpy((char *)global_shard_contact,\
	(char *)cnf_getString(global_shard_contact,"shard_contact","global",cfg));

	//vault initial data
	strcpy((char *)global_neighborhood_name,\
	(char *)cnf_getString(global_neighborhood_name,"neighborhood_name","global",cfg));
	strcpy((char *)global_neighborhood_comment,\
	(char *)cnf_getString(global_neighborhood_comment,"neighborhood_comment",\
	"global",cfg));
	strcpy((char *)global_welcome_title,\
	(char *)cnf_getString(global_welcome_title,"welcome_title","global",cfg));
	strcpy((char *)global_welcome_text,\
	(char *)cnf_getString(global_welcome_text,"welcome_text","global",cfg));

	//for debugging
	global_fork=cnf_getByte(global_fork,"load_on_demand","global",cfg);

	//metaserver
	global_enable_metaserver=cnf_getByte(global_enable_metaserver,"enable_metaserver",\
	"global",cfg);

	//auth
	global_default_access_level=cnf_getByte(global_default_access_level,\
	"default_access_level","global",cfg);


	//depreceated (but we keep them for backwards compatibility)
	strcpy((char *)global_age_filename,\
	(char *)cnf_getString(global_age_filename,"age_filename","global",cfg));
	Byte str_guid[50];
	hex2ascii2(str_guid,global_age_guid,16);
	strcpy((char *)str_guid,\
	(char *)cnf_getString(str_guid,"age_guid","global",cfg));
	ascii2hex2((unsigned char *)global_age_guid,(unsigned char *)str_guid,8);
	global_broadcast=cnf_getByte(global_broadcast,"broadcast","global",cfg);
	strcpy((char *)global_age_filename,\
	(char *)cnf_getString(global_age_filename,"age_filename","global",cfg));
	strcpy((char *)global_auth_hostname,\
	(char *)cnf_getString(global_auth_hostname,"auth_server","global",cfg));
	strcpy((char *)global_vault_hostname,\
	(char *)cnf_getString(global_vault_hostname,"vault_server","global",cfg));
	strcpy((char *)global_tracking_hostname,\
	(char *)cnf_getString(global_tracking_hostname,"tracking_server","global",cfg));
	global_vault_port=cnf_getU16(global_vault_port,"vault_server_port","global",cfg);
	global_auth_port=cnf_getU16(global_auth_port,"auth_server_port","global",cfg);
	global_tracking_port=cnf_getU16(global_tracking_port,"tracking_server_port",\
	"global",cfg);


	global_muinstance_mode=cnf_getByte(global_muinstance_mode,\
	"instance_mode","global",cfg);
	strcpy((char *)global_private_ages,\
	(char *)cnf_getString(global_private_ages,"private_ages","global",cfg));

	global_auto_register_account=cnf_getByte(global_auto_register_account,\
	"auto_register_account","global",cfg);
	global_allow_unknown_accounts=cnf_getByte(global_allow_unknown_accounts,\
	"allow_unknown_accounts","global",cfg);


	//get per server directives

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
		default:
			srv=4;
			break;
	}

	//srv=0; //<- who did that evil thing?

	//set auth, tracking and vault servers
	if(whoami!=KAuth) {
		strcpy((char *)global_auth_hostname,\
		(char *)cnf_getString(global_auth_hostname,"bind","auth",cfg));
		global_auth_port=cnf_getU16(global_auth_port,"port","auth",cfg);
	}

	if(whoami!=KVault) {
		strcpy((char *)global_vault_hostname,\
		(char *)cnf_getString(global_vault_hostname,"bind","vault",cfg));
		global_vault_port=cnf_getU16(global_vault_port,"port","vault",cfg);
	}

	if(whoami!=KTracking) {
		strcpy((char *)global_tracking_hostname,\
		(char *)cnf_getString(global_tracking_hostname,"bind","tracking",cfg));
		global_tracking_port=cnf_getU16(global_tracking_port,"port",\
		"tracking",cfg);
	}

	//override by per server directives
	global_daemon_mode=cnf_getByte(global_daemon_mode,"daemon",server[srv],cfg);
	global_verbose_level=cnf_getByte(global_verbose_level,"verbose_level",server[srv],cfg);
	global_port=cnf_getU16(global_port,"port",server[srv],cfg);
	strcpy((char *)global_bind_hostname,\
	(char *)cnf_getString(global_bind_hostname,"bind",server[srv],cfg));
	strcpy((char *)global_log_files_path,\
	(char *)cnf_getString(global_log_files_path,"log_files_path",server[srv],cfg));
	global_connection_timeout=cnf_getU32(global_connection_timeout,\
	"connection_timeout",server[srv],cfg);
	global_max_clients=cnf_getU32(global_max_clients,"max_players",server[srv],cfg);
	global_bandwidth=1024*cnf_getU32(global_bandwidth/1024,"bandwidth",server[srv],cfg);

	//database
	strcpy((char *)global_db_server,\
	(char *)cnf_getString(global_db_server,"db_server",server[srv],cfg));
	strcpy((char *)global_db_name,\
	(char *)cnf_getString(global_db_name,"db_name",server[srv],cfg));
	strcpy((char *)global_db_username,\
	(char *)cnf_getString(global_db_username,"db_username",server[srv],cfg));
	strcpy((char *)global_db_passwd,\
	(char *)cnf_getString(global_db_passwd,"db_passwd",server[srv],cfg));
	global_db_port=cnf_getU16(global_db_port,"db_port",server[srv],cfg);

	//set some values
	switch (global_verbose_level) {
		case 0:
			silent = 3;
			break;
		case 1:
			silent = 2;
			break;
		case 2:
			silent = 1;
			break;
		default:
			silent = 0;
			break;
	}

	switch(global_dataset) {
		case -3:
			strcpy((char *)global_dataset_tag,"pre1");
			break;
		case -2:
			strcpy((char *)global_dataset_tag,"pre2");
			break;
		case -1:
			strcpy((char *)global_dataset_tag,"undef");
			break;
		case 0:
			strcpy((char *)global_dataset_tag,"custom");
			break;
		case 1:
			strcpy((char *)global_dataset_tag,"prime");
			break;
		case 2:
			strcpy((char *)global_dataset_tag,"prime12");
			break;
		case 3:
			strcpy((char *)global_dataset_tag,"live");
			break;
		case 4:
			strcpy((char *)global_dataset_tag,"todni");
			break;
		case 5:
			strcpy((char *)global_dataset_tag,"tpots");
			break;
		case 6:
			strcpy((char *)global_dataset_tag,"tpots2");
			break;
		default:
			strcpy((char *)global_dataset_tag,(char *)cnf_getString(global_dataset_tag,"dataset","global",cfg));
			break;
	}

	//validate binary path
	strcpy((char *)global_binary_path,\
    (char *)cnf_getString(global_binary_path,"bin","global",cfg));

	//validate hostnames here, but please send the full hostname to the netcore
	//TODO

	return ret;
}
#endif
