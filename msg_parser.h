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

#ifndef __MSG_SPARSER_S_H_
#define __MSG_SPARSER_S_H_

#define __MSG_SPARSER_S_H_ID "$Id$"

//Another message parser
int parse_uru_object_desc(Byte * buf,st_uru_object_desc * o);
int parse_sub_msg(Byte * buf,int size,st_sub_msg * s);
int parse_adv_msg(Byte * buf,int size,st_uru_adv_msg * m,st_uru_client * u);
int put_uru_object_desc(Byte * buf,st_uru_object_desc * o);
int create_sub_msg(Byte * buf,st_sub_msg * s);
int create_adv_msg(Byte * buf,st_uru_adv_msg * m,st_uru_client * u);
int send_response(int sock,st_uru_adv_msg * m,st_uru_client * u);
int send_load_player_list(int sock,st_uru_client * u);

#endif
