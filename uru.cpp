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

/* Build vars - Don't touch - NEVER */
const char * ID = "$Id$";
const char * BUILD =  __DATE__ " " __TIME__;
const char * SNAME = "Alcugs (uru server)";
const char * VERSION = "1.1.15";
const char * __uru_head ="\nAlcugs H'uru Server project\n\n";

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#define I_AM_NOT_A_CLIENT

/*Includes - will be fixed with the rewrite*/
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>

#include "license.h" //mandatory, contains the license information

#include "data_types.h" //!<for data types used in this file
#include "config_parser.h" //for parsing configuration files (all globals are here)
#include "tmp_config.h"
#include "stdebug.h" //for debugging

#include "protocol.h" //all protocol specs
#include "urunet.h" //network functions

//global variable that points to all players struct
st_uru_client * all_players=NULL;
st_uru_client * auth=NULL; //points to the auth server
st_uru_client * vault=NULL; //points to the vault server
st_uru_client * track=NULL; //poinst to the tracking server

st_uru_client * meta=NULL; //points to the metaserver

int whoami=0; //Server type

//WORLD vars
//int global_n_players=0; //total number of local players

//--- protocol ---
#include "urumsg.h" //all <SND> msg's
#include "pservermsg.h" //all server messages parser
#include "pclientmsg.h" //client message parser
#include "pdefaultmsg.h" //default message (default)
#include "pscauthmsg.h" //server-client auth message parser (also all other messages)

#ifdef I_AM_THE_AUTH_SERVER
#ifdef _INTERNAL_STUFF_AUTH //*removed* from the public tree
#include "psauthmsg_internal.cpp" //old auth server message parser
#else
#include "psauthmsg.h" //auth server message parser
#endif
#endif

#ifdef I_AM_THE_VAULT_SERVER
#include "psvaultmsg.h" //vault server message parser
#include "vault_db.h"
#endif

#ifdef I_AM_THE_TRACKING_SERVER
#include "pstrackingmsg.h" //tracking server message parser
#endif

#ifdef I_AM_THE_META_SERVER
#include "psmetamsg.h" //tracking server message parser
#endif

#ifdef I_AM_A_GAME_SERVER
#include "psgamemsg.h" //Game server message parser
#endif

#include "sdlparser.h" //SDL byte code parser
#include "ageparser.h" //Age parser


//full sdl headers only
t_sdl_def * global_sdl=NULL; //sdl struct
int global_sdl_n=0; //number of global sdl records
//we need TODO..
//  -- minimized only sdl headers
//  -- dynamic game objects memory struct (read/write it from disk on server start/stop)

t_age_def * global_age_def=NULL; //age struct
int global_age_def_n=0; //total number of ages



int __s_run; //is the server running
int __s_terminated_flag; //if true log out all players and then put it false again


int _force_config_read=0;

/**
Prints version info to the specific file descriptor
*/
void version_info(FILE * f_dsc) {
	print2log(f_dsc,"%s",__uru_disclaimer_short);
	print2log(f_dsc,"%s",__uru_head);
	print2log(f_dsc,"\n%s.  Build %s - Version %s\nId: %s\n",SNAME,BUILD,VERSION,ID);
	print2log(f_dsc,"Uru protocol %s min version %s max version supported\n",__U_PROTOCOL_VMIN,__U_PROTOCOL_VMAX);
	logflush(f_dsc);
}


void parameters_usage() {
	printf("Usage: urud [-VDhf] [-v n] [-p 5000] [-c uru.conf] [-guid 0000000000000010] [-log logs] [-bcast 1]\n\n\
 -V: show version and end\n\
 -D: Daemon mode\n\
 -v 3: Set Verbose level\n\
 -h: Show short help and end\n\
 -p 5000: select the listenning port\n\
 -c uru.conf: Set an alternate configuration file\n\
 -f : Force to start the servers including if parse errors occured\n\
 -guid XXXXXXXX: Set the server guid\n\
 -name XXXXX: Set the age filename\n\
 -log folder: Set the logging folder\n\
 -bcast 1/0: Enable disable server broadcasts\n\n");
}

/*******************************************************************
   check the parameters
	 i -> position, iterator
	 what -> the paramenter to check "-v", "-V"
	 type -> 0 check only the presence
	         1 is a string, get it
	         2 is an integer, get it
	 result -> if 1, then the pointer to the string is stored here
	 iresult -> if 2, then the pointer to the int is stored here

	 returns 1 if the value is present
	         0 if not
	         -1 on error

******************************************************************/
int parameters_parse(int argc, char * argv[], int i, char * what, char type, int * result) {
	if(!strcmp(argv[i],what)) {
		if(type!=0 && i==argc-1) {
			fprintf(stderr,"ERR: Incorrect number of arguments supplied for %s\n\n",what);
			parameters_usage();
			exit(-1);
			return -1;
		}
		if(type==0) { return 1; }
		if(type==1) { //string
			*result=i+1; //return the interator to that string
			return 1;
		}
		if(type==2 && isdigit(argv[i+1][0])) {
			*result=atoi(argv[i+1]);
			return 1;
		}
		fprintf(stderr,"ERR: Incorrect supplied arguments for %s\n\n",what);
		parameters_usage();
		exit(-1);
		return -1;
	}
	return 0;

}

/**
   searches the uru.conf file trought different
	 places
*/
int load_configuration(FILE * dsc, Byte * aux_file) {
	int res;
	if(aux_file!=NULL) {
		res=get_config(dsc,(char *)aux_file);
		if(res==2) {
			fprintf(dsc,"ERR: Cannot read custom %s - using defaults\n",global_config_alt);
			return 0;
		}
	} else
	res=get_config(dsc,(char *)"uru.conf");
	if(res<=-4) {
		fprintf(stderr,"ERR: Fatal error ocurred reading config file!\n");
		exit(-1); //avoid to do nothing
	}
	if(res==-3) {
		//if(read_config(dsc,(Byte *)"/etc/uru/uru.conf")==2) {
			//if(read_config(dsc,(Byte *)".uru.conf")==2) {
				//if(read_config(dsc,(Byte *)"init/uru.conf")==2) {
					fprintf(dsc,"ERR: Cannot find/read uru.conf - using defaults\n");
					return 0;
				//}
			//}
		//}
	}
	if(res<0 && res>-3) return -1;
	return 1; //success or parse error!!
}

/**
    A decent way to start/stop the server
*/
void s_handler(int s) {
	static int st_alarm=0;
	stamp2log(f_uru);
	print2log(f_uru,"DBG: Recieved signal %i\n",s);
	switch (s) {
		case SIGHUP: //reload configuration
			print2log(f_uru,"INF: ReReading configuration\n\n");
			//parse uru.conf goes here
			load_configuration(f_err,global_config_alt);
			signal(SIGHUP, s_handler);
			break;
		case SIGALRM:
		case SIGTERM: //terminate the server in a elegant way
		case SIGINT:
			if(__s_run==0) {
				print2log(f_uru,"INF: Killed\n");
				close_log_files();
				printf("Killed!\n");
				exit(0);
			} //At second Time, kill it
			__s_terminated_flag=1;
			__s_run=0; //tell to the main while, that the server stops the execution
			signal(s,s_handler);
			break;
		case SIGUSR1:
			//I need to add here the logrotate function to rotate the logs.
			//Send an emergency "server will be down" message, start a countdown and terminate the server in a elegant way
			if(st_alarm) {
				print2log(f_uru,"INF: Automatic -Emergency- Shutdown CANCELLED\n\n");
				//code to send to players a Shutdown Cancelled
				st_alarm=0;
				alarm(0);
			} else {
				print2log(f_uru,"INF: Automatic -Emergency- Shutdown In progress in 30 seconds\n\n");
				//code to send to players a Shutdown In progress
				st_alarm=1;
				signal(SIGALRM, s_handler);
				alarm(30);
			}
			signal(SIGUSR1, s_handler);
			break;
		case SIGUSR2: //Send a TERMINATED message to all logged players.
			//Put on the terminated flag, this will send the TERMINATED message to all players
			__s_terminated_flag=1;
			print2log(f_uru,"INF: TERMINATED message sent to all players.\n\n");
			signal(SIGUSR2, s_handler);
			break;
#if 0
		case SIGCHLD:
			stamp2log(f_err);
			print2log(f_err,"INF: RECEIVED SIGCHLD: a child has exited\n\n");
			signal(SIGCHLD, s_handler);
			break;
#endif
		case SIGSEGV:
			stamp2log(f_err);
			print2log(f_err,"TERRIBLE FATAL ERROR: SIGSEGV recieved!!!\n\n");
			close_log_files();
			exit(-1);
		default:
			stamp2log(f_err);
			print2log(f_err,"\nERR: Unexpected signal recieved!\n\n");
	}
}

/**
 Install the handlers
*/
void install_handlers() {
	signal(SIGHUP, s_handler); //reload configuration
	signal(SIGTERM, s_handler); //terminate the server in a elegant way
	signal(SIGINT, s_handler); //terminate the server in a elegant way
	signal(SIGUSR1, s_handler); //Send an emergency "server will be down" message, start a countdown and terminate the server in a elegant way
	signal(SIGUSR2, s_handler); //Send a TERMINATED message to all loged players.
	//signal(SIGCHLD, s_handler);
	signal(SIGCHLD, SIG_IGN); // avoid zombies
	//signal(SIGSEGV, s_handler); //Cath SIGSEGV
}

/**
  parse arguments, returns -1 if it fails
*/
int u_parse_arguments(int argc, char * argv[]) {
	int i,ret;
	for(i=1; i<argc; i++) {
		if(parameters_parse(argc,argv,i,"-V",0,NULL)) { version_info(stdout); exit(0); }
		else if(parameters_parse(argc,argv,i,"-l",0,NULL)) {
			fprintf(stdout,"%s",__uru_disclaimer_long);
			exit(0);
		}
		else if(parameters_parse(argc,argv,i,"-D",0,NULL)) { global_daemon_mode=1; }
		else if(parameters_parse(argc,argv,i,"-D",0,NULL)) { _force_config_read=1; }
		else if(parameters_parse(argc,argv,i,"-v",2,&ret)) {
			i++;
			global_verbose_level=(Byte)ret;
			switch (global_verbose_level) {
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
		}
		else if (parameters_parse(argc,argv,i,"-p",2,&global_port)) i++; //server port
		else if (parameters_parse(argc,argv,i,"-c",1,&ret)) { //server config file
			i++;
			global_config_alt=(Byte *)argv[i];
		}
		else if (parameters_parse(argc,argv,i,"-guid",1,&ret)) { //server guid
			i++;
			strcpy((char *)global_age_guid,argv[i]);
			strcpy((char *)global_age.guid,argv[i]);
		}
		else if (parameters_parse(argc,argv,i,"-name",1,&ret)) { //server name
			i++;
			strcpy((char *)global_age.filename,argv[i]);
			strcpy((char *)global_age.name,argv[i]);
		}
		else if (parameters_parse(argc,argv,i,"-log",1,&ret)) { //server logs path
			i++;
			strcpy((char *)global_log_files_path,argv[i]);
		}
		else if (parameters_parse(argc,argv,i,"-bcast",2,&global_broadcast)) i++; //server broadcast flag
		else {
			parameters_usage();
			exit(0);
		}
	}
	return 0;
}

/* -----------------------------------------------
    MAIN CODE
-------------------------------------------------*/
int main(int argc, char * argv[]) {

	int sock, i, n; //the socket, generic iterator, output buffer size
	int sid; //session identifier

	Byte* buf; //pointer to the buffer

	//allocate the big session memory struct
	//st_uru_client * session=NULL; //alls sessions are here

	//set initial number of players
	global_max_clients=10;

	//protection to avoid a possible unwanted DOS
	int __meta_status=0;

	global_logs_enabled=1;

// to parse configuration file according to server type
#if defined(I_AM_A_GAME_SERVER)
	whoami=KGame;
#elif defined(I_AM_THE_AUTH_SERVER)
	whoami=KAuth;
#elif defined(I_AM_THE_VAULT_SERVER)
	whoami=KVault;
#elif defined(I_AM_THE_TRACKING_SERVER)
	whoami=KTracking;
#elif defined(I_AM_A_LOBBY_SERVER)
	whoami=KLobby;
#elif defined(I_AM_THE_META_SERVER)
	whoami=KMeta;
#else
#error Unsuported server type!
#endif

#if defined(I_AM_THE_TRACKING_SERVER) || defined(I_AM_THE_VAULT_SERVER) || defined(I_AM_THE_AUTH_SERVER) || defined(I_AM_THE_META_SERVER)
	global_the_bar=0; //no clients
#else
	#define AUTH_SERVER_REQUIRED
	#define VAULT_SERVER_REQUIRED
	#define TRACKING_SERVER_REQUIRED
	global_the_bar=3; //1 auth connection, 2 vault connection, 3 tracking connection
#endif

	global_client_count=global_the_bar;

	strcpy((char *)global_age.guid,"0000000000000000");
	strcpy((char *)global_age.name,"");
#ifdef I_AM_A_LOBBY_SERVER
	strcpy((char *)global_age.name,"Lobby");
#endif

	//if(license_check(argc,argv)==1) {
	//	return -1;
	//}

	//parse arguments goes here
	u_parse_arguments(argc,argv);

	//parse uru.conf goes here
	if(load_configuration(stderr,global_config_alt)==-1 && _force_config_read!=1) {
		fprintf(stderr,"FATAL, reading configuration, please check the syntax\n");
		return -1;
	}

	//here
#ifdef I_AM_A_LOBBY_SERVER
#define META_SERVER_REQUIRED
	if(global_enable_metaserver==1) {
		global_the_bar++;
		global_client_count++;
	}
#endif

	all_players=(st_uru_client *)(malloc(sizeof(st_uru_client) * (global_client_count+1)));

	if(all_players==NULL) {
		fprintf(stderr,"FATAL, not enough memory to operate\n");
		return -1;
	}

	//all_players=session;

	//init the big session struct to default values
	u_init_session(all_players,global_client_count);

	/*install some handlers */
	install_handlers();

	//enter into dameon mode if required
	if(global_daemon_mode) { daemon(1,0); }

	//open log files
	open_log_files();
	//<-

	//if(license_check(argc,argv)==1) {
	//	return -1;
	//}

	//some info
	version_info(f_uru);

#if defined(I_AM_A_GAME_SERVER)
	print2log(f_uru,"<GAME SERVER>\n");
#elif defined(I_AM_THE_AUTH_SERVER)
	print2log(f_uru,"<AUTH SERVER>\n");
#elif defined(I_AM_THE_VAULT_SERVER)
	print2log(f_uru,"<VAULT SERVER>\n");
#elif defined(I_AM_THE_TRACKING_SERVER)
	print2log(f_uru,"<TRACKING SERVER>\n");
#elif defined(I_AM_A_LOBBY_SERVER)
	print2log(f_uru,"<LOBBY SERVER>\n");
#elif defined(I_AM_THE_META_SERVER)
	print2log(f_uru,"<META SERVER>\n");
#else
#error Unsuported server type!
#endif

#ifdef I_AM_A_GAME_SERVER
	print2log(f_uru,"Parsing SDL files...\n");
	if(read_sdl_files((char *)global_sdl_folder,&global_sdl,&global_sdl_n)<0) {
		print2log(f_err,"FATAL, failed to parse the sdl files...\n");
		return -1;
	}
	print2log(f_uru,"Parsing AGE descriptor...\n");

	char path_to_age[500];

	sprintf(path_to_age,"%s/%s.age",global_age_folder,global_age.name);


	if(read_age_descriptor(f_sdl,path_to_age,&global_age_def)<0) {
		print2log(f_err,"FATAL, failed to load %s...\n",path_to_age);
		return -1;
	}
	dump_age_descriptor(f_sdl,*global_age_def);
	global_age_def_n=1;


#endif

#if defined(I_AM_THE_TRACKING_SERVER) or defined(I_AM_THE_VAULT_SERVER)

	print2log(f_uru,"Parsing AGE descriptors...\n");

	global_age_def_n=read_all_age_descriptors(f_sdl,(char *)global_age_folder,&global_age_def);
	if(global_age_def_n<=0) {
		print2log(f_err,"FATAL, failed to read AGE descriptors from %s...\n",global_age_folder);
		return -1;
	}

#endif

	print2log(f_uru,"\n");
	stamp2log(f_uru);
	print2log(f_uru,"\nPresh CTRL+C to kill the server.\n\n");
	print2log(f_uru,"Max Clients allowed: %i\n\n",global_max_clients);
	print2log(f_uru,"Broadcast is %i\n\n",global_broadcast);
	logflush(f_uru);
	//<--

	//multithreaded is necessary, and goes here (ummm, I never did something multithreaded at this level of complexity before

	//Start the Network Operation
#if I_AM_A_GAME_SERVER
tryagain:
#endif

/*
	This is a small temporal fix to avoid the current linking problems.

	but, this operation needs to be performed in the tracking server, and not
	in the game server. (It will gibe up as soon as reaches the port 6000)

	/me noticed that plasma does something similar, it tryes several times on
	the same port until the old instance stops, but this way is more faster.
*/

		sock=plNetStartOp(global_port,(char *)global_bind_hostname);
		if(sock<0) {
			stamp2log(f_err);
			print2log(f_err,"ERR: FATAL cannot start the Network Operation\n");
			#if I_AM_A_GAME_SERVER
				print2log(f_uru,"Attempting on another port\n");
				global_port+=27;
				if(global_port<=6000) { goto tryagain; }
			#endif
			close_log_files();
			error("ERR: FATAL cannot start the Network Operation\n");
			return -1; //be sure that all is killed
		}


	//SET global system flags.
	__s_run=1; //server running
	__s_terminated_flag=0; //TERMINATED message flag

	int ret=0;
#ifdef AUTH_SERVER_REQUIRED
	//auth always in sid=1
	auth=&all_players[1];

	//get the host
	if(unet_set_host_info((char *)global_auth_hostname,global_auth_port,auth)<0) {
		print2log(f_err,"Error parsing auth server hostname\n");
	}

	//set validation and version
	auth->validation=0x02;
	auth->minor_version=6;
	auth->major_version=12;
	auth->maxPacketSz=1024; //set maxpacketsize
	auth->release=0x00; //magic number 7//TDbg; //set debug release

	//init client & server headers (on udp server is this program & client is the remote host)
	uru_init_header(&auth->server,auth->validation);
	uru_init_header(&auth->client,auth->validation);

	//initial negotiation
	auth->authenticated=0;
	auth->negotiated=0;
	auth->flag=1; //solved 234 bugs with this line (is very important) also added a SEGFAULT
	auth->tpots=2; //2-no tpots

	//set the session initial time
	time(&auth->timestamp);
	auth->microseconds=get_microseconds();

	auth->adv_msg.max_version=12;
	auth->adv_msg.min_version=6;

	ret=plNetClientComm(sock,auth);
	if(ret<0) {
		print2log(f_err,"There was a problem contacting with the auth server\n");
	}


#endif

#ifdef VAULT_SERVER_REQUIRED
	//vault always in sid=2
	vault=&all_players[2];

	//get the host
	if(unet_set_host_info((char *)global_vault_hostname,global_vault_port,vault)<0) {
		print2log(f_err,"Error parsing vault server hostname\n");
	}

	//set validation and version
	vault->validation=0x02;
	vault->minor_version=7;
	vault->major_version=12;
	vault->maxPacketSz=1024; //set maxpacketsize
	vault->release=0x00; //magic number 7//TDbg; //set debug release

	//init client & server headers (on udp server is this program & client is the remote host)
	uru_init_header(&vault->server,vault->validation);
	uru_init_header(&vault->client,vault->validation);

	//initial negotiation
	vault->authenticated=0;
	vault->negotiated=0;
	vault->flag=1; //solved 234 bugs with this line (is very important) also added a SEGFAULT
	vault->tpots=2;

	//set the session initial time
	time(&vault->timestamp);
	vault->microseconds=get_microseconds();

	vault->adv_msg.max_version=12;
	vault->adv_msg.min_version=7;

	ret=plNetClientComm(sock,vault);
	if(ret<0) {
		print2log(f_err,"There was a problem contacting with the vault server\n");
	}

#endif

#ifdef TRACKING_SERVER_REQUIRED
	//tracking always in sid=3
	track=&all_players[3];

	//get the host
	if(unet_set_host_info((char *)global_tracking_hostname,global_tracking_port,track)<0) {
		print2log(f_err,"Error parsing tracking server hostname\n");
	}

	//set validation and version
	track->validation=0x02;
	track->minor_version=7;
	track->major_version=12;
	track->maxPacketSz=1024; //set maxpacketsize
	track->release=0x00; //magic number 7//TDbg; //set debug release

	//init client & server headers (on udp server is this program & client is the remote host)
	uru_init_header(&track->server,track->validation);
	uru_init_header(&track->client,track->validation);

	//initial negotiation
	track->authenticated=0;
	track->negotiated=0;
	track->flag=1; //solved 234 bugs with this line (is very important) also added a SEGFAULT
	track->tpots=2;

	//set the session initial time
	time(&track->timestamp);
	track->microseconds=get_microseconds();

	track->adv_msg.max_version=12;
	track->adv_msg.min_version=7;

	ret=plNetClientComm(sock,track);
	if(ret<0) {
		print2log(f_err,"There was a problem contacting with the tracking server\n");
	}


#endif

#ifdef META_SERVER_REQUIRED
	if(global_enable_metaserver==1) {
		//tracking always in sid=4
		meta=&all_players[4];

		//get the host
		if(unet_set_host_info((char *)global_metaserver_address,global_metaserver_port,meta)<0) {
			print2log(f_err,"Error parsing metaserver server hostname\n");
		}

		//set validation and version
		meta->validation=0x02;
		meta->minor_version=7;
		meta->major_version=12;
		meta->maxPacketSz=1024; //set maxpacketsize
		meta->release=0x00; //magic number 7//TDbg; //set debug release

		//init client & server headers (on udp server is this program & client is the remote host)
		uru_init_header(&meta->server,meta->validation);
		uru_init_header(&meta->client,meta->validation);

		//initial negotiation
		meta->authenticated=0;
		meta->negotiated=0;
		meta->flag=1; //solved 234 bugs with this line (is very important) also added a SEGFAULT
		meta->tpots=2;

		//set the session initial time
		time(&meta->timestamp);
		meta->microseconds=get_microseconds();

		meta->adv_msg.max_version=12;
		meta->adv_msg.min_version=7;

		ret=plNetClientComm(sock,meta);
		if(ret<0) {
			print2log(f_err,"There was a problem contacting with the meta server\n");
		}
	}

#endif

#ifdef I_AM_THE_VAULT_SERVER

	int ver;
	ver=plVaultGetVersion();

	if(ver<vault_version && ver>=0) {
		plog(f_uru,"INF: Your vault format is outdated, I'm going to try to migrate it to the new format.\n");
		if(plVaultMigrate(ver)<0) {
			plog(f_err,"WAR: Fatal, vault migration from version %i to version %i failed!!\n",ver,vault_version);
		}
	} //else //ok, error, non-existent, failed, etc..

	if(plVaultGetFolder()<0) {
		print2log(f_err,"FATAL, cannot negotiate the folder name, the mysql database may be down, or there is a corruption in the vault database, check your configuration settings!\n");
		return -1;
	}
#endif

//#ifdef I_AM_A_GAME_SERVER
if(track!=NULL) {
	plNetMsgCustomSetGuid(sock,global_age.guid,global_age.name,\
global_public_ip_address,global_private_network_mask,track);
}
//#endif


	plog(f_uru,"INF: %s [%s] ready - node %i\n",global_age.name,global_age.guid,global_age.node);

	//SingleThreaded main bucle
	while(__s_run==1) {
		//waiting for Messages
		/*if(global_client_count>=4) {
			DBG(6,"[%i] ip is %08X\n",4,session[4].client_ip);
		}*/
		DBG(4,"after plNetRecv\n");
		n = plNetRecv(sock,&sid,&all_players);
		DBG(4,"before plNetRecv\n");
		//all_players=session;
#ifdef AUTH_SERVER_REQUIRED
		//auth always in sid=1
		auth=&all_players[1];
#endif
#ifdef VAULT_SERVER_REQUIRED
		//vault always in sid=2
		vault=&all_players[2];
#endif
#ifdef TRACKING_SERVER_REQUIRED
		//tracking always in sid=3
		track=&all_players[3];
#endif
#ifdef META_SERVER_REQUIRED
		if(global_enable_metaserver==1) {
			meta=&all_players[4];
		}
#endif

		/*if(global_client_count>=4) {
			DBG(6,"[%i] ip is %08X\n",4,session[4].client_ip);
		}*/

		DBG(5,"step 1\n");
		if(n<0 && n!=-1) {
			stamp2log(f_err);
			print2log(f_err,"ERR: Fatal cannot recieve a message\n");
			ERR(4,"FATAL - CANNOT RECIEVE\n");
			//return -1; //be sure that all is killed
		}
		DBG(5,"step 2\n");
		if(__s_terminated_flag==1) { //Kill all players
			print2log(f_uru,"INF: Server logoff, disconnecting all players\n");
			for(i=global_the_bar+1; i<=global_client_count; i++) {
				if(all_players[i].client_ip!=0) {
		#if defined(I_AM_THE_TRACKING_SERVER) || defined(I_AM_THE_VAULT_SERVER) || defined(I_AM_THE_AUTH_SERVER)
					all_players[i].ki=0;
					all_players[i].adv_msg.ki=0;
		#endif
					plNetMsgTerminated(sock,RKickedOff,&all_players[i]);
					all_players[i].client_ip=0;
					all_players[i].flag=0;
				}
			}
			__s_terminated_flag=0;
		}

		DBG(5,"step 3\n");
		DBG(5,"n is %i\n",n);
		//if nothing went bad, then process the packet
		if(n>0) {

			buf=all_players[sid].rcv_msg;

			switch(all_players[sid].client.t) {
				case NetMsg0:
				case NetMsg1:
					//THE MOM OF THE EGGS... In other words... LA MARE DELS OUS!!!
					int off;
					off=parse_plNet_msg(all_players[sid].rcv_msg,&all_players[sid]);
					st_uru_client * a;
					a=&all_players[sid];
					if(sid!=0 && sid<=global_the_bar) { //act as a client parser
						ret=process_client_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						if(ret==-7) {
							ret=process_scauth_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
						if(ret==-7) {
							ret=process_default_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
						if(ret<0) {
							if(a->adv_msg.cmd!=NetMsgTerminated && a->adv_msg.cmd!=NetMsgPlayerTerminated) {
							print2log(f_err,"ERR: There was a problem parsing client message %04X\n",a->adv_msg.cmd);
							}
							//search by ki
#if 1
							if(a->adv_msg.ki>1000 && (a->adv_msg.cmd==NetMsgTerminated || a->adv_msg.cmd==NetMsgPlayerTerminated)) {
								int aux_reason=a->adv_msg.reason;
//#ifdef I_AM_THE_GAME_SERVER
								if(track!=NULL && a==track && a->adv_msg.cmd==NetMsgTerminated) {
									plNetMsgCustomSetGuid(sock,global_age.guid,global_age.name,\
global_public_ip_address,global_private_network_mask,track);

								}
//#endif
								for(i=global_the_bar+1; i<=global_client_count; i++) {
									if((U32)all_players[i].ki==a->adv_msg.ki) {
										a=&all_players[i];
										plNetMsgTerminated(sock,aux_reason,a);
										break;
									}
								}
							} else if(a->adv_msg.cmd==NetMsgTerminated && a==track) {
									plog(f_uru,"Tracking says to shut down the service... Shutting down...\n");
									__s_run=0;
							}
							//end search
#endif
						}
					} else {
						ret=-7;
						//GUI(4,session[sid].guid);
#ifdef I_AM_THE_AUTH_SERVER
						if(ret==-7) {
							ret=process_auth_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
#endif
			//GUI(4,session[sid].guid);
#ifdef I_AM_THE_VAULT_SERVER
						if(ret==-7) {
							ret=process_svault_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
#endif
#ifdef I_AM_THE_TRACKING_SERVER
						if(ret==-7) {
							ret=process_stracking_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
#endif
#ifdef I_AM_THE_META_SERVER
						if(ret==-7) {
							ret=process_smeta_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
#endif
			//GUI(4,session[sid].guid);
						if(ret==-7) {
							ret=process_server_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
#ifdef I_AM_A_GAME_SERVER
						if(ret==-7 || ret==-15) {
							ret=process_sgame_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
#endif
									//GUI(4,session[sid].guid);
						if(ret==-7) {
							ret=process_default_plNetMsg(sock,a->rcv_msg+off,n-off,a);
						}
									//GUI(4,session[sid].guid);
						if(ret==-15) {
							print2log(f_uru,"The player attempted to send a message without being authed\n");
							plNetMsgTerminated(sock,RNotAuthenticated,a);
						} else if(ret<0) {
							print2log(f_uru,"There was a problem parsing server message %04X\n",a->adv_msg.cmd);
#ifdef I_AM_A_GAME_SERVER
							if(0) {
#endif
								plNetMsgTerminated(sock,RUnknown,a);
#ifdef I_AM_A_GAME_SERVER
							}
#endif
						}
						//GUI(4,session[sid].guid);
					}

					/*if(sid!=0 && sid<s_the_bar) { //client parser
						process_sclient_plNetMsg(sock,
					}
					process_server_plNetMsg(sock,\
session[sid].rcv_msg+off,n-off,&session[sid]);*/
					//n=uru_process_packet(buf,n,i,session[sid],&flags);
					//GUI(4,session[sid].guid);
					break;
				default:
					stamp2log(f_une);
					ip2log(f_une,&all_players[sid]);
					print2log(f_une,"[sid: %i] ERR: Recieved an unexpected packet code %0x02\n",sid,all_players[sid].client.t);
					//abort();
					print2log(f_une,"<----------- Unexpected packet ------------->\n");
					dump_packet(f_une,buf,n,0,5);
					print2log(f_une,"\n<-------------------------------------------->\n\n");
			}
		} else {

			static Byte timer=0;

			//idle operations
			if(n==-1) {
				timer++;
				if(timer%30==0) {
					print2log(f_uru,"Idle...\n");
					DBG(3,"Doing idle operation...\n");
					for(i=global_the_bar; i>0; i--) {
						if(plNetCheckAckDone(&all_players[i])==1) {
							if(meta!=&all_players[i]) {
								DBG(3,"Sending an alive message to %i\n",i);
								plNetMsgAlive(sock,&all_players[i]);
							} else {
								__meta_status=0;
							}
						} else {
							if(meta!=NULL && meta==&all_players[i]) {
								__meta_status++;
							}
						}
					}
#ifdef I_AM_THE_TRACKING_SERVER
					do_idle_operation();
#endif

#ifdef I_AM_THE_META_SERVER
					do_meta_idle_operation(sock);
#endif

#ifdef I_AM_A_GAME_SERVER
					static Byte kill_me=0;
					if(global_the_bar>=global_client_count) {
						kill_me++;
					} else {
						if(kill_me>=2) {
							plNetMsgCustomSetGuid(sock,global_age.guid,global_age.name,\
global_public_ip_address,global_private_network_mask,track);
						}
						kill_me=0;
					}
					if(kill_me==2) {
						plNetMsgLeave(sock,RQuitting,track); //attempt to accelerate logout
					}
					if(kill_me>2) { //set more time, and see what happens
						__s_run=0;
						plog(f_uru,"Since I'm alone, and sad, I'm going to kill myself\n");
					}
#endif
				}
			}

		}
		static int counter=0;
		counter++;
		if(n!=-1 && counter>300) {
			counter=0;
			for(i=global_the_bar; i>0; i--) {
				if(plNetCheckAckDone(&all_players[i])==1) {
					if(meta!=&all_players[i]) {
						DBG(3,"Sending an alive message to %i\n",i);
						plNetMsgAlive(sock,&all_players[i]);
					} else {
						__meta_status=0;
					}
				} else {
					if(meta!=NULL && meta==&all_players[i]) {
						__meta_status++;
					}
				}
			}
		}

		//print2log(f_uru,"%i\n",__meta_status);

		if(meta!=NULL && __meta_status>1) {
			print2log(f_err,"INFO: Seems that the metaserver is down, unreachable, dead, or overloaded :(\n");
			meta->client_ip=0;
			meta=NULL; //disable the metaserver
			global_enable_metaserver=0;
		}

		//clear the last session id
		//sid=0;
		DBG(5,"step 4\n");

	} //main gameserver loop

		DBG(5,"ending game server loop\n");

	print2log(f_uru,"INF: Server logoff, disconnecting all players\n");
	for(i=global_the_bar+1; i<=global_client_count; i++) {
		if(all_players[i].client_ip!=0) {
#if defined(I_AM_THE_TRACKING_SERVER) || defined(I_AM_THE_VAULT_SERVER) || defined(I_AM_THE_AUTH_SERVER)
			all_players[i].ki=0;
			all_players[i].adv_msg.ki=0;
#endif
			plNetMsgTerminated(sock,RKickedOff,&all_players[i]);
			all_players[i].client_ip=0;
			all_players[i].flag=0;
		}
	}

	for(i=global_the_bar; i>0; i--) {
		print2log(f_uru,"Leaving... disconnecting peer %i\n",i);
		if(all_players[i].client_ip!=0) {
			plNetMsgLeave(sock,RQuitting,&all_players[i]);
		}
	}

	close(sock); //close the socket
	print2log(f_uru,"INF: SERVICE Sanely TERMINATED\n\n");
	/* closing the log files */
	close_log_files();

	return 0;
}

