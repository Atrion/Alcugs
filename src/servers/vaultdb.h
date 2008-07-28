/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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

#ifndef __U_VAULTDB_H
#define __U_VAULTDB_H
/* CVS tag - DON'T TOUCH*/
#define __U_VAULTDB_H_ID "$Id$"

#include <protocol/vaultproto.h>

namespace alc {

	////DEFINITIONS
	class tVaultDB {
	public:
		tVaultDB(tLog *log);
		~tVaultDB(void) { if (sql) delete sql; }
		
		/** queries the player list and saves it in the buffer \returns the number of players */
		int getPlayerList(tMBuf &t, const Byte *uid);
		
		/** checks if this account (uid) owns that ki and saves the avatar name (array must be at least 256 Bytes)
		    \returns 1 when the avatar belings to that account, 0 otherwise */
		int checkKi(U32 ki, const Byte *uid, Byte *avatar);
		
		/** looks up a node in the database, using all fields which have their flag turned on (except for timestamps and blobs)
		    \returns the ID of the found/created node, 0 if neither found nor created */
		U32 findNode(tvNode &node, bool create);
		
		U32 createNode(tvNode &node);
	private:
		bool prepare(void);
		int getVersion(void);
		void migrateVersion2to3(void);
		void convertIntToTimestamp(const char *table, const char *intColumn, const char *timestampColumn);
		
		tSQL *sql;
		tLog *log;
	};


} //End alc namespace

#endif
