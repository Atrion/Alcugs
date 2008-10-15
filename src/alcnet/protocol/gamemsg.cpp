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
	tmJoinReq::tmJoinReq(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{ }
	
	//// tmJoinAck
	tmJoinAck::tmJoinAck(tNetSession *u) : tmMsgBase(NetMsgJoinAck, plNetAck | plNetCustom | plNetKi | plNetX | plNetFirewalled, u)
	{
		x = u->x;
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

} //end namespace alc

