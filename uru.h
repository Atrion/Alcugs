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

#ifndef __U_URU_H_
#define __U_URU_H_
/* CVS tag - DON'T TOUCH*/
#define __U_URU_H_ID "$Id$"

#include "config_parser.h"

extern st_config * global_config; //the config params are here

void kill_player(st_unet * net,int reason,int sid,Byte flag);

#if 0

#include "ageparser.h"
#include "sdlparser.h"

extern t_sdl_def * global_sdl; //sdl struct
extern int global_sdl_n; //number of global sdl records

extern t_age_def * global_age_def; //age struct
extern int global_age_def_n; //number of ages

#endif

#endif

