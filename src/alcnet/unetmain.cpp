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

#include "alcnet.h"

////extra includes

#include <alcdebug.h>


namespace alc {


tAlcUnetMain::tAlcUnetMain(bool globalLogfiles) : tAlcMain(globalLogfiles), stateRunning(2), alarmRunning(false), net(NULL)
{
	installUnetHandlers(true);
}

tAlcUnetMain::~tAlcUnetMain(void)
{
	installUnetHandlers(false);
}

void tAlcUnetMain::installUnetHandlers(bool install)
{
	installHandler(SIGTERM,install);
	installHandler(SIGINT,install);
#ifndef __WIN32__
	installHandler(SIGHUP,install);
	installHandler(SIGUSR1,install);
	installHandler(SIGUSR2,install);
#endif
}

bool tAlcUnetMain::onSignal(int s) {
	if (tAlcMain::onSignal(s)) return true; // done
	if (!net) return false;
	try {
		switch (s) {
			case SIGHUP: //reload configuration
				lstd->log("INF: ReReading configuration\n\n");
				loadUnetConfig();
				net->reload();
				return true;
			case SIGALRM:
			case SIGTERM:
			case SIGINT:
				switch(stateRunning) {
					case 2:
						net->stop();
						stateRunning--;
						return true;
					case 1:
						net->forcestop();
						stateRunning--;
						lstd->log("INF: Warning another CTRL+C will kill the server permanently causing data loss\n");
						return true;
					default:
						lstd->log("INF: Killed\n");
						printf("Killed!\n");
						exit(0);
				}
				break;
			case SIGUSR1:
				if(alarmRunning) {
					lstd->log("INF: Automatic -Emergency- Shutdown CANCELLED\n\n");
					alarmRunning=false;
					installHandler(SIGALRM,false);
					alarm(0);
				} else {
					lstd->log("INF: Automatic -Emergency- Shutdown In progress in 30 seconds\n\n");
					alarmRunning=true;
					installHandler(SIGALRM,true);
					alarm(30);
				}
				return true;
			case SIGUSR2:
				lstd->log("INF: TERMINATED message sent to all players.\n\n");
				net->terminatePlayers();
				return true;
		}
	} catch(txBase &t) {
		lerr->log("FATAL Exception %s\n%s\n",t.what(),t.backtrace()); return true;
	} catch(...) {
		lerr->log("FATAL Unknown Exception\n"); return true;
	}
	return false;
}

void tAlcUnetMain::loadUnetConfig(void) {
	//Load and parse config files
	tString var;
	var=cfg.getVar("read_config","cmdline");
	if(var.isEmpty()) {
		var="uru.conf";
		cfg.setVar(var.c_str(),"read_config","cmdline");
	}
	loadConfig(var);
	//Set config settings
	cfg.copyKey("global",alcNetName);
	cfg.copyKey("global","cmdline");
	DBG(5,"setting config aliases...");
	alcNetSetConfigAliases();
	DBGM(5," done\n");
	DBG(5,"applying config...");
	onApplyConfig();
	DBGM(5," done\n");
	var=cfg.getVar("cfg.dump","global");
	if(!var.isEmpty() && var.asByte()) {
		dumpConfig();
	}
}

} //end namespace alc

