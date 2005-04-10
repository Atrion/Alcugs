/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004  The Alcugs H'uru Server Team                          *
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

#ifndef _VAULT_STRUCTS
#define _VAULT_STRUCTS
#define _VAULT_STRUCTS_ID "$Id$"

//masks!!
#define MIndex     0x00000001 //00000001 (unkB 1) [Index] *
#define MType      0x00000002 //00000010 (unkB 1) [Type] *
#define MPerms     0x00000004 //00000100 (unkB 1) [Permissions] *
#define MOwner     0x00000008 //00001000 (unkB 1) [Owner ID] *
#define MUnk1      0x00000010 //00010000 (unkB 1) [Group ID] *
#define MStamp1    0x00000020 //00100000 (unkB 1) [Modify Time] *
#define MId1       0x00000040 //01000000 (unkB 1) [Creator ID]
#define MStamp2    0x00000080 //10000000 (unkB 1) [Create Time]
#define MStamp3    0x00000100 //00000001 (unkB 2) [Create Age Time]
#define MAgeCoords 0x00000200 //00000010 (unkB 2) [Create Age Coords]
#define MAgeName   0x00000400 //00000100 (unkB 2) [Create Age Name]
#define MHexGuid   0x00000800 //00001000 (unkB 2) [Create Age Guid]
#define MTorans    0x00001000 //00010000 (unkB 2) [Int32_1]
#define MDistance  0x00002000 //00100000 (unkB 2) [Int32_2]
#define MElevation 0x00004000 //01000000 (unkB 2) [Int32_3]
#define MUnk5      0x00008000 //10000000 (unkB 2) [Int32_4]
#define MId2       0x00010000 //00000001 (unkB 3) [UInt32_1]
#define MUnk7      0x00020000 //00000010 (unkB 3) [UInt32_2]
#define MUnk8      0x00040000 //00000100 (unkB 3) [UInt32_3]
#define MUnk9      0x00080000 //00001000 (unkB 3) [UInt32_4]
#define MEntryName 0x00100000 //00010000 (unkB 3) [String64_1]
#define MSubEntry  0x00200000 //00100000 (unkB 3) [String64_2]
#define MOwnerName 0x00400000 //01000000 (unkB 3) [String64_3]
#define MGuid      0x00800000 //10000000 (unkB 3) [String64_4]
#define MStr1      0x01000000 //00000001 (unkB 4) [String64_5]
#define MStr2      0x02000000 //00000010 (unkB 4) [String64_6]
#define MAvie      0x04000000 //00000100 (unkB 4) [lString64_1]
#define MUid       0x08000000 //00001000 (unkB 4) [lString64_2]
#define MEValue    0x10000000 //00010000 (unkB 4) [Text_1]
#define MEntry2    0x20000000 //00100000 (unkB 4) [Text_2]
#define MData1     0x40000000 //01000000 (unkB 4) [Blob1]
#define MData2     0x80000000 //10000000 (unkB 4) [Blob2]
#define MBlob1     0x00000001 //00000001 (unkC 1) [Blob1_guid]
#define MBlob2     0x00000002 //00000010 (unkC 1) [Blob2_guid]

//define the base vault index
#define KVaultID 20001

//seen node types
#define KInvalidNode 0x00

#define KVNodeMgrPlayerNode 0x02 //2
#define KVNodeMgrAgeNode 0x03
#define KVNodeMgrGameServerNode 0x04
#define KVNodeMgrAdminNode 0x05
#define KVNodeMgrServerNode 0x06
#define KVNodeMgrCCRNode 0x07

#define KFolderNode 0x16 //22
#define KPlayerInfoNode 0x17 //23
#define KSystem 0x18 //24
#define KImageNode 0x19 //25
#define KTextNoteNode 0x1A //26
#define KSDLNode 0x1B //27
#define KAgeLinkNode 0x1C //28
#define KChronicleNode 0x1D //29
#define KPlayerInfoListNode 0x1E //30

#define KMarkerNode 0x20 //32
#define KAgeInfoNode 0x21 //33
#define KAgeInfoListNode 0x22
#define KMarkerListNode 0x23 //35

//permissions
#define KOwnerRead 0x01
#define KOwnerWrite 0x02
#define KGroupRead 0x04
#define KGroupWrite 0x08
#define KOtherRead 0x10
#define KOtherWrite 0x20

//0x01 + 0x02 + 0x04 + 0x10
#define KDefaultPermissions 0x17
/* where persmissions are
-------------------------
| Other | Group | Owner |
-------------------------
| w | r | w | r | w | r |
-------------------------
*/

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
//end folder types

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

//vault ID's
/*
#define VIDUnk1 0x00 //A Integer, Always seen 0xC0AB3041 (integer)
#define VIDNodeType 0x01 //A Integer with the Node Type (integer)
#define VIDUniqueId 0x02 //A vault node Id, VMGR id (integer)
#define VIDIntList 0x0A //A list of integers (creatablestream)
#define VID
#define VIDFolder 0x17 //The Vault folder name (strign)
*/

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
//tpots
#define DVaultNodeRef2         0x0439
#define DVaultNode             0x0439
//tpots
#define DVaultNode2            0x043A

//sub data types
#define DInteger 0x00 //(4 bytes) integer
#define DFloat 0x01 //(4 bytes) float
#define DBool 0x02 //(1 byte) byte (boolean value)
#define DUruString 0x03 //an (32 bytes STRING)
#define DPlKey 0x04 //uruobject
#define DStruct 0x05 //a list of variables (an struct)
#define DCreatable 0x06 //arghhah!! **
#define DTimestamp 0x07 //a Timestamp (I think that is in double format) **
#define DTime 0x08 // the timestamp and the microseconds (8 bytes)
#define DByte 0x09 //a byte
#define DShort 0x0A //a short integer (2 bytes)
#define DAgeTimeOfDay 0x0B //I bet that is also timestamp+microseconds (8 bytes) **
#define DVector3 0x32 //Three floats (4+4+4 bytes)
#define DPoint3 0x33 //Three floats (4+4+4 bytes)
#define DQuaternion 0x36 //Four floats (4+4+4+4 bytes)
#define DRGB8 0x37 //3 bytes (RBG color)


//---internal for now (later we will use dynamic memory)
//#ifndef STR_MAX_SIZE
//#endif
//useful structs

//!the vault struct (for vault.dat files)
typedef struct {
	Byte version; //always 0x01
	U32 magic1; //always 1
	U32 magic2; //always 1
	Byte unkbyte; //always 0x03
	U32 magic3; //always 1
	U32 magic4; //always 0x07
	U32 last_node; //the last stored node id
	U32 n_nodes; //the number of stored nodes
} t_vault_dat_header;

//!manifest
typedef struct {
	U32 index;
	double timestamp;
} t_vault_manifest;
//when client request manifests, it's encapsulated into a plCreatableStream 0x0389

//!cross reference
typedef struct {
	U32 id1; //the (saver)
	U32 id2; //the "node" (parent)
	U32 id3; //the soon (child)
	U32 timestamp;
	U32 microseconds;
	Byte flag; //0x00 not seen, 0x01 seen
} t_vault_cross_ref;
//plNodeRef 0x0438 (in tpots 0x0439)

//!a node
typedef struct {
	U32 unkA; //maskA
	U32 unkB; //maskB
	//------------------------------------------------
	U32 unkC; //optional maskC
	//------------------------------------------------
	U32 index; //node index (4 bytes) [id]
	Byte type; //type of the node (1 byte) [Type]
	U32 permissions; //permission flags (4 bytes) [Permissions]
	S32 owner; //node owner (4 bytes) [Owner ID]
	U32 unk1; //unkown data, seen always 0 (4 bytes) [Group ID]
	U32 timestamp; //node timestamp (4 bytes) [Modify Time]
	U32 microseconds; //node micros (4 bytes) [Modify Time]
	//------------------------------------------------
	U32 id1; //owner (4 bytes) [Creator ID]
	U32 timestamp2; //creation timestamp (4 bytes) [Create Time] [Auto Time]
	U32 microseconds2; //micrs(again?)(4bytes)[Create Time][Auto Time]
	U32 timestamp3; //create2 (4 bytes) [Create Age Time]
	U32 microseconds3; //create2 (4 bytes) [Create Age Time/Coords?]
	Byte age_name[STR_MAX_SIZE+1]; //name of the age Ustr16 [Create Age Name]
	Byte age_guid[9+1]; //the guid of the age in hex, 8 bytes [Create Age Guid]
	//-----------------------------------------------------
	S32 torans; //Int32_1 (folder type, online status, and more...)
	S32 distance; //Int32_2 (is the player banned?)
	S32 elevation; //Int32_3
	S32 unk5; //Int32_4 //(online time)
	U32 id2; //UInt32_1
	U32 unk7; //UInt32_2
	U32 unk8; //UInt32_3
	U32 unk9; //UInt32_4
	//----------------------------------------------------
	Byte entry_name[STR_MAX_SIZE+1]; //avatar gender Ustr16 [String64_1]
	Byte sub_entry_name[STR_MAX_SIZE+1]; //age name Ustr16 [String64_2]
	Byte owner_name[STR_MAX_SIZE+1]; //Ustr16 [String64_3]
	Byte guid[STR_MAX_SIZE+1]; //age guid in ascii Ustr16 [String64_4]
	Byte str1[STR_MAX_SIZE+1]; //[String64_5]
	Byte str2[STR_MAX_SIZE+1];
	Byte avie[STR_MAX_SIZE+1]; //avatar name. uStr16 [String64_6]
	Byte uid[STR_MAX_SIZE+1]; //the uid. UStr16 [lString64_1]
	//---------------------------------------------------
	Byte entry_value[STR_MAX_SIZE+1]; ////fullname,chron val UStr16 [Text_1]
	Byte entry2[STR_MAX_SIZE+1]; // [Text_2]
	//---------------------------------------------------
	U32 data_size; //the size of the sdl byte code, age link info, etc...
	//onfly if data_size>0
	Byte * data; //pointer to the data
	U32 data2_size; //the size of the 2nd blob
	Byte * data2; //pointer to the data
	//---------------------------------------------------
	U32 unk13; // <--- blob1 A
	U32 unk14; // <--- blob1 B [BlobGuid_1]
	/* unk15 & unk16 removed from vault version 2 and above */
	U32 unk15; // <--- blob2 A
	U32 unk16; // <--- blob2 B [BlobGuid_2]
	//eof -- 0x6A (total blank node size in bytes)
} t_vault_node;
//plVaultNode 0x0439

typedef struct {
	Byte format; //data type
	void * data; //the data (a integer, float, etc...)
} t_CreatableGenericValue;
//0x0387

typedef struct {
	U32 size; //the size of the contained data stream
	void * data; //the stream of data
} t_CreatableStream;
//0x0389

//!a vault object
typedef struct {
	Byte id; //a identifier of action (1 byte) <- vault command, or mask ->
	Byte unk1; //a blank space (1 byte)
	U16 dtype; //the vault data type (2 bytes)
	//one or other
	//Byte format; //the data type format (1 byte)
	//U32 data_size; //the data size (4 bytes)
	void * data; //the raw data pointer
	//<->
	//U32 num_items; //the number of sub-items (4 bytes or 2 bytes)
	//abstract - data contents
	/*
	U32 integer;
	Byte ustring[200]; //an uru string
	Byte guid[0x20]; //be sure that is big
	U32 * table; //a table of integers (the pointer to the table) 0x0A
	t_vault_manifest * manifest; //the list of nodes (the manifest)
	t_vault_cross_ref * cross_ref; //the list of cross references
	t_vault_node * node; //a list of nodes
	//U32 raw_data_size; //raw data size*/
} t_vault_object;


//!vault main object struct
typedef struct {
	//1st object
	Byte cmd; //command (1 byte)
	U16 result; //always 0 (2 bytes)
	Byte zlib; //msg compression (1 byte) 0x01 (uncompressed) 0x03 (compressed)
	//---><--
	U32 real_size; //(uncompressed if 0x03) size of the data (4 bytes)
	//---<>--
	U32 comp_size; //IF 0x03, compressed size of the data
	//Byte * data; //pointer to the offset where the data starts
	//the data contents
	U16 n_itms; //number of packed items (2 bytes)
	t_vault_object * itm; //pointer to the first vault item
	//---end -- 2nd object
	U16 ctx; //context (2 byte)    // In Vtask is Sub (1 byte)
	U16 res; //always 0 (2 bytes)  // Not in Vtask
	U32 mgr; //vmgr id (4 bytes)   // In Vtask is Client (4 bytes)
	U16 vn; //(old tail) (2 bytes) // Not in Vtask
} t_vault_mos;

#endif
