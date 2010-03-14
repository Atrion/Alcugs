/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs Server Team                           *
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

/** Base abstract class, you need to derive your server/client app's from here
		This class registers against tAlcUnetMain to make sure there's never more than 1 instance! */
class tUnetBase :public tUnet {
	friend class tAlcUnetMain; // these classes have a tight relationship, e.g. you need exactly one of both 
	
public:
	tUnetBase(Byte whoami); //port & host read from config (or by setBindPort(port) and setBindAddress(addr))
	virtual ~tUnetBase();
	/** Runs the netcore */
	void run();
	
	/** terminate this session */
	inline void terminate(tNetSession *u, Byte reason = 0) { terminate(u, reason, /*gotEndMsg*/false); }
	
	/** check whether the server is still running */
	inline bool isRunning(void) { return running; }
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
			\param u The peer session object
			\return true if it's ok to kick the player, false if not
	*/
	virtual bool onConnectionFlood(tNetSession * /*u*/) { return true; }
	
	/** This event is raised when a peer timed out as the connection dropped.
			You need to override it in your derived classes with your implementation.
			If it is not vetoed, it will terminate the connection.
			\param u The peer session object
			\return true if it's ok to kick the player, false if not
	*/
	virtual bool onConnectionTimeout(tNetSession * /*u*/) { return true; }
	
	/** This event is raised each time after the event queue got emtpied.
			You need to override it in your derived classes with your implementation.
			\param idle Set to true if the core is idle (no messages remaining to be sent or parsed)
	*/
	virtual void onIdle(bool /*idle*/=false) {}
	
	/** This is called before entering the event loop */
	virtual void onStart() {}
	
	/** This is called after leaving the event loop and closing all connections */
	virtual void onStop() {}
	
	/** This is called after loading and reloading the config (the first time is BEFORE onStart was called).
		You can rely on this being called, so put everything dealing with reading the config and opening logfiles here! */
	virtual void onApplyConfig() {}
	
	/** Stops the netcore in a sane way
			\param timeout Sets the timeout to wait for closing the connection to all peers (<0 gets timeout from config file)	*/
	void stop(SByte timeout=-1);
	/** Stops the netcore in a sane way, but without waiting for the clients to properly quit */
	inline void forcestop() { stop(0); /* stop with a timeout of 0 */ }
private:
	/** Just kills the socket - should only be used if the process was forked */
	inline void kill() { stopOp(); }
	
	void terminate(tNetSession *u, Byte reason, bool gotEndMsg);
	void terminateAll(bool playersOnly = false);
	inline void terminatePlayers() { terminateAll(/*playersOnly*/true); }
	void removeConnection(tNetSession *u); //!< destroy that session
	
	void processEventQueue(bool shutdown);
	int parseBasicMsg(tUnetMsg * msg, tNetSession * u, bool shutdown);
	void applyConfig();
	bool running;
	Byte stop_timeout;
#ifdef ENABLE_THREADS
	Byte pool_size;
#endif
};


}

#endif
