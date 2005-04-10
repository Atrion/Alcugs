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

#ifndef __U_VAULTSUBSYS_H_
#define __U_VAULTSUBSYS_H_
/* CVS tag - DON'T TOUCH*/
#define __U_VAULTSUBSYS_H_ID "$Id: psauthmsg_internal.cpp,v 1.1 2004/10/24 11:20:27 almlys Exp $"

#include "ageparser.h"

int init_vault_subsys();
void stop_vault_subsys();
void reload_vault_subsys();

extern st_log * f_vmgr;
extern st_log * f_vhtml;

extern char * system_addr;
extern U16 system_port;

extern int vault_initialitzed;

//extern t_age_def * global_age_def; //age struct
//extern int global_age_def_n; //total number of ages


#endif

