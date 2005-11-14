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

/**
	Alcugs Lib Main code
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCMAIN_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include <cstdlib>
namespace std {
#include <signal.h>
};

#include "alcdebug.h"

namespace alc {

static volatile bool alcInitialized=false;
tConfig * alcGlobalConfig=NULL;
bool alcIngoreParseErrors=false;

void alcInit(int argc,char ** argv,bool shutup) {
	if(alcInitialized) return;
	alcInitialized=true;
	DBG(5,"alcInit()\n");
	
	DBG(6,"Starting log system...\n");
	alcLogInit();
	alcLogOpenStdLogs(shutup);
	if(!alcVerifyVersion()) {
		lerr->log("ERR: Alcugs Library version mismatch! %s!=%s\n",alcSTR_VER,alcVerifyVersionStr);
		_DIE("ERR: Alcugs Library version mismatch");
	}

	//init entropy
	srandom(alcGetMicroseconds() + (alcGetTime() % 10000));
	
	//init config system
	if(alcGlobalConfig!=NULL) {
		delete alcGlobalConfig;
	}
	alcGlobalConfig = new tConfig();
	
	atexit(&alcShutdown);
}

void alcShutdown() {
	if(!alcInitialized) return;
	alcInitialized=false;
	DBG(5,"alcShutdown()\n");
	
	DBG(5,"Stopping log system...\n");
	alcLogShutdown();

	if(alcGlobalConfig!=NULL) {
		delete alcGlobalConfig;
		alcGlobalConfig=NULL;
	}
}

void alcOnFork() {

	DBG(5,"alcLogShutdown from a forked child...\n");
	alcLogShutdown(true);

}

void alcSignal(int signum, void (*handler)(int)) {
	#ifdef __CYGWIN__
	signal(signum,handler);
	#else
	std::signal(signum,handler);
	#endif
}

tConfig * alcGetConfig() {
	return alcGlobalConfig;
}

void alcIngoreConfigParseErrors(bool val) {
	alcIngoreParseErrors=val;
}

bool alcParseConfig(tStrBuf & path) {
	std::printf("parsing %s...\n",path.c_str());

	tXParser parser;
	parser.setConfig(alcGetConfig());
	dmalloc_verify(NULL);
	#if defined(__WIN32__) or defined(__CYGWIN__)
	path.convertSlashesFromWinToUnix();
	#endif
	parser.setBasePath(path.dirname());
	tFBuf f1;
	
	bool ok=true;
	
	try {
		f1.open(path.c_str());
		f1.rewind();
		f1.get(parser);
		f1.close();
		
		//tStrBuf out;
		//out.put(parser);
		//std::printf("result \n%s\n",out.c_str());
	} catch(txNotFound &t) {
		std::fprintf(stderr,"Error: %s\n",t.what());
		ok=false;
	}
	
	if(alcIngoreParseErrors) {
		ok=true;
	}
	
	return ok;
}

}
