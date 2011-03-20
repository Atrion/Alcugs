/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs Server Team                           *
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
#define __U_VAULTROUTER_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "vaultproto.h"

#include "netexception.h"
#include "netsession.h"
#include "protocol.h"
#include <alcutil/alcos.h>
#include <urutypes/plbase.h>

#include <cstring>
#include <sys/stat.h>


namespace alc {

	
	//// tvAgeInfoStruct
	tvAgeInfoStruct::tvAgeInfoStruct(const tString &filename, const tString &instanceName, const tString &userDefName, const tString &displayName, const uint8_t *guid)
	: tvBase(), filename(filename), instanceName(instanceName), userDefName(userDefName), displayName(displayName)
	{
		flags = 0x01 | 0x02 | 0x04 | 0x08 | 0x20; // instanceName, filename, GUID, user defined name, display name
		memcpy(this->guid, guid, 8);
	}
	
	tvAgeInfoStruct::tvAgeInfoStruct(const tString &filename, const uint8_t *guid) : tvBase(), filename(filename)
	{
		flags =  0x02 | 0x04; // filename, GUID
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
		flags = t.get8();
		uint8_t check = 0x02 | 0x01 | 0x04 | 0x08 | 0x20 | 0x40;
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
			uint32_t language = t.get32(); // always seen 0
			if (language != 0) throw txProtocolError(_WHERE("Language value of an AgeInfoStruct is 0x%08X instead of 0\n", language));
		}
	}
	
	void tvAgeInfoStruct::stream(tBBuf &t) const
	{
		// see store for description of flags
		t.put8(flags);
		
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
			t.put32(0);
	}
	
	tString tvAgeInfoStruct::str(void) const
	{
		tString dbg;
		dbg.printf("Filename: %s", filename.c_str());
		if (flags & 0x01) // instance name
			dbg.printf(", Instance Name: %s", instanceName.c_str());
		if (flags & 0x04) // GUID
			dbg.printf(", GUID: %s", alcGetStrGuid(guid).c_str());
		if (flags & 0x08) // user defined name
			dbg.printf(", User defined name: %s", userDefName.c_str());
		if (flags & 0x20) // display name
			dbg.printf(", Display name: %s", displayName.c_str());
		if (flags & 0x40) // language
			dbg.printf(", Language: 0");
		return dbg;
	}
	
	void tvAgeInfoStruct::asHtml(tLog *log, bool /*shortLog*/)
	{
		log->print("Age Info: %s<br />\n", str().c_str());
	}
	
	//// tvSpawnPoint
	tvSpawnPoint::tvSpawnPoint(const tString &title, const tString &name, const tString &cameraStack) : title(title), name(name), cameraStack(cameraStack)
	{
		flags = 0x00000007;
	}
	
	void tvSpawnPoint::store(tBBuf &t)
	{
		//tvSpawnPoint flags
		//Found:
		// always 0x00000007
		//Supposicions:
		// 0x00000007: 3 bits for title, name and cameraStack
		flags = t.get32();
		if (flags != 0x00000007) throw txProtocolError(_WHERE("The SpawnPoint flag must always be 0x00000007 (it is 0x%08X)", flags));
		t.get(title);
		t.get(name);
		t.get(cameraStack);
	}
	
	void tvSpawnPoint::stream(tBBuf &t) const
	{
		// see store for description of flags
		t.put32(flags);
		t.put(title);
		t.put(name);
		t.put(cameraStack);
	}
	
	tString tvSpawnPoint::str(void) const
	{
		tString dbg;
		dbg.printf("Title: %s, Name: %s, Camera Stack: %s", title.c_str(), name.c_str(), cameraStack.c_str());
		return dbg;
	}
	
	void tvSpawnPoint::asHtml(tLog *log, bool /*shortLog*/)
	{
		log->print("Spawn Point: %s<br />\n", str().c_str());
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
		flags = t.get16();
		uint16_t check = 0x0023 | 0x0010 | 0x0040;
		if (flags & ~(check))
			throw txProtocolError(_WHERE("unknown flag 0x%04X for AgeLinkStruct", flags));
		if (!(flags & 0x0023)) // this must always be set (AgeInfoStruct, LinkingRules and SpawnPoint)
			throw txProtocolError(_WHERE("the 0x0023 flag must always be set in AgeLinkStruct"));
		
		t.get(ageInfo);
		linkingRule = t.get8();
		uint32_t unk = t.get32(); // unknown, always seen 0x00000001
		if (unk != 0x00000001)
			throw txProtocolError(_WHERE("unknown unk value for AgeLinkStruct, must always be 0x00000001 but is 0x%08X", unk));
		t.get(spawnPoint);
		
		// now come the optional fields
		if (flags & 0x0010) // CCR
			ccr = t.get8();
		else
			ccr = 0;
		
		if (flags & 0x0040) // parent age name
			t.get(parentAgeName);
	}
	
	void tvAgeLinkStruct::stream(tBBuf &t) const
	{
		// see store for description of flags
		t.put16(flags);
		
		t.put(ageInfo);
		t.put8(linkingRule);
		t.put32(0x00000001); // unknown
		t.put(spawnPoint);
		
		// optional fields
		if (flags & 0x0010) // CCR
			t.put8(ccr);
		if (flags & 0x0040) // parent age name
			t.put(parentAgeName);
	}
	
	tString tvAgeLinkStruct::str(void) const
	{
		tString dbg;
		dbg.printf("Age Info [%s], Linking Rule: 0x%02X (%s), Spawn Point [%s]", ageInfo.str().c_str(), linkingRule, alcUnetGetLinkingRule(linkingRule), spawnPoint.str().c_str());
		if (flags & 0x0010) // CCR
			dbg.printf(", CCR: 0x%02X", ccr);
		if (flags & 0x0040) // parent age name
			dbg.printf(", Parent Age: %s", parentAgeName.c_str());
		return dbg;
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
	tvManifest::tvManifest(uint32_t id, double timestamp)
	{
		this->id = id;
		this->time = timestamp;
	}
	
	void tvManifest::store(tBBuf &t)
	{
		id = t.get32();
		time = t.getDouble();
	}
	
	void tvManifest::stream(tBBuf &t) const
	{
		t.put32(id);
		t.putDouble(time);
	}
	
	void tvManifest::asHtml(tLog *log, bool /*shortLog*/)
	{
		log->print("ID: 0x%08X (%d), Stamp: %s<br />\n", id, id, time ? tTime(time).str().c_str() : "0");
	}
	
	//// tvNodeRef
	tvNodeRef::tvNodeRef(uint32_t saver, uint32_t parent, uint32_t child, uint32_t time, uint32_t microsec, uint8_t flags)
	{
		this->saver = saver;
		this->parent = parent;
		this->child = child;
		this->time = time;
		this->microsec = microsec;
		this->flags = flags;
	}
	
	tvNodeRef::tvNodeRef(uint32_t saver, uint32_t parent, uint32_t child)
	{
		this->saver = saver;
		this->parent = parent;
		this->child = child;
		this->time = this->microsec = 0;
		this->flags = 0;
	}
	
	void tvNodeRef::store(tBBuf &t)
	{
		saver = t.get32();
		parent = t.get32();
		child = t.get32();
		time = t.get32();
		microsec = t.get32();
		flags = t.get8();
	}
	
	void tvNodeRef::stream(tBBuf &t) const
	{
		t.put32(saver);
		t.put32(parent);
		t.put32(child);
		t.put32(time);
		t.put32(microsec);
		t.put8(flags);
	}
	
	void tvNodeRef::asHtml(tLog *log, bool /*shortLog*/)
	{
		log->print("Saver: 0x%08X (%d), Parent:  0x%08X (%d), Child: 0x%08X (%d), ", saver, saver, parent, parent, child, child);
		log->print("Stamp: %s, Flags: 0x%02X<br />\n", time ? tTime(time, microsec).str().c_str() : "0", flags);
	}
	
	//// tvCreatableGenericValue
	tvCreatableGenericValue::tvCreatableGenericValue(uint32_t integer) : tvBase()
	{
		format = DInteger;
		this->integer = integer;
	}
	
	tvCreatableGenericValue::tvCreatableGenericValue(double time) : tvBase()
	{
		format = DTimestamp;
		this->time = time;
	}
	
	tvCreatableGenericValue::tvCreatableGenericValue(const tString &str) : tvBase(), str(str)
	{
		format = DUruString;
	}
	
	void tvCreatableGenericValue::store(tBBuf &t)
	{
		format = t.get8();
		
		switch (format) {
			case DInteger:
				integer = t.get32();
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
		t.put8(format);
		switch (format) {
			case DInteger:
				t.put32(integer);
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
	
	uint32_t tvCreatableGenericValue::asInt(void) const
	{
		if (format != DInteger)
			throw txProtocolError(_WHERE("expected a GenericValue.format of 0x%02X (DInteger) but got 0x%02X", DInteger, format));
		return integer;
	}
	
	const tString &tvCreatableGenericValue::asString(void) const
	{
		if (format != DUruString)
			throw txProtocolError(_WHERE("expected a GenericValue.format of 0x%02X (DUruString) but got 0x%02X", DUruString, format));
		return str;
	}
	
	void tvCreatableGenericValue::asHtml(tLog *log, bool /*shortLog*/)
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
	tvCreatableStream::tvCreatableStream(uint8_t id, tvBase **dataList, int nData)
	{
		this->id = id;
		// put the data in the buffer
		tMBuf buf;
		buf.put32(nData);
		for (int i = 0; i < nData; ++i)
			buf.put(*dataList[i]);
		// put the buffer in our space
		buf.rewind();
		size = buf.size();
		data = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * size));
		if (data == NULL) throw txNoMem(_WHERE("NoMem"));
		memcpy(data, buf.read(size), size);
	}
	
	tvCreatableStream::tvCreatableStream(uint8_t id, tMBuf &buf)
	{
		this->id = id;
		size = buf.size();
		data = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * size));
		if (data == NULL) throw txNoMem(_WHERE("NoMem"));
		buf.rewind();
		memcpy(data, buf.read(size), size);
	}
	
	tvCreatableStream::~tvCreatableStream()
	 { if (data) free(data); }
	
	void tvCreatableStream::store(tBBuf &t)
	{
		size = t.get32();
		if (data) free(data);
		data = static_cast<uint8_t *>(malloc(sizeof(uint8_t) * size));
		if (data == NULL) throw txNoMem(_WHERE("NoMem"));
		memcpy(data, t.read(size), size);
	}
	
	void tvCreatableStream::stream(tBBuf &t) const
	{
		t.put32(size);
		if (data)
			t.write(data, size);
	}
	
	void tvCreatableStream::asHtml(tLog *log, bool shortLog)
	{
		log->print("Size: %d<br />\n", size);
		tSBuf buf = getData();
		// the format of the content depends on the ID
		switch (id) {
			case 6:
				log->print("<table border='1'>\n");
				while (!buf.eof()) {
					tvNode node;
					buf.get(node);
					node.asHtml(log, shortLog);
				}
				log->print("</table>\n");
				break;
			case 10:
			{
				uint16_t num = buf.get16();
				log->print("number of IDs: %d<br />\n", num);
				if (num == 0) break;
				// this is not printed in short log
				// it is read anyway to verify the data
				if (!shortLog) log->print("Value(s):");
				for (uint16_t i = 0; i < num; ++i) {
					uint32_t val = buf.get32();
					if (shortLog) continue;
					if (i > 0) log->print(",");
					log->print(" 0x%08X (%d)", val, val);
				}
				if (!shortLog) log->print("<br />\n");
				break;
			}
			case 14:
			{
				uint32_t num = buf.get32();
				log->print("number of manifests: %d<br />\n", num);
				for (uint32_t i = 0; i < num; ++i) {
					tvManifest manifest;
					buf.get(manifest);
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
				uint32_t num = buf.get32();
				log->print("number of VaultNodeRefs: %d<br />\n", num);
				for (uint32_t i = 0; i < num; ++i) {
					tvNodeRef nodeRef;
					buf.get(nodeRef);
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
		if (!buf.eof())
			log->print("<span style='color:red'>Strange, this stream has some bytes left where none are expected!</span><br />\n");
	}
	
	tSBuf tvCreatableStream::getData(void) const
	{
		tSBuf buf = tSBuf(data, size);
		buf.rewind();
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
	
	void tvServerGuid::asHtml(tLog *log, bool /*shortLog*/)
	{
		log->print("%s<br />\n", alcGetStrGuid(guid).c_str());
	}
	
	//// tvNode
	tvNode::tvNode(uint32_t flagB) : tvBase()
	{
		// set flags to empty
		this->flagB = flagB;
		this->flagC = 0x00000000;
	}
	
	void tvNode::store(tBBuf &t)
	{
		// get flags
		uint32_t size = t.get32(); // this is the number of U32 values in the bitvector. Seen 1 and 2 only.
		if (size != 1 && size != 2) { // check for unknown values
			throw txProtocolError(_WHERE("invalid bitvector size (%d)", size));
		}
		flagB = t.get32(); // this is the main flag, all 32 bits are known
		if (size == 2) { // it contains flagC
			flagC = t.get32();
			uint32_t check = MBlob1Guid | MBlob2Guid | 0x00000004; // the latter is unknown and seems to be unused
			if (flagC & ~(check)) { // check for unknown values
				throw txProtocolError(_WHERE("invalid flagC (0x%08X)", flagC));
			}
		}
		else
			flagC = 0;
		
		// mandatory fields - it doesn't matter whether the flags are on or not
		index = t.get32();
		type = t.get8();
		permissions = t.get32();
		if ((permissions & ~(KAllPermissions)) != 0)
			throw txProtocolError(_WHERE("invalid permissions mask (0x%08X)", permissions));
		owner = t.get32();
		group = t.get32();
		modTime = t.get32();
		modTime += t.get32()/1000000.0; // these are the microseconds
		
		// optional fields
		if (flagB & MCreator)
			creator = t.get32();
		else
			creator = 0;
		
		if (flagB & MCrtTime) {
			crtTime = t.get32();
			t.get32(); // ignore the microseconds
		}
		else
			crtTime = 0;
		
		if (flagB & MAgeCoords) // unused, do nothing
			{}
		
		if (flagB & MAgeTime) {
			ageTime = t.get32();
			t.get32(); // ignore the microseconds
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
			int1 = t.get32();
		else
			int1 = 0;
		
		if (flagB & MInt32_2)
			int2 = t.get32();
		else
			int2 = 0;
		
		if (flagB & MInt32_3)
			int3 = t.get32();
		else
			int3 = 0;
		
		if (flagB & MInt32_4)
			int4 = t.get32();
		else
			int4 = 0;
		
		if (flagB & MUInt32_1)
			uInt1 = t.get32();
		else
			uInt1 = 0;
		
		if (flagB & MUInt32_2)
			uInt2 = t.get32();
		else
			uInt2 = 0;
		
		if (flagB & MUInt32_3)
			uInt3 = t.get32();
		else
			uInt3 = 0;
		
		if (flagB & MUInt32_4)
			uInt4 = t.get32();
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
		
		blob1.clear();
		if (flagB & MBlob1) {
			uint32_t blob1Size = t.get32();
			blob1.write(t.read(blob1Size), blob1Size);
		}
		
		if (flagB & MBlob2) {
			uint32_t blob2Size = t.get32();
			if (blob2Size > 0)
				throw txProtocolError(_WHERE("Blob2Size > 0 - unimplemented"));
		}
		
		// the two blob guids must always be 0
		uint8_t blobGuid[8], zeroGuid[8];
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
		t.put32(flagC != 0 ? 2 : 1); // the number of U32 values in the bitvector
		t.put32(flagB);
		if (flagC != 0) {
			t.put32(flagC);
		}
		
		// write mandatory data
		t.put32(index);
		t.put8(type);
		t.put32(permissions);
		t.put32(owner);
		t.put32(group);
		uint32_t modTimeSec = static_cast<uint32_t>(modTime);
		t.put32(modTimeSec);
		t.put32(static_cast<uint32_t>((modTime-modTimeSec)*1000000)); // microseconds
		
		// write optional data
		if (flagB & MCreator) t.put32(creator);
		if (flagB & MCrtTime) {
			t.put32(crtTime);
			t.put32(0);
		}
		if (flagB & MAgeCoords) // unused, do nothing
			{}
		if (flagB & MAgeTime) {
			t.put32(ageTime);
			t.put32(0);
		}
		if (flagB & MAgeName) t.put(ageName);
		if (flagB & MAgeGuid) t.write(ageGuid, 8);
		if (flagB & MInt32_1) t.put32(int1);
		if (flagB & MInt32_2) t.put32(int2);
		if (flagB & MInt32_3) t.put32(int3);
		if (flagB & MInt32_4) t.put32(int4);
		if (flagB & MUInt32_1) t.put32(uInt1);
		if (flagB & MUInt32_2) t.put32(uInt2);
		if (flagB & MUInt32_3) t.put32(uInt3);
		if (flagB & MUInt32_4) t.put32(uInt4);
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
			t.put32(blob1.size());
			t.put(blob1);
		}
		if (flagB & MBlob2) {
			t.put32(0); // blob2 is always empty
		}
		// the two blob guids are always zero
		uint8_t zeroGuid[8];
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
	
	void tvNode::blobAsHtml(tLog *log, const tMBuf &blob)
	{
		tString filename, path;
		if (type == KImageNode) { // the first 4 bytes are skipped so anything smaller than that would make problems
			log->print("Image note:<br />\n");
			if (blob1.size() < 4) {
				log->print("<span style='color:red'>Too small to be a picture!</span><br />\n");
				return;
			}
			// get the file name
			filename.printf("%s.%s.%d.%s.jpg", ageName.c_str(), str1.c_str(), index, tTime(modTime).str().c_str());
			filename = alcStrFiltered(filename); // don't trust user input
			path = log->getDir() + "data/";
			alcMkdir(path, 00750); // make sure the path exists
			path += filename;
			// save the file
			tFBuf file;
			file.open(path.c_str(), "wb");
			file.write(blob.data()+4, blob.size()-4); // skip the first 4 bytes to make it a valid picture
			file.close();
			log->print("<img src='data/%s' /><br />\n", filename.c_str());
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
			log->dumpbuf(blob.data(), blob.size());
			log->print("</pre><br />\n");
			// dump it to a file
			// get the file name
			filename.printf("%s.%s.%d.%s.%s", ageName.c_str(), str1.c_str(), index, tTime(modTime).str().c_str(), suffix);
			filename = alcStrFiltered(filename); // don't trust user input
			path = log->getDir() + "data/";
			alcMkdir(path, 00750); // make sure the path exists
			path += filename;
			// save the file
			tFBuf file;
			file.open(path.c_str(), "wb");
			file.put(blob);
			file.close();
			log->print("<a href='data/%s'>%s</a><br />\n", filename.c_str(), filename.c_str());
		}
	}
	
	void tvNode::asHtml(tLog *log, bool shortLog)
	{
		// mandatory flieds
		log->print("<tr><th style='background-color:yellow'>Vault Node %d</th></tr>\n", index, index);
		log->print("<tr><td>\n");
		log->print("<b>Flags:</b> 0x%08X (%d), 0x%08X (%d)<br />\n", flagB, flagB, flagC, flagC);
		log->print("<b>Type:</b> 0x%02X (%s)<br />\n", type, alcVaultGetNodeType(type));
		permissionsAsHtml(log);
		log->print("<b>Owner:</b> 0x%08X (%d)<br />\n", owner, owner);
		log->print("<b>Group:</b> 0x%08X (%d)<br />\n", group, group);
		log->print("<b>Modification time:</b> %s<br />\n", tTime(modTime).str().c_str());
		// optional fields
		if (!shortLog) { // only print this in long logs
			if (flagB & MCreator) log->print("<b>Creator:</b> 0x%08X (%d)<br />\n", creator, creator);
			if (flagB & MCrtTime) log->print("<b>Create time:</b> %s<br />\n", crtTime ? tTime(crtTime).str().c_str() : "0");
			if (flagB & MAgeCoords) log->print("<b>Age coords:</b> unused<br />\n");
			if (flagB & MAgeTime) log->print("<b>Age time:</b> %s<br />\n", ageTime ? tTime(ageTime).str().c_str() : "0");
			if (flagB & MAgeName) log->print("<b>Age name:</b> %s<br />\n", ageName.c_str());
			if (flagB & MAgeGuid) log->print("<b>Age guid:</b> %s<br />\n", alcGetStrGuid(ageGuid).c_str());
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
				log->print("<b>Blob1:</b> Size: %d<br />\n", blob1.size());
				if (!blob1.isEmpty()) blobAsHtml(log, blob1);
			}
			if (flagB & MBlob2) log->print("<b>Blob2:</b> Size: 0<br />\n"); // blob2 is always empty
			// the blob guids are always zero
			if (flagC & MBlob1Guid) log->print("<b>Blob1Guid:</b> 0000000000000000<br />\n");
			if (flagC & MBlob2Guid) log->print("<b>Blob1Guid:</b> 0000000000000000<br />\n");
		}
		log->print("</td></tr>\n");
	}
	
	//// tvItem
	tvItem::tvItem(uint8_t id, uint32_t integer) : tvBase()
	{
		this->id = id;
		type = plCreatableGenericValue;
		data = new tvCreatableGenericValue(integer);
	}
	
	tvItem::tvItem(uint8_t id, double time) : tvBase()
	{
		this->id = id;
		type = plCreatableGenericValue;
		data = new tvCreatableGenericValue(time);
	}
	
	tvItem::tvItem(uint8_t id, const tString &str)
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
	
	tvItem::tvItem(uint8_t id, tvNodeRef *ref)
	{
		this->id = id;
		type = plVaultNodeRef;
		data = ref;
	}
	
	void tvItem::store(tBBuf &t)
	{
		id = t.get8();
		uint8_t unk = t.get8();
		if (unk != 0) {
			throw txProtocolError(_WHERE("bad item.unk value 0x%02X", unk));
		}
		type = t.get16();
		// it's Until Uru, use other type IDs (some are incremented by 1 in POTS) - remember to also update tvItem::stream!
		if (UUFormat) type = alcOpcodeUU2POTS(type);
		
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
		t.put8(id);
		t.put8(0); // unknown
		uint16_t sentType = type;
		// it's Until Uru, use other type IDs (some are incremented by 1 in POTS) - remember to also update tvItem::store!
		if (UUFormat) sentType = alcOpcodePOTS2UU(type);
		t.put16(sentType);
		t.put(*data);
	}
	
	uint32_t tvItem::asInt(void) const
	{
		if (type != plCreatableGenericValue)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plCreatableGenericValue", id, alcGetPlasmaType(type)));
		return static_cast<tvCreatableGenericValue *>(data)->asInt();
	}
	
	const tString &tvItem::asString(void) const
	{
		if (type != plCreatableGenericValue)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plCreatableGenericValue", id, alcGetPlasmaType(type)));
		return static_cast<tvCreatableGenericValue *>(data)->asString();
	}
	
	const uint8_t *tvItem::asGuid(void) const
	{
		if (type != plServerGuid)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plServerGuid", id, alcGetPlasmaType(type)));
		return static_cast<tvServerGuid *>(data)->guid;
	}
	
	tvNode *tvItem::asNode(void) const
	{
		if (type != plVaultNode)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plVaultNode", id, alcGetPlasmaType(type)));
		return static_cast<tvNode *>(data);
	}
	
	tvNodeRef *tvItem::asNodeRef(void) const
	{
		if (type != plVaultNodeRef)
			throw txProtocolError(_WHERE("vault item with id %d is a %s, but I expected a plVaultNodeRef", id, alcGetPlasmaType(type)));
		return static_cast<tvNodeRef *>(data);
	}
	
	tvAgeLinkStruct *tvItem::asAgeLink(void) const
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
		operator=(msg);
	}
	
	tvMessage::tvMessage(uint8_t cmd, bool task)
	{
		UUFormat = false;
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
	
	const tvMessage &tvMessage::operator=(const tvMessage &msg)
	{
		UUFormat = false;
		task = msg.task;
		cmd = msg.cmd;
		compress = false;
		context = msg.context;
		vmgr = msg.vmgr;
		vn = msg.vn;
		return *this;
	}
	
	void tvMessage::store(tBBuf &t)
	{
		// parse the header
		cmd = t.get8();
		uint16_t result = t.get16();
		if (result != 0) {
			throw txProtocolError(_WHERE("bad 1st result code 0x%04X", result));
		}
		uint8_t compressed = t.get8();
		if (compressed != 0x01 && compressed != 0x03)
			throw txProtocolError(_WHERE("unknown compression format 0x%02X", compressed));
		uint32_t realSize = t.get32();
		size_t startPos = t.tell(); // remember the pos to verify the real size
		DBG(5, "vault message: command: 0x%02X, compressed: 0x%02X, real size: %d", cmd, compressed, realSize);
		
		tBBuf *buf = &t;
		if (compressed == 0x03) { // it's compressed, so decompress it
			compress = true;
			uint32_t compressedSize = t.get32();
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
		uint16_t numItems = buf->get16();
		DBGM(5, ", number of items: %d\n", numItems);
		items.clear();
		items.reserve(numItems);
		for (uint16_t i = 0; i < numItems; ++i) {
			tvItem *item = new tvItem(UUFormat);
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
			context = t.get8(); // in vtask, this is "sub" (?)
			vmgr = t.get32(); // in vtask, this is the client
			vn = 0; // not existing in vtask
		}
		else {
			context = t.get16();
			result = t.get16();
			if (result != 0) {
				throw txProtocolError(_WHERE("bad 2nd result code 0x%04X", result));
			}
			vmgr = t.get32();
			vn = t.get16();
		}
		DBG(5, "remaining info: context: %d, vmgr: %d, vn: %d\n", context, vmgr, vn);
		
		if (!t.eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after what we parsed above
	}
	
	void tvMessage::stream(tBBuf &t) const
	{
		t.put8(cmd);
		t.put16(0);// result
		t.put8(compress ? 0x03 : 0x01);
		
		// put the items into a temporary buffer which might be compressed
		tMBuf buf; // this should be created on the stack to avoid leaks when there's an exception
		buf.put16(items.size());
		for (tItemList::const_iterator it = items.begin(); it != items.end(); ++it) {
			(*it)->UUFormat = UUFormat; // make sure the right TPOTS value is used
			buf.put(**it);
		}
		
		// save the real size of the buffer
		t.put32(buf.size());
		
		if (compress) {
			tZBuf content;
			content.put(buf);
			content.compress();
			t.put32(content.size());
			t.put(content);
		}
		else {
			t.put(buf);
		}
		
		// put remaining info
		if (task) {
			t.put8(context);
			t.put32(vmgr);
		}
		else {
			t.put16(context);
			t.put16(0); // result
			t.put32(vmgr);
			t.put16(vn);
		}
	}
	
	void tvMessage::print(tLog *log, bool clientToServer, tNetSession *client, bool shortLog, uint32_t ki)
	{
		if (!log->doesPrint()) return; // don't do anything if log is disabled
		log->checkRotate(50); // check whether we should rotate each 50th packet
			
		tString clientDesc;
		if (ki) // we're in the vault server and the "client" is only forwarding
			clientDesc.printf("KI %d, routed by %s", ki, client ? client->str().c_str() : "?");
		else if (client)
			clientDesc = client->str();
		else
			clientDesc = "?";
		if (clientToServer)
			log->print("<h2 style='color:blue'>%s: From client (%s) to vault</h2>\n", tTime::now().str().c_str(), clientDesc.c_str());
		else
			log->print("<h2 style='color:green'>%s: From vault to client (%s)</h2>\n", tTime::now().str().c_str(), clientDesc.c_str());
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
	
	const char *alcVaultGetCmd(uint8_t cmd)
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
	
	const char *alcVaultGetTask(uint8_t cmd)
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
	
	const char *alcVaultGetNodeType(uint8_t type)
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
	
	const char *alcVaultGetFolderType(uint32_t type)
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
