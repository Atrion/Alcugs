/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs H'uru Server Team                     *
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
	Threading classes
*/

/* CVS tag - DON'T TOUCH*/
#define __U_ALCTHREAD_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "alcthread.h"

#include <alcexception.h>

#include <cerrno>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>


namespace alc {


pthread_t alcGetSelfThreadId() {
	return pthread_self();
}

static void* _alcThreadSpawner(void * s) {
	try {
		static_cast<tThread *>(s)->main();
	}
	catch (txBase &t) {
		_DIE(_WHERE("FATAL Exception in thread: %s. Nothing I can do, aborting.", t.what()).c_str());
	}
	catch (...) {
		_DIE("FATAL Unknown exception in thread, nothing I can do, aborting.");
	}
	return (NULL);
}

tThread::tThread() : spawned(false) {}
tThread::~tThread() {
	if (spawned) throw txBase(_WHERE("Attempting to delete a running thread!"));
}
void tThread::spawn() {
	if(spawned) return;
	bool fail = false;
	// make sure all signals are blocked
	sigset_t set, oldset;
	sigfillset(&set);
	fail = pthread_sigmask(SIG_BLOCK, &set, &oldset);
	// launch thread
	fail = fail || pthread_create(&id, NULL, _alcThreadSpawner,this); // don't even launch thread if we failed eaely
	// restore signal mask
	fail = pthread_sigmask(SIG_SETMASK, &oldset, NULL) || fail;// restore sigmask in any case
	if (fail) throw txBase(_WHERE("Something went wrong during thread spawn!"));
	// done!
	spawned=true;
}
void tThread::join() {
	if (!spawned) return;
	if (pthread_join(id,NULL)!=0) throw txBase(_WHERE("error joining thread"));
	spawned=false;
}

tMutex::tMutex() 
{
	static pthread_mutexattr_t attr;
	static bool attrInited = false;
	if (!attrInited) {
		if (pthread_mutexattr_init (&attr)) throw txBase(_WHERE("error creating mutex attribute"));
#ifndef NDEBUG
		if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP)) throw txBase(_WHERE("error setting mutex attribute"));
#endif
		attrInited = true;
	}
	if (pthread_mutex_init(&id, &attr)) throw txBase(_WHERE("error creating mutex"));
}
tMutex::~tMutex() {
	if(pthread_mutex_destroy(&id)) throw txBase(_WHERE("error destroying mutex"));
}
void tMutex::lock() {
	if(pthread_mutex_lock(&id)) throw txBase(_WHERE("cannot lock mutex"));
}
bool tMutex::trylock() {
	int retval;
	retval=pthread_mutex_trylock(&id);
	if(retval==EBUSY) return false;
	if(retval!=0) throw txBase(_WHERE("cannot trylock mutex"));
	return true;
}
void tMutex::unlock() {
	if(pthread_mutex_unlock(&id)) throw txBase(_WHERE("cannot unlock mutex"));
}

tReadWriteEx::tReadWriteEx()
{
	if (pthread_rwlock_init(&id, NULL)) throw txBase(_WHERE("Error creating read-write lock"));
}
tReadWriteEx::~tReadWriteEx()
{
	if (pthread_rwlock_destroy(&id)) throw txBase(_WHERE("Error destroying read-write lock"));
}
void tReadWriteEx::read()
{
	if (pthread_rwlock_rdlock(&id)) throw txBase(_WHERE("Error read-locking"));
}
void tReadWriteEx::write()
{
	if (pthread_rwlock_wrlock(&id)) throw txBase(_WHERE("Error write-locking"));
}
void tReadWriteEx::unlock()
{
	if (pthread_rwlock_unlock(&id)) throw txBase(_WHERE("Error unlocking read-write lock"));
}


} //end namespace alc

