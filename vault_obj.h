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

#ifndef __VAULT_OBJ_S_H_
#define __VAULT_OBJ_S_H_

#define __VAULT_OBJ_S_H_ID "$Id$"

#include "data_types.h"

//New vault parsing functions
void plMainVaultInit(t_vault_mos * mobj);
void plMainVaultDestroy(t_vault_mos * mobj,int tpots_flag);
void plVaultItmInit(t_vault_object * itm);
void plVaultItmDestroy(t_vault_object * itm,int tpots_flag);
int storeCreatableGenericValue(void ** data,Byte * buf,int size);
void destroyCreatableGenericValue(void ** data);
int storeCreatableStream(void ** data,Byte * buf,int size);
void destroyCreatableStream(void ** data);
int storeVaultNodeRef(void ** data,Byte * buf,int size);
void destroyVaultNodeRef(void ** data);
void destroyServerGuid(void ** data);
int streamServerGuid(Byte * buf,void * data);
int storeServerGuid(void ** data,Byte * buf,int size);
int storeAgeLinkStruct(void ** data,Byte * buf,int size);
int streamAgeLinkStruct(Byte * buf,void * data);
void destroyAgeLinkStruct(void ** data);


int plVItmGetInteger(t_vault_object * o);
void plVItmPutInteger(t_vault_object * o,int integer);
double plVItmGetTimestamp(t_vault_object * o);
void plVItmPutTimestamp(t_vault_object * o,double stamp);
Byte * plVItmGetString(t_vault_object * o);
void plVItmPutString(t_vault_object * o,Byte * str);

//!Creates objects data structs (remember to destroy them!)
t_vault_object * vaultCreateItems(int n);

//creates this data type
int streamCreatableGenericValue(Byte * buf,void * data);

int streamCreatableStream(Byte * buf,void * data);

void destroyCreatableStream(void ** data);

void destroyServerGuid(void ** data);

int storeVaultNodeRef(void ** data,Byte * buf,int size);

void destroyVaultNodeRef(void ** data);

int streamVaultNodeRef(Byte * buf,void * data);

int storeVaultNode(void ** data,Byte * buf, int size);

int streamVaultNode(Byte * buf,void * data);

/** Unpacks the vault into the correct structs,
 it also umcompresses if is necessary
 flags 0x01 vault packet
       0x02 vtask packet
*/
int plVaultUnpack(Byte * buf,t_vault_mos * obj,int size,st_uru_client * u,int flags);

/** packs the vault into the buffer,
 it also compress if is necessary
 flags 0x01 vault packet
       0x02 vtask packet
*/
int plVaultPack(Byte ** dbuf,t_vault_mos * obj,st_uru_client * u,Byte flags);

#endif
