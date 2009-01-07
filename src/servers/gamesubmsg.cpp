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

	//// tLoadAvatarMsg
	void tLoadAvatarMsg::store(tBBuf &t)
	{
		Byte byteVal;
		U16 u16Val;
		U32 u32Val;
		tUStr strVal(5); // 5 = inverted UruString
		
		byteVal = t.getByte();
		if (byteVal != 0) throw txProtocolError(_WHERE("CloneMsgBase.unk1 must be 0x00 but is 0x%02X", byteVal));
		u32Val = t.getU32();
		if (u32Val != 1) throw txProtocolError(_WHERE("CloneMsgBase.unk2 must be 0x00000001 but is 0x%08X", u32Val));
		u32Val = t.getU32();
		if (u32Val != 1) throw txProtocolError(_WHERE("CloneMsgBase.unk3 must be 0x00000001 but is 0x%08X", u32Val));
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("CloneMsgBase.unk4 must be 0x00000000 but is 0x%08X", u32Val));
		
		u16Val = t.getU16(); // 0x0052 = plNetClientMgr
		if (u16Val != 0x0052) throw txProtocolError(_WHERE("CloneMsgBase.keyType must be 0x0052 but is 0x%04X", u16Val));
		t.get(strVal);
		if (strVal != "kNetClientMgr_KEY")
			throw txProtocolError(_WHERE("CloneMsgBase.keyTypeString must be \"kNetClientMgr_KEY\" but is \"%s\"", strVal.c_str()));
		
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("CloneMsgBase.unk5 must be 0x00000000 but is 0x%08X", u32Val));
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("CloneMsgBase.unk6 must be 0x00000000 but is 0x%08X", u32Val));
		
		unk7 = t.getU32();
		if (unk7 != 0x00000840 && unk7 != 0x00000040)
			throw txProtocolError(_WHERE("CloneMsgBase.unk7 must be 0x00000840 or 0x00000040 but is 0x%08X", unk7));
		
		byteVal = t.getByte();
		if (byteVal != 1) throw txProtocolError(_WHERE("CloneMsgBase.unk8 must be 0x01 but is 0x%02X", byteVal));
		
		t.get(clonedObj);
		if (!clonedObj.hasCloneId) throw txProtocolError(_WHERE("CloneMsgBase.clonedObj must have the clone ID set"));
		
		u32Val = t.getU32();
		if (u32Val != 1) throw txProtocolError(_WHERE("CloneMsgBase.unk9 must be 0x00000001 but is 0x%08X", u32Val));
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("CloneMsgBase.unk10 must be 0x00000000 but is 0x%08X", u32Val));
		
		u16Val = t.getU16(); // 0x00F4 = plAvatarMgr
		if (u16Val != 0x00F4) throw txProtocolError(_WHERE("CloneMsgBase.keyValue must be 0x0052 but is 0x%04X", u16Val));
		t.get(strVal);
		if (strVal != "kAvatarMgr_KEY")
			throw txProtocolError(_WHERE("CloneMsgBase.keyValueString must be \"kAvatarMgr_KEY\" but is \"%s\"", strVal.c_str()));
		
		u32Val = t.getU32(); // nodeId
		if (u32Val != clonedObj.clonePlayerId)
			throw txProtocolError(_WHERE("CloneMsgBase.nodeId (%d) must be the same as the cloneObj.clonePlayerId (%d)", u32Val, clonedObj.clonePlayerId));
		
		u32Val = t.getU32();
		if (u32Val != 0) throw txProtocolError(_WHERE("CloneMsgBase.unk11 must be 0x00000000 but is 0x%08X", u32Val));
		byteVal = t.getByte();
		if (byteVal != 1) throw txProtocolError(_WHERE("CloneMsgBase.unk12 must be 0x01 but is 0x%02X", byteVal));
		
		byteVal = t.getByte();
		if (byteVal != 0x00 && byteVal != 0x01) throw txProtocolError(_WHERE("CloneMsgBase.isLoad must be 0x00 or 0x01 but is 0x%02X", byteVal));
		isLoad = byteVal;
		
		u16Val = t.getU16();
		if (u16Val != 0x8000) throw txProtocolError(_WHERE("CloneMsgBase.unk13 must be 0x8000 but is 0x%04X", u16Val));
		
		byteVal = t.getByte();
		if (byteVal != 0x00 && byteVal != 0x01)
			throw txProtocolError(_WHERE("CloneMsgBase.isPlayerAvatar must be 0x00 or 0x01 but is 0x%02X", byteVal));
		isPlayerAvatar = byteVal;
		
		byteVal = t.getByte();
		if (byteVal != 0 && byteVal != 1)
			throw txProtocolError(_WHERE("CloneMsgBase.hasParentObj must be 0x00 or 0x01 but is 0x%02X", byteVal));
		hasParentObj = byteVal;
		if (isPlayerAvatar && hasParentObj) throw txProtocolError(_WHERE("Player avatars must not have a parent object"));
		
		if (hasParentObj) t.get(parentObj);
		
		byteVal = t.getByte();
		if (byteVal != 0) throw txProtocolError(_WHERE("CloneMsgBase.unk14 must be 0x00 but is 0x%02X", byteVal));
	}
	
	void tLoadAvatarMsg::stream(tBBuf &t)
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
		
		t.putU32(0); // unk11
		t.putByte(1); // unk12
		
		t.putByte(isLoad);
		
		t.putU16(0x8000); // unk13
		
		t.putByte(isPlayerAvatar);
		t.putByte(hasParentObj);
		if (hasParentObj) t.put(parentObj);
		
		t.putByte(0); // unk14
	}
	
	void tLoadAvatarMsg::check(tmLoadClone &loadClone)
	{
		if (clonedObj != loadClone.obj)
			throw txProtocolError(_WHERE("CloneMsgBase.clonedObj must be the same as LoadClone.obj, but these are different: %s != %s", clonedObj.str(), loadClone.obj.str()));
		if (isLoad != loadClone.isLoad)
			throw txProtocolError(_WHERE("CloneMsgBase.isLoad must be the same as LoadClone.isLoad, but these are different: %d != %d", isLoad, loadClone.isLoad));
		if (isPlayerAvatar != loadClone.isPlayerAvatar)
			throw txProtocolError(_WHERE("CloneMsgBase.isPlayerAvatar must be the same as LoadClone.isPlayerAvatar, but these are different: %d != %d", isPlayerAvatar, loadClone.isPlayerAvatar));
	}

} //end namespace alc

