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

#ifndef __U_VAULTROUTER_H
#define __U_VAULTROUTER_H
/* CVS tag - DON'T TOUCH*/
#define __U_VAULTROUTER_H_ID "$Id$"

namespace alc {

//vault node flag masks                             (Vault Manager names)
#define MIndex     0x00000001 //00000001 (flagB 1) [Index]
#define MType      0x00000002 //00000010 (flagB 1) [Type]
#define MPerms     0x00000004 //00000100 (flagB 1) [Permissions]
#define MOwner     0x00000008 //00001000 (flagB 1) [Owner ID]
#define MGroup     0x00000010 //00010000 (flagB 1) [Group ID]
#define MModTime   0x00000020 //00100000 (flagB 1) [Modify Time]
#define MCreator   0x00000040 //01000000 (flagB 1) [Creator ID]
#define MCrtTime   0x00000080 //10000000 (flagB 1) [Create Time]
#define MAgeCoords 0x00000100 //00000001 (flagB 2) [Create Age Coords]
#define MAgeTime   0x00000200 //00000010 (flagB 2) [Create Age Time]
#define MAgeName   0x00000400 //00000100 (flagB 2) [Create Age Name]
#define MAgeGuid   0x00000800 //00001000 (flagB 2) [Create Age Guid]
#define MInt32_1   0x00001000 //00010000 (flagB 2) [Int32_1]
#define MInt32_2   0x00002000 //00100000 (flagB 2) [Int32_2]
#define MInt32_3   0x00004000 //01000000 (flagB 2) [Int32_3]
#define MInt32_4   0x00008000 //10000000 (flagB 2) [Int32_4]
#define MUInt32_1  0x00010000 //00000001 (flagB 3) [UInt32_1]
#define MUInt32_2  0x00020000 //00000010 (flagB 3) [UInt32_2]
#define MUInt32_3  0x00040000 //00000100 (flagB 3) [UInt32_3]
#define MUInt32_4  0x00080000 //00001000 (flagB 3) [UInt32_4]
#define MStr64_1   0x00100000 //00010000 (flagB 3) [String64_1]
#define MStr64_2   0x00200000 //00100000 (flagB 3) [String64_2]
#define MStr64_3   0x00400000 //01000000 (flagB 3) [String64_3]
#define MStr64_4   0x00800000 //10000000 (flagB 3) [String64_4]
#define MStr64_5   0x01000000 //00000001 (flagB 4) [String64_5]
#define MStr64_6   0x02000000 //00000010 (flagB 4) [String64_6]
#define MlStr64_1  0x04000000 //00000100 (flagB 4) [lString64_1]
#define MlStr64_2  0x08000000 //00001000 (flagB 4) [lString64_2]
#define MText_1    0x10000000 //00010000 (flagB 4) [Text_1]
#define MText_2    0x20000000 //00100000 (flagB 4) [Text_2]
#define MBlob1     0x40000000 //01000000 (flagB 4) [Blob1]
#define MBlob2     0x80000000 //10000000 (flagB 4) [Blob2]
#define MBlob1Guid 0x00000001 //00000001 (flagC 1) [Blob1_guid]
#define MBlob2Guid 0x00000002 //00000010 (flagC 1) [Blob2_guid]

//vault operations
#define VConnect 0x01
#define VDisconnect 0x02
#define VAddNodeRef 0x03
#define VRemoveNodeRef 0x04
#define VNegotiateManifest 0x05
#define VSaveNode 0x06
#define VFindNode 0x07
#define VFetchNode 0x08
#define VSendNode 0x09
#define VSetSeen 0x0A
#define VOnlineState 0x0B

//data types
#define DCreatableGenericValue 0x0387
#define DCreatableStream       0x0389
#define DVaultNode             0x0439
#define DVaultNode2            0x043A // TPOTS

//sub data types
#define DInteger 0x00 // (4 bytes) integer
#define DUruString 0x03 // string
#define DTimestamp 0x07 // (8 bytes) timestamp as double

	const char *alcVaultGetCmd(Byte cmd);
	const char *alcVaultGetTaskCmd(Byte cmd);
	const char *alcVaultGetDataType(U16 type);

	////DEFINITIONS
	class tvBase : public tBaseType {
	public:
		tvBase(void) : tBaseType() {}
		virtual void asHtml(tLog *log) = 0;
	};
	
	class tvCreatableGenericValue : public tvBase {
	public:
		tvCreatableGenericValue(void) : tvBase() { str.setVersion(5); /* inverted UruString */ }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log);
	private:
		Byte format;
		S32 integer;
		tUStr str;
		double time;
	};
	
	class tvCreatableStream : public tvBase {
	public:
		tvCreatableStream(Byte id) : tvBase() { this->id = id; data = NULL; }
		virtual ~tvCreatableStream(void) { if (data) free(data); }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log);
	private:
		U32 size; // only defined when data != NULL
		Byte id;
		Byte *data;
	};
	
	class tvNode : public tvBase {
	public:
		tvNode(void) : tvBase() { blob1Size = 0; blob1 = NULL; }
		virtual ~tvNode(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log);
	private:
		void flagsAsHtml(tLog *log);
		void permissionsAsHtml(tLog *log);
	
		U32 flagA, flagB, flagC;
		U32 index;
		Byte type;
		U32 permissions;
		S32 owner;
		U32 group;
		U32 modTime, modMicrosec;
		U32 creator;
		U32 crtTime, crtMicrosec;
		U32 ageTime, ageMicrosec;
		tUStr ageName;
		Byte ageGuid[8];
		U32 int1, int2, int3, int4;
		U32 uInt1, uInt2, uInt3, uInt4;
		tUStr str1, str2, str3, str4, str5, str6;
		tUStr lStr1, lStr2;
		tUStr text1, text2;
		U32 blob1Size;
		Byte *blob1;
	};
	
	class tvItem : public tvBase {
	public:
		tvItem(Byte tpots) : tvBase() { this->tpots = tpots; data = NULL; }
		virtual ~tvItem(void) { if (data) delete data; }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log);
		
		Byte tpots; // 1: generate/parse for TPOTS client, everything else: for non-TPOTS client (or the vault server)
	private:
		Byte id;
		U16 type;
		tvBase *data;
	};
	
	class tvMessage : public tvBase {
	public:
		tvMessage(bool isTask, Byte tpots) : tvBase() { task = isTask; this->tpots = tpots; items = NULL; }
		virtual ~tvMessage(void);
		virtual void store(tBBuf &t); //!< unpacks the message
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log);
		void print(tLog *log, bool clientToServer, tNetSession *client);
		
		Byte tpots; // 1: generate/parse for TPOTS client, everything else: for non-TPOTS client (or the vault server)
	private:
		bool task;
		Byte cmd; //!< the vault command
		Byte compressed; //!< 1 when uncompressed, 3 when compressed
		U32 realSize; //!< the real size of the message
		U16 numItems; //!< number of items
		tvItem **items;
		U16 context; // sub in VaultTask
		U32 vmgr; // client in VaultTask
		U16 vn; // what is that?
		
	};
	
	
} //End alc namespace

#endif
