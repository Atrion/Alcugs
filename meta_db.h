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

/*
	MySQL driver for the Meta server
	*public implementation*
*/


#ifndef __U_META_DB_H_
#define __U_META_DB_H_
/* CVS tag - DON'T TOUCH*/
#define __U_META_DB_H_ID "$Id$"

typedef struct {
	Byte address[100];
	int id;
} t_meta_servers;

int plMetaAddServer(Byte * ip,Byte * address,Byte * name,Byte * desc,Byte * contact,\
Byte * site,U32 dset,Byte * passwd);
int plMetaUpdateServer(int id,Byte * ip,int status,double ping,int pop);
int plMetaGetServers(t_meta_servers ** servers);

#endif
