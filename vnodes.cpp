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

#ifndef _VAULT_NODES
#define _VAULT_NODES
#define _VAULT_NODES_ID "$Id$"

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

//delimiters (out dated)
//const U32 delimiter1=0x00000002;
//const U32 delimiter2=0xFFFFFFFF;
//const U32 delimiter3=0x00000007;


/** Initialitzes a node
*/
void init_node(t_vault_node * node) {
	bzero(node,sizeof(t_vault_node));
	node->data=NULL;
	node->data2=NULL;
}

/** Destroys the node
*/
void destroy_node(t_vault_node * node) {
	if(node->data_size!=0 && node->data!=NULL) {
		free(node->data);
	}
	if(node->data2_size!=0 && node->data2!=NULL) {
		free(node->data2);
	}
}

/** Node dumper
*/
void dump_node(Byte * buf, int max) {
	U32 nice=0xFFFFFFFF;
	char cokorota[500];
	FILE * f_exp;
	mkdir("dumps",0750);
	sprintf(cokorota,"dumps/unk_nodes.node_raw");
	f_exp=fopen(cokorota,"ab");
	if(f_exp!=NULL) {
		fwrite(buf,max * sizeof(Byte),1,f_exp);
		fwrite(&nice,sizeof(U32),1,f_exp);
		fwrite(&nice,sizeof(U32),1,f_exp);
		fwrite(&nice,sizeof(U32),1,f_exp);
		fwrite(&nice,sizeof(U32),1,f_exp);
		fwrite(&nice,sizeof(U32),1,f_exp);
		fwrite(&nice,sizeof(U32),1,f_exp);
		fwrite(&nice,sizeof(U32),1,f_exp);
		fwrite(&nice,sizeof(U32),1,f_exp);
		fclose(f_exp);
	} else {
		fprintf(stderr,"Error opening %s!\n",cokorota);
	}
}

/** Puts the node into the buffer, and returns the size of the node
*/
int node2stream(Byte * buf,t_vault_node * node) {
	int offset=0; //the offset
	//found two vaules for this var 0x00000002 and 0x00000001
	*(U32 *)(buf+offset)=node->unkA;
	offset+=0x04;
	//check the know values
	if(!(node->unkA==0x00000002 || node->unkA==0x00000001)) {
		fprintf(f_vmgr,"UNE: sep1=%08X(%i)\n",node->unkA,node->unkA);
	}
	//these are bool variables
	*(U32 *)(buf+offset)=node->unkB;
	offset+=0x04;
	//seems to be more bool variables
	if(node->unkA==0x00000002) {
		*(U32 *)(buf+offset)=node->unkC;
		offset+=0x04;
		if((node->unkC & 0xFFFFFFF8)!=0x00000000) {
			fprintf(f_vmgr,"UNE: sep3=%08X(%i)\n",node->unkC,node->unkC);
		}
	}
	//mandatory - always seen values
	*(U32 *)(buf+offset)=node->index;
	offset+=0x04;
	*(Byte *)(buf+offset)=node->type;
	offset+=0x01;
	*(U32 *)(buf+offset)=node->permissions;
	offset+=0x04;
	if((node->permissions & 0xFFFFFF00) !=0) {
		fprintf(f_vmgr,"ERR: Warning format inconsitency on node.permissions at %i\n",node->index);
	}
	*(S32 *)(buf+offset)=node->owner;
	offset+=0x04;
	*(U32 *)(buf+offset)=node->unk1;
	offset+=0x04;
	*(U32 *)(buf+offset)=node->timestamp;
	offset+=0x04;
	*(U32 *)(buf+offset)=node->microseconds;
	offset+=0x04;
	//end mandatory------------------------
	if(buf[4] & 0x40) { //01000000
		*(U32 *)(buf+offset)=node->id1;
		offset+=0x04;
	}
	if((buf[4] & 0x80)) { // && (buf[5] & 0x01)) { //10000000 - 00000001
		*(U32 *)(buf+offset)=node->timestamp2;
		offset+=0x04;
		*(U32 *)(buf+offset)=node->microseconds2;
		offset+=0x04;
	}
#if 0
	if(!(buf[4] & 0x80) && (buf[5] & 0x01)) {
		fprintf(f_vmgr,"UNE: WARNING Bytes 1 and 2 contain unexpected timestamp flags\n");
		//dump_node(buf,max_stream); //dump the node to the filesystem
		//return -1;
	}
#endif
	if(buf[5] & 0x02 || buf[5] & 0x01) { //00000010
		*(U32 *)(buf+offset)=node->timestamp3;
		offset+=0x04;
		*(U32 *)(buf+offset)=node->microseconds3;
		offset+=0x04;
	}
	if(buf[5] & 0x04) { //00000100
		offset+=encode_urustring(buf+offset,node->age_name,strlen((const char *)node->age_name),0);
	}
	if(buf[5] & 0x08) { //00001000
		memcpy(buf+offset,node->age_guid,8);
		offset+=0x08; //the age guid is always present
	}
	if(buf[5] & 0x10) { //IT's the folder type, but sometimes :?? //00010000
		*(U32 *)(buf+offset)=node->torans; //folder type? - not they are torans!!
		offset+=0x04;
	}
	if(buf[5] & 0x20) { //00100000
		*(U32 *)(buf+offset)=node->distance; //distance
		offset+=0x04;
	}
	if(buf[5] & 0x40) { //01000000
		*(U32 *)(buf+offset)=node->elevation; //elevation
		offset+=0x04;
	}
	if(buf[5] & 0x80) { //10000000
		*(U32 *)(buf+offset)=node->unk5;
		offset+=0x04;
	}
	if(buf[6] & 0x01) { //00000001
		*(U32 *)(buf+offset)=node->id2; //correct, another ki number (or node index)
		offset+=0x04;
	}
	if(buf[6] & 0x02) { //00000010
		*(U32 *)(buf+offset)=node->unk7;
		offset+=0x04;
	}
	if(buf[6] & 0x04) { //00000100
		*(U32 *)(buf+offset)=node->unk8;
		offset+=0x04;
	}
	if(buf[6] & 0x08) { //00001000
		*(U32 *)(buf+offset)=node->unk9;
		offset+=0x04;
	}
	if(buf[6] & 0x10) { //00010000
		offset+=encode_urustring(buf+offset,node->entry_name,strlen((const char *)node->entry_name),0);
	}
	if(buf[6] & 0x20) { //00100000
		offset+=encode_urustring(buf+offset,node->sub_entry_name,strlen((const char *)node->sub_entry_name),0);
	}
	if(buf[6] & 0x40) { //01000000
		offset+=encode_urustring(buf+offset,node->owner_name,strlen((const char *)node->owner_name),0);
	}
	if(buf[6] & 0x80) { //10000000
		offset+=encode_urustring(buf+offset,node->guid,strlen((const char *)node->guid),0);
	}
	if(buf[7] & 0x01) { //00000001
		offset+=encode_urustring(buf+offset,node->str1,strlen((const char *)node->str1),0);
	}
	if(buf[7] & 0x02) { //00000010
		offset+=encode_urustring(buf+offset,node->str2,strlen((const char *)node->str2),0);
	}
	if(buf[7] & 0x04) { //00000100
		offset+=encode_urustring(buf+offset,node->avie,strlen((const char *)node->avie),0);
	}
	if(buf[7] & 0x08) { //00001000
		offset+=encode_urustring(buf+offset,node->uid,strlen((const char *)node->uid),0);
	}
	if(buf[7] & 0x10) { //00010000
		offset+=encode_urustring(buf+offset,node->entry_value,\
strlen((const char *)node->entry_value),0);
	}
	if(buf[7] & 0x20) {
		offset+=encode_urustring(buf+offset,node->entry2,\
strlen((const char *)node->entry2),0);
	}
	if(buf[7] & 0x40) {
		*(U32 *)(buf+offset)=node->data_size;
		offset+=0x04;
		memcpy(buf+offset,node->data,node->data_size);
		offset+=node->data_size;
	}
	if(buf[7] & 0x80) {
		*(U32 *)(buf+offset)=node->data2_size;
		offset+=0x04;
		memcpy(buf+offset,node->data2,node->data2_size);
		offset+=node->data2_size;
	}
	//last values------------------------
	if(node->unkA==0x00000002) {
		if(buf[8] & 0x01) {
			*(U32 *)(buf+offset)=node->unk13;
			offset+=0x04;
			*(U32 *)(buf+offset)=node->unk14;
			offset+=0x04;
		}
		if((buf[8] & 0x02)) {
			*(U32 *)(buf+offset)=node->unk15;
			offset+=0x04;
			*(U32 *)(buf+offset)=node->unk16;
			offset+=0x04;
		}
	}
	return offset;
}

/** Parse node
	\param f: file descriptor where errors, an unexpected things are dumped out\n
	\param buf: the buffer where all the stuff is located\n
	\param max_stream: maxium size of the buffer, to avoid reading ahead...\n
	\param node: the addres of the node struct, where only _one_ node will be stored\n
	returns the size of the parsed node
*/
int vault_parse_node(FILE * f,Byte * buf, int max_stream, t_vault_node * node) {
	int offset=0; //the offset
	//found two vaules for this var 0x00000002 and 0x00000001
	node->unkA=*(U32 *)(buf+offset);
	offset+=0x04;
	//check the known values
	if(!(node->unkA==0x00000002 || node->unkA==0x00000001)) {
		fprintf(f,"UNE: sep1=%08X(%i)\n",node->unkA,node->unkA);
		dump_node(buf,max_stream); //dump the node to the filesystem
		return -1;
	}
	//these are bool variables
	node->unkB=*(U32 *)(buf+offset);
	offset+=0x04;
	//seems to be more bool variables
	if(node->unkA==0x00000002) {
		node->unkC=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unkC & 0xFFFFFFF8)!=0x00000000) {
			fprintf(f,"UNE: sep3=%08X(%i)\n",node->unkC,node->unkC);
			dump_node(buf,max_stream); //dump the node to the filesystem
			return -1;
		}
	} else {
		node->unkC=0;
	}
	//----------------------------------------
	//mandatory - always seen values
	node->index=*(U32 *)(buf+offset);
	offset+=0x04;
	node->type=*(Byte *)(buf+offset);
	offset+=0x01;
	node->permissions=*(U32 *)(buf+offset);
	offset+=0x04;
	if((node->permissions & 0xFFFFFF00) !=0) {
		fprintf(f,"ERR: Warning format inconsitency on node.permissions at %i\n",node->index);
	}
	node->owner=*(S32 *)(buf+offset);
	offset+=0x04;
	node->unk1=*(U32 *)(buf+offset); //always 0
	offset+=0x04;
	if((node->unk1 & 0xFFFFFFFF) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk1 at %i\n",node->unk1,node->unk1,node->index);
	}
	node->timestamp=*(U32 *)(buf+offset);
	offset+=0x04;
	node->microseconds=*(U32 *)(buf+offset);
	offset+=0x04;
	//end mandatory------------------------
	if(buf[4] & 0x40) { //01000000
		node->id1=*(U32 *)(buf+offset);
		offset+=0x04;
	} else { node->id1=0; }
	//we are not sure about the next one
	if((buf[4] & 0x80)) { // && (buf[5] & 0x01)) { //10000000 - 00000001
		node->timestamp2=*(U32 *)(buf+offset);
		offset+=0x04;
		node->microseconds2=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->timestamp2=0;
		node->microseconds2=0;
	}
#if 0
	if(!(buf[4] & 0x80) && (buf[5] & 0x01)) {
		fprintf(f,"UNE: WARNING Bytes 1 and 2 contain unexpected timestamp flags\n");
		dump_node(buf,max_stream); //dump the node to the filesystem
		return -1;
	}
#endif
	if(buf[5] & 0x02) { //00000010
		node->timestamp3=*(U32 *)(buf+offset);
		offset+=0x04;
		node->microseconds3=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->timestamp3=0;
		node->microseconds3=0;
	}
	if(buf[5] & 0x04) { //00000100
		offset+=decode_urustring(node->age_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->age_name,"");
	}
	if(buf[5] & 0x08) { //00001000
		memcpy(node->age_guid,buf+offset,8);
		node->age_guid[8]=0;
		offset+=0x08; //the age guid is always present
	} else {
		//bzero(node->age_guid,8);
		memset(node->age_guid,0,8);
	}
	if(buf[5] & 0x10) { //00010000
		node->torans=*(U32 *)(buf+offset); //folder type/torans
		offset+=0x04;
	} else {
		node->torans=0;
	}
	if(buf[5] & 0x20) { //00100000
		node->distance=*(U32 *)(buf+offset); //distance
		offset+=0x04;
	} else {
		node->distance=0;
	}
	if(buf[5] & 0x40) { //01000000
		node->elevation=*(U32 *)(buf+offset); //elevation
		offset+=0x04;
	} else {
		node->elevation=0;
	}
	if(buf[5] & 0x80) { //10000000
		node->unk5=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->unk5=0;
	}
	if(buf[6] & 0x01) { //00000001
		node->id2=*(U32 *)(buf+offset); //int (ki number/index)
		offset+=0x04;
	} else {
		node->id2=0;
	}
	if(buf[6] & 0x02) { //00000010
		node->unk7=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->unk7=0;
	}
	if(buf[6] & 0x04) { //00000100
		node->unk8=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->unk8=0;
	}
	if(buf[6] & 0x08) { //00001000
		node->unk9=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->unk9=0;
	}
	if(buf[6] & 0x10) { //00010000
		offset+=decode_urustring(node->entry_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->entry_name,"");
	}
	if(buf[6] & 0x20) { //00100000
		offset+=decode_urustring(node->sub_entry_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->sub_entry_name,"");
	}
	if(buf[6] & 0x40) { //01000000
		offset+=decode_urustring(node->owner_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->owner_name,"");
	}
	if(buf[6] & 0x80) { //10000000
		offset+=decode_urustring(node->guid,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->guid,"");
	}
	if(buf[7] & 0x01) { //00000001
		offset+=decode_urustring(node->str1,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->str1,"");
	}
	if(buf[7] & 0x02) { //00000010
		offset+=decode_urustring(node->str2,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->str2,"");
	}
	if(buf[7] & 0x04) { //00000100
		offset+=decode_urustring(node->avie,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->avie,"");
	}
	if(buf[7] & 0x08) { //00001000
		offset+=decode_urustring(node->uid,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->uid,"");
	}
	if(buf[7] & 0x10) { //00010000
		offset+=decode_urustring(node->entry_value,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->entry_value,"");
	}
	if(buf[7] & 0x20) { //00100000
		offset+=decode_urustring(node->entry2,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->entry2,"");
	}
	if(buf[7] & 0x40) { //01000000
		node->data_size=*(U32 *)(buf+offset);
		offset+=0x04;
		if(node->data_size>0) {
			node->data=(Byte *)malloc(sizeof(Byte) * node->data_size);
			memcpy(node->data,buf+offset,node->data_size);
		} else {
			node->data=NULL;
		}
		//node->data=buf+offset;
		offset+=node->data_size;
	} else {
		node->data_size=0;
		node->data=NULL;
	}
	if(buf[7] & 0x80) { //10000000
		node->data2_size=*(U32 *)(buf+offset);
		offset+=0x04;
		if(node->data2_size>0) {
			node->data2=(Byte *)malloc(sizeof(Byte) * node->data2_size);
			memcpy(node->data2,buf+offset,node->data2_size);
		} else {
			node->data2=NULL;
		}
		//node->data=buf+offset;
		offset+=node->data2_size;
	} else {
		node->data2_size=0;
		node->data2=NULL;
	}
	//last unknow values
	if(node->unkA==0x00000002) { //blob id 1 & blob id 2
		if(buf[8] & 0x01) {
			node->unk13=*(U32 *)(buf+offset);
			offset+=0x04;
			node->unk14=*(U32 *)(buf+offset);
			offset+=0x04;
		} else {
			node->unk13=0;
			node->unk14=0;
		}
		if((buf[8] & 0x02)) {
			node->unk15=*(U32 *)(buf+offset);
			offset+=0x04;
			node->unk16=*(U32 *)(buf+offset);
			offset+=0x04;
		} else {
			node->unk15=0;
			node->unk16=0;
		}
	} else {
			node->unk13=0;
			node->unk14=0;
			node->unk15=0;
			node->unk16=0;
	}
	return offset;
}

#if 0
/** Parse node
	\param f: file descriptor where errors, an unexpected things are dumped out\n
	\param buf: the buffer where all the stuff is located\n
	\param max_stream: maxium size of the buffer, to avoid reading ahead...\n
	\param node: the addres of the node struct, where only _one_ node will be stored\n
*/
int vault_parse_node(FILE * f,Byte * buf, int max_stream, t_vault_node * node) {
	int offset=0; //the offset
	//const int node_size=0x83; //the size of a blank node
	//if(max_stream<node_size) {
	//	fprintf(f,"ERR: Corruption, invalid node size %i\n",max_stream);
	//	return -1;
	//}

	//found two vaules for this var 0x00000002 and 0x00000001
	node->unkA=*(U32 *)(buf+offset);
	offset+=0x04;
	//check the know values
	if(!(node->unkA==0x00000002 || node->unkA==0x00000001)) {
		fprintf(f,"UNE: sep1=%08X(%i)\n",node->unkA,node->unkA);
		dump_node(buf,max_stream); //dump the node to the filesystem
		return -1;
	}
	//these are bool variables
	node->unkB=*(U32 *)(buf+offset);
	offset+=0x04;
	//check the know values
	//if((node->unkB & 0x3FFFFEEF)!=0x3FFFFEEF) {
	//	fprintf(f,"UNE: sep2=%08X(%i)\n",node->unkB,node->unkB);
	//	dump_node(buf,max_stream); //dump the node to the filesystem
	//	return -1;
	//}
	//seems to be more bool variables
	if(node->unkA==0x00000002) {
		node->unkC=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unkC & 0xFFFFFFF8)!=0x00000000) {
			fprintf(f,"UNE: sep3=%08X(%i)\n",node->unkC,node->unkC);
			dump_node(buf,max_stream); //dump the node to the filesystem
			return -1;
		}
	} else {
		node->unkC=0;
	}

	//mandatory - always seen values
	node->index=*(U32 *)(buf+offset);
	offset+=0x04;
	node->type=*(Byte *)(buf+offset);
	offset+=0x01;
	node->permissions=*(U32 *)(buf+offset);
	offset+=0x04;
	if((node->permissions & 0xFFFFFF00) !=0) {
		fprintf(f,"ERR: Warning format inconsitency on node.permissions at %i\n",node->index);
	}
	node->owner=*(S32 *)(buf+offset);
	offset+=0x04;
	node->unk1=*(U32 *)(buf+offset); //always 0
	offset+=0x04;
	if((node->unk1 & 0xFFFFFFFF) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk1 at %i\n",node->unk1,node->unk1,node->index);
	}
	node->timestamp=*(U32 *)(buf+offset);
	offset+=0x04;
	node->microseconds=*(U32 *)(buf+offset);
	offset+=0x04;
	//end mandatory------------------------
	if(buf[4] & 0x40) { //01000000
		node->id1=*(U32 *)(buf+offset);
		offset+=0x04;
	} else { node->id1=0; }
	//we are not sure about the next one
	if((buf[4] & 0x80)) { // && (buf[5] & 0x01)) { //10000000 - 00000001
		node->timestamp2=*(U32 *)(buf+offset);
		offset+=0x04;
		node->microseconds2=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->timestamp2=0;
		node->microseconds2=0;
	}
	if(!(buf[4] & 0x80) && (buf[5] & 0x01)) {
		fprintf(f,"UNE: WARNING Bytes 1 and 2 contain unexpected timestamp flags\n");
		dump_node(buf,max_stream); //dump the node to the filesystem
		return -1;
	}

	if(buf[5] & 0x02) { //00000010
		node->timestamp3=*(U32 *)(buf+offset);
		offset+=0x04;
		node->microseconds3=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->timestamp3=0;
		node->microseconds3=0;
	}
	if(buf[5] & 0x04) { //00000100
		offset+=decode_urustring(node->age_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->age_name,"");
	}
	if(buf[5] & 0x08) { //00001000
		memcpy(node->age_guid,buf+offset,8);
		node->age_guid[8]=0;
		offset+=0x08; //the age guid is always present
	} else {
		//bzero(node->age_guid,8);
		memset(node->age_guid,0,8);
	}
	if(buf[5] & 0x10) { //IT's the folder type, but sometimes :?? //00010000
		node->torans=*(U32 *)(buf+offset); //folder type? - not they are torans!!
		offset+=0x04;
/*	if((node->unk2 & 0xFFFFFF00) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk2 at %i\n",node->unk2,node->unk2,node->index);
	}*/
	} else {
		node->torans=0;
	}
	if(buf[5] & 0x20) { //00100000
		node->distance=*(U32 *)(buf+offset); //distance
		offset+=0x04;
/*	if((node->unk3 & 0xFFFFFF00) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk3 at %i\n",node->unk3,node->unk3,node->index);
	}*/
	} else {
		node->distance=0;
	}
	if(buf[5] & 0x40) { //01000000
		node->elevation=*(U32 *)(buf+offset); //elevation
		offset+=0x04;
/*	if((node->unk4) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk4 at %i\n",node->unk4,node->unk4,node->index);
	}*/
	} else {
		node->elevation=0;
	}
	if(buf[5] & 0x80) { //10000000
		node->unk5=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unk5) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk5 at %i\n",node->unk5,node->unk5,node->index);
		}
	} else {
		node->unk5=0;
	}
	if(buf[6] & 0x01) { //00000001
		node->id2=*(U32 *)(buf+offset); //correct, another ki number (or node index)
		offset+=0x04;
/*	if((node->id2) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.id2 at %i\n",node->unk6,node->unk6,node->index);
	}*/
	} else {
		node->id2=0;
	}
	if(buf[6] & 0x02) { //00000010
		node->unk7=*(U32 *)(buf+offset);
		offset+=0x04;
	/*if((node->unk7) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk7 at %i\n",node->unk7,node->unk7,node->index);
	}*/
	} else {
		node->unk7=0;
	}
	if(buf[6] & 0x04) { //00000100
		node->unk8=*(U32 *)(buf+offset);
		offset+=0x04;
	/*if((node->unk8) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk8 at %i\n",node->unk8,node->unk8,node->index);
	}*/
	} else {
		node->unk8=0;
	}
	if(buf[6] & 0x08) { //00001000
		node->unk9=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unk9) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk9 at %i\n",node->unk9,node->unk9,node->index);
		}
	} else {
		node->unk9=0;
	}
	if(buf[6] & 0x10) { //00010000
		offset+=decode_urustring(node->entry_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->entry_name,"");
	}
	if(buf[6] & 0x20) { //00100000
		offset+=decode_urustring(node->sub_entry_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->sub_entry_name,"");
	}
	if(buf[6] & 0x40) { //01000000
		offset+=decode_urustring(node->owner_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->owner_name,"");
	}
	if(buf[6] & 0x80) { //10000000
		offset+=decode_urustring(node->guid,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->guid,"");
	}
	if((buf[7] & (0x01 | 0x02 | 0x04 | 0x08))) {
		node->unk10=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unk10) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk10 at %i\n",node->unk10,node->unk10,node->index);
		}
		offset+=decode_urustring(node->avie,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
		offset+=decode_urustring(node->uid,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		node->unk10=0;
		strcpy((char *)node->avie,"");
		strcpy((char *)node->uid,"");
		if((buf[7] & 0x01) | (buf[7] & 0x02) | (buf[7] & 0x04) | (buf[7] & 0x08)) {
			fprintf(f,"UNE: WARNING Found the awaited chunck of data\n");
			dump_node(buf,max_stream); //dump the node to the filesystem
			return -1;
		}
	}
	if(buf[7] & 0x10) {
		offset+=decode_urustring(node->entry_value,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->entry_value,"");
	}
	if(buf[7] & 0x20) {
		node->unk11=*(U16 *)(buf+offset);
		offset+=0x02;
		if((node->unk11) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %04X(%i) on node.unk11 at %i\n",node->unk11,node->unk11,node->index);
		}
	} else {
		node->unk11=0;
	}
	if(buf[7] & 0x40) {
		node->data_size=*(U32 *)(buf+offset);
		offset+=0x04;
		node->data=buf+offset;
		offset+=node->data_size;
	} else {
		node->data_size=0;
		node->data=buf;
	}
	if(buf[7] & 0x80) {
		node->unk12=*(U32 *)(buf+offset);
		offset+=0x04;
		/*if((node->unk12) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk12 at %i\n",node->unk11,node->unk11,node->index);
		}*/
	} else {
		node->unk12=0;
	}
	//last unknow values
	if(node->unkA==0x00000002) {
		if(buf[8] & 0x01) {
			node->unk13=*(U32 *)(buf+offset);
			offset+=0x04;
			node->unk14=*(U32 *)(buf+offset);
			offset+=0x04;
		} else {
			node->unk13=0;
			node->unk14=0;
		}
		if((buf[8] & 0x02)) {
			node->unk15=*(U32 *)(buf+offset);
			offset+=0x04;
			node->unk16=*(U32 *)(buf+offset);
			offset+=0x04;
		} else {
			node->unk15=0;
			node->unk16=0;
		}
		/*if(buf[8] & 0x04) {
			node->unk16=*(U32 *)(buf+offset);
			offset+=0x04;
		} else {
			node->unk16=0;
		}*/
	} else {
			node->unk13=0;
			node->unk14=0;
			node->unk15=0;
			node->unk16=0;
	}
	return offset;
}


/** Parse node 2
	\param f: file descriptor where errors, an unexpected things are dumped out\n
	\param buf: the buffer where all the stuff is located\n
	\param max_stream: maxium size of the buffer, to avoid reading ahead...\n
	\param node: the addres of the node struct, where only _one_ node will be stored\n
*/
int vault_parse_node2(FILE * f,Byte * buf, int max_stream, t_vault_node * node) {
	int offset=0; //the offset
	node->data=NULL; //security
	//found two vaules for this var 0x00000002 and 0x00000001
	node->unkA=*(U32 *)(buf+offset);
	offset+=0x04;
	//check the know values
	if(!(node->unkA==0x00000002 || node->unkA==0x00000001)) {
		fprintf(f,"UNE: sep1=%08X(%i)\n",node->unkA,node->unkA);
		dump_node(buf,max_stream); //dump the node to the filesystem
		return -1;
	}
	//these are bool variables
	node->unkB=*(U32 *)(buf+offset);
	offset+=0x04;
	//check the know values
	//seems to be more bool variables
	if(node->unkA==0x00000002) {
		node->unkC=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unkC & 0xFFFFFFF8)!=0x00000000) {
			fprintf(f,"UNE: sep3=%08X(%i)\n",node->unkC,node->unkC);
			dump_node(buf,max_stream); //dump the node to the filesystem
			return -1;
		}
	} else {
		node->unkC=0;
	}
	//mandatory - always seen values
	node->index=*(U32 *)(buf+offset);
	offset+=0x04;
	node->type=*(Byte *)(buf+offset);
	offset+=0x01;
	node->permissions=*(U32 *)(buf+offset);
	offset+=0x04;
	if((node->permissions & 0xFFFFFF00) !=0) {
		fprintf(f,"ERR: Warning format inconsitency on node.permissions at %i\n",node->index);
	}
	node->owner=*(S32 *)(buf+offset);
	offset+=0x04;
	node->unk1=*(U32 *)(buf+offset); //always 0
	offset+=0x04;
	if((node->unk1 & 0xFFFFFFFF) !=0) {
		fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk1 at %i\n",node->unk1,node->unk1,node->index);
	}
	node->timestamp=*(U32 *)(buf+offset);
	offset+=0x04;
	node->microseconds=*(U32 *)(buf+offset);
	offset+=0x04;
	//end mandatory------------------------
	if(buf[4] & 0x40) { //01000000
		node->id1=*(U32 *)(buf+offset);
		offset+=0x04;
	} else { node->id1=0; }
	//we are not sure about the next one
	if((buf[4] & 0x80)) { // && (buf[5] & 0x01)) { //10000000 - 00000001
		node->timestamp2=*(U32 *)(buf+offset);
		offset+=0x04;
		node->microseconds2=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->timestamp2=0;
		node->microseconds2=0;
	}
	if(!(buf[4] & 0x80) && (buf[5] & 0x01)) {
		fprintf(f,"UNE: WARNING Bytes 1 and 2 contain unexpected timestamp flags\n");
		dump_node(buf,max_stream); //dump the node to the filesystem
		return -1;
	}

	if(buf[5] & 0x02) { //00000010
		node->timestamp3=*(U32 *)(buf+offset);
		offset+=0x04;
		node->microseconds3=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->timestamp3=0;
		node->microseconds3=0;
	}
	if(buf[5] & 0x04) { //00000100
		offset+=decode_urustring(node->age_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->age_name,"");
	}
	if(buf[5] & 0x08) { //00001000
		memcpy(node->age_guid,buf+offset,8);
		node->age_guid[8]=0;
		offset+=0x08; //the age guid is always present
	} else {
		//bzero(node->age_guid,8);
		memset(node->age_guid,0,8);
	}
	if(buf[5] & 0x10) { //IT's the folder type, but sometimes :?? //00010000
		node->torans=*(U32 *)(buf+offset); //folder type? - not they are torans!!
		offset+=0x04;
	} else {
		node->torans=0;
	}
	if(buf[5] & 0x20) { //00100000
		node->distance=*(U32 *)(buf+offset); //distance
		offset+=0x04;
	} else {
		node->distance=0;
	}
	if(buf[5] & 0x40) { //01000000
		node->elevation=*(U32 *)(buf+offset); //elevation
		offset+=0x04;
	} else {
		node->elevation=0;
	}
	if(buf[5] & 0x80) { //10000000
		node->unk5=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unk5) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk5 at %i\n",node->unk5,node->unk5,node->index);
		}
	} else {
		node->unk5=0;
	}
	if(buf[6] & 0x01) { //00000001
		node->id2=*(U32 *)(buf+offset); //correct, another ki number (or node index)
		offset+=0x04;
	} else {
		node->id2=0;
	}
	if(buf[6] & 0x02) { //00000010
		node->unk7=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->unk7=0;
	}
	if(buf[6] & 0x04) { //00000100
		node->unk8=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->unk8=0;
	}
	if(buf[6] & 0x08) { //00001000
		node->unk9=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unk9) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk9 at %i\n",node->unk9,node->unk9,node->index);
		}
	} else {
		node->unk9=0;
	}
	if(buf[6] & 0x10) { //00010000
		offset+=decode_urustring(node->entry_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->entry_name,"");
	}
	if(buf[6] & 0x20) { //00100000
		offset+=decode_urustring(node->sub_entry_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->sub_entry_name,"");
	}
	if(buf[6] & 0x40) { //01000000
		offset+=decode_urustring(node->owner_name,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->owner_name,"");
	}
	if(buf[6] & 0x80) { //10000000
		offset+=decode_urustring(node->guid,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->guid,"");
	}
	if((buf[7] & (0x01 | 0x02 | 0x04 | 0x08))) {
		node->unk10=*(U32 *)(buf+offset);
		offset+=0x04;
		if((node->unk10) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %08X(%i) on node.unk10 at %i\n",node->unk10,node->unk10,node->index);
		}
		offset+=decode_urustring(node->avie,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
		offset+=decode_urustring(node->uid,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		node->unk10=0;
		strcpy((char *)node->avie,"");
		strcpy((char *)node->uid,"");
		if((buf[7] & 0x01) | (buf[7] & 0x02) | (buf[7] & 0x04) | (buf[7] & 0x08)) {
			fprintf(f,"UNE: WARNING Found the awaited chunck of data\n");
			dump_node(buf,max_stream); //dump the node to the filesystem
			return -1;
		}
	}
	if(buf[7] & 0x10) {
		offset+=decode_urustring(node->entry_value,buf+offset,STR_MAX_SIZE);
		offset+=0x02;
	} else {
		strcpy((char *)node->entry_value,"");
	}
	if(buf[7] & 0x20) {
		node->unk11=*(U16 *)(buf+offset);
		offset+=0x02;
		if((node->unk11) !=0) {
			fprintf(f,"ERR: Warning format inconsitency %04X(%i) on node.unk11 at %i\n",node->unk11,node->unk11,node->index);
		}
	} else {
		node->unk11=0;
	}
	if(buf[7] & 0x40) {
		node->data_size=*(U32 *)(buf+offset);
		offset+=0x04;
		node->data=(Byte *)malloc(sizeof(Byte) * node->data_size);
		memcpy(node->data,buf+offset,node->data_size);
		//node->data=buf+offset;
		offset+=node->data_size;
	} else {
		node->data_size=0;
		node->data=NULL;
	}
	if(buf[7] & 0x80) {
		node->unk12=*(U32 *)(buf+offset);
		offset+=0x04;
	} else {
		node->unk12=0;
	}
	//last unknow values
	if(node->unkA==0x00000002) {
		if(buf[8] & 0x01) {
			node->unk13=*(U32 *)(buf+offset);
			offset+=0x04;
			node->unk14=*(U32 *)(buf+offset);
			offset+=0x04;
		} else {
			node->unk13=0;
			node->unk14=0;
		}
		if((buf[8] & 0x02)) {
			node->unk15=*(U32 *)(buf+offset);
			offset+=0x04;
			node->unk16=*(U32 *)(buf+offset);
			offset+=0x04;
		} else {
			node->unk15=0;
			node->unk16=0;
		}
	} else {
			node->unk13=0;
			node->unk14=0;
			node->unk15=0;
			node->unk16=0;
	}
	return offset;
}
#endif

#if 0


/** Parse a node
		\param f_vault: is the File descriptor where all dbg messages are dumped..\n
    \param buf: is the pointer to the first delimiter\n
		\param n: is the total number of nodes\n
		\param max_stream: is the maxium size that the buffer can have\n
*/
int vault_parse_nodes(FILE * f_vault, Byte * buf, int n, int max_stream) {

	int i; // an iterator
	//int delimiter_found=0; //delimiter flag
	int aux;

	t_vault_node node; //the node

	int offset=0; //the stream offset
	//int new_offset=0; //another one

	//int unk_data; //unk number of bytes

	//char * timestamp; //for the date functions

	/*if(verbose==1) {
		printf("Node   Type Permissi Owner    Unk1   Micros Timestamp\n");
	}*/

	/*U32 del1;
	U32 del2;
	U32 del3;*/

	if(n==0) { //do by offset
		while(offset<max_stream) {
			/*del1=*(U32 *)(buf+offset);
			offset+=4;
			del2=*(U32 *)(buf+offset);
			offset+=4;
			del3=*(U32 *)(buf+offset);
			offset+=4;
			fprintf(f_vault,"<b>del1:</b> %08X(%i)<br>\n",del1,del1);
			fprintf(f_vault,"<b>del2:</b> %08X(%i)<br>\n",del2,del2);
			fprintf(f_vault,"<b>del3:</b> %08X(%i)<br>\n",del3,del3);*/
			aux=vault_parse_node(stderr,buf+offset,max_stream-offset,&node);
			if(aux<0) { //anti-segfault
				return offset;
			}
			offset+=aux;
			vault_parse_node_data2(f_vault,&node);
		}


	} else { //do by number of nodes if n_nodes!=0

	for(i=0; i<n; i++) {

		//unk_data=0;

		/*while(delimiter_found!=1 && offset<(max_stream-12)) {
			//check for the delimiter //ummm????
			if(delimiter1==*(U32 *)(buf+offset) && delimiter2==*(U32 *)(buf+offset+0x04) && delimiter3==*(U32 *)(buf+offset+0x08)) {
				delimiter_found=1;
				offset+=12;
				new_offset+=12;
			} else {
				delimiter_found=0;
				offset++; //unk data at end of the node parsed info
				unk_data++;
			}
		}*/

			//if(delimiter_found==1) {
			//if(delimiter1==*(U32 *)(buf+offset) && delimiter2==*(U32 *)(buf+offset+0x04) && delimiter3==*(U32 *)(buf+offset+0x08)) {
			//	offset+=12;
				//printf("OK - delimiter found!\n");
				//new way
				aux=vault_parse_node(stderr,buf+offset,max_stream-offset,&node);
				if(aux<0) { //anti-segfault
					return offset;
				}
				offset+=aux;
				vault_parse_node_data2(f_vault,&node);
				//delimiter_found=0; //reset the stuff
			//} else {
			//	fprintf(stderr,"FATAL: Unknow node format at offset %08X\n",offset);
			//}

	}

	/*if(max_stream!=offset) {
		fprintf(stderr,"WARNING, there is still %i bytes left in the buffer!!\n",max_stream-offset);
		//dump_packet(stderr,buf+offset,max_stream-offset,0,7);
		//fprintf(stderr,"\n");
	}*/
	}

	return offset;

}

/**reference (cross-reference) tree index
	\param f the file to dump out
	\param buf the buffer
	\param max_size the maxium size to avoid SEGFAULTS
	\return the number of parsed bytes
*/
int vault_parse_server_reference_index(FILE * f,Byte * buf,int max_size) {

	U16 n_objs; //number of objects
	U32 id; //the ki, or node?
	U16 n_id; //number of ids in the vector
	U32 size; //the size of the sav files

	//Byte age_guid[9]; //the age guid
	Byte age_guid[17];

	int offset=0; //The Offset
	int i,e; //the iterators

	n_objs=*(U16 *)(buf+offset);
	offset+=2;

	fprintf(f,"<b>Number of Indexed nodes:</b> %08X(%i)<br>\n",n_objs,n_objs);

	//sanity check
	if(n_objs>1000) {
		fprintf(stderr,"Wait a moment, are you saying more than 1000 objects?!, then is sure that something is going wrong!!\n");
		return 0;
	}

	for(i=0; i<n_objs; i++) {
		id=*(U32 *)(buf+offset); //the index
		offset+=4;
		fprintf(f,"<h2><b>Id:</b><a href=\"#%i\">%08X(%i)</a></h2>\n",id,id,id);

		n_id=*(U16 *)(buf+offset); //the number of associated elements to that index
		offset+=2;

		for(e=0; e<n_id; e++) {
			id=*(U32 *)(buf+offset); //the index
			offset+=4;
			fprintf(f,"<b>Id:</b><a href=\"#%i\">%08X(%i)</a><br>\n",id,id,id);
		}
	}

	n_objs=*(U16 *)(buf+offset); //number of packet ages
	offset+=2;

	if(n_objs>0) {
		fprintf(f,"<h1>Age State Description language files</h1>\n");
		fprintf(f,"<b>Number of files:</b> %i<br>\n",n_objs);
	}

	for(i=0; i<n_objs; i++) {
		hex2ascii(buf+offset,age_guid,8);
		offset+=8; //azrghh!!
		//check format
		if(*(U32 *)(buf+offset)!=1 || *(U16 *)(buf+offset+4)!=1 || *(U16 *)(buf+offset+6)!=1 || *(Byte *)(buf+offset+8)!=0) {
			fprintf(stderr,"WARNING INVALID SAV FORMAT! at offset %08X\n",offset);
			dump_packet(stderr,buf+offset,9,0,7);
			fprintf(stderr,"\na: %i b: %i c: %i d: %i\n",*(U32 *)(buf+offset),*(U16 *)(buf+offset+4),*(U16 *)(buf+offset+6),*(Byte *)(buf+offset+8));
			return 0;
		}
		offset+=9+4; //plus uncompressed size
		size=*(U32 *)(buf+offset)+4+4+8+1; //plus the file header
		//go back
		offset-=9+4;
		//open a file, and dump the contents into the file
		FILE * savfile;
		char filename[500];
		sprintf(filename,"%s/%s.sav",path_to_images,age_guid);
		savfile=fopen(filename,"wb");
		if(savfile!=NULL) {
			fwrite(buf+offset,size*sizeof(Byte),1,savfile);
			fclose(savfile);
		} else {
			fprintf(stderr,"What's up with the file!, %s?\n",filename);
		}
		offset+=size;
		fprintf(f,"<a href=\"%s.sav\">%s.sav</a><br>\n",age_guid,age_guid);
	}

	id=*(U32 *)(buf+offset); //the index
	offset+=4;
	fprintf(f,"<h2>Tail</h2><b>Id:</b>%08X(%i)\n",id,id);
	if(*(U32 *)(buf+offset)!=0 || (*(U16 *)(buf+offset+4)!=0)) {
		fprintf(f,"FATAL, tail format unknown! at offset %i\n",offset);
	}
	offset+=6;
	return offset;
}


/**reference (cross-reference) tree index
	\param f the file to dump out
	\param buf the buffer
	\param max_size the maxium size to avoid SEGFAULTS
	\return the number of parsed bytes
*/
int vault_parse_offline_reference_index(FILE * f,Byte * buf,int max_size) {

	U16 n_objs; //number of objects
	U32 id1; //the ki, or node?
	U32 id2;
	U32 id3;
	U32 timestamp;
	U32 microseconds;
	char * ctimestamp;
	Byte flag;

	int offset=0; //The Offset
	int i; //the iterators

	n_objs=*(U32 *)(buf+offset);
	offset+=4;
	//id1=*(U32 *)(buf+offset);
	//offset+=4;

	fprintf(f,"<b>Number of Indexed nodes:</b> %08X(%i)<br>\n",n_objs,n_objs);
	//fprintf(f,"<b>Index Id:<a href=\"#%i\">%08X(%i)</a><br>\n",id1,id1,id1);

	//sanity check
	if(n_objs>1000) {
		fprintf(stderr,"Wait a moment, are you saying more than 1000 objects?!, then is sure that something is going wrong!!\n");
		return 0;
	}

	for(i=0; i<n_objs; i++) {
		id1=*(U32 *)(buf+offset); //the index
		offset+=4;
		id2=*(U32 *)(buf+offset); //the index
		offset+=4;
		id3=*(U32 *)(buf+offset); //the index
		offset+=4;
		timestamp=*(U32 *)(buf+offset); //the index
		offset+=4;
		microseconds=*(U32 *)(buf+offset); //the index
		offset+=4;
		ctimestamp=ctime((time_t *)&timestamp);
		flag=*(Byte *)(buf+offset); //a flag
		offset++;

		fprintf(f,"<b>Item:</b> %i <br>\n",i);
		fprintf(f,"<b>Id1:</b><a href=\"#%i\">%08X(%i)</a><br>\n",id1,id1,id1);
		fprintf(f,"<b>Id2:</b><a href=\"#%i\">%08X(%i)</a><br>\n",id2,id2,id2);
		fprintf(f,"<b>Id3:</b><a href=\"#%i\">%08X(%i)</a><br>\n",id3,id3,id3);
		fprintf(f,"<b>Timestamp:</b> %s %i<br>\n",ctimestamp,microseconds);
		fprintf(f,"<b>Flag:</b> %02X(%i)<br><br>\n",flag,flag);
		free(ctimestamp);
	}

	return offset;
}

#endif


#endif

