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
	MySQL driver for the Auth server
	*public implementation*
*/


#ifndef __U_AUTH_DB_H_
#define __U_AUTH_DB_H_
/* CVS tag - DON'T TOUCH*/
#define __U_AUTH_DB_H_ID "$Id$"

int plVaultInitializeAuthDB();
/*-----------------------------------------------------------
  Query the database for an specified username
	get the passwd and
	Then return the user access_level
	returns -1 if it was no possible to find a user
------------------------------------------------------------*/
int plVaultQueryUserName(Byte * login);
/* Store user informations info accounts table */
int plVaultStoreUserInfo(Byte * login, int exists, int dologin);
#endif
