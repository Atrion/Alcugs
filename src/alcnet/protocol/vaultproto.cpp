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
	
	void tvCreatableGenericValue::stream(tBBuf &t)
	{
		t.putByte(format);
		switch (format) {
			case DInteger:
				t.putS32(integer);
				break;
			case DUruString:
				t.put(str);
				break;
			case DTimestamp:
				t.putDouble(time);
				break;
			default:
				throw txProtocolError(_WHERE("unknown creatable generic value format"));
		}
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
		data = (Byte *)malloc(sizeof(Byte) * size);
		memcpy(data, t.read(size), size);
	}
	
	void tvCreatableStream::stream(tBBuf &t)
	{
		if (!data) throw txProtocolError(_WHERE("don\'t have any data to write"));
		t.putU32(size);
		t.write(data, size);
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
				log->print("<table border='1'>\n");
				while (!buf.eof()) {
					tvNode node;
					buf.get(node);
					node.asHtml(log);
				}
				log->print("</table>\n");
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
	tvNode::~tvNode(void)
	{
		if (blob1) free(blob1);
	}
	
	void tvNode::store(tBBuf &t)
	{
		// get flags
		flagA = t.getU32(); // I think this is something like a version number. version 1 contains only flagB, version 2 also flagC
		if (flagA != 0x00000001 && flagA != 0x00000002) { // check for unknown values
			throw txProtocolError(_WHERE("invalid flagA"));
		}
		flagB = t.getU32(); // this is the main flag, all 32 bits are known
		if (flagA == 0x00000002) { // it contains flagC
			flagC = t.getU32();
			U32 check = MBlob1Guid | MBlob2Guid | 0x00000004; // the latter is unknown and seems to be unused
			if (flagC & ~(check)) { // check for unknown values
				throw txProtocolError(_WHERE("invalid flagC"));
			}
		}
		else
			flagC = 0;
		
		// mandatory fields - it doesn't matter whether the flags are on or not
		index = t.getU32();
		type = t.getByte();
		permissions = t.getU32();
		if ((permissions & 0xFFFFFF00) != 0)
			throw txProtocolError(_WHERE("invalid permissions mask"));
		owner = t.getS32();
		group = t.getU32();
		modTime = t.getU32();
		modMicrosec = t.getU32();
		
		// optional fields
		if (flagB & MCreator)
			creator = t.getU32();
		else
			creator = 0;
		
		if (flagB & MCrtTime) {
			crtTime = t.getU32();
			crtMicrosec = t.getU32();
		}
		else
			crtTime = crtMicrosec = 0;
		
		if (flagB & MAgeCoords) // unused, do nothing
			;
		
		if (flagB & MAgeTime) {
			ageTime = t.getU32();
			ageMicrosec = t.getU32();
		}
		else
			ageTime = ageMicrosec = 0;
		
		if (flagB & MAgeName)
			t.get(ageName);
		
		if (flagB & MAgeGuid)
			memcpy(ageGuid, t.read(8), 8);
		else
			memset(ageGuid, 0, 8);
		
		if (flagB & MInt32_1)
			int1 = t.getU32();
		else
			int1 = 0;
		
		if (flagB & MInt32_2)
			int2 = t.getU32();
		else
			int2 = 0;
		
		if (flagB & MInt32_3)
			int3 = t.getU32();
		else
			int3 = 0;
		
		if (flagB & MInt32_4)
			int4 = t.getU32();
		else
			int4 = 0;
		
		if (flagB & MUInt32_1)
			uInt1 = t.getU32();
		else
			uInt1 = 0;
		
		if (flagB & MUInt32_2)
			uInt2 = t.getU32();
		else
			uInt2 = 0;
		
		if (flagB & MUInt32_3)
			uInt3 = t.getU32();
		else
			uInt3 = 0;
		
		if (flagB & MUInt32_4)
			uInt4 = t.getU32();
		else
			uInt4 = 0;
		
		if (flagB & MStr64_1)
			t.get(str1);
		
		if (flagB & MStr64_2)
			t.get(str2);
		
		if (flagB & MStr64_3)
			t.get(str3);
		
		if (flagB & MStr64_4)
			t.get(str4);
		
		if (flagB & MStr64_5)
			t.get(str5);
		
		if (flagB & MStr64_6)
			t.get(str6);
		
		if (flagB & MlStr64_1)
			t.get(lStr1);
		
		if (flagB & MlStr64_2)
			t.get(lStr2);
		
		if (flagB & MText_1)
			t.get(text1);
		
		if (flagB & MText_2)
			t.get(text2);
		
		if (blob1) {
			free(blob1);
			blob1 = NULL;
		}
		if (flagB & MBlob1) {
			blob1Size = t.getU32();
			if (blob1Size > 0) {
				blob1 = (Byte *)malloc(sizeof(Byte) * blob1Size);
				memcpy(blob1, t.read(blob1Size), blob1Size);
			}
		}
		
		if (flagB & MBlob2) {
			U32 blob2Size = t.getU32();
			if (blob2Size > 0)
				throw txProtocolError(_WHERE("Blob2Size > 0 - unimplemented"));
		}
		
		// the two blob guids must always be 0
		Byte blobGuid[8], zeroGuid[8];
		memset(zeroGuid, 0, 8);
		if (flagC & MBlob1Guid) {
			memcpy(blobGuid, t.read(8), 8);
			if (memcmp(blobGuid, zeroGuid, 8) != 0)
				throw txProtocolError(_WHERE("Blob1Guid must always be zero"));
		}
		DBG(6, "5: %d bytes remaining\n", t.remaining());
		if (flagC & MBlob2Guid) {
			memcpy(blobGuid, t.read(8), 8);
			if (memcmp(blobGuid, zeroGuid, 8) != 0)
				throw txProtocolError(_WHERE("Blob2Guid must always be zero"));
		}
	}
	
	void tvNode::stream(tBBuf &t)
	{
		// write flags
		t.putU32(flagA);
		if (flagA != 0x00000001 && flagA != 0x00000002) { // check for unknown values
			throw txProtocolError(_WHERE("invalid flagA"));
		}
		t.putU32(flagB);
		if (flagA == 0x00000002) {
			t.putU32(flagC);
			U32 check = MBlob1Guid | MBlob2Guid | 0x00000004; // the latter is unknown and seems to be unused
			if (flagC & ~(check)) { // check for unknown values
				throw txProtocolError(_WHERE("invalid flagC"));
			}
		}
		
		// write mandatory data
		t.putU32(index);
		t.putByte(type);
		t.putU32(permissions);
		if ((permissions & 0xFFFFFF00) != 0)
			throw txProtocolError(_WHERE("invalid permissions mask"));
		t.putS32(owner);
		t.putU32(group);
		t.putU32(modTime);
		t.putU32(modMicrosec);
		
		// write optional data
		if (flagB & MCreator) t.putU32(creator);
		if (flagB & MCrtTime) {
			t.putU32(crtTime);
			t.putU32(crtMicrosec);
		}
		if (flagB & MAgeCoords) // unused, do nothing
			;
		if (flagB & MAgeTime) {
			t.putU32(ageTime);
			t.putU32(ageMicrosec);
		}
		if (flagB & MAgeName) t.put(ageName);
		if (flagB & MAgeGuid) t.write(ageGuid, 8);
		if (flagB & MInt32_1) t.putU32(int1);
		if (flagB & MInt32_2) t.putU32(int2);
		if (flagB & MInt32_3) t.putU32(int3);
		if (flagB & MInt32_4) t.putU32(int4);
		if (flagB & MUInt32_1) t.putU32(uInt1);
		if (flagB & MUInt32_2) t.putU32(uInt2);
		if (flagB & MUInt32_3) t.putU32(uInt3);
		if (flagB & MUInt32_4) t.putU32(uInt4);
		if (flagB & MStr64_1) t.put(str1);
		if (flagB & MStr64_2) t.put(str2);
		if (flagB & MStr64_3) t.put(str3);
		if (flagB & MStr64_4) t.put(str4);
		if (flagB & MStr64_5) t.put(str5);
		if (flagB & MStr64_6) t.put(str6);
		if (flagB & MlStr64_1) t.put(lStr1);
		if (flagB & MlStr64_2) t.put(lStr2);
		if (flagB & MText_1) t.put(text1);
		if (flagB & MText_2) t.put(text2);
		if (flagB & MBlob1) {
			t.putU32(blob1Size);
			if (blob1Size > 0) t.write(blob1, blob1Size);
		}
		if (flagB & MBlob2) {
			t.putU32(0); // blob2 is always empty
		}
		// the two blob guids are always zero
		Byte zeroGuid[8];
		memset(zeroGuid, 0, 8);
		if (flagC & MBlob1Guid) t.write(zeroGuid, 8);
		if (flagC & MBlob2Guid) t.write(zeroGuid, 8);
	}
	
	void tvNode::flagsAsHtml(tLog *log)
	{
		log->print("<b>Flags:</b> 0x%08X (%d), 0x%08X (%d), 0x%08X (%d)<br />\n", flagA, flagA, flagB, flagB, flagC, flagC);
	}
	
	void tvNode::permissionsAsHtml(tLog *log)
	{
		log->print("<b>Permissions:</b> 0x%08X (%d)<br />\n", permissions, permissions);
	}
	
	void tvNode::asHtml(tLog *log)
	{
		// mandatory flieds
		log->print("<tr><th style='background-color:yellow'>Vault Node %d</th></tr>\n", index, index);
		log->print("<tr><td>\n");
		flagsAsHtml(log);
		log->print("<b>Type:</b> 0x%02X (%d)<br />\n", type, type); // FIXME: print type as string
		permissionsAsHtml(log);
		log->print("<b>Owner:</b> 0x%08X (%d)<br />\n", owner, owner);
		log->print("<b>Group:</b> 0x%08X (%d)<br />\n", group, group);
		log->print("<b>Modification time:</b> %s<br />\n", alcGetStrTime(modTime, modMicrosec));
		// optional fields
		if (flagB & MCreator) log->print("<b>Creator:</b> 0x%08X (%d)<br />\n", creator, creator);
		if (flagB & MCrtTime) log->print("<b>Create time:</b> %s<br />\n", alcGetStrTime(crtTime, crtMicrosec));
		if (flagB & MAgeCoords) log->print("<b>Age coords:</b> unused<br />\n");
		if (flagB & MAgeTime) log->print("<b>Age time:</b> %s<br />\n", alcGetStrTime(ageTime, ageMicrosec));
		if (flagB & MAgeName) log->print("<b>Age name:</b> %s<br />\n", ageName.c_str());
		if (flagB & MAgeGuid) log->print("<b>Age guid:</b> %s<br />\n", alcGetStrGuid(ageGuid));
		if (flagB & MInt32_1) log->print("<b>Int32_1:</b> 0x%08X (%d)<br />\n", int1, int1); // FIXME: this is (among others) the folder type, print it as string
		if (flagB & MInt32_2) log->print("<b>Int32_2:</b> 0x%08X (%d)<br />\n", int2, int2);
		if (flagB & MInt32_3) log->print("<b>Int32_3:</b> 0x%08X (%d)<br />\n", int3, int3);
		if (flagB & MInt32_4) log->print("<b>Int32_4:</b> 0x%08X (%d)<br />\n", int4, int4);
		if (flagB & MUInt32_1) log->print("<b>UInt32_1:</b> 0x%08X (%d)<br />\n", uInt1, uInt1);
		if (flagB & MUInt32_2) log->print("<b>UInt32_2:</b> 0x%08X (%d)<br />\n", uInt2, uInt2);
		if (flagB & MUInt32_3) log->print("<b>UInt32_3:</b> 0x%08X (%d)<br />\n", uInt3, uInt3);
		if (flagB & MUInt32_4) log->print("<b>UInt32_4:</b> 0x%08X (%d)<br />\n", uInt4, uInt4);
		if (flagB & MStr64_1) log->print("<b>Str64_1:</b> %s<br />\n", str1.c_str());
		if (flagB & MStr64_2) log->print("<b>Str64_2:</b> %s<br />\n", str2.c_str());
		if (flagB & MStr64_3) log->print("<b>Str64_3:</b> %s<br />\n", str3.c_str());
		if (flagB & MStr64_4) log->print("<b>Str64_4:</b> %s<br />\n", str4.c_str());
		if (flagB & MStr64_5) log->print("<b>Str64_5:</b> %s<br />\n", str5.c_str());
		if (flagB & MStr64_6) log->print("<b>Str64_6:</b> %s<br />\n", str6.c_str());
		if (flagB & MlStr64_1) log->print("<b>lStr64_1:</b> %s<br />\n", lStr1.c_str());
		if (flagB & MlStr64_2) log->print("<b>lStr64_2:</b> %s<br />\n", lStr2.c_str());
		if (flagB & MText_1) log->print("<b>Text_1:</b> %s<br />\n", text1.c_str());
		if (flagB & MText_2) log->print("<b>Text_2:</b> %s<br />\n", text2.c_str());
		// FIXME: do something with the data
		log->print("</td></tr>\n");
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
	
	void tvItem::stream(tBBuf &t)
	{
		if (!data) throw txProtocolError(_WHERE("don\'t have any data to write"));
		t.putByte(id);
		t.putByte(0); // unknown
		t.putU16(type);
		t.put(*data);
	}
	
	void tvItem::asHtml(tLog *log)
	{
		log->print("Id: <b>0x%02X (%d)</b>, type: 0x%04X (%s)<br />\n", id, id, type, alcVaultGetDataType(type));
		if (type == DVaultNode) log->print("<table border='1'>\n");
		data->asHtml(log);
		if (type == DVaultNode) log->print("</table>\n");
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
	
	void tvMessage::stream(tBBuf &t)
	{
		if (!items) throw txProtocolError(_WHERE("don\'t have any items to write"));
		t.putByte(cmd);
		t.putU16(0);// result
		t.putByte(compressed);
		t.putU32(realSize);
		
		// put the items into a temporary buffer which might be compressed
		tMBuf buf; // this should be created on the stack to avoid leaks when there's an exception
		buf.putU16(numItems);
		for (int i = 0; i < numItems; ++i) {
			items[i]->tpots = tpots; // make sure the right TPOTS value is used
			buf.put(*items[i]);
		}
		
		if (compressed == 0x03) {
			tZBuf content;
			content.put(buf);
			content.compress();
			t.putU32(content.size());
			t.put(content);
		}
		else if (compressed == 0x01) {
			t.put(buf);
		}
		else
			throw txProtocolError(_WHERE("unknown compression format"));
		
		// put remaining info
		if (task) {
			t.putU16(context);
			t.putU32(vmgr);
		}
		else {
			t.putU16(context);
			t.putU16(0); // result
			t.putU32(vmgr);
			t.putU16(vn);
		}
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
			case 0x0439:
				ret = "DVaultNode";
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

