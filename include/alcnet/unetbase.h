/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs Server Team                           *
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
	URUNET 3+
*/

#ifndef __U_UNETBASE_H
#define __U_UNETBASE_H
/* CVS tag - DON'T TOUCH*/
#define __U_UNETBASE_H_ID "$Id$"

namespace alc {

/** Base abstract class, you need to derive your server/client app's from here */
class tUnetBase :public tUnet {
public:
	tUnetBase(); //port & host read from config (or by setBindPort(port) and setBindAddress(addr))
	virtual ~tUnetBase();
	/** Runs the netcore */
	void run();
	
	/** Stops the netcore
			\param timeout Sets the timeout to wait for closing the connection to all peers (<0 gets timeout from config file)
	*/
	void stop(SByte timeout=-1);
	
	inline void forcestop() { stop(0); /* stop with a timeout of 0 */ }
	/** Terminates all connections to players */
	inline void terminatePlayers() { terminateAll(/*playersOnly*/true); }
	
	/** Force a reload of the netcore settings (after changing the configuration for example) */
	void reload()
	{
		onUnloadConfig();
		closelogs();
		reconfigure();
		openlogs();
		onReloadConfig();
		onLoadConfig();
	}
	
	/** terminate this session */
	inline void terminate(tNetSession *u, Byte reason = 0) { terminate(u, reason, /*gotLeave*/false); }
	
	/** check whether the server is still running */
	inline bool isRunning(void) { return state_running; }
protected:
	/** This event is raised when we have a new connection
			You need to override it in your derived classes with your implementation.
			\param u The peer session object
	*/
	virtual void onNewConnection(tNetSession * /*u*/) {}
	
	/** This event is raised when we recieve a new message
			You need to override it in your derived classes with your implementation.
			\param msg The message object
			\param u The peer session object
	*/
	virtual int onMsgRecieved(tUnetMsg * msg,tNetSession * u) = 0;
	
	/** This event is raised when a connection is being terminated
			You need to override it in your derived classes with your implementation.
			\param u The peer session object
			\param reason the reason for which the client left/was kicked
	*/
	virtual void onConnectionClosing(tNetSession * /*u*/, Byte /*reason*/) {}
	
	/** This event is raised when a peer is sending too many messages per second.
			Disabling this protection, your server will be vulnerable to DoS attacks.
			You need to override it in your derived classes with your implementation.
			If it is not vetoed, it will terminate the connection.
			\param ev The event object (to be vetoed)
			\param u The peer session object
	*/
	virtual void onConnectionFlood(tNetEvent * /*ev*/,tNetSession * /*u*/) {}
	
	/** This event is raised when a peer timed out as the connection dropped.
			You need to override it in your derived classes with your implementation.
			If it is not vetoed, it will terminate the connection.
			\param ev The event object (to be vetoed)
			\param u The peer session object
	*/
	virtual void onConnectionTimeout(tNetEvent * /*ev*/,tNetSession * /*u*/) {}
	
	/** This event is raised each time after the event queue got emtpied.
			You need to override it in your derived classes with your implementation.
			\param idle Set to true if the core is idle (no messages remaining to be sent or parsed)
	*/
	virtual void onIdle(bool /*idle*/=false) {}
	
	/** This is called before entering the event loop */
	virtual void onStart() {}
	
	/** This is called after leaving the event loop and closing all connections */
	virtual void onStop() {}
	
	/** this is called after loading and reloading the config */
	virtual void onLoadConfig() {}
	
	/** this is called before reloading the config and before quitting.
			Everything created in onLoadConfig must be freed here */
	virtual void onUnloadConfig() {}
	
	/** this is called while reloading the config */
	virtual void onReloadConfig() {}
private:
	void terminate(tNetSession *u, Byte reason, bool gotLeave);
	void terminateAll(bool playersOnly = false);
	void removeConnection(tNetSession *u); //!< destroy that session
	
	void processEventQueue(bool shutdown);
	int parseBasicMsg(tNetEvent * ev,tUnetMsg * msg,tNetSession * u,bool shutdown);
	void reconfigure();
	bool state_running;
	Byte stop_timeout;
#ifdef ENABLE_THREADS
	Byte pool_size;
#endif
};


}

#endif
