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

//define the base vault index
#define KVaultID 20001

//node types
#define KInvalidNode 0x00

#define KVNodeMgrPlayerNode 0x02
#define KVNodeMgrAgeNode 0x03
#define KVNodeMgrGameServerNode 0x04
#define KVNodeMgrAdminNode 0x05
#define KVNodeMgrServerNode 0x06
#define KVNodeMgrCCRNode 0x07

#define KFolderNode 0x16
#define KPlayerInfoNode 0x17
#define KSystem 0x18
#define KImageNode 0x19
#define KTextNoteNode 0x1A
#define KSDLNode 0x1B
#define KAgeLinkNode 0x1C
#define KChronicleNode 0x1D
#define KPlayerInfoListNode 0x1E

#define KMarkerNode 0x20
#define KAgeInfoNode 0x21
#define KAgeInfoListNode 0x22
#define KMarkerListNode 0x23

//folder types
#define KGeneric 0
#define KInboxFolder 1
#define KBuddyListFolder 2
#define KIgnoreListFolder 3
#define KPeopleIKnowAboutFolder 4
#define KVaultMgrGlobalDataFolder 5
#define KChronicleFolder 6
#define KAvatarOutfitFolder 7
#define KAgeTypeJournalFolder 8
#define KSubAgesFolder 9
#define KDeviceInboxFolder 10
#define KHoodMembersFolder 11
#define KAllPlayersFolder 12
#define KAgeMembersFolder 13
#define KAgeJournalsFolder 14
#define KAgeDevicesFolder 15
#define KAgeInstaceSDLNode 16
#define KAgeGlobalSDLNode 17
#define KCanVisitFolder 18
#define KAgeOwnersFolder 19
#define KAllAgeGlobalSDLNodesFolder 20
#define KPlayerInfoNodeFolder 21
#define KPublicAgesFolder 22
#define KAgesIOwnFolder 23
#define KAgesICanVisitFolder 24
#define KAvatarClosetFolder 25
#define KAgeInfoNodeFolder 26
#define KSystemNode 27
#define KPlayerInviteFolder 28
#define KCCRPlayersFolder 29
#define KGlobalInboxFolder 30
#define KChildAgesFolder 31

//permissions
#define KOwnerRead 0x01
#define KOwnerWrite 0x02
#define KGroupRead 0x04
#define KGroupWrite 0x08
#define KOtherRead 0x10
#define KOtherWrite 0x20
#define KDefaultPermissions  KOwnerRead | KOwnerWrite | KGroupRead | KOtherRead
#define KAllPermissions KOwnerRead | KOwnerWrite | KGroupRead | KGroupWrite | KOtherRead | KOtherWrite

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

//vault tasks
#define TCreatePlayer 0x01
#define TDeletePlayer 0x02
#define TGetPlayerList 0x03
#define TCreateNeighborhood 0x04
#define TJoinNeighborhood 0x05
#define TSetAgePublic 0x06
#define TIncPlayerOnlineTime 0x07
#define TEnablePlayer 0x08
#define TRegisterOwnedAge 0x09
#define TUnRegisterOwnedAge 0x0A
#define TRegisterVisitAge 0x0B
#define TUnRegisterVisitAge 0x0C
#define TFriendInvite 0x0D

//data types
#define DAgeLinkStruct         0x02BF
#define DCreatableGenericValue 0x0387
#define DCreatableStream       0x0389
#define DServerGuid            0x034D
#define DVaultNodeRef          0x0438
#define DVaultNodeRef2         0x0439 // TPOTS
#define DVaultNode             0x0439
#define DVaultNode2            0x043A // TPOTS

//sub data types (incomplete)
#define DInteger 0x00 // (4 bytes) integer
#define DUruString 0x03 // string
#define DTimestamp 0x07 // (8 bytes) timestamp as double

	const char *alcVaultGetCmd(Byte cmd);
	const char *alcVaultGetTask(Byte cmd);
	const char *alcVaultGetDataType(U16 type);
	const char *alcVaultGetNodeType(Byte type);
	const char *alcVaultGetFolderType(U32 type);

	////DEFINITIONS
	class tvBase : public tBaseType {
	public:
		tvBase(void) : tBaseType() {}
		virtual void asHtml(tLog *log, bool shortLog) = 0; //!< print the data as HTML. Also does further verification for some types.
	};
	
	class tvAgeInfoStruct : public tvBase {
	public:
		tvAgeInfoStruct(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog) {}
		const Byte *str(void);
		// format
		Byte flags;
		tUStr filename, instanceName;
		Byte guid[8];
		tUStr userDefName, displayName;
		U32 language;
	private:
		tStrBuf dbg;
	};
	
	class tvSpawnPoint : public tvBase {
	public:
		tvSpawnPoint(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog) {}
		const Byte *str(void);
		// format
		U32 flags;
		tUStr title, name, cameraStack;
	};
	
	class tvAgeLinkStruct : public tvBase {
	public:
		tvAgeLinkStruct(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog) {}
		const Byte *str(void);
		// format
		U16 flags;
		tvAgeInfoStruct ageInfo;
		Byte linkingRule;
		tvSpawnPoint spawnPoint;
		Byte ccr;
	private:
		tStrBuf dbg;
	};
	
	class tvCreatableGenericValue : public tvBase {
	public:
		tvCreatableGenericValue(S32 integer);
		tvCreatableGenericValue(Byte *str);
		tvCreatableGenericValue(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog);
		
		S32 asInt(void);
		// format
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
		virtual void asHtml(tLog *log, bool shortLog);
		// format
		U32 size; // only defined when data != NULL
		Byte id;
		Byte *data;
	};
	
	class tvServerGuid : public tvBase {
	public:
		tvServerGuid(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog);
		// format
		Byte guid[8];
	};
	
	class tvManifest : public tvBase {
	public:
		tvManifest(void) : tvBase() {}
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog);
		// format
		U32 id;
		double time;
	};
	
	class tvNodeRef : public tvBase {
	public:
		tvNodeRef(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog);
		// format
		U32 saver, parent, child;
		U32 time, microsec;
		Byte flags; // 0x00 not seen; 0x01 seen
	};
	
	class tvNode : public tvBase {
	public:
		/** initializes a vault node. make sure you initialize the first fields (up to modMicrosec) before sending since they
		    will also be sent when their flag is off */
		tvNode(void);
		
		virtual ~tvNode(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog);
	
		// format
		U32 flagA, flagB, flagC;
		// you can only rely on these being defined if the flag is set
		U32 index;
		Byte type;
		U32 permissions;
		S32 owner;
		U32 group;
		U32 modTime, modMicrosec;
		U32 creator;
		U32 crtTime;
		U32 ageTime;
		tUStr ageName;
		Byte ageGuid[8];
		U32 int1, int2, int3, int4;
		U32 uInt1, uInt2, uInt3, uInt4;
		tUStr str1, str2, str3, str4, str5, str6;
		tUStr lStr1, lStr2;
		tUStr text1, text2;
		U32 blob1Size;
		Byte *blob1;

	private:
		void permissionsAsHtml(tLog *log);
		void blobAsHtml(tLog *log, Byte *blob, U32 size);
	};
	
	class tvItem : public tvBase {
	public:
		tvItem(Byte id, S32 integer);
		tvItem(Byte id, Byte *str);
		tvItem(Byte tpots) : tvBase() { this->tpots = tpots; data = NULL; }
		virtual ~tvItem(void) { if (data) delete data; }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog);
		
		S32 asInt(void);
		
		Byte tpots; // 1: generate/parse for TPOTS client, everything else: for non-TPOTS client (or the vault server)
		// format
		Byte id;
		U16 type;
		tvBase *data;
	};
	
	class tvMessage : public tvBase {
	public:
		tvMessage(tvMessage &msg, int nItems);
		tvMessage(bool isTask, Byte tpots) : tvBase() { task = isTask; this->tpots = tpots; items = NULL; }
		virtual ~tvMessage(void);
		virtual void store(tBBuf &t); //!< unpacks the message
		virtual void stream(tBBuf &t);
		virtual void asHtml(tLog *log, bool shortLog);
		void print(tLog *log, bool clientToServer, tNetSession *client, bool shortLog, U32 ki = 0);
		
		Byte tpots; // 1: generate/parse for TPOTS client, everything else: for non-TPOTS client (or the vault server)
		// format
		bool task;
		Byte cmd; //!< the vault command
		Byte compressed; //!< 1 when uncompressed, 3 when compressed
		U16 numItems; //!< number of items
		tvItem **items;
		U16 context; // sub in VaultTask
		U32 vmgr; // client in VaultTask
		U16 vn; // what is that?
		
	};
	
	
} //End alc namespace

#endif
