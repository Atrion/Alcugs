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

#ifndef _HTML_DUMPER_H_
#define _HTML_DUMPER_H_
#define _HTML_DUMPER_H_ID "$Id$"

//returns the pointer to the string associated to this node
const char * vault_get_type(Byte type);
//get the permissions
void dump_permissions(st_log * f,U32 permissions);
//print the flags
void dump_flags(st_log * f,U32 flag1,U32 flag2,U32 flag3);

//returns the pointer to the string associated to this node
const char * vault_get_folder_type(U32 type);
const char * vault_get_operation(Byte type);
const char * vault_get_task(Byte type);
const char * vault_get_data_type(U16 dtype);
const char * vault_get_sub_data_type(Byte type);

//html dumper
int vault_parse_node_data(st_log * f, t_vault_node * n);
void htmlParseCreatableGenericValue(st_log * f,void * data);
void htmlParseCreatableStream(st_log * f,void * data,Byte id);
//rcv flag 0x01 from peer, 0x00 to peer
void htmlVaultParse(st_unet * net,t_vault_mos * vobj,int sid,int recv);

#endif

