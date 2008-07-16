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

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"
#include "protocol/msgparsers.h"
#include "protocol/vaultproto.h"

////extra includes

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	
	//// tvCreatableGenericValue
	void tvCreatableGenericValue::store(tBBuf &t)
	{
		format = t.getByte();
		
		switch (format) {
			case DInteger:
				integer = t.getS32();
				break;
			case DUruString:
				t.get(str);
				break;
			case DTimestamp:
				time = t.getDouble();
				break;
			default:
				lerr->log("got creatable generic value with unknown format 0x%02X\n", format);
				throw txProtocolError(_WHERE("unknown creatable generic value format"));
		}
	}
	
	int tvCreatableGenericValue::stream(tBBuf &t)
	{
		int off = 0;
		t.putByte(format); ++off;
		switch (format) {
			case DInteger:
				t.putS32(integer); off += 4;
				break;
			case DUruString:
				off += t.put(str);
				break;
			case DTimestamp:
				t.putDouble(time); off += 8;
				break;
			default:
				throw txProtocolError(_WHERE("unknown creatable generic value format"));
		}
		return off;
	}
	
	void tvCreatableGenericValue::asHtml(tLog *log)
	{
		switch (format) {
			case DInteger:
				log->print("DInteger: 0x%08X (%d)<br />\n", integer, integer);
				break;
			case DUruString:
				log->print("DUruString: %s<br />\n", str.c_str());
				break;
			case DTimestamp:
				log->print("DTimestamp: %f<br />\n", time);
				break;
			default:
				throw txProtocolError(_WHERE("unknown creatable generic value format"));
		}
	}
	
	//// tvCreatableStream
	void tvCreatableStream::store(tBBuf &t)
	{
		size = t.getU32();
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
	
	void tvCreatableStream::asHtml(tLog *log)
	{
		log->print("Size: %d<br />\n", size);
		tMBuf buf;
		buf.write(data, size);
		buf.rewind();
		// the format of the content depends on the ID
		switch (id) {
			case 0x06:
				log->print("<span style='color:red'>FIXME: there are some vault nodes here</span><br />\n");
				buf.end();
				break;
			case 0x0A:
			{
				U16 num = buf.getU16();
				log->print("number of values: %d<br />\n", num);
				if (num == 0) break;
				log->print("Value(s):");
				for (U16 i = 0; i < num; ++i) {
					U32 val = buf.getU32();
					if (i > 0) log->print(",");
					log->print(" 0x%08X (%d)", val, val);
				}
				log->print("<br />\n");
				break;
			}
			case 0x0E:
			{
				U32 num = buf.getU32();
				log->print("number of values: %d<br />\n", num);
				for (U32 i = 0; i < num; ++i) {
					U32 val = buf.getU32();
					double time = buf.getDouble();
					log->print("[%d] ID: 0x%08X (%d), Stamp: %f<br />\n", i+1, val, val, time);
				}
				break;
			}
			case 0x0F:
			{
				U32 num = buf.getU32();
				log->print("number of values: %d<br />\n", num);
				for (U32 i = 0; i < num; ++i) {
					U32 val = buf.getU32();
					log->print("[%d] ID1: 0x%08X (%d), ", i+1, val, val);
					val = buf.getU32();
					log->print("ID2: 0x%08X (%d), ", val, val);
					val = buf.getU32();
					log->print("ID3: 0x%08X (%d), ", val, val);
					time_t stamp = buf.getU32();
					val = buf.getU32();
					log->print("Stamp: %s %d, ", alcGetStrTime(stamp), val);
					val = buf.getByte();
					log->print("Flag: %d<br />\n", val);
				}
				break;
			}
			default:
				log->print("<span style='color:red'>Unknown strange stream data type!</span><br />\n");
				buf.end();
				break;
		}
		if (!buf.eof())
			log->print("<span style='color:red'>Strange, this stream has some bytes left where none are expected!</span><br />\n");
	}
	
	//// tvNode
	void tvNode::store(tBBuf &t)
	{
		// get flags
		flagA = t.getU32(); // I think this is something like a version number. version 1 contains only flagB, version 2 also flagC
		if (flagA != 0x00000001 && flagA != 0x00000002) { // check for unknown values
			// FIXME: dump packet
		}
		flagB = t.getU32(); // this is the main flag, all 32 bits are known
		if (flagA == 0x00000002) { // it contains flagC
			flagC = t.getU32();
			U32 check = MBlob1Guid | MBlob2Guid;
			if (flagC & ~(check)) { // check for unknown values
				// FIXME: dump packet
			}
		}
		else
			flagC = 0;
		
		// now read data according to flags
		// FIXME: do that
		
		throw txProtocolError(_WHERE("cant parse vault node, so I cant go on")); // FIXME: remove this when above function is completed
	}
	
	int tvNode::stream(tBBuf &t)
	{
		return 0;
	}
	
	void tvNode::asHtml(tLog *log)
	{
		
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
		if (tpots == 1 && type == DVaultNode2) type = DVaultNode; // a DVaultNode is called DVaultNode2 in TPOTS
		
		if (data) delete data;
		switch (type) {
			case DCreatableGenericValue:
				data = new tvCreatableGenericValue;
				break;
			case DCreatableStream:
				data = new tvCreatableStream(id);
				break;
			case DVaultNode:
				data = new tvNode;
				break;
			default:
				// FIXME: add more types
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
	
	void tvItem::asHtml(tLog *log)
	{
		log->print("Id: <b>0x%02X (%d)</b>, type: 0x%04X (%s)<br />\n", id, id, type, alcVaultGetDataType(type));
		data->asHtml(log);
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
		DBG(5, "vault message: command: 0x%02X, compressed: 0x%02X, real size: %d", cmd, compressed, realSize);
		
		tBBuf *buf = &t;
		if (compressed == 0x03) { // it's compressed, so decompress it
			U32 compressedSize = t.getU32();
			DBGM(5, ", compressed size: %d", compressedSize);
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
		DBGM(5, ", number of items: %d\n", numItems);
		if (items) {
			for (int i = 0; i < numItems; ++i) delete items[i];
			free(items);
		}
		items = (tvItem **)malloc(numItems * sizeof(tvItem *));
		memset(items, 0, numItems * sizeof(tvItem *));
		for (int i = 0; i < numItems; ++i) {
			items[i] = new tvItem(tpots);
			buf->get(*items[i]);
		}
		
		if (compressed == 0x03) { // it was compressed, so we have to clean up
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
		for (int i = 0; i < numItems; ++i) {
			items[i]->tpots = tpots; // make sure the right TPOTS value is used
			buf->put(*items[i]);
		}
		
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
	
	void tvMessage::print(tLog *log, bool clientToServer, tNetSession *client)
	{
		if (log == lnull) return; // don't do anything if log is disabled
		if (clientToServer)
			log->print("<h2 style='color:blue'>From client (%s) to vault</h2>\n", client->str());
		else
			log->print("<h2 style='color:green'>From vault to client (%s)</h2>\n", client->str());
		asHtml(log);
		log->print("<hr>\n\n");
		log->flush();
	}
	
	void tvMessage::asHtml(tLog *log)
	{
		// the header
		if (task) log->print("<b>NetMsgVaultTask ");
		else      log->print("<b>NetMsgVault ");
		log->print("CMD: 0x%02X ", cmd);
		if (task) log->print("(%s)</b><br />\n", alcVaultGetTaskCmd(cmd));
		else      log->print("(%s)</b><br />\n", alcVaultGetCmd(cmd));
		log->print("compressed: %d, real size: %d<br />\n", compressed, realSize);
		if (task) log->print("sub: %d, <b>client: %d</b><br />\n", context, vmgr);
		else      log->print("context: %d, <b>vmgr: %d</b>, vn: %d<br />\n", context, vmgr, vn);
		
		// the items
		if (numItems > 0) {
			log->print("<table border='1'>\n");
			for (int i = 0; i < numItems; ++i) {
				log->print("<tr><th style='background-color:cyan;'>Item %d</th></tr>\n", i+1);
				log->print("<tr><td>\n");
				items[i]->asHtml(log);
				log->print("</td></tr>\n");
			}
			log->print("</table>");
		}
	}
	
	const char *alcVaultGetDataType(U16 type)
	{
		static const char *ret;
		switch (type) {
			case 0x0387:
				ret = "DCreatableGenericValue";
				break;
			case 0x0389:
				ret = "DCreatableStream";
				break;
			default:
				ret = "<span style='color:red'>DUnknown</span>";
				break;
		}
		return ret;
	}
	
	const char *alcVaultGetCmd(Byte cmd)
	{
		static const char *ret;
		switch (cmd) {
			case 0x01:
				ret = "VConnect";
				break;
			case 0x02:
				ret = "VDisconnect";
				break;
			case 0x03:
				ret = "VAddNodeRef";
				break;
			case 0x04:
				ret = "VRemoveNodeRef";
				break;
			case 0x05:
				ret = "VNegotiateManifest";
				break;
			case 0x06:
				ret = "VSaveNode";
				break;
			case 0x07:
				ret = "VFindNode";
				break;
			case 0x08:
				ret = "VFetchNode";
				break;
			case 0x09:
				ret = "VSendNode";
				break;
			case 0x0A:
				ret = "VSetSeen";
				break;
			case 0x0B:
				ret = "VOnlineState";
				break;
			default:
				ret = "<span style='color:red'>VUnknown</span>";
				break;
		}
		return ret;
	}
	
	const char *alcVaultGetTaskCmd(Byte cmd)
	{
		static const char *ret;
		switch (cmd) {
			default:
				ret = "<span style='color:red'>TUnknown</span>";
				break;
		}
		return ret;
	}

} //end namespace alc

