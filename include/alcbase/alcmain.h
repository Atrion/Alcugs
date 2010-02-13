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
	Alcugs Lib Main code.
*/

#ifndef __U_ALCMAIN_H
#define __U_ALCMAIN_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCMAIN_H_ID "$Id$"

/* You need to define these vars in your app's (FIXME?) */
extern const char * alcXSNAME;
extern const char * alcXBUILD;
extern const char * alcXVERSION;
extern const char * alcXID;

namespace alc {

// FIXME this definitely needs a global class... what about tAlcMain?

/** 
		\brief Start Alcugs library.
		\param argc Number of args
		\param argv args
		\param shutup Enable logging subsystem?
*/
void alcInit(bool shutup=false);
/** \brief Stop Alcugs library */
void alcShutdown();
/** \brief Call this after a fork() call */
void alcOnFork();

/** \brief Gets a pointer to the config object */
tConfig * alcGetConfig();

void alcDumpConfig();

tTime alcGetUptime();
tTime alcGetBornTime();

void alcIngoreConfigParseErrors(bool val);

/** \brief Parses the given configuration file */
bool alcParseConfig(const tString & path);
void alcReApplyConfig();

/** \brief Interface for installing signals */
void alcSignal(int signum,bool install=true);

U32 alcGetMainThreadId();

void alcCrashAction();

class tSignalHandler {
public:
	virtual ~tSignalHandler() { }
	virtual void handle_signal(int s);
	virtual void install_handlers(bool install = true);
};

void alcInstallSignalHandler(tSignalHandler * t);

#if !defined(IN_ALC_LIBRARY) and defined(IN_ALC_PROGRAM)
bool alcVerifyVersion() {
	return(alcGetMaxVersion()==alcMAX_VER &&
	alcGetMinVersion()==alcMIN_VER &&
	alcGetRelVersion()==alcREL_VER &&
	alcGetBetVersion()==alcBET_VER &&
	alcGetProtocolVersion()==alcProtoVer);
}
const char * alcVerifyVersionStr = alcSTR_VER;
#else
extern bool alcVerifyVersion();
extern char * alcVerifyVersionStr;
#endif


}

#endif
