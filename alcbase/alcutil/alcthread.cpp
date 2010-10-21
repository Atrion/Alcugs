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
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCTHREAD_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#ifdef ENABLE_THREADS
#include <pthread.h>
#endif

#include <cerrno>

#include "alcdebug.h"

namespace alc {


U32 alcGetSelfThreadId() {
	#ifdef ENABLE_THREADS
		#ifdef __WIN32__
			return GetCurrentThreadId();
		#else
			return pthread_self();
		#endif
	#else
	return 0;
	#endif
}

#ifndef __WIN32__
#define THREAD_RET void *
#else
#define THREAD_RET unsigned __stdcall
#endif

#ifdef ENABLE_THREADS
static THREAD_RET _alcThreadSpawner(void * s) {
	tThread * t;
	t=static_cast<tThread *>(s);
	t->main();
	#ifndef __WIN32__
	return (NULL);
	#else
	return 0;
	#endif
}
#endif

tThread::tThread() {
	spawned=false;
}
tThread::~tThread() {
	join();
}
void tThread::spawn() {
#ifdef ENABLE_THREADS
	if(spawned) return;
	#ifndef __WIN32__
	if(pthread_create(&id, NULL, _alcThreadSpawner,this)!=0) {
		throw txBase(_WHERE("Cannot create thread"));
	}
	#else
	id = (HANDLE)_beginthreadex(NULL,0,_alcThreadSpawner,(void *)this,0,NULL);
	if((int)id==-1) throw txBase(_WHERE("Cannot create thread"));
	#endif
	spawned=true;
#else //enable thread
	main();
#endif
}
void tThread::join() {
#ifdef ENABLE_THREADS
	if(!spawned) return;
	#ifndef __WIN32__
	if(pthread_join(id,NULL)!=0) {
		throw txBase(_WHERE("error joining thread"));
	}
	#else
	if(WaitForSingleObject(id,INFINITE)==WAIT_FAILED) {
		throw txBase(_WHERE("error joining thread"));
	}
	if(CloseHandle(id)==0) {
		throw txBase(_WHERE("error joining thread (CloseHandle)"));
	}
	#endif
	spawned=false;
#else //enable threads
	//Noop
#endif
}

tMutex::tMutex() {
#ifdef ENABLE_THREADS
	#ifndef __WIN32__
	if(pthread_mutex_init(&id,NULL)!=0) {
		throw txBase(_WHERE("error creating mutex"));
	}
	#else
	id=CreateMutex(NULL,FALSE,NULL);
	if(id==NULL) throw txBase(_WHERE("error creating mutex"));
	#endif
	islocked=false;
#endif
}
tMutex::~tMutex() {
#ifdef ENABLE_THREADS
	unlock();
	#ifndef __WIN32__
	if(pthread_mutex_destroy(&id)!=0) {
		throw txBase(_WHERE("error destroying mutex"));
	}
	#else
	if(CloseHandle(id)==0) {
		throw txBase(_WHERE("error destroying mutex"));
	}
	#endif
#endif
}
void tMutex::lock() {
#ifdef ENABLE_THREADS
	#ifndef __WIN32__
	if(pthread_mutex_lock(&id)!=0) {
		throw txBase(_WHERE("cannot lock mutex"));
	}
	#else
	if(WaitForSingleObject(id,INFINITE)==WAIT_FAILED) {
		throw txBase(_WHERE("cannot lock mutex"));
	}
	#endif
	islocked=true;
#endif
}
bool tMutex::trylock() {
#ifdef ENABLE_THREADS
	int retval;
	#ifndef __WIN32__
	retval=pthread_mutex_trylock(&id);
	if(retval==EBUSY) return 0;
	if(retval!=0) {
		throw txBase(_WHERE("cannot trylock mutex"));
	}
	#else
	retval=WaitForSingleObject(id,0);
	if(retval==WAIT_TIMEOUT) return 0;
	if(retval==WAIT_FAILED) {
		throw txBase(_WHERE("cannot trylock mutex"));
	}
	#endif
	islocked=true;
#endif
	return 1;
}
void tMutex::unlock() {
#ifdef ENABLE_THREADS
	if(!islocked) return; // we must never unlock an unlocked mutex, or destroying it will fail
	islocked=false;
	#ifndef __WIN32__
	if(pthread_mutex_unlock(&id)!=0) {
		throw txBase(_WHERE("cannot unlock mutex"));
	}
	#else
	if(ReleaseMutex(id)==0) {
		throw txBase(_WHERE("cannot unlock mutex"));
	}
	#endif
#endif
}

} //end namespace alc

