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

#include <alcugs.h>

////extra includes

#include "alcdebug.h"

namespace alc {

	//// tpMessage
	tpMessage::tpMessage(U16 type, const tUruObjectRef &parentObj) : tpObject(type), parentObj(parentObj)
	{
		// the reference list stays empty
		unk1 = unk2 = flags = 0;
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
		t.get(parentObj);
		U32 refCount = t.getU32();
		// read array of references - perhaps put this into a helper function?
		references.clear();
		references.reserve(refCount);
		tReferenceList::iterator it;
		for (U32 i = 0; i < refCount; ++i) {
			it = references.insert(references.end(), tUruObjectRef());
			t.get(*it);
		}
		// remaining values
		unk1 = t.getU32();
		if (unk1 != 0) throw txUnexpectedData(_WHERE("plMessage.unk1 must be 0 but is %d", unk1));
		unk2 = t.getU32();
		if (unk2 != 0) throw txUnexpectedData(_WHERE("plMessage.unk1 must be 0 but is %d", unk2));
		flags = t.getU32();
	}
	
	void tpMessage::stream(tBBuf &t)
	{
		t.put(parentObj);
		t.putU32(references.size());
		for (tReferenceList::iterator it = references.begin(); it != references.end(); ++it)
			t.put(*it);
		t.putU32(unk1);
		t.putU32(unk2);
		t.putU32(flags);
	}
	
	void tpMessage::toString()
	{
		strBuf.printf(" Parent: [%s]\n", parentObj.str());
		int nr = 1;
		for (tReferenceList::iterator it = references.begin(); it != references.end(); ++it, ++nr) {
			strBuf.printf(" Reference %d: [%s]\n", nr, it->str());
		}
		strBuf.printf(" Unk1: %d, Unk2: %d, Flags: 0x%08X\n", unk1, unk2, flags);
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
		
		unk3 = t.getU32();
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
		
		if (getType() == plLoadCloneMsg && !t.eof()) // there may be subclasses
			throw txUnexpectedData(_WHERE("Got a plLoadCloneMsg which is too long: %d Bytes remaining after parsing", t.remaining()));
	}
	
	void tpLoadCloneMsg::stream(tBBuf &t)
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
	
	void tpLoadCloneMsg::toString()
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
		
		if (!t.eof())
			throw txUnexpectedData(_WHERE("Got a plLoadAvatarMsg which is too long: %d Bytes remaining after parsing", t.remaining()));
	}
	
	void tpLoadAvatarMsg::stream(tBBuf &t)
	{
		tpLoadCloneMsg::stream(t);
		
		t.putByte(isPlayerAvatar);
		t.put(unkObj2);
		t.putByte(0);
	}
	
	void tpLoadAvatarMsg::toString()
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
	
	void tpParticleTransferMsg::stream(tBBuf &t)
	{
		tpMessage::stream(t);
		t.put(unkObj1);
		t.putU16(count);
	}
	
	void tpParticleTransferMsg::toString()
	{
		tpMessage::toString();
		strBuf.printf(" Unknown object: [%s], Count: %d\n", unkObj1.str(), count);
	}
	
	//// tpServerReplyMsg
	void tpServerReplyMsg::store(tBBuf &t)
	{
		tpMessage::store(t);
		unk3 = t.getU32();
	}
	
	void tpServerReplyMsg::stream(tBBuf &t)
	{
		tpMessage::stream(t);
		t.putU32(unk3);
	}
	
	void tpServerReplyMsg::toString()
	{
		tpMessage::toString();
		strBuf.printf(" Unknown 3: %d\n", unk3);
	}

} //end namespace alc

