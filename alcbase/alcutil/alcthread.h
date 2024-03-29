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

#ifndef __U_ALCTHREAD_H
#define __U_ALCTHREAD_H

#include "useful.h"

namespace alc {

pthread_t alcGetSelfThreadId(); // the actual type is pthread_t, which might not be available

#define DECLARE_LOCK(ExName, ExLockFunc, LockName) \
	class LockName { \
	public: \
		LockName(ExName &ex) : ex(&ex) { \
			this->ex->ExLockFunc(); \
		} \
		~LockName(void) { \
			ex->unlock(); \
		} \
	private: \
		ExName *ex; \
		\
		FORBID_CLASS_COPY(LockName) \
	}

class tSignalSetter {
public:
	tSignalSetter(bool block); //!< when true, blocks all signals, otherwise allows all signals
	~tSignalSetter();
private:
	sigset_t oldset;
};

class tThread {
public:
	tThread();
	virtual ~tThread();
	void spawn();
	void join();
	virtual void main()=0;
	bool isSpawned() { return spawned; }
private:
	bool spawned;
	pthread_t id;
	
	FORBID_CLASS_COPY(tThread)
};

class tMutex {
public:
	tMutex();
	~tMutex();
	void lock();
	void unlock();
	
	pthread_mutex_t *getMutex() { return &id; }
private:
	pthread_mutex_t id;
	
	FORBID_CLASS_COPY(tMutex)
};

class tSpinEx {
public:
	tSpinEx();
	~tSpinEx();
	void lock();
	void unlock();
private:
	pthread_spinlock_t id;
};

class tReadWriteEx {
public:
	tReadWriteEx();
	~tReadWriteEx();
	
	void read();
	void write();
	void unlock();
private:
	pthread_rwlock_t id;
	
	FORBID_CLASS_COPY(tReadWriteEx);
};

DECLARE_LOCK(tMutex, lock, tMutexLock);
DECLARE_LOCK(tSpinEx, lock, tSpinLock);
DECLARE_LOCK(tReadWriteEx, read, tReadLock);
DECLARE_LOCK(tReadWriteEx, write, tWriteLock);


} //End alc namespace

#endif
