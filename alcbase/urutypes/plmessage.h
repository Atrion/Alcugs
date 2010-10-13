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

#ifndef __U_PLMESSAGE_H
#define __U_PLMESSAGE_H
/* CVS tag - DON'T TOUCH*/
#define __U_PLMESSAGE_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/
	class tpMessage : public tpObject {
	public:
		tpMessage(U16 type, bool incomplete = false) : tpObject(type, incomplete) {}
		tpMessage(U16 type, const tUruObjectRef &sender);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str(void) const;
		
		static tpMessage *create(U16 type, bool mustBeComplete = true);
		
		typedef std::vector<tUruObjectRef> tReceiverList;
		// format
		tUruObjectRef sender;
		tReceiverList receivers;
		U32 flags;
	};
	
	class tpAvatarMsg : public tpMessage {
	public:
		tpAvatarMsg(void) : tpMessage(plAvatarMsg) {}
	protected:
		tpAvatarMsg(U16 type) : tpMessage(type) {}
		tpAvatarMsg(U16 type, const tUruObjectRef &sender) : tpMessage(type, sender) {}
	};
	
	class tpLoadCloneMsg : public tpMessage {
	public:
		tpLoadCloneMsg(void) : tpMessage(plLoadCloneMsg) { subMessage = NULL; }
		~tpLoadCloneMsg(void) { if (subMessage) delete subMessage; }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str(void) const;
		
		static tpLoadCloneMsg *create(U16 type, bool mustBeComplete = true);
		
		// format
		tUruObjectRef clonedObj;
		tUruObjectRef unkObj1;
		U32 unk3;
		bool isLoad;
		tpObject *subMessage;
	protected:
		tpLoadCloneMsg(U16 type) : tpMessage(type) { subMessage = NULL; } // to be used by tpLoadAvatarMsg
		FORBID_CLASS_COPY(tpLoadCloneMsg)
	};
	
	class tpLoadAvatarMsg : public tpLoadCloneMsg {
	public:
		tpLoadAvatarMsg(void) : tpLoadCloneMsg(plLoadAvatarMsg) {}
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str(void) const;
		
		// format
		bool isPlayerAvatar;
		tUruObjectRef unkObj2;
	};
	
	class tpParticleTransferMsg : public tpMessage {
	public:
		tpParticleTransferMsg(void) : tpMessage(plParticleTransferMsg) {}
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str(void) const;
		
		// format
		tUruObjectRef unkObj1;
		U16 count;
	};
	
	class tpAvBrainGenericMsg : public tpAvatarMsg {
	public:
		tpAvBrainGenericMsg(void) : tpAvatarMsg(plAvBrainGenericMsg) {}
		tpAvBrainGenericMsg(const tUruObjectRef &sender) : tpAvatarMsg(plAvBrainGenericMsg, sender)
			{ unk3 = unk4 = 2; unk5 = unk7 = unk8 = 0; unk9 = 0.0; }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str(void) const;
		
		// format
		U32 unk3, unk4;
		Byte unk5, unk7, unk8;
		float unk9;
	};
	
	class tpServerReplyMsg : public tpMessage {
	public:
		tpServerReplyMsg(void) : tpMessage(plServerReplyMsg) {}
		tpServerReplyMsg(const tUruObjectRef &sender) : tpMessage(plServerReplyMsg, sender), replyType(0) { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str(void) const;
		
		// format
		U32 replyType; // 0x00 = deny, 0x01 = affirm, 0xFF = uninit
	};
	
	class tpKIMsg : public tpMessage {
	public:
		tpKIMsg(void) : tpMessage(pfKIMsg) {}
		tpKIMsg(const tUruObjectRef &sender, const tString &senderName, U32 senderKi, const tString &text);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str(void) const;
		
		// format
		tUruString senderName;
		U32 senderKi;
		tUruString text;
		U32 messageType;
	};
	
	class tpAvatarInputStateMsg : public tpMessage {
	public:
		tpAvatarInputStateMsg(void) : tpMessage(plAvatarInputStateMsg) {}
		tpAvatarInputStateMsg(const tUruObjectRef &sender) : tpMessage(plAvatarInputStateMsg, sender) { state = 0; }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual tString str(void) const;
		
		// format
		U16 state;
	};

} //End alc namespace

#endif