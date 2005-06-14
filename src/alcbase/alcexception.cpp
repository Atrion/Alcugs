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
	Alcugs exception Handler.
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCEXCEPTION_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"

#ifndef __WIN32__
namespace std {
extern "C" {
#include <execinfo.h>
}
}
#endif

//FIXME
namespace std {
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
}
}


#if defined(HAVE_GOOGLE_COREDUMPER_H) and defined(_GOOGLE_COREDUMPER_)
namespace google {
#include <google/coredumper.h>
}
#else
#undef HAVE_GOOGLE_COREDUMPER_H
#endif

//alcexception already included in alcugs.h

#include "alcdebug.h"

namespace alc {

#ifdef _TX_ABORT_
static bool txvAbort=1;
#else
static bool txvAbort=0;
#endif

static char txvCore=0x01; //0x00 - dissabled, 0x01 - enabled, 0x02 -always
static char * txvCorePath=NULL;

void alcWriteCoreDump() {
	DBG(5,"alcWriteCoreDump ");
	#if !defined(__WIN32__) and defined(HAVE_GOOGLE_COREDUMPER_H)
	DBG(5,"is enabled\n");
	unsigned int t,pid;
	pid=getpid();
	t=(unsigned int)std::time(NULL);
	
	int strsize=60;
	if(txvCorePath!=NULL) strsize+=strlen(txvCorePath);
	char * where=(char *)malloc(sizeof(char) * strsize+1);
	if(where) {
		memset(where,0,strsize+1);
		if(txvCorePath!=NULL) sprintf(where,"%s/core-%06i-%08X.core",txvCorePath,pid,t);
		else sprintf(where,"core-%06i-%08X.core",pid,t);
	
		if(txvCore & 0x01) google::WriteCoreDump((const char *)where);
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
char * alcGetCoreDumpPath() {
	return txvCorePath;
}
void alcSetAbort(bool c) {
	txvAbort=c;
}


//Exceptions

//Base class
txBase::txBase(char * msg,bool abort,bool core) {
	this->name=NULL;
	this->abort=abort;
	this->core=core;
	this->msg=msg;
	this->size=0;
	this->bt=NULL;
	this->imsg=NULL;
	this->_preparebacktrace();
}
txBase::txBase(char * name,char * msg,bool abort,bool core) {
	this->name=name;
	this->imsg=(char *)malloc(sizeof(char) * (strlen(name) + strlen(msg) + 2));
	if(this->imsg!=NULL) { strcpy(this->imsg,name); strcat(this->imsg,":"); strcat(this->imsg,msg); }
	this->abort=abort;
	this->core=core;
	this->msg=this->imsg;
	this->size=0;
	this->bt=NULL;
	this->_preparebacktrace();
}
void txBase::_preparebacktrace() {
//TODO: Porting - This code only works under Linux (it's part of the libc)
#if !defined(__WIN32__)
	//get the backtrace
	char **strings;
	size=std::backtrace(btArray,txExcLevels);
	strings=std::backtrace_symbols(btArray,size);
	
	/*
	int f;
	f=creat("backtrace.txt",0640);
	if(f!=-1) {
		std::backtrace_symbols_fd(btArray,size,f);
		close(f);
	}*/
	
	unsigned int i,msize=0;
	
	for(i=0; i<size; i++) {
		msize+=std::strlen(strings[i])+6;
	}
	msize+=30;
	bt=(char *)malloc(sizeof(char) * msize);
	if(bt!=NULL) {
		std::memset(bt,0,msize);
		std::sprintf(bt,"Backtrace with %u levels:\n",size);
		for(i=0; i<size; i++) {
			std::strcat(bt," ");
			std::strcat(bt,strings[i]);
			std::strcat(bt,"\n");
		}
	}
	if(strings!=NULL) free((void *)strings);
#else
	bt=(char *)std::malloc(sizeof(char) * 50);
	std::sprintf(bt,"Backtrace not implemented in your OS\n");
#endif
	if((txvCore & 0x02) || this->core) { alcWriteCoreDump(); }
	if(txvAbort || this->abort) {
		fprintf(stderr,"Exception %s:\n%s\n",this->what(),this->backtrace());
		FILE * f;
		f=fopen("BackTrace.txt","w");
		if(f!=NULL) {
			fprintf(f,"Exception %s:\n%s\n",this->what(),this->backtrace());
			fclose(f);
		}
		fprintf(stderr,"An exception requested to abort\n");
		std::abort();
	}
}
char * txBase::what() { return msg; }
char * txBase::backtrace() { return bt; }
txBase::~txBase() {
	if(bt!=NULL) free((void *)bt);
	if(imsg!=NULL) free((void *)imsg);
}
//End base



//End Exceptions

} //End alc namespace

