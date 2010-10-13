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
	Alcugs exception Handler.
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCEXCEPTION_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#if !(defined(__WIN32__) or defined(__CYGWIN__)) and defined(HAVE_EXECINFO_H)
//namespace std {
//extern "C" {
#include <execinfo.h>
//}
//}
#endif

#if defined(HAVE_GOOGLE_COREDUMPER_H)
namespace google {
#include <google/coredumper.h>
}
#endif

//alcexception already included in alcugs.h

#include "alcdebug.h"

namespace alc {

static void alcWriteCoreDump(void) {
	DBG(5,"alcWriteCoreDump ");
	#if !(defined(__WIN32__) or defined(__CYGWIN__)) and defined(HAVE_GOOGLE_COREDUMPER_H)
	DBG(5,"is enabled\n");
	
	tString filename;
	filename.printf("core-%06i-%08X.core",getpid(),time(NULL));
	google::WriteCoreDump(filename.c_str());
	#else
	DBG(5,"is not enabled\n");
	#ifdef __WIN32__
	DBG(5,"and you should get a better OS, bacause the one that you are using now, sucks :(\n");
	#endif
	#endif
}


//Exceptions

//Base class
txBase::txBase(const tString &msg,bool abort,bool core) : msg(msg) {
	this->abort=abort;
	this->core=core;
	this->_preparebacktrace();
}
txBase::txBase(const tString &name,const tString &msg,bool abort,bool core) {
	this->msg = name + ": " + msg;
	this->abort=abort;
	this->core=core;
	this->_preparebacktrace();
}
txBase::txBase(const txBase &t) {
	copy(t);
}
void txBase::copy(const txBase &t) {
	DBG(5,"copy\n");
	this->abort=t.abort;
	this->core=t.core;
	this->msg=t.msg;
	this->bt=t.bt;
}
void txBase::_preparebacktrace() {
// This needs porting - This code only works under Linux (it's part of the libc)
#if !(defined(__WIN32__) or defined(__CYGWIN__)) and defined(HAVE_EXECINFO_H)
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
	for(U32 i=0; i<size; i++) {
		bt.printf("  %s\n", strings[i]);
	}
	bt += "c++filt and addr2line may be useful\n";
	free(strings);
#else
	bt = "Backtrace not implemented in your OS\n";
#endif
	if(this->core) { alcWriteCoreDump(); }
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
	filename.printf("BackTrace-%06i-%08X.txt",getpid(),time(NULL));
	FILE * f=NULL;
	f=fopen(filename.c_str(),"w");
	if(f!=NULL) {
		fprintf(f,"Servers Build info:\n%s\n",alcVersionTextShort());
		fprintf(f,"System info: %s\n\n",alcSystemInfo());
		fprintf(f,"Born:    %s\n",alcGetMain()->bornTime().str().c_str());
		tTime now;
		now.setToNow();
		fprintf(f,"Defunct: %s\n",now.str().c_str());
		fprintf(f,"Uptime:  %s\n",alcGetMain()->upTime().str(0x01).c_str());
		fprintf(f,"Main thread id: %d\n",alcGetMain()->threadId());
		fprintf(f,"This thread id: %d\n",alcGetSelfThreadId());
		fprintf(f,"Exception %s:\n%s\n",msg.c_str(),bt.c_str());
		fclose(f);
	}
}
//End base

//End Exceptions

} //End alc namespace
