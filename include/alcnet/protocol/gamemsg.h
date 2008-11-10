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
	
	class tmJoinReq : public tmMsgBase {
	public:
		tmJoinReq(tNetSession *u);
		virtual void store(tBBuf &t);
		
	};
	
	class tmJoinAck : public tmMsgBase {
	public:
		tmJoinAck(tNetSession *u, U32 x);
		virtual void stream(tBBuf &t);
	};
	
	class tmGameMessage : public tmMsgBase {
	public:
		tmGameMessage(tNetSession *u);
		tmGameMessage(U16 cmd, U32 flags, tNetSession *u);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		Byte unk2; // unk1 and unk3 have always the same value, so they don't have to be saved
		tMBuf message; // saves only the complete message content
	};
	
	class tmGameMessageDirected : public tmGameMessage {
	public:
		tmGameMessageDirected(tNetSession *u);
		tmGameMessageDirected(U16 cmd, U32 flags, tNetSession *u);
		~tmGameMessageDirected(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		// format
		Byte nRecipients;
		U32 *recipients;
	};
	
	class tmLoadClone : public tmMsgBase {
	public:
		tmLoadClone(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmPagingRoom : public tmMsgBase {
	public:
		tmPagingRoom(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmPlayerPage : public tmMsgBase {
	public:
		tmPlayerPage(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmGameStateRequest : public tmMsgBase {
	public:
		tmGameStateRequest(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmInitialAgeStateSent : public tmMsgBase {
	public:
		tmInitialAgeStateSent(tNetSession *u, U32 num);
		virtual void stream(tBBuf &t);
		// format
		U32 num;
	protected:
		virtual void additionalFields();
	};
	
	class tmMembersListReq : public tmMsgBase {
	public:
		tmMembersListReq(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmTestAndSet : public tmMsgBase {
	public:
		tmTestAndSet(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		tUruObject obj;
		bool lockReq;
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
		virtual void store(tBBuf &t);
	};
	
	class tmSDLStateBCast : public tmMsgBase {
	public:
		tmSDLStateBCast(tNetSession *u);
		virtual void store(tBBuf &t);
	};
	
	class tmSetTimeout : public tmMsgBase {
	public:
		tmSetTimeout(tNetSession *u);
		virtual void store(tBBuf &t);
	};

} //End alc namespace

#endif
