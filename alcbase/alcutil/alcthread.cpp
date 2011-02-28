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


namespace alc {


pthread_t alcGetSelfThreadId() {
	return pthread_self();
}

static void* _alcThreadSpawner(void * s) {
	tThread * t;
	t=static_cast<tThread *>(s);
	t->main();
	return (NULL);
}

tThread::tThread() {
	spawned=false;
}
tThread::~tThread() {
	join();
}
void tThread::spawn() {
	if(spawned) return;
	if(pthread_create(&id, NULL, _alcThreadSpawner,this)!=0) {
		throw txBase(_WHERE("Cannot create thread"));
	}
	spawned=true;
}
void tThread::join() {
	if(!spawned) return;
	if(pthread_join(id,NULL)!=0) {
		throw txBase(_WHERE("error joining thread"));
	}
	spawned=false;
}

tMutex::tMutex() {
	if(pthread_mutex_init(&id,NULL)!=0) {
		throw txBase(_WHERE("error creating mutex"));
	}
}
tMutex::~tMutex() {
	unlock();
	if(pthread_mutex_destroy(&id)!=0) {
		throw txBase(_WHERE("error destroying mutex"));
	}
}
void tMutex::lock() {
	if(pthread_mutex_lock(&id)!=0) {
		throw txBase(_WHERE("cannot lock mutex"));
	}
}
bool tMutex::trylock() {
	int retval;
	retval=pthread_mutex_trylock(&id);
	if(retval==EBUSY) return false;
	if(retval!=0) {
		throw txBase(_WHERE("cannot trylock mutex"));
	}
	return true;
}
void tMutex::unlock() {
	if(pthread_mutex_unlock(&id)!=0) {
		throw txBase(_WHERE("cannot unlock mutex"));
	}
}

} //end namespace alc

