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

#ifndef __U_VAULT_TASKS_H_
#define __U_VAULT_TASKS_H_
/* CVS tag - DON'T TOUCH*/
#define __U_VAULT_TASKS_H_ID "$Id$"

#include "vaultstrs.h"
#include "urustructs.h"

/*
 Parses and generates Vault tasks
*/

int plVaultInitVault();

/** Creates an Age, awesome eh!
*/
int plVaultCreateAge(t_AgeInfoStruct * ainfo);

int plVaultAddLinkingPoint(st_unet * net,int ki,int age_id,t_SpawnPoint * spoint);

//Age id, is the Age Info node eh!!
int plVaultAddOwnerToAge(st_unet * net,int age_id,int ki);

int plVaultUpdatePlayerStatus(st_unet * net,U32 id,Byte * age,Byte * guid,Byte state,U32 online_time,int sid);

/*--------------------------------------------------------------------
  Creates a new player, and returns the KI
	returns -1 on db error
	returns 0 if player already exists
	returns KI if player has been succesfully created
--------------------------------------------------------------------*/
int plVaultCreatePlayer(st_unet * net,Byte * login,Byte * guid, Byte * avie,Byte * gender,Byte access_level);

/*--------------------------------------------------------------------
  Deletes a player
	returns -1 on db error
	returns 0 or >0 on success
--------------------------------------------------------------------*/
int plVaultDeletePlayer(st_unet * net,Byte * guid, U32 ki,Byte access_level);

//we sent a struct (with the vault task stuff)
int plAdvVaultTaskParser(st_unet * net,t_vault_mos * obj,int sid);

#endif
