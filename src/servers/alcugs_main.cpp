/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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
//#define _DBG_LEVEL_ 10

//Program vars
#define IN_ALC_PROGRAM
#define ALC_PROGRAM_ID "$Id$"
#define ALC_PROGRAM_NAME "Alcugs Server"

#include <alcnet.h>

#if defined(I_AM_THE_LOBBY_SERVER)
#include "lobbyserver.h"
const char *alcNetName = "Lobby";
#elif defined(I_AM_THE_GAME_SERVER)
#include "gameserver.h"
const char *alcNetName = "Game";
#elif defined(I_AM_THE_AUTH_SERVER)
#include "authserver.h"
const char *alcNetName = "Auth";
#elif defined(I_AM_THE_VAULT_SERVER)
#include "vaultserver.h"
const char *alcNetName = "Vault";
#elif defined(I_AM_THE_TRACKING_SERVER)
#include "trackingserver.h"
const char *alcNetName = "Tracking";
/*#elif defined(I_AM_THE_META_SERVER)
#include "metaserver.h"
#elif defined(I_AM_THE_DATA_SERVER)
#include "dataserver.h"
#elif defined(I_AM_THE_ADMIN_SERVER)
#include "adminserver.h"
#elif defined(I_AM_THE_PROXY_SERVER)
#include "proxyserver.h"
#elif defined(I_AM_THE_PLFIRE_SERVER)
#include "plfireserver.h"*/
#else
#error UNKNOWN SERVER
#endif

#include <alcdebug.h>

using namespace alc;

void parameters_usage() {
	printf("Usage: uru_service [-VDhfl] [-v n] [-p 5000] [-c uru.conf] [-name AvatarCustomization] [-guid 0000000000000010] [-log logs] [-bcast 1] [-nokill] [-clean] [-oOption1 value1] [-oOption2 value2] ...\n\n\
 -V: show version and end\n\
 -D: Daemon mode\n\
 -v 3: Set Verbose level\n\
 -h: Show short help and end\n\
 -p 5000: select the listenning port\n\
 -c uru.conf: Set an alternate configuration file\n\
 -guid XXXXXXXX: Set the server guid [game server only]\n\
 -name XXXXX: Set the age filename [game server only]\n\
 -log folder: Set the logging folder\n\
 -bcast 1/0: Enable/disable server broadcasts\n\
 -l: Shows the servers license\n\
 -nokill: Don't kill the gameservers, let them running [game server only].\n\
 -clean: Performs vault cleanup on vault server startup (make a backup of your database first!) [vault server only].\n\
 -oOption value: Sets addional configuration settings, for example -oPort 5000\n\n");
}

/**
  parse arguments, returns -1 if it fails
*/
int u_parse_arguments(int argc, char * argv[]) {
	int i;
	
	tConfig * cfg=alcGetMain()->config();

	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) {
			parameters_usage(); return -1;
		} else if(!strcmp(argv[i],"-V")) {
			puts(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-l")) {
			puts(alcVersionTextShort());
			puts(alcLicenseText());
			return -1;
		} else if(!strcmp(argv[i],"-D")) {
			cfg->setVar("1","daemon","cmdline");
		} else if(!strcmp(argv[i],"-nokill")) {
			cfg->setVar("1","game.persistent","cmdline");
		} else if(!strcmp(argv[i],"-v") && argc>i+1) {
			i++;
			cfg->setVar(argv[i],"verbose_level","cmdline");
		} else if(!strcmp(argv[i],"-p") && argc>i+1) {
			i++;
			cfg->setVar(argv[i],"port","cmdline");
		} else if(!strcmp(argv[i],"-c") && argc>i+1) {
			i++;
			cfg->setVar(argv[i],"read_config","cmdline");
		} else if(!strcmp(argv[i],"-guid") && argc>i+1) {
			i++;
			cfg->setVar(argv[i],"age_guid","cmdline");
		} else if(!strcmp(argv[i],"-name") && argc>i+1) {
			i++;
			cfg->setVar(argv[i],"age_filename","cmdline");
		} else if(!strcmp(argv[i],"-log") && argc>i+1) {
			i++;
			cfg->setVar(argv[i],"log_files_path","cmdline");
		} else if(!strcmp(argv[i],"-bcast") && argc>i+1) {
			i++;
			cfg->setVar(argv[i],"broadcast","cmdline");
		} else if(!strcmp(argv[i],"-clean")) {
			cfg->setVar("1","vault.clean","cmdline");
		} else if(!strncmp(argv[i],"-o",2) && argc>i+1) {
			tString opt(argv[i]);
			i++;
			tString val(argv[i]);
			opt=opt.substring(2,opt.size()-2);
			cfg->setVar(val.c_str(),opt.c_str(),"cmdline");
		} else if(!strcmp(argv[i],"-L")) {
			// we don't yet have the logging system
			printf("The \"-L\" option is no longer supported and will be ignored.\n");
		} else {
			parameters_usage();
			return -1;
		}
	}
	return 0;
}

/** apply compatbility aliases */
void setConfigAliases(tConfig *cfg) {
	tString val;
	// NOTE: when copying a value and both source and destination exist, the source is used!
	
	// save the auth, tracking and vault host and port in global: [auth/tracking/vault] and global: [auth/tracking/vault].port
	// use bind and port in the corresponding section
	cfg->copyValue("auth","bind","global","auth");
	cfg->copyValue("vault","bind","global","vault");
	cfg->copyValue("tracking","bind","global","tracking");
	cfg->copyValue("auth.port","port","global","auth");
	cfg->copyValue("vault.port","port","global","vault");
	cfg->copyValue("tracking.port","port","global","tracking");
	// if available, prefer global: [auth/tracking/vault]_server and global: [auth/tracking/vault]_server_port
	cfg->copyValue("auth","auth_server","global","global");
	cfg->copyValue("vault","vault_server","global","global");
	cfg->copyValue("tracking","tracking_server","global","global");
	cfg->copyValue("auth.port","auth_server_port","global","global");
	cfg->copyValue("vault.port","vault_server_port","global","global");
	cfg->copyValue("tracking.port","tracking_server_port","global","global");

	val=cfg->getVar("bandwidth","global");
	if(!val.isEmpty()) {
		val=cfg->getVar("net.up","global");
		if(val.isEmpty()) {
			cfg->copyValue("net.up","bandwidth","global","global");
		}
		val=cfg->getVar("net.down","global");
		if(val.isEmpty()) {
			cfg->copyValue("net.down","bandwidth","global","global");
		}
	}
	
	// Everything below here is legacy settings support!

	//database
	cfg->copyValue("db.host","db_server","global","global");
	cfg->copyValue("db.name","db_name","global","global");
	cfg->copyValue("db.username","db_username","global","global");
	cfg->copyValue("db.passwd","db_passwd","global","global");
	cfg->copyValue("db.passwd","db_password","global","global");
	cfg->copyValue("db.passwd","db.password","global","global");
	cfg->copyValue("db.port","db_port","global","global");

	//unet
	cfg->copyValue("net.timeout","connection_timeout","global","global");
	cfg->copyValue("net.maxconnections","max_clients","global","global");

	//vault
	cfg->copyValue("vault.hood.name","neighborhood_name","global","global");
	cfg->copyValue("vault.hood.desc","neighborhood_comment","global","global");

	//game
	bool found;
	cfg->getVar("game.tmp.hacks.resetting_ages", "global", &found);
	if (!found) { // don't overwrite the new value if it exists
		cfg->copyValue("game.tmp.hacks.resetting_ages", "tracking.tmp.hacks.resetting_ages", "global", "global");
		cfg->copyValue("game.tmp.hacks.resetting_ages", "tracking.tmp.hacks.resetting_ages", "global", "tracking");
	}
}

int main(int argc, char * argv[]) {
	// initialize
	tAlcUnetMain alcMain(alcNetName);
	try {
		//Parse command line
		DBG(5,"parsing cmd...");
		if (u_parse_arguments(argc,argv)!=0) return -1;
		DBGM(5," done\n");
		DBG(5,"loading config...\n");
		//Load and parse config files
		alcMain.loadUnetConfig();
		setConfigAliases(alcMain.config());
		DBGM(5," done\n");
		
		// apply server settings
		//stopped?
		tConfig * cfg=alcMain.config();
		tString var;
		var=cfg->getVar("stop","global");
		if(!var.isEmpty() && var.asByte()) {
			lstd->log("INFO: Administratively disabled! Please read the Alcugs configuration file (usually uru.conf) carefully to solve this issue.\n");
			return -1;
		}
		//daemon?
		var=cfg->getVar("daemon","global");
		if(var.isEmpty()) {
			var="0";
			cfg->setVar("0","daemon","global");
		}
		if(var.asByte()) {
			daemon(1,0);
		}
		
		// print basic version info
		lstd->print(alcVersionText());
		var=alcNetName;
		lstd->print("<%s SERVER>\n",var.upper().c_str());
		lstd->nl();
		lstd->log("The Server is running...\n Press CTRL+C to kill the server.\n\n");
		
		//Create the server
		#if defined(I_AM_THE_LOBBY_SERVER)
		tUnetLobbyServer * service;
		service = new tUnetLobbyServer();
		#elif defined(I_AM_THE_GAME_SERVER)
		tUnetGameServer * service;
		service = new tUnetGameServer();
		#elif defined(I_AM_THE_AUTH_SERVER)
		tUnetAuthServer * service;
		service = new tUnetAuthServer();
		#elif defined(I_AM_THE_VAULT_SERVER)
		tUnetVaultServer * service;
		service = new tUnetVaultServer();
		#elif defined(I_AM_THE_TRACKING_SERVER)
		tUnetTrackingServer * service;
		service = new tUnetTrackingServer();
		/*#elif defined(I_AM_THE_META_SERVER)
		tUnetMetaServer * service;
		service = new tUnetMetaServer();
		#elif defined(I_AM_THE_DATA_SERVER)
		tUnetDataServer * service;
		service = new tUnetDataServer();
		#elif defined(I_AM_THE_ADMIN_SERVER)
		tUnetAdminServer * service;
		service = new tUnetAdminServer();
		#elif defined(I_AM_THE_PROXY_SERVER)
		tUnetProxyServer * service;
		service = new tUnetProxyServer();
		#elif defined(I_AM_THE_PLFIRE_SERVER)
		tUnetPlFireServer * service;
		service = new tUnetPlFireServer();*/
		#else
		#error UNKNOWN SERVER
		#endif
		
		//run the server
		service->run();
		
		delete service;
		lstd->print("The service has succesfully terminated\n");
		lstd->print("Born:    %s\n",alcGetMain()->bornTime().str());
		tTime now;
		now.setToNow();
		lstd->print("Defunct: %s\n",now.str());
		lstd->print("Uptime:  %s\n",alcGetMain()->upTime().str(0x01));
		lstd->print("========================================\n");
	} catch(txBase &t) {
		t.dump(false); // don't dump to stderr, we would get the backtrace twice
		lerr->log("FATAL Server died: Exception %s\n%s\n",t.what(),t.backtrace());
		lstd->print("The service has been unexpectely killed!!!\n");
		lstd->print("Born:    %s\n",alcGetMain()->bornTime().str());
		tTime now;
		now.setToNow();
		lstd->print("Defunct: %s\n",now.str());
		lstd->print("Uptime:  %s\n",alcGetMain()->upTime().str(0x01));
		lstd->print("========================================\n");
		alcMain.onCrash();
		return -1;
	} catch(...) {
		lerr->log("FATAL Server died: Unknown Exception\n");
		lstd->print("The service has been unexpectely killed!!!\n");
		lstd->print("Born:    %s\n",alcGetMain()->bornTime().str());
		tTime now;
		now.setToNow();
		lstd->print("Defunct: %s\n",now.str());
		lstd->print("Uptime:  %s\n",alcGetMain()->upTime().str(0x01));
		lstd->print("========================================\n");
		alcMain.onCrash();
		return -1;
	}
	return 0;
}

