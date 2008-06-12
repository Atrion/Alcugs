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
	
	//// tvCreatableGenericValue
	void tvCreatableGenericValue::store(tBBuf &t)
	{
		format = t.getByte();
		DBG(5, "creatable generic value: format: 0x%02X", format);
		
		switch (format) {
			case 0x00: // integer (signed, 4 bytes)
				integer = t.getS32();
				DBGM(5, ", integer value: %d\n", integer);
				break;
			case 0x03: // uru string (inverted)
				t.get(str);
				DBGM(5, ", string value: %s\n", str.c_str());
				break;
			// FIXME: add 0x07 (timestamp)
			default:
				DBGM(5, "\n");
				lerr->log("got creatable generic value with unknown format 0x%02X\n", format);
				throw txProtocolError(_WHERE("unknown creatable generic value format"));
		}
	}
	
	int tvCreatableGenericValue::stream(tBBuf &t)
	{
		int off = 0;
		t.putByte(format); ++off;
		switch (format) {
			case 0x00:
				t.putS32(integer); off += 4;
				break;
			case 0x03:
				off += t.put(str);
				break;
			// FIXME: add 0x07 (timestamp)
			default:
				throw txProtocolError(_WHERE("unknown creatable generic value format"));
		}
		return off;
	}
	
	//// tvCreatableStream
	void tvCreatableStream::store(tBBuf &t)
	{
		size = t.getU32();
		DBG(5, "creatable stream: size: %d\n", size);
		if (data) free(data);
		data = (Byte *)malloc(size);
		memcpy(data, t.read(size), size);
	}
	
	int tvCreatableStream::stream(tBBuf &t)
	{
		if (!data) throw txProtocolError(_WHERE("don\'t have any data to write"));
		int off = 0;
		t.putU32(size); off += 4;
		t.write(data, size); off += size;
		return off;
	}
	
	//// tvItem
	void tvItem::store(tBBuf &t)
	{
		id = t.getByte();
		Byte unk = t.getByte();
		if (unk != 0) {
			lerr->log("got vault message with bad item.unk value 0x%02X\n", unk);
			throw txProtocolError(_WHERE("bad item.unk value"));
		}
		type = t.getU16();
		DBG(5, "vault item: id 0x%02X, type: 0x%04X\n", id, type);
		
		if (data) delete data;
		switch (type) {
			case DCreatableGenericValue:
				data = new tvCreatableGenericValue;
				break;
			case DCreatableStream:
				data = new tvCreatableStream;
				break;
			// FIXME: add more types
			default:
				lerr->log("got vault message with unknown data type 0x%04X\n", type);
				throw txProtocolError(_WHERE("unknown vault data type"));
		}
		t.get(*data);
	}
	
	int tvItem::stream(tBBuf &t)
	{
		if (!data) throw txProtocolError(_WHERE("don\'t have any data to write"));
		int off = 0;
		t.putByte(id); ++off;
		t.putByte(0); ++off; // unknown
		t.putU16(type); off += 2;
		off += t.put(*data);
		return off;
	}
	
	//// tvMessage
	tvMessage::~tvMessage(void)
	{
		if (items) {
			for (int i = 0; i < numItems; ++i) {
				if (items[i]) delete items[i];
			}
			free(items);
		}
	}
	
	void tvMessage::store(tBBuf &t)
	{
		// parse the header
		cmd = t.getByte();
		U16 result = t.getU16();
		if (result != 0) {
			lerr->log("got vault message with bad 1st result 0x%04X\n", result);
			throw txProtocolError(_WHERE("bad 1st result code"));
		}
		compressed = t.getByte();
		realSize = t.getU32();
		DBG(5, "vault message: command: 0x%02X, compressed: 0x%02X, real size: %d\n", cmd, compressed, realSize);
		
		tBBuf *buf = &t;
		if (compressed == 0x03) { // it's compressed, so decompress it
			U32 compressedSize = t.getU32();
			DBG(5, "    compressed size: %d\n", compressedSize);
			tZBuf *content = new tZBuf;
			content->write(t.read(compressedSize), compressedSize);
			content->uncompress(realSize);
			buf = content;
		}
		else if (compressed != 0x01) {
			lerr->log("Unknown compression format 0x%02X\n", compressed);
			throw txProtocolError(_WHERE("unknown compression format"));
		}
		
		// get the items
		numItems = buf->getU16();
		DBG(5, "number of items: %d\n", numItems);
		if (items) {
			for (int i = 0; i < numItems; ++i) delete items[i];
			free(items);
		}
		items = (tvItem **)malloc(numItems * sizeof(tvItem *));
		memset(items, 0, numItems * sizeof(tvItem *));
		for (int i = 0; i < numItems; ++i) {
			items[i] = new tvItem;
			buf->get(*items[i]);
		}
		
		if (compressed == 0x03) { // we have to clean up
			if (!buf->eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after what we parsed above
			delete buf;
		}
		
		// get remaining info (which is always uncompressed)
		if (task) {
			context = t.getU16(); // in vtask, this is "sub" (?)
			vmgr = t.getU32(); // in vtask, this is the client
			vn = 0; // not existant in vtask
		}
		else {
			context = t.getU16();
			result = t.getU16();
			if (result != 0) {
				lerr->log("got vault message with bad 2nd result 0x%04X\n", result);
				throw txProtocolError(_WHERE("bad 2nd result code"));
			}
			vmgr = t.getU32();
			vn = t.getU16();
		}
		DBG(5, "remaining info: context: %d, vmgr: %d, vn: %d\n", context, vmgr, vn);
		
		if (!t.eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after what we parsed above
	}
	
	int tvMessage::stream(tBBuf &t)
	{
		if (!items) throw txProtocolError(_WHERE("don\'t have any items to write"));
		int off = 0;
		t.putByte(cmd); ++off;
		t.putU16(0); off += 2; // result
		t.putByte(compressed); ++off;
		t.putU32(realSize); off += 4;
		
		// put the items into a temporary buffer which might be compressed
		tMBuf *buf = new tMBuf;
		buf->putU16(numItems);
		for (int i = 0; i < numItems; ++i)
			buf->put(*items[i]);
		
		if (compressed == 0x03) {
			tZBuf content;
			content.put(*buf);
			content.compress();
			t.putU32(content.size()); off += 4;
			off += t.put(content);
			delete buf;
		}
		else if (compressed == 0x01) {
			off += t.put(*buf);
			delete buf;
		}
		else
			throw txProtocolError(_WHERE("unknown compression format"));
		
		// put remaining info
		if (task) {
			t.putU16(context); off += 2;
			t.putU32(vmgr); off += 4;
		}
		else {
			t.putU16(context); off += 2;
			t.putU16(0); off += 2; // result
			t.putU32(vmgr); off += 4;
			t.putU16(vn); off += 2;
		}
		
		return off;
	}

} //end namespace alc

