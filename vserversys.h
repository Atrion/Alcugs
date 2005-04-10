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

//server side message parser

#ifndef __U_VAULTSERVERSYS_H_
#define __U_VAULTSERVERSYS_H_
/* CVS tag - DON'T TOUCH*/
#define __U_VAULTSERVERSYS_H_ID "$Id: psauthmsg_internal.cpp,v 1.1 2004/10/24 11:20:27 almlys Exp $"

#include "sql.h"

extern st_sql db;

extern int vault_server_initialitzed;
extern Byte max_players_per_account;

extern char global_vault_folder_name[20];

extern char * ghood_name;
extern char * ghood_desc;

extern char * gvinit_title;
extern char * gvinit_desc;


//!vault managers
typedef struct {
	int id; //if >100 it's a normal node KI number for a player, elsewhere is a server manager node
	int node; //the node VMGR id that we are monitoring
	//U32 stamp; //a timestamp
	U32 ip; //ip that identifies the server session
	U16 port; //port that identifies the server session
	int sid; //the unique session identifier
} st_vault_mgrs;


int init_vault_server_subsys();
void stop_vault_server_subsys();
void reload_vault_server_subsys();
void update_vault_server_subsys();
void vault_server_subsys_idle_operation();

#endif

