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

extern const char * alcNetName;
extern Byte alcWhoami;

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
	void forcestop() { stop(0); /* stop with a timeout of 0 */ }
	/** Terminates all connections
	*/
	virtual void terminateAll();
	/** Force a reload of the netcore settings (after changing the configuration for example)
	*/
	void reload()
	{
		onUnloadConfig();
		closelogs();
		reconfigure();
		openlogs();
		onReloadConfig();
		onLoadConfig();
	}
protected:
	/** Terminates the connection of the specified peer
			\param who A session iterator that points to the desired peer
			\param reason The reason code (error code) (if 0, send RKickedOff to clients and RQutting for servers)
			\param destroyOnly false=sends a terminated/leave message, true=silently closes the connection and destroys it ASAP
	*/
	virtual void terminate(tNetSession *u, Byte reason = 0, bool destroyOnly = false);
	/** destroy that session and do an onConnectionClosed */
	void closeConnection(tNetSession *u);
	inline bool isRunning(void) { return state_running; }

	/** This event is raised when we have a new connection
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param u The peer session object
	*/
	virtual void onNewConnection(tNetEvent * ev,tNetSession * u) {}
	
	/** This event is raised when we recieve a new message
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param msg The message object
			\param u The peer session object
	*/
	virtual int onMsgRecieved(tNetEvent * ev,tUnetMsg * msg,tNetSession * u) { return 0; }
	
	/** This event is raised when a connection closes
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param u The peer session object
	*/
	virtual void onConnectionClosed(tNetEvent * ev,tNetSession * u) {}
	
	/** This event is raised when a peer have left (disconnected)
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param reason The reason code
			\param u The peer session object
	*/
	virtual void onLeave(tNetEvent * ev,Byte reason,tNetSession * u) {}
	
	/** This event is raised when a peer have sent a terminated message
			Note: Only servers can send terminated messages, a termintated message from a client
			is illegal, and should be processed as a leave message.
			If a server have send us a terminated message, and we are a server slave, it will be
			necessary to reconnect again.
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param reason The reason code
			\param u The peer session object
	*/
	virtual void onTerminated(tNetEvent * ev,Byte reason,tNetSession * u) {}
	
	/** This event is raised when a peer is sending too many messages per second.
			Disabling this protection, your server will be vulnerable to DoS attacks.
			You need to override it in your derived classes with your implementation.
			If it is not vetoed, it will terminate the connection.
			\param ev The event object (can be vetoed)
			\param u The peer session object
	*/
	virtual void onConnectionFlood(tNetEvent * ev,tNetSession * u) {}
	
	virtual void onConnectionTimeout(tNetEvent * ev,tNetSession * u) {}
	virtual void onIdle(bool idle=false) {}
	virtual void onStop() {}
	virtual void onStart() {}
	
	/** this is called after loading and reloading the config */
	virtual void onLoadConfig() {}
	/** this is called before reloading the config and before quitting.
			Everything created in onLoadConfig must be freed here */
	virtual void onUnloadConfig() {}
	/** this is called after loading and reloading the config */
	virtual void onReloadConfig() {}
private:
	void processEvent(tNetEvent *evt, tNetSession *u, bool shutdown = false);
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
