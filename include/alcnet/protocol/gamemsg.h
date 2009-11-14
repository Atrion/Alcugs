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

#ifndef __U_FILENAME_H
#define __U_FILENAME_H
/* CVS tag - DON'T TOUCH*/
#define __U_FILENAME_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/
	
	// Helper class for the member list messages
	class tMemberInfo : public tBaseType {
	public:
		tMemberInfo(tNetSession *u, const tUruObject &obj, bool hidePlayer);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		const char *str(void);
	private:
		U32 ki;
		tStrBuf avatar;
		bool hidePlayer;
		Byte buildType;
		U32 ip;
		U16 port;
		tUruObject obj;
		
		tStrBuf dbg;
	};
	
	class tmJoinReq : public tmMsgBase {
	public:
		tmJoinReq(tNetSession *u);
		virtual void store(tBBuf &t);
		
	};
	
	class tmJoinAck : public tmMsgBase {
	public:
		tmJoinAck(tNetSession *u, U32 x, tBaseType *sdl);
		virtual void stream(tBBuf &t) const;
		// format
		tStreamedObject sdlStream;
	};
	
	class tmGameMessage : public tmMsgBase {
	public:
		tmGameMessage(tNetSession *u);
		tmGameMessage(tNetSession *u, const tmGameMessage &msg);
		tmGameMessage(tNetSession *u, U32 ki, tpObject *obj);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		// format
		tStreamedObject msgStream;
	protected:
		tmGameMessage(U16 cmd, tNetSession *u, const tmGameMessage &msg);
		tmGameMessage(U16 cmd, tNetSession *u, U32 ki, tpObject *obj);
	};
	
	class tmGameMessageDirected : public tmGameMessage {
	public:
		tmGameMessageDirected(tNetSession *u);
		tmGameMessageDirected(tNetSession *u, const tmGameMessageDirected &msg);
		tmGameMessageDirected(tNetSession *u, U32 ki, tpObject *obj);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		// format
		typedef std::vector<U32> tRecList;
		tRecList recipients;
	protected:
		tmGameMessageDirected(U16 cmd, tNetSession *u, const tmGameMessageDirected &msg);
		tmGameMessageDirected(U16 cmd, tNetSession *u, U32 ki, tpObject *obj);
	};
	
	class tmLoadClone : public tmGameMessage {
	public:
		tmLoadClone(tNetSession *u);
		tmLoadClone(tNetSession *u, const tmLoadClone &msg);
		tmLoadClone(tNetSession *u, tpLoadCloneMsg *subMsg, bool isInitial);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		
		void checkSubMsg(tpLoadCloneMsg *subMsg);
		// format
		tUruObject obj;
		bool isPlayerAvatar;
		bool isLoad;
		bool isInitial;
	protected:
		virtual void additionalFields();
	};
	
	class tmPagingRoom : public tmMsgBase {
	public:
		tmPagingRoom(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		U32 pageId;
		U16 pageType;
		tStrBuf pageName;
		bool isPageOut;
	protected:
		virtual void additionalFields();
	};
	
	class tmGroupOwner : public tmMsgBase {
	public:
		tmGroupOwner(tNetSession *u, tPageInfo *page, bool isOwner);
		virtual void stream(tBBuf &t) const;
		// format
		U32 pageId;
		U16 pageType;
		bool isOwner;
	protected:
		virtual void additionalFields();
	};
	
	class tmPlayerPage : public tmMsgBase {
	public:
		tmPlayerPage(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		bool isPageOut;
		tUruObject obj;
	protected:
		virtual void additionalFields();
	};
	
	class tmGameStateRequest : public tmMsgBase {
	public:
		tmGameStateRequest(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		typedef std::vector<U32> tPageList;
		tPageList pages;
	protected:
		virtual void additionalFields();
	};
	
	class tmInitialAgeStateSent : public tmMsgBase {
	public:
		tmInitialAgeStateSent(tNetSession *u, U32 num);
		virtual void stream(tBBuf &t) const;
		// format
		U32 num;
	protected:
		virtual void additionalFields();
	};
	
	class tmMembersListReq : public tmMsgBase {
	public:
		tmMembersListReq(tNetSession *u);
	};
	
	class tmTestAndSet : public tmMsgBase {
	public:
		tmTestAndSet(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		tUruObject obj;
		bool isLockReq;
	protected:
		virtual void additionalFields();
	};
	
	class tmRelevanceRegions : public tmMsgBase {
	public:
		tmRelevanceRegions(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmSDLState : public tmMsgBase {
	public:
		tmSDLState(tNetSession *u);
		tmSDLState(tNetSession *u, const tUruObject &obj, tBaseType *sdl, bool isInitial);
		
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
	
		// format
		tUruObject obj;
		tStreamedObject sdlStream;
		bool isInitial;
	protected:
		tmSDLState(U16 cmd, tNetSession *u, const tmSDLState &msg);
		
		virtual void additionalFields();
	};
	
	class tmSDLStateBCast : public tmSDLState {
	public:
		tmSDLStateBCast(tNetSession *u);
		tmSDLStateBCast(tNetSession *u, const tmSDLStateBCast & msg);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
	};
	
	class tmSetTimeout : public tmMsgBase {
	public:
		tmSetTimeout(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		float timeout;
	private:
		virtual void additionalFields();
	};
	
	class tmMembersList : public tmMsgBase {
	public:
		tmMembersList(tNetSession *u);
		virtual void stream(tBBuf &t) const;
		// format
		typedef std::vector<tMemberInfo> tMemberList;
		tMemberList members;
	private:
		virtual void additionalFields();
	};
	
	class tmMemberUpdate : public tmMsgBase {
	public:
		tmMemberUpdate(tNetSession *u, const tMemberInfo &info, bool isJoined);
		virtual void stream(tBBuf &t) const;
		// format
		tMemberInfo info;
		bool isJoined;
	protected:
		virtual void additionalFields();
	};
	
	class tmPython: public tmMsgBase {
	public:
		tmPython(tNetSession *u);
		virtual void store(tBBuf &t);
	};

} //End alc namespace

#endif
