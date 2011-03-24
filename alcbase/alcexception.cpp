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

/* CVS tag - DON'T TOUCH*/
#define __U_ALCEXCEPTION_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "alcexception.h"

#include "alcmain.h"
#include "alcversion.h"

#define __STDC_FORMAT_MACROS
#include <unistd.h>
#include <ctime>
#include <cstdlib>
#include <inttypes.h>

#if defined(HAVE_EXECINFO_H)
	#include <execinfo.h>
#endif


namespace alc {
	
	
const int txExcLevels = 20;


//Exceptions

//Base class
txBase::txBase(const tString &msg,bool abort) : msg(msg) {
	this->abort=abort;
	this->_preparebacktrace();
}
txBase::txBase(const tString &name,const tString &msg,bool abort) {
	this->msg = name + ": " + msg;
	this->abort=abort;
	this->_preparebacktrace();
}
txBase::txBase(const txBase &t) {
	copy(t);
}
void txBase::copy(const txBase &t) {
	DBG(5,"copy\n");
	this->abort=t.abort;
	this->msg=t.msg;
	this->bt=t.bt;
}
void txBase::_preparebacktrace() {
// This needs porting - This code only works under Linux (it's part of the libc)
#ifdef HAVE_EXECINFO_H
	//get the backtrace
	void * btArray[txExcLevels];
	char **strings;
	unsigned int size=::backtrace(btArray,txExcLevels);
	strings=backtrace_symbols(btArray,size);
	
	/*
	int f;
	f=creat("backtrace.txt",0640);
	if(f!=-1) {
		std::backtrace_symbols_fd(btArray,size,f);
		close(f);
	}*/
	
	bt.clear();
	
	bt.printf("Backtrace with %u levels:\n",size);
	for(size_t i=0; i<size; i++) {
		bt.printf("  %s\n", strings[i]);
	}
	bt += "c++filt and addr2line may be useful\n";
	free(strings);
#else
	bt = "Backtrace not implemented in your OS, or execinfo.h not found\n";
#endif
	if(this->abort) {
		dump();
		alcGetMain()->onCrash();
		fprintf(stderr,"An exception requested to abort\n");
		::abort();
	}
}
void txBase::dump(bool toStderr) {
	if (toStderr)
		fprintf(stderr,"Exception %s:\n%s\n",msg.c_str(),bt.c_str());

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
		fprintf(f,"Exception %s:\n%s\n",msg.c_str(),bt.c_str());
		fclose(f);
	}
}
//End base

//End Exceptions

} //End alc namespace

