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

#ifndef _HTML_DUMPER
#define _HTML_DUMPER
#define _HTML_DUMPER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include<stdio.h>
#include <string.h>

#include "debug.h"
#include "stdebug.h"
#include "files.h"
#include "conv_funs.h"
#include "data_types.h"
#include "vaultstrs.h"
#include "vnodes.h"
#include "prot.h"

#include "htmldumper.h"

//returns the pointer to the string associated to this node
const char * vault_get_type(Byte type) {
	switch (type) {
		case KInvalidNode: return("KInvalidNode");
		case KVNodeMgrPlayerNode: return("KVNodeMgrPlayerNode");
		case KVNodeMgrAgeNode: return("KVNodeMgrAgeNode");
		case KFolderNode: return("KFolderNode");
		case KPlayerInfoNode: return("KPlayerInfoNode");
		case KImageNode: return("KImageNode");
		case KTextNoteNode: return("KTextNoteNode");
		case KSDLNode: return("KSDLNode");
		case KAgeLinkNode: return("KAgeLinkNode");
		case KChronicleNode: return("KChronicleNode");
		case KPlayerInfoListNode: return("KPlayerInfoListNode");
		case KMarkerNode: return("KMarkerNode");
		case KAgeInfoNode: return("KAgeInfoNode");
		case KMarkerListNode: return("KMarkerListNode");
		case KSystem: return("KSystem");
		case KVNodeMgrGameServerNode: return("<font color=\"orange\">KVNodeMgrGameServerNode</font>");
		case KVNodeMgrAdminNode: return("<font color=\"orange\">KVNodeMgrAdminNode</font>");
		case KVNodeMgrServerNode: return("<font color=\"orange\">KVNodeMgrServerNode</font>");
		case KVNodeMgrCCRNode: return("<font color=\"orange\">KVNodeMgrCCRNode</font>");
		case KAgeInfoListNode: return("<font color=\"orange\">KAgeInfoListNode</font>");
		default: return("<font color=\"red\">Unknown Type of node!</font>");
	}
}

//get the permissions
void dump_permissions(FILE * f,U32 permissions) {
	Byte perm;
	fprintf(f,"<table border=1><tr><td colspan=2>Other</td><td colspan=2>Group</td><td colspan=2>Owner</td></tr>\n");
	fprintf(f,"<tr><td>w</td><td>r</td><td>w</td><td>r</td><td>w</td><td>r</td></tr>\n");
	fprintf(f,"<tr><td>");
	perm=(Byte)permissions;
	if(perm & KOtherWrite) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"</td><td>");
	if(perm & KOtherRead) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"</td><td>");
	if(perm & KGroupWrite) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"</td><td>");
	if(perm & KGroupRead) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"</td><td>");
	if(perm & KOwnerWrite) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"</td><td>");
	if(perm & KOwnerRead) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"</td></tr>");
	fprintf(f,"</table>\n");
}

//print the flags
void dump_flags(FILE * f,U32 flag1,U32 flag2,U32 flag3) {
	//Byte perm;
	fprintf(f,"<table border=1><tr><td>flag_name</td><td>value</td></tr>\n");
	fprintf(f,"<tr><td>MIndex</td><td>");
	if(flag2 & MIndex) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MType</td><td>");
	if(flag2 & MType) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MOwner</td><td>");
	if(flag2 & MOwner) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MUnk1</td><td>");
	if(flag2 & MUnk1) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MStamp1</td><td>");
	if(flag2 & MStamp1) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MId1</td><td>");
	if(flag2 & MId1) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MStamp2</td><td>");
	if(flag2 & MStamp2) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MStamp3</td><td>");
	if(flag2 & MStamp3) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MAgeCoords</td><td>");
	if(flag2 & MAgeCoords) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MAgeName</td><td>");
	if(flag2 & MAgeName) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MHexGuid</td><td>");
	if(flag2 & MHexGuid) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MTorans</td><td>");
	if(flag2 & MTorans) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MDistance</td><td>");
	if(flag2 & MDistance) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MElevation</td><td>");
	if(flag2 & MElevation) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MUnk5</td><td>");
	if(flag2 & MUnk5) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MId2</td><td>");
	if(flag2 & MId2) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MUnk7</td><td>");
	if(flag2 & MUnk7) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MUnk8</td><td>");
	if(flag2 & MUnk8) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MUnk9</td><td>");
	if(flag2 & MUnk9) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MEntryName</td><td>");
	if(flag2 & MEntryName) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MSubEntry</td><td>");
	if(flag2 & MSubEntry) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MOwnerName</td><td>");
	if(flag2 & MOwnerName) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MGuid</td><td>");
	if(flag2 & MGuid) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MStr1</td><td>");
	if(flag2 & MStr1) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MStr2</td><td>");
	if(flag2 & MStr2) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MAvie</td><td>");
	if(flag2 & MAvie) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MUid</td><td>");
	if(flag2 & MUid) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MEValue</td><td>");
	if(flag2 & MEValue) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MEntry2</td><td>");
	if(flag2 & MEntry2) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MData1</td><td>");
	if(flag2 & MData1) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MData2</td><td>");
	if(flag2 & MData2) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MBlob1</td><td>");
	if(flag3 & MBlob1) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"<tr><td>MBlob2</td><td>");
	if(flag3 & MBlob2) {
		fprintf(f,"X");
	} else {
		fprintf(f,"&nbsp;");
	}
	fprintf(f,"</td></tr>");
	fprintf(f,"</table>\n");
}

//returns the pointer to the string associated to this node
const char * vault_get_folder_type(U32 type) {
	switch (type) {
		case KGeneric: return("KGeneric");
		case KInboxFolder: return("KInboxFolder");
		case KBuddyListFolder: return("KBuddyListFolder");
		case KIgnoreListFolder: return("KIgnoreListFolder");
		case KPeopleIKnowAboutFolder: return("KPeopleIKnowAboutFolder");
		case KVaultMgrGlobalDataFolder: return("KVaultMgrGlobalDataFolder");
		case KChronicleFolder: return("KChronicleFolder");
		case KAvatarOutfitFolder: return("KAvatarOutfitFolder");
		case KAgeTypeJournalFolder: return("KAgeTypeJournalFolder");
		case KSubAgesFolder: return("KSubAgesFolder");
		case KDeviceInboxFolder: return("KDeviceInboxFolder");
		case KHoodMembersFolder: return("KHoodMembersFolder");
		case KAllPlayersFolder: return("KAllPlayersFolder");
		case KAgeMembersFolder: return("KAgeMembersFolder");
		case KAgeJournalsFolder: return("KAgeJournalsFolder");
		case KAgeDevicesFolder: return("KAgeDevicesFolder");
		case KAgeInstaceSDLNode: return("KAgeInstaceSDLNode");
		case KAgeGlobalSDLNode: return("KAgeGlobalSDLNode");
		case KCanVisitFolder: return("KCanVisitFolder");
		case KAgeOwnersFolder: return("KAgeOwnersFolder");
		case KAllAgeGlobalSDLNodesFolder: return("KAllAgeGlobalSDLNodesFolder");
		case KPlayerInfoNodeFolder: return("KPlayerInfoNodeFolder");
		case KPublicAgesFolder: return("KPublicAgesFolder");
		case KAgesIOwnFolder: return("KAgesIOwnFolder");
		case KAgesICanVisitFolder: return("KAgesICanVisitFolder");
		case KAvatarClosetFolder: return("KAvatarClosetFolder");
		case KAgeInfoNodeFolder: return("KAgeInfoNodeFolder");
		case KSystemNode: return("KSystemNode");
		case KPlayerInviteFolder: return("KPlayerInviteFolder");
		case KCCRPlayersFolder: return("KCCRPlayersFolder");
		case KGlobalInboxFolder: return("KGlobalInboxFolder");
		case KChildAgesFolder: return("KChildAgesFolder");
		default: return("<font color=\"red\">Unknown Type of folder!</font>");
	}
}

const char * vault_get_operation(Byte type) {
	switch (type) {
		case VConnect: return("VConnect");
		case VDisconnect: return("VDisconnect");
		case VAddNodeRef: return("VAddNodeRef");
		case VRemoveNodeRef: return("VRemoveNodeRef");
		case VNegotiateManifest: return("VNegotiateManifest");
		case VSaveNode: return("VSaveNode");
		case VFindNode: return("VFindNode");
		case VFetchNode: return("VFetchNodes");
		case VSendNode: return("VSendNode");
		case VSetSeen: return("VSetSeen");
		case VOnlineState: return("VOnlineState");
		default: return("<font color=\"red\">VUnknown</font>");
	}
}

const char * vault_get_task(Byte type) {
	switch (type) {
		case TCreatePlayer: return("TCreatePlayer");
		case TDeletePlayer: return("TDeletePlayer");
		case TGetPlayerList: return("TGetPlayerList");
		case TCreateNeighborhood: return("TCreateNeighborhood");
		case TJoinNeighborhood: return("TJoinNeighborhood");
		case TSetAgePublic: return("TSetAgePublic");
		case TIncPlayerOnlineTime: return("TIncPlayerOnlineTime");
		case TEnablePlayer: return("TEnablePlayer");
		case TRegisterOwnedAge: return("TRegisterOwnedAge");
		case TUnRegisterOwnedAge: return("TUnRegisterOwnedAge");
		case TRegisterVisitAge: return("TRegisterVisitAge");
		case TUnRegisterVisitAge: return("TUnRegisterVisitAge");
		case TFriendInvite: return("TFriendInvite");
		default: return("<font color=\"red\">TUnknown</font>");
	}
}

const char * vault_get_data_type(U16 dtype) {
	switch (dtype) {
		case DCreatableGenericValue: return("DCreatableGenericValue");
		case DCreatableStream: return("DCreatableStream");
		case DServerGuid: return("DServerGuid");
		case DVaultNodeRef: return("DVaultNodeRef");
		case DVaultNode: return("DVaultNode / DVaultNodeRef (tpots)");
		case DVaultNode2: return("DVaultNode (tpots)");
		case DAgeLinkStruct: return("DAgeLinkStruct");
		default: return("<font color=\"red\">DUnknown</font>");
	}
}

const char * vault_get_sub_data_type(Byte type) {
	switch (type) {
		case DInteger: return("DInteger");
		case DFloat: return("DFloat?");
		case DBool: return("DBool?");
		case DUruString: return("DUruString");
		case DPlKey: return("DPlKey?");
		case DStruct: return("DStruct?");
		case DCreatable: return("DCreatable??");
		case DTimestamp: return("DTimestamp?");
		case DTime: return("DTime?");
		case DByte: return("DByte?");
		case DShort: return("DShort?");
		case DAgeTimeOfDay: return("DAgeTimeOfDay?");
		case DVector3: return("DVector3?");
		case DPoint3: return("DPoint3?");
		case DQuaternion: return("DQuaternion?");
		case DRGB8: return("DRGB8?");
		default: return("<font color=\"red\">DUnknown</font>");
	}
}


//html dumper
int vault_parse_node_data(FILE * f, t_vault_node * n) {
	char * cstamp;
	Byte guid[9];
	fprintf(f,"<h1 id=\"%i\">Node %i</h1>\n",n->index,n->index);

	fprintf(f,"<b>Sep1:</b> %08X(%i)<br>\n",n->unkA,n->unkA);
	fprintf(f,"<b>Sep2:</b> %08X(%i)<br>\n",n->unkB,n->unkB);
	fprintf(f,"<b>Sep3:</b> %08X(%i)<br>\n",n->unkC,n->unkC);
	dump_flags(f,n->unkA,n->unkB,n->unkC);

	fprintf(f,"<b>Type:</b> %s %02X(%i)<br>\n",vault_get_type(n->type),n->type,n->type);
	fprintf(f,"<b>Permissions:</b> %02X(%i)<br>\n",n->permissions,n->permissions);
	dump_permissions(f,n->permissions);
	fprintf(f,"<b>Owner:</b> <a href=\"#%i\">%08X(%i)</a><br>\n",n->owner,n->owner,n->owner);
	if(n->unk1!=0) fprintf(f,"<b>Unk1:</b> %08X(%i)<br>\n",n->unk1,n->unk1);
	if(n->timestamp!=0) {
		cstamp=ctime((time_t *)&n->timestamp);
		fprintf(f,"<b>Timestamp:</b> %s %i<br>\n",cstamp,n->microseconds);
		//free(cstamp);
	}
	fprintf(f,"<b>Id1:</b> %08X(%i)<br>\n",n->id1,n->id1);
	if(n->timestamp2!=0) {
		cstamp=ctime((time_t *)&n->timestamp2);
		fprintf(f,"<b>Timestamp2:</b> %s %i<br>\n",cstamp,n->microseconds2);
		//free(cstamp);
	}
	if(n->timestamp3!=0) {
		cstamp=ctime((time_t *)&n->timestamp3);
		fprintf(f,"<b>Timestamp3:</b> %s %i<br>\n",cstamp,n->microseconds3);
		//free(cstamp);
	}
	if(n->age_name[0]!=0) fprintf(f,"<b>Age Name:</b> %s<br>\n",n->age_name);
	hex2ascii2(guid,n->age_guid,8);
	if(guid[0]!=0) fprintf(f,"<b>Age Hex Guid:</b> %s<br>\n",guid);
	if(n->torans!=0) fprintf(f,"<b>Torans:</b> %08X(%i) %s<br>\n",n->torans,n->torans,vault_get_folder_type(n->torans));
	if(n->distance!=0) fprintf(f,"<b>Distance:</b> %08X(%i)<br>\n",n->distance,n->distance);
	if(n->elevation!=0) fprintf(f,"<b>Elevation:</b> %08X(%i)<br>\n",n->elevation,n->elevation);
	if(n->unk5!=0) fprintf(f,"<b>Unk5:</b> %08X(%i)<br>\n",n->unk5,n->unk5);
	if(n->id2!=0) fprintf(f,"<b>Id2:</b> %08X(%i)<br>\n",n->id2,n->id2);
	if(n->unk7!=0) fprintf(f,"<b>Unk7:</b> %08X(%i)<br>\n",n->unk7,n->unk7);
	if(n->unk8!=0) fprintf(f,"<b>Unk8:</b> %08X(%i)<br>\n",n->unk8,n->unk8);
	if(n->unk9!=0) fprintf(f,"<b>Unk9:</b> %08X(%i)<br>\n",n->unk9,n->unk9);
	//str_filter(n->entry_name); //keep out < >
	Byte auxiliar23[200];
	strcpy((char *)auxiliar23,(char *)n->entry_name);
	str_filter(auxiliar23);
	if(n->entry_name[0]!=0) fprintf(f,"<b>Entry Name:</b> %s<br>\n",auxiliar23);
	if(n->sub_entry_name[0]!=0) fprintf(f,"<b>Sub Entry Name:</b> %s<br>\n",n->sub_entry_name);
	if(n->owner_name[0]!=0) fprintf(f,"<b>Owner Name:</b> %s<br>\n",n->owner_name);
	if(n->guid[0]!=0) fprintf(f,"<b>Guid:</b> %s<br>\n",n->guid);
	if(n->str1[0]!=0) fprintf(f,"<b>Str1:</b> %s<br>\n",n->str1);
	if(n->str2[0]!=0) fprintf(f,"<b>Str2:</b> %s<br>\n",n->str2);
	if(n->avie[0]!=0) fprintf(f,"<b>Avie:</b> %s<br>\n",n->avie);
	if(n->uid[0]!=0) fprintf(f,"<b>Uid:</b> %s<br>\n",n->uid);
	if(n->entry_value[0]!=0) fprintf(f,"<b>Entry value:</b> %s<br>\n",n->entry_value);
	if(n->entry2[0]!=0) fprintf(f,"<b>Entry2:</b> %s<br>\n",n->entry2);
	if(n->data_size!=0) {
		fprintf(f,"<b>Data Size:</b> %08X(%i)<br>\n",n->data_size,n->data_size);
		fprintf(f,"<b>Data contents:</b><br>\n");
		char filename1[1000];
		char filename2[1000];
		int raw_data=0;
		Byte auxiliar24[200];
		strcpy((char *)auxiliar24,(char *)n->age_name);
		str_filter(auxiliar24);
		if(strlen((char *)auxiliar24)==0) {
			strcpy((char *)auxiliar24,"Unnamed");
		}
		switch (n->type) {
			case KImageNode:
				//the image
				sprintf(filename1,"%s.%s.%i.%i.%i.jpg",auxiliar24,auxiliar23,n->index,n->timestamp,n->microseconds);
				str_filter((Byte *)filename1); //NEVER trust USER INPUT!!
				strcpy(filename2,(const char *)global_log_files_path); //path_to_images
				strcat(filename2,"/data/");
				mkdir(filename2,00750);
				strcat(filename2,filename1);
				FILE * image_file;
				image_file=fopen(filename2,"wb");
				if(image_file!=NULL) {
					fwrite(n->data+0x04,(*(U32 *)n->data)*sizeof(Byte),1,image_file);
					fprintf(f,"<img src=\"data\%s\"><br>\n",filename1);
					fclose(image_file);
				} else {
					fprintf(f,"<b>Error creating image...</b><br>\n");
					fprintf(stderr,"? %s - %s ",filename2,global_log_files_path); //path_to_images
					perror("error creating file...");
				}
				break;
			case KAgeLinkNode:
				sprintf(filename1,"%s.%s.%i.%i.%i.links",n->age_name,n->entry_name,n->index,n->timestamp,n->microseconds);
				str_filter((Byte *)filename1); //NEVER trust USER INPUT!!

				fprintf(f,"List of linking points<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				//fprintf(f,"%s",n->data);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
			case KTextNoteNode:
				sprintf(filename1,"%s.%s.%i.%i.%i.txt",n->age_name,n->entry_name,n->index,n->timestamp,n->microseconds);
				str_filter((Byte *)filename1); //NEVER trust USER INPUT!!

				fprintf(f,"Text Note<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
			case KMarkerNode:
				sprintf(filename1,"%s.%s.%i.%i.%i.marker",n->age_name,n->entry_name,n->index,n->timestamp,n->microseconds);
				str_filter((Byte *)filename1); //NEVER trust USER INPUT!!
				fprintf(f,"MARKER byte code<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
			case KSDLNode:
				sprintf(filename1,"%s.%s.%i.%i.%i.sdl_byte",n->age_name,n->entry_name,n->index,n->timestamp,n->microseconds);
				str_filter((Byte *)filename1); //NEVER trust USER INPUT!!
				fprintf(f,"SDL byte code<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
			default:
				sprintf(filename1,"%s.%s.%i.%i.%i.raw",n->age_name,n->entry_name,n->index,n->timestamp,n->microseconds);
				str_filter((Byte *)filename1); //NEVER trust USER INPUT!!
				fprintf(f,"Unknown byte code<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
		}
		if(raw_data==1) {
			strcpy(filename2,(const char *)global_log_files_path);
			strcat(filename2,"/data/");
			mkdir(filename2,00750);
			strcat(filename2,filename1);
			FILE * image_file;
			image_file=fopen(filename2,"wb");
			if(image_file!=NULL) {
				fwrite(n->data,n->data_size*sizeof(Byte),1,image_file);
				fprintf(f,"<a href=\"data\%s\">%s</a><br>\n",filename1,filename1);
				fclose(image_file);
			} else {
				fprintf(f,"<b>Error creating data file...</b><br>\n");
				fprintf(stderr,"? %s - %s ",filename2,global_log_files_path);
				perror("error creating file...");
			}
		}
	}
	if(n->data2_size!=0) fprintf(f,"<b>Data2:</b> %08X(%i) bytes !! (not implemented, if we found one, I will implement it!<br>\n",n->data2_size,n->data2_size);
	if(n->unk13!=0) fprintf(f,"<b>Unk13:</b> %08X(%i)<br>\n",n->unk13,n->unk13);
	if(n->unk14!=0) fprintf(f,"<b>Unk14:</b> %08X(%i)<br>\n",n->unk14,n->unk14);
	if(n->unk15!=0) fprintf(f,"<b>Unk15:</b> %08X(%i)<br>\n",n->unk15,n->unk15);
	if(n->unk16!=0) fprintf(f,"<b>Unk16:</b> %08X(%i)<br>\n",n->unk16,n->unk16);
	fprintf(f,"<hr>");

	return 0;
}

void htmlParseCreatableGenericValue(FILE * f,void * data) {
	t_CreatableGenericValue * wdata;
	wdata=(t_CreatableGenericValue *)data;
	fprintf(f,"Dtype: %i-%s<br>Value(s):\n",\
wdata->format,vault_get_sub_data_type(wdata->format));
	switch(wdata->format) {
		case DInteger:
			fprintf(f,"(%08X)%i",*(U32 *)(wdata->data),*(U32 *)(wdata->data));
			break;
		case DUruString:
			fprintf(f,"%s",(Byte *)(wdata->data));
			break;
		case DTimestamp:
			fprintf(f,"%f",*(double *)(wdata->data));
			break;
		default:
			fprintf(f,"<font color=red>Unknown strange sub-meta-data type!</font>\n");
			break;
	}
	fprintf(f,"<br>");
}

void htmlParseCreatableStream(FILE * f,void * data,Byte id) {
	t_CreatableStream * wdata;
	wdata=(t_CreatableStream *)data;
	fprintf(f,"Size: %i<br>Contents:<br>\n",wdata->size);
	int offset=0;
	int n_itms,i;
	U32 val;
	Byte * buf;
	buf=(Byte *)(wdata->data);
	double time;
	char * timestr; //--destroy it!
	switch(id) {
		case 0x0A:
			n_itms=*(U16 *)(buf+offset);
			offset+=2;
			fprintf(f,"NVals: %i\n<br>Value(s):",n_itms);
			for(i=0; i<n_itms; i++) {
				val=*(U32 *)(buf+offset);
				offset+=4;
				fprintf(f," (%08X)%i,",val,val);
			}
			//fprintf(f,"(%08X)%i",*(U32 *)(wdata->data),*(U32 *)(wdata->data));
			break;
		case 0x0E:
			n_itms=*(U32 *)(buf+offset);
			offset+=4;
			fprintf(f,"NVals: %i\n<br>Value(s):<br>",n_itms);
			for(i=0; i<n_itms; i++) {
				val=*(U32 *)(buf+offset);
				offset+=4;
				time=*(double *)(buf+offset);
				offset+=8;
				fprintf(f,"[%i] ID:(%08X)%i Stamp:%f<br>",i+1,val,val,time);
			}
			break;
		case 0x0F:
			n_itms=*(U32 *)(buf+offset);
			offset+=4;
			fprintf(f,"NVals: %i\n<br>Value(s):<br>",n_itms);
			for(i=0; i<n_itms; i++) {
				val=*(U32 *)(buf+offset);
				offset+=4;
				fprintf(f,"[%i] Id1:(%08X)%i ",i+1,val,val);
				val=*(U32 *)(buf+offset);
				offset+=4;
				fprintf(f,"Id2:(%08X)%i ",val,val);
				val=*(U32 *)(buf+offset);
				offset+=4;
				fprintf(f,"Id3:(%08X)%i ",val,val);
				timestr=ctime((time_t *)(buf+offset));
				offset+=4;
				val=*(U32 *)(buf+offset);
				offset+=4;
				fprintf(f,"Stamp:%s %ius ",timestr,val);
				//free((void *)timestr);
				val=*(Byte *)(buf+offset);
				offset++;
				fprintf(f,"Flag:%i<br>\n",val);
			}
			break;
		case 0x06:
			t_vault_node node;
			offset=0;
			int ret;
			fprintf(f,"<table border=1>");
			while(offset<(int)wdata->size) {
				ret=vault_parse_node(f,buf+offset,wdata->size,&node);
				if(ret<0) {
					fprintf(f,"<font color=red>An error ocurred parsing the stream!!</font>\n");
					break;
				} else {
					offset+=ret;
				}
				fprintf(f,"<tr><td bgcolor=green><font color=blue>Node: %i</font></td></tr>",node.index);
				fprintf(f,"<tr><td>\n");
				vault_parse_node_data(f,&node);
				fprintf(f,"</td></tr>\n");
			}
			fprintf(f,"</table>");
			break;
		default:
			fprintf(f,"<font color=red>Unknown strange stream-data type!</font>\n");
			break;
	}
	fprintf(f,"<br>\n");
}

void htmlVaultParse(t_vault_mos * vobj,st_uru_client * u,int recv) {

	FILE * f;
	f=f_vhtml;

	DBG(8,"step1\n");
	//recieved from client?
	if(recv==1) {
		fprintf(f,"<h1><font color=blue>From: ");
		ip2log(f,u);
		fprintf(f," To Server: [%s:%i]</font></h1>",global_bind_hostname,global_port);
	} else {
		fprintf(f,"<h1><font color=red>From Server: [%s:%i] To: ",global_bind_hostname,global_port);
		ip2log(f,u);
		fprintf(f,"</font></h1>");
	}

	DBG(8,"step2\n");
	fprintf(f,"<b>Client Id: %i[%i] (%s,%s)</b><br>\n",u->ki,u->adv_msg.ki,u->avatar_name,u->login);

	//now the vault
	if(u->adv_msg.cmd==NetMsgVault || u->adv_msg.cmd==NetMsgVault) {
		fprintf(f,"<b>NetMsgVault ");
	} else if(u->adv_msg.cmd==NetMsgVaultTask) {
		fprintf(f,"<b>NetMsgVaultTask ");
	} else {
		fprintf(f,"<b>??%04X?? ",u->adv_msg.cmd);
	}
	if(recv==1) {
		fprintf(f,"<RCV>");
	} else {
		fprintf(f,"<SND>");
	}
	fprintf(f," CMD: %i",vobj->cmd);
	if(u->adv_msg.cmd!=NetMsgVaultTask) {
		fprintf(f," %s</b><br>\n",vault_get_operation(vobj->cmd));
	} else {
		fprintf(f," %s</b><br>\n",vault_get_task(vobj->cmd));
	}
	fprintf(f,"Result: %i, Zlib: %i, RSize: %i, CSize: %i<br>\n",vobj->result,vobj->zlib,vobj->real_size,vobj->comp_size);
	fprintf(f,"NItems: %i,",vobj->n_itms);
	if(u->adv_msg.cmd!=NetMsgVaultTask) {
		fprintf(f,"Ctx:%i,Res:%i,<b>Mgr:%i</b>,Vn:%i<br>\n",vobj->ctx,\
	vobj->res,vobj->mgr,vobj->vn);
	} else {
		fprintf(f,"Sub:%i,<b>Client:%i</b><br>\n",vobj->ctx,vobj->mgr);
	}
	int i;
	fprintf(f,"<table border=1>");

	DBG(8,"step3\n");

	for(i=0; i<vobj->n_itms; i++) {
		fprintf(f,"<tr><td bgcolor=cyan><b>Item %i</b></td></tr>",i+1);
		fprintf(f,"<tr><td>");
		fprintf(f,"<font color=red>Id: <b>%i(%02X)</b></font>",vobj->itm[i].id,vobj->itm[i].id);
		fprintf(f," Unk1: %i, dtype: %04X %s<br>\n",vobj->itm[i].unk1,vobj->itm[i].dtype,vault_get_data_type(vobj->itm[i].dtype));

		//now parse the sub-data
		switch(vobj->itm[i].dtype) {
			case DCreatableGenericValue:
				DBG(7,"CreatableGenericValue\n");
				htmlParseCreatableGenericValue(f,vobj->itm[i].data);
				break;
			case DCreatableStream:
				DBG(7,"CreatableStream\n");
				htmlParseCreatableStream(f,vobj->itm[i].data,vobj->itm[i].id);
				break;
			case DServerGuid:
				DBG(7,"ServerGuid\n");
				Byte dugid[40];
				hex2ascii2(dugid,(Byte *)vobj->itm[i].data,8);
				fprintf(f,"Server Guid: %s<br>\n",dugid);
				break;
			case DVaultNode:
				DBG(7,"VaultNode\n");
				vault_parse_node_data(f,(t_vault_node *)vobj->itm[i].data);
				break;
			case DVaultNodeRef:
				DBG(7,"VaultNodeRef\n");
				char * timestr2; //destroy it!
				t_vault_cross_ref * wcross;
				DBG(8,"step1\n");
				wcross=(t_vault_cross_ref *)vobj->itm[i].data;
				DBG(8,"step2\n");
				fprintf(f,"Id1:(%08X)%i ",wcross->id1,wcross->id1);
				DBG(8,"step3\n");
				fprintf(f,"Id2:(%08X)%i ",wcross->id2,wcross->id2);
				DBG(8,"step4\n");
				fprintf(f,"Id3:(%08X)%i ",wcross->id3,wcross->id3);
				DBG(8,"step5\n");
				timestr2=ctime((const time_t *)&(wcross->timestamp)); //<-- windoze is crashing here?
				DBG(8,"step6\n");
				fprintf(f,"Stamp:%s %ius ",timestr2,wcross->microseconds);
				//fprintf(f,"Stamp:%s %ius ",wcross->timestamp,wcross->microseconds);
				DBG(8,"step7\n");
				//free((void *)timestr2);
				fprintf(f,"Flag:%i<br>\n",wcross->flag);
				DBG(8,"step8\n");
				break;
			case DAgeLinkStruct:
				DBG(7,"AgeLinkStruct\n");
				t_AgeLinkStruct * wlink;
				wlink=(t_AgeLinkStruct *)vobj->itm[i].data;
				Byte digid[40];
				hex2ascii2(digid,(Byte *)wlink->ainfo.guid,8);

				fprintf(f,"Mask: %04X<br>",wlink->mask);
				fprintf(f,"AgeInfoStruct: Mask:%02X,fname:%s,iname:%s,guid:%s,\
uname:%s,dname:%s,lan:%i<br>",\
wlink->ainfo.mask,wlink->ainfo.filename,wlink->ainfo.instance_name,\
digid,wlink->ainfo.user_name,wlink->ainfo.display_name,wlink->ainfo.language);
				fprintf(f,"UNK_BYTE: %02X<br>",wlink->unk);
				fprintf(f,"LinkingRules: %i<br>",wlink->rules);
				fprintf(f,"SpawnPoint: Mask:%08X,t:%s,n:%s,c:%s<br>\n",wlink->spoint.mask,\
wlink->spoint.title,wlink->spoint.name,wlink->spoint.camera);
				fprintf(f,"CCR:%i<br>\n",wlink->ccr);
				break;
			default:
				DBG(7,"Unk\n");
				fprintf(f,"<font color=Red>Unknonw Meta-Data type!</font><br>\n");
				break;
		}
		//end sub meta-data parser
		DBG(8,"step4\n");

		fprintf(f,"</td></tr>\n");
	}
	fprintf(f,"</table>\n");

	//end
	fprintf(f,"<i>End of transmission</i><hr>\n");
	fflush(f);
}

#if 0

//html dumper
int vault_parse_node_data2(FILE * f, t_vault_node * n) {
	char * cstamp;
	Byte guid[9];
	fprintf(f,"<h1 id=\"%i\">Node %i</h1>\n",n->index,n->index);

	fprintf(f,"<b>Sep1:</b> %08X(%i)<br>\n",n->unkA,n->unkA);
	fprintf(f,"<b>Sep2:</b> %08X(%i)<br>\n",n->unkB,n->unkB);
	fprintf(f,"<b>Sep3:</b> %08X(%i)<br>\n",n->unkC,n->unkC);

	fprintf(f,"<b>Type:</b> %s %02X(%i)<br>\n",vault_get_type(n->type),n->type,n->type);
	fprintf(f,"<b>Permissions:</b> %02X(%i)<br>\n",n->permissions,n->permissions);
	dump_permissions(f,n->permissions);
	fprintf(f,"<b>Owner:</b> <a href=\"#%i\">%08X(%i)</a><br>\n",n->owner,n->owner,n->owner);
	if(n->unk1!=0) fprintf(f,"<b>Unk1:</b> %08X(%i)<br>\n",n->unk1,n->unk1);
	if(n->timestamp!=0) {
		cstamp=ctime((time_t *)&n->timestamp);
		fprintf(f,"<b>Timestamp:</b> %s %i<br>\n",cstamp,n->microseconds);
		free(cstamp);
	}
	fprintf(f,"<b>Id1:</b> %08X(%i)<br>\n",n->id1,n->id1);
	if(n->timestamp2!=0) {
		cstamp=ctime((time_t *)&n->timestamp2);
		fprintf(f,"<b>Timestamp2:</b> %s %i<br>\n",cstamp,n->microseconds2);
		free(cstamp);
	}
	if(n->timestamp3!=0) {
		cstamp=ctime((time_t *)&n->timestamp3);
		fprintf(f,"<b>Timestamp3:</b> %s %i<br>\n",cstamp,n->microseconds3);
		free(cstamp);
	}
	if(n->age_name[0]!=0) fprintf(f,"<b>Age Name:</b> %s<br>\n",n->age_name);
	hex2ascii(n->age_guid,guid,8);
	if(guid[0]!=0) fprintf(f,"<b>Age Hex Guid:</b> %s<br>\n",guid);
	if(n->torans!=0) fprintf(f,"<b>Torans:</b> %08X(%i)<br>\n",n->torans,n->torans);
	if(n->distance!=0) fprintf(f,"<b>Distance:</b> %08X(%i)<br>\n",n->distance,n->distance);
	if(n->elevation!=0) fprintf(f,"<b>Elevation:</b> %08X(%i)<br>\n",n->elevation,n->elevation);
	if(n->unk5!=0) fprintf(f,"<b>Unk5:</b> %08X(%i)<br>\n",n->unk5,n->unk5);
	if(n->id2!=0) fprintf(f,"<b>Id2:</b> %08X(%i)<br>\n",n->id2,n->id2);
	if(n->unk7!=0) fprintf(f,"<b>Unk7:</b> %08X(%i)<br>\n",n->unk7,n->unk7);
	if(n->unk8!=0) fprintf(f,"<b>Unk8:</b> %08X(%i)<br>\n",n->unk8,n->unk8);
	if(n->unk9!=0) fprintf(f,"<b>Unk9:</b> %08X(%i)<br>\n",n->unk9,n->unk9);
	str_filter(n->entry_name); //keep out < >
	if(n->entry_name[0]!=0) fprintf(f,"<b>Entry Name:</b> %s<br>\n",n->entry_name);
	if(n->sub_entry_name[0]!=0) fprintf(f,"<b>Sub Entry Name:</b> %s<br>\n",n->sub_entry_name);
	if(n->owner_name[0]!=0) fprintf(f,"<b>Owner Name:</b> %s<br>\n",n->owner_name);
	if(n->guid[0]!=0) fprintf(f,"<b>Guid:</b> %s<br>\n",n->guid);
	if(n->unk10!=0) fprintf(f,"<b>Unk10:</b> %08X(%i)<br>\n",n->unk10,n->unk10);
	if(n->avie[0]!=0) fprintf(f,"<b>Avie:</b> %s<br>\n",n->avie);
	if(n->uid[0]!=0) fprintf(f,"<b>Uid:</b> %s<br>\n",n->uid);
	if(n->entry_value[0]!=0) fprintf(f,"<b>Entry value:</b> %s<br>\n",n->entry_value);
	if(n->unk11!=0) fprintf(f,"<b>Unk11:</b> %08X(%i)<br>\n",n->unk11,n->unk11);
	if(n->data_size!=0) {
		fprintf(f,"<b>Data Size:</b> %08X(%i)<br>\n",n->data_size,n->data_size);
		fprintf(f,"<b>Data contents:</b><br>\n");
		char filename1[1000];
		char filename2[1000];
		int raw_data=0;
		if(n->age_name[0]==0) {
			strcpy((char *)n->age_name,"Unnamed");
		}
		switch (n->type) {
			case KImageNode:
				//the image
				sprintf(filename1,"%s.%s.%i.%i.jpg",n->age_name,n->entry_name,n->index,n->microseconds);
				strcpy(filename2,path_to_images);
				strcat(filename2,"/");
				strcat(filename2,filename1);
				FILE * image_file;
				image_file=fopen(filename2,"wb");
				if(image_file!=NULL) {
					fwrite(n->data+0x04,(*(U32 *)n->data)*sizeof(Byte),1,image_file);
					fprintf(f,"<img src=\"%s\"><br>\n",filename1);
					fclose(image_file);
				} else {
					fprintf(f,"<b>Error creating image...</b><br>\n");
					fprintf(stderr,"? %s - %s ",filename2,path_to_images);
					perror("error creating file...");
				}
				break;
			case KAgeLinkNode:
				sprintf(filename1,"%s.%s.%i.%i.links",n->age_name,n->entry_name,n->index,n->microseconds);
				fprintf(f,"List of linking points<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
			case KTextNoteNode:
				sprintf(filename1,"%s.%s.%i.%i.txt",n->age_name,n->entry_name,n->index,n->microseconds);
				fprintf(f,"Text Note<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
			case KMarkerNode:
				sprintf(filename1,"%s.%s.%i.%i.marker",n->age_name,n->entry_name,n->index,n->microseconds);
				fprintf(f,"SDL byte code<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
			case KSDLNode:
				sprintf(filename1,"%s.%s.%i.%i.sdl_byte",n->age_name,n->entry_name,n->index,n->microseconds);
				fprintf(f,"SDL byte code<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
			default:
				sprintf(filename1,"%s.%s.%i.%i.raw",n->age_name,n->entry_name,n->index,n->microseconds);
				fprintf(f,"Unknown byte code<pre>\n");
				dump_packet(f,n->data,n->data_size,0,7);
				fprintf(f,"</pre><br>\n");
				raw_data=1;
				break;
		}
		if(raw_data==1) {
			strcpy(filename2,path_to_images);
			strcat(filename2,"/");
			strcat(filename2,filename1);
			FILE * image_file;
			image_file=fopen(filename2,"wb");
			if(image_file!=NULL) {
				fwrite(n->data,n->data_size*sizeof(Byte),1,image_file);
				fprintf(f,"<a href=\"%s\">%s</a><br>\n",filename1,filename1);
				fclose(image_file);
			} else {
				fprintf(f,"<b>Error creating data file...</b><br>\n");
				fprintf(stderr,"? %s - %s ",filename2,path_to_images);
				perror("error creating file...");
			}
		}
	}
	if(n->unk12!=0) fprintf(f,"<b>Unk12:</b> %08X(%i)<br>\n",n->unk12,n->unk12);
	if(n->unk13!=0) fprintf(f,"<b>Unk13:</b> %08X(%i)<br>\n",n->unk13,n->unk13);
	if(n->unk14!=0) fprintf(f,"<b>Unk14:</b> %08X(%i)<br>\n",n->unk14,n->unk14);
	if(n->unk15!=0) fprintf(f,"<b>Unk15:</b> %08X(%i)<br>\n",n->unk15,n->unk15);
	if(n->unk16!=0) fprintf(f,"<b>Unk16:</b> %08X(%i)<br>\n",n->unk16,n->unk16);
	fprintf(f,"<hr>");

	return 0;
}

#endif

#endif

