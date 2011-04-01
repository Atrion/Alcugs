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
	Alcugs exception Handler.
*/

//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "alcexception.h"

#include "alcmain.h"
#include "alcversion.h"

#define __STDC_FORMAT_MACROS
#include <cassert>
#include <unistd.h>
#include <ctime>
#include <cstdlib>
#include <inttypes.h>
#include <dlfcn.h>
#include <execinfo.h>
#include <cxxabi.h>


namespace alc {

//Exceptions

//Base class
void txBase::storeBacktrace()
{
	size=::backtrace(bt,txExcLevels);
}

static tString getReadableAddress(void *address)
{
	// what we can always do: just print the plain address
	tString result;
	result.printf("%p", address);
	// get library information
	Dl_info info;
	dladdr( address, &info );
	assert(static_cast<uint8_t*>(address) >= static_cast<uint8_t*>(info.dli_fbase));
	
	// create addr2line query
	char buf[1024];
	tString filename = info.dli_fname[0] == '/' ? info.dli_fname : ("$(which "+tString(info.dli_fname)+")"); // get a WD-independant name
	bool sharedLib = filename.find(".so") != npos || filename.find(".sl") != npos; // the base program does not get the offset substracted, so detect it
	long unsigned int offset = sharedLib ? static_cast<uint8_t*>(address)-static_cast<uint8_t*>(info.dli_fbase) : reinterpret_cast<long unsigned int>(address);
	snprintf(buf, 1023, "addr2line -e %s 0x%lX", filename.c_str(), offset);
	
	// get addr2line answer
	FILE *pf = popen(buf, "r");
	if (!pf) return result;
	if (fgets(buf, 1023, pf)) {
		if (strlen(buf) > 2 && strncmp(buf, "??", 2) != 0) { // a usable address :)
			result = tString(buf).stripped('\n');
		}
	}
	pclose(pf);
	return result;
}
static tString getReadableFunctionName(tString symbol)
{
	// get mangled function name, if existing
	size_t endFunction = symbol.find(' ');
	if (endFunction == npos) return symbol; // this is really weird, should never happen
	symbol = symbol.substring(0, endFunction);
	size_t beginName = symbol.find('(');
	size_t endName = symbol.find('+');
	if (beginName == npos ||endName == npos || endName <= beginName+1) return symbol; // it could not find the function name
	symbol = symbol.substring(beginName+1, endName-beginName-1);
	
	// demangle function name
	int status;
	char *buf = abi::__cxa_demangle(symbol.c_str(), NULL, NULL, &status);
	if (status == 0) {
		// hooray, we got a demangled name!
		symbol = buf;
	}
	free(buf);
	return symbol;
}
tString txBase::printBacktrace(void *const* bt, unsigned int size) {
	alcGetMain()->installSigchildHandler(false);
	//get the backtrace
	char **btStrings=backtrace_symbols(bt,size);
	
	tString btPrinted;
	btPrinted.printf("Backtrace with %u levels:\n",size);
	for(size_t i=0; i<size; i++) {
		btPrinted.printf("#%02Zd  %s at %s\n", i, getReadableFunctionName(btStrings[i]).c_str(), getReadableAddress(bt[i]).c_str());
	}
	free(btStrings);
	
	// restore signal handling
	alcGetMain()->installSigchildHandler();
	return btPrinted;
}

void txBase::dump() const {
	fprintf(stderr,"Exception %s:\n%s\n",what(),backtrace());

	tString filename;
	filename.printf("BackTrace-%06i-%08X.txt",getpid(),static_cast<uint32_t>(time(NULL)));
	FILE * f=NULL;
	f=fopen(filename.c_str(),"w");
	if(f!=NULL) {
		fprintf(f,"Servers Build info:\n%s\n",alcVersionTextShort());
		fprintf(f,"System info: %s\n\n",alcSystemInfo());
		fprintf(f,"Born:    %s\n",alcGetMain()->bornTime().str().c_str());
		tTime now = tTime::now();
		fprintf(f,"Defunct: %s\n",now.str().c_str());
		fprintf(f,"Uptime:  %s\n",alcGetMain()->upTime().str(/*relative*/true).c_str());
		fprintf(f,"Main thread id: %"PRIu64"\n",alcGetMain()->threadId());
		fprintf(f,"This thread id: %"PRIu64"\n",static_cast<uint64_t>(alcGetSelfThreadId()));
		fprintf(f,"Exception %s:\n%s\n",what(),backtrace());
		fclose(f);
	}
}
//End base

//End Exceptions

} //End alc namespace

