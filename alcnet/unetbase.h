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

#ifndef __U_UNETBASE_H
#define __U_UNETBASE_H

#include "unet.h"

namespace alc {

	class tUnetMsg;


/** Base abstract class, you need to derive your server/client app's from here
 * This class registers against tAlcUnetMain to make sure there's never more than 1 instance!
 * The class offers a loop function and ways to shut the server down sanely, and spawns a 
 * worker thread that processes events from the event queue and dispatches them to virtual functions
 * that you msut override in your subclasses.
 */
class tUnetBase :public tUnet {
	friend class tAlcUnetMain; // these classes have a tight relationship, e.g. you need exactly one of both 
	
public:
	tUnetBase(uint8_t whoami); //port & host read from config (or by setBindPort(port) and setBindAddress(addr))
	virtual ~tUnetBase();
	/** Runs the netcore */
	void run();
	
	/** check whether the server is still running (thread-safe) */
	bool isRunning(void);
protected:
	/** This event is raised when we have a new connection
			You need to override it in your derived classes with your implementation.
			Called in worker thread!
			\param u The peer session object
	*/
	virtual void onNewConnection(tNetSession * /*u*/) {}
	
	/** This event is raised when we recieve a new message
			You need to override it in your derived classes with your implementation.
			Called in worker thread!
			\param msg The message object
			\param u The peer session object
	*/
	virtual int onMsgRecieved(tUnetMsg * msg,tNetSession * u) = 0;
	
	/** This event is raised when a connection is being terminated
			You need to override it in your derived classes with your implementation.
			Called in worker thread!
			\param u The peer session object
			\param reason the reason for which the client left/was kicked
	*/
	virtual void onConnectionClosing(tNetSession * /*u*/, uint8_t /*reason*/) {}
	
	/** This event is raised when a peer is sending too many messages per second.
			Disabling this protection, your server will be vulnerable to DoS attacks.
			You need to override it in your derived classes with your implementation.
			If it is not vetoed, it will terminate the connection.
			Called in worker thread!
			\param u The peer session object
			\return true if it's ok to kick the player, false if not
	*/
	virtual bool onConnectionFlood(tNetSession * /*u*/) { return true; }
	
	/** This event is raised when a peer timed out as the connection dropped.
			You need to override it in your derived classes with your implementation.
			If it is not vetoed, it will terminate the connection.
			Called in worker thread!
			\param u The peer session object
			\return true if it's ok to kick the player, false if not
	*/
	virtual bool onConnectionTimeout(tNetSession * /*u*/) { return true; }
	
	/** This event is raised each time after the incoming messages were processed.
			You need to override it in your derived classes with your implementation.
			Called in main thread!
	*/
	virtual void onIdle() {}
	
	/** This is called before entering the event loop. Called in main thread, and worker is not running! */
	virtual void onStart() {}
	
	/** This is called after leaving the event loop and closing all connections Called in main thread, and worker is not running! */
	virtual void onStop() {}
	
	/** This is called after loading and reloading the config (the first time is BEFORE onStart was called).
		You can rely on this being called, so put everything dealing with reading the config and opening logfiles here!
		Called in main thread, and worker is not running! */
	virtual void onApplyConfig() {}
	
	
	/** Stops the netcore in a sane way */
	void stop();
	/** Stops the netcore in a sane way, but without waiting for the clients to properly quit */
	void forcestop();
private:
	bool terminateAll(bool playersOnly = false); //!< \returns if a session was terminated
	bool terminatePlayers() { return terminateAll(/*playersOnly*/true); }
	void applyConfig();
	tNetTime getStopTimeout() { tSpinLock lock(runModeMutex); return stop_timeout; }
	
	void processEvent(tNetEvent *evt); //!< dispatches most recent event, called in worker thread!
	
	bool configured; //!< stores whether applyConfig() has been called at least once
	bool running; //!< stores whether the server is running or shutting down, protected by below mutex
	tNetTime stop_timeout; //!< stores the timeout till the server shuts off, protected by below mutex
	tSpinEx runModeMutex;
	
	class tUnetWorkerThread : public tThread
	{
	public:
		tUnetWorkerThread(tUnetBase *unet): tThread(), net(unet) {}
		virtual ~tUnetWorkerThread() { stop(); }
		
		virtual void main(void);
		void stop();
		
	private:
		tUnetBase *net;
	};
	tUnetWorkerThread workerThread;
};


}

#endif
