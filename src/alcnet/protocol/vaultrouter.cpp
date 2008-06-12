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
#define __U_VAULTROUTER_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"
#include "protocol/msgparsers.h"
#include "protocol/vaultrouter.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	
	//// tVaultItem
	void tVaultItem::store(tBBuf &t)
	{
		id = t.getByte();
		Byte unk = t.getByte();
		if (unk != 0) {
			lerr->log("got vault message with bad item.unk value 0x%04X\n", unk);
			throw txProtocolError(_WHERE("bad item.unk value"));
		}
		type = t.getU16();
		DBG(5, "vault item: id 0x%02X, type: 0x%04X\n", id, type);
		
		switch (type) {
			case DCreatableGenericValue:
				// FIXME: parse it
				break;
			// FIXME: add more types
			default:
				lerr->log("got vault message with unknown data type 0x%04X\n", type);
				throw txProtocolError(_WHERE("unknown data type"));
		}
	}
	
	//// tVaultMessage
	tVaultMessage::~tVaultMessage(void)
	{
		if (items) {
			for (int i = 0; i < numItems; ++i) {
				if (items[i]) delete items[i];
			}
			free(items);
		}
	}
	
	void tVaultMessage::store(tBBuf &t)
	{
		// parse the header
		cmd = t.getByte();
		U16 result = t.getU16();
		if (result != 0) {
			lerr->log("got vault message with bad result 0x%04X\n", result);
			throw txProtocolError(_WHERE("bad result code"));
		}
		compressed = t.getByte();
		realSize = t.getU32();
		DBG(5, "command: 0x%02X, compressed: 0x%02X, real size: %d\n", cmd, compressed, realSize);
		
		if (compressed == 0x03) {
			// FIXME: uncompress
			throw txProtocolError(_WHERE("compressed vault message"));
		}
		else if (compressed != 0x01) {
			lerr->log("Unknown compression format 0x%02X\n", compressed);
			throw txProtocolError(_WHERE("unknown compression format"));
		}
		
		// get the items
		numItems = t.getU16();
		DBG(5, "number of items: %d\n", numItems);
		if (items) {
			for (int i = 0; i < numItems; ++i) delete items[i];
			free(items);
		}
		items = (tVaultItem **)malloc(numItems * sizeof(tVaultItem *));
		memset(items, 0, numItems * sizeof(tVaultItem *));
		for (int i = 0; i < numItems; ++i) {
			items[i] = new tVaultItem;
			t.get(*items[i]);
			break; // FIXME: only one item is parsed ATM as it's not completely parsed so the beginning of the next one is unknown
		}
		
		// FIXME: there's still more to be parsed
	}

} //end namespace alc

