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

#ifndef __U_ALCTHREAD_H
#define __U_ALCTHREAD_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCTHREAD_H_ID "$Id$"

namespace alc {

////DEFINITIONS

#ifdef ENABLE_THREADS
#define alcBeginCriticalSection(a) { static tMutex a; a.lock(); }
#define alcEndCriticalSection(a)   a.unlock()
#else
#define alcBeginCriticalSection(a)
#define alcEndCriticalSection(a)
#endif

U32 alcGetSelfThreadId();

class tThread {
public:
	tThread();
	virtual ~tThread();
	void spawn();
	void join();
	virtual void main()=0;
private:
	bool spawned;
#ifdef ENABLE_THREADS
	#ifndef __WIN32__
	pthread_t id;
	#else
	HANDLE id;
	#endif
#endif
	
	// prevent copying
	tThread(const tThread &);
	const tThread &operator=(const tThread &);
};

class tMutex {
public:
	tMutex();
	virtual ~tMutex();
	void lock();
	bool trylock();
	void unlock();
private:
#ifdef ENABLE_THREADS
	bool islocked;
	#ifndef __WIN32__
	pthread_mutex_t id;
	#else
	HANDLE id;
	#endif
#endif
	
	// prevent copying
	tMutex(const tMutex &);
	const tMutex &operator=(const tMutex &);
};

class tMutexLock {
public:
	inline tMutexLock(tMutex &mutex) {
#ifdef ENABLE_THREADS
		this->mutex = &mutex;
		this->mutex->lock();
#endif
	}
	inline ~tMutexLock(void) {
#ifdef ENABLE_THREADS
		mutex->unlock();
#endif
	}
private:
#ifdef ENABLE_THREADS
	tMutex *mutex;
#endif
	
	// prevent copying
	tMutexLock(const tMutexLock &);
	const tMutexLock &operator=(const tMutexLock &);
};



} //End alc namespace

#endif
