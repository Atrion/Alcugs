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

namespace alc {

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/

	class tUnetTrackingServer : public tUnetServerBase {
	public:
		tUnetTrackingServer(void) : tUnetServerBase()
		{
			lstd->log("WARNING: The tracking server is not finished yet. So if it doesn\'t work, that's not even a bug.\n");
		}
		~tUnetTrackingServer(void) { delete trackingBackend; }
		
		virtual void onStart(void) {
			trackingBackend = new tTrackingBackend(smgr);
		}
		virtual void onConnectionClosed(tNetEvent * ev,tNetSession * u) {
			trackingBackend->removeServer(u);
		}
		virtual int onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u);
	private:
		tTrackingBackend *trackingBackend;
	};
	
} //End alc namespace

#endif
