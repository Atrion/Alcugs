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
const char * VERSION = "1.3.1p"; //Urunet 3, updated 27/01/2005

//#define _DBG_LEVEL_ 5

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#ifdef __WIN32__
#include "windoze.h"
#else
#include <netdb.h>
#endif

#ifndef __MSVC__
#include <unistd.h>
#endif

#include "license.h" //mandatory, contains the license information
#include "version.h"

#include "data_types.h" //for data types used in this file
#include "stdebug.h" //for debugging

/* netcore */
#include "urunet.h" //network functions
#include "protocol.h" //protocol stuff
#include "prot.h"

#include "useful.h"

#include "config_parser.h" //configuration

#include "settings.h" //uru settings

//--- protocol msgs---
/* msg generator */
#include "gbasicmsg.h" //basic msg's

/* msg parser */
#include "pbasicmsg.h" //basic server msg's
#include "pnetmsg.h" //basic server msg's (before auth)
#include "pdefaultmsg.h" //default message (default)

#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
#include "gcsbvaultmsg.h" //vault msgs
#include "gctrackingmsg.h" //tracking msgs
#include "pcauthmsg.h" //client auth
#include "psauthmsg.h" //server auth
#include "pcbvaultmsg.h" //client basic vault
#include "psbvaultmsg.h" //server basic vault
#include "pclobbymsg.h" //lobby basic
#include "ptrackingmsg.h" //server tracking
#include "pvaultfordmsg.h" //vault forwarder
#include "pvaultroutermsg.h" //vault router
#include "lobbysubsys.h" //lobby subsystem
#endif

//auth subsystem
#if defined(I_AM_THE_AUTH_SERVER)
#include "pscauthmsg.h" //auth server message parser
#include "auth.h" //authentication subsystem
#endif

//vault subsystem
#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER) || (I_AM_THE_VAULT_SERVER)
#include "vaultsubsys.h" //vault subsystem
#endif

#ifdef I_AM_THE_VAULT_SERVER
#include "pvaultservermsg.h" //NetMsgVault and NetMsgVaultTask adv parsers
#include "pcsbvaultmsg.h" //vault server basic vault
#include "vserversys.h" //vault server subsystem
#endif

#ifdef I_AM_A_GAME_SERVER
#include "pcgamemsg.h" //game message parser
#include "pcjgamemsg.h" //game message parser (only the joinreq message)
#include "gamesubsys.h" //game server subsystem
#include "pythonsubsys.h" //Python subsystem
#endif

#ifdef I_AM_THE_TRACKING_SERVER
#include "pctrackingmsg.h" //tracking server message parser
#include "trackingsubsys.h" //tracking subsystem
#endif

#ifdef I_AM_THE_META_SERVER
//#include "psmetamsg.h" //tracking server message parser
#endif

#include "debug.h"

//Be carefull with this, well, It's temporal, but It's going to change!
st_unet net; //net session

//globals & shared
st_config * global_config=NULL; //the config params are here

//netcore status
int __state_running=1; //is the server running?
int __s_terminated_flag=0; //if true log out all players and then put it false again

//other
int _force_config_read=0; //force server launch with syntax errors on config files?

void parameters_usage() {
	printf("Usage: urud [-VDhfl] [-v n] [-p 5000] [-c uru.conf] [-name AvatarCustomization] [-guid 0000000000000010] [-log logs] [-bcast 1] [-nokill] [-clean]\n\n\
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
 -bcast 1/0: Enable disable server broadcasts\n\
 -l: Shows the servers license\n\
 -nokill: Don't kill the gameservers, let them running [kGame].\n\
 -clean: Performs vault cleanup on vault server startup [kVault].\n\n");
}

/**
   searches the uru.conf file trought different places
	 flags:
	 0x00 don't override
	 0x01 override some directives
*/
int load_configuration(FILE * dsc, Byte * aux_file,st_config ** cfg2,char flags) {
	int res;
	if(aux_file!=NULL) {
		res=get_config(dsc,(char *)aux_file,cfg2,flags);
	} else {
		res=get_config(dsc,(char *)"uru.conf",cfg2,flags);
	}

	if(res==2) {
		fprintf(dsc,"ERR: Cannot read custom %s - using defaults\n",aux_file);
	}
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
	plog(f_uru,"DBG: Recieved signal %i\n",s);
	switch (s) {
		case SIGHUP: //reload configuration
			plog(f_uru,"INF: ReReading configuration\n\n");
			//parse uru.conf goes here
			load_configuration(stderr,(Byte *)cnf_getString("uru.conf","read_config","global",\
			global_config),&global_config,0x01);
			signal(SIGHUP, s_handler);
			//stop/start the diferent subsystems
#if defined(I_AM_THE_AUTH_SERVER)
			reload_auth_driver();
#endif
#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
			reload_lobby_subsys();
			reload_vault_subsys();
#endif
#if defined(I_AM_THE_VAULT_SERVER)
			reload_vault_subsys();
			update_vault_server_subsys(); //Don't use reload, because it will drop the MGRS!
#endif
#if defined(I_AM_THE_TRACKING_SERVER)
			reload_tracking_subsys();
#endif
#if defined(I_AM_THE_GAME_SERVER)
			update_game_subsys();
			update_python_subsys();
#endif
			break;
		case SIGALRM:
		case SIGTERM: //terminate the server in a elegant way
		case SIGINT:
			if(__state_running==0) {
				plog(f_uru,"INF: Killed\n");
				log_shutdown();
				printf("Killed!\n");
				exit(0);
			} //At second Time, kill it
			//__s_terminated_flag=1;
			__state_running=0; //tell to the main while, that the server stops the execution
			signal(s,s_handler);
			break;
		case SIGUSR1:
			//I need to add here the logrotate function to rotate the logs.
			//Send an emergency "server will be down" message, start a countdown and terminate the server in a elegant way
			if(st_alarm) {
				plog(f_uru,"INF: Automatic -Emergency- Shutdown CANCELLED\n\n");
				//code to send to players a Shutdown Cancelled
				st_alarm=0;
				alarm(0);
			} else {
				plog(f_uru,"INF: Automatic -Emergency- Shutdown In progress in 30 seconds\n\n");
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
			plog(f_uru,"INF: TERMINATED message sent to all players.\n\n");
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
			plog(f_err,"TERRIBLE FATAL ERROR: SIGSEGV recieved!!!\n\n");
			log_shutdown();
			exit(-1);
		default:
			plog(f_err,"\nERR: Unexpected signal recieved!\n\n");
	}
}

/**
 Install the handlers
*/
void install_handlers() {
	signal(SIGTERM, s_handler); //terminate the server in a elegant way
	signal(SIGINT, s_handler); //terminate the server in a elegant way
#ifndef __WIN32__
	signal(SIGHUP, s_handler); //reload configuration
	signal(SIGUSR1, s_handler); //Send an emergency "server will be down" message, start a countdown and terminate the server in a elegant way
	signal(SIGUSR2, s_handler); //Send a TERMINATED message to all logged players.
	//signal(SIGCHLD, s_handler);
	signal(SIGCHLD, SIG_IGN); // avoid zombies
#endif
	//signal(SIGSEGV, s_handler); //Cath SIGSEGV
}

/**
  parse arguments, returns -1 if it fails
*/
int u_parse_arguments(int argc, char * argv[],st_config ** cfg2) {
	int i,ret;
	int silent;

	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) {
			parameters_usage(); return -1;
		} else if(!strcmp(argv[i],"-V")) {
			version(stdout);
			return -1;
		} else if(!strcmp(argv[i],"-l")) {
			version(stdout);
			show_bigdisclaimer();
			return -1;
		} else if(!strcmp(argv[i],"-D")) {
			cnf_add_key("1","daemon","global",cfg2);
		} else if(!strcmp(argv[i],"-nokill")) {
			cnf_add_key("1","game.persistent","global",cfg2);
		} else if(!strcmp(argv[i],"-f")) {
			_force_config_read=1;
		} else if(!strcmp(argv[i],"-v") && argc>i+1) {
			i++; ret=atoi(argv[i]);
			switch (ret) {
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
			cnf_add_key(argv[i],"verbose_level","global",cfg2);
			cnf_setU32(silent,"d.silent","global",cfg2);
		} else if(!strcmp(argv[i],"-p") && argc>i+1) {
			i++;
			cnf_add_key(argv[i],"port","global",cfg2);
		} else if(!strcmp(argv[i],"-c") && argc>i+1) {
			i++;
			cnf_add_key(argv[i],"read_config","global",cfg2);
		} else if(!strcmp(argv[i],"-guid") && argc>i+1) {
			i++;
			cnf_add_key(argv[i],"age_guid","global",cfg2);
		} else if(!strcmp(argv[i],"-name") && argc>i+1) {
			i++;
			cnf_add_key(argv[i],"age_filename","global",cfg2);
		} else if(!strcmp(argv[i],"-log") && argc>i+1) {
			i++;
			cnf_add_key(argv[i],"log_files_path","global",cfg2);
		} else if(!strcmp(argv[i],"-bcast") && argc>i+1) {
			i++;
			cnf_add_key(argv[i],"broadcast","global",cfg2);
		} else if(!strcmp(argv[i],"-clean")) {
			cnf_add_key("1","vault.clean","global",cfg2);
		} else {
			parameters_usage();
			return -1;
		}
	}
	return 0;
}

/**
	Reload the configuration when the server is running.
	0x00 startup
	0x01 reload
*/
void	reaply_settings(st_unet * net,Byte flags) {

	if(cnf_exists("net.maxconnections","global",global_config)==1) {
		net->max=cnf_getByte(net->max,"net.maxconnections","global",global_config);
	}
	if(cnf_exists("net.timeout","global",global_config)==1) {
		net->timeout=cnf_getU32(net->max,"net.timeout","global",global_config);
	}
	if(cnf_exists("net.up","global",global_config)==1) {
		net->nat_up=cnf_getU32(net->nat_up/1000,"net.up","global",global_config)*1000;
	}
	if(cnf_exists("net.down","global",global_config)==1) {
		net->nat_down=cnf_getU32(net->nat_down/1000,"net.down","global",global_config)*1000;
	}
	if(cnf_exists("net.lan.up","global",global_config)==1) {
		net->lan_up=cnf_getU32(net->lan_up/1000,"net.up","global",global_config)*1000;
	}
	if(cnf_exists("net.lan.down","global",global_config)==1) {
		net->lan_down=cnf_getU32(net->lan_down/1000,"net.down","global",global_config)*1000;
	}

	//public address
	strcpy(net->address,(char *)cnf_getString(net->address,\
	"public_address","global",global_config));

	net->lan_mask=(U32)inet_addr(cnf_getString("255.255.255.0",\
	"private_mask","global",global_config));

	if(cnf_exists("private_network","global",global_config)==1) {
		net->lan_addr=(U32)inet_addr(cnf_getString("0.0.0.0",\
		"private_network","global",global_config));
	} else {
		struct hostent *host;
		host=gethostbyname(cnf_getString("localhost",\
	"bind","global",global_config));
		if(host!=NULL) {
			net->lan_addr=*(U32 *)host->h_addr_list[0] & net->lan_mask;
		} else {
			net->lan_addr=0;
		}
	}

	net->spawn_start=cnf_getU16(5000,"spawn.start","global",global_config);
	net->spawn_stop=cnf_getU16(6000,"spawn.stop","global",global_config);

	if(cnf_getByte(0,"net.noflood","global",global_config)) {
		net->flags |=  UNET_NOFLOOD; //enable anti flooding
	} else {
		net->flags &=  ~UNET_NOFLOOD; //disable
	}

	//depreceated ~old protocol
	// will be removed in the future, we are not going to support the old protocol, in
	// new releases
	net->pro_auth=cnf_getByte(0,"auth.oldprotocol","global",global_config);
	net->pro_vault=cnf_getByte(0,"vault.oldprotocol","global",global_config);
	net->pro_tracking=cnf_getByte(0,"tracking.oldprotocol","global",global_config);
	if(net->pro_auth!=0 && net->pro_auth!=1) net->pro_auth=0;
	if(net->pro_vault!=0 && net->pro_vault!=1) net->pro_vault=0;
	if(net->pro_tracking!=0 && net->pro_tracking!=1) net->pro_tracking=0;
	if(net->pro_tracking==0) {
		_DIE("You want to fry your tracking server?, then set tracking.oldprotocol=1\n");
	}

}

/** Kills the specific player by sid
		flag: 0x01 clean the session
		flag: 0x02 don't send terminated
*/
void kill_player(st_unet * net,int reason,int sid,Byte flag) {
	nlog(net->sec,net,sid,"kill_player() request peer:%i,whoami:%i[%s],reason:%i[%s],flag:%i\n",sid,net->s[sid].whoami,\
	unet_get_destination(net->s[sid].whoami),reason,unet_get_reason_code(reason),flag);
	if(net_check_address(net,sid)!=0) {
		plog(net->sec,"kill_player request failed, Reason: Out of range %i\n",sid);
		return;
	}

	//but be sure that we are not killing a server...
	if(net->whoami==KLobby || net->whoami==KGame) {
		if(net->s[sid].whoami!=KClient && net->s[sid].whoami!=0) {
			plog(net->sec,"kill_player request failed, Reason: Invalid type of client #1 %i\n",net->s[sid].whoami,\
	unet_get_destination(net->s[sid].whoami));
			return;
		}
	} else {
		if(net->s[sid].whoami!=KGame && net->s[sid].whoami!=KLobby &&\
		 net->s[sid].whoami!=0) {
			plog(net->sec,"kill_player request failed, Reason: Invalid type of client #2 %i\n",net->s[sid].whoami,\
	unet_get_destination(net->s[sid].whoami));
			return;
		}
	}

#if !defined(I_AM_A_GAME_SERVER) && !defined(I_AM_A_LOBBY_SERVER)
	net->s[sid].hmsg.ki=0;
#endif

	//do here the correct an nice player cleanning
	nlog(net->sec,net,sid,"Killing... disconnecting peer %i: %i %s\n",sid,net->s[sid].whoami,\
	unet_get_destination(net->s[sid].whoami));

	if(!(flag & 0x02)) {
		plog(net->sec,"kill_player, a plNetMsgTerminated reason %i was sent",reason);
		plNetMsgTerminated(net,reason,sid);
	}

#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
	if(net->s[sid].ki!=0) {
		U32 timer;
		timer=time(NULL);

		timer=timer-net->s[sid].nego_stamp;

		plog(net->sec,"kill_player, Player status update code is being run...\n");
		if(reason!=RLoggedInElsewhere) {
			plNetMsgCustomPlayerStatus(net,0,0,net->tracking,sid,net->pro_tracking);
		}

		net->s[net->pro_vault].hmsg.ki=net->s[sid].ki;

		plNetMsgCustomVaultPlayerStatus(net,(char *)"",(char *)"0000000000000000",\
		0,timer,net->vault,net->pro_vault);
	}
#endif

	nlog(net->sec,net,sid,"Connection terminated %i %s %i\n",reason,unet_get_reason_code(reason),flag);

	if(flag & 0x01) {
		plog(net->sec,"plNetDestroySession() call\n");
		plNetDestroySession(net,sid); //hmmm
	} else {
		plog(net->sec,"plNetEndConnection() call\n");
		plNetEndConnection(net,sid); //<- this one is better, a lot better
	}
}

/** Kills the specific player by ki
*/
void kill_playerbyki(st_unet * net,int reason,int ki,Byte flag) {
	DBG(5,"kill_playerbyki ki:%i f:%i\n",ki,flag);
	if(ki<=0) return;

	int i;
	for(i=0; i<(int)net->n; i++) {
		DBG(5,"[%i] %i=%i?\n",i,net->s[i].ki,ki);
		if(net->s[i].ki==ki) {
			kill_player(net,reason,i,flag);
		}
	}
}

//flag 0x01 and clean, 0x00 and not clean
void kill_players(st_unet * net,Byte flag) {
	int i;
	plog(f_uru,"INF: Server logoff, disconnecting all players\n");
	for(i=0; i<(int)net->n; i++) {
		if(net->s[i].ip!=0) {
			kill_player(net,RKickedOff,i,flag);
		}
	}
}

#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
/**
	Starts the connection to the specified peer
*/
int reconnect2peer(st_unet * net,int dst) {
	int ret=0;
	U16 port;
	char * host;
	int sid=-1;
	Byte flags=0;

	switch(dst) {
		case KAuth:
			host=(char *)cnf_getString("127.0.0.1","auth","global",global_config);
			port=cnf_getU16(2010,"auth.port","global",global_config);
			break;
		case KVault:
			host=(char *)cnf_getString("127.0.0.1","vault","global",global_config);
			port=cnf_getU16(2012,"vault.port","global",global_config);
			flags |=UNET_VAL0;
			break;
		case KTracking:
			host=(char *)cnf_getString("127.0.0.1","tracking","global",global_config);
			port=cnf_getU16(2011,"tracking.port","global",global_config);
			flags |=UNET_VAL0;
			break;
		case KMeta:
			host="metaserver.uru3.almlys.dyns.net";
			port=8000;
			break;
		default:
			plog(f_err,"Connection to the unknown service %i requested!\n",dst);
			ret=UNET_ERR;
			break;
	}

	if(ret==UNET_OK) {
		ret=plNetConnect(net,&sid,host,port,flags);
	}

	if(ret==UNET_OK) {
		net->s[sid].timeout=5*60; //set timeout
		net->s[sid].whoami=dst; //set peer type
		//and send server hello if required

		switch(dst) {
			case KAuth:
				net->auth=sid;
				plNetMsgAlive(net,sid);
				break;
			case KVault:
				net->vault=sid;
				plNetMsgAlive(net,sid);
				break;
			case KTracking:
				net->tracking=sid;
				plNetMsgAlive(net,sid);
				plNetMsgCustomSetGuid(net,sid,net->pro_tracking);
				break;
			case KMeta:
				net->meta=sid;
				break;
		}
	}

	if(ret!=UNET_OK) {
		print2log(f_err,"There was a problem contacting with the %i %s server. Error %i %s\n",dst,unet_get_destination(dst),ret,get_unet_error(ret));
		sid=-1;
	}
	return sid;
}
#endif

#ifdef __WIN32__
int __stdcall _windows_control_handler(unsigned long type)
{
   if(type==CTRL_C_EVENT || type==CTRL_CLOSE_EVENT)
   {
      s_handler(SIGINT);
      return true;
   }
   return false;
}
#endif

/*-----------------------------------------------
    MAIN CODE
-------------------------------------------------*/
int main(int argc, char * argv[]) {

#ifdef __WIN32__
	SetConsoleCtrlHandler(_windows_control_handler, true);
#endif

	char hostname[300];
	U16 port=5000;

	Byte * msg=NULL;
	int ret,i,off,size,sid; //return codes, msg offset, msg size, session identifier

	//Init the net struct
	plNetInitStruct(&net);

	//Set here initial default unet flags
	net.flags &= (~UNET_ELOG & ~UNET_FLOG); //globally disable logging

	//init entropy
	srandom(get_microseconds() + ((U32)(time(NULL) % 10000)));

	//set the server role
#if defined(I_AM_A_GAME_SERVER)
	net.whoami=KGame;
	strcpy(net.name,"Game");
	Byte persistent=0;
#elif defined(I_AM_THE_AUTH_SERVER)
	net.whoami=KAuth;
	strcpy(net.name,"Auth");
	net.timeout=15*60; //Change the timeout
#elif defined(I_AM_THE_VAULT_SERVER)
	net.whoami=KVault;
	strcpy(net.name,"Vault");
	net.timeout=30*60; //Change the timeout
#elif defined(I_AM_THE_TRACKING_SERVER)
	net.whoami=KTracking;
	strcpy(net.name,"Tracking");
	net.timeout=5*60; //Change the timeout
#elif defined(I_AM_A_LOBBY_SERVER)
	net.whoami=KLobby;
	strcpy(net.name,"Lobby");
#elif defined(I_AM_THE_META_SERVER)
	net.whoami=KMeta;
	strcpy(net.name,"Meta");
	net.timeout=15; //Change the timeout
#elif defined(I_AM_THE_DATA_SERVER)
	net.whoami=KData;
	strcpy(net.name,"Data");
	net.timeout=15; //Change the timeout
#elif defined(I_AM_THE_ADMIN_SERVER)
	net.whoami=KAdmin;
	strcpy(net.name,"Admin");
	net.timeout=15; //Change the timeout
#else
#error Unsuported server type!
#endif

	cnf_setByte(net.whoami,"whoami","global",&global_config);
	strcpy(net.guid,"0000000000000000");

	//parse arguments goes here
	if(u_parse_arguments(argc,argv,&global_config)!=0) { return -1; }

	//parse uru.conf goes here
	if(load_configuration(stderr,\
	(Byte *)cnf_getString("uru.conf","read_config","global",global_config),\
	&global_config,0)==-1 && _force_config_read!=1) {
		fprintf(stderr,"FATAL, reading configuration, please check the syntax\n");
		return -1;
	}

	/*install some handlers */
	install_handlers();

	//enter into dameon mode if required
	if(cnf_getByte(0,"daemon","global",global_config)) { daemon(1,0); }

	log_init(); //automatically started by the netcore, but the log_shutdown call is mandatory
	//log_init() is a must before this call
	stdebug_config->silent=cnf_getByte(0,"d.silent","global",global_config);

	if(cnf_exists("log_files_path","global",global_config)==1) {
		char * path;
		path=(char *)cnf_getString("log/","log_files_path","global",global_config);
		//we can safely ignore the leak, since this line is only called one time,
		// but if you want to be nice, you can add a free at the end of the code, being
		// extremely carefull, since the default path points to an static struct.
		stdebug_config->path=(char *)malloc(sizeof(char)*(strlen(path)+1));
		strcpy(stdebug_config->path,path);
	}
	//set other stdebug_config params __here__

	if(cnf_getByte(1,"log.enabled","global",global_config)) {
		log_openstdlogs();
	}

	//set other net params __here__
	if(cnf_getByte(1,"net.log.enabled","global",global_config)) {
		net.flags |= UNET_FLOG | UNET_ELOG;
	}

	if(cnf_getByte(0,"net.auth","global",global_config)==0) {
		net.flags &= ~UNET_NETAUTH; //dissable net.auth
	}

	//logs
	if(cnf_getByte(1,"net.log.chk","global",global_config)==0) {
		net.flags |= UNET_DLCHK;
	}
	if(cnf_getByte(1,"net.log.une","global",global_config)==0) {
		net.flags |= UNET_DLUNE;
	}
	if(cnf_getByte(1,"net.log.ack","global",global_config)==0) {
		net.flags |= UNET_DLACK;
	}
	if(cnf_getByte(1,"net.log.sec","global",global_config)==0) {
		net.flags |= UNET_DLSEC;
	}

	if(cnf_exists("age_guid","global",global_config)) {
		strncpy(net.guid,cnf_getString("","age_guid","global",global_config),17);
	}
	if(cnf_exists("age_filename","global",global_config)) {
		strncpy(net.name,cnf_getString("","age_filename","global",global_config),190);
	}

	//some info
	version_info(f_uru);

	if(cnf_getByte(0,"stop","global",global_config)!=0) {
		plog(f_err,"Server startup aborted!\n\nRTFM!!.\nThere is a stop or dissabled entry in the server configuration file (remove that entry).\n\n");
		fprintf(stderr,"Server startup aborted!\n");
		log_shutdown();
		return -1;
	}

	reaply_settings(&net,0);


#if defined(I_AM_A_GAME_SERVER)
	print2log(f_uru,"<GAME SERVER>\n");
	persistent=cnf_getByte(0,"game.persistent","global",global_config);
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
#elif defined(I_AM_THE_DATA_SERVER)
	print2log(f_uru,"<DATA SERVER>\n");
#elif defined(I_AM_THE_ADMIN_SERVER)
	print2log(f_uru,"<ADMIN SERVER>\n");
#else
#error Unsuported server type!
#endif
	lognl(f_uru);
	plog(f_uru,"The Server is running...\nPresh CTRL+C to kill the server.\n\n");
	print2log(f_uru,"Max Clients allowed: ");
	if(net.n==0) {
		print2log(f_uru,"unlimited, yay!\n\n");
	} else {
		print2log(f_uru,"%i\n\n",net.n);
	}
	logflush(f_uru);
	//<-- multithreaded goes here
	//set binding port/address
	port=cnf_getU16(port,"port","global",global_config);
	strcpy(hostname,(char *)cnf_getString("0.0.0.0","bind","global",global_config));

	ret=plNetStartOp(port,hostname,&net);
	DBG(2,"plNetStartOp res:%i\n",ret);

#if I_AM_A_GAME_SERVER
	if(ret!=UNET_OK) {
		plog(f_err,"Urunet startup failed, with return code n: %i %s!\n",ret,\
		get_unet_error(ret));
		int i;
		for(i=0; i<25; i++) {
			port+=(random() % 12);
			plog(f_err,"Attempting on port %i\n",port);
			ret=plNetStartOp(port,hostname,&net);
			if(ret==UNET_OK) break;
		}
	}
	cnf_setU16(port,"port","global",&global_config);
#endif

	if(ret!=UNET_OK) {
		plog(f_err,"Urunet startup failed, with return code n: %i %s!\n",ret,\
		get_unet_error(ret));
		cnf_destroy(&global_config); //destroy the configuration struct
		log_shutdown();
		exit(-1);
	}

//Startup the different subsystems
#if defined(I_AM_THE_AUTH_SERVER)
	if(init_auth_driver()<0 && _force_config_read!=1) {
		plog(f_err,"Auth Subsystem failure!\n Check the auth.log and sql.log for more information\n\n");
		plNetStopOp(&net);
		cnf_destroy(&global_config); //destroy the configuration struct
		log_shutdown();
		exit(-1);
	}
#endif

#if defined(I_AM_A_GAME_SERVER)
	if(init_game_subsys(&net)<0 && _force_config_read!=1) {
		plog(f_err,"Game Server Subsystem failure!\n Check the sdl.log for more information\n\n");
		plNetStopOp(&net);
		cnf_destroy(&global_config); //destroy the configuration struct
		log_shutdown();
		exit(-1);
	}
	if(init_python_subsys(&net)<0 && _force_config_read!=1) {
		plog(f_err,"Python Subsystem failure!\n Check the python.log for more information\n\n");
		stop_game_subsys();
		plNetStopOp(&net);
		cnf_destroy(&global_config); //destroy the configuration struct
		log_shutdown();
		exit(-1);
	}
#endif

#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
	reconnect2peer(&net,KAuth);
	reconnect2peer(&net,KVault);
	reconnect2peer(&net,KTracking);
	init_lobby_subsys();
	init_vault_subsys();
#endif

#if defined(I_AM_THE_VAULT_SERVER)
	init_vault_subsys();
	if(init_vault_server_subsys()<0 && _force_config_read!=1) {
		plog(f_err,"Vault Server Subsystem failure!\n Check the vmgr.log and sql.log for more information\n\n");
		stop_vault_subsys();
		plNetStopOp(&net);
		cnf_destroy(&global_config); //destroy the configuration struct
		log_shutdown();
		exit(-1);
	}
#endif

#if defined(I_AM_THE_TRACKING_SERVER)
	init_tracking_subsys();
#endif

	plog(f_uru,"INF: %s [%s] ready\n",net.name,net.guid);

	//SingleThreaded main bucle
	while(__state_running==1) {
		//waiting for Messages
		DBG(4,"after plNetRecv\n");
		ret=plNetRecv(&net,&sid);
		DBG(2,"plNetRcv event:%i from peer:%i\n",ret,sid);

		if(ret<0) { //cath up error codes
			nlog(f_err,&net,sid,"ERR: %i %s\n",ret,get_unet_error(ret));
		}

		if(__s_terminated_flag==1) { //Kill all players
			kill_players(&net,0);
			__s_terminated_flag=0;
		}
		DBG(5,"step 3\n");

		switch(ret) {
			case UNET_FLOOD:
#if defined(I_AM_A_LOBBY_SERVER) || defined(I_AM_A_GAME_SERVER)
				if(net.s[sid].whoami==KClient || net.s[sid].whoami==0) {
					nlog(net.sec,&net,sid,"WAR: Kicked a player that was flooding the server.\n");
					logflush(net.sec);
					kill_player(&net,RKickedOff,sid,0);
					//plNetDestroySession(&net,sid);
					break;
				}
#endif
			case UNET_MSGRCV:
			case UNET_NEWCONN:
				DBG(3,"ret:%i peer:%i, new message from %s:%i\n",ret,\
				sid,get_ip(net.s[sid].ip),htons(net.s[sid].port));
				size=plNetGetMsg(&net,sid,&msg); //get the message
				DBG(5,"Got a message from sid:%i of %i bytes\n",sid,size);
				//parse the message
				#if _DBG_LEVEL_ > 3
				dumpbuf(f_uru,msg,size);
				lognl(f_uru);
				#endif
				off=parse_plNet_msg(&net,msg,size,sid); //parse the header
				//--->
				if(net.s[sid].hmsg.cmd==NetMsgLeave) {
					ret=*(Byte *)(msg+off); //get the reason code
					nlog(net.log,&net,sid,"NetMsgLeave - Reason %i %s\n",\
					ret,unet_get_reason_code(ret));
#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
					if(net.s[sid].whoami==KClient || net.s[sid].whoami==0) {
						//do here the player disconnection... (from tracking and vault)
						kill_player(&net,ret,sid,0x02);
						//plNetEndConnection(&net,sid);
						//plNetDestroySession(&net,sid);
					}
#else
					//plNetDestroySession(&net,sid);
					plNetEndConnection(&net,sid);
#endif
				} else if(net.s[sid].hmsg.cmd==NetMsgTerminated || \
				net.s[sid].hmsg.cmd==NetMsgPlayerTerminated) {
					ret=*(Byte *)(msg+off); //get the reason code
					nlog(net.log,&net,sid,"NetMsgTerminated - Reason %i %s\n",\
					ret,unet_get_reason_code(ret));
#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
					if(net.s[sid].whoami==KTracking || net.s[sid].whoami==KVault || \
					 net.s[sid].whoami==KAuth) {
						if(net.s[sid].whoami==KTracking && net.s[sid].hmsg.ki==0 && \
						 net.s[sid].hmsg.cmd==NetMsgTerminated) {
							plog(f_uru,"Tracking says to shut down the service... Shutting down...\n");
							__state_running=0;
						} else {
							if(net.s[sid].hmsg.ki>0) {
								kill_playerbyki(&net,ret,net.s[sid].hmsg.ki,0);
								//abort();
							}
							//run here code to reconnect to the server (another hello, etc..)
							if(net.s[sid].hmsg.cmd==NetMsgTerminated) {
								reconnect2peer(&net,net.s[sid].whoami);
							}
						}
					}
#endif
				} else { //process here the messages
					//message parser
					ret=process_basic_plNetMsg(&net,msg+off,size-off,sid);
					if(ret==0) {
#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
						if(net.s[sid].whoami==KClient || net.s[sid].whoami==0) {
							//for clients
							if(net.s[sid].authenticated!=1) { //is authed?
								ret=process_cauth_plNetMsg(&net,msg+off,size-off,sid); //auth
							} else { //is authed
								ret=0;
								if(ret==0 && net.s[sid].ki!=0) { //check other sublevel messages
									//check for NetMsgVault and VaultTask messages, and directly send
									// themm to the Vault
									ret=process_vaultford_plNetMsg(&net,msg+off,size-off,sid);
#if defined(I_AM_A_GAME_SERVER)
									if(ret==0) {
										if(net.s[sid].paged==0x01) {
											ret=process_cgame_plNetMsg(&net,msg+off,size-off,sid); //game message parser (other messages)
										} else {
											ret=process_cjgame_plNetMsg(&net,msg+off,size-off,sid); //game message parser (netmsgjoinreq)
										}
									}
#endif
								}
								if(ret==0) {
									ret=process_clobby_plNetMsg(&net,msg+off,size-off,sid); //lobby funs
								}
								if(ret==0) {
									ret=process_cbvault_plNetMsg(&net,msg+off,size-off,sid); //basic vault
								}
							}
						} else {
							//for servers
							switch(net.s[sid].whoami) {
								case KAuth:
									ret=process_sauth_plNetMsg(&net,msg+off,size-off,sid);
									break;
								case KVault:
									ret=process_vaultrouter_plNetMsg(&net,msg+off,size-off,sid);
									if(ret==0) {
										ret=process_sbvault_plNetMsg(&net,msg+off,size-off,sid);
									}
									break;
								case KTracking:
									ret=process_tracking_plNetMsg(&net,msg+off,size-off,sid);
									break;
								default:
									nlog(f_err,&net,sid,"WAR: Recieved a message from an unknown peer\n");
									ret=0;
									break;
							}
						}
#elif defined(I_AM_THE_AUTH_SERVER)
						ret=process_scauth_plNetMsg(&net,msg+off,size-off,sid);
#elif defined(I_AM_THE_VAULT_SERVER)
						ret=process_vaultserver_plNetMsg(&net,msg+off,size-off,sid);
						if(ret==0) {
							ret=process_csbvault_plNetMsg(&net,msg+off,size-off,sid);
						}
#elif defined(I_AM_THE_TRACKING_SERVER)
						ret=process_ctracking_plNetMsg(&net,msg+off,size-off,sid);
#endif
					}//end per server msg processor

					if(ret==0 && net.s[sid].authenticated==1) {
						ret=process_net_plNetMsg(&net,msg+off,size-off,sid); //basic (authed)
					}

					if(ret==0) { //default message
						ret=process_default_plNetMsg(&net,msg+off,size-off,sid);
					}

					/*
						0  not parsed
						1  succesfully parsed
						-1 hack attempt, kick the player, kill it, and report to the authorities what he has done.
						-2 unknown, the server is depressed because all eforts to understand the message failed.
						elsewhere, kill or not to kill, that's is the question.
					*/
					if(ret!=1) {

#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
						nlog(f_err,&net,sid,"ERR: Unimplemented message %08X %s, or client is not authenticated\n",net.s[sid].hmsg.cmd,unet_get_msg_code(net.s[sid].hmsg.cmd));
						if(net.s[sid].whoami==KClient) {
							if(ret==-1) {
								kill_player(&net,RKickedOff,sid,0);
							} else if(ret==-2) {
								kill_player(&net,RUnknown,sid,0);
							} else {
								//kill_player(&net,RUnknown,sid,0);
							}
						} else if(net.s[sid].whoami==0) {
							kill_player(&net,RNotAuthenticated,sid,0x02);
						}
#else
						nlog(f_err,&net,sid,"WAR: Ignored message %08X %s\n",net.s[sid].hmsg.cmd,unet_get_msg_code(net.s[sid].hmsg.cmd));
#endif
					}
				} //end message parser
				logflush(f_uru);
				logflush(f_err);
				//--->
				//ha, this caused a big memory leak
				if(msg!=NULL) {
					free((void *)msg);
					msg=NULL;
				}
				break;
			case UNET_TIMEOUT:
			case UNET_TERMINATED:
				DBG(3,"ret:%i peer:%i, connection terminated/timeout from %s:%i\n",ret,\
				sid,get_ip(net.s[sid].ip),htons(net.s[sid].port));
				nlog(f_uru,&net,sid,"Timeout\n");
#if defined(I_AM_A_LOBBY_SERVER) || defined(I_AM_A_GAME_SERVER)
				if(net.s[sid].whoami==0) {
					plNetDestroySession(&net,sid);
				} else if(net.s[sid].whoami==KClient) {
					kill_player(&net,RTimedOut,sid,0x01);
					//plNetDestroySession(&net,sid);
				} else {
					//Check the peer, and send again a ServerHello message
					int peer;
					peer=net.s[sid].whoami;
					plNetDestroySession(&net,sid);
					plog(f_err,"WAR: Peer %i:%s is not responding (timeout)...\n",\
					peer,unet_get_destination(peer));
					logflush(f_err);
					reconnect2peer(&net,peer);
				}
#elif defined(I_AM_THE_TRACKING_SERVER) || defined(I_AM_THE_VAULT_SERVER) || defined(I_AM_THE_AUTH_SERVER)
				if(net.s[sid].whoami==0) {
					plNetDestroySession(&net,sid);
				} else {
					//plNetMsgTerminated(&net,RTimedOut,sid);
					plNetDestroySession(&net,sid);
				}
#elif defined(I_AM_THE_META_SERVER)
				plNetDestroySession(&net,sid); //destroy in silence
#else
				plNetMsgTerminated(&net,RTimedOut,sid);
				plNetDestroySession(&net,sid);
#endif
				break;
			//case UNET_OK:
			default:
				//netcore idling..
				//ignore this event
				//Idle stuff
				//plog(f_uru,"Netcore is idle...\n");
				#if defined(I_AM_A_GAME_SERVER)
				static U32 timer=0;
				static int left=0;
				int players=0;
				if(!persistent) {
					for(i=0; i<(int)net.n; i++) {
						if(net.s[i].whoami==KClient) { players=1; break; }
					}
					if(players!=0 || timer==0) {
						if(timer!=0 && net.timestamp-timer>=4) {
							reconnect2peer(&net,KTracking);
							left=0;
						}
						timer=net.timestamp;
					}
					else if((net.timestamp-timer)>=4) {
							if(left==0) {
								plNetMsgLeave(&net,RQuitting,net.tracking);
								left=1;
							}
							if((net.timestamp-timer)>=8) {
								__state_running=0;
								plog(f_uru,"Since I'm alone, and sad, I'm going to kill myself\n");
							}
					}
				}
				python_subsys_idle_operation();
				#elif defined(I_AM_THE_AUTH_SERVER)
				auth_idle_operation();
				#elif defined(I_AM_THE_VAULT_SERVER)
				vault_server_subsys_idle_operation();
				#elif defined(I_AM_THE_TRACKING_SERVER)
				tracking_subsys_idle_operation(&net);
				#endif
				break;
		}


#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
		//check for sending alive to the server peers
		int peer;
		peer=plNetServerSearch(&net,KAuth);
		if(peer!=-1 && (net.timestamp-net.s[peer].alive_stamp)>(net.s[peer].timeout/2)) {
			net.s[peer].alive_stamp=net.timestamp;
			plNetMsgAlive(&net,peer);
		}
		peer=plNetServerSearch(&net,KVault);
		if(peer!=-1 && (net.timestamp-net.s[peer].alive_stamp)>(net.s[peer].timeout/2)) {
			net.s[peer].alive_stamp=net.timestamp;
			plNetMsgAlive(&net,peer);
		}
		peer=plNetServerSearch(&net,KTracking);
		if(peer!=-1 && (net.timestamp-net.s[peer].alive_stamp)>(net.s[peer].timeout/2)) {
			net.s[peer].alive_stamp=net.timestamp;
			plNetMsgAlive(&net,peer);
		}
#endif
		DBG(5,"step 4\n");
	} //main gameserver loop
	DBG(5,"ending game server loop\n");

	kill_players(&net,1);

	for(i=0; i<(int)net.n; i++) {
		if(net.s[i].ip!=0 && net.s[i].whoami!=0) {
			nlog(f_uru,&net,i,"Leaving... disconnecting peer %i: %i %s\n",i,net.s[i].whoami,\
			unet_get_destination(net.s[i].whoami));
			plNetMsgLeave(&net,RQuitting,i);
		}
	}

	//stop the different subsystems (In the correct order)
#if defined(I_AM_THE_AUTH_SERVER)
	stop_auth_driver();
#endif

#if defined(I_AM_THE_VAULT_SERVER)
	stop_vault_server_subsys();
	stop_vault_subsys();
#endif

#if defined(I_AM_A_GAME_SERVER)
	stop_python_subsys();
	stop_game_subsys();
#endif

#if defined(I_AM_A_GAME_SERVER) || defined(I_AM_A_LOBBY_SERVER)
	stop_vault_subsys();
	stop_lobby_subsys();
#endif

#if defined(I_AM_THE_TRACKING_SERVER)
	stop_tracking_subsys();
#endif

	plNetStopOp(&net);
	cnf_destroy(&global_config); //destroy the configuration struct
	print2log(f_uru,"INF: SERVICE Sanely TERMINATED\n\n");
	log_shutdown();

#ifdef __WIN32__
	SetConsoleCtrlHandler(_windows_control_handler, false);
#endif

	return 0;
}

