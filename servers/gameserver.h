/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

#include <unetlobbyserverbase.h>
#include <protocol/gamemsg.h>
#include <urutypes/ageinfo.h>

#include <map>

namespace alc {

	class tmGameMessageDirected;
	class tmGameMessage;
	class tAgeStateManager;
	class tAgeInfo;

	class tGameData : public tBaseType {
	public:
		tGameData(const tUruObject &obj) : obj(obj), isHidden(false) {}
		tUruObject obj;
		bool isHidden;
		
		tMemberInfo createInfo(tNetSession *u) {
			return tMemberInfo(u, obj, isHidden);
		}
		
		FORBID_CLASS_COPY(tGameData)
	};
	
	class tPage
	{
	public:
		typedef std::vector<uint32_t> tPlayerList;
		
		tPage(uint16_t number, bool alwaysLoaded);
		bool hasPlayer(uint32_t ki) const;
		bool removePlayer(uint32_t ki); //!< \returns false if that players was not on the list, true if it got removed
		
		// we do not store the name - the ages don't always use the same name for scene node and page
		const uint16_t number;
		const bool alwaysLoaded;
		
		// data for the messages (filled when we get a NetMsgPagingRom for this page)
		uint32_t owner;
		uint32_t pageId; // this and the next one are used when we send a NetMsgGroupOwner
		uint16_t pageType;
		tPlayerList players;
	};
	
	class tAgePages : public tAgeInfo
	{
	public:
		tAgePages(tString dir, const tString &name);
		tPage *getPage(uint32_t pageId); // returns the page for the given Id and name, if necessary creating it on-the-fly
		bool isConditionallyLoaded(uint32_t pageId) const;
	
		typedef std::map<uint16_t, tPage> tPageList;
		tPageList pages;
	};

	class tUnetGameServer : public tUnetLobbyServerBase {
	public:
		tUnetGameServer(void);
		~tUnetGameServer(void);
		
		template <typename T> void bcastMessage(const T &msg, tNetTime delay = 0) //!< delay is in secs
		{ // template functions must be in the header so they can be instanciated properly
			// broadcast message
			tReadLock lock(smgrMutex);
			for (tNetSessionMgr::tIterator it(smgr); it.next();) {
				if (it->joined && it->ki != msg.ki) {
					T fwdMsg(*it, msg);
					send(fwdMsg, delay);
				}
			}
		}
		
		/** creates a message to bring that player into the idle state. When inputState is < 0, generate a plAvBrainGenericMsg which
		gets the avatar out of afk/KI state. When it is >= 0, send a plAvatarInputStateMsg with the given state to get it out of animations */
		tmGameMessage makePlayerIdle(tNetSession *u, tUruObject rec, int inputState = -1);
		bool getLinkingOutIdle() { return linkingOutIdle; }
	protected:
		virtual int onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u);
		virtual void onIdle();
		virtual void onConnectionClosing(tNetSession *u, uint8_t reason);
		virtual void onApplyConfig(void);

		virtual void onVaultMessageForward(tNetSession *u, tvMessage *msg);
		virtual void onPlayerAuthed(tNetSession *u);
		
		virtual bool canPortBeUsed(uint16_t port);
	private:
		void fwdDirectedGameMsg(tmGameMessageDirected &msg);
		void bcastMemberUpdate(tNetSession *u, bool isJoined);
		
		void removePlayerFromPage(tPage *page, uint32_t ki);
		bool checkIfOnlyPlayer(tNetSession *exclude);
		
		bool processGameMessage(tStreamedObject *msg, tNetSession *u, tUruObjectRef *receiver = NULL); //!< returns true if the message was processed and should not be broadcasted or forwarded
		void processKICommand(const tString &text, tNetSession *u);
		void sendKIMessage(const tString &text, tNetSession *u);
	
		tAgePages *agePages;
		tAgeStateManager *ageState;
		bool resetStateWhenEmpty; //!< used by /!resetage
		
		time_t lastPlayerLeft, lingerTime; //!< protected by below mutex
		tSpinEx autoShutdownMutex;
		
		bool noReltoShare, serverSideCommands, linkingOutIdle;
		tString shardIdentifier;
	};

} //End alc namespace

#endif
