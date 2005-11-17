/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs H'uru Server Team                     *
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
#define _DBG_LEVEL_ 10

//Program vars
#define IN_ALC_PROGRAM
#define ALC_PROGRAM_ID "$Id$"
#define ALC_PROGRAM_NAME "Alcugs server"

#include<alcugs.h>
#include<urunet/unet.h>

#if defined(I_AM_THE_LOBBY_SERVER)
#include "lobbyserver.h"
#elif defined(I_AM_THE_GAME_SERVER)
#include "gameserver.h"
#elif defined(I_AM_THE_AUTH_SERVER)
#include "authserver.h"
#elif defined(I_AM_THE_VAULT_SERVER)
#include "vaultserver.h"
#elif defined(I_AM_THE_TRACKING_SERVER)
#include "trackingserver.h"
#elif defined(I_AM_THE_META_SERVER)
#include "metaserver.h"
#elif defined(I_AM_THE_DATA_SERVER)
#include "dataserver.h"
#elif defined(I_AM_THE_ADMIN_SERVER)
#include "adminserver.h"
#elif defined(I_AM_THE_PROXY_SERVER)
#include "proxyserver.h"
#elif defined(I_AM_THE_PLFIRE_SERVER)
#include "plfireserver.h"
#else
#error UNKNOWN SERVER
#endif

#include<alcdebug.h>

using namespace alc;

void parameters_usage() {
	printf("Usage: uru_service [-VDhfl] [-v n] [-p 5000] [-c uru.conf] [-name AvatarCustomization] [-guid 0000000000000010] [-log logs] [-bcast 1] [-nokill] [-clean]\n\n\
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
 -clean: Performs vault cleanup on vault server startup [kVault].\n\
 -oOption value: Sets addional configuration settings, for example -oPort 5000\n\n");
}

/**
  parse arguments, returns -1 if it fails
*/
int u_parse_arguments(int argc, char * argv[]) {
	int i;
	
	tConfig * cfg=alcGetConfig();

	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) {
			parameters_usage(); return -1;
		} else if(!strcmp(argv[i],"-V")) {
			printf(alcVersionText());
			return -1;
		} else if(!strcmp(argv[i],"-l")) {
			printf(alcVersionTextShort());
			printf(alcLicenseText());
			return -1;
		} else if(!strcmp(argv[i],"-D")) {
			cfg->setVar("1","daemon","cmdline");
		} else if(!strcmp(argv[i],"-nokill")) {
			cfg->setVar("1","game.persistent","cmdline");
		} else if(!strcmp(argv[i],"-f")) {
			alcIngoreConfigParseErrors(1);
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
			tStrBuf opt=argv[i];
			i++;
			tStrBuf val=argv[i];
			opt=opt.substring(2,opt.size()-2);
			cfg->setVar(val.c_str(),opt.c_str(),"cmdline");
		} else {
			parameters_usage();
			return -1;
		}
	}
	return 0;
}

/*
class s_handler {
public:
	s_handler(tUnetBase * net) {
		this->net=net;
		__state_running=2;
		st_alarm=0;
	}
	void handle_signal(int s) {
		try {
			switch (s) {
				case SIGHUP: //reload configuration
					lstd->log("INF: ReReading configuration\n\n");
					alcSignal(SIGHUP, (void *)this->handle_signal);
					//parse
					tStrBuf var;
					tConfig * cfg=alcGetConfig();
					var=cfg->getVar("read_config","cmdline");
					if(var.isNull()) { var="uru.conf"; }
					if(!alcParseConfig(var)) {
						lerr->log("FATAL, reading configuration, please check the syntax\n");
					}
					//Set config settings
					cfg->copy("global",alcNetName);
					cfg->copy("global","cmdline");
					cfg->setVar(alcNetName,"internal.net.name","global");
					var="";
					var.printf("%i",alcWhoami);
					cfg->setVar(var.c_str(),"whoami","global");
					alcNetSetConfigAliases();
					alcReApplyConfig();
					var=cfg->getVar("cfg.dump","global");
					if(!var.isNull() && var.asByte()) {
						alcDumpConfig();
					}
					if(net!=NULL) { net->reload(); }
					break;
				case SIGALRM:
				case SIGTERM:
				case SIGINT:
					switch(__state_running) {
						case 2:
							alcSignal(s, this->handle_signal);
							net->stop(-1);
							__state_running--;
							break;
						case 1:
							alcSignal(s, this->handle_signal);
							net->forcestop();
							__state_running--;
							lstd->log("INF: Warning another CTRL+C will kill the server permanently causing data loss\n");
							break;
						default:
							lstd->log("INF: Killed\n");
							printf("Killed!\n");
							exit(0);
					}
				case SIGUSR1:
					if(st_alarm) {
						lstd->log("INF: Automatic -Emergency- Shutdown CANCELLED\n\n");
						//net->bcast("NOTICE: The Server Shutdown sequence has been cancelled");
						st_alarm=0;
						alarm(0);
					} else {
						lstd->log("INF: Automatic -Emergency- Shutdown In progress in 30 seconds\n\n");
						//net->bcast("NOTICE: Server is going down in 30 seconds");
						st_alarm=1;
						alcSignal(SIGALRM, this->handle_signal);
						alarm(30);
					}
					alcSignal(SIGUSR1, this->handle_signal);
					break;
				case SIGUSR2:
					lstd->log("INF: TERMINATED message sent to all players.\n\n");
					net->terminateAll();
					alcSignal(SIGUSR2, this->handle_signal);
					break;
				case SIGCHLD:
					lstdl->log("INF: RECIEVED SIGCHLD: a child has exited.\n\n");
					int status;
					waitpid(-1, &status, WNOHANG);
					alcSignal(SIGCHLD, this->handle_signal);
					break;
				case SIGSEGV:
					lerr->log("\n PANIC!!!\n");
					lerr->log("TERRIBLE FATAL ERROR: SIGSEGV recieved!!!\n\n");
					//TODO: generate a Crash report here
					throw txBase("Panic: Segmentation Fault - dumping core",1,1);
			}
		} catch(txBase &t) {
			lerr->log("FATAL Exception %s\n%s\n",t.what(),t.backtrace());
		} catch(...) {
			lerr->log("FATAL Unknown Exception\n");
		}
	}
	void install_handlers() {
		alcSignal(SIGTERM, this->handle_signal);
		alcSignal(SIGINT, this->handle_signal);
#ifndef __WIN32__
		alcSignal(SIGHUP, this->handle_signal);
		alcSignal(SIGUSR1, this->handle_signal);
		alcSignal(SIGUSR2, this->handle_signal);
		alcSignal(SIGCHLD, this->handle_signal);
		tStrBuf var;
		tConfig * cfg=alcGetConfig();
		var=cfg->getVar("system.segfault_handler","global");
		if(!var.isNull() && var.asByte()) {
			alcSignal(SIGSEGV, this->handle_signal);
		}
#endif
	}
private:
	int __state_running;
	tUnetBase * net;
	int st_alarm;
};
*/

int main(int argc, char * argv[]) {
	try {
		//start Alcugs library
		alcInit(argc,argv,true);
		//Parse command line
		if (u_parse_arguments(argc,argv)!=0) return -1;
		
		//Load and parse config files
		if(!alcUnetReloadConfig(true)) return -1;
		
		//daemon?
		tConfig * cfg=alcGetConfig();
		tStrBuf var;
		var=cfg->getVar("daemon","global");
		if(var.isNull()) {
			var="0";
			cfg->setVar("0","daemon","global");
		}
		if(var.asByte()) {
			daemon(1,0);
		}

		lstd->print(alcVersionText());
		var=alcNetName;
		lstd->print("<%s SERVER>\n",var.upper().c_str());
		lstd->nl();
		lstd->log("The Server is running...\nPresh CTRL+C to kill the server.\n\n");

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
		#elif defined(I_AM_THE_META_SERVER)
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
		service = new tUnetPlFireServer();
		#else
		#error UNKNOWN SERVER
		#endif
		
		//run the server
		service->run();
		
		delete service;
		lstd->log("The service has succesfully terminated\n");
		lstd->print("Born:    %s\n",alcGetBornTime().str());
		tTime now;
		now.now();
		lstd->print("Defunct: %s\n",now.str());
		lstd->print("Uptime:  %s\n",alcGetUptime().str(0x01));
		lstd->print("========================================\n");
	} catch(txBase &t) {
		fprintf(stderr,"FATAL Server died: Exception %s\n%s\n",t.what(),t.backtrace());
		lerr->log("FATAL Server died: Exception %s\n%s\n",t.what(),t.backtrace());
		lstd->log("The service has been unexpectely killed!!!\n");
		lstd->print("Born:    %s\n",alcGetBornTime().str());
		tTime now;
		now.now();
		lstd->print("Defunct: %s\n",now.str());
		lstd->print("Uptime:  %s\n",alcGetUptime().str(0x01));
		lstd->print("========================================\n");
		return -1;
	} catch(...) {
		fprintf(stderr,"FATAL Server died: Unknown Exception\n");
		lerr->log("FATAL Server died: Unknown Exception\n");
		lstd->log("The service has been unexpectely killed!!!\n");
		lstd->print("Born:    %s\n",alcGetBornTime().str());
		tTime now;
		now.now();
		lstd->print("Defunct: %s\n",now.str());
		lstd->print("Uptime:  %s\n",alcGetUptime().str(0x01));
		lstd->print("========================================\n");
		return -1;
	}
	return 0;
}

