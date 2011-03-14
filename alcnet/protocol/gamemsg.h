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

#ifndef __U_FILENAME_H
#define __U_FILENAME_H
/* CVS tag - DON'T TOUCH*/
#define __U_FILENAME_H_ID "$Id$"

#include "protocol.h"

namespace alc {
	
	class tpLoadCloneMsg;
	class tPageInfo;

	
	// Helper class for the member list messages
	class tMemberInfo : public tStreamable {
	public:
		tMemberInfo(tNetSession *u, const tUruObject &obj, bool hidePlayer);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		tString str(void) const;
	private:
		uint32_t ki;
		tString avatar;
		bool hidePlayer;
		uint8_t buildType;
		uint32_t ip;
		uint16_t port;
		tUruObject obj;
	};
	
	class tmJoinReq : public tmMsgBase {
	public:
		tmJoinReq(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		uint32_t ip; //network order
		uint16_t port; //network order
	};
	
	class tmJoinAck : public tmMsgBase {
	public:
		tmJoinAck(tNetSession *u, uint32_t x, const tStreamable *sdl);
		virtual void stream(tBBuf &t) const;
		// format
		tStreamedObject sdlStream;
	};
	
	class tmGameMessage : public tmMsgBase {
	public:
		tmGameMessage(tNetSession *u);
		tmGameMessage(tNetSession *u, const tmGameMessage &msg);
		tmGameMessage(tNetSession *u, uint32_t ki, tpObject *obj);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		// format
		tStreamedObject msgStream;
	protected:
		tmGameMessage(uint16_t cmd, tNetSession *u, const tmGameMessage &msg);
		tmGameMessage(uint16_t cmd, tNetSession *u, uint32_t ki, tpObject *obj);
	};
	
	class tmGameMessageDirected : public tmGameMessage {
	public:
		tmGameMessageDirected(tNetSession *u);
		tmGameMessageDirected(tNetSession *u, const tmGameMessageDirected &msg);
		tmGameMessageDirected(tNetSession *u, uint32_t ki, tpObject *obj);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		// format
		typedef std::vector<uint32_t> tRecList;
		tRecList recipients;
	protected:
		tmGameMessageDirected(uint16_t cmd, tNetSession *u, const tmGameMessageDirected &msg);
		tmGameMessageDirected(uint16_t cmd, tNetSession *u, uint32_t ki, tpObject *obj);
	};
	
	class tmLoadClone : public tmGameMessage {
	public:
		tmLoadClone(tNetSession *u);
		tmLoadClone(tNetSession *u, const tmLoadClone &msg);
		tmLoadClone(tNetSession *u, tpLoadCloneMsg *subMsg, bool isInitial);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		
		void checkSubMsg(tpLoadCloneMsg *subMsg);
		// format
		tUruObject obj;
		bool isPlayerAvatar;
		bool isLoad;
		bool isInitial;
	};
	
	class tmPagingRoom : public tmMsgBase {
	public:
		tmPagingRoom(tNetSession *u) : tmMsgBase(u) {}
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		uint32_t pageId;
		uint16_t pageType;
		tString pageName;
		bool isPageOut;
	};
	
	class tmGroupOwner : public tmMsgBase {
	public:
		tmGroupOwner(tNetSession *u, tPageInfo *page, bool isOwner);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint32_t pageId;
		uint16_t pageType;
		bool isOwner;
	};
	
	class tmPlayerPage : public tmMsgBase {
	public:
		tmPlayerPage(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		bool isPageOut;
		tUruObject obj;
	};
	
	class tmGameStateRequest : public tmMsgBase {
	public:
		tmGameStateRequest(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		typedef std::vector<uint32_t> tPageList;
		tPageList pages;
	};
	
	class tmInitialAgeStateSent : public tmMsgBase {
	public:
		tmInitialAgeStateSent(tNetSession *u, uint32_t num);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		uint32_t num;
	};
	
	class tmMembersListReq : public tmMsgBase {
	public:
		tmMembersListReq(tNetSession *u);
	};
	
	class tmStreamedObject : public tmMsgBase {
	public:
		tmStreamedObject(tNetSession *u);
		
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
	
		// format
		tUruObject obj;
		tStreamedObject content;
		
	protected:
		tmStreamedObject(uint16_t cmd, tNetSession *u, const tmStreamedObject &msg);
		tmStreamedObject(uint16_t cmd, tNetSession *u, const tUruObject &obj, tStreamable *content);
	};
	
	class tmTestAndSet : public tmStreamedObject {
	public:
		tmTestAndSet(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		bool isLockReq;
	};
	
	class tmSDLState : public tmStreamedObject {
	public:
		tmSDLState(tNetSession *u);
		tmSDLState(tNetSession *u, const tUruObject &obj, tStreamable *content, bool isInitial);
		
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
	
		// format
		bool isInitial;
		
	protected:
		tmSDLState(uint16_t cmd, tNetSession *u, const tmSDLState &msg);
	};
	
	class tmSDLStateBCast : public tmSDLState {
	public:
		tmSDLStateBCast(tNetSession *u);
		tmSDLStateBCast(tNetSession *u, const tmSDLStateBCast & msg);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
	};
	
	class tmRelevanceRegions : public tmMsgBase {
	public:
		tmRelevanceRegions(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmSetTimeout : public tmMsgBase {
	public:
		tmSetTimeout(tNetSession *u);
		virtual void store(tBBuf &t);
		virtual tString additionalFields(tString dbg) const;
		// format
		float timeout;
	};
	
	class tmMembersList : public tmMsgBase {
	public:
		tmMembersList(tNetSession *u);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		typedef std::vector<tMemberInfo> tMemberList;
		tMemberList members;
	};
	
	class tmMemberUpdate : public tmMsgBase {
	public:
		tmMemberUpdate(tNetSession *u, const tMemberInfo &info, bool isJoined);
		virtual void stream(tBBuf &t) const;
		virtual tString additionalFields(tString dbg) const;
		// format
		tMemberInfo info;
		bool isJoined;
	};
	
	class tmPython: public tmMsgBase {
	public:
		tmPython(tNetSession *u);
		virtual void store(tBBuf &t);
	};

} //End alc namespace

#endif
