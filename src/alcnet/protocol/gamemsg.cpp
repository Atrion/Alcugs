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
	
	void tmGameMessage::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// store the whole message
		gameMessage.clear();
		t.get(gameMessage);
		// now, verify (will throw a txOutOfRange when too small)
		gameMessage.rewind();
		gameMessage.read(5); // ignore the first bytes
		U32 gameMsgSize = gameMessage.getU32();
		gameMessage.read(gameMsgSize); // this is the message itself
		gameMessage.read(1); // ignore 1 byte
		if (cmd == NetMsgGameMessage && !gameMessage.eof()) // the derived msg types do this check themselves
			throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after the message
	}
	
	void tmGameMessage::stream(tBBuf &t)
	{
		tmMsgBase::stream(t);
		gameMessage.rewind();
		t.put(gameMessage);
	}
	
	//// tmGameMessageDirected
	tmGameMessageDirected::tmGameMessageDirected(tNetSession *u) : tmGameMessage(u)
	{ recipients = NULL; }
	
	tmGameMessageDirected::tmGameMessageDirected(U16 cmd, U32 flags, tNetSession *u) : tmGameMessage(cmd, flags, u)
	{ recipients = NULL; }
	
	tmGameMessageDirected::~tmGameMessageDirected(void)
	{
		if (recipients) free(recipients);
	}
	
	void tmGameMessageDirected::store(tBBuf &t)
	{
		tmGameMessage::store(t);
		// get list of recipients
		nRecipients = gameMessage.getByte();
		if (recipients) free(recipients);
		recipients = (U32 *)malloc(nRecipients*sizeof(U32));
		for (int i = 0; i < nRecipients; ++i) recipients[i] = gameMessage.getU32();
		if (!gameMessage.eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after the recipient list
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
		t.read(); // just accept whatever we get
		// FIXME: actually parse the message
	}
	
	//// tmPlayerPage
	tmPlayerPage::tmPlayerPage(tNetSession *u) : tmMsgBase(u)
	{ }
	
	void tmPlayerPage::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		t.read(); // just accept whatever we get
		// FIXME: actually parse the message
	}

} //end namespace alc

