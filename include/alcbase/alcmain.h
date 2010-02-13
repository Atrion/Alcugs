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

extern const char * alcNAME;
extern const char * alcBUILD;
extern const char * alcVERSION;
extern const char * alcID;

namespace alc {

//! Global library management class - always create exactly one instance of this one!
class tAlcMain {
	friend class tLog; // tLog has to acces the log config
public:
	tAlcMain(void); //!< Run this directly in main(), not in a try...catch - it will deal with that itself
	virtual ~tAlcMain(void);
	
	inline tTime bornTime(void) { return born; }
	tTime upTime(void);
	inline U32 threadId(void) { return mainThreadId; }
	inline tConfig *config(void) { return &cfg; }
	void dumpConfig();
	
	inline tLog *std() { return stdLog; }
	inline tLog *err() { return errLog; }
	
	// some global events
	virtual void onApplyConfig(); //!< applies the previously loaded config to all submodules. This enables file-logging per default!
	virtual void onCrash(void);
	virtual void onForked();
	virtual bool onSignal(int s); //!< returns whether the signal has been handled (true) or still needs handling (false)

protected:
	void installHandler(int signum, bool install = true);
	void loadConfig(const tString & path);
	
	tConfig cfg;
	tLog *stdLog, *errLog;
private:
	void installBaseHandlers(bool install = true);
	
	U32 mainThreadId;
	tTime born;
	tLogConfig logCfg;
};

//! Get global management class
tAlcMain *alcGetMain(void);

// Magic to detect version mismatch
#if !defined(IN_ALC_LIBRARY) and defined(IN_ALC_PROGRAM)
bool alcVerifyVersion() {
	return(alcGetMaxVersion()==alcMAX_VER &&
	alcGetMinVersion()==alcMIN_VER &&
	alcGetRelVersion()==alcREL_VER &&
	alcGetBetVersion()==alcBET_VER &&
	alcGetProtocolVersion()==alcPROTO_VER);
}
const char * alcVerifyVersionStr = alcSTR_VER;
#else
extern bool alcVerifyVersion();
extern char * alcVerifyVersionStr;
#endif


}

#endif
