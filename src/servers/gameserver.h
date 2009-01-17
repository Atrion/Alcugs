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

#ifndef __U_GAMESERVER_H
#define __U_GAMESERVER_H
/* CVS tag - DON'T TOUCH*/
#define __U_GAMESERVER_H_ID "$Id$"

#include <unetlobbyserverbase.h>
#include "sdl.h"

namespace alc {

	class tmGameMessageDirected;

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/
	class tGameData : public tNetSessionData {
	public:
		tGameData(const tUruObject &obj) : obj(obj) { }
		tUruObject obj;
	};

	class tUnetGameServer : public tUnetLobbyServerBase {
	public:
		tUnetGameServer(void);
		~tUnetGameServer(void);
		
		template <class T> void bcastMessage(T &msg);
	protected:
		virtual void onConnectionClosed(tNetEvent *ev, tNetSession *u);
		virtual int onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u);
		virtual void onIdle(bool idle);
		virtual void terminate(tNetSession *u, Byte reason = RKickedOff, bool destroyOnly = false);
		virtual void onReloadConfig(bool reload) {
			ageState->reload();
		}
		virtual void onLoadConfig(void);

		virtual void additionalVaultProcessing(tNetSession *u, tvMessage *msg);
		virtual void playerAuthed(tNetSession *u);
		
		virtual bool canPortBeUsed(U16 port);
	private:
		Byte fwdDirectedGameMsg(tmGameMessageDirected &msg);
		void bcastMemberUpdate(tNetSession *u, bool isJoined);
		
		void removePlayerFromPage(tPageInfo *page, U32 ki);
	
		tAgeInfo *ageInfo;
		tAgeStateManager *ageState;
		
		U32 lastPlayerLeft;
		U32 lingerTime;
	};

} //End alc namespace

#endif
