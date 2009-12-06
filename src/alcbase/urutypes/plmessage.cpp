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

/* CVS tag - DON'T TOUCH*/
#define __U_PLMESSAGE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	//// tpMessage
	tpMessage::tpMessage(U16 type, const tUruObjectRef &sender) : tpObject(type), sender(sender)
	{
		// the reference list stays empty
		flags = 0;
	}
	
	tpMessage *tpMessage::create(U16 type, bool mustBeComplete)
	{
		tpObject *obj = alcCreatePlasmaObject(type, mustBeComplete);
		tpMessage *specificObj = dynamic_cast<tpMessage *>(obj);
		if (specificObj == NULL)
			throw txUnexpectedData(_WHERE("Unwanted message type %s (0x%04X)", alcGetPlasmaType(type), type));
		return specificObj;
	}
	
	void tpMessage::store(tBBuf &t)
	{
		U32 u32Val;
		
		t.get(sender);
		U32 refCount = t.getU32();
		// read array of receivers - perhaps put this into a helper function?
		receivers.clear();
		receivers.reserve(refCount);
		tReceiverList::iterator it;
		for (U32 i = 0; i < refCount; ++i) {
			it = receivers.insert(receivers.end(), tUruObjectRef());
			t.get(*it);
		}
		// remaining values
		u32Val = t.getU32();
		if (u32Val != 0) throw txUnexpectedData(_WHERE("plMessage.unk1 must be 0 but is %d", u32Val));
		u32Val = t.getU32();
		if (u32Val != 0) throw txUnexpectedData(_WHERE("plMessage.unk2 must be 0 but is %d", u32Val));
		flags = t.getU32();
	}
	
	void tpMessage::stream(tBBuf &t) const
	{
		t.put(sender);
		t.putU32(receivers.size());
		for (tReceiverList::const_iterator it = receivers.begin(); it != receivers.end(); ++it)
			t.put(*it);
		t.putU32(0); // unk1
		t.putU32(0); // unk2
		t.putU32(flags);
	}
	
	void tpMessage::toString() const
	{
		strBuf.printf(" Sender: [%s]\n", sender.str());
		int nr = 1;
		for (tReceiverList::const_iterator it = receivers.begin(); it != receivers.end(); ++it, ++nr) {
			strBuf.printf(" Receiver %d: [%s]\n", nr, it->str());
		}
		strBuf.printf(" Flags: 0x%08X\n", flags);
	}
	
	//// tpLoadCloneMsg
	tpLoadCloneMsg *tpLoadCloneMsg::create(U16 type, bool mustBeComplete)
	{
		tpObject *obj = alcCreatePlasmaObject(type, mustBeComplete);
		tpLoadCloneMsg *specificObj = dynamic_cast<tpLoadCloneMsg *>(obj);
		if (specificObj == NULL)
			throw txUnexpectedData(_WHERE("Unwanted message type %s (0x%04X)", alcGetPlasmaType(type), type));
		return specificObj;
	}
	
	void tpLoadCloneMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		
		Byte byteVal;
		U16 u16Val;
		U32 u32Val;
		
		t.get(clonedObj);
		if (!clonedObj.hasObj) throw txUnexpectedData(_WHERE("plLoadCloneMsg.clonedObj must not be null"));
		t.get(unkObj1);
		
		u32Val = t.getU32();
		if (u32Val != clonedObj.obj.clonePlayerId)
			throw txUnexpectedData(_WHERE("plLoadCloneMsg.id (%d) must be the same as the clonedObj.clonePlayerId (%d)", u32Val, clonedObj.obj.clonePlayerId));
		
		unk3 = t.getU32(); // when loading an avatar, this is the avatar KI; for the bugs, it's zero
		if (unk3 != 0 && unk3 != clonedObj.obj.clonePlayerId)
			throw txUnexpectedData(_WHERE("plLoadCloneMsg.unk3 (%d) must be 0 or the same as the clonedObj.clonePlayerId (%d)", unk3, clonedObj.obj.clonePlayerId));
		
		byteVal = t.getByte();
		if (byteVal != 0x01) throw txUnexpectedData(_WHERE("plLoadCloneMsg.unk4 must be 0x01 but is 0x%02X", byteVal));
		
		byteVal = t.getByte();
		if (byteVal != 0x00 && byteVal != 0x01)
			throw txUnexpectedData(_WHERE("plLoadCloneMsg.isLoad must be 0x00 or 0x01 but is 0x%02X", byteVal));
		isLoad = byteVal;
		
		u16Val = t.getU16();
		if (u16Val != plNull && u16Val != plParticleTransferMsg)
			throw txUnexpectedData(_WHERE("Invalid type of plLoadCloneMsg.subMsg: %s (0x%04X)", alcGetPlasmaType(u16Val), u16Val));
		if (subMessage) delete subMessage;
		subMessage = NULL;
		subMessage = alcCreatePlasmaObject(u16Val);
		t.get(*subMessage);
	}
	
	void tpLoadCloneMsg::stream(tBBuf &t) const
	{
		if (subMessage == NULL) throw txUnexpectedData(_WHERE("A subMessage must be set"));
		tpMessage::stream(t);
		
		t.put(clonedObj);
		t.put(unkObj1);
		
		t.putU32(clonedObj.obj.clonePlayerId);
		t.putU32(unk3);
		t.putByte(1);
		t.putByte(isLoad);
		t.putU16(subMessage->getType());
		t.put(*subMessage);
	}
	
	void tpLoadCloneMsg::toString() const
	{
		tpMessage::toString();
		strBuf.printf(" Cloned object: [%s]\n", clonedObj.str());
		strBuf.printf(" Unknown object 1: [%s]\n", unkObj1.str());
		strBuf.printf(" Unk3: %d, Load: ", unk3);
		strBuf.printBoolean(isLoad);
		strBuf.nl();
		strBuf.printf(" [[ Sub message:\n %s ]]\n", subMessage->str());
	}
	
	//// tpLoadAvatarMsg
	void tpLoadAvatarMsg::store(tBBuf &t)
	{
		tpLoadCloneMsg::store(t);
		
		Byte byteVal;
		
		byteVal = t.getByte();
		if (byteVal != 0x00 && byteVal != 0x01)
			throw txUnexpectedData(_WHERE("plLoadAvatarMsg.isPlayerAvatar must be 0x00 or 0x01 but is 0x%02X", byteVal));
		isPlayerAvatar = byteVal;
		
		t.get(unkObj2);
		
		byteVal = t.getByte();
		if (byteVal != 0x00)
			throw txUnexpectedData(_WHERE("plLoadAvatarMsg.unk5 must be 0x00 but is 0x%02X", byteVal));
	}
	
	void tpLoadAvatarMsg::stream(tBBuf &t) const
	{
		tpLoadCloneMsg::stream(t);
		
		t.putByte(isPlayerAvatar);
		t.put(unkObj2);
		t.putByte(0);
	}
	
	void tpLoadAvatarMsg::toString() const
	{
		tpLoadCloneMsg::toString();
		strBuf.printBoolean(" Player avatar: ", isPlayerAvatar);
		strBuf.printf(", Unknown object 2: [%s]\n", unkObj2.str());
	}
	
	//// tpParticleTransferMsg
	void tpParticleTransferMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		t.get(unkObj1);
		count = t.getU16();
	}
	
	void tpParticleTransferMsg::stream(tBBuf &t) const
	{
		tpMessage::stream(t);
		t.put(unkObj1);
		t.putU16(count);
	}
	
	void tpParticleTransferMsg::toString() const
	{
		tpMessage::toString();
		strBuf.printf(" Unknown object: [%s], Count: %d\n", unkObj1.str(), count);
	}
	
	//// tpAvBrainGenericMsg
	void tpAvBrainGenericMsg::store(tBBuf &t)
	{
		tpAvatarMsg::store(t);
		
		unk3 = t.getU32();
		unk4 = t.getU32();
		unk5 = t.getByte();
		
		float varFloat = t.getFloat();
		if (varFloat != 0.0) throw txUnexpectedData(_WHERE("plAvBrainGenericMsg.unk6 must be 0.0, not %f", varFloat));
		
		unk7 = t.getByte();
		unk8 = t.getByte();
		unk9 = t.getFloat();
	}
	
	void tpAvBrainGenericMsg::stream(tBBuf &t) const
	{
		tpAvatarMsg::stream(t);
		t.putU32(unk3);
		t.putU32(unk4);
		t.putByte(unk5);
		t.putFloat(0.0); // unk6
		t.putByte(unk7);
		t.putByte(unk8);
		t.putFloat(unk9);
	}
	
	void tpAvBrainGenericMsg::toString() const
	{
		tpAvatarMsg::toString();
		strBuf.printf(" Unkonw 5: %d, Unknown 8: %d, Unknown 9: %f\n", unk5, unk8, unk9);
	}
	
	//// tpServerReplyMsg
	void tpServerReplyMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		replyType = t.getU32();
	}
	
	void tpServerReplyMsg::stream(tBBuf &t) const
	{
		tpMessage::stream(t);
		t.putU32(replyType);
	}
	
	void tpServerReplyMsg::toString() const
	{
		tpMessage::toString();
		strBuf.printf(" Reply Type: %d\n", replyType);
	}
	
	//// tpKIMsg
	tpKIMsg::tpKIMsg(const tUruObjectRef &sender, const tString &senderName, U32 senderKi, const tString &text)
	 : tpMessage(pfKIMsg, sender), senderName(senderName), senderKi(senderKi), text(text)
	{
		messageType = 0;
	}
	
	void tpKIMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		
		Byte byteVal;
		float floatVal;
		U32 u32Val;
		
		byteVal = t.getByte();
		if (byteVal != 0) throw txUnexpectedData(_WHERE("pfKIMsg.unk3 must be 0 but is %d", byteVal));
		
		t.get(senderName);
		senderKi = t.getU32();
		t.get(text);
		messageType = t.getU32();
		
		floatVal = t.getFloat();
		if (floatVal != 0.0) throw txUnexpectedData(_WHERE("pfKIMsg.unk4 must be 0.0 but is %f", floatVal));
		u32Val = t.getU32();
		if (u32Val != 0) throw txUnexpectedData(_WHERE("pfKIMsg.unk5 must be 0 but is %d", u32Val));
	}
	
	void tpKIMsg::stream(tBBuf &t) const
	{
		tpMessage::stream(t);
		t.putByte(0); // unk3
		t.put(senderName);
		t.putU32(senderKi);
		t.put(text);
		t.putU32(messageType);
		t.putFloat(0.0); // unk4
		t.putU32(0); // unk5
	}
	
	void tpKIMsg::toString() const
	{
		tpMessage::toString();
		strBuf.printf(" Sender: %s (KI: %d)\n", senderName.c_str(), senderKi);
		strBuf.printf(" Text: %s, Message Type: 0x%08X\n", text.c_str(), messageType);
	}
	
	//// tpAvatarInputStateMsg
	void tpAvatarInputStateMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		state = t.getU16();
	}
	
	void tpAvatarInputStateMsg::stream(tBBuf &t) const
	{
		tpMessage::stream(t);
		t.putU16(state);
	}
	
	void tpAvatarInputStateMsg::toString() const
	{
		tpMessage::toString();
		strBuf.printf(" State: %d\n", state);
	}

} //end namespace alc

