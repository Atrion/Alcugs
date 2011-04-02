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
	Threading classes
*/

//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "alcthread.h"

#include <alcexception.h>

#include <cerrno>
#include <pthread.h>
#include <cstdlib>
#include <csignal>


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

tSignalSetter::tSignalSetter(bool block)
{
	sigset_t set;
	sigfillset(&set);
	if (pthread_sigmask(block ? SIG_BLOCK : SIG_UNBLOCK, &set, &oldset))
		throw txBase(_WHERE("Error setting sigmask"));
}
tSignalSetter::~tSignalSetter()
{
	if (pthread_sigmask(SIG_SETMASK, &oldset, NULL))
		throw txBase(_WHERE("Error setting sigmask"));
}

tThread::tThread() : spawned(false) {}
tThread::~tThread() {
	if (spawned) throw txBase(_WHERE("Attempting to delete a running thread!"));
}
void tThread::spawn() {
	if(spawned) return;
	// make sure all signals are blocked (this is inherited to the child thread)
	tSignalSetter doNotDisturb(/*block*/true);
	// launch thread
	if (pthread_create(&id, NULL, _alcThreadSpawner,this))
		throw txBase(_WHERE("Something went wrong during thread spawn!"));
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
void tMutex::unlock() {
	if(pthread_mutex_unlock(&id)) throw txBase(_WHERE("cannot unlock mutex"));
}

tSpinEx::tSpinEx()
{
	if (pthread_spin_init(&id, NULL)) throw txBase(_WHERE("Error creating spinlock"));
}
tSpinEx::~tSpinEx()
{
	if (pthread_spin_destroy(&id)) throw txBase(_WHERE("Error destroying spinlock"));
}
void tSpinEx::lock()
{
	if (pthread_spin_lock(&id)) throw txBase(_WHERE("Error locking spinlock"));
}
void tSpinEx::unlock()
{
	if (pthread_spin_unlock(&id)) throw txBase(_WHERE("Error unlocking spinlock"));
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

