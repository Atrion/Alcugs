/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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

#ifndef __U_TRACKINGSERVER_H
#define __U_TRACKINGSERVER_H
/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGSERVER_H_ID "$Id$"

#include "trackingbackend.h"
#include <unetserverbase.h>

namespace alc {

	////DEFINITIONS

	class tUnetTrackingServer : public tUnetServerBase {
	public:
		tUnetTrackingServer(void) : tUnetServerBase(KTracking), trackingBackend(this, bindaddr, bindport) {}
	protected:
		virtual void onStart(void) {
			trackingBackend.setServerManager(smgr);
		}
		virtual void onConnectionClosing(tNetSession * u, Byte /*reason*/) {
			trackingBackend.removeServer(u);
		}
		virtual void onApplyConfig(void) {
			trackingBackend.applyConfig();
		}
		virtual void onIdle(bool /*idle*/) {
			trackingBackend.updateStatusFile();
		}
		virtual void onNewConnection(tNetSession * u) {
			u->setTypeToGame(); // assume everyone connecting to us is a game (could also be a lobby though)
		}
		
		virtual int onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u);
	private:
		tTrackingBackend trackingBackend;
	};
	
} //End alc namespace

#endif
