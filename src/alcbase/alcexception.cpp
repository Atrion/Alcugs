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

static bool txvAbort=0;
static char txvCore=0x01; //0x00 - disabled, 0x01 - enabled, 0x02 -always
static char * txvCorePath=NULL;

void alcWriteCoreDump(const char * name) {
	DBG(5,"alcWriteCoreDump ");
	#if !(defined(__WIN32__) or defined(__CYGWIN__)) and defined(HAVE_GOOGLE_COREDUMPER_H)
	DBG(5,"is enabled\n");
	unsigned int t,pid;
	pid=getpid();
	t=(unsigned int)time(NULL);
	
	int strsize=60;
	if(txvCorePath!=NULL) strsize+=strlen(txvCorePath);
	char * where=(char *)malloc(sizeof(char) * strsize+1);
	if(where) {
		memset(where,0,strsize+1);
		if(txvCorePath!=NULL) sprintf(where,"%s/core-%06i-%08X-%s.core",txvCorePath,pid,t,name);
		else sprintf(where,"core-%06i-%08X-%s.core",pid,t,name);
	
		if(txvCore & 0x01) google::WriteCoreDump(where);
		free((void *)where);
	}
	#else
	DBG(5,"is not enabled\n");
	#ifdef __WIN32__
	DBG(5,"and you should get a better OS, bacause the one that you are using now, sucks :(\n");
	#endif
	#endif
}

void alcSetCoreDumpFlags(char f) {
	txvCore=f;
}
void alcSetCoreDumpPath(char * p) {
	txvCorePath=p;
}
const char * alcGetCoreDumpPath() {
	return txvCorePath;
}
void alcSetAbort(bool c) {
	txvAbort=c;
}


//Exceptions

//Base class
txBase::txBase(const char * msg,bool abort,bool core) {
	this->name=NULL;
	this->abort=abort;
	this->core=core;
	this->msg=msg;
	this->size=0;
	this->bt=NULL;
	this->imsg=NULL;
	this->_preparebacktrace();
}
txBase::txBase(const char * name,const char * msg,bool abort,bool core) {
	this->name=name;
	this->imsg=static_cast<char *>(malloc(sizeof(char) * (strlen(name) + strlen(msg) + 2)));
	if(this->imsg!=NULL) { strcpy(this->imsg,name); strcat(this->imsg,":"); strcat(this->imsg,msg); }
	this->abort=abort;
	this->core=core;
	this->msg=this->imsg;
	this->size=0;
	this->bt=NULL;
	this->_preparebacktrace();
}
txBase::txBase(const txBase &t) {
	copy(t);
}
void txBase::copy(const txBase &t) {
	DBG(5,"copy\n");
	this->name=NULL;
	this->abort=t.abort;
	this->core=t.core;
	this->msg=NULL;
	this->size=t.size;
	this->bt=static_cast<char *>(malloc(sizeof(char) * (strlen(t.bt)+1)));
	strcpy(this->bt,t.bt);
	this->imsg=static_cast<char *>(malloc(sizeof(char) * (strlen(t.imsg)+1)));
	strcpy(this->imsg,t.imsg);
}
void txBase::_preparebacktrace() {
// This needs porting - This code only works under Linux (it's part of the libc)
#if !(defined(__WIN32__) or defined(__CYGWIN__)) and defined(HAVE_EXECINFO_H)
	//get the backtrace
	char **strings;
	size=::backtrace(btArray,txExcLevels);
	strings=backtrace_symbols(btArray,size);
	
	/*
	int f;
	f=creat("backtrace.txt",0640);
	if(f!=-1) {
		std::backtrace_symbols_fd(btArray,size,f);
		close(f);
	}*/
	
	unsigned int i,msize=0;
	
	for(i=0; i<size; i++) {
		msize+=strlen(strings[i])+6;
	}
	msize+=30+50;
	bt=static_cast<char *>(malloc(sizeof(char) * msize));
	if(bt!=NULL) {
		memset(bt,0,msize);
		sprintf(bt,"Backtrace with %u levels:\n",size);
		for(i=0; i<size; i++) {
			strcat(bt," ");
			strcat(bt,strings[i]);
			strcat(bt,"\n");
		}
		strcat(bt,"c++filt and addr2line may be useful\n");
	}
	free(strings);
#else
	bt=static_cast<char *>(malloc(sizeof(char) * 50));
	sprintf(bt,"Backtrace not implemented in your OS\n");
#endif
	if((txvCore & 0x02) || this->core) { alcWriteCoreDump(); }
	if(txvAbort || this->abort) {
		dump();
		alcCrashAction();
		fprintf(stderr,"An exception requested to abort\n");
		::abort();
	}
}
void txBase::dump(bool toStderr) {
	if (toStderr)
		fprintf(stderr,"Exception %s:\n%s\n",this->what(),this->backtrace());

	unsigned int t,pid;
	pid=getpid();
	t=time(NULL);
	int strsize=60;
	if(txvCorePath!=NULL) strsize+=strlen(txvCorePath);
	char * where=static_cast<char *>(malloc(sizeof(char) * strsize+1));
	if(where) {
		memset(where,0,strsize+1);
		if(txvCorePath!=NULL) sprintf(where,"%s/BackTrace-%06i-%08X.txt",txvCorePath,pid,t);
		else sprintf(where,"BackTrace-%06i-%08X.txt",pid,t);
		FILE * f=NULL;
		f=fopen(where,"w");
		if(f!=NULL) {
			fprintf(f,"Servers Build info:\n%s\n",alcVersionTextShort());
			fprintf(f,"System info: %s\n\n",alcSystemInfo());
			fprintf(f,"Born:    %s\n",alcGetBornTime().str());
			tTime now;
			now.now();
			fprintf(f,"Defunct: %s\n",now.str());
			fprintf(f,"Uptime:  %s\n",alcGetUptime().str(0x01));
			fprintf(f,"Main thread id: %d\n",alcGetMainThreadId());
			fprintf(f,"This thread id: %d\n",alcGetSelfThreadId());
			fprintf(f,"Exception %s:\n%s\n",this->what(),this->backtrace());
			fclose(f);
		}
		free(where);
	}
}
const char * txBase::what() { return msg; }
const char * txBase::backtrace() { return bt; }
txBase::~txBase() {
	free(bt);
	free(imsg);
}
//End base

//End Exceptions

} //End alc namespace

