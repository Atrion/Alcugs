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
	Alcugs Lib Main code
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCMAIN_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#ifndef __WIN32__
#include <sys/wait.h>
#endif

#include "alcdebug.h"

// We are using SIG_DFL here which implies an old-style cast
#pragma GCC diagnostic ignored "-Wold-style-cast"

namespace alc {

static tAlcMain *alcMain = NULL; // this is a global singleton

tAlcMain *alcGetMain(void)
{
	return alcMain;
}

static void _alcHandleSignal(int s) {
	if(alcMain != NULL) alcMain->onSignal(s);
}

tAlcMain::tAlcMain(void)
{
	try {
		// global library management
		if (alcMain) _DIE("You can NEVER create several instances of tAlcMain");
		alcMain = this;
		if(!alcVerifyVersion()) _DIE("ERR: Alcugs Library version mismatch")
		
		// initialization
		mainThreadId = alcGetSelfThreadId();
		born.setToNow();
		alcLogInit();
		
		// open logfiles
		stdLog = new tLog(NULL,2,DF_STDOUT);
		errLog = new tLog(NULL,2,DF_STDERR);
		
		//init entropy
		srandom(alcGetMicroseconds() + (alcGetTime() % 10000));
		
		//init signal handlers
		installBaseHandlers();
	}
	catch (...) {
		_DIE("Error initializing Alcugs - can't even do proper error reporting....");
	}
}

tAlcMain::~tAlcMain() {
	installBaseHandlers(/*install*/false);
	delete stdLog;
	delete errLog;
	alcLogShutdown();
	alcMain = NULL;
}

tTime tAlcMain::upTime(void)
{
	tTime now;
	now.setToNow();
	return now-born;
}

void tAlcMain::onApplyConfig() {
	tString var;
	var=cfg.getVar("verbose_level","global");
	if(var.isEmpty()) {
		var="3";
	}
	alcLogSetLogLevel(var.asByte());
	
	var=cfg.getVar("log_files_path","global");
	if(var.isEmpty()) {
		var="log/";
	}
	alcLogSetLogPath(var);
	
	var=cfg.getVar("log.n_rotate","global");
	if(var.isEmpty()) {
		var="5";
	}
	alcLogSetFiles2Rotate(var.asByte());
	
	var=cfg.getVar("log.enabled","global");
	if(var.isEmpty()) {
		var="1";
	}
	if (var.asByte()) {
		stdLog->open("alcugs.log",2,DF_STDOUT);
		errLog->open("error.log",2,DF_STDERR);
	}
	else {
		stdLog->open(NULL,2,DF_STDOUT);
		errLog->open(NULL,2,DF_STDERR);
	}
}

void tAlcMain::dumpConfig() {
	tXParser parser;
	parser.setConfig(&cfg);
	tString out;
	out.put(parser);
	stdLog->print("Config Dump:\n%s\n",out.c_str());
}

void tAlcMain::loadConfig(const tString &path) {
	tXParser parser;
	parser.setConfig(&cfg);
	parser.setBasePath(path.dirname());
	tFBuf f1(path.c_str());
	f1.get(parser);
}

void tAlcMain::onCrash() {
	tString var;
	var=cfg.getVar("crash.action","global");
	if(!var.isEmpty()) {
		system(var.c_str());
	}
}

void tAlcMain::onForked() {
	DBG(5,"alcLogShutdown from a forked child...\n");
	alcLogShutdown(true);
}

void tAlcMain::installBaseHandlers(bool install)
{
	installHandler(SIGSEGV, install);
#ifndef __WIN32__
	installHandler(SIGCHLD, install);
#endif
}

void tAlcMain::installHandler(int signum, bool install) {
	DBG(5,"(un)install signal handler %i - %i\n",signum,install);
	if(install) {
		signal(signum, _alcHandleSignal);
	} else {
		signal(signum, SIG_DFL);
	}
}

bool tAlcMain::onSignal(int s) {
	stdLog->log("INF: Recieved signal %i\n",s);
	installHandler(s);
	#ifndef __WIN32__ // On windows, the signal handler runs on another thread
	if (alcGetSelfThreadId() != mainThreadId) return true;
	#endif
	try {
		switch (s) {
#ifndef __WIN32__
			case SIGCHLD:
				stdLog->log("INF: RECIEVED SIGCHLD: a child has exited.\n");
				stdLog->flush();
				wait(NULL); // properly exit child
				return true;
#endif
			case SIGSEGV:
				errLog->log("\n PANIC!!!\n");
				errLog->log("TERRIBLE FATAL ERROR: SIGSEGV recieved!!!\n\n");
				errLog->flush();
				throw txBase("Panic: Segmentation Fault - dumping core",/*abort*/true,/*dump core*/true);
		}
	} catch(txBase &t) {
		errLog->log("FATAL Exception %s\n%s\n",t.what(),t.backtrace()); return true;
	} catch(...) {
		errLog->log("FATAL Unknown Exception\n"); return true;
	}
	return false;
}

} //end namespace
