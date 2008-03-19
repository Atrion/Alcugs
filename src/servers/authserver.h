/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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

#ifndef __U_AUTHSERVER_H
#define __U_AUTHSERVER_H
/* CVS tag - DON'T TOUCH*/
#define __U_AUTHSERVER_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/

	class tUnetAuthServer :public tUnetServerBase {
	public:
		// these are pure virtual but we don't need them
		virtual void onConnectionClosed(tNetEvent * ev,tNetSession * u) {}
		virtual void onLeave(tNetEvent * ev,Byte reason,tNetSession * u) {}
		virtual void onTerminated(alc::tNetEvent*, alc::Byte, alc::tNetSession*) {}
		virtual void onConnectionTimeout(alc::tNetEvent*, alc::tNetSession*) {}
		virtual void onStart() {}
		virtual void onNewConnection(alc::tNetEvent*, alc::tNetSession*) {}
		virtual int onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
			{ return tUnetServerBase::onMsgRecieved(ev, msg, u); }
		virtual void onConnectionFlood(alc::tNetEvent*, alc::tNetSession*) {}
		virtual void onIdle(bool) {}
		virtual void onStop() {}
	};
	
} //End alc namespace

#endif
