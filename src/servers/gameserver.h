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

#include "sdl.h"

namespace alc {

	class tmGameMessageDirected;

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/
	class tGameData : public tNetSessionData {
	public:
		tGameData(const tUruObject &obj, tNetSession *u) : obj(obj) { isHidden = false; this->u = u; }
		tUruObject obj;
		bool isHidden;
		tNetSession *u;
		
		inline tMemberInfo createInfo(void) {
			return tMemberInfo(u, obj, isHidden);
		}
	};

	class tUnetGameServer : public tUnetLobbyServerBase {
	public:
		tUnetGameServer(void);
		~tUnetGameServer(void);
		
		template <class T> void bcastMessage(const T &msg, U32 delay = 0) //!< delay is in msecs
		{ // template functions must be in the header so they can be instanciated properly
			// broadcast message
			tNetSession *session;
			smgr->rewind();
			while ((session = smgr->getNext())) {
				if (session->joined && session->ki != msg.ki) {
					T fwdMsg(session, msg);
					send(fwdMsg, delay);
				}
			}
		}
		
		/** creates a message to bring that player into the idle state. When inputState is < 0, generate a plAvBrainGenericMsg which
		gets the avatar out of afk/KI state. When it is >= 0, send a plAvatarInputStateMsg with the given state to get it out of animations */
		tmGameMessage makePlayerIdle(tNetSession *u, tUruObject rec, S32 inputState = -1);
	protected:
		virtual int onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u);
		virtual void onIdle(bool idle);
		virtual void terminate(tNetSession *u, Byte reason, bool gotLeave = false);
		virtual void onReloadConfig(bool reload) {
			ageState->reload();
		}
		virtual void onLoadConfig(void);

		virtual void additionalVaultProcessing(tNetSession *u, tvMessage *msg);
		virtual void playerAuthed(tNetSession *u);
		
		virtual bool canPortBeUsed(U16 port);
	private:
		void fwdDirectedGameMsg(tmGameMessageDirected &msg);
		void bcastMemberUpdate(tNetSession *u, bool isJoined);
		
		void removePlayerFromPage(tPageInfo *page, U32 ki);
		
		bool processGameMessage(tStreamedObject *msg, tNetSession *u, tUruObjectRef *receiver = NULL); //!< returns true if the message was processed and should not be broadcasted or forwarded
		void processKICommand(const tStrBuf &text, tNetSession *u);
		void sendKIMessage(const tStrBuf &text, tNetSession *u);
	
		tAgeInfo *ageInfo;
		tAgeStateManager *ageState;
		
		U32 lastPlayerLeft;
		U32 lingerTime;
		bool noReltoShare, serverSideCommands;
	};

} //End alc namespace

#endif
