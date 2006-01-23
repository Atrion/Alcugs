/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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

/**
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_UNETMAIN_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "urunet/unet.h"

////extra includes

#include "alcdebug.h"


namespace alc {

int alcUnetReloadConfig(bool firsttime) {
	//Load and parse config files
	tStrBuf var;
	tConfig * cfg=alcGetConfig();
	var=cfg->getVar("read_config","cmdline");
	if(var.isNull()) {
		var="uru.conf";
		cfg->setVar(var.c_str(),"read_config","cmdline");
	}
	if(!alcParseConfig(var)) {
		if(firsttime) {
			fprintf(stderr,"FATAL, reading configuration, please check the syntax\n");
			return -1;
		} else {
			lerr->log("FATAL, reading configuration, please check the syntax\n");
		}
	}
	//Set config settings
	cfg->copy("global",alcNetName);
	cfg->copy("global","cmdline");
	cfg->setVar(alcNetName,"internal.net.name","global");
	var="";
	var.printf("%i",alcWhoami);
	cfg->setVar(var.c_str(),"whoami","global");
	DBG(5,"setting config aliases...");
	alcNetSetConfigAliases();
	DBGM(5," done\n");
	DBG(5,"applying config...");
	alcReApplyConfig();
	DBGM(5," done\n");
	var=cfg->getVar("cfg.dump","global");
	if(!var.isNull() && var.asByte()) {
		alcDumpConfig();
	}
	return 1;
}

tUnetSignalHandler::tUnetSignalHandler(tUnetBase * netcore) :tSignalHandler() {
	DBG(5,"tUnetSignalHandler()\n");
	this->net=netcore;
	__state_running=2;
	st_alarm=0;
	install_handlers();
}
tUnetSignalHandler::~tUnetSignalHandler() {

}
void tUnetSignalHandler::handle_signal(int s) {
	tSignalHandler::handle_signal(s);
	try {
		switch (s) {
			case SIGHUP: //reload configuration
				if(alcGetSelfThreadId()!=alcGetMainThreadId()) return;
				alcSignal(SIGHUP,1);
				lstd->log("INF: ReReading configuration\n\n");
				alcUnetReloadConfig(false);
				if(net!=NULL) { net->reload(); }
				break;
			case SIGALRM:
			case SIGTERM:
			case SIGINT:
				switch(__state_running) {
					case 2:
						alcSignal(s,1);
						#ifndef __WIN32__
						if(alcGetSelfThreadId()!=alcGetMainThreadId()) return;
						#endif
						net->stop(-1);
						__state_running--;
						break;
					case 1:
						alcSignal(s,1);
						if(alcGetSelfThreadId()!=alcGetMainThreadId()) return;
						net->forcestop();
						__state_running--;
						lstd->log("INF: Warning another CTRL+C will kill the server permanently causing data loss\n");
						break;
					default:
						lstd->log("INF: Killed\n");
						printf("Killed!\n");
						exit(0);
				}
				break;
			case SIGUSR1:
				if(alcGetSelfThreadId()!=alcGetMainThreadId()) return;
				if(st_alarm) {
					lstd->log("INF: Automatic -Emergency- Shutdown CANCELLED\n\n");
					//net->bcast("NOTICE: The Server Shutdown sequence has been cancelled");
					st_alarm=0;
					alarm(0);
				} else {
					lstd->log("INF: Automatic -Emergency- Shutdown In progress in 30 seconds\n\n");
					//net->bcast("NOTICE: Server is going down in 30 seconds");
					st_alarm=1;
					alcSignal(SIGALRM,1);
					alarm(30);
				}
				alcSignal(SIGUSR1,1);
				break;
			case SIGUSR2:
				if(alcGetSelfThreadId()!=alcGetMainThreadId()) return;
				lstd->log("INF: TERMINATED message sent to all players.\n\n");
				net->terminateAll();
				alcSignal(SIGUSR2,1);
				break;
		}
	} catch(txBase &t) {
		lerr->log("FATAL Exception %s\n%s\n",t.what(),t.backtrace());
	} catch(...) {
		lerr->log("FATAL Unknown Exception\n");
	}
}
void tUnetSignalHandler::install_handlers() {
	DBG(5,"installing handlers\n");
	alcSignal(SIGTERM,1);
	alcSignal(SIGINT,1);
#ifndef __WIN32__
	alcSignal(SIGHUP,1);
	alcSignal(SIGUSR1,1);
	alcSignal(SIGUSR2,1);
#endif
}
void tUnetSignalHandler::unistall_handlers() {

}

	
} //end namespace alc

