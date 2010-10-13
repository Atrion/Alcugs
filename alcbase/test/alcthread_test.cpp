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

//#define _DBG_LEVEL_ 10

#define IN_ALC_PROGRAM
#define ALC_PROGRAM_ID "$Id$"

#include <alcugs.h>

#include <alcdebug.h>

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
		for(i=0; i<num; i++) {
			{
				tMutexLock lock(gmutex);
				alcGetMain()->std()->log("I am a child thread %i: %i!\n",threadnum,i);
				usleep(timer/4);
				alcGetMain()->std()->log("I am a child thread %i: %i!\n",threadnum,i);
			}
			usleep(timer);
		}
	}
};

int main(void) {
	std::cout << std::endl << "Alcugs test suite - alcthread tests" <<std::endl;
	tAlcMain alcMain;
	try {

		alcMain.std()->log("Init...\n");
		
		tmyfunc * thread;
		thread = new tmyfunc(1,15,2000);
		thread->spawn();
		
		tmyfunc * thread2;
		thread2 = new tmyfunc(2,5,1000);
		thread2->spawn();

		thread2->join();
		int i;
		std::cout<<"Main thread: "<<alcGetSelfThreadId()<<std::endl;
		for(i=0; i<10; i++) {
			{
				tMutexLock lock(gmutex);
				alcGetMain()->std()->log("I am the main thread: %i!\n",i);
				usleep(1000);
				alcGetMain()->std()->log("I am the main thread: %i!\n",i);
			}
			usleep(5000);
		}
		thread->join();
		delete thread;
		delete thread2;

		//alcShutdown();
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
