/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs H'uru Server Team                     *
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

/* Globally needed Uru constants */
/* Most of the vault constants can be found in include/alcnet/protocol/vaultproto.h */

#ifndef __U_PROT_H
#define __U_PROT_H
/* CVS tag - DON'T TOUCH*/
#define __U_PROT_H_ID "$Id$"

////============================================================================
//// UruMsgFlags
#define UNetAckReply    0x80
#define UNetNegotiation 0x40
#define UNetAckReq      0x02
//custom
#define UNetForce0      0x20
#define UNetExt         0x10 //Alcugs extension request - Validation 3 (reduced header)
#define UNetUrgent      0x04 //Urgent message
#define UNetExp         0x01 //Additional flags byte

//known flags for plNetMsg's
#define plNetFirewalled 0x00000020 //Firewalled flag only (seen only in NetMsgJoinAck)
#define plNetBcast      0x00000400 //Bcast flag only (if that flag is enabled, do a bcast)
#define plNetAck        0x00040000 //Ack flag only

//currently handled by the netcore
#define plNetVersion    0x00010000 //* contains version numbers
#define plNetTimestamp  0x00000001 //* contains a Timestamp
#define plNetX          0x00000200 //* contains the X
#define plNetKi         0x00001000 //* contains the ki
#define plNetUID        0x00004000 //* contains a player uid
#define plNetIP         0x00000010 //* contains client's ip address

//Suppositions for unidentified flags.
#define plNetUnk1       0x00000002 //Unknown, seen in NetMsgGameMessage
#define plNetUnk2       0x00002000 //Unknown, seen in NetMsgGameMessage and NetMsgSDLStateBCast
#define plNetCustom     0x00020000 //Unknown purpose flag
#define plNetStateReq   0x00000800 //Unknown, seen on the 1st plNetMsgStateRequest
#define plNetDirected   0x00008000 //Unknown, seen on plNetMsgGameMessageDirected
#define plNetP2P        0x08000000 //p2p request? (seen on plNetMsgJoinReq)

//CUSTOM plNetFlags
#define plNetSid        0x00800000 //* this message contains a sid (used when X is already used otherwise)

//An '*' means that the flag adds a new data field in the unet header.
// the rest is only a flag...
////============================================================================


////============================================================================
//// Account access levels
#define AcRoot 0
#define AcAdmin 3 // can set current player and remove avatars from all accounts
#define AcDebug 5
#define AcCCR 7 // VaultManager access, and create avatars with special types
#define AcMod 10 // can create as many avatars as he wants, can hide online state and change avatar name
#define AcPlayer 15 // default for new players
#define AcWarned 16
#define AcActivated 24 // game access (default, can be changed by configuration)
#define AcNotActivated (AcActivated+1)
#define AcBanned 30
#define AcNotRes 40 // unable to verify authentication request (user not found, DB query failed)
////============================================================================


////============================================================================
//// Type of clients
#define TExtRel 0x03
#define TIntRel 0x02
#define TDbg 0x01
////============================================================================


////============================================================================
//// Type of server destinations (for ping and other stuff)
#define KAgent 1 //unused, lobby does the job
#define KLobby 2
#define KGame 3
#define KVault 4
#define KAuth 5
#define KAdmin 6
#define KLookup 7
#define KClient 8
//custom
#define KMeta 9
#define KTracking 7 // the same as KLookup
#define KTest 10
#define KData 11
#define KProxy 12
#define KPlFire 13
#define KBcast 255
////============================================================================


////============================================================================
//// Reason Flags
// Auth
#define AAuthSucceeded 0x00
#define AAuthHello 0x01
#define AProtocolOlder 0xF7
#define AProtocolNewer 0xF8
#define AAccountExpired 0xFB
#define AAccountDisabled 0xFC
#define AInvalidPasswd 0xFD
#define AInvalidUser 0xFE
#define AUnspecifiedServerError 0xFF
//Custom
//#define AHacked 0xF6
//#define ABanned 0xF5

// Leave
#define RStopResponding 0x00
#define RInRoute 0x16
#define RArriving 0x17
#define RJoining 0x18
#define RLeaving 0x19
#define RQuitting 0x1A
//custom
#define RActive 0x14
//Reasons (Terminated)
#define RUnknown 0x01
#define RKickedOff 0x02
#define RTimedOut 0x03
#define RLoggedInElsewhere 0x04
#define RNotAuthenticated 0x05
#define RUnprotectedCCR 0x06
#define RIllegalCCRClient 0x07
//custom (Uru will show them with the Terminated dialog, and Unknown reason)
#define RHackAttempt 0x08
#define RUnimplemented 0x09
#define RParseError 0x10

/* Full error table (does not match with above table)
Generic 01
//Terminated list
LoggedInElsewhere 02
TimedOut 03
NotAuthenticated 04
KickedOff 05
Unknown 06
UnprotectedCCR 07
IllegalCCRClient 08
Unknown 09
//other
ServerSilence 10
BadVersion 11
PlayerDisabled 12 (raised when ban flag is set on player vault)
CantFindAge 13
AuthResponseFailed 14 (typical error (when the remote game server is down, or authentication time outs)
AuthTimeout 15
SDLDescProblem 16 (it's raised when the user don't reads the instructions, well, if your client SDL descriptors are outdated)
UnespecifedError 17 (hmm??)
FailedToSendMessage 18 (when you try too send a big message)
AuthTimeout2 19 (another one)
PeerTimeout 20
ServerSilence2 21
ProtocolVersionMismatch 22 (when you are mixing different game versions, with different server versions)
AuthFailed 23
FailedToCreatePlayer 24
InvalidErrorCode 25
LinkingBanned 26
LinkingRestored 27
silenced 28
unsilenced 29
*/
////============================================================================

////============================================================================
//// CreateAvatar Result codes
#define AOK 0x00
#define AUnknown 0x80
#define ANameDoesNotHaveEnoughLetters 0xF8
#define ANameIsTooShort 0xF9
#define ANameIsTooLong 0xFA
#define AInvitationNotFound 0xFB
#define ANameIsAlreadyInUse 0xFC
#define ANameIsNotAllowed 0xFD
#define AMaxNumberPerAccountReached 0xFE
#define AUnspecifiedServerError 0xFF
////============================================================================


////============================================================================
//// Linking Rules
#define KBasicLink 0
#define KOriginalBook 1
#define KSubAgeBook 2
#define KOwnedBook 3
#define KVisitBook 4
#define KChildAgeBook 5
////============================================================================


////============================================================================
//// Var types (incomplete) (used by vault and sdl)
#define DInteger 0x00 // (4 bytes) integer
#define DFloat 0x01 //(4 bytes) float
#define DBool 0x02 //(1 byte) boolean value
#define DUruString 0x03 // string
#define DPlKey 0x04 // an uruobject
#define DStruct 0x05 //a list of variables (an struct)
#define DCreatable 0x06 // a creatable stream
#define DTimestamp 0x07 // (8 bytes) timestamp as double
#define DTime 0x08 // the timestamp and the microseconds (8 bytes)
#define DByte 0x09 // a byte
#define DShort 0x0A // (2 bytes) a short integer
#define DAgeTimeOfDay 0x0B // the timestamp and the microseconds (8 bytes)
#define DVector3 0x32 // (4+4+4 bytes) three floats
#define DPoint3 0x33 // (4+4+4 bytes) three floats
#define DQuaternion 0x36 // (4+4+4+4 bytes) four floats
#define DRGB8 0x37 // (3 bytes) RBG color
#define DInvalid 0xFF
////============================================================================


////============================================================================
////plNetMsg's
#define NetMsgPagingRoom               0x0218

#define NetMsgJoinReq                  0x025A
#define NetMsgJoinAck                  0x025B
#define NetMsgLeave                    0x025C
#define NetMsgPing                     0x025D

#define NetMsgGroupOwner               0x025F

#define NetMsgGameStateRequest         0x0260

#define NetMsgGameMessage              0x0266

#define NetMsgVoice                    0x0274 //*

#define NetMsgTestAndSet               0x0278

#define NetMsgMembersListReq           0x02A8
#define NetMsgMembersList              0x02A9

#define NetMsgMemberUpdate             0x02AC

#define NetMsgCreatePlayer             0x02AE
#define NetMsgAuthenticateHello        0x02AF
#define NetMsgAuthenticateChallenge    0x02B0

#define NetMsgInitialAgeStateSent      0x02B3

#define NetMsgVaultTask                0x02BE

#define NetMsgAlive                    0x02C5
#define NetMsgTerminated               0x02C6

#define NetMsgSDLState                 0x02C8

#define NetMsgSDLStateBCast            0x0324

#define NetMsgGameMessageDirected      0x0329

#define NetMsgRequestMyVaultPlayerList 0x034E

#define NetMsgVaultPlayerList          0x0373
#define NetMsgSetMyActivePlayer        0x0374

#define NetMsgPlayerCreated            0x0377

#define NetMsgFindAge                  0x037A
#define NetMsgFindAgeReply             0x037B

#define NetMsgDeletePlayer             0x0384

#define NetMsgAuthenticateResponse     0x0393
#define NetMsgAccountAuthenticated     0x0394

#define NetMsgRelevanceRegions         0x03A7

#define NetMsgLoadClone                0x03AE
#define NetMsgPlayerPage               0x03AF

#define NetMsgVault_UU                 0x0428 // UntilUru
#define NetMsgVault                    0x0429

#define NetMsgSetTimeout_UU            0x0464 // UntilUru
#define NetMsgSetTimeout               0x0465
#define NetMsgActivePlayerSet_UU       0x0465 // UntilUru
#define NetMsgActivePlayerSet          0x0466

//tpots note
//type 0x02BC is now 0x03BC
//All types that are >0x03BC are incremented +1 in TPOTS (compared to UU)
//thx to ngilb120

//custom
#define NetMsgCustomAuthAsk            0x1001
#define NetMsgCustomAuthResponse       0x1002
#define NetMsgCustomVaultAskPlayerList 0x1003
#define NetMsgCustomVaultPlayerList    0x1004
#define NetMsgCustomVaultCreatePlayer  0x1005
#define NetMsgCustomVaultPlayerCreated 0x1006
#define NetMsgCustomVaultDeletePlayer  0x1007
#define NetMsgCustomPlayerStatus       0x1008
#define NetMsgCustomVaultCheckKi       0x1009
#define NetMsgCustomVaultKiChecked     0x100A
#define NetMsgCustomRequestAllPlStatus 0x100B //*
#define NetMsgCustomAllPlayerStatus    0x100C //*
#define NetMsgCustomSetGuid            0x100D
#define NetMsgCustomFindServer         0x100E
#define NetMsgCustomServerFound        0x100F
#define NetMsgCustomForkServer         0x1010
#define NetMsgPlayerTerminated         0x1011
#define NetMsgCustomVaultPlayerStatus  0x1012
#define NetMsgCustomMetaRegister       0x1013 //*
#define NetMsgCustomMetaPing           0x1014 //*
#define NetMsgCustomServerVault        0x1015 //*
#define NetMsgCustomServerVaultTask    0x1016 //*
#define NetMsgCustomSaveGame           0x1017 //*
#define NetMsgCustomLoadGame           0x1018 //*
#define NetMsgCustomCmd                0x1019 //*
#define NetMsgCustomDirectedFwd        0x101A
#define NetMsgCustomPlayerToCome       0x101B
#define NetMsgCustomTest               0x1313
// * unimplemented, unknown format
////============================================================================


////============================================================================
//// Plasma Object Types
// message types
#define plAnimCmdMsg               0x0206
#define plControlEventMsg          0x0210
#define plLoadCloneMsg             0x024E
#define plEnableMsg                0x024F
#define plWarpMsg                  0x0250
#define plServerReplyMsg           0x026A
#define plAvTaskMsg                0x0293
#define plLinkToAgeMsg             0x02E1
#define plNotifyMsg                0x02E8
#define plLinkEffectsTriggerMsg    0x02FB
#define plParticleTransferMsg      0x032E
#define plAvatarInputStateMsg      0x0342
#define plLinkingMgrMsg            0x0346
#define plClothingMsg              0x0352
#define plInputIfaceMgrMsg         0x035E
#define pfKIMsg                    0x035F
#define plAvBrainGenericMsg        0x038A
#define plLoadAvatarMsg            0x03AC
#define plSubWorldMsg              0x03BA

// vault types
#define plAgeLinkStruct            0x02BF
#define plCreatableGenericValue    0x0387
#define plCreatableStream          0x0389
#define plServerGuid               0x034D
#define plVaultNodeRef_UU          0x0438 // UntilUru
#define plVaultNodeRef             0x0439
#define plVaultNode_UU             0x0439 // UntilUru
#define plVaultNode                0x043A

// NULL type
#define plNull                     0x8000
////============================================================================

#endif
