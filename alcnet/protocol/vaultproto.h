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

#ifndef __U_VAULTROUTER_H
#define __U_VAULTROUTER_H

#include <urutypes/urubasetypes.h>

#include <vector>

namespace alc {
	
	class tLog;
	class tvItem;
	class tNetSession;

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

#define KVNodeMgrMAX 0x07

#define KFolderNode 0x16 // 22
#define KPlayerInfoNode 0x17 // 23
#define KSystem 0x18 // 24
#define KImageNode 0x19 // 25
#define KTextNoteNode 0x1A // 26
#define KSDLNode 0x1B // 27
#define KAgeLinkNode 0x1C // 28
#define KChronicleNode 0x1D // 29
#define KPlayerInfoListNode 0x1E // 30

#define KMarkerNode 0x20 // 32
#define KAgeInfoNode 0x21 // 33
#define KAgeInfoListNode 0x22 // 34
#define KMarkerListNode 0x23 // 35

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

	const char *alcVaultGetCmd(uint8_t cmd);
	const char *alcVaultGetTask(uint8_t cmd);
	const char *alcVaultGetNodeType(uint8_t type);
	const char *alcVaultGetFolderType(uint32_t type);
	
	class tvBase : public tStreamable {
	public:
		virtual void asHtml(tLog *log, bool shortLog) = 0; //!< print the data as HTML. Also does further verification for some types.
	};
	
	class tvManifest : public tvBase {
	public:
		tvManifest(uint32_t id, double timestamp);
		tvManifest(void) : tvBase() {}
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual void asHtml(tLog *log, bool shortLog);
		// format
		uint32_t id;
		double time;
	};
	
	class tvNodeRef : public tvBase {
	public:
		tvNodeRef(uint32_t saver, uint32_t parent, uint32_t child, uint32_t time, uint32_t microsec,  uint8_t flags);
		tvNodeRef(uint32_t saver, uint32_t parent, uint32_t child);
		tvNodeRef(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual void asHtml(tLog *log, bool shortLog);
		// format
		uint32_t saver, parent, child;
		uint32_t time, microsec;
		uint8_t flags; // 0x00 not seen; 0x01 seen
	};
	
	class tvCreatableGenericValue : public tvBase {
	public:
		tvCreatableGenericValue(uint32_t integer);
		tvCreatableGenericValue(double time);
		tvCreatableGenericValue(const tString &str);
		tvCreatableGenericValue(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual void asHtml(tLog *log, bool shortLog);
		
		uint32_t asInt(void) const;
		const tString &asString(void) const;
		
		// format
		uint8_t format;
		uint32_t integer;
		tUruString str;
		double time;
	};
	
	class tvCreatableStream : public tvBase {
	public:
		 /** create a stream with a list of vault objects. These must be tvManifest or tvNodeRef, otherwise that's not a valid stream! */
		tvCreatableStream(uint8_t id, tvBase **dataList, int nData);
		tvCreatableStream(uint8_t id, tMBuf &buf);
		tvCreatableStream(uint8_t id) : tvBase() { this->id = id; size = 0; data = NULL; }
		virtual ~tvCreatableStream(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual void asHtml(tLog *log, bool shortLog);
		tSBuf getData(void) const; //!< remember to delete the MBuf
		// format
		size_t size;
		uint8_t id;
		void *data;
		FORBID_CLASS_COPY(tvCreatableStream) // the "data" would have to be copied manually
	};
	
	class tvServerGuid : public tvBase {
	public:
		tvServerGuid(void) : tvBase() { }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual void asHtml(tLog *log, bool shortLog);
		// format
		uint8_t guid[8];
	};
	
	class tvNode : public tvBase {
	public:
		/** initializes a vault node. make sure you initialize the first fields (up to modMicrosec) before sending since they
		    will also be sent when their flag is off */
		tvNode(uint32_t flagB = 0);
		
		virtual ~tvNode(void) {}
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual void asHtml(tLog *log, bool shortLog);
	
		// format
		uint32_t flagB, flagC;
		// you can only rely on these being defined if the flag is set
		uint32_t index;
		uint8_t type;
		uint32_t permissions;
		uint32_t owner;
		uint32_t group;
		double modTime;
		uint32_t creator;
		uint32_t crtTime;
		uint32_t ageTime;
		tString ageName;
		uint8_t ageGuid[8];
		uint32_t int1, int2, int3, int4;
		uint32_t uInt1, uInt2, uInt3, uInt4;
		tString str1, str2, str3, str4, str5, str6;
		tString lStr1, lStr2;
		tString text1, text2;
		tMBuf blob1;

	private:
		void permissionsAsHtml(tLog *log);
		void blobAsHtml(tLog *log, const tMBuf &blob);
		FORBID_CLASS_COPY(tvNode) // this is a huge class and there is no reason to copy it, so prevent it from happening accidently
	};
	
	class tvItem : public tvBase {
	public:
		tvItem(uint8_t id, uint32_t integer);
		tvItem(uint8_t id, double time);
		tvItem(uint8_t id, const tString &str);
		tvItem(tvCreatableStream *stream);
		tvItem(uint8_t id, tvNodeRef *ref);
		tvItem(bool UUFormat) : tvBase(), UUFormat(UUFormat), data(NULL) {}
		virtual ~tvItem(void) { delete data; }
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		virtual void asHtml(tLog *log, bool shortLog);
		
		uint32_t asInt(void) const;
		const tString &asString(void) const;
		const uint8_t *asGuid(void) const;
		tvNode *asNode(void) const;
		tvNodeRef *asNodeRef(void) const;
		tAgeLinkStruct *asAgeLink(void) const;
		
		bool UUFormat; //!< stores whether the packet is stored and streamed in POTS or UU format
		// format
		uint8_t id;
		uint16_t type;
		tStreamable *data;
		FORBID_CLASS_COPY(tvItem) // the "data" would have to be copied manually
	};
	
	class tvMessage : public tvBase {
	public:
		tvMessage(const tvMessage &msg);
		tvMessage(uint8_t cmd, bool task = false);
		tvMessage(bool isTask, bool UUFormat) : tvBase(), UUFormat(UUFormat), task(isTask) {}
		virtual ~tvMessage(void);
		virtual void store(tBBuf &t); //!< unpacks the message
		virtual void stream(tBBuf &t) const;
		virtual void asHtml(tLog *log, bool shortLog);
		void print(tLog *log, bool clientToServer, tNetSession *client, bool shortLog, uint32_t ki = 0);
		const tvMessage &operator=(const tvMessage &msg); //!< does not copy the items, just the "outer" data
		
		typedef std::vector<tvItem *> tItemList; // to avoid re-allocating and since tvItems can't be copied, this is a vector of pointers
		
		bool UUFormat; //!< stores whether the packet is stored and streamed in POTS or UU format
		// format
		bool task;
		uint8_t cmd; //!< the vault command
		bool compress;
		tItemList items;
		uint16_t context; //!< vault context; sub in VaultTask
		uint32_t vmgr; //!< vault manager; client in VaultTask
		uint16_t vn; //!< vault number
	};
	
	
} //End alc namespace

#endif
