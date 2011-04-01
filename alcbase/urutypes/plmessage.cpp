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

//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "plmessage.h"

#include "alcexception.h"

namespace alc {

	//// tpMessage
	tpMessage::tpMessage(uint16_t type, const tUruObjectRef &sender) : tpObject(type), sender(sender)
	{
		// the reference list stays empty
		flags = 0;
	}
	
	tpMessage *tpMessage::createFromStream(tStreamedObject *stream, bool UUFormat, bool mustBeComplete)
	{
		tpObject *obj = tpObject::createFromStream(stream, UUFormat, mustBeComplete);
		tpMessage *specificObj = dynamic_cast<tpMessage *>(obj);
		if (specificObj == NULL) {
			uint16_t type = obj->getType();
			delete obj;
			throw txUnexpectedData(_WHERE("Unwanted message type %s (0x%04X)", alcGetPlasmaType(type), type));
		}
		return specificObj;
	}
	
	void tpMessage::store(tBBuf &t)
	{
		uint32_t tmpVal;
		
		t.get(sender);
		uint32_t refCount = t.get32();
		// read array of receivers - perhaps put this into a helper function?
		receivers.clear();
		receivers.reserve(refCount);
		for (uint32_t i = 0; i < refCount; ++i) {
			t.get(*receivers.insert(receivers.end(), tUruObjectRef())); // first insert, then fill with data
		}
		// remaining values
		tmpVal = t.get32();
		if (tmpVal != 0) throw txUnexpectedData(_WHERE("plMessage.unk1 must be 0 but is %d", tmpVal));
		tmpVal = t.get32();
		if (tmpVal != 0) throw txUnexpectedData(_WHERE("plMessage.unk2 must be 0 but is %d", tmpVal));
		flags = t.get32();
	}
	
	void tpMessage::stream(tBBuf &t) const
	{
		t.put(sender);
		t.put32(receivers.size());
		for (tReceiverList::const_iterator it = receivers.begin(); it != receivers.end(); ++it)
			t.put(*it);
		t.put32(0); // unk1
		t.put32(0); // unk2
		t.put32(flags);
	}
	
	tString tpMessage::str(void) const
	{
		tString strBuf = tpObject::str();
		strBuf.printf(" Sender: [%s]\n", sender.str().c_str());
		int nr = 1;
		for (tReceiverList::const_iterator it = receivers.begin(); it != receivers.end(); ++it, ++nr) {
			strBuf.printf(" Receiver %d: [%s]\n", nr, it->str().c_str());
		}
		strBuf.printf(" Flags: 0x%08X\n", flags);
		return strBuf;
	}
	
	//// tpLoadCloneMsg
	tpLoadCloneMsg *tpLoadCloneMsg::createFromStream(tStreamedObject *stream, bool UUFormat, bool mustBeComplete)
	{
		tpObject *obj = tpObject::createFromStream(stream, UUFormat, mustBeComplete);
		tpLoadCloneMsg *specificObj = dynamic_cast<tpLoadCloneMsg *>(obj);
		if (specificObj == NULL) {
			uint16_t type = obj->getType();
			delete obj;
			throw txUnexpectedData(_WHERE("Unwanted message type %s (0x%04X)", alcGetPlasmaType(type), type));
		}
		return specificObj;
	}
	
	void tpLoadCloneMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		
		uint32_t tmpVal;
		
		t.get(clonedObj);
		if (!clonedObj.hasObj) throw txUnexpectedData(_WHERE("plLoadCloneMsg.clonedObj must not be null"));
		t.get(unkObj1);
		
		tmpVal = t.get32();
		if (tmpVal != clonedObj.obj.clonePlayerId)
			throw txUnexpectedData(_WHERE("plLoadCloneMsg.id (%d) must be the same as the clonedObj.clonePlayerId (%d)", tmpVal, clonedObj.obj.clonePlayerId));
		
		unk3 = t.get32(); // when loading an avatar, this is the avatar KI; for the bugs, it's zero
		if (unk3 != 0 && unk3 != clonedObj.obj.clonePlayerId)
			throw txUnexpectedData(_WHERE("plLoadCloneMsg.unk3 (%d) must be 0 or the same as the clonedObj.clonePlayerId (%d)", unk3, clonedObj.obj.clonePlayerId));
		
		tmpVal = t.get8();
		if (tmpVal != 0x01) throw txUnexpectedData(_WHERE("plLoadCloneMsg.unk4 must be 0x01 but is 0x%02X", tmpVal));
		
		tmpVal = t.get8();
		if (tmpVal != 0x00 && tmpVal != 0x01)
			throw txUnexpectedData(_WHERE("plLoadCloneMsg.isLoad must be 0x00 or 0x01 but is 0x%02X", tmpVal));
		isLoad = tmpVal;
		
		uint16_t subMsgType = t.get16();
		if (UUFormat) subMsgType = alcOpcodeUU2POTS(subMsgType);
		if (subMsgType != plNull && subMsgType != plParticleTransferMsg)
			throw txUnexpectedData(_WHERE("Invalid type of plLoadCloneMsg.subMsg: %s (0x%04X)", alcGetPlasmaType(subMsgType), subMsgType));
		if (subMessage) delete subMessage;
		subMessage = NULL;
		subMessage = createByType(subMsgType);
		subMessage->setUUFormat(UUFormat);
		t.get(*subMessage);
	}
	
	void tpLoadCloneMsg::stream(tBBuf &t) const
	{
		if (subMessage == NULL) throw txUnexpectedData(_WHERE("A subMessage must be set"));
		tpMessage::stream(t);
		
		t.put(clonedObj);
		t.put(unkObj1);
		
		t.put32(clonedObj.obj.clonePlayerId);
		t.put32(unk3);
		t.put8(1);
		t.put8(isLoad);
		t.put16(UUFormat ? alcOpcodePOTS2UU(subMessage->getType()) : subMessage->getType());
		subMessage->setUUFormat(UUFormat);
		t.put(*subMessage);
	}
	
	tString tpLoadCloneMsg::str(void) const
	{
		tString strBuf = tpMessage::str();
		strBuf.printf(" Cloned object: [%s]\n", clonedObj.str().c_str());
		strBuf.printf(" Unknown object 1: [%s]\n", unkObj1.str().c_str());
		strBuf.printf(" Unk3: %d, Load: ", unk3);
		strBuf.printBoolean(isLoad);
		strBuf.nl();
		strBuf.printf(" [[ Sub message:\n %s ]]\n", subMessage->str().c_str());
		return strBuf;
	}
	
	//// tpLoadAvatarMsg
	void tpLoadAvatarMsg::store(tBBuf &t)
	{
		tpLoadCloneMsg::store(t);
		
		uint32_t tmpVal;
		
		tmpVal = t.get8();
		if (tmpVal != 0x00 && tmpVal != 0x01)
			throw txUnexpectedData(_WHERE("plLoadAvatarMsg.isPlayerAvatar must be 0x00 or 0x01 but is 0x%02X", tmpVal));
		isPlayerAvatar = tmpVal;
		
		t.get(unkObj2);
		
		tmpVal = t.get8();
		if (tmpVal != 0x00)
			throw txUnexpectedData(_WHERE("plLoadAvatarMsg.unk5 must be 0x00 but is 0x%02X", tmpVal));
	}
	
	void tpLoadAvatarMsg::stream(tBBuf &t) const
	{
		tpLoadCloneMsg::stream(t);
		
		t.put8(isPlayerAvatar);
		t.put(unkObj2);
		t.put8(0);
	}
	
	tString tpLoadAvatarMsg::str(void) const
	{
		tString strBuf = tpLoadCloneMsg::str();
		strBuf.printBoolean(" Player avatar: ", isPlayerAvatar);
		strBuf.printf(", Unknown object 2: [%s]\n", unkObj2.str().c_str());
		return strBuf;
	}
	
	//// tpParticleTransferMsg
	void tpParticleTransferMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		t.get(unkObj1);
		count = t.get16();
	}
	
	void tpParticleTransferMsg::stream(tBBuf &t) const
	{
		tpMessage::stream(t);
		t.put(unkObj1);
		t.put16(count);
	}
	
	tString tpParticleTransferMsg::str(void) const
	{
		tString strBuf = tpMessage::str();
		strBuf.printf(" Unknown object: [%s], Count: %d\n", unkObj1.str().c_str(), count);
		return strBuf;
	}
	
	//// tpAvBrainGenericMsg
	void tpAvBrainGenericMsg::store(tBBuf &t)
	{
		tpAvatarMsg::store(t);
		
		unk3 = t.get32();
		unk4 = t.get32();
		unk5 = t.get8();
		
		float varFloat = t.getFloat();
		if (varFloat != 0.0) throw txUnexpectedData(_WHERE("plAvBrainGenericMsg.unk6 must be 0.0, not %f", varFloat));
		
		unk7 = t.get8();
		unk8 = t.get8();
		unk9 = t.getFloat();
	}
	
	void tpAvBrainGenericMsg::stream(tBBuf &t) const
	{
		tpAvatarMsg::stream(t);
		t.put32(unk3);
		t.put32(unk4);
		t.put8(unk5);
		t.putFloat(0.0); // unk6
		t.put8(unk7);
		t.put8(unk8);
		t.putFloat(unk9);
	}
	
	tString tpAvBrainGenericMsg::str(void) const
	{
		tString strBuf = tpAvatarMsg::str();
		strBuf.printf(" Unkonw 5: %d, Unknown 8: %d, Unknown 9: %f\n", unk5, unk8, unk9);
		return strBuf;
	}
	
	//// tpServerReplyMsg
	void tpServerReplyMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		replyType = t.get32();
	}
	
	void tpServerReplyMsg::stream(tBBuf &t) const
	{
		tpMessage::stream(t);
		t.put32(replyType);
	}
	
	tString tpServerReplyMsg::str(void) const
	{
		tString strBuf = tpMessage::str();
		strBuf.printf(" Reply Type: %d\n", replyType);
		return strBuf;
	}
	
	//// tpKIMsg
	tpKIMsg::tpKIMsg(const tUruObjectRef &sender, const tString &senderName, uint32_t senderKi, const tString &text)
	 : tpMessage(pfKIMsg, sender), senderName(senderName), senderKi(senderKi), text(text)
	{
		messageType = 0;
	}
	
	void tpKIMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		
		uint32_t tmpIntVal;
		float tmpFloatVal;
		
		tmpIntVal = t.get8();
		if (tmpIntVal != 0) throw txUnexpectedData(_WHERE("pfKIMsg.unk3 must be 0 but is %d", tmpIntVal));
		
		t.get(senderName);
		senderKi = t.get32();
		t.get(text);
		messageType = t.get32();
		
		tmpFloatVal = t.getFloat();
		if (tmpFloatVal != 0.0) throw txUnexpectedData(_WHERE("pfKIMsg.unk4 must be 0.0 but is %f", tmpFloatVal));
		tmpIntVal = t.get32();
		if (tmpIntVal != 0) throw txUnexpectedData(_WHERE("pfKIMsg.unk5 must be 0 but is %d", tmpIntVal));
	}
	
	void tpKIMsg::stream(tBBuf &t) const
	{
		tpMessage::stream(t);
		t.put8(0); // unk3
		t.put(senderName);
		t.put32(senderKi);
		t.put(text);
		t.put32(messageType);
		t.putFloat(0.0); // unk4
		t.put32(0); // unk5
	}
	
	tString tpKIMsg::str(void) const
	{
		tString strBuf = tpMessage::str();
		strBuf.printf(" Sender: %s (KI: %d)\n", senderName.c_str(), senderKi);
		strBuf.printf(" Text: %s, Message Type: 0x%08X\n", text.c_str(), messageType);
		return strBuf;
	}
	
	//// tpAvatarInputStateMsg
	void tpAvatarInputStateMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		state = t.get16();
	}
	
	void tpAvatarInputStateMsg::stream(tBBuf &t) const
	{
		tpMessage::stream(t);
		t.put16(state);
	}
	
	tString tpAvatarInputStateMsg::str(void) const
	{
		tString strBuf = tpMessage::str();
		strBuf.printf(" State: %d\n", state);
		return strBuf;
	}

} //end namespace alc

