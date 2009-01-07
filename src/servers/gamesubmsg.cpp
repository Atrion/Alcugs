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
#define __U_GAMESUBMSG_ID "$Id$"

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/msgparsers.h>

////extra includes
#include "gamesubmsg.h"

#include <alcdebug.h>

namespace alc {

	//// tLoadCloneMsg
	void tLoadCloneMsg::store(tBBuf &t)
	{
		Byte byteVal;
		U16 u16Val;
		U32 u32Val;
		tUStr strVal(5); // 5 = inverted UruString
		
		byteVal = t.getByte();
		if (byteVal != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk1 must be 0x00 but is 0x%02X", byteVal));
		u32Val = t.getU32();
		if (u32Val != 1) throw txProtocolError(_WHERE("LoadCloneMsg.unk2 must be 0x00000001 but is 0x%08X", u32Val));
		u32Val = t.getU32();
		if (u32Val != 1) throw txProtocolError(_WHERE("LoadCloneMsg.unk3 must be 0x00000001 but is 0x%08X", u32Val));
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk4 must be 0x00000000 but is 0x%08X", u32Val));
		
		u16Val = t.getU16(); // 0x0052 = plNetClientMgr
		if (u16Val != 0x0052) throw txProtocolError(_WHERE("LoadCloneMsg.keyType must be 0x0052 but is 0x%04X", u16Val));
		t.get(strVal);
		if (strVal != "kNetClientMgr_KEY")
			throw txProtocolError(_WHERE("LoadCloneMsg.keyTypeString must be \"kNetClientMgr_KEY\" but is \"%s\"", strVal.c_str()));
		
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk5 must be 0x00000000 but is 0x%08X", u32Val));
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk6 must be 0x00000000 but is 0x%08X", u32Val));
		
		unk7 = t.getU32();
		if (loadAvatarMsg) {
			if (unk7 != 0x00000840 && unk7 != 0x00000040)
				throw txProtocolError(_WHERE("LoadCloneMsg.unk7 must be 0x00000840 or 0x00000040 but is 0x%08X", unk7));
		}
		else if (unk7 != 0x00000AC0 && unk7 != 0x00000040) {
			throw txProtocolError(_WHERE("LoadCloneMsg.unk7 must be 0x00000AC0 but is 0x%08X", unk7));
		}
		
		byteVal = t.getByte(); // might some kind of object present flag
		if (byteVal != 1) throw txProtocolError(_WHERE("LoadCloneMsg.unk8 must be 0x01 but is 0x%02X", byteVal));
		
		t.get(clonedObj);
		if (!clonedObj.hasCloneId) throw txProtocolError(_WHERE("LoadCloneMsg.clonedObj must have the clone ID set"));
		
		u32Val = t.getU32();
		if (u32Val != 1) throw txProtocolError(_WHERE("LoadCloneMsg.unk9 must be 0x00000001 but is 0x%08X", u32Val));
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk10 must be 0x00000000 but is 0x%08X", u32Val));
		
		u16Val = t.getU16(); // 0x00F4 = plAvatarMgr
		if (u16Val != 0x00F4) throw txProtocolError(_WHERE("LoadCloneMsg.keyValue must be 0x0052 but is 0x%04X", u16Val));
		t.get(strVal);
		if (strVal != "kAvatarMgr_KEY")
			throw txProtocolError(_WHERE("LoadCloneMsg.keyValueString must be \"kAvatarMgr_KEY\" but is \"%s\"", strVal.c_str()));
		
		u32Val = t.getU32(); // nodeId
		if (u32Val != clonedObj.clonePlayerId)
			throw txProtocolError(_WHERE("LoadCloneMsg.nodeId (%d) must be the same as the cloneObj.clonePlayerId (%d)", u32Val, clonedObj.clonePlayerId));
		
		u32Val = t.getU32();
		if (loadAvatarMsg) {
			if (u32Val != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk11 must be 0x00000000 but is 0x%08X", u32Val));
		}
		else if (u32Val != clonedObj.clonePlayerId) {
			throw txProtocolError(_WHERE("LoadCloneMsg.unk11 must be the same as nodeId (%d) but is %d", u32Val, clonedObj.clonePlayerId));
		}
		byteVal = t.getByte();
		if (byteVal != 1) throw txProtocolError(_WHERE("LoadCloneMsg.unk12 must be 0x01 but is 0x%02X", byteVal));
		
		byteVal = t.getByte();
		if (byteVal != 0x00 && byteVal != 0x01) throw txProtocolError(_WHERE("LoadCloneMsg.isLoad must be 0x00 or 0x01 but is 0x%02X", byteVal));
		isLoad = byteVal;
		
		unk13 = t.getU16();
		if (loadAvatarMsg) {
			if (unk13 != 0x8000) throw txProtocolError(_WHERE("LoadCloneMsg.unk13 must be 0x8000 but is 0x%04X", unk13));
		}
		else {
			if (unk13 != 0x8000 && unk13 != 0x032E)
				throw txProtocolError(_WHERE("LoadCloneMsg.unk13 must be 0x8000 or 0x032E but is 0x%04X", unk13));
			else if (!isLoad && unk13 != 0x8000)
				throw txProtocolError(_WHERE("An unloading plLoadCLoneMsg must have an unk13 of 0x8000"));
		}
		
		if ( !(loadAvatarMsg == false && unk13 == 0x8000) ) { // if this is NOT a plLoadCloneMsg with unk13 = 0x8000, we can go on
			byteVal = t.getByte();
			if (byteVal != 0x00 && byteVal != 0x01)
				throw txProtocolError(_WHERE("LoadCloneMsg.isPlayerAvatar must be 0x00 or 0x01 but is 0x%02X", byteVal));
			isPlayerAvatar = byteVal;
			if (!loadAvatarMsg && isPlayerAvatar)
				throw txProtocolError(_WHERE("A plLoadCloneMsg must have isPlayerAvatar set to false"));
			
			if (unk13 == 0x032E) { // it's a plLoadCloneMsg with unk13 = 0x032E
				u16Val = t.getU16();
				if (u16Val != 1) throw txProtocolError(_WHERE("LoadCloneMsg.unk14 must be 0x0001 but is 0x%04X", u16Val));
				u16Val = t.getU16();
				if (u16Val != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk15 must be 0x0000 but is 0x%04X", u16Val));
			}
			
			byteVal = t.getByte();
			if (byteVal != 0 && byteVal != 1)
				throw txProtocolError(_WHERE("LoadCloneMsg.hasParentObj must be 0x00 or 0x01 but is 0x%02X", byteVal));
			hasParentObj = byteVal;
			if (isPlayerAvatar && hasParentObj) throw txProtocolError(_WHERE("Player avatars must not have a parent object"));
			else if (!isLoad && hasParentObj) throw txProtocolError(_WHERE("Load message must not have a parent object"));
			else if (unk13 == 0x032E && !hasParentObj) throw txProtocolError(_WHERE("Messages with unk13 = 0x032E must have a parent object"));
			
			if (hasParentObj) t.get(parentObj);
			
			if (unk13 == 0x8000) { // it's a plLoadAvatarMsg with unk13 = 0x8000
				byteVal = t.getByte();
				if (byteVal != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk16 must be 0x00 but is 0x%02X", byteVal));
			}
			else { // it's a plLoadCloneMsg with unk13 = 0x032E
				u32Val = t.getU32();
				if (u32Val != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk17 must be 0x00000000 but is 0x%08X", u32Val));
				u32Val = t.getU32();
				if (u32Val != 0) throw txProtocolError(_WHERE("LoadCloneMsg.unk18 must be 0x00000000 but is 0x%08X", u32Val));
				u32Val = t.getU32();
				if (u32Val != unk7)
					throw txProtocolError(_WHERE("LoadCloneMsg.unk19 must be the same as unk7 (0x%08X) but is 0x%08X", unk7, u32Val));
				byteVal = t.getByte(); // might some kind of object present flag
				if (byteVal != 1) throw txProtocolError(_WHERE("LoadCloneMsg.unk20 must be 0x01 but is 0x%02X", byteVal));
				
				t.get(unkObj);
				
				count = t.getU16();
			}
		}
		
		if (!t.eof())
			throw txProtocolError(_WHERE("Got a plLoadCloneMsg which is too long: %d Bytes remaining after parsing", t.remaining()));
	}
	
	void tLoadCloneMsg::stream(tBBuf &t)
	{
		t.putByte(0); // unk1
		t.putU32(1); // unk2
		t.putU32(1); // unk3
		t.putU32(0); // unk4
		
		t.putU16(0x0052); // keyType = plNetClientMgr
		tUStr keyTypeStr("kNetClientMgr_KEY", 5); // 5 = inverted UruString
		t.put(keyTypeStr);
		
		t.putU32(0); // unk5
		t.putU32(0); // unk6
		t.putU32(unk7);
		t.putByte(1); // unk8
		
		t.put(clonedObj);
		
		t.putU32(1); // unk9
		t.putU32(0); // unk10
		
		t.putU16(0x00F4); // keyValue = plAvatarMgr
		tUStr keyValueStr("kAvatarMgr_KEY", 5); // 5 = inverted UruString
		t.put(keyValueStr);
		
		t.putU32(clonedObj.clonePlayerId); // nodeId
		
		if (loadAvatarMsg)
			t.putU32(0); // unk11
		else
			t.putU32(clonedObj.clonePlayerId); // unk11
		t.putByte(1); // unk12
		
		t.putByte(isLoad);
		
		t.putU16(unk13); // unk13
		
		if (loadAvatarMsg == false && unk13 == 0x8000) return; // no further data for these
		
		t.putByte(isPlayerAvatar);
		
		if (unk13 == 0x032E) { // it's a plLoadCloneMsg with unk13 = 0x032E
			t.putU16(1); // unk14
			t.putU16(0); // unk15
		}
		
		t.putByte(hasParentObj);
		if (hasParentObj) t.put(parentObj);
		
		if (unk13 == 0x8000) { // it's a plLoadAvatarMsg with unk13 = 0x8000
			t.putByte(0); // unk16
		}
		else { // it's a plLoadCloneMsg with unk13 = 0x032E
			t.putU32(0); // unk17
			t.putU32(0); // unk18
			t.putU32(unk7); // unk19 = unk7
			t.putByte(1); // unk20
			t.put(unkObj);
			t.putU16(count);
		}
	}
	
	void tLoadCloneMsg::checkNetMsg(tmLoadClone &loadClone)
	{
		if (clonedObj != loadClone.obj)
			throw txProtocolError(_WHERE("LoadCloneMsg.clonedObj must be the same as LoadClone.obj, but these are different: %s != %s", clonedObj.str(), loadClone.obj.str()));
		if (isLoad != loadClone.isLoad)
			throw txProtocolError(_WHERE("LoadCloneMsg.isLoad must be the same as LoadClone.isLoad, but these are different: %d != %d", isLoad, loadClone.isLoad));
		if ( !(loadAvatarMsg == false && unk13 == 0x8000) ) { // if this is NOT a plLoadCloneMsg with unk13 = 0x8000, we can go on
			if (isPlayerAvatar != loadClone.isPlayerAvatar)
				throw txProtocolError(_WHERE("LoadCloneMsg.isPlayerAvatar must be the same as LoadClone.isPlayerAvatar, but these are different: %d != %d", isPlayerAvatar, loadClone.isPlayerAvatar));
		}
		else
			if (loadClone.isPlayerAvatar)
				throw txProtocolError(_WHERE("LoadCloneMsg.isPlayerAvatar must be false for plLoadCloneMsg with unk13 = 0x800, but it is true"));
	}
	
	tmLoadClone tLoadCloneMsg::createNetMsg(tNetSession *u, bool isInitial)
	{
		tmLoadClone loadClone(u, clonedObj, isPlayerAvatar, isLoad, isInitial);
		if (loadAvatarMsg) loadClone.message.putU16(0x03AC); // plLoadAvatarMsg
		else               loadClone.message.putU16(0x024E); // plLoadCloneMsg
		stream(loadClone.message);
		return loadClone;
	}

} //end namespace alc

