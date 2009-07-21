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

namespace alc {

static volatile bool alcInitialized=false;
tConfig * alcGlobalConfig=NULL;
bool alcIngoreParseErrors=false;
tTime alcBorn;
tSignalHandler * alcSignalHandler=NULL;
U32 alcMainThreadId=0;


U32 alcGetMainThreadId() {
	return alcMainThreadId;
}

void alcInit(int argc,char ** argv,bool shutup) {
	if(alcInitialized) return;
	alcMainThreadId=alcGetSelfThreadId();
	alcInitialized=true;
	DBG(5,"alcInit()\n");
	alcBorn.now();
	DBG(6,"Starting log system...");
	alcLogInit();
	DBGM(6," done\n");
	alcLogOpenStdLogs(shutup);
	if(!alcVerifyVersion()) {
		lerr->log("ERR: Alcugs Library version mismatch! %s!=%s\n",alcSTR_VER,alcVerifyVersionStr);
		_DIE("ERR: Alcugs Library version mismatch");
	}

	//init entropy
	srandom(alcGetMicroseconds() + (alcGetTime() % 10000));
	
	//init config system
	delete alcGlobalConfig;
	alcGlobalConfig = new tConfig();
	tSignalHandler * h = new tSignalHandler();
	DBG(7,"Installing default signal handler...");
	alcInstallSignalHandler(h);
	DBGM(7," done\n");
	
	atexit(&alcShutdown);
}

void alcShutdown() {
	if(!alcInitialized) return;
	alcInitialized=false;
	DBG(5,"alcShutdown()\n");
	DBG(5,"Stopping log system...");
	alcLogShutdown();
	DBGM(5," done\n");
	delete alcGlobalConfig;
	alcGlobalConfig=NULL;
	//the last thing
	alcInstallSignalHandler(NULL);
}

void alcOnFork() {
	DBG(5,"alcLogShutdown from a forked child...\n");
	alcLogShutdown(true);
}

tConfig * alcGetConfig() {
	return alcGlobalConfig;
}

tTime alcGetUptime() {
	tTime now;
	now.now();
	return now-alcBorn;
}

tTime alcGetBornTime() {
	return alcBorn;
}

void alcReApplyConfig() {
	tStrBuf var;
	tConfig * cfg=alcGetConfig();
	var=cfg->getVar("verbose_level","global");
	if(var.isNull()) {
		var="3";
	}
	alcLogSetLogLevel(var.asByte());
	var=cfg->getVar("log_files_path","global");
	if(var.isNull()) {
		var="log/";
	}
	alcLogSetLogPath(var);
	var=cfg->getVar("log.n_rotate","global");
	if(var.isNull()) {
		var="5";
	}
	alcLogSetFiles2Rotate(var.asByte());
	var=cfg->getVar("log.enabled","global");
	if(var.isNull()) {
		var="1";
	}
	alcLogOpenStdLogs(!var.asByte());
	var=cfg->getVar("system.segfault_handler","global");
	if(!var.isNull() && !var.asByte()) {
		alcSignal(SIGSEGV,0);
	} else {
		alcSignal(SIGSEGV,1);
	}
	#ifndef __WIN32__
	alcSignal(SIGCHLD,1);
	#endif
}

void alcIngoreConfigParseErrors(bool val) {
	alcIngoreParseErrors=val;
}

void alcDumpConfig() {
	tXParser parser;
	parser.setConfig(alcGetConfig());
	tStrBuf out;
	out.put(parser);
	lstd->print("Config Dump:\n%s\n",out.c_str());
}

bool alcParseConfig(const tStrBuf & path) {
	tXParser parser;
	DBG(5,"setting config parser...");
	parser.setConfig(alcGetConfig());
	DBGM(5," done\n");
	#if defined(__WIN32__) or defined(__CYGWIN__)
	path.convertSlashesFromWinToUnix();
	#endif
	parser.setBasePath(path.dirname());
	tFBuf f1;
	
	bool ok=true;
	
	try {
		f1.open(path.c_str());
		f1.rewind();
		DBG(5,"f1.get(parser)...\n");
		f1.get(parser);
		DBGM(5," done\n");
		f1.close();
		
		//tStrBuf out;
		//out.put(parser);
	} catch(txNotFound &t) {
		fprintf(stderr,"Error: %s\n",t.what());
		ok=false;
	}
	
	if(alcIngoreParseErrors) {
		ok=true;
	}
	
	return ok;
}

void alcInstallSignalHandler(tSignalHandler * t) {
	delete alcSignalHandler;
	alcSignalHandler = t;
}

void _alcHandleSignal(int s) {
	if(alcSignalHandler!=NULL)
		alcSignalHandler->handle_signal(s);
}

void alcSignal(int signum, bool install) {
	DBG(5,"alcSignal()\n");
	if(alcSignalHandler==NULL) return;
	DBG(5,"%i - %i\n",signum,install);
	if(install) {
		signal(signum,_alcHandleSignal);
	} else {
		signal(signum,SIG_DFL);
	}
}

void alcCrashAction() {
	tStrBuf var;
	tConfig * cfg=alcGetConfig();
	var=cfg->getVar("crash.action","global");
	if(!var.isNull()) {
		system(var.c_str());
	}
}

void tSignalHandler::handle_signal(int s) {
	lstd->log("INF: Recieved signal %i\n",s);
	try {
		switch (s) {
#ifndef __WIN32__
			case SIGCHLD:
				lstd->log("INF: RECIEVED SIGCHLD: a child has exited.\n");
				lstd->flush();
				alcSignal(SIGCHLD);
				wait(NULL); // properly exit child
				break;
#endif
			case SIGSEGV:
				lerr->log("\n PANIC!!!\n");
				lerr->log("TERRIBLE FATAL ERROR: SIGSEGV recieved!!!\n\n");
				lerr->flush();
				//On windows, the signal handler runs on another thread
				#ifndef __WIN32__
				if(alcGetSelfThreadId()!=alcGetMainThreadId()) return;
				#endif
				throw txBase("Panic: Segmentation Fault - dumping core",1,1);
		}
	} catch(txBase &t) {
		lerr->log("FATAL Exception %s\n%s\n",t.what(),t.backtrace());
	} catch(...) {
		lerr->log("FATAL Unknown Exception\n");
	}
}

} //end namespace
