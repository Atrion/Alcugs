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

/**
	Alcugs Lib Main code.
*/

#ifndef __U_ALCMAIN_H
#define __U_ALCMAIN_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCMAIN_H_ID "$Id$"

#include "alcutil/alclog.h"
#include "alcutil/alccfgtypes.h"

namespace alc {

//! Global library management class - always create exactly one instance of this one!
class tAlcMain {
	friend class tLog; // tLog has to acces the log config
public:
	tAlcMain(const tString &appName); //!< Run this directly in main(), not in a try...catch - it will deal with that itself
	virtual ~tAlcMain(void);
	
	const tTime &bornTime(void) { return born; }
	tTime upTime(void);
	uint64_t threadId(void) { return mainThreadId; }
	tConfig *config(void); //!< returns the configuration manager - must only be used in main thread!
	tString name(void) { return appName; }
	
	tLog *std() { return stdLog; }
	tLog *err() { return errLog; }
	
	// some global events
	virtual void applyConfig(); //!< applies the previously loaded config to all submodules. This enables file-logging per default! Call after you set up the configuration
	virtual void onCrash(void);
	virtual bool onSignal(int s); //!< returns whether the signal has been handled (true) or still needs handling (false)

protected:
	void installHandler(int signum, bool install = true);
	void loadConfig(const tString & path);
	
	tConfig cfg;
	tLog *stdLog, *errLog;
private:
	void installBaseHandlers(bool install = true);
	void dumpConfig();
	
	uint64_t mainThreadId;
	tTime born;
	tLogConfig logCfg;
	tString appName;
	
	FORBID_CLASS_COPY(tAlcMain)
};

//! Get global management class
tAlcMain *alcGetMain(void);

}

#endif
