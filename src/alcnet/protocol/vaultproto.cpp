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

#include "alcnet.h"
#include "protocol/ext-protocol.h"

////extra includes
#include <sys/stat.h>

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	
	//// tvAgeInfoStruct
	tvAgeInfoStruct::tvAgeInfoStruct(const char *filename, const char *instanceName, const char *userDefName, const char *displayName, const Byte *guid) : tvBase()
	{
		flags = 0x01 | 0x02 | 0x04 | 0x08 | 0x20; // instanceName, filename, GUID, user defined name, display name
		this->filename.writeStr(filename);
		this->instanceName.writeStr(instanceName);
		this->userDefName.writeStr(userDefName);
		this->displayName.writeStr(displayName);
		memcpy(this->guid, guid, 8);
	}
	
	tvAgeInfoStruct::tvAgeInfoStruct(const char *filename, const Byte *guid) : tvBase()
	{
		flags =  0x02 | 0x04; // filename, GUID
		this->filename.writeStr(filename);
		memcpy(this->guid, guid, 8);
	}
	
	void tvAgeInfoStruct::store(tBBuf &t)
	{
		//AgeInfoStruct flags
		//Found:
		// 0x02 filename
		// 0x03 filename,instance name
		// 0x0B filename,instance name,user name
		// 0x0F filename,instance name,guid,user name
		// 0x2F filename,instance name,guid,user name,display name
		// 0x6F filename,instance name,guid,user name,display name,language
		//Supposicions:
		// 0x01: instance name
		// 0x02: filename (must always be set)
		// 0x04: The Age Guid
		// 0x08: The user defined name
		// 0x20: DisplayName (Desc's name)
		// 0x40: Language
		flags = t.getByte();
		Byte check = 0x02 | 0x01 | 0x04 | 0x08 | 0x20 | 0x40;
		if (flags & ~(check))
			throw txProtocolError(_WHERE("unknown flag 0x%02X for AgeInfoStruct", flags));
		if (!(flags & 0x02)) // this must always be set (filename)
			throw txProtocolError(_WHERE("the 0x02 flag must always be set in AgeInfoStruct"));
		
		t.get(filename);
		
		if (flags & 0x01) // instance name
			t.get(instanceName);
		else { // instance name disabled
			throw txProtocolError(_WHERE("instance name flag not set... what to do?"));
		}
		
		if (flags & 0x04) // GUID
			memcpy(guid, t.read(8), 8);
		else
			memset(guid, 0, 8); // some parts of the server rely on this being all zero when no GUID is set
		
		if (flags & 0x08) // user defined name
			t.get(userDefName);
		
		if (flags & 0x20) // display name
			t.get(displayName);
		
		if (flags & 0x40) { // language
			// this is not the language of the client, but something else
			U32 language = t.getU32(); // always seen 0
			if (language != 0) throw txProtocolError(_WHERE("Language value of an AgeInfoStruct is 0x%08X instead of 0\n", language));
		}
	}
	
	void tvAgeInfoStruct::stream(tBBuf &t) const
	{
		// see store for description of flags
		t.putByte(flags);
		
		t.put(filename);
		
		if (flags & 0x01) // instance name
			t.put(instanceName);
		
		if (flags & 0x04) // GUID
			t.write(guid, 8);
		
		if (flags & 0x08) // user defined name
			t.put(userDefName);
		
		if (flags & 0x20) // display name
			t.put(displayName);
		
		if (flags & 0x40) // language
			t.putU32(0);
	}
	
	const char *tvAgeInfoStruct::str(void)
	{
		dbg.clear();
		dbg.printf("Filename: %s", filename.c_str());
		if (flags & 0x01) // instance name
			dbg.printf(", Instance Name: %s", instanceName.c_str());
		if (flags & 0x04) // GUID
			dbg.printf(", GUID: %s", alcGetStrGuid(guid));
		if (flags & 0x08) // user defined name
			dbg.printf(", User defined name: %s", userDefName.c_str());
		if (flags & 0x20) // display name
			dbg.printf(", Display name: %s", displayName.c_str());
		if (flags & 0x40) // language
			dbg.printf(", Language: 0");
		return dbg.c_str();
	}
	
	void tvAgeInfoStruct::asHtml(tLog *log, bool shortLog)
	{
		log->print("Age Info: %s<br />\n", str());
	}
	
	//// tvSpawnPoint
	tvSpawnPoint::tvSpawnPoint(const char *title, const char *name, const char *cameraStack)
	{
		flags = 0x00000007;
		this->title.writeStr(title);
		this->name.writeStr(name);
		this->cameraStack.writeStr(cameraStack);
	}
	
	void tvSpawnPoint::store(tBBuf &t)
	{
		//tvSpawnPoint flags
		//Found:
		// always 0x00000007
		//Supposicions:
		// 0x00000007: 3 bits for title, name and cameraStack
		flags = t.getU32();
		if (flags != 0x00000007) throw txProtocolError(_WHERE("The SpawnPoint flag must always be 0x00000007 (it is 0x%08X)", flags));
		t.get(title);
		t.get(name);
		t.get(cameraStack);
	}
	
	void tvSpawnPoint::stream(tBBuf &t) const
	{
		// see store for description of flags
		t.putU32(flags);
		t.put(title);
		t.put(name);
		t.put(cameraStack);
	}
	
	const char *tvSpawnPoint::str(void)
	{
		static char dbg[256];
		sprintf(dbg, "Title: %s, Name: %s, Camera Stack: %s", title.c_str(), name.c_str(), cameraStack.c_str());
		return dbg;
	}
	
	void tvSpawnPoint::asHtml(tLog *log, bool shortLog)
	{
		log->print("Spawn Point: %s<br />\n", str());
	}
	
	//// tvAgeLinkStruct
	void tvAgeLinkStruct::store(tBBuf &t)
	{
		//AgeLinkStruct flags
		//Found:
		// 0x0023 In VaultTasks
		// 0x0033 In FindAge msg's
		// 0x0073 Found when linking to Ahnonay (temple) from Restoration Guild
		//Supposicions:
		// 0x0023: 3 bits for AgeInfoStruct LinkingRules and SpawnPoint (must always be set)
		// 0x0010: CCR flag
		// 0x0040: Parent age name (according to libPlasma)
		flags = t.getU16();
		U16 check = 0x0023 | 0x0010 | 0x0040;
		if (flags & ~(check))
			throw txProtocolError(_WHERE("unknown flag 0x%04X for AgeLinkStruct", flags));
		if (!(flags & 0x0023)) // this must always be set (AgeInfoStruct, LinkingRules and SpawnPoint)
			throw txProtocolError(_WHERE("the 0x0023 flag must always be set in AgeLinkStruct"));
		
		t.get(ageInfo);
		linkingRule = t.getByte();
		U32 unk = t.getU32(); // unknown, always seen 0x00000001
		if (unk != 0x00000001)
			throw txProtocolError(_WHERE("unknown unk value for AgeLinkStruct, must always be 0x00000001 but is 0x%08X", unk));
		t.get(spawnPoint);
		
		// now come the optional fields
		if (flags & 0x0010) // CCR
			ccr = t.getByte();
		else
			ccr = 0;
		
		if (flags & 0x0040) // parent age name
			t.get(parentAgeName);
	}
	
	void tvAgeLinkStruct::stream(tBBuf &t) const
	{
		// see store for description of flags
		t.putU16(flags);
		
		t.put(ageInfo);
		t.putByte(linkingRule);
		t.putU32(0x00000001); // unknown
		t.put(spawnPoint);
		
		// optional fields
		if (flags & 0x0010) // CCR
			t.putByte(ccr);
		if (flags & 0x0040) // parent age name
			t.put(parentAgeName);
	}
	
	const char *tvAgeLinkStruct::str(void)
	{
		dbg.clear();
		dbg.printf("Age Info [%s], Linking Rule: 0x%02X (%s), Spawn Point [%s]", ageInfo.str(), linkingRule, alcUnetGetLinkingRule(linkingRule), spawnPoint.str());
		if (flags & 0x0010) // CCR
			dbg.printf(", CCR: 0x%02X", ccr);
		if (flags & 0x0040) // parent age name
			dbg.printf(", Parent Age: %s", parentAgeName.c_str());
		return dbg.c_str();
	}
	
	void tvAgeLinkStruct::asHtml(tLog *log, bool shortLog)
	{
		log->print("Flags: 0x%04X<br />\n", flags);
		ageInfo.asHtml(log, shortLog);
		log->print("Linking rule: 0x%02X<br />\n", linkingRule);
		spawnPoint.asHtml(log, shortLog);
		if (flags & 0x0010) // CCR
			log->print("CCR: 0x%02X<br />\n", ccr);
		if (flags & 0x0040) // parent age name
			log->print("Parent Age: %s<br />\n", parentAgeName.c_str());
	}
	
	//// tvManifest
	tvManifest::tvManifest(U32 id, double timestamp)
	{
		this->id = id;
		this->time = timestamp;
	}
	
	void tvManifest::store(tBBuf &t)
	{
		id = t.getU32();
		time = t.getDouble();
	}
	
	void tvManifest::stream(tBBuf &t) const
	{
		t.putU32(id);
		t.putDouble(time);
	}
	
	void tvManifest::asHtml(tLog *log, bool shortLog)
	{
		log->print("ID: 0x%08X (%d), Stamp: %s<br />\n", id, id, time ? alcGetStrTime(time) : "0");
	}
	
	//// tvNodeRef
	tvNodeRef::tvNodeRef(U32 saver, U32 parent, U32 child, U32 time, U32 microsec, Byte flags)
	{
		this->saver = saver;
		this->parent = parent;
		this->child = child;
		this->time = time;
		this->microsec = microsec;
		this->flags = flags;
	}
	
	tvNodeRef::tvNodeRef(U32 saver, U32 parent, U32 child)
	{
		this->saver = saver;
		this->parent = parent;
		this->child = child;
		this->time = this->microsec = 0;
		this->flags = 0;
	}
	
	void tvNodeRef::store(tBBuf &t)
	{
		saver = t.getU32();
		parent = t.getU32();
		child = t.getU32();
		time = t.getU32();
		microsec = t.getU32();
		flags = t.getByte();
	}
	
	void tvNodeRef::stream(tBBuf &t) const
	{
		t.putU32(saver);
		t.putU32(parent);
		t.putU32(child);
		t.putU32(time);
		t.putU32(microsec);
		t.putByte(flags);
	}
	
	void tvNodeRef::asHtml(tLog *log, bool shortLog)
	{
		log->print("Saver: 0x%08X (%d), Parent:  0x%08X (%d), Child: 0x%08X (%d), ", saver, saver, parent, parent, child, child);
		log->print("Stamp: %s, Flags: 0x%02X<br />\n", time ? alcGetStrTime(time, microsec) : "0", flags);
	}
	
	//// tvCreatableGenericValue
	tvCreatableGenericValue::tvCreatableGenericValue(U32 integer) : tvBase()
	{
		format = DInteger;
		this->integer = integer;
	}
	
	tvCreatableGenericValue::tvCreatableGenericValue(double time) : tvBase()
	{
		format = DTimestamp;
		this->time = time;
	}
	
	tvCreatableGenericValue::tvCreatableGenericValue(const char *str) : tvBase()
	{
		format = DUruString;
		this->str.writeStr(str);
	}
	
	void tvCreatableGenericValue::store(tBBuf &t)
	{
		format = t.getByte();
		
		switch (format) {
			case DInteger:
				integer = t.getU32();
				break;
			case DUruString:
				t.get(str);
				break;
			case DTimestamp:
				time = t.getDouble();
				break;
			default:
				throw txProtocolError(_WHERE("unknown creatable generic value format 0x%02X", format));
		}
	}
	
	void tvCreatableGenericValue::stream(tBBuf &t) const
	{
		t.putByte(format);
		switch (format) {
			case DInteger:
				t.putU32(integer);
				break;
			case DUruString:
				t.put(str);
				break;
			case DTimestamp:
				t.putDouble(time);
				break;
			default:
				throw txProtocolError(_WHERE("unknown creatable generic value format 0x%02X", format));
		}
	}
	
	U32 tvCreatableGenericValue::asInt(void)
	{
		if (format != DInteger)
			throw txProtocolError(_WHERE("expected a GenericValue.format of 0x%02X (DInteger) but got 0x%02X", DInteger, format));
		return integer;
	}
	
	const char *tvCreatableGenericValue::asString(void)
	{
		if (format != DUruString)
			throw txProtocolError(_WHERE("expected a GenericValue.format of 0x%02X (DUruString) but got 0x%02X", DUruString, format));
		return str.c_str();
	}
	
	void tvCreatableGenericValue::asHtml(tLog *log, bool shortLog)
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
				throw txProtocolError(_WHERE("unknown creatable generic value format 0x%02X", format));
		}
	}
	
	//// tvCreatableStream
	tvCreatableStream::tvCreatableStream(Byte id, tvBase **dataList, int nData)
	{
		this->id = id;
		// put the data in the buffer
		tMBuf buf;
		buf.putU32(nData);
		for (int i = 0; i < nData; ++i)
			buf.put(*dataList[i]);
		// put the buffer in our space
		buf.rewind();
		size = buf.size();
		data = (Byte *)malloc(sizeof(Byte) * size);
		if (data == NULL) throw txNoMem(_WHERE("NoMem"));
		memcpy(data, buf.read(size), size);
	}
	
	tvCreatableStream::tvCreatableStream(Byte id, tMBuf &buf)
	{
		this->id = id;
		size = buf.size();
		data = (Byte *)malloc(sizeof(Byte) * size);
		if (data == NULL) throw txNoMem(_WHERE("NoMem"));
		buf.rewind();
		memcpy(data, buf.read(size), size);
	}
	
	void tvCreatableStream::store(tBBuf &t)
	{
		size = t.getU32();
		if (data) free(data);
		data = (Byte *)malloc(sizeof(Byte) * size);
		if (data == NULL) throw txNoMem(_WHERE("NoMem"));
		memcpy(data, t.read(size), size);
	}
	
	void tvCreatableStream::stream(tBBuf &t) const
	{
		t.putU32(size);
		if (data)
			t.write(data, size);
	}
	
	void tvCreatableStream::asHtml(tLog *log, bool shortLog)
	{
		log->print("Size: %d<br />\n", size);
		tMBuf *buf = getData();
		// the format of the content depends on the ID
		switch (id) {
			case 6:
				log->print("<table border='1'>\n");
				while (!buf->eof()) {
					tvNode node;
					buf->get(node);
					node.asHtml(log, shortLog);
				}
				log->print("</table>\n");
				break;
			case 10:
			{
				U16 num = buf->getU16();
				log->print("number of IDs: %d<br />\n", num);
				if (num == 0) break;
				// this is not printed in short log
				// it is read anyway to verify the data
				if (!shortLog) log->print("Value(s):");
				for (U16 i = 0; i < num; ++i) {
					U32 val = buf->getU32();
					if (shortLog) continue;
					if (i > 0) log->print(",");
					log->print(" 0x%08X (%d)", val, val);
				}
				if (!shortLog) log->print("<br />\n");
				break;
			}
			case 14:
			{
				U32 num = buf->getU32();
				log->print("number of manifests: %d<br />\n", num);
				for (U32 i = 0; i < num; ++i) {
					tvManifest manifest;
					buf->get(manifest);
					// this is not printed in short log
					// it is read anyway to verify the data
					if (!shortLog) {
						log->print("[%d] ", i+1);
						manifest.asHtml(log, shortLog);
					}
				}
				break;
			}
			case 15:
			{
				U32 num = buf->getU32();
				log->print("number of VaultNodeRefs: %d<br />\n", num);
				for (U32 i = 0; i < num; ++i) {
					tvNodeRef nodeRef;
					buf->get(nodeRef);
					// this is not printed in short log
					// it is read anyway to verify the data
					if (!shortLog) nodeRef.asHtml(log, shortLog);
				}
				break;
			}
			default:
				log->print("<span style='color:red'>Unknown strange stream data type!</span><br />\n");
				break;
		}
		if (!buf->eof())
			log->print("<span style='color:red'>Strange, this stream has some bytes left where none are expected!</span><br />\n");
		delete buf;
	}
	
	tMBuf *tvCreatableStream::getData(void) const
	{
		tMBuf *buf = new tMBuf;
		buf->write(data, size);
		buf->rewind();
		return buf;
	}
	
	//// tvServerGuid
	void tvServerGuid::store(tBBuf &t)
	{
		memcpy(guid, t.read(8), 8);
	}
	
	void tvServerGuid::stream(tBBuf &t) const
	{
		t.write(guid, 8);
	}
	
	void tvServerGuid::asHtml(tLog *log, bool shortLog)
	{
		log->print("%s<br />\n", alcGetStrGuid(guid));
	}
	
	//// tvNode
	tvNode::tvNode(U32 flagB) : tvBase()
	{
		blob1Size = 0;
		blob1 = NULL;
		
		// set flags to empty
		this->flagA = 0x00000001; // this means that flagC is ignored
		this->flagB = flagB;
		this->flagC = 0x00000000;
	}
	
	tvNode::~tvNode(void)
	{
		if (blob1) free(blob1);
	}
	
	void tvNode::store(tBBuf &t)
	{
		// get flags
		flagA = t.getU32(); // I think this is something like a version number. version 1 contains only flagB, version 2 also flagC
		if (flagA != 0x00000001 && flagA != 0x00000002) { // check for unknown values
			throw txProtocolError(_WHERE("invalid flagA (0x%08X)", flagA));
		}
		flagB = t.getU32(); // this is the main flag, all 32 bits are known
		if (flagA == 0x00000002) { // it contains flagC
			flagC = t.getU32();
			U32 check = MBlob1Guid | MBlob2Guid | 0x00000004; // the latter is unknown and seems to be unused
			if (flagC & ~(check)) { // check for unknown values
				throw txProtocolError(_WHERE("invalid flagC (0x%08X)", flagC));
			}
		}
		else
			flagC = 0;
		
		// mandatory fields - it doesn't matter whether the flags are on or not
		index = t.getU32();
		type = t.getByte();
		permissions = t.getU32();
		if ((permissions & ~(KAllPermissions)) != 0)
			throw txProtocolError(_WHERE("invalid permissions mask (0x%08X)", permissions));
		owner = t.getU32();
		group = t.getU32();
		modTime = t.getU32();
		modTime += t.getU32()/1000000.0; // these are the microseconds
		
		// optional fields
		if (flagB & MCreator)
			creator = t.getU32();
		else
			creator = 0;
		
		if (flagB & MCrtTime) {
			crtTime = t.getU32();
			t.getU32(); // ignore the microseconds
		}
		else
			crtTime = 0;
		
		if (flagB & MAgeCoords) // unused, do nothing
			;
		
		if (flagB & MAgeTime) {
			ageTime = t.getU32();
			t.getU32(); // ignore the microseconds
		}
		else
			ageTime = 0;
		
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
				if (blob1 == NULL) throw txNoMem(_WHERE("NoMem"));
				memcpy(blob1, t.read(blob1Size), blob1Size);
			}
		}
		else
			blob1Size = 0;
		
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
		if (flagC & MBlob2Guid) {
			memcpy(blobGuid, t.read(8), 8);
			if (memcmp(blobGuid, zeroGuid, 8) != 0)
				throw txProtocolError(_WHERE("Blob2Guid must always be zero"));
		}
	}
	
	void tvNode::stream(tBBuf &t) const
	{
		// write flags
		t.putU32(flagA);
		t.putU32(flagB);
		if (flagA == 0x00000002) {
			t.putU32(flagC);
		}
		
		// write mandatory data
		t.putU32(index);
		t.putByte(type);
		t.putU32(permissions);
		t.putU32(owner);
		t.putU32(group);
		U32 modTimeSec = (U32)modTime;
		t.putU32(modTimeSec);
		t.putU32((U32)((modTime-modTimeSec)*1000000)); // microseconds
		
		// write optional data
		if (flagB & MCreator) t.putU32(creator);
		if (flagB & MCrtTime) {
			t.putU32(crtTime);
			t.putU32(0);
		}
		if (flagB & MAgeCoords) // unused, do nothing
			;
		if (flagB & MAgeTime) {
			t.putU32(ageTime);
			t.putU32(0);
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
	
	void tvNode::permissionsAsHtml(tLog *log)
	{
		// make a permission stingas it's common on linux, i.e. rwr-r-r for the default permissions
		char permStr[7] = "------"; // no permissions
		if (permissions & KOwnerRead) permStr[0] = 'r';
		if (permissions & KOwnerWrite) permStr[1] = 'w';
		if (permissions & KGroupRead) permStr[2] = 'r';
		if (permissions & KGroupWrite) permStr[3] = 'w';
		if (permissions & KOtherRead) permStr[4] = 'r';
		if (permissions & KOtherWrite) permStr[5] = 'w';
		log->print("<b>Permissions:</b> 0x%08X (%s)<br />\n", permissions, permStr);
	}
	
	void tvNode::blobAsHtml(tLog *log, const Byte *blob, U32 size)
	{
		char filename[512], path[1024];
		if (type == KImageNode) { // the first 4 bytes are skipped so anything smaller than that would make problems
			log->print("Image note:<br />\n");
			if (size < 4) {
				log->print("<span style='color:red'>Too small to be a picture!</span><br />\n");
				return;
			}
			// get the file name
			sprintf(filename, "%s.%s.%d.%s.jpg", ageName.c_str(), str1.c_str(), index, alcGetStrTime(modTime));
			alcStrFilter(filename); // don't trust user input
			strncpy(path, log->getDir(), 511);
			strncat(path, "data/", 511);
			mkdir(path, 00750); // make sure the path exists
			strncat(path, filename, 1023);
			// save the file
			tFBuf file;
			file.open(path, "wb");
			file.write(blob+4, size-4); // skip the first 4 bytes to make it a valid picture
			file.close();
			log->print("<img src='data/%s' /><br />\n", filename);
		}
		else {
			const char *suffix = "raw";
			// print the data
			switch (type) {
				case KAgeLinkNode:
					log->print("List of linking points:<br />\n");
					suffix = "links";
					break;
				case KTextNoteNode:
					log->print("Text Note:<br />\n");
					suffix = "txt";
					break;
				case KMarkerNode:
					log->print("Marker byte code:<br />\n");
					suffix = "marker";
					break;
				case KSDLNode:
					log->print("SDL byte code:<br />\n");
					suffix = "sdl_byte";
					break;
				default:
					log->print("<span style='color:red'>Unknown byte code</span>:<br />\n");
					break;
			}
			log->print("<pre>");
			log->dumpbuf(blob, size);
			log->print("</pre><br />\n");
			// dump it to a file
			// get the file name
			sprintf(filename, "%s.%s.%d.%s.%s", ageName.c_str(), str1.c_str(), index, alcGetStrTime(modTime), suffix);
			alcStrFilter(filename); // don't trust user input
			strncpy(path, log->getDir(), 511);
			strncat(path, "data/", 511);
			mkdir(path, 00750); // make sure the path exists
			strncat(path, filename, 1023);
			// save the file
			tFBuf file;
			file.open(path, "wb");
			file.write(blob, size);
			file.close();
			log->print("<a href='data/%s'>%s</a><br />\n", filename, filename);
		}
	}
	
	void tvNode::asHtml(tLog *log, bool shortLog)
	{
		// mandatory flieds
		log->print("<tr><th style='background-color:yellow'>Vault Node %d</th></tr>\n", index, index);
		log->print("<tr><td>\n");
		log->print("<b>Flags:</b> 0x%08X (%d), 0x%08X (%d), 0x%08X (%d)<br />\n", flagA, flagA, flagB, flagB, flagC, flagC);
		log->print("<b>Type:</b> 0x%02X (%s)<br />\n", type, alcVaultGetNodeType(type));
		permissionsAsHtml(log);
		log->print("<b>Owner:</b> 0x%08X (%d)<br />\n", owner, owner);
		log->print("<b>Group:</b> 0x%08X (%d)<br />\n", group, group);
		log->print("<b>Modification time:</b> %s<br />\n", alcGetStrTime(modTime));
		// optional fields
		if (!shortLog) { // only print this in long logs
			if (flagB & MCreator) log->print("<b>Creator:</b> 0x%08X (%d)<br />\n", creator, creator);
			if (flagB & MCrtTime) log->print("<b>Create time:</b> %s<br />\n", crtTime ? alcGetStrTime(crtTime) : "0");
			if (flagB & MAgeCoords) log->print("<b>Age coords:</b> unused<br />\n");
			if (flagB & MAgeTime) log->print("<b>Age time:</b> %s<br />\n", ageTime ? alcGetStrTime(ageTime) : "0");
			if (flagB & MAgeName) log->print("<b>Age name:</b> %s<br />\n", ageName.c_str());
			if (flagB & MAgeGuid) log->print("<b>Age guid:</b> %s<br />\n", alcGetStrGuid(ageGuid));
			if (flagB & MInt32_1) {
				if (type == KFolderNode || type == KPlayerInfoListNode || type == KAgeInfoListNode)
					log->print("<b>Int32_1:</b> 0x%08X (%s)<br />\n", int1, alcVaultGetFolderType(int1));
				else log->print("<b>Int32_1:</b> 0x%08X (%d)<br />\n", int1, int1);
			}
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
			if (flagB & MBlob1) {
				log->print("<b>Blob1:</b> Size: %d<br />\n", blob1Size);
				if (blob1Size > 0) blobAsHtml(log, blob1, blob1Size);
			}
			if (flagB & MBlob2) log->print("<b>Blob2:</b> Size: 0<br />\n"); // blob2 is always empty
			// the blob guids are always zero
			if (flagC & MBlob1Guid) log->print("<b>Blob1Guid:</b> 0000000000000000<br />\n");
			if (flagC & MBlob2Guid) log->print("<b>Blob1Guid:</b> 0000000000000000<br />\n");
		}
		log->print("</td></tr>\n");
	}
	
	//// tvItem
	tvItem::tvItem(Byte id, U32 integer) : tvBase()
	{
		this->id = id;
		type = plCreatableGenericValue;
		data = new tvCreatableGenericValue(integer);
	}
	
	tvItem::tvItem(Byte id, double time) : tvBase()
	{
		this->id = id;
		type = plCreatableGenericValue;
		data = new tvCreatableGenericValue(time);
	}
	
	tvItem::tvItem(Byte id, const char *str)
	{
		this->id = id;
		type = plCreatableGenericValue;
		data = new tvCreatableGenericValue(str);
	}
	
	tvItem::tvItem(tvCreatableStream *stream)
	{
		this->id = stream->id;
		type = plCreatableStream;
		data = stream;
	}
	
	tvItem::tvItem(Byte id, tvNodeRef *ref)
	{
		this->id = id;
		type = plVaultNodeRef;
		data = ref;
	}
	
	void tvItem::store(tBBuf &t)
	{
		id = t.getByte();
		Byte unk = t.getByte();
		if (unk != 0) {
			throw txProtocolError(_WHERE("bad item.unk value 0x%02X", unk));
		}
		type = t.getU16();
		if (tpots == 2) { // it's Until Uru, use other type IDs
			if (type == plVaultNode_UU) type = plVaultNode;
			else if (type == plVaultNodeRef_UU) type = plVaultNodeRef;
		}
		
		if (data) delete data;
		switch (type) {
			case plAgeLinkStruct:
				data = new tvAgeLinkStruct;
				break;
			case plCreatableGenericValue:
				data = new tvCreatableGenericValue;
				break;
			case plCreatableStream:
				data = new tvCreatableStream(id);
				break;
			case plServerGuid:
				data = new tvServerGuid;
				break;
			case plVaultNodeRef:
				data = new tvNodeRef;
				break;
			case plVaultNode:
				data = new tvNode;
				break;
			default:
				throw txProtocolError(_WHERE("unknown vault data type 0x%04X", type));
		}
		t.get(*data);
	}
	
	void tvItem::stream(tBBuf &t) const
	{
		if (!data) throw txProtocolError(_WHERE("don\'t have any data to write"));
		t.putByte(id);
		t.putByte(0); // unknown
		U16 sentType = type;
		if (tpots == 2) { // it's Until Uru, use other type IDs
			if (sentType == plVaultNode) sentType = plVaultNode_UU;
			else if (sentType == plVaultNodeRef) sentType = plVaultNodeRef_UU;
		}
		t.putU16(sentType);
		t.put(*data);
	}
	
	U32 tvItem::asInt(void)
	{
		if (type != plCreatableGenericValue)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plCreatableGenericValue", id, alcGetPlasmaType(type)));
		return static_cast<tvCreatableGenericValue *>(data)->asInt();
	}
	
	const char *tvItem::asString(void)
	{
		if (type != plCreatableGenericValue)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plCreatableGenericValue", id, alcGetPlasmaType(type)));
		return static_cast<tvCreatableGenericValue *>(data)->asString();
	}
	
	const Byte *tvItem::asGuid(void)
	{
		if (type != plServerGuid)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plServerGuid", id, alcGetPlasmaType(type)));
		return static_cast<tvServerGuid *>(data)->guid;
	}
	
	tvNode *tvItem::asNode(void)
	{
		if (type != plVaultNode)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plVaultNode", id, alcGetPlasmaType(type)));
		return static_cast<tvNode *>(data);
	}
	
	tvNodeRef *tvItem::asNodeRef(void)
	{
		if (type != plVaultNodeRef)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plVaultNodeRef", id, alcGetPlasmaType(type)));
		return static_cast<tvNodeRef *>(data);
	}
	
	tvAgeLinkStruct *tvItem::asAgeLink(void)
	{
		if (type != plAgeLinkStruct)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plAgeLinkStruct", id, alcGetPlasmaType(type)));
		return static_cast<tvAgeLinkStruct *>(data);
	}
	
	void tvItem::asHtml(tLog *log, bool shortLog)
	{
		log->print("Id: <b>0x%02X (%d)</b>, type: 0x%04X (%s)<br />\n", id, id, type, alcGetPlasmaType(type));
		if (type == plVaultNode) log->print("<table border='1'>\n");
		data->asHtml(log, shortLog);
		if (type == plVaultNode) log->print("</table>\n");
	}
	
	//// tvMessage
	tvMessage::tvMessage(const tvMessage &msg) : tvBase()
	{
		tpots = 0;
		task = msg.task;
		cmd = msg.cmd;
		compress = false;
		context = msg.context;
		vmgr = msg.vmgr;
		vn = msg.vn;
	}
	
	tvMessage::tvMessage(Byte cmd, bool task)
	{
		tpots = 0;
		this->task = task;
		this->cmd = cmd;
		compress = false;
		context = 0;
		vmgr = 0;
		vn = 0;
	}
	
	tvMessage::~tvMessage(void)
	{
		for (tItemList::iterator it = items.begin(); it != items.end(); ++it) {
			delete *it;
		}
	}
	
	void tvMessage::store(tBBuf &t)
	{
		// parse the header
		cmd = t.getByte();
		U16 result = t.getU16();
		if (result != 0) {
			throw txProtocolError(_WHERE("bad 1st result code 0x%04X", result));
		}
		Byte compressed = t.getByte();
		if (compressed != 0x01 && compressed != 0x03)
			throw txProtocolError(_WHERE("unknown compression format 0x%02X", compressed));
		U32 realSize = t.getU32();
		U32 startPos = t.tell(); // remember the pos to verify the real size
		DBG(5, "vault message: command: 0x%02X, compressed: 0x%02X, real size: %d", cmd, compressed, realSize);
		
		tBBuf *buf = &t;
		if (compressed == 0x03) { // it's compressed, so decompress it
			compress = true;
			U32 compressedSize = t.getU32();
			DBGM(5, ", compressed size: %d", compressedSize);
			tZBuf *content = new tZBuf;
			content->write(t.read(compressedSize), compressedSize);
			content->uncompress(realSize);
			buf = content;
		}
		else {
			compress = false;
		}
		
		// get the items
		U16 numItems = buf->getU16();
		DBGM(5, ", number of items: %d\n", numItems);
		items.clear();
		items.reserve(numItems);
		for (int i = 0; i < numItems; ++i) {
			tvItem *item = new tvItem(tpots);
			buf->get(*item);
			items.push_back(item);
		}
		
		if (compressed == 0x03) { // it was compressed, so we have to clean up
			if (!buf->eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after what we parsed above
			delete buf;
		}
		else if (t.tell()-startPos != realSize)
			throw txProtocolError(_WHERE("Size mismatch (the packet says %d but it is %d)",realSize,t.tell()-startPos));
		
		// get remaining info (which is always uncompressed)
		if (task) {
			context = t.getByte(); // in vtask, this is "sub" (?)
			vmgr = t.getU32(); // in vtask, this is the client
			vn = 0; // not existing in vtask
		}
		else {
			context = t.getU16();
			result = t.getU16();
			if (result != 0) {
				throw txProtocolError(_WHERE("bad 2nd result code 0x%04X", result));
			}
			vmgr = t.getU32();
			vn = t.getU16();
		}
		DBG(5, "remaining info: context: %d, vmgr: %d, vn: %d\n", context, vmgr, vn);
		
		if (!t.eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after what we parsed above
	}
	
	void tvMessage::stream(tBBuf &t) const
	{
		t.putByte(cmd);
		t.putU16(0);// result
		t.putByte(compress ? 0x03 : 0x01);
		
		// put the items into a temporary buffer which might be compressed
		tMBuf buf; // this should be created on the stack to avoid leaks when there's an exception
		buf.putU16(items.size());
		for (tItemList::const_iterator it = items.begin(); it != items.end(); ++it) {
			(*it)->tpots = tpots; // make sure the right TPOTS value is used
			buf.put(**it);
		}
		
		// save the real size of the buffer
		t.putU32(buf.size());
		
		if (compress) {
			tZBuf content;
			content.put(buf);
			content.compress();
			t.putU32(content.size());
			t.put(content);
		}
		else {
			t.put(buf);
		}
		
		// put remaining info
		if (task) {
			t.putByte(context);
			t.putU32(vmgr);
		}
		else {
			t.putU16(context);
			t.putU16(0); // result
			t.putU32(vmgr);
			t.putU16(vn);
		}
	}
	
	void tvMessage::print(tLog *log, bool clientToServer, tNetSession *client, bool shortLog, U32 ki)
	{
		static int count = 0;
		if (!log->doesPrint()) return; // don't do anything if log is disabled
		// rotation check
		++count;
		if (count > 50) {
			log->rotate(false); // rotate if file is too big
			count = 0;
		}
		
		char clientDesc[512];
		if (ki) // we're in the vault server and the "client" is only forwarding
			sprintf(clientDesc, "KI %d, routed by %s", ki, client ? client->str() : "?");
		else if (client)
			strncpy(clientDesc, client->str(), 511);
		else
			strcpy(clientDesc, "?");
		if (clientToServer)
			log->print("<h2 style='color:blue'>%s: From client (%s) to vault</h2>\n", alcGetStrTime(), clientDesc);
		else
			log->print("<h2 style='color:green'>%s: From vault to client (%s)</h2>\n", alcGetStrTime(), clientDesc);
		asHtml(log, shortLog);
		log->print("<hr>\n\n");
		log->flush();
	}
	
	void tvMessage::asHtml(tLog *log, bool shortLog)
	{
		// the header
		if (task) log->print("<b>NetMsgVaultTask ");
		else      log->print("<b>NetMsgVault ");
		log->print("CMD: 0x%02X ", cmd);
		if (task) log->print("(%s)</b>, ", alcVaultGetTask(cmd));
		else      log->print("(%s)</b>, ", alcVaultGetCmd(cmd));
		log->print("compressed: %s<br />\n", compress ? "yes" : "no");
		if (task) log->print("sub: %d, <b>client: %d</b><br />\n", context, vmgr);
		else      log->print("context: %d, <b>vmgr: %d</b>, vn: %d<br />\n", context, vmgr, vn);
		
		// the items
		if (items.size() > 0) {
			log->print("<table border='1'>\n");
			int i = 0;
			for (tItemList::iterator it = items.begin(); it != items.end(); ++it) {
				++i;
				log->print("<tr><th style='background-color:cyan;'>Item %d</th></tr>\n", i);
				log->print("<tr><td>\n");
				(*it)->asHtml(log, shortLog);
				log->print("</td></tr>\n");
			}
			log->print("</table>");
		}
	}
	
	const char *alcVaultGetCmd(Byte cmd)
	{
		switch (cmd) {
			case VConnect: return "VConnect";
			case VDisconnect: return "VDisconnect";
			case VAddNodeRef: return "VAddNodeRef";
			case VRemoveNodeRef: return "VRemoveNodeRef";
			case VNegotiateManifest: return "VNegotiateManifest";
			case VSaveNode: return "VSaveNode";
			case VFindNode: return "VFindNode";
			case VFetchNode: return "VFetchNode";
			case VSendNode: return "VSendNode";
			case VSetSeen: return "VSetSeen";
			case VOnlineState: return "VOnlineState";
			default: return "Unknown";
		}
	}
	
	const char *alcVaultGetTask(Byte cmd)
	{
		switch (cmd) {
			case TCreatePlayer: return "TCreatePlayer";
			case TDeletePlayer: return "TDeletePlayer";
			case TGetPlayerList: return "TGetPlayerList";
			case TCreateNeighborhood: return "TCreateNeighborhood";
			case TJoinNeighborhood: return "TJoinNeighborhood";
			case TSetAgePublic: return "TSetAgePublic";
			case TIncPlayerOnlineTime: return "TIncPlayerOnlineTime";
			case TEnablePlayer: return "TEnablePlayer";
			case TRegisterOwnedAge: return "TRegisterOwnedAge";
			case TUnRegisterOwnedAge: return "TUnRegisterOwnedAge";
			case TRegisterVisitAge: return "TRegisterVisitAge";
			case TUnRegisterVisitAge: return "TUnRegisterVisitAge";
			case TFriendInvite: return "TFriendInvite";
			default: return "Unknown";
		}
	}
	
	const char *alcVaultGetNodeType(Byte type)
	{
		switch (type) {
			case KInvalidNode: return "KInvalidNode";
			case KVNodeMgrPlayerNode: return "KVNodeMgrPlayerNode";
			case KVNodeMgrAgeNode: return "KVNodeMgrAgeNode";
			case KVNodeMgrGameServerNode: return "KVNodeMgrGameServerNode";
			case KVNodeMgrAdminNode: return "KVNodeMgrAdminNode";
			case KVNodeMgrServerNode: return "KVNodeMgrServerNode";
			case KVNodeMgrCCRNode: return "KVNodeMgrCCRNode";
			case KFolderNode: return "KFolderNode";
			case KPlayerInfoNode: return "KPlayerInfoNode";
			case KSystem: return "KSystem";
			case KImageNode: return "KImageNode";
			case KTextNoteNode: return "KTextNoteNode";
			case KSDLNode: return "KSDLNode";
			case KAgeLinkNode: return "KAgeLinkNode";
			case KChronicleNode: return "KChronicleNode";
			case KPlayerInfoListNode: return "KPlayerInfoListNode";
			case KMarkerNode: return "KMarkerNode";
			case KAgeInfoNode: return "KAgeInfoNode";
			case KAgeInfoListNode: return "KAgeInfoListNode";
			case KMarkerListNode: return "KMarkerListNode";
			default: return "Unknown";
		}
	}
	
	const char *alcVaultGetFolderType(U32 type)
	{
		switch (type) {
			case KGeneric: return "KGeneric";
			case KInboxFolder: return "KInboxFolder";
			case KBuddyListFolder: return "KBuddyListFolder";
			case KIgnoreListFolder: return "KIgnoreListFolder";
			case KPeopleIKnowAboutFolder: return "KPeopleIKnowAboutFolder";
			case KVaultMgrGlobalDataFolder: return "KVaultMgrGlobalDataFolder";
			case KChronicleFolder: return "KChronicleFolder";
			case KAvatarOutfitFolder: return "KAvatarOutfitFolder";
			case KAgeTypeJournalFolder: return "KAgeTypeJournalFolder";
			case KSubAgesFolder: return "KSubAgesFolder";
			case KDeviceInboxFolder: return "KDeviceInboxFolder";
			case KHoodMembersFolder: return "KHoodMembersFolder";
			case KAllPlayersFolder: return "KAllPlayersFolder";
			case KAgeMembersFolder: return "KAgeMembersFolder";
			case KAgeJournalsFolder: return "KAgeJournalsFolder";
			case KAgeDevicesFolder: return "KAgeDevicesFolder";
			case KAgeInstaceSDLNode: return "KAgeInstaceSDLNode";
			case KAgeGlobalSDLNode: return "KAgeGlobalSDLNode";
			case KCanVisitFolder: return "KCanVisitFolder";
			case KAgeOwnersFolder: return "KAgeOwnersFolder";
			case KAllAgeGlobalSDLNodesFolder: return "KAllAgeGlobalSDLNodesFolder";
			case KPlayerInfoNodeFolder: return "KPlayerInfoNodeFolder";
			case KPublicAgesFolder: return "KPublicAgesFolder";
			case KAgesIOwnFolder: return "KAgesIOwnFolder";
			case KAgesICanVisitFolder: return "KAgesICanVisitFolder";
			case KAvatarClosetFolder: return "KAvatarClosetFolder";
			case KAgeInfoNodeFolder: return "KAgeInfoNodeFolder";
			case KSystemNode: return "KSystemNode";
			case KPlayerInviteFolder: return "KPlayerInviteFolder";
			case KCCRPlayersFolder: return "KCCRPlayersFolder";
			case KGlobalInboxFolder: return "KGlobalInboxFolder";
			case KChildAgesFolder: return "KChildAgesFolder";
			default: return "Unknown";
		}
	}

} //end namespace alc
