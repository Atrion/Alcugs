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

#ifndef __U_GAMESERVER_H
#define __U_GAMESERVER_H
/* CVS tag - DON'T TOUCH*/
#define __U_GAMESERVER_H_ID "$Id$"

#include <netsession.h>
#include <unetlobbyserverbase.h>
#include <protocol/gamemsg.h>

namespace alc {

	class tmGameMessageDirected;
	class tmGameMessage;
	class tAgeStateManager;
	class tAgeInfo;

	class tGameData : public tNetSessionData {
	public:
		tGameData(const tUruObject &obj, tNetSession *u) : obj(obj) { isHidden = false; this->u = u; }
		tUruObject obj;
		bool isHidden;
		tNetSession *u;
		
		inline tMemberInfo createInfo(void) {
			return tMemberInfo(u, obj, isHidden);
		}
		
		FORBID_CLASS_COPY(tGameData)
	};

	class tUnetGameServer : public tUnetLobbyServerBase {
	public:
		tUnetGameServer(void);
		~tUnetGameServer(void);
		
		template <class T> void bcastMessage(const T &msg, unsigned int delay = 0) //!< delay is in msecs
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
		tmGameMessage makePlayerIdle(tNetSession *u, tUruObject rec, int inputState = -1);
		inline bool getLinkingOutIdle() { return linkingOutIdle; }
	protected:
		virtual int onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u);
		virtual void onIdle(bool idle);
		virtual void onConnectionClosing(tNetSession *u, uint8_t reason);
		virtual void onApplyConfig(void);

		virtual void onVaultMessageForward(tNetSession *u, tvMessage *msg);
		virtual void onPlayerAuthed(tNetSession *u);
		
		virtual bool canPortBeUsed(uint16_t port);
	private:
		void fwdDirectedGameMsg(tmGameMessageDirected &msg);
		void bcastMemberUpdate(tNetSession *u, bool isJoined);
		
		void removePlayerFromPage(tPageInfo *page, uint32_t ki);
		bool checkIfOnlyPlayer(tNetSession *u);
		
		bool processGameMessage(tStreamedObject *msg, tNetSession *u, tUruObjectRef *receiver = NULL); //!< returns true if the message was processed and should not be broadcasted or forwarded
		void processKICommand(const tString &text, tNetSession *u);
		void sendKIMessage(const tString &text, tNetSession *u);
	
		tAgeInfo *ageInfo;
		tAgeStateManager *ageState;
		bool resetStateWhenEmpty; //!< used by /!resetage
		
		uint32_t lastPlayerLeft;
		unsigned int lingerTime;
		bool noReltoShare, serverSideCommands, linkingOutIdle;
		tString shardIdentifier;
	};

} //End alc namespace

#endif
