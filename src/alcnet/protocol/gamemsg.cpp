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
#define __U_GAMEMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "alcnet.h"
#include "protocol/gamemsg.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	//// tMemberInfo
	tMemberInfo::tMemberInfo(tNetSession *u, const tUruObject &obj) : avatar(u->avatar), obj(obj)
	{
		avatar.setVersion(0); // normal UruString
		ki = u->ki;
		ip = u->getIp();
		port = u->getPort();
	}
	
	void tMemberInfo::store(tBBuf &t)
	{
		throw txProtocolError(_WHERE("Storing a tMemberInfo is not supported"));
	}
	
	void tMemberInfo::stream(tBBuf &t)
	{
		t.putU32(0x00000020); //unknown, seen 0x20 and 0x22 (seems to be a flag)
		t.putU16(0x03EA); //always seen that value
		t.putU32(ki);
		t.put(avatar);
		t.putByte(0x00); //always seen that value
		t.putByte(0x03); //always seen that value
		t.putU32(ntohl(ip));
		t.putU16(ntohs(port));
		t.putByte(0x00); //always seen that value
		t.put(obj);
	}
	
	const Byte *tMemberInfo::str(void)
	{
		dbg.clear();
		dbg.printf("Avatar Name: %s, IP: %s:%i, Object reference: [%s]", avatar.c_str(), alcGetStrIp(ip), ntohs(port), obj.str());
		return dbg.c_str();
	}

	//// tmJoinReq
	tmJoinReq::tmJoinReq(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmJoinReq::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX | plNetKi)) throw txProtocolError(_WHERE("X or KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
	}
	
	//// tmJoinAck
	tmJoinAck::tmJoinAck(tNetSession *u, U32 x) : tmMsgBase(NetMsgJoinAck, plNetAck | plNetCustom | plNetKi | plNetX | plNetFirewalled, u)
	{
		this->x = x;
		ki = u->ki;
	}
	
	void tmJoinAck::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		
		t.putU16(0); // unknown flag ("joinOrder" and "ExpLevel")
		t.put(sdl);
	}
	
	//// tmGameMessage
	tmGameMessage::tmGameMessage(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmGameMessage::tmGameMessage(U16 cmd, U32 flags, tNetSession *u) : tmMsgBase(cmd, flags, u)
	{ memset(header, 0, 5); }
	
	tmGameMessage::tmGameMessage(U16 cmd, U32 flags, tNetSession *u, tmGameMessage &msg)
	 : tmMsgBase(cmd, flags, u), message(msg.message)
	{ copyBaseProps(msg); }
	
	tmGameMessage::tmGameMessage(tNetSession *u, tmGameMessage &msg)
	 : tmMsgBase(NetMsgGameMessage, plNetAck | plNetKi, u), message(msg.message)
	{ copyBaseProps(msg); }
	
	tmGameMessage::tmGameMessage(tNetSession *u, U32 ki) : tmMsgBase(NetMsgGameMessage, plNetAck | plNetKi, u)
	{
		this->ki = ki;
		memset(header, 0, 5);
	}
	
	void tmGameMessage::copyBaseProps(tmGameMessage &msg)
	{
		if (!msg.hasFlags(plNetAck)) unsetFlags(plNetAck);
		memcpy(header, msg.header, 5);
		ki = msg.ki;
	}
	
	void tmGameMessage::store(tBBuf &t)
	{
		message.clear();
		
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || (cmd != NetMsgCustomDirectedFwd && ki != u->ki)) // don't kick connection game <-> tracking
			throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		memcpy(header, t.read(5), 5);
		U32 gameMsgSize = t.getU32();
		message.write(t.read(gameMsgSize), gameMsgSize); // that's the message itself
		Byte unk = t.getByte();
		if (unk != 0x00)
			throw txProtocolError(_WHERE("Unexpected NetMsgGameMessage.unk of 0x%02X (should be 0x00)", unk));
	}
	
	void tmGameMessage::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.write(header, 5);
		t.putU32(message.size());
		t.put(message);
		t.putByte(0); // unk
	}
	
	U16 tmGameMessage::getSubMsgType()
	{
		message.rewind();
		return message.getU16();
	}
	
	//// tmGameMessageDirected
	tmGameMessageDirected::tmGameMessageDirected(tNetSession *u) : tmGameMessage(u)
	{ }
	
	tmGameMessageDirected::tmGameMessageDirected(U16 cmd, U32 flags, tNetSession *u) : tmGameMessage(cmd, flags, u)
	{ }
	
	tmGameMessageDirected::tmGameMessageDirected(U16 cmd, U32 flags, tNetSession *u, tmGameMessageDirected &msg)
	 : tmGameMessage(cmd, flags, u, msg), recipients(msg.recipients)
	{ }
	
	tmGameMessageDirected::tmGameMessageDirected(tNetSession *u, tmGameMessageDirected &msg)
	 : tmGameMessage(NetMsgGameMessageDirected, plNetAck | plNetKi | plNetDirected, u, msg), recipients(msg.recipients)
	{ }
	
	void tmGameMessageDirected::store(tBBuf &t)
	{
		tmGameMessage::store(t);
		if (cmd == NetMsgGameMessageDirected && !hasFlags(plNetDirected)) throw txProtocolError(_WHERE("Directed flag missing"));
		// get list of recipients
		Byte nRecipients = t.getByte();
		recipients.clear();
		recipients.reserve(nRecipients); // avoid re-allocating memory
		for (int i = 0; i < nRecipients; ++i) recipients.push_back(t.getU32());
	}
	
	void tmGameMessageDirected::stream(tBBuf &t)
	{
		tmGameMessage::stream(t);
		t.putByte(recipients.size());
		for (tRecList::iterator it = recipients.begin(); it != recipients.end(); ++it)
			t.putU32(*it);
	}
	
	//// tmLoadClone
	tmLoadClone::tmLoadClone(tNetSession *u) : tmGameMessage(u)
	{ }
	
	tmLoadClone::tmLoadClone(tNetSession *u, tmLoadClone &msg)
	 : tmGameMessage(NetMsgLoadClone, plNetAck | plNetKi, u, msg), obj(msg.obj)
	{
		isPlayerAvatar = msg.isPlayerAvatar;
		isLoad = msg.isLoad;
		isInitial = msg.isInitial;
	}
	
	tmLoadClone::tmLoadClone(tNetSession *u, tUruObject obj, bool isPlayerAvatar, bool isLoad, bool isInitial)
	 : tmGameMessage(NetMsgLoadClone, plNetAck | plNetKi, u), obj(obj)
	{
		ki = obj.clonePlayerId;
		this->isPlayerAvatar = isPlayerAvatar;
		this->isLoad = isLoad;
		this->isInitial = isInitial;
	}
	
	void tmLoadClone::store(tBBuf &t)
	{
		tmGameMessage::store(t);
		
		// check if header is all zero
		Byte zero[5];
		memset(zero, 0, 5);
		if (memcmp(header, zero, 5) != 0)
			throw txProtocolError(_WHERE("The header of a NetMsgLoadClone must be all zero"));
		
		t.get(obj);
		if (!obj.hasCloneId) throw txProtocolError(_WHERE("The UruObject of a NetMsgLoadClone must have the clone ID set"));
		if (obj.clonePlayerId != ki)
			throw txProtocolError(_WHERE("ClonePlayerID of loaded clone must be the same as Player KI (%d) but is %d", ki, obj.clonePlayerId));
		
		Byte playerAvatar = t.getByte();
		if (playerAvatar != 0x00 && playerAvatar != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgLoadClone.playerAvatar of 0x%02X (should be 0x00 or 0x01)", playerAvatar));
		isPlayerAvatar = playerAvatar;
		
		Byte load = t.getByte();
		if (load != 0x00 && load != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgLoadClone.load of 0x%02X (should be 0x00 or 0x01)", load));
		isLoad = load;
		
		Byte initial = t.getByte();
		if (initial != 0x00 && initial != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgLoadClone.initial of 0x%02X (should be 0x00 or 0x01)", initial));
		isInitial = initial;
	}
	
	void tmLoadClone::stream(tBBuf &t)
	{
		tmGameMessage::stream(t);
		t.put(obj);
		t.putByte(isPlayerAvatar);
		t.putByte(isLoad);
		t.putByte(isInitial);
	}
	
	void tmLoadClone::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Object reference: [%s], is player avatar: ", obj.str());
		if (isPlayerAvatar) dbg.printf("yes");
		else                dbg.printf("no");
		dbg.printf(", is load: ");
		if (isLoad) dbg.printf("yes");
		else        dbg.printf("no");
		dbg.printf(", is initial age state: ");
		if (isInitial) dbg.printf("yes");
		else           dbg.printf("no");
	}
	
	//// tmPagingRoom
	tmPagingRoom::tmPagingRoom(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmPagingRoom::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		U32 format = t.getU32();
		if (format != 1)
			throw txProtocolError(_WHERE("NetMsgPagingRoom.format must be 1, but is %d", format));
		pageId = t.getU32();
		pageType = t.getU16();
		t.get(pageName);
		Byte pageFlag = t.getByte();
		if (pageFlag == 0x00 || pageFlag == 0x01) isPageOut = pageFlag;
		else
			throw txProtocolError(_WHERE("NetMsgPagingRoom.pageFlag must be 0x00 or 0x01 but is 0x%02X", pageFlag));
	}
	
	void tmPagingRoom::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Page ID: 0x%08X, Page Type: 0x%04X, Page Name: %s, Paged out: ", pageId, pageType, pageName.c_str());
		if (isPageOut) dbg.printf("yes");
		else           dbg.printf("no");
	}
	
	//// tmGroupOwner
	tmGroupOwner::tmGroupOwner(tNetSession *u, tPageInfo *page, bool isOwner) : tmMsgBase(NetMsgGroupOwner, plNetAck | plNetCustom, u)
	{
		this->pageId = page->plasmaPageId;
		this->pageType = page->plasmaPageType;
		this->isOwner = isOwner;
	}
	
	void tmGroupOwner::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU32(1); // format
		t.putU32(pageId);
		t.putU16(pageType);
		t.putByte(0x00);
		t.putByte(isOwner);
	}
	
	void tmGroupOwner::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Page ID: 0x%08X, Page Type: 0x%04X, is owner: ", pageId, pageType);
		if (isOwner) dbg.printf("yes");
		else         dbg.printf("no");
	}
	
	//// tmPlayerPage
	tmPlayerPage::tmPlayerPage(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmPlayerPage::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		Byte pageFlag = t.getByte();
		if (pageFlag == 0x00 || pageFlag == 0x01) isPageOut = pageFlag;
		else
			throw txProtocolError(_WHERE("NetMsgPlayerPage.pageFlag must be 0x00 or 0x01 but is 0x%02X", pageFlag));
		t.get(obj);
	}
	
	void tmPlayerPage::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Paged out: ");
		if (isPageOut) dbg.printf("yes");
		else         dbg.printf("no");
		dbg.printf(", Object reference: [%s]", obj.str());
	}
	
	//// tmGameStateRequest
	tmGameStateRequest::tmGameStateRequest(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmGameStateRequest::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		U32 nPages = t.getU32();
		if (nPages == 0 && !hasFlags(plNetStateReq))
			throw txProtocolError(_WHERE("StateReq flag missing"));
		else if (nPages != 0 && hasFlags(plNetStateReq))
			throw txProtocolError(_WHERE("Unexpected StateReq flag"));
		
		pages.clear();
		pages.reserve(nPages);
		tUStr pageName;
		for (U32 i = 0; i < nPages; ++i) {
			pages.push_back(t.getU32());
			t.getU16(); // ignore pageType
			t.get(pageName); // ignore pageName
		}
	}
	
	void tmGameStateRequest::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Explicitly requested pages: %d", pages.size());
	}
	
	//// tmInitialAgeStateSent
	tmInitialAgeStateSent::tmInitialAgeStateSent(tNetSession *u, U32 num) : tmMsgBase(NetMsgInitialAgeStateSent, plNetCustom | plNetAck, u)
	{
		this->num = num;
	}
	
	void tmInitialAgeStateSent::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU32(num);
	}
	
	void tmInitialAgeStateSent::additionalFields()
	{
		dbg.nl();
		dbg.printf(" number of sent states: %d", num);
	}
	
	//// tmMembersListReq
	tmMembersListReq::tmMembersListReq(tNetSession *u) : tmMsgBase(u)
	{ }
	
	//// tmTestAndSet
	tmTestAndSet::tmTestAndSet(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmTestAndSet::store(tBBuf &t)
	{
		Byte flag, state1, state2;
		Byte lockReq;
		U32 unk;
		tUStr trigger, triggered;
	
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		t.get(obj);
		flag = t.getByte();
		if (flag != 0x00)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.flag1 of 0x%02X (should be 0x00)", flag));
		unk = t.getU32();
		if (unk != 0x00000000)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.unk1 of 0x%08X (should be 0x00000000)", unk));
		unk = t.getU32();
		if (unk != 0x0000001D)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.unk2 of 0x%08X (should be 0x0000001D)", unk));
		t.get(trigger);
		if (trigger != "TrigState")
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.Trigger of %s (should be \"TrigState\")", trigger.c_str()));
		unk = t.getU32();
		if (unk != 0x00000001)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.unk3 of 0x%08X (should be 0x00000001)", unk));
		state1 = t.getByte();
		if (state1 != 0x00 && state1 != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.state1 of 0x%02X (should be 0x00 or 0x01)", state1));
		t.get(triggered);
		if (triggered != "Triggered")
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.Triggered of %s (should be \"Triggered\")", triggered.c_str()));
		flag = t.getByte();
		if (flag != 0x02)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.flag2 of 0x%02X (should be 0x02)", flag));
		state2 = t.getByte();
		if (state2 != 0x00 && state2 != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.state2 of 0x%02X (should be 0x00 or 0x01)", state2));
		lockReq = t.getByte();
		if (lockReq != 0x00 && lockReq != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.lockReq of 0x%02X (should be 0x00 or 0x01)", lockReq));
		isLockReq = lockReq;
		
		// now check if the states are a valid combination
		if (state1 == lockReq)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet: state1 (0x%02X) and lockReq (0x%02X) must not be equal", state1, lockReq));
		if (state2 != lockReq)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet: state2 (0x%02X) and lockReq (0x%02X) must be equal", state2, lockReq));
	}
	
	void tmTestAndSet::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Object reference: [%s], Lock requested: ", obj.str());
		if (isLockReq) dbg.printf("yes");
		else         dbg.printf("no");
	}
	
	//// tmRelevanceRegions
	tmRelevanceRegions::tmRelevanceRegions(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmRelevanceRegions::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		U32 unk = t.getU32();
		if (unk != 1) throw txProtocolError(_WHERE("NetMsgRelevanceRegions.unk1 must be 1 but is %d", unk));
		rgnsICareAbout = t.getU32();
		unk = t.getU32();
		if (unk != 1) throw txProtocolError(_WHERE("NetMsgRelevanceRegions.unk2 must be 1 but is %d", unk));
		rgnsImIn = t.getU32();
		if (rgnsImIn > rgnsICareAbout)
			throw txProtocolError(_WHERE("rgnsImIn must be equal or lower than rgnsICareAbout, but %d > %d", rgnsImIn, rgnsICareAbout));
	}
	
	void tmRelevanceRegions::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Regions I care about: %d, Regions I\'m in: %d", rgnsICareAbout, rgnsImIn);
	}
	
	//// tmSDLState
	tmSDLState::tmSDLState(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmSDLState::tmSDLState(tNetSession *u, bool isInitial) : tmMsgBase(NetMsgSDLState, plNetAck, u)
	{
		this->isInitial = isInitial;
	}
	
	void tmSDLState::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		sdl.clear();
		U32 sdlSize = t.remaining()-1;
		sdl.write(t.read(sdlSize), sdlSize);
		Byte initial = t.getByte();
		if (initial == 0x00 || initial == 0x01) isInitial = initial;
		else
			throw txProtocolError(_WHERE("NetMsgSDLState.initial must be 0x00 or 0x01 but is 0x%02X", initial));
	}
	
	void tmSDLState::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(sdl);
		t.putByte(isInitial);
	}
	
	void tmSDLState::additionalFields()
	{
		dbg.nl();
		dbg.printf(" is initial age state: ");
		if (isInitial) dbg.printf("yes");
		else           dbg.printf("no");
	}
	
	//// tmSDLStateBCast
	tmSDLStateBCast::tmSDLStateBCast(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmSDLStateBCast::tmSDLStateBCast(tNetSession *u, tmSDLStateBCast & msg)
	 : tmMsgBase(NetMsgSDLStateBCast, plNetAck | plNetKi | plNetBcast, u), sdl(msg.sdl)
	{
		if (!msg.hasFlags(plNetBcast)) unsetFlags(plNetBcast);
		ki = msg.ki;
	}
	
	void tmSDLStateBCast::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		sdl.clear();
		U32 sdlSize = t.remaining()-2;
		sdl.write(t.read(sdlSize), sdlSize);
		
		Byte unk = t.getByte();
		if (unk != 0x00) // this is most likely "initial", but since BCasts are never sent on initializing, it is always 0
			throw txProtocolError(_WHERE("NetMsgSDLStateBCast.unk1 must be 0x00 but is 0x%02X", unk));
		unk = t.getByte();
		if (unk != 0x01)
			throw txProtocolError(_WHERE("NetMsgSDLStateBCast.unk2 must be 0x01 but is 0x%02X", unk));
	}
	
	void tmSDLStateBCast::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(sdl);
		t.putByte(0x00); // unk1 (initial?)
		t.putByte(0x01); // unk2
	}
	
	//// tmSetTimeout
	tmSetTimeout::tmSetTimeout(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmSetTimeout::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		U32 unk = t.getU32();
		if (unk != 0x43340000) throw txProtocolError(_WHERE("NetMsgSetTimeout.unk must be 0x43340000 but is 0x%08X", unk));
		// it seems this means 180sec, but I have no clue how
	}
	
	//// tmMembersList
	tmMembersList::tmMembersList(tNetSession *u) : tmMsgBase(NetMsgMembersList, plNetAck | plNetCustom, u)
	{ }
	
	void tmMembersList::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.putU16(members.size());
		for (tMemberList::iterator i = members.begin(); i != members.end(); ++i)
			t.put(*i);
	}
	
	void tmMembersList::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Number of other players: %d", members.size());
	}
	
	//// tmMemberUpdate
	tmMemberUpdate::tmMemberUpdate(tNetSession *u, tNetSession *memberSession, const tUruObject &obj, bool isJoined)
	 : tmMsgBase(NetMsgMemberUpdate, plNetAck | plNetCustom, u), info(memberSession, obj)
	{
		this->isJoined = isJoined;
	}
	
	void tmMemberUpdate::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.put(info);
		t.putByte(isJoined);
	}
	
	void tmMemberUpdate::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Member Info: [%s], is joined: ", info.str());
		if (isJoined) dbg.printf("yes");
		else          dbg.printf("no");
	}

} //end namespace alc

