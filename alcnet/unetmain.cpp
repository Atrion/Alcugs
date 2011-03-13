/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

/* CVS tag - DON'T TOUCH*/
#define __U_UNETMAIN_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "unetmain.h"

#include "netexception.h"
#include "unetbase.h"

#include <csignal>
#include <unistd.h>
#include <cassert>


namespace alc {


tAlcUnetMain::tAlcUnetMain(const tString &netName) : tAlcMain("Alcugs "+netName+" Server"),
    stateRunning(2), alarmRunning(false), netName(netName), net(NULL)
{
	installUnetHandlers(true);
}

tAlcUnetMain::~tAlcUnetMain(void)
{
	if (net) throw txUnet(_WHERE("the tUnetBase instance must be destroyed before the tAlcUnetMain instance"));
	installUnetHandlers(false);
}

void tAlcUnetMain::setNet(tUnetBase *netcore)
{
	if (net && netcore) throw txBase(_WHERE("You can NEVER have several instances of tUnetBase around!"));
	net = netcore;
}

void tAlcUnetMain::installUnetHandlers(bool install)
{
	installHandler(SIGTERM,install);
	installHandler(SIGINT,install);
	installHandler(SIGHUP,install);
	installHandler(SIGUSR1,install);
	installHandler(SIGUSR2,install);
}

bool tAlcUnetMain::onSignal(int s) {
	if (tAlcMain::onSignal(s)) return true; // done
	if (!net) return false;
	try {
		switch (s) {
			case SIGHUP: //reload configuration
				assert(alcGetSelfThreadId() == threadId()); // should only occur in main thread
				stdLog->log("INF: Re-reading configuration\n\n");
				net->workerThread.stop(); // make sure we can safely modify the config
				applyConfig();
				net->workerThread.spawn();
				return true;
			case SIGALRM:
			case SIGTERM:
			case SIGINT:
				assert(alcGetSelfThreadId() == threadId()); // should only occur in main thread
				switch(stateRunning) {
					case 2:
						net->stop();
						stateRunning--;
						return true;
					case 1:
						net->forcestop();
						stateRunning--;
						stdLog->log("INF: Warning another CTRL+C will kill the server permanently causing data loss\n");
						return true;
					default:
						stdLog->log("INF: Killed\n");
						printf("Killed!\n");
						exit(0);
				}
				break;
			case SIGUSR1:
				assert(alcGetSelfThreadId() == threadId()); // should only occur in main thread
				if(alarmRunning) {
					stdLog->log("INF: Automatic -Emergency- Shutdown CANCELLED\n\n");
					alarmRunning=false;
					installHandler(SIGALRM,false);
					alarm(0);
				} else {
					stdLog->log("INF: Automatic -Emergency- Shutdown In progress in 30 seconds\n\n");
					alarmRunning=true;
					installHandler(SIGALRM,true);
					alarm(30);
				}
				return true;
			case SIGUSR2:
				assert(alcGetSelfThreadId() == threadId()); // should only occur in main thread
				stdLog->log("INF: TERMINATED message sent to all players.\n\n");
				net->terminatePlayers();
				return true;
		}
	} catch(txBase &t) {
		errLog->log("FATAL Exception %s\n%s\n",t.what(),t.backtrace()); return true;
	} catch(...) {
		errLog->log("FATAL Unknown Exception\n"); return true;
	}
	return false;
}

void tAlcUnetMain::applyConfig()
{
	assert(alcGetSelfThreadId() == threadId()); // should only occur in main thread
	//Load and parse config files
	tString var;
	var=cfg.getVar("read_config","cmdline");
	if(!var.isEmpty()) {
		loadConfig(var);
	}
	// put the current server into "global"
	cfg.copyKey("global",netName);
	cfg.copyKey("global","cmdline");
	// apply configuration
	tAlcMain::applyConfig();
	if (net) net->applyConfig(); // trigger the apply process in the netcore
}

} //end namespace alc

