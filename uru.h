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

#ifndef __U_URU_H_
#define __U_URU_H_
/* CVS tag - DON'T TOUCH*/
#define __U_URU_H_ID "$Id$"

#include "ageparser.h"
#include "sdlparser.h"

extern st_uru_client * all_players; //the big struct

extern st_uru_client * auth; //points to the auth server
extern st_uru_client * vault; //points to the vault server
extern st_uru_client * track; //poinst to the tracking server

extern st_uru_client * meta; //points to the metaserver

extern int whoami; //To fix a big problem caused by the restructuration
	//same codes as prot.h
	/* KAgent 1 //unused, lobby does the job
	   KLobby 2
	   KGame 3
	   KVault 4
	   KAuth 5
	   KAdmin 6
	   KLookup 7 //tracking
	   KClient 8
	   KMeta 9
		 KTracking 7
	*/

extern t_sdl_def * global_sdl; //sdl struct
extern int global_sdl_n; //number of global sdl records

extern t_age_def * global_age_def; //age struct
extern int global_age_def_n; //number of ages

#endif

