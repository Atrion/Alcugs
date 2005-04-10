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

#ifndef __VAULT_OBJ_S
#define __VAULT_OBJ_S

#define __VAULT_OBJ_S_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <zlib.h>

#include "stdebug.h"
#include "files.h"
#include "conv_funs.h"
#include "data_types.h"
#include "urustructs.h"
#include "vaultstrs.h"
#include "vnodes.h"

#include "vaultsubsys.h"

#include "urunet.h"
#include "protocol.h"

#include "vault_obj.h"

#include "debug.h"

int plVItmGetInteger(t_vault_object * o) {
	if(o->dtype!=DCreatableGenericValue || \
((t_CreatableGenericValue *)(o->data))->format!=DInteger) {
		return 0;
	} else {
		return *(S32 *)(((t_CreatableGenericValue *)(o->data))->data);
	}
}

void plVItmPutInteger(t_vault_object * o,int integer) {
	o->dtype=DCreatableGenericValue;
	//oups !!
	o->data=malloc(sizeof(t_CreatableGenericValue) * 1);
	((t_CreatableGenericValue *)(o->data))->format=DInteger;
	(((t_CreatableGenericValue *)(o->data))->data)=malloc(sizeof(int) * 1);
	*(S32 *)(((t_CreatableGenericValue *)(o->data))->data)=integer;
}

double plVItmGetTimestamp(t_vault_object * o) {
	if(o->dtype!=DCreatableGenericValue || \
((t_CreatableGenericValue *)(o->data))->format!=DTimestamp) {
		return 0;
	} else {
		return *(double *)(((t_CreatableGenericValue *)(o->data))->data);
	}
}

void plVItmPutTimestamp(t_vault_object * o,double stamp) {
	o->dtype=DCreatableGenericValue;
	//oups !!
	o->data=malloc(sizeof(t_CreatableGenericValue) * 1);
	((t_CreatableGenericValue *)(o->data))->format=DTimestamp;
	(((t_CreatableGenericValue *)(o->data))->data)=malloc(sizeof(double) * 1);
	*(double *)(((t_CreatableGenericValue *)(o->data))->data)=stamp;
}

Byte * plVItmGetString(t_vault_object * o) {
	if(o->dtype!=DCreatableGenericValue || \
((t_CreatableGenericValue *)(o->data))->format!=DUruString) {
		return (Byte *)"";
	} else {
		return (Byte *)(((t_CreatableGenericValue *)(o->data))->data);
	}
}

void plVItmPutString(t_vault_object * o,Byte * str) {
	o->dtype=DCreatableGenericValue;
	o->data=malloc(sizeof(t_CreatableGenericValue) * 1);
	((t_CreatableGenericValue *)(o->data))->format=DUruString;
	(((t_CreatableGenericValue *)(o->data))->data)=malloc(sizeof(Byte) * (strlen((char *)str))+1);
	strcpy((char *)(((t_CreatableGenericValue *)(o->data))->data),(char *)str);
}


//init the main vault object
void plMainVaultInit(t_vault_mos * mobj) {
	memset(mobj,0,sizeof(t_vault_mos));
	mobj->itm=NULL;
}

//destroy the main vault object
void plMainVaultDestroy(t_vault_mos * mobj,int tpots_flag) {
	int i;
	DBG(6,"Attempting to Destroy vault allocated data, %i items...\n",mobj->n_itms);
	if(mobj->itm!=NULL) {
		for(i=0; i<mobj->n_itms; i++) {
			DBG(6,"Destroying Item %i...\n",i)
			plVaultItmDestroy(&mobj->itm[i],tpots_flag);
			DBG(6,"destroyed.\n");
		}
		free((void *)mobj->itm);
		DBG(6,"destroyed.\n");
	}
	plMainVaultInit(mobj);
	DBG(6,"Vault Allocated data Succesfully deallocated\n");
}

void plVaultItmInit(t_vault_object * itm) {
	memset(itm,0,sizeof(t_vault_object));
	itm->data=NULL;
}

void plVaultItmDestroy(t_vault_object * itm,int tpots_flag) {
	int tpots_mod;
	if(tpots_flag==1) {
		tpots_mod=1;
	} else {
		tpots_mod=0;
	}
	if(itm->data!=NULL) {
		//switch() <--> destroy sub objects
		DBG(6,"Attempting to destroy object type %04X\n",itm->dtype);
		switch(itm->dtype) {
			case DCreatableGenericValue:
				DBG(6,"Attemting to destroy a DCreatableGenericValue..\n");
				destroyCreatableGenericValue(&itm->data);
				DBG(6,"destroyed.\n");
				break;
			case DCreatableStream:
				DBG(6,"Attemting to destroy a DCreatableStream..\n");
				destroyCreatableStream(&itm->data);
				DBG(6,"destroyed.\n");
				break;
			case DServerGuid:
				DBG(6,"Attemting to destroy a DServerGuid..\n");
				destroyServerGuid(&itm->data);
				DBG(6,"destroyed.\n");
				break;
			case DAgeLinkStruct:
				DBG(6,"Attemting to destroy a AgeLinkStruct..\n");
				destroyAgeLinkStruct(&itm->data);
				DBG(6,"destroyed.\n");
				break;
			default:
				if(itm->dtype==DVaultNodeRef+tpots_mod) {
					DBG(6,"Attemting to destroy a DVaultNodeRef+%i..\n",tpots_mod);
					destroyVaultNodeRef(&itm->data);
					DBG(6,"destroyed.\n");
				} else if(itm->dtype==DVaultNode+tpots_mod) {
					DBG(6,"Attemting to destroy a DVaultNode+%i..\n",tpots_mod);
					destroy_node((t_vault_node *)itm->data);
					free((void *)itm->data);
					DBG(6,"destroyed.\n");
				} else {
					DBG(6,"Attemting to destroy a %04X..\n",itm->dtype);
					free((void *)itm->data);
					DBG(6,"destroyed.\n");
					plog(f_vmgr,"ERR: Attemting to destroy a unknwon type? %s\n",_WHERE("VaultItmDestroy"));
				}
		}
		//free((void *)itm->data);
		itm->data=NULL;
	}
	plVaultItmInit(itm);
}

//!Creates objects data structs (remember to destroy them!)
t_vault_object * vaultCreateItems(int n) {
	t_vault_object * o;
	int i;
	o=(t_vault_object *)malloc(sizeof(t_vault_object) * n);
	for(i=0; i<n; i++) {
		plVaultItmInit(&o[i]);
	}
	return o;
}


///Creatable Generic Value

//creates this data type
int storeCreatableGenericValue(void ** data,Byte * buf,int size) {
	int off=0;
	int ret=0;
	t_CreatableGenericValue * wdata; //working data
	*data=malloc(sizeof(t_CreatableGenericValue) * 1);
	if(*data==NULL) return -1;
	wdata=(t_CreatableGenericValue *)(*data);
	wdata->format=*(Byte *)(buf+off);
	wdata->data=NULL;
	off++;

	switch(wdata->format) {
		case 0x00: //integer (4 bytes)
			//check size
			if((size-off)<4) { ret=-1; break; }
			wdata->data=malloc(sizeof(S32) * 1);
			if(wdata->data==NULL) { ret=-1; break; }
			*(S32 *)(wdata->data)=*(S32 *)(buf+off);
			off+=4;
			break;
		case 0x03: //ustr (x bytes)
			wdata->data=malloc(sizeof(Byte) * 202);
			if(wdata->data==NULL) { ret=-1; break; }
			off+=decode_urustring((Byte *)wdata->data,buf+off,200);
			off+=2;
			break;
		case 0x07: //timestamp (8 bytes)
			wdata->data=malloc(sizeof(Byte) * 8);
			if(wdata->data==NULL) { ret=-1; break; }
			*(double *)(wdata->data)=*(double *)(buf+off);
			off+=8;
			break;
		default: //unimplemented
			print2log(f_vmgr,"Fix Me, unimplemented plCreatableGenericValue %02X\n",wdata->format);
			ret=-1;
			break;
	}
	if(ret<0) {
		if(*data!=NULL) {
			free(*data);
		}
		return ret;
	}
	return off;
}

//creates this data type
int streamCreatableGenericValue(Byte * buf,void * data) {
	int off=0;
	t_CreatableGenericValue * wdata; //working data
	wdata=(t_CreatableGenericValue *)(data);
	*(Byte *)(buf+off)=wdata->format;
	off++;

	switch(wdata->format) {
		case 0x00: //integer (4 bytes)
			*(S32 *)(buf+off)=*(S32 *)(wdata->data);
			off+=4;
			break;
		case 0x03: //ustr (x bytes) (inverted)
			off+=encode_urustring(buf+off,(Byte *)wdata->data,\
strlen((char *)wdata->data),0xF0);
			break;
		case 0x07: //timestamp (8 bytes)
			*(double *)(buf+off)=*(double *)(wdata->data);
			off+=8;
			break;
		default: //unimplemented
			print2log(f_vmgr,"Fix Me, unimplemented plCreatableGenericValue %02X\n",wdata->format);
			break;
	}
	return off;
}

//destroys the data
void destroyCreatableGenericValue(void ** data) {
	t_CreatableGenericValue * wdata; //working data
	wdata=(t_CreatableGenericValue *)(*data);

	if(wdata->data!=NULL) {
		free(wdata->data);
		wdata->data=NULL;
	}
	free(*data);
	*data=NULL;
}

/// Creatable Stream

int storeCreatableStream(void ** data,Byte * buf,int size) {
	int off=0;
	int ret=0;
	t_CreatableStream * wdata;
	*data=malloc(sizeof(t_CreatableStream) * 1);
	if(*data==NULL) return -1;
	wdata=(t_CreatableStream *)(*data);
	wdata->size=*(U32 *)(buf+off);
	off+=4;
	wdata->data=NULL;

	static int t=0;
	plog(f_vmgr,"storeCreatableStream %i times\n",t++);

	if(wdata->size>(U32)size) { ret=-1; }
	else {
		wdata->data=malloc(sizeof(Byte) * wdata->size);
		if(wdata->data!=NULL) {
			memcpy((Byte *)wdata->data,buf+off,wdata->size);
			off+=wdata->size;
		} else {
			ret=-1;
		}
	}
	if(ret<0) {
		if(wdata->data!=NULL) {
			free(wdata->data);
			wdata->data=NULL;
		}
		if(*data!=NULL) {
			free(*data);
		}
		return ret;
	}
	return off;
}

int streamCreatableStream(Byte * buf,void * data) {
	int off=0;
	t_CreatableStream * wdata;
	wdata=(t_CreatableStream *)(data);
	*(U32 *)(buf+off)=wdata->size;
	off+=4;

	memcpy(buf+off,(Byte *)wdata->data,wdata->size);
	off+=wdata->size;
	return off;
}

void destroyCreatableStream(void ** data) {
	t_CreatableStream * wdata;
	if(*data!=NULL) {
		wdata=(t_CreatableStream *)(*data);
		if(wdata->data!=NULL) {
			free(wdata->data);
			wdata->data=NULL;
		}
		free(*data);
		*data=NULL;
	}
	static int t=0;
	plog(f_vmgr,"destroyCreatableStream %i times\n",t++);
}

/// Age Link Struct

int storeAgeLinkStruct(void ** data,Byte * buf,int size) {
	int offset=0;
	//int ret=0;
	t_AgeLinkStruct * wdata;
	*data=malloc(sizeof(t_AgeLinkStruct) * 1);
	if(*data==NULL) return -1;
	wdata=(t_AgeLinkStruct *)(*data);

	memset(wdata,0,sizeof(t_AgeLinkStruct));

	static int t=0;
	plog(f_vmgr,"storeAgeLinkStruct %i times\n",t++);

	//Initial Mask (U16) for the AgeLinkStruct
	// Found 0x0023 In VaultTasks
	// Found 0x0033 In FindAge msg's
	// Found 0x0073 Found when linking to Ahnonay (temple) from Restoration Guild
	// Then 0x0010 May be the CCR flag.
	// Then 0x0040 May be the age description? (text1?)
	// We have 3 remaining bits, one for the AgeInfoStruct, the other one
	//for the SpawnPoint and the other one may be the LinkingRules
	//
	//Note: I think that the mask is only ONE byte, but we are not 100% sure..

	wdata->mask=*(U16 *)(buf+offset);
	offset+=2;
	if((wdata->mask & 0xFF8C) || !(wdata->mask & 0x0023)) {
		plog(f_une,"AgeLinkStruct: Unimplemented mask found! %04X\n",wdata->mask);
		dump_packet(f_une,buf,size,0,7);
		print2log(f_une,"\n");
		return -1;
	}

	//Now It's supposed that We are parsing the AgeLinkStruct object

	//AgeInfoStruct mask
	//We have seen this masks soo far..
	// 0x02 (1) filename,??
	// 0x03 (2) filename,instance name
	// 0x0B (3) filename,instance name,user name
	// 0x0F (4) filename,instance name,guid,user name
	// 0x2F (5) filename,instance name,guid,user name,display name
	// 0x6F (6) filename,instance name,guid,user name,display name,language
	// Supposicions:
	// 0x04 The Age Guid
	// 0x08 The user defined name
	// 0x20 It's the DisplayName (Desc's name)
	// 0x40 Language

	wdata->ainfo.mask=*(Byte *)(buf+offset);
	offset++;
	if((wdata->ainfo.mask & 0x90)) { // || !(wdata->ainfo.mask & 0x03)) {
		plog(f_une,"AgeInfoStruct: Unimplemented mask found! %04X\n",wdata->ainfo.mask);
		dump_packet(f_une,buf,size,0,7);
		print2log(f_une,"\n");
		return -1;
	}

	if(wdata->ainfo.mask & 0x02) { //Filename
		//supposition 0x02
		offset+=decode_urustring(wdata->ainfo.filename,buf+offset,150); //the age filename
		offset+=2;
	}

	if(wdata->ainfo.mask & 0x01) { //Instance Name
		//supposition 0x01
		offset+=decode_urustring(wdata->ainfo.instance_name,buf+offset,150);
		offset+=2;
	} else {
		if(wdata->ainfo.mask & 0x02) {
			strcpy((char *)wdata->ainfo.instance_name,(char *)wdata->ainfo.filename);
			wdata->ainfo.mask |= 0x01; //and set the missing mask
		}
	}

	if(wdata->ainfo.mask & 0x04) {//Guid
		memcpy(wdata->ainfo.guid,buf+offset,8); //the guid
		offset+=8;
	}

	if(wdata->ainfo.mask & 0x08) { //User defined name
		offset+=decode_urustring(wdata->ainfo.user_name,buf+offset,150);
		//the owner (user defined name)
		offset+=2;
	}

	if(wdata->ainfo.mask & 0x20) {//Display Name
		offset+=decode_urustring(wdata->ainfo.display_name,buf+offset,150); //the owner
		offset+=2;
	}

	if(wdata->ainfo.mask & 0x40) {//Language
		wdata->ainfo.language=*(U32 *)(buf+offset); //seen 0 (language?)
		offset+=4;
	}
	//Here Ends the AgeInfoStruct

	wdata->unk=*(Byte *)(buf+offset); //Unknown Byte, always 0x01 (seen 0x00) (linking rules?) (Confirmed, that they are linking rules)
	offset++;

	#if 0
	if((wdata->unk!=0x01)) {
		plog(f_une,"AgeLinkStruct: Unimplemented unk byte found! %02X\n",wdata->unk);
		dump_packet(f_une,buf,size,0,7);
		print2log(f_une,"\n");
		return -1;
	}
	#endif

	wdata->rules=*(U32 *)(buf+offset); //link rules?
	offset+=4; //(not linking rules, seems another type of mask) (always 0x01)

	//SpawnPoint
	//SpawnPoint mask
	//Is always 0x00000007
	//An there are always 3 elements: Title,name & camera stack

	wdata->spoint.mask=*(U32 *)(buf+offset); //The Mask
	offset+=4;

	if((wdata->spoint.mask & 0xFFFFFFF8) || !(wdata->ainfo.mask & 0x00000007)) {
		plog(f_une,"SpawnPoint: Unimplemented mask found! %04X\n",wdata->ainfo.mask);
		dump_packet(f_une,buf,size,0,7);
		print2log(f_une,"\n");
		return -1;
	}

	offset+=decode_urustring(wdata->spoint.title,buf+offset,150); //the spawn point
	offset+=2;
	offset+=decode_urustring(wdata->spoint.name,buf+offset,150); //the linking point
	offset+=2;
	offset+=decode_urustring(wdata->spoint.camera,buf+offset,150); //the camera stack
	offset+=2;

	if(wdata->mask & 0x0010) {
		wdata->ccr=*(Byte *)(buf+offset); //ccr, I'm going to ban internal releases...
		offset++;
	}

	if(wdata->mask & 0x0040) {
		//ignoring this name for know
		Byte temp[100];
		offset+=decode_urustring(temp,buf+offset,99);
		offset+=2;
		//and disable the flag
		wdata->mask=(wdata->mask & ~(wdata->mask & 0x0040));
	}

	return offset;
}

int streamAgeLinkStruct(Byte * buf,void * data) {
	int offset=0;
	t_AgeLinkStruct * wdata;
	wdata=(t_AgeLinkStruct *)(data);

	//Initial Mask (U16) for the AgeLinkStruct
	// Found 0x0023 In VaultTasks
	// Found 0x0033 In FindAge msg's
	// Then 0x0010 May be the CCR flag.
	// We have 3 remaining bits, one for the AgeInfoStruct, the other one
	//for the SpawnPoint and the other one may be the LinkingRules
	//
	//Note: I think that the mask is only ONE byte, but we are not 100% sure..

	*(U16 *)(buf+offset)=wdata->mask;
	offset+=2;
	if((wdata->mask & 0xFFCC) || !(wdata->mask & 0x0023)) {
		plog(f_une,"AgeLinkStruct: Unimplemented mask found! %04X\n",wdata->mask);
	}

	//Now It's supposed that We are parsing the AgeLinkStruct object

	//AgeInfoStruct mask
	//We have seen this masks soo far..
	// 0x02 (2) filename,
	// 0x0B (3) filename,instance name,user name
	// 0x0F (4) filename,instance name,guid,user name
	// 0x2F (5) filename,instance name,guid,user name,display name
	// 0x6F (6) filename,instance name,guid,user name,display name,language
	// Supposicions:
	// 0x04 The Age Guid
	// 0x20 It's the DisplayName (Desc's name)
	// 0x40 Language

	*(Byte *)(buf+offset)=wdata->ainfo.mask;
	offset++;
	if((wdata->ainfo.mask & 0x90)) { // || !(wdata->ainfo.mask & 0x0B)) {
		plog(f_une,"AgeInfoStruct: Unimplemented mask found! %04X\n",wdata->ainfo.mask);
	}

	if(wdata->ainfo.mask & 0x02) {
		offset+=encode_urustring(buf+offset,wdata->ainfo.filename,\
	strlen((char *)wdata->ainfo.filename),0); //the age filename
	}
	if(wdata->ainfo.mask & 0x01) {
		offset+=encode_urustring(buf+offset,wdata->ainfo.instance_name,\
strlen((char *)wdata->ainfo.instance_name),0);
	}

	if(wdata->ainfo.mask & 0x04) {//Guid
		memcpy(buf+offset,wdata->ainfo.guid,8); //the guid
		offset+=8;
	}

	if(wdata->ainfo.mask & 0x08) {//User Defined name
	offset+=encode_urustring(buf+offset,wdata->ainfo.user_name,\
strlen((char *)wdata->ainfo.user_name),0);
	//the owner (user defined name)
	}

	if(wdata->ainfo.mask & 0x20) {//Display Name
		offset+=encode_urustring(buf+offset,wdata->ainfo.display_name,\
strlen((char *)wdata->ainfo.display_name),0); //the owner
	}

	if(wdata->ainfo.mask & 0x40) {//Language
		*(U32 *)(buf+offset)=wdata->ainfo.language; //seen 0 (language?)
		offset+=4;
	}
	//Here Ends the AgeInfoStruct

	*(Byte *)(buf+offset)=wdata->unk; //Unknown Byte, always 0x01
	offset++;
	if((wdata->unk!=0x01)) {
		plog(f_une,"AgeLinkStruct: Unimplemented unk byte found! %02X\n",wdata->unk);
	}

	*(U32 *)(buf+offset)=wdata->rules; //link rules?
	offset+=4;

	//SpawnPoint
	//SpawnPoint mask
	//Is always 0x00000007
	//An there are always 3 elements: Title,name & camera stack

	*(U32 *)(buf+offset)=wdata->spoint.mask; //The Mask
	offset+=4;

	if((wdata->spoint.mask & 0xFFFFFFF8) || !(wdata->ainfo.mask & 0x00000007)) {
		plog(f_une,"SpawnPoint: Unimplemented mask found! %04X\n",wdata->ainfo.mask);
	}

	offset+=encode_urustring(buf+offset,wdata->spoint.title,\
strlen((char *)wdata->spoint.title),0); //the spawn point
	offset+=encode_urustring(buf+offset,wdata->spoint.name,\
strlen((char *)wdata->spoint.name),0); //the linking point
	offset+=encode_urustring(buf+offset,wdata->spoint.camera,\
strlen((char *)wdata->spoint.camera),0); //the camera stack

	if(wdata->mask & 0x0010) {
		*(Byte *)(buf+offset)=wdata->ccr; //ccr, I'm going to ban internal releases...
		offset++;
	}

	return offset;
}

void destroyAgeLinkStruct(void ** data) {
	//t_AgeLinkStruct * wdata;
	if(*data!=NULL) {
		free(*data);
		*data=NULL;
	}

	static int t=0;
	plog(f_vmgr,"destroyAgeLinkStruct %i times\n",t++);

}


/// Server Guid

int storeServerGuid(void ** data,Byte * buf,int size) {
	int off=0;
	Byte * wdata;
	*data=malloc(sizeof(Byte) * 8);
	if(*data==NULL) return -1;
	wdata=(Byte *)(*data);
	memcpy(wdata,buf,8);
	off+=8;

	return off;
}

int streamServerGuid(Byte * buf,void * data) {
	int off=0;
	Byte * wdata;
	wdata=(Byte *)(data);
	memcpy(buf+off,wdata,8);
	off+=8;
	return off;
}

void destroyServerGuid(void ** data) {
	if(*data!=NULL) {
		free(*data);
		*data=NULL;
	}
}

//// Vault Node Ref

int storeVaultNodeRef(void ** data,Byte * buf,int size) {
	int off=0;
	t_vault_cross_ref * wdata;
	*data=malloc(sizeof(t_vault_cross_ref) * 1);
	if(*data==NULL) return -1;
	wdata=(t_vault_cross_ref *)(*data);

	wdata->id1=*(U32 *)(buf+off);
	off+=4;
	wdata->id2=*(U32 *)(buf+off);
	off+=4;
	wdata->id3=*(U32 *)(buf+off);
	off+=4;
	wdata->timestamp=*(U32 *)(buf+off);
	off+=4;
	wdata->microseconds=*(U32 *)(buf+off);
	off+=4;
	wdata->flag=*(Byte *)(buf+off);
	off++;

	return off;
}

void destroyVaultNodeRef(void ** data) {
	if(*data!=NULL) {
		free(*data);
		*data=NULL;
	}
}

int streamVaultNodeRef(Byte * buf,void * data) {
	int off=0;
	t_vault_cross_ref * wdata;
	wdata=(t_vault_cross_ref *)(data);

	*(U32 *)(buf+off)=wdata->id1;
	off+=4;
	*(U32 *)(buf+off)=wdata->id2;
	off+=4;
	*(U32 *)(buf+off)=wdata->id3;
	off+=4;
	*(U32 *)(buf+off)=wdata->timestamp;
	off+=4;
	*(U32 *)(buf+off)=wdata->microseconds;
	off+=4;
	*(Byte *)(buf+off)=wdata->flag;
	off++;

	return off;
}

//// A node

int storeVaultNode(void ** data,Byte * buf, int size) {
	//int off=0;
	int ret=0;
	t_vault_node * wdata;
	if(*data!=NULL) { free(*data); *data=NULL; }
	*data=malloc(sizeof(t_vault_node) * 1);
	if(*data==NULL) return -1;
	wdata=(t_vault_node *)(*data);

	init_node(wdata);
	ret=vault_parse_node(f_vmgr,buf,size,wdata);

	static int t=0;
	plog(f_vmgr,"storeVaultNode called %i times...\n",t++);

	if(ret<0) {
		print2log(f_vmgr,"An error ocurred parsing a vault node\n");
		destroy_node(wdata);
		free(*data);
		*data=NULL;
		plog(f_vmgr,"ERR: parsing a vault node %s\n",_WHERE("storeVaultNode"));
	}
	return ret;
}

int streamVaultNode(Byte * buf,void * data) {
	int off=0;
	t_vault_node * wdata;
	wdata=(t_vault_node *)(data);

	off+=node2stream(buf,wdata);

	return off;
}

//// Vault packer - unpacker

/** Unpacks the vault into the correct structs,
 it also umcompresses if is necessary
 flags 0x01 vault packet
       0x02 vtask packet
*/
int plVaultUnpack(Byte * buf,t_vault_mos * obj,int size,st_uru_client * u,int flags) {
	int offset=0;
	Byte * old_buf=NULL;
	U32 old_size;
	int old_offset;

	int success=0;

	int ret;

	int i;//,e,f;

	int tpots_mod;
	if(u->tpots==1) {
		tpots_mod=1;
	} else {
		tpots_mod=0;
	}

	obj->cmd=*(Byte *)(buf+offset); //vault command / operation
	offset++;
	obj->result=*(U16 *)(buf+offset); //result
	offset+=2;
	if(obj->result!=0) {
		print2log(f_vmgr,"VPR: Warning, non-zero %i (%04X) result code\n",obj->result,obj->result);
		dump_packet(f_vmgr,buf,size,0,7);
		print2log(f_vmgr,"\n");
		plog(f_vmgr,"ERR: bad result %s\n",_WHERE("vaultunpack"));
		return -1;
	}
	obj->zlib=*(Byte *)(buf+offset);
	offset++;
	obj->real_size=*(U32 *)(buf+offset);
	offset+=4;
	if(obj->zlib==0x03) { //we are in front of a compressed packet
		obj->comp_size=*(U32 *)(buf+offset);
		offset+=4;
		//store the old buffer settings
		old_buf=buf;
		old_size=size;
		old_offset=offset;
		//reset vars, and allocate memory
		buf=(Byte *)malloc(obj->real_size * sizeof(Byte));
		if(buf==NULL) {
			print2log(f_vmgr,"Not enough free memory!\n");
			plog(f_vmgr,"ERR: where did you put your ram? - %s\n",_WHERE("vaultunpack"));
			return -1;
		}
		offset=0;
		size=obj->real_size;
		//uncompress
		if(uncompress(buf,(uLongf *)&size,old_buf+old_offset,obj->comp_size)!=0) {
			print2log(f_vmgr,"Seems that an error ocurred uncompressing the stream!\n");
			plog(f_vmgr,"ERR: cannot uncompress on %s\n",_WHERE("vaultunpack"));
			free((void *)buf); //free the buffer
			return -1;
		}
		old_offset+=obj->comp_size;
		//now all continues
	} else if(obj->zlib!=0x01) {
			print2log(f_vmgr,"Unknow (zlib)format %02X\n",obj->zlib);
			plog(f_vmgr,"ERR: unknow zlib format on %s\n",_WHERE("vaultunpack"));
			return -1;
	}
	//normal parser here
	obj->n_itms=*(U16 *)(buf+offset);
	offset+=2;
	//allocate memory for the struct
	obj->itm=(t_vault_object *)malloc(sizeof(t_vault_object)*(obj->n_itms+1));
	for(i=0; (U32)i<obj->n_itms; i++) {
		plVaultItmInit(&obj->itm[i]);
		obj->itm[i].id=*(Byte *)(buf+offset);
		offset++;
		obj->itm[i].unk1=*(Byte *)(buf+offset);
		offset++;
		if(obj->itm[i].unk1!=0) {
			print2log(f_vmgr,"Unexpected itm.unk1 value of %02X at offset %08X, item %i\n",obj->itm[i].unk1,offset,i);
			success=-1;
			goto _end_for;
			//return -1;
		}
		obj->itm[i].dtype=*(U16 *)(buf+offset);
		offset+=2;

		ret=0;
		switch(obj->itm[i].dtype) {
			case DCreatableGenericValue:
				ret=storeCreatableGenericValue(&obj->itm[i].data,buf+offset,size-offset);
				break;
			case DCreatableStream:
				ret=storeCreatableStream(&obj->itm[i].data,buf+offset,size-offset);
				break;
			case DServerGuid:
				ret=storeServerGuid(&obj->itm[i].data,buf+offset,size-offset);
				break;
			case DAgeLinkStruct:
				ret=storeAgeLinkStruct(&obj->itm[i].data,buf+offset,size-offset);
				break;
			default:
				if(obj->itm[i].dtype==DVaultNodeRef+tpots_mod) {
					obj->itm[i].dtype=DVaultNodeRef;
					ret=storeVaultNodeRef(&obj->itm[i].data,buf+offset,size-offset);
				} else if(obj->itm[i].dtype==DVaultNode+tpots_mod) {
					obj->itm[i].dtype=DVaultNode;
					ret=storeVaultNode(&obj->itm[i].data,buf+offset,size-offset);
				} else {
					print2log(f_vmgr,"Unimplemented Data type %04X stream, fix-me\n",obj->itm[i].dtype);
					success=-1;
					goto _end_for;
				}
		}

		if(ret<0) {
			print2log(f_vmgr,"An error ocurred parsing a Vault Data type\n");
			success=-1;
			goto _end_for;
		}

		offset+=ret; //inc the offset
	}
_end_for:
	//end normal parser
	if(obj->zlib==0x03) { //it was compressed, let's go to clear some things
		if(offset!=size) {
			print2log(f_vmgr,"Warning, size mismatch in the buffer! %i vs %i\n",offset,size);
			dump_packet(f_vmgr,buf,size,0,7);
			print2log(f_vmgr,"\n");
			return -1;
		}
		if(buf!=NULL) {
			free((void *)buf);
		}
		//restore the old things
		buf=old_buf;
		offset=old_offset;
		size=old_size;
	}
	if(success!=0) { return -1; }
	//the 2nd object goes here
	if(flags==0x01) { //Only In vault messages
		obj->ctx=*(U16 *)(buf+offset);
		offset+=2;
		obj->res=*(U16 *)(buf+offset);
		offset+=2;
		if(obj->res!=0) {
			print2log(f_vmgr,"Unexpected non-zero res value at offset %08X\n",offset);
			return -1;
		}
	} else { //Nothing in vault tasks
		obj->ctx=0;
		obj->res=0;
		obj->vn=0;
	}
	if(flags==0x02) {
		obj->ctx=*(Byte *)(buf+offset);
		offset++;
	}
	obj->mgr=*(U32 *)(buf+offset);
	offset+=4;
	if(flags==0x01) {
		obj->vn=*(U16 *)(buf+offset);
		offset+=2;
	}
	if(offset!=size) {
		print2log(f_vmgr,"Warning, size mismatch! %i vs %i\n",offset,size);
		dump_packet(f_vmgr,buf,size,0,7);
		print2log(f_vmgr,"\n");
		plog(f_err,"ERR: size mismatch on %s\n",_WHERE("vaultunpack"));
		return -1;
	}

	return success;
}


/** packs the vault into the buffer,
 it also compress if is necessary
 flags 0x01 vault packet
       0x02 vtask packet
*/
int plVaultPack(Byte ** dbuf,t_vault_mos * obj,st_uru_client * u,Byte flags) {
	int off=0;
	int offset=0;
	int size_offset;
	int i;//,e;
	int size;

	int success=0;

	Byte * buf=NULL;

	//first calculate the maxium data size
	size=1+2+1+4+4+2+2+2+4+2; //standard max size for an main vault object
	DBG(4,"size is %i\n",size);
	//brownse the vault object for item sizes
	for(i=0; i<obj->n_itms; i++) {
		size+=1+1+2; //add more
		switch(obj->itm[i].dtype) {
			case DCreatableGenericValue:
				size+=1;
				switch(((t_CreatableGenericValue *)(obj->itm[i].data))->format) {
					case DInteger:
						size+=4;
						break;
					case DUruString:
						size+=strlen((char *)(((t_CreatableGenericValue *)(obj->itm[i].data))->data));
						size+=3;
						break;
					case DTimestamp:
						size+=8;
						break;
					default:
						DBG(4,"Unimplemented sub_dtype\n");
						size+=1000;
						break;
				}
				break;
			case DCreatableStream:
				size+=4;
				size+=(((t_CreatableStream *)(obj->itm[i].data))->size);
				break;
			case DServerGuid:
				size+=8;
				break;
			case DAgeLinkStruct:
				size+=sizeof(t_AgeLinkStruct);
				break;
			case DVaultNode:
				size+=(((t_vault_node *)(obj->itm[i].data))->data_size + ((t_vault_node *)(obj->itm[i].data))->data2_size + 200*12 + 30*4 + 8);
				break;
			case DVaultNodeRef:
				size+=4+4+4+8+1;
				break;
			default:
				print2log(f_vmgr,"FATAL: Invalid non-implemented format Type - STOP!\n");
				plog(f_vmgr,"ERR: unimplemented stuff on %s\n",_WHERE("vaultpack"));
				return -1;
		}
		size+=1000;
	}
	//now it's supposed that we have the maxium data size, let's go to create a big buffer
	*dbuf=(Byte *)malloc(sizeof(Byte) * size);
	DBG(5,"and now size is %i\n",size);

	//savefile((char *)*dbuf,size,"zlibdumpbefore.raw");

	//object one
	*(Byte *)(*dbuf+off)=obj->cmd;
	off++;
	*(U16 *)(*dbuf+off)=obj->result;
	off+=2;
	*(Byte *)(*dbuf+off)=obj->zlib;
	off++;
	size_offset=off; //store the offset where the one/two size vars go
	//*(U32 *)(buf+offset)=obj->real_size;
	//we don't have real_size (yet);
	off+=4;

	//-- dump after the for

	DBG(5,"checkpoint...\n");
	offset=0;
	if(obj->zlib==0x03) { //check if is a compressed stream
		//*(U32 *)(buf+offset)=obj->comp_size;
		//we don't have compressed size (yet);
		off+=4;
		//allocate a buffer for the things that will be compressed
		buf=(Byte *)malloc(sizeof(Byte) * size);
		//savefile((char *)buf,size,"zlibdump2before.raw");
	} else if(obj->zlib==0x01) {
		buf=*dbuf+off;
	} else {
		print2log(f_vmgr,"Hei, what's up with this code %02X\n",obj->zlib);
		plog(f_vmgr,"ERR: unimplemented stuff on %s\n",_WHERE("vaultpack"));
		free((void *)dbuf);
		return -1;
	}

	DBG(5,"Parser...\n");
	//normal parser here
	*(U16 *)(buf+offset)=obj->n_itms;
	offset+=2;
	for(i=0; (U32)i<obj->n_itms; i++) {
		*(Byte *)(buf+offset)=obj->itm[i].id;
		offset++;
		*(Byte *)(buf+offset)=obj->itm[i].unk1;
		offset++;
//-- tpots --
		if(u->tpots==1) {
			if(obj->itm[i].dtype==DVaultNode) {
				obj->itm[i].dtype=DVaultNode2;
			} else if(obj->itm[i].dtype==DVaultNodeRef) {
				obj->itm[i].dtype=DVaultNodeRef2;
			}
		}
//-- end tpots --
		*(U16 *)(buf+offset)=obj->itm[i].dtype;
//-- tpots --
		if(u->tpots==1) {
			if(obj->itm[i].dtype==DVaultNode2) {
				obj->itm[i].dtype=DVaultNode;
			} else if(obj->itm[i].dtype==DVaultNodeRef2) {
				obj->itm[i].dtype=DVaultNodeRef;
			}
		}
//-- end tpots --
		offset+=2;
		DBG(5,"i:%i,n_itms:%i\n",i,obj->n_itms);
		switch(obj->itm[i].dtype) {
			case DCreatableGenericValue:
				offset+=streamCreatableGenericValue(buf+offset,obj->itm[i].data);
				break;
			case DCreatableStream:
				offset+=streamCreatableStream(buf+offset,obj->itm[i].data);
				break;
			case DServerGuid:
				offset+=streamServerGuid(buf+offset,obj->itm[i].data);
				break;
			case DAgeLinkStruct:
				offset+=streamAgeLinkStruct(buf+offset,obj->itm[i].data);
				break;
			case DVaultNode:
				offset+=streamVaultNode(buf+offset,obj->itm[i].data);
				break;
			case DVaultNodeRef:
				offset+=streamVaultNodeRef(buf+offset,obj->itm[i].data);
				break;
			default:
				plog(f_vmgr,"fatal!! - fix-me not implemented\n");
				plog(f_vmgr,"ERR: unimplemented vault data type error on %s\n",_WHERE("vaultpack"));
				logflush(f_err);
		}
	}
	//end normal parser

	//well, now all data must be in the correct place, let's go to do more stuff
	//currently 'offset', must contain the exact size of these data, that, now we
	//need to put in the header
	DBG(5,"Checkpoint 34...\n");
	//real data size
	*(U32 *)(*dbuf+size_offset)=offset;

	if(obj->zlib==0x03) { //we are in front of a compressed packet
		//*(U32 *)(buf+offset)=obj->comp_size;
		//we don't have compressed size (yet);
		obj->comp_size=size; //this must be the buffer size before calling compress!!
		int ret;
		DBG(5,"Compressing...comp_size:%i,off:%i,offset:%i\n",obj->comp_size,off,offset);
			//savefile((char *)buf,offset,"zlibdump.raw");
			//savefile((char *)*dbuf,size,"zlibdump2.raw");

		ret=compress(*dbuf+off,(uLongf *)&obj->comp_size,buf,offset);
		DBG(5,"Compresssed...\n");
		if(ret!=0) {
			print2log(f_vmgr,"Seems that an error ocurred compressing the stream!\n");
			//ERR(4,"mecachis!\n");
			//perror("bad, very bad...");
			print2log(f_vmgr,"zlib error-> comp_size:%i,off:%i,offset:%i,ret:%i\n",obj->comp_size,off,offset,ret);
			success=-1;
			//savefile((char *)buf,offset,"zlibdump.raw");
			//savefile((char *)*dbuf,size,"zlibdump2.raw");
			//abort();
			plog(f_vmgr,"ERR: zlib error %s\n",_WHERE("vaultpack"));
		}
		//store the compressed size
		*(U32 *)(*dbuf+size_offset+4)=obj->comp_size;
		off+=obj->comp_size; //and just update the offset
		//also free the buffer
		free((void *)buf);
	} else {
		off+=offset; //no compression, let's go to continue with the old offset
	}
	DBG(6,"Checkpoint 120\n");

	if(success!=0) {
		plog(f_vmgr,"ERR: non-success code on %s\n",_WHERE("vaultpack"));
		free((void *)*dbuf);
		return -1;
	}

	//the 2nd object goes here
	if(flags==0x02) { //vaulttask
		*(Byte *)(*dbuf+off)=obj->ctx; //[Sub]
		off++;
		*(U32 *)(*dbuf+off)=obj->mgr; //[Client Id]
		off+=4;
	} else { //0x01 vault
		*(U16 *)(*dbuf+off)=obj->ctx;
		off+=2;
		*(U16 *)(*dbuf+off)=obj->res;
		off+=2;
		*(U32 *)(*dbuf+off)=obj->mgr;
		off+=4;
		*(U16 *)(*dbuf+off)=obj->vn;
		off+=2;
	}

	DBG(5,"off is %i\n",off);

	return off;
}

#if 0

void vault_item_destroy(t_vault_mos * obj) { //destroys all data of the obj struct
	int i,e;
	for(i=0; i<obj->n_itms; i++) {
		if(obj->itm[i].format==0xFF && obj->itm[i].raw_data!=NULL) {
			printf("destroying raw_data\n"); fflush(0);
			free((void *)obj->itm[i].raw_data);
			printf("destroyed raw_data!\n"); fflush(0);
		} else if(obj->itm[i].format==0xFE && obj->itm[i].table!=NULL) {
			printf("destroying table\n"); fflush(0);
			free((void *)obj->itm[i].table);
			printf("destroyed table!\n"); fflush(0);
		} else if(obj->itm[i].format==0xFD && obj->itm[i].manifest!=NULL) {
			printf("destroying manifest\n"); fflush(0);
			free((void *)obj->itm[i].manifest);
			printf("destroyed manifest!\n"); fflush(0);
		} else if(obj->itm[i].format==0xFC && obj->itm[i].cross_ref!=NULL) {
			printf("destroying cross_ref\n"); fflush(0);
			free((void *)obj->itm[i].cross_ref);
			printf("destroyed cross_ref!\n"); fflush(0);
		} else if(obj->itm[i].format==0xFB && obj->itm[i].node!=NULL) {
			for(e=0; (U32)e<obj->itm[i].num_items; e++) {
				if(obj->itm[i].node[e].data!=NULL) {
					printf("destroying node.data\n"); fflush(0);
					free((void *)obj->itm[i].node[e].data);
					printf("destroyed node.data!\n"); fflush(0);
				}
			}
			printf("destroying node\n"); fflush(0);
			free((void *)obj->itm[i].node);
			printf("destroyed node!\n"); fflush(0);
		}
	}
	if(obj->itm!=NULL) {
		free((void *)obj->itm);
	}
}

//!Creates objects data structs (remember to destroy them!)
t_vault_object * create_objects(int n) {
	return (t_vault_object *)malloc(sizeof(t_vault_object) * n);
}

//we sent a struct (with the vault stuff)
int vault_do_magic_stuff(int sock,st_uru_client * u,t_vault_mos * obj) {

	t_vault_mos sobj; //send object
	//U32 p_ki;
	Byte * data_buf=NULL; //pointer to the vault data
	int size; //data size

	int success=0;
	//int i;
	int a;
	//int b;

	//copy the data structs default values
	sobj.ki=obj->ki;
	sobj.code=obj->code;
	sobj.unk1=0; //set 0
	sobj.format=0x01; //default format
	sobj.fcode=obj->fcode;
	sobj.unk2=0;
	sobj.unk_byte=0; //obj->unk_byte; //ummm?
	sobj.fki=obj->fki;
	sobj.tail=obj->tail; //send the same as client//0x0001; //NOT always

	sobj.itm=NULL; //security

	switch (obj->code) {
		//****************************** CONNECT ******************************
		case 0x01: //Connect
			print2log(f_vmgr,"Vault Connect request for %i\n",obj->ki);
			if(obj->n_itms!=2 && obj->n_itms!=3) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//check item 1
			if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2 && obj->itm[0].integer!=3)) {
				print2log(f_vmgr,"Unknown unexpected Connect request format in item 1\n");
				success=-1;
				goto end_3;
			}
			if(obj->itm[0].integer==2) {
				//NOTES:
				/*
					The client starts the connection, sending the player ki number.
					Server replyes with the vnode folder id.

				*/
				//allocating for two items
				sobj.itm=create_objects(2);
				sobj.n_itms=2; //number of items
				a=1;
				//get ki from item2
				if(obj->itm[1].cmd!=0x0387 || obj->itm[1].format_flag!=0x02) {
					print2log(f_vmgr,"Unknown unexpected Connect request format in item 2\n");
					success=-1;
					goto end_3;
				}
				if(obj->itm[1].integer!=(U32)u->ki) {
					print2log(f_vmgr,"Ki mismatch!\n");
					success=-1;
					goto end_3;
				}
				//generate response here
				//itm 1
				sobj.itm[0].format_flag=0x02; //the ki
				sobj.itm[0].unk1=0x00; //always 0
				sobj.itm[0].cmd=0x0387;
				sobj.itm[0].format=0x00; //integer
				sobj.itm[0].integer=obj->itm[1].integer;
			} else {
				//NOTES:
				/*
					The client, starts the connection, sending the age name + age guid
					The server replyes with the node id of the age, and the folder id.
				*/
				//allocating for 3 items
				sobj.itm=create_objects(3);
				sobj.n_itms=3; //number of items
				a=2;
				//generate response here
				//itm 1
				sobj.itm[0].format_flag=0x02; //the age node
				sobj.itm[0].unk1=0x00; //always 0
				sobj.itm[0].cmd=0x0387;
				sobj.itm[0].format=0x00; //integer
#ifdef I_AM_A_GAME_SERVER
				sobj.itm[0].integer=global_age.node; //the age node
#else
				sobj.itm[0].integer=0; //there shouldn't be a node here
#endif
				//itm 2
				sobj.itm[1].format_flag=0x15; //the age name
				sobj.itm[1].unk1=0x00; //always 0
				sobj.itm[1].cmd=0x0387;
				sobj.itm[1].format=0x03; //a string
#ifdef I_AM_A_GAME_SERVER
				strcpy((char *)sobj.itm[1].ustring,(char *)global_age.filename); //the age name
#else
				strcpy((char *)sobj.itm[1].ustring,"NULLAGE"); //??
#endif
			}

			//itm 2
			sobj.itm[a].format_flag=0x17; //vault folder
			sobj.itm[a].unk1=0x00; //always 0
			sobj.itm[a].cmd=0x0387;
			sobj.itm[a].format=0x03; //ustring
			strcpy((char *)sobj.itm[a].ustring,global_vault_folder_name);
			print2log(f_vmgr,"Connect request to folder %s sent",global_vault_folder_name);
			if(obj->itm[0].integer==3) {
				print2log(f_vmgr," to age %s, node %i\n",sobj.itm[1].ustring,sobj.itm[0].integer);
			} else {
				print2log(f_vmgr,"\n");
			}
			break;
		//****************************** NegotiateManifest ******************************
		case 0x05: //NegotiateManifest
			print2log(f_vmgr,"Vault NegotiateManifest for %i\n",obj->ki);
			if(obj->n_itms!=2 && obj->n_itms!=3) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//allocating for two items
			sobj.itm=create_objects(2);
			sobj.n_itms=2; //number of items
			if(obj->n_itms==3) {
				//check item 0
				if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2 && obj->itm[0].integer!=3 && obj->itm[0].integer!=0x21)) {
					print2log(f_vmgr,"Unknown unexpected NegotiateManifest request format in item 1\n");
					success=-1;
					goto end_3;
				}
				a=1;
			} else {
				a=0;
			}
			//check item A
			if(obj->itm[a].format_flag!=0x0A || obj->itm[a].cmd!=0x0389) {
				print2log(f_vmgr,"Unknown unexpected NegotiateManifest request format in item %i\n",a+1);
				success=-1;
				goto end_3;
			}
			//check item A+1
			if(obj->itm[a+1].format_flag!=0x14 || obj->itm[a+1].cmd!=0x0387 || obj->itm[a+1].integer!=0xFFFFFFFF) {
				print2log(f_vmgr,"Unknown unexpected NegotiateManifest request format in item %i\n",a+2);
				success=-1;
				goto end_3;
			}
			//get the unical Vault Node, from where we have to get the manifest
			if(obj->itm[a].num_items!=1) {
				print2log(f_vmgr,"Unexpected number of items %i, for this type of request\n",obj->n_itms);
				success=-1;
				goto end_3;
			} //we have the node index at: obj->itm[0].table[0]
			//sleep(3);
			//abort();
			//generate response here
			//main object
			sobj.format=0x03; //enable compression
			//itm 1 - Manifest
			sobj.itm[0].format_flag=0x0E; //the manifest
			sobj.itm[0].unk1=0x00; //always 0
			sobj.itm[0].cmd=0x0389;
			sobj.itm[0].format=0xFD; //manifest
			sobj.itm[0].num_items=plVaultGetManifest(obj->itm[a].table[0],&sobj.itm[0].manifest);
			sobj.itm[0].data_size=(sobj.itm[0].num_items*(4+8))+4;
			//itm 2 - CrossReference
			sobj.itm[1].format_flag=0x0F; //cross reference
			sobj.itm[1].unk1=0x00; //always 0
			sobj.itm[1].cmd=0x0389;
			sobj.itm[1].format=0xFC; //cross reference
			sobj.itm[1].num_items=plVaultGetCrossRef(obj->itm[a].table[0],&sobj.itm[1].cross_ref);
			sobj.itm[1].data_size=(sobj.itm[1].num_items*(4*3+8+1))+4;
			print2log(f_vmgr,"%i manifest entryes, and %i cross references sent\n",sobj.itm[0].num_items,sobj.itm[1].num_items);
			break;
		//****************************** FetchNodes ******************************
		case 0x08: //FetchNodes
			print2log(f_vmgr,"Vault FetchNodes for %i\n",obj->ki);
			if(obj->n_itms!=1 && obj->n_itms!=2) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//allocating for three items
			sobj.itm=create_objects(3);
			sobj.n_itms=3; //number of items
			if(obj->n_itms==2) {
				//check item 0
				if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2 && obj->itm[0].integer!=3 && obj->itm[0].integer!=0x21)) {
					print2log(f_vmgr,"Unknown unexpected FetchNodes request format in item 1\n");
					success=-1;
					goto end_3;
				}
				a=1;
			} else {
				a=0;
			}

			//check item A
			if(obj->itm[a].format_flag!=0x0A || obj->itm[a].cmd!=0x0389) {
				print2log(f_vmgr,"Unknown unexpected FetchNodes request format in item %i\n",a+1);
				success=-1;
				goto end_3;
			}
			//Fetch the nodes
			//generate response here
			//main object
			sobj.format=0x03; //enable compression
			//itm 1 - Nodes
			sobj.itm[0].format_flag=0x06; //nodes
			sobj.itm[0].unk1=0x00; //always 0
			sobj.itm[0].cmd=0x0389;
			sobj.itm[0].format=0xFB; //nodes
			sobj.itm[0].num_items=obj->itm[a].num_items;
			//sobj.itm[0].data_size=(sobj.itm[0].num_items*(4+8))+4;
			sobj.itm[0].data_size=plVaultFetchNodes(&sobj.itm[0].node,obj->itm[a].num_items,obj->itm[a].table);
			//itm 2 - Number of nodes
			sobj.itm[1].format_flag=0x19; //number of nodes
			sobj.itm[1].unk1=0x00; //always 0
			sobj.itm[1].cmd=0x0387;
			sobj.itm[1].format=0x00; //integer
			sobj.itm[1].integer=obj->itm[a].num_items; //number of nodes
			//itm 3 - End of transmission
			sobj.itm[2].format_flag=0x1F; //eof
			sobj.itm[2].unk1=0x00; //always 0
			sobj.itm[2].cmd=0x0387;
			sobj.itm[2].format=0x00; //integer
			sobj.itm[2].integer=0; //always 0

			print2log(f_vmgr,"%i nodes sent\n",sobj.itm[0].num_items,sobj.itm[1].num_items);
			break;
		//****************************** FindNode ******************************
		case 0x07: //find node
			print2log(f_vmgr,"Vault FindNode for %i\n",obj->ki);
			if(obj->n_itms!=3) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//allocating for two items
			sobj.itm=create_objects(2);
			sobj.n_itms=2; //number of items
			//check item 1
			if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2 && obj->itm[0].integer!=3 && obj->itm[0].integer!=0x21)) {
				print2log(f_vmgr,"Unknown unexpected FindNode request format in item 1\n");
				success=-1;
				goto end_3;
			}
			//check item2
			if(obj->itm[1].format_flag!=0x05 || (obj->itm[1].cmd!=0x0439 && obj->itm[1].cmd!=0x043A)) {
				print2log(f_vmgr,"Unknown unexpected FindNode request format in item 2\n");
				success=-1;
				goto end_3;
			}
			//check item3
			if(obj->itm[2].format_flag!=0x10 || obj->itm[2].cmd!=0x0387 || (obj->itm[2].integer!=0 && obj->itm[2].integer!=1)) {
				print2log(f_vmgr,"Unknown unexpected FindNode request format in item 3\n");
				success=-1;
				goto end_3;
			}
			//generate response here
			t_vault_manifest mfst;

			plVaultFindNode(obj->itm[1].node, &mfst);

			//main object
			///sobj.format=0x03; //enable compression
			//itm 1 - Node index
			sobj.itm[0].format_flag=0x09; //node index
			sobj.itm[0].unk1=0x00; //always 0
			sobj.itm[0].cmd=0x0387;
			sobj.itm[0].format=0x00; //integer
			sobj.itm[0].integer=mfst.index;
			//itm 2 - timestamp
			sobj.itm[1].format_flag=0x18; //node timestamp
			sobj.itm[1].unk1=0x00; //always 0
			sobj.itm[1].cmd=0x0387;
			sobj.itm[1].format=0x07; //an 8 bytes number
			*(double *)sobj.itm[1].guid=mfst.timestamp;
			print2log(f_vmgr,"node %i-%d sent\n",mfst.index,mfst.timestamp);

			break;
		//****************************** AddNodeRef ******************************
		case 0x03: //add node ref
			print2log(f_vmgr,"Vault AddNodeRef for %i\n",obj->ki);
			if(obj->n_itms!=2) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//allocating for two items -- no response --
			//sobj.itm=create_objects(2);
			sobj.n_itms=0; //number of items
			//check item 1
			if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2 && obj->itm[0].integer!=3 && obj->itm[0].integer!=0x21)) {
				print2log(f_vmgr,"Unknown unexpected AddNodeRef request format in item 1\n");
				success=-1;
				goto end_3;
			}
			//check item2
			if(obj->itm[1].format_flag!=0x07 || (obj->itm[1].cmd!=0x0438 && obj->itm[1].cmd!=0x0439)) {
				print2log(f_vmgr,"Unknown unexpected AddNodeRef request format in item 2\n");
				success=-1;
				goto end_3;
			}

			if(obj->itm[1].cross_ref[0].timestamp==0) {
				time((time_t *)&obj->itm[1].cross_ref[0].timestamp);
				struct timeval tv;
				gettimeofday(&tv,NULL);
				obj->itm[1].cross_ref[0].microseconds=tv.tv_usec;
			}

			plVaultCreateRef(obj->itm[1].cross_ref[0].id1,obj->itm[1].cross_ref[0].id2,\
			obj->itm[1].cross_ref[0].id3,obj->itm[1].cross_ref[0].timestamp,\
			obj->itm[1].cross_ref[0].microseconds,obj->itm[1].cross_ref[0].flag);

			print2log(f_vmgr,"node %i-%d sent\n",mfst.index,mfst.timestamp);

			break;
		//****************************** RemoveNodeRef ******************************
		case 0x04: //remove node ref
			print2log(f_vmgr,"Vault RemoveNodeRef for %i\n",obj->ki);
			if(obj->n_itms!=3) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//allocating for two items -- no response --
			//sobj.itm=create_objects(2);
			sobj.n_itms=0; //number of items
			//check item 1
			if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2 && obj->itm[0].integer!=3 && obj->itm[0].integer!=0x21)) {
				print2log(f_vmgr,"Unknown unexpected RemoveNodeRef request format in item 1\n");
				success=-1;
				goto end_3;
			}
			//check item2
			if(obj->itm[1].format_flag!=0x09 || obj->itm[1].cmd!=0x0387) { //son
				print2log(f_vmgr,"Unknown unexpected RemoveNodeRef request format in item 2\n");
				success=-1;
				goto end_3;
			}
			//check item3
			if(obj->itm[2].format_flag!=0x0D || obj->itm[2].cmd!=0x0387) { //parent
				print2log(f_vmgr,"Unknown unexpected RemoveNodeRef request format in item 3\n");
				success=-1;
				goto end_3;
			}
			plVaultRemoveNodeRef(obj->itm[2].integer,obj->itm[1].integer);
			print2log(f_vmgr,"node_ref %i-%i deleted\n",obj->itm[2].integer,obj->itm[1].integer);

			break;
		//****************************** SaveNode ******************************
		case 0x06: //save node
			print2log(f_vmgr,"Vault SaveNode for %i\n",obj->ki);
			if(obj->n_itms!=2) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//check item 1
			if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2 && obj->itm[0].integer!=3)) {
				print2log(f_vmgr,"Unknown unexpected FindNode request format in item 1\n");
				success=-1;
				goto end_3;
			}
			//check item2
			if(obj->itm[1].format_flag!=0x05 || (obj->itm[1].cmd!=0x0439 && obj->itm[1].cmd!=0x043A)) {
				print2log(f_vmgr,"Unknown unexpected FindNode request format in item 2\n");
				success=-1;
				goto end_3;
			}
			//generate response here

			if(obj->itm[1].node[0].index<19000) {
				//allocating for two items
				sobj.itm=create_objects(2);
				sobj.n_itms=2; //number of items
				//query db
				sobj.itm[1].integer=plVaultCreateNode(obj->itm[1].node); // ****
			} else {
				//allocating for 1 items
				sobj.itm=create_objects(1);
				sobj.n_itms=1; //number of items
				//query db
				plVaultUpdateNode(obj->itm[1].node);
			}

			//main object
			///sobj.format=0x03; //enable compression
			//itm 1 - Node index
			sobj.itm[0].format_flag=0x09; //node index
			sobj.itm[0].unk1=0x00; //always 0
			sobj.itm[0].cmd=0x0387;
			sobj.itm[0].format=0x00; //integer
			sobj.itm[0].integer=obj->itm[1].node[0].index; //the old index
			if(sobj.n_itms==2) {
				//itm 2 - re-index node
				sobj.itm[1].format_flag=0x0B; //new node index
				sobj.itm[1].unk1=0x00; //always 0
				sobj.itm[1].cmd=0x0387;
				sobj.itm[1].format=0x00; //a integer ****
				print2log(f_vmgr,"node %i created as %i\n",sobj.itm[0].integer,sobj.itm[1].integer);
			} else {
				print2log(f_vmgr,"node %i updated\n",sobj.itm[0].integer);
			}

			break;
		//****************************** Disconnect ******************************
		case 0x02: //disconnect
			print2log(f_vmgr,"Vault Disconnect for %i\n",obj->ki);
			if(obj->n_itms!=1) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//allocating for two items -- no response --
			//sobj.itm=create_objects(2);
			sobj.n_itms=0; //number of items
			//check item 1
			if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2 && obj->itm[0].integer!=3 && obj->itm[0].integer!=0x21)) {
				print2log(f_vmgr,"Unknown unexpected Disconnect request format in item 1\n");
				success=-1;
				goto end_3;
			}
			print2log(f_vmgr,"vault disconnected\n");
			break;
		//****************************** SendNode ******************************
		case 0x09:
			print2log(f_vmgr,"Vault SendNode for %i\n",obj->ki);
			if(obj->n_itms!=3) {
				print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
				success=-1;
				goto end_3;
			}
			//allocating for two items -- no response --
			//sobj.itm=create_objects(2);
			sobj.n_itms=0; //number of items
			//check item 1
			if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2)) { // && obj->itm[0].integer!=3 && obj->itm[0].integer!=0x21)) {
				print2log(f_vmgr,"Unknown unexpected SendNode request format in item 1\n");
				success=-1;
				goto end_3;
			}
			//check item2
			if(obj->itm[1].format_flag!=0x04 || obj->itm[1].cmd!=0x0387) {
				print2log(f_vmgr,"Unknown unexpected SendNode request format in item 2\n");
				success=-1;
				goto end_3;
			}
			//check item3
			if(obj->itm[2].format_flag!=0x05 || (obj->itm[2].cmd!=0x0439 && obj->itm[2].cmd!=0x043A)) {
				print2log(f_vmgr,"Unknown unexpected SendNode request format in item 2\n");
				success=-1;
				goto end_3;
			}

			t_vault_node a_node;

			init_node(&a_node);

			//search for the inbox folder
			a_node.owner = obj->itm[1].integer;
			a_node.type = KFolderNode; //0x16 22
			a_node.torans = 0x01; //inbox folder 1

			t_vault_manifest a_mfs;

			print2log(f_vmgr,"Querying for inbox folder for that player\n");
			plVaultFindNode(&a_node,&a_mfs);

			time_t vlttime;
			struct timeval tv;
			gettimeofday(&tv,NULL);
			time(&vlttime);

			//create a reference to the recieved node
			plVaultCreateRef(a_node.owner,a_mfs.index,obj->itm[2].node[0].index,\
(U32)vlttime,tv.tv_usec,0);

			destroy_node(&a_node);

			print2log(f_vmgr,"node sent\n",mfst.index,mfst.timestamp);

			//print2log(f_vmgr,"Unimplemented, and ignored SendNode petition\n");
			sobj.n_itms=0;
			break;
		//****************************** SetSeen ******************************
		case 0x0A: //setseen
			print2log(f_vmgr,"Vault SetSeen for %i\n",obj->ki);
			print2log(f_vmgr,"Unimplemented, and ignored SetSeen petition\n");
			sobj.n_itms=0;
			break;
		default:
			print2log(f_vmgr,"Unkwon non-implemented vault request code %02X\n",obj->code);
			return -1;
	}

	//send here the item
	if(success==0 && (sobj.n_itms>0 || obj->code==0x02)) {
		//a data buffer will be created here
		print2log(f_vmgr,"packing the vault\n");
		logflush(f_vmgr);
		size=pack_vault(&data_buf,&sobj);
		//dump the packet vault
		store_packet(data_buf, size, "cow_dumps");
		//---- ecks
		print2log(f_vmgr,"finished packing the vault\n");
		logflush(f_vmgr);
		if(size>0) {
			//first pass throught the parser
			fprintf(f_vhtml,"<hr><font color=\"blue\"><b>SENT TO CLIENT</b></font>\n");
			dump_packet(f_vmgr,data_buf,size,0,7);
			print2log(f_vmgr,"\n");
			print2log(f_vmgr,"Calling parser...\n");
			logflush(f_vmgr);
			vault_parse_the_msg(data_buf,size,u);
			print2log(f_vmgr,"Succesfully parsed!, Sending %i bytes..,\n",size);
			logflush(f_vmgr);
			//then sent it
			success=plNetMsgVault(sock,u,data_buf,size);
			print2log(f_vmgr,"Succesfully sent!\n");
			logflush(f_vmgr);
		} else {
			success=-1;
		}
		//destroy the data buffer
		free((void *)data_buf);
	}

end_3:
	if(sobj.itm!=NULL) {
		printf("Destroying items\n");
		fflush(0);
		vault_item_destroy(&sobj);
		printf("End the items destruction\n");
		fflush(0);
	}
	if(success<0) {
		print2log(f_vmgr,"Failed parsing vault stream!\n");
		return success;
	}
	printf("RETURNING from vault_do_magic_stuff with value %i\n",success);
	fflush(0);
	return success;
}

//Sends the hosted age anounce message to the client
int sent_vault_age_anounce(int sock,st_uru_client * u,t_vault_node * node) {

	t_vault_mos sobj; //send object
	Byte * data_buf=NULL; //pointer to the vault data
	int size; //data size

	//copy the data structs default values
	sobj.ki=u->ki;
	sobj.code=0x0B; //anounce
	sobj.unk1=0; //set 0
	sobj.format=0x01; //default format
	sobj.fcode=0; //ctx (the context)
	sobj.unk2=0;
	sobj.unk_byte=0;
	sobj.fki=u->ki;
	sobj.tail=0x0000; //always??????????????

	sobj.itm=NULL; //security

	//allocating for 5 items
	sobj.itm=create_objects(5);
	sobj.n_itms=5; //number of items

	//main object
	///sobj.format=0x03; //enable compression
	//itm 1 - Node index
	sobj.itm[0].format_flag=0x09; //node index
	sobj.itm[0].unk1=0x00; //always 0
	sobj.itm[0].cmd=0x0387;
	sobj.itm[0].format=0x00; //integer
	sobj.itm[0].integer=node->index; //the player node
	//itm 2 - timestamp
	sobj.itm[1].format_flag=0x18; //node timestamp
	sobj.itm[1].unk1=0x00; //always 0
	sobj.itm[1].cmd=0x0387;
	sobj.itm[1].format=0x07; //an 8 bytes number //timestamp
	*(double *)sobj.itm[1].guid=(double)node->timestamp + ((double)node->timestamp/1000000); //the timestamp of the node
	//itm 3 - Age filename
	sobj.itm[2].format_flag=0x1B; //age filename
	sobj.itm[2].unk1=0x00; //always 0
	sobj.itm[2].cmd=0x0387;
	sobj.itm[2].format=0x03; //uru string (age filename)
	strcpy((char *)sobj.itm[2].ustring,(const char *)node->entry_name);
	//itm 4 - Age guid
	sobj.itm[3].format_flag=0x1C; //age filename
	sobj.itm[3].unk1=0x00; //always 0
	sobj.itm[3].cmd=0x0387;
	sobj.itm[3].format=0x03; //uru string (age guid)
	strcpy((char *)sobj.itm[3].ustring,(const char *)node->sub_entry_name);
	//itm 5 - an integer?
	sobj.itm[4].format_flag=0x1D; //end?
	sobj.itm[4].unk1=0x00; //always 0
	sobj.itm[4].cmd=0x0387;
	sobj.itm[4].format=0x00; //integer
	sobj.itm[4].integer=0x01; //always?? (number of players??!!)
	//0x01 - online
	//0x00 - offline

	print2log(f_vmgr,"vault anounce for %s, sent to %i\n",node->entry_name,node->index);

	print2log(f_vmgr,"packing the vault\n");
	logflush(f_vmgr);
	size=pack_vault(&data_buf,&sobj);
	//dump the packet vault
	store_packet(data_buf, size, "cow_dumps");
	//---- ecks
	print2log(f_vmgr,"finished packing the vault\n");
	logflush(f_vmgr);
	if(size>0) {
		//first pass throught the parser
		fprintf(f_vhtml,"<hr><font color=\"blue\"><b>SENT TO CLIENT</b></font>\n");
		dump_packet(f_vmgr,data_buf,size,0,7);
		print2log(f_vmgr,"\n");
		print2log(f_vmgr,"Calling parser...\n");
		logflush(f_vmgr);
		vault_parse_the_msg(data_buf,size,u);
		print2log(f_vmgr,"Succesfully parsed!, Sending %i bytes..,\n",size);
		logflush(f_vmgr);
		//then sent it
		plNetMsgVault(sock,u,data_buf,size);
		print2log(f_vmgr,"Succesfully sent!\n");
		logflush(f_vmgr);
	}
	//destroy the data buffer
	free((void *)data_buf);

	if(sobj.itm!=NULL) {
		vault_item_destroy(&sobj);
	}

	return 0;
}

//Sends the RefNode creation notification to the client
int plVaultCreateRefNotification(U32 id1,U32 id2,U32 id3,U32 stamp,U32 micros,Byte flag,int sock, st_uru_client * u) {

	t_vault_mos sobj; //send object
	Byte * data_buf=NULL; //pointer to the vault data
	int size; //data size

	//copy the data structs default values
	sobj.ki=u->ki;
	sobj.code=0x03; //AddNodeRef
	sobj.unk1=0; //set 0
	sobj.format=0x01; //default format
	sobj.fcode=0; //ctx (the context)
	sobj.unk2=0;
	sobj.unk_byte=0;
	sobj.fki=u->ki;
	sobj.tail=0x0000; //always??????????????

	sobj.itm=NULL; //security

	//allocating for 1 item
	sobj.itm=create_objects(1);
	sobj.n_itms=1; //number of items

	//main object
	///sobj.format=0x03; //enable compression
	//itm 1 - Node index
	sobj.itm[0].format_flag=0x07; //node index
	sobj.itm[0].unk1=0x00; //always 0
	if(u->tpots==1) { //tpots flag
		sobj.itm[0].cmd=0x0439;
	} else {
		sobj.itm[0].cmd=0x0438;
	}
	sobj.itm[0].format=0xFC; //cross_ref

	sobj.itm[0].num_items=1;
	//not sure, so let's go to calculate it for security
	sobj.itm[0].data_size=21; //sizeof(t_vault_cross_ref);

	sobj.itm[0].cross_ref=(t_vault_cross_ref *)(malloc(sizeof(t_vault_cross_ref)));
	sobj.itm[0].cross_ref->id1=id1;
	sobj.itm[0].cross_ref->id2=id2;
	sobj.itm[0].cross_ref->id3=id3;
	sobj.itm[0].cross_ref->timestamp=stamp;
	sobj.itm[0].cross_ref->microseconds=micros;
	sobj.itm[0].cross_ref->flag=flag;

	print2log(f_vmgr,"node ref added notification for %i...",u->ki);

	print2log(f_vmgr,"packing the vault\n");
	logflush(f_vmgr);
	size=pack_vault(&data_buf,&sobj);
	//dump the packet vault
	store_packet(data_buf, size, "cow_dumps");
	//---- ecks
	print2log(f_vmgr,"finished packing the vault\n");
	logflush(f_vmgr);
	if(size>0) {
		//first pass throught the parser
		fprintf(f_vhtml,"<hr><font color=\"blue\"><b>SENT TO CLIENT</b></font>\n");
		dump_packet(f_vmgr,data_buf,size,0,7);
		print2log(f_vmgr,"\n");
		print2log(f_vmgr,"Calling parser...\n");
		logflush(f_vmgr);
		vault_parse_the_msg(data_buf,size,u);
		print2log(f_vmgr,"Succesfully parsed!, Sending %i bytes..,\n",size);
		logflush(f_vmgr);
		//then sent it
		plNetMsgVault(sock,u,data_buf,size);
		print2log(f_vmgr,"Succesfully sent!\n");
		logflush(f_vmgr);
	}
	//destroy the data buffer
	free((void *)data_buf);

	if(sobj.itm!=NULL) {
		vault_item_destroy(&sobj);
	}

	return 0;
}

#endif

#endif
