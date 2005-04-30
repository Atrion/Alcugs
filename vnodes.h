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

#ifndef _VAULT_NODES_H_
#define _VAULT_NODES_H_
#define _VAULT_NODES_H_ID "$Id$"

#include "data_types.h"
#include "vaultstrs.h"

void init_node(t_vault_node * node);
void destroy_node(t_vault_node * node);
void dump_node(Byte * buf, int max);

/** Puts the node into the buffer, and returns the size of the node
*/
int node2stream(Byte * buf,t_vault_node * node);

/** Parse node
	\param f: file descriptor where errors, an unexpected things are dumped out\n
	\param buf: the buffer where all the stuff is located\n
	\param max_stream: maxium size of the buffer, to avoid reading ahead...\n
	\param node: the addres of the node struct, where only _one_ node will be stored\n
	returns the size of the parsed node
*/
int vault_parse_node(st_log * f,Byte * buf, int max_stream, t_vault_node * node);


#endif

