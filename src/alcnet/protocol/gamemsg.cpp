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

#include "alcnet.h"
#include "protocol/ext-protocol.h"

////extra includes

#include <alcdebug.h>

namespace alc {

	//// tMemberInfo
	tMemberInfo::tMemberInfo(tNetSession *u, const tUruObject &obj, bool hidePlayer) : avatar(u->avatar), obj(obj)
	{
		ki = u->ki;
		this->hidePlayer = hidePlayer;
		buildType = u->buildType;
		ip = u->getIp();
		port = u->getPort();
	}
	
	void tMemberInfo::store(tBBuf &t)
	{
		throw txProtocolError(_WHERE("Storing a tMemberInfo is not supported"));
	}
	
	void tMemberInfo::stream(tBBuf &t) const
	{
		t.putU32(0x00000020); // unknown, seen 0x20 and 0x22 - a flag
		// begin of the plClientGuid
		t.putU16(0x03EA); // a flag defining the content
		/* according to libPlasma, 0x03EA is the combination of:
		  KI = 0x0002, CCR Level = 0x0008,
		  Build Type = 0x0020, Player Name = 0x0040, Source Address = 0x0080,
		  Source Port = 0x0100, Reserved = 0x0200 */
		t.putU32(ki);
		t.put(avatar);
		t.putByte(hidePlayer); // CCR flag - when set to 1, the player is hidden on the age list
		t.putByte(buildType);
		t.putU32(ntohl(ip));
		t.putU16(ntohs(port));
		t.putByte(0x00); // always seen that value - might be the reserved field, but according to libPlasma that would be two bytes...
		// end of the plClientGuid
		t.put(obj);
	}
	
	const char *tMemberInfo::str(void)
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
		
		//Unfortunately it looks like the IP address is transmitted in
		// network byte order. This means that if we just use getU32(),
		// on little-endian systems ip will be in network order and on
		// big-endian systems it will be byte-swapped from *both*
		// network and host order (which are the same).
		ip=letoh32(t.getU32());
		//The port is transmitted in little-endian order, so is in host
		// order after getU16().
		port=htons(t.getU16());
	}
	
	void tmJoinReq::additionalFields()
	{
		dbg.nl();
		dbg.printf(" IP: %s:%i", alcGetStrIp(ip), ntohs(port));
	}
	
	//// tmJoinAck
	tmJoinAck::tmJoinAck(tNetSession *u, U32 x, tBaseType *sdl)
	 : tmMsgBase(NetMsgJoinAck, plNetAck | plNetKi | plNetX/* | plNetFirewalled*/, u)
	{
		this->x = x;
		ki = u->ki;
		sdlStream.put(*sdl);
		sdlStream.compress();
	}
	
	void tmJoinAck::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		
		t.putU16(0); // unknown ("joinOrder" and "ExpLevel")
		t.put(sdlStream);
	}
	
	//// tmGameMessage
	// public constructors
	tmGameMessage::tmGameMessage(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmGameMessage::tmGameMessage(tNetSession *u, const tmGameMessage &msg)
	 : tmMsgBase(NetMsgGameMessage, msg.flags, u), msgStream(msg.msgStream)
	{
		ki = msg.ki;
		msgStream.compress();
	}
	
	tmGameMessage::tmGameMessage(tNetSession *u, U32 ki, tpObject *obj)
	 : tmMsgBase(NetMsgGameMessage, plNetAck | plNetKi, u), msgStream(obj) // this already compresses the stream
	{ this->ki = ki; }
	
	// constructors for sub-classes
	tmGameMessage::tmGameMessage(U16 cmd, tNetSession *u, const tmGameMessage &msg)
	 : tmMsgBase(cmd, msg.flags, u), msgStream(msg.msgStream)
	{
		ki = msg.ki;
		msgStream.compress();
	}
	
	tmGameMessage::tmGameMessage(U16 cmd, tNetSession *u, U32 ki, tpObject *obj)
	 : tmMsgBase(cmd, plNetAck | plNetKi, u), msgStream(obj) // this already compresses the stream
	{ this->ki = ki; }
	
	// methods
	void tmGameMessage::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (cmd != NetMsgCustomDirectedFwd && (ki == 0 || ki != u->ki)) // don't kick connection game <-> tracking
			throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		t.get(msgStream);
		
		Byte hasTime = t.getByte();
		if (hasTime != 0x00)
			throw txProtocolError(_WHERE("Unexpected NetMsgGameMessage.hasTime of 0x%02X (should be 0x00)", hasTime));
	}
	
	void tmGameMessage::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(msgStream);
		t.putByte(0); // hasTime
	}
	
	//// tmGameMessageDirected
	// public constructors
	tmGameMessageDirected::tmGameMessageDirected(tNetSession *u) : tmGameMessage(u)
	{ }
	
	tmGameMessageDirected::tmGameMessageDirected(tNetSession *u, const tmGameMessageDirected &msg)
	 : tmGameMessage(NetMsgGameMessageDirected, u, msg), recipients(msg.recipients)
	{ }
	
	tmGameMessageDirected::tmGameMessageDirected(tNetSession *u, U32 ki, tpObject *obj)
	 : tmGameMessage(NetMsgGameMessageDirected, u, ki, obj)
	{ }
	
	// constructors for sub-classes
	tmGameMessageDirected::tmGameMessageDirected(U16 cmd, tNetSession *u, const tmGameMessageDirected &msg)
	 : tmGameMessage(cmd, u, msg), recipients(msg.recipients)
	{ }
	
	tmGameMessageDirected::tmGameMessageDirected(U16 cmd, tNetSession *u, U32 ki, tpObject *obj)
	 : tmGameMessage(cmd, u, ki, obj)
	{ }
	
	// methods
	void tmGameMessageDirected::store(tBBuf &t)
	{
		tmGameMessage::store(t);
		
		// get list of recipients
		Byte nRecipients = t.getByte();
		recipients.clear();
		recipients.reserve(nRecipients); // avoid re-allocating memory
		for (int i = 0; i < nRecipients; ++i) recipients.push_back(t.getU32());
	}
	
	void tmGameMessageDirected::stream(tBBuf &t) const
	{
		tmGameMessage::stream(t);
		t.putByte(recipients.size());
		for (tRecList::const_iterator it = recipients.begin(); it != recipients.end(); ++it)
			t.putU32(*it);
	}
	
	//// tmLoadClone
	tmLoadClone::tmLoadClone(tNetSession *u) : tmGameMessage(u)
	{ }
	
	tmLoadClone::tmLoadClone(tNetSession *u, const tmLoadClone &msg)
	 : tmGameMessage(NetMsgLoadClone, u, msg), obj(msg.obj)
	{
		isPlayerAvatar = msg.isPlayerAvatar;
		isLoad = msg.isLoad;
		isInitial = msg.isInitial;
	}
	
	tmLoadClone::tmLoadClone(tNetSession *u, tpLoadCloneMsg *subMsg, bool isInitial)
	 : tmGameMessage(NetMsgLoadClone, u, subMsg->clonedObj.obj.clonePlayerId, subMsg), obj(subMsg->clonedObj.obj)
	{
		this->isLoad = subMsg->isLoad;
		this->isInitial = isInitial;
		this->isPlayerAvatar = false;
		if ( subMsg->getType() == plLoadAvatarMsg ) {
			tpLoadAvatarMsg *avMsg = static_cast<tpLoadAvatarMsg *>(subMsg);
			this->isPlayerAvatar = avMsg->isPlayerAvatar;
		}
	}
	
	void tmLoadClone::store(tBBuf &t)
	{
		tmGameMessage::store(t);
		
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
	
	void tmLoadClone::stream(tBBuf &t) const
	{
		tmGameMessage::stream(t);
		t.put(obj);
		t.putByte(isPlayerAvatar);
		t.putByte(isLoad);
		t.putByte(isInitial);
	}
	
	void tmLoadClone::checkSubMsg(tpLoadCloneMsg *subMsg)
	{
		if (obj != subMsg->clonedObj.obj)
			throw txProtocolError(_WHERE("LoadClone.obj must be the same as plLoadCloneMsg.clonedObj, but these are different: %s != %s", obj.str(), subMsg->clonedObj.str()));
		if (isLoad != subMsg->isLoad)
			throw txProtocolError(_WHERE("LoadClone.isLoad must be the same as plLoadCloneMsg.isLoad, but these are different: %d != %d", isLoad, subMsg->isLoad));
		if ( subMsg->getType() == plLoadAvatarMsg ) {
			tpLoadAvatarMsg *avMsg = static_cast<tpLoadAvatarMsg *>(subMsg);
			if (isPlayerAvatar != avMsg->isPlayerAvatar)
				throw txProtocolError(_WHERE("LoadClone.isPlayerAvatar must be the same as plLoadAvatarMsg.isPlayerAvatar, but these are different: %d != %d", isPlayerAvatar, avMsg->isPlayerAvatar));
		}
	}
	
	void tmLoadClone::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Object reference: [%s], player avatar: ", obj.str());
		dbg.printBoolean(isPlayerAvatar);
		dbg.printBoolean(", load: ", isLoad);
		dbg.printBoolean(", initial age state: ", isInitial);
	}
	
	//// tmPagingRoom
	tmPagingRoom::tmPagingRoom(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmPagingRoom::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		U32 n = t.getU32(); // the number of stored rooms
		if (n != 1)
			throw txProtocolError(_WHERE("NetMsgPagingRoom.n must be 1, but is %d", n));
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
		dbg.printBoolean(isPageOut);
	}
	
	//// tmGroupOwner
	tmGroupOwner::tmGroupOwner(tNetSession *u, tPageInfo *page, bool isOwner) : tmMsgBase(NetMsgGroupOwner, plNetAck, u)
	{
		this->pageId = page->pageId;
		this->pageType = page->pageType;
		this->isOwner = isOwner;
	}
	
	void tmGroupOwner::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.putU32(1); // number of sent blocks (Alcugs does not support sending several of them at once)
		// begin of the plNetGroupId
		t.putU32(pageId);
		t.putU16(pageType);
		t.putByte(0x00); // the flags of a plNetGroupId
		// end of the plNetGroupId
		t.putByte(isOwner);
	}
	
	void tmGroupOwner::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Page ID: 0x%08X, Page Type: 0x%04X, owner: ", pageId, pageType);
		dbg.printBoolean(isOwner);
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
		dbg.printBoolean(" Paged out: ", isPageOut);
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
		if (nPages == 0 && !hasFlags(plNetStateReq1))
			throw txProtocolError(_WHERE("StateReq flag missing"));
		else if (nPages != 0 && hasFlags(plNetStateReq1))
			throw txProtocolError(_WHERE("Unexpected StateReq flag"));
		
		pages.clear();
		pages.reserve(nPages);
		tStrBuf pageName;
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
	tmInitialAgeStateSent::tmInitialAgeStateSent(tNetSession *u, U32 num) : tmMsgBase(NetMsgInitialAgeStateSent, plNetAck, u)
	{
		this->num = num;
	}
	
	void tmInitialAgeStateSent::stream(tBBuf &t) const
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
	
	//// tmStreamedObject
	tmStreamedObject::tmStreamedObject(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmStreamedObject::tmStreamedObject(U16 cmd, tNetSession *u, const tmStreamedObject &msg)
	: tmMsgBase(cmd, msg.flags, u), obj(msg.obj), content(msg.content)
	{
		content.compress();
	}
	
	tmStreamedObject::tmStreamedObject(U16 cmd, tNetSession *u, const tUruObject &obj, tBaseType *content)
	: tmMsgBase(cmd, plNetAck, u), obj(obj)
	{
		this->content.put(*content);
		this->content.compress();
	}
	
	void tmStreamedObject::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		t.get(obj);
		t.get(content);
	}
	
	void tmStreamedObject::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(obj);
		t.put(content);
	}
	
	void tmStreamedObject::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Object reference: [%s]", obj.str());
	}
	
	//// tmTestAndSet (NetMsgTestAndSet and NetMsgSharedState are identical)
	tmTestAndSet::tmTestAndSet(tNetSession *u) : tmStreamedObject(u)
	{ }
	
	void tmTestAndSet::store(tBBuf &t)
	{
		tmStreamedObject::store(t);
		
		tMBuf state = content.fullContent();
		// Verify state
		tStrBuf stateName;
		tUStr varName;
		U32 count;
		Byte mayDelete, varType, varValue;
		state.get(stateName);
		if (stateName != "TrigState")
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.stateName of %s (should be \"TrigState\")", stateName.c_str()));
		count = state.getU32();
		if (count != 1)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.count of %d (should be 1)", count));
		mayDelete = state.getByte();
		if (mayDelete != 0x00 && mayDelete != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.mayDelete of 0x%02X (should be 0x00 or 0x01)", mayDelete));
		// This is now the first and only variable in that state
		state.get(varName);
		if (varName != "Triggered")
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.varName of %s (should be \"Triggered\")", varName.c_str()));
		varType = state.getByte();
		if (varType != 0x02) // 2 is a boolean value
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.varType of %d (should be 2)", varType));
		varValue = state.getByte();
		if (varValue != 0x00 && varValue != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.varValue of 0x%02X (should be 0x00 or 0x01)", varValue));
		if (!state.eof())
			throw txUnexpectedData(_WHERE("Got a state description which is too long: %d bytes remaining after parsing", state.remaining()));
		// End of state
		
		Byte lockReq = t.getByte();
		if (lockReq != 0x00 && lockReq != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.lockReq of 0x%02X (should be 0x00 or 0x01)", lockReq));
		isLockReq = lockReq;
		
		// for some reason, these variables occur only in a certain combination
		if (mayDelete == lockReq)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet: mayDelete (0x%02X) and lockReq (0x%02X) must not be equal", mayDelete, lockReq));
		if (varValue != lockReq)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet: varValue (0x%02X) and lockReq (0x%02X) must be equal", varValue, lockReq));
	}
	
	void tmTestAndSet::additionalFields()
	{
		tmStreamedObject::additionalFields();
		dbg.printBoolean(", Lock requested: ", isLockReq);
	}
	
	//// tmSDLState
	tmSDLState::tmSDLState(tNetSession *u) : tmStreamedObject(u)
	{ }
	
	tmSDLState::tmSDLState(tNetSession *u, const tUruObject &obj, tBaseType *content, bool isInitial)
	 : tmStreamedObject(NetMsgSDLState, u, obj, content)
	{
		this->isInitial = isInitial;
		
	}
	
	tmSDLState::tmSDLState(U16 cmd, tNetSession *u, const tmSDLState &msg)
	 : tmStreamedObject(cmd, u, msg), isInitial(msg.isInitial)
	{
		isInitial = msg.isInitial;
	}
	
	void tmSDLState::store(tBBuf &t)
	{
		tmStreamedObject::store(t);
		if (content.getType() != plNull)
			throw txProtocolError(_WHERE("Plasma object type of an SDL must be plNull"));
		
		Byte initial = t.getByte();
		if (initial != 0x00 && initial != 0x01)
			throw txProtocolError(_WHERE("NetMsgSDLState.initial must be 0x00 or 0x01 but is 0x%02X", initial));
		isInitial = initial;
	}
	
	void tmSDLState::stream(tBBuf &t) const
	{
		tmStreamedObject::stream(t);
		t.putByte(isInitial);
	}
	
	void tmSDLState::additionalFields()
	{
		tmStreamedObject::additionalFields();
		dbg.printBoolean(", initial age state: ", isInitial);
	}
	
	//// tmSDLStateBCast
	tmSDLStateBCast::tmSDLStateBCast(tNetSession *u) : tmSDLState(u)
	{ }
	
	tmSDLStateBCast::tmSDLStateBCast(tNetSession *u, const tmSDLStateBCast & msg)
	 : tmSDLState(NetMsgSDLStateBCast, u, msg)
	{
		ki = msg.ki;
	}
	
	void tmSDLStateBCast::store(tBBuf &t)
	{
		tmSDLState::store(t);
		Byte persistentOnServer = t.getByte();
		if (persistentOnServer != 0x01)
			throw txProtocolError(_WHERE("NetMsgSDLStateBCast.persistentOnServer must be 0x01 but is 0x%02X", persistentOnServer));
	}
	
	void tmSDLStateBCast::stream(tBBuf &t) const
	{
		tmSDLState::stream(t);
		t.putByte(0x01); // persistentOnServer
	}
	
	//// tmRelevanceRegions
	tmRelevanceRegions::tmRelevanceRegions(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmRelevanceRegions::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		// These are actually two 8-byte bitfields ("regions I care about" and "regions I'm in"), but we don't bother saving them
		t.read(16);
	}
	
	//// tmSetTimeout
	tmSetTimeout::tmSetTimeout(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmSetTimeout::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		if (ki == 0 || ki != u->ki) throw txProtocolError(_WHERE("KI mismatch (%d != %d)", ki, u->ki));
		
		timeout = t.getFloat();
	}
	
	void tmSetTimeout::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Timeout: %f", timeout);
	}
	
	//// tmMembersList
	tmMembersList::tmMembersList(tNetSession *u) : tmMsgBase(NetMsgMembersList, plNetAck, u)
	{ }
	
	void tmMembersList::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.putU16(members.size());
		for (tMemberList::const_iterator i = members.begin(); i != members.end(); ++i)
			t.put(*i);
	}
	
	void tmMembersList::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Number of other players: %d", members.size());
	}
	
	//// tmMemberUpdate
	tmMemberUpdate::tmMemberUpdate(tNetSession *u, const tMemberInfo &info, bool isJoined)
	 : tmMsgBase(NetMsgMemberUpdate, plNetAck, u), info(info)
	{
		this->isJoined = isJoined;
	}
	
	void tmMemberUpdate::stream(tBBuf &t) const
	{
		tmMsgBase::stream(t);
		t.put(info);
		t.putByte(isJoined);
	}
	
	void tmMemberUpdate::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Member Info: [%s], is joined: ", info.str());
		dbg.printBoolean(isJoined);
	}
	
	//// tmPython
	tmPython::tmPython(tNetSession *u) : tmMsgBase(u) {}
	
	void tmPython::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.end(); // just ignore everything
	}

} //end namespace alc

