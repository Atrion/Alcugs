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

/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"
#include "protocol/trackingmsg.h"

#include <alcdebug.h>

namespace alc {

	//// tmCustomSetGuid
	tmCustomSetGuid::tmCustomSetGuid(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		age.setVersion(0); // normal UrurString
		netmask.setVersion(0); // normal UrurString
		ip.setVersion(0); // normal UrurString
	}
	
	void tmCustomSetGuid::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		// there's already a guid member in tmMsgBase, so let's use that (though we need only 8 bytes)
		tUStr guid_str(5); // inverted UruString
		t.get(guid_str);
		alcAscii2Hex(guid, (Byte *)guid_str.c_str(), 8);
		
		t.get(age);
		t.get(netmask);
		t.get(ip);
	}
	
	void tmCustomSetGuid::additionalFields()
	{
		dbg.nl();
		dbg.printf(" GUID: %s, Age filename: %s, Netmask: %s, IP: %s", alcGetStrGuid(guid, 8), age.c_str(), netmask.c_str(), ip.c_str());
	}
	
	//// tmCustomPlayerStatus
	tmCustomPlayerStatus::tmCustomPlayerStatus(tNetSession *u) : tmMsgBase(0, 0, u) // it's not capable of sending
	{
		account.setVersion(0); // normal UrurString
		avatar.setVersion(0); // normal UrurString
	}
	
	void tmCustomPlayerStatus::store(tBBuf &t)
	{
		tmMsgBase::store(t);
		memcpy(guid, t.read(16), 16);
		t.get(account);
		t.get(avatar);
		playerFlag = t.getByte();
		playerStatus = t.getByte();
	}
	
	void tmCustomPlayerStatus::additionalFields()
	{
		dbg.nl();
		dbg.printf(" GUID: %s, Account: %s, Avatar: %s, Flag: 0x%02X, Status: 0x%02X (%s)", alcGetStrGuid(guid, 16), account.c_str(), avatar.c_str(), playerFlag, playerStatus, alcUnetGetReasonCode(playerStatus));
	}

} //end namespace alc
