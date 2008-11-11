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

	//// tmJoinReq
	tmJoinReq::tmJoinReq(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmJoinReq::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		if (!hasFlags(plNetX)) throw txProtocolError(_WHERE("X flag missing"));
	}
	
	//// tmJoinAck
	tmJoinAck::tmJoinAck(tNetSession *u, U32 x) : tmMsgBase(NetMsgJoinAck, plNetAck | plNetCustom | plNetKi | plNetX, u)
	{
		this->x = x;
		ki = u->ki;
	}
	
	void tmJoinAck::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		
		t.putU16(0); // unknown flag
		// write an empty SDL. FIXME: send the age state here
		t.putU32(0); // real size
		t.putByte(0); // compression flag
		t.putU32(0); // sent size
	}
	
	//// tmGameMessage
	tmGameMessage::tmGameMessage(tNetSession *u) : tmMsgBase(u)
	{ }
	
	tmGameMessage::tmGameMessage(U16 cmd, U32 flags, tNetSession *u) : tmMsgBase(cmd, flags, u)
	{ }
	
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
		memcpy(header, msg.header, 5);
		ki = msg.ki;
	}
	
	void tmGameMessage::store(tBBuf &t)
	{
		message.clear();
		
		tmMsgBase::store(t);
		if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
		
		memcpy(header, t.read(5), 5);
		U32 gameMsgSize = t.getU32();
		message.write(t.read(gameMsgSize), gameMsgSize); // that's the message itself
		Byte unk3 = t.getByte();
		if (unk3 != 0x00)
			throw txProtocolError(_WHERE("Unexpected NetMsgGameMessage.unk3 of 0x%02X (should be 0x00)", unk3));
	}
	
	void tmGameMessage::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		t.write(header, 5);
		t.putU32(message.size());
		t.put(message);
		t.putByte(0); // unk3
	}
	
	//// tmGameMessageDirected
	tmGameMessageDirected::tmGameMessageDirected(tNetSession *u) : tmGameMessage(u)
	{ recipients = NULL; }
	
	tmGameMessageDirected::tmGameMessageDirected(U16 cmd, U32 flags, tNetSession *u) : tmGameMessage(cmd, flags, u)
	{ recipients = NULL; }
	
	tmGameMessageDirected::tmGameMessageDirected(U16 cmd, U32 flags, tNetSession *u, tmGameMessageDirected &msg)
	 : tmGameMessage(cmd, flags, u, msg)
	{ copyDrctProps(msg); }
	
	tmGameMessageDirected::tmGameMessageDirected(tNetSession *u, tmGameMessageDirected &msg)
	 : tmGameMessage(NetMsgGameMessageDirected, plNetAck | plNetKi | plNetDirected, u, msg)
	{ copyDrctProps(msg); }
	
	tmGameMessageDirected::~tmGameMessageDirected(void)
	{
		if (recipients) free(recipients);
	}
	
	void tmGameMessageDirected::copyDrctProps(tmGameMessageDirected &msg)
	{
		nRecipients = msg.nRecipients;
		recipients = (U32 *)malloc(nRecipients*sizeof(U32));
		memcpy(recipients, msg.recipients, nRecipients*sizeof(U32));
	}
	
	void tmGameMessageDirected::store(tBBuf &t)
	{
		tmGameMessage::store(t);
		// get list of recipients
		nRecipients = t.getByte();
		if (recipients) free(recipients);
		recipients = (U32 *)malloc(nRecipients*sizeof(U32));
		for (int i = 0; i < nRecipients; ++i) recipients[i] = t.getU32();
	}
	
	void tmGameMessageDirected::stream(tBBuf &t)
	{
		if (!recipients) throw txProtocolError(_WHERE("I've got nothing to send"));
		tmGameMessage::stream(t);
		t.putByte(nRecipients);
		for (int i = 0; i < nRecipients; ++i) t.putU32(recipients[i]);
	}
	
	//// tmLoadClone
	tmLoadClone::tmLoadClone(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmLoadClone::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.read(); // just accept whatever we get
		// FIXME: actually parse the message
	}
	
	//// tmPagingRoom
	tmPagingRoom::tmPagingRoom(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmPagingRoom::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		U32 format = t.getU32();
		if (format != 1)
			throw txProtocolError(_WHERE("NetMsgPagingRoom.format must be 1, but is %d", format));
		pageId = t.getU32();
		pageType = t.getU16();
		t.get(pageName);
		isPageOut = t.getByte();
	}
	
	void tmPagingRoom::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Page ID: 0x%08X, Page Type: 0x%04X, Page Name: %s, Paged out: ", pageId, pageType, pageName.c_str());
		if (isPageOut) dbg.printf("yes");
		else         dbg.printf("no");
	}
	
	//// tmPlayerPage
	tmPlayerPage::tmPlayerPage(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmPlayerPage::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		isPageOut = t.getByte();
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
		int nPages = t.getU32();
		// FIXME: accept nPages != 0
		if (nPages || !hasFlags(plNetStateReq))
			throw txProtocolError(_WHERE("Rejecting GameStateReq which contains a list of pages"));
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
		Byte flag, state1, state2, state3;
		U32 unk;
		tUStr trigger, triggered;
	
		tmMsgBase::store(t);
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
		state3 = t.getByte();
		if (state3 != 0x00 && state3 != 0x01)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet.state3 of 0x%02X (should be 0x00 or 0x01)", state3));
		
		// now check if the states are a valid combination
		if (state2 != state3)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet: state2 (0x%02X) and state3 (0x%02X) must be equal", state2, state3));
		if (state2 == state1)
			throw txProtocolError(_WHERE("Unexpected NetMsgTestAndSet: state1 (0x%02X) and state2 (0x%02X) must not be equal", state2, state3));
		/* When state1 and state2 are not the same while state2 and state3 are the same, only the following combinations are possible:
		0-1-1 (lockReq = true)
		1-0-0 (lockReq = false) */
		lockReq = state2;
	}
	
	void tmTestAndSet::additionalFields()
	{
		dbg.nl();
		dbg.printf(" Object reference: [%s], Lock requested: ", obj.str());
		if (lockReq) dbg.printf("yes");
		else         dbg.printf("no");
	}
	
	//// tmRelevanceRegions
	tmRelevanceRegions::tmRelevanceRegions(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmRelevanceRegions::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		U32 unk = t.getU32();
		if (unk != 1) throw txProtocolError(_WHERE("NetMsgRelevanceRegions.Unk1 must be 1 but is %d", unk));
		unk = t.getU32();
		if (unk != 1) throw txProtocolError(_WHERE("NetMsgRelevanceRegions.Unk2 must be 1 but is %d", unk));
		unk = t.getU32();
		if (unk != 1) throw txProtocolError(_WHERE("NetMsgRelevanceRegions.Unk3 must be 1 but is %d", unk));
		unk = t.getU32();
		if (unk != 1) throw txProtocolError(_WHERE("NetMsgRelevanceRegions.Unk4 must be 1 but is %d", unk));
	}
	
	//// tmSDLState
	tmSDLState::tmSDLState(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmSDLState::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.read(); // just accept whatever we get
		// FIXME: actually parse the message
	}
	
	//// tmSDLStateBCast
	tmSDLStateBCast::tmSDLStateBCast(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmSDLStateBCast::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.read(); // just accept whatever we get
		// FIXME: actually parse the message
	}
	
	//// tmSetTimeout
	tmSetTimeout::tmSetTimeout(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmSetTimeout::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		U32 unk = t.getU32();
		if (unk != 0x43340000) throw txProtocolError(_WHERE("NetMsgSetTimeout.Unk must be 0x43340000 but is 0x%08X", unk));
	}

} //end namespace alc

