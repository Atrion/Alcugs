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

#ifndef __VAULT_ADVP_S_H_
#define __VAULT_ADVP_S_H_

#define __VAULT_ADVP_S_H_ID "$Id$"

extern st_vault_mgrs * vmgrs;
extern int n_vmgrs;

void init_vmgr_data(st_vault_mgrs ** t);
int check_me_in(st_unet * net,int id,int node,int sid);
int plVaultBcastNodeReferenceAdded(st_unet * net,int father,int son);
int plVaultBcastNodeReferenceRemoved(st_unet * net,int father,int son);
int plVaultBcastNodeSaved(st_unet * net,int index,double timestamp);
int plVaultBcastOnlineState(st_unet * net,t_vault_node * n);
int plAdvVaultParser(st_unet * net,t_vault_mos * obj,int sid);

#endif
