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

//#define _DBG_LEVEL_ 10
#undef NDEBUG // always enable asserts etc. in tests
#include "alcdefs.h"

#include <alcmain.h>
#include <alcexception.h>
#include <unistd.h>
#include <iostream>

using namespace alc;

tMutex gmutex;


class tmyfunc :public tThread {
public:
	tmyfunc(int threadnum,int num,int timer) {
		this->threadnum=threadnum;
		this->num=num;
		this->timer=timer;
	}
private:
	int num;
	int timer;
	int threadnum;
	virtual void main() {
		int i;
		std::cout<<"Child thread "<<threadnum<<": "<<alcGetSelfThreadId()<<std::endl;
		for(i=1; i <= num; i++) {
			{
				tMutexLock lock(gmutex);
				alcGetMain()->std()->log("I am a child thread %i: %i begin\n",threadnum,i);
				usleep(timer/4);
				alcGetMain()->std()->log("I am a child thread %i: %i end\n",threadnum,i);
			}
			usleep(timer);
		}
	}
};

void test_startSomeThreads() {
	DBG(1, "Init...\n");
	tmyfunc * thread;
	thread = new tmyfunc(1,15,1000);
	thread->spawn();
	
	tmyfunc * thread2;
	thread2 = new tmyfunc(2,5,2000);
	thread2->spawn();

	thread2->join();
	int i;
	std::cout<<"Main thread: "<<alcGetSelfThreadId()<<std::endl;
	for(i=0; i<10; i++) {
		{
			tMutexLock lock(gmutex);
			alcGetMain()->std()->log("I am the main thread: %i!\n",i);
			usleep(2000/4);
			alcGetMain()->std()->log("I am the main thread: %i!\n",i);
		}
		usleep(2000);
	}
	thread->join();
	delete thread;
	delete thread2;
	std::cout << std::endl;
}

void test_mutex(void)
{
	tMutex test1;
	std::cout << "Locking Mutex\n";
	test1.lock();
	std::cout << "Unlocking Mutex\n";
	test1.unlock();
	std::cout << "Mutex done\n" << std::endl;
}

void test_rwlock(void)
{
	tReadWriteEx test;
/*	pthreads rwlocks do not support nesting :(
	// test nesting writes within reads
	{
		std::cout << "Read-locking\n";
		test.read();
		std::cout << "Write-locking\n";
		test.write();
		std::cout << "Unlocking\n";
		test.unlock();
		test.unlock();
	}
	// test nesting reads within writes
	{
		std::cout << "Write-locking\n";
		test.write();
		std::cout << "Read-locking\n";
		test.read();
		std::cout << "Unlocking\n";
		test.unlock(); // will unlock the write lock first!
		test.unlock();
	}
	std::cout << "Read-write done\n" << std::endl;*/
}

int main(void) {
	std::cout << std::endl << "Alcugs test suite - alcthread tests" <<std::endl;
	tAlcMain alcMain("Alcthread Test");
	try {

		test_startSomeThreads();
		test_mutex();
		test_rwlock();
		
		std::cout<< "Success!!" << std::endl;
	} catch (txBase &t) {
		std::cout<< "Cauth Exception " << t.what() << std::endl;
		std::cout<< t.backtrace() << std::endl;
		return -1;
	} catch (...) {
		std::cout<< "Cauth Unknown Exception" <<std::endl;
		return -1;
	}
	return 0;

}

