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

#ifndef __U_NETCORE_H
#define __U_NETCORE_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETCORE_H_ID "$Id$"

namespace alc {

extern const char * alcNetName;
extern Byte alcWhoami;

/** Base abstract class, you need to derive your server/client app's from here */
class tUnetBase :public tUnet {
public:
	tUnetBase(); //port & host read from config (or by setBindPort(port) and setBindAddress(addr))
	~tUnetBase();
	/** Runs the netcore */
	void run();
	/** Stops the netcore
			\param timeout Sets the timeout to wait for clossing the connection to all peers (<0 gets timeout from config file)
	*/
	void stop(SByte timeout=-1);
	void forcestop() {}
	/** Terminates the connection of the specified peer
			\param who A session iterator that points to the desired peer
			\param silent false=sends a terminated message, true=silently closes the connection
			\param reason The reason code (error code)
	*/
	void terminate(tNetSessionIte & who,bool silent=false,Byte reason=RKickedOff);
	/** Terminates all connections, only client peers will recieve terminated messages.
	*/
	void terminateAll() {}
	/** Disconnects the client from the server
	*/
	void leave(tNetSessionIte & who,Byte reason=RQuitting);
	/** Force a reload of the netcore settings (after changing the configuration for example)
	*/
	void reload() { _reconfigure(); }

	/** This event is raised when we have a new connection
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param u The peer session object
	*/
	virtual void onNewConnection(tNetEvent * ev,tNetSession * u) = 0;
	/** This event is raised when we recieve a new message
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param msg The message object
			\param u The peer session object
	*/
	virtual int onMsgRecieved(tNetEvent * ev,tUnetMsg * msg,tNetSession * u) = 0;
	/** This event is raised when a connection closes
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param u The peer session object
	*/
	virtual void onConnectionClossed(tNetEvent * ev,tNetSession * u) = 0;
	/** This event is raised when a peer have left (disconnected)
			You need to override it in your derived classes with your implementation.
			\param ev The event object (cannot be vetoed)
			\param reason The reason code
			\param u The peer session object
	*/
	virtual void onLeave(tNetEvent * ev,Byte reason,tNetSession * u) = 0;
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
	virtual void onTerminated(tNetEvent * ev,Byte reason,tNetSession * u) = 0;
	/** This event is raised when a peer is sending too many messages per second.
			Dissabling this protection, your server will be vulnerable to DoS attacks.
			You need to override it in your derived classes with your implementation.
			If it is not vetoed, it will terminate the connection.
			\param ev The event object (can be vetoed)
			\param u The peer session object
	*/
	virtual void onConnectionFlood(tNetEvent * ev,tNetSession * u) = 0;
	virtual void onConnectionTimeout(tNetEvent * ev,tNetSession * u) = 0;
	virtual void onIdle(bool idle=false) = 0;
	virtual void onStop() = 0;
	virtual void onStart() = 0;
	//virtual void onConnectionClossing(tNetEvent * ev) {}
private:
	void installSignalHandler();
	int parseBasicMsg(tNetEvent * ev,tUnetMsg * msg,tNetSession * u);
	bool state_running;
	Byte stop_timeout;
	void _reconfigure();
	Byte pool_size;
};


}

#endif
