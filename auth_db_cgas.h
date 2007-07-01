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

/*
	MySQL driver for the Auth server
*/

#ifndef __U_AUTH_DB_H_
#define __U_AUTH_DB_H_
/* CVS tag - DON'T TOUCH*/
#define __U_AUTH_DB_H_ID "$Id: auth_db.h 213 2005-04-30 18:45:56Z almlys $"

int plVaultQueryUserName(Byte * login, U32 * attempt, U32 * att,st_sql * db);
int plVaultAddUser(Byte * login,Byte * guid,Byte * ip,Byte a_level,st_sql * db);
int plVaultUpdateUser(Byte * login,Byte * guid,Byte * ip,int n_tryes,st_sql * db);
int plVaultInitializeAuthDB(st_sql * db,char force);

#endif
