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
	URUNET 3+
*/

#ifndef __U_NETCORE_H
#define __U_NETCORE_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETCORE_H_ID "$Id$"

namespace alc {

/** Base abstract class, you need to derive your server/client app's from here */
class tUnetBase :public tUnet {
public:
	tUnetBase(char * lhost="0.0.0.0",U16 lport=0);
	~tUnetBase();
	void run();
	void stop(Byte timeout=5);
	void terminate(tNetSessionIte & who,bool silent=false,Byte reason=RKickedOff);
	void leave(tNetSessionIte & who,Byte reason=RQuitting);

	virtual void onNewConnection(tNetEvent * ev,tNetSession * u) {}
	virtual int onMsgRecieved(tNetEvent * ev,tUnetMsg * msg,tNetSession * u)=0;
	virtual void onConnectionClossed(tNetEvent * ev,tNetSession * u) {}
	virtual void onLeave(tNetEvent * ev,Byte reason,tNetSession * u) {}
	virtual void onTerminated(tNetEvent * ev,Byte reason,tNetSession * u) {}
	virtual void onConnectionFlood(tNetEvent * ev,tNetSession * u) {}
	virtual void onConnectionTimeout(tNetEvent * ev,tNetSession * u) {}
	virtual void onIdle(bool idle=false) {}
	//virtual void onConnectionClossing(tNetEvent * ev) {}
private:
	int parseBasicMsg(tNetEvent * ev,tUnetMsg * msg,tNetSession * u);
	bool state_running;
	Byte stop_timeout;
};


}

#endif
