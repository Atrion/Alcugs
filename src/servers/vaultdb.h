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

namespace alc {

	////DEFINITIONS
	class tVaultDB {
	public:
		tVaultDB(tLog *log);
		~tVaultDB(void) { delete sql; }
		void getVaultFolderName(char *folder);
		
		/** queries the player list and saves it in the buffer if the point is not NULL \returns the number of players */
		int getPlayerList(const Byte *uid, tMBuf *t = NULL);
		
		/** checks if this account (uid) owns that ki and saves the avatar name (array must be at least 256 Bytes)
		    \returns true when the avatar belongs to that account, false otherwise */
		bool checkKi(U32 ki, const Byte *uid, char *avatar);
		
		/** looks up a node in the database, using all fields which have their flag turned on (except for blobs)
		    \returns the ID of the found/created node, 0 if neither found nor created */
		U32 findNode(tvNode &node, bool create = false, tvManifest *mfs = NULL);
		
		/** creates a new node and returns its ID */
		U32 createNode(tvNode &node);
		
		/** creates a new node and a ref to it
		    \returns the ID of the node */
		U32 createChildNode(U32 saver, U32 parent, tvNode &node);
		
		/** updates a given vault node */
		void updateNode(tvNode &node);
		
		/** queries all direct and indirect child nodes of the given base node and saves their manifest as well as the refs connecting them.
		    Remember to free the tables and delete all their elements! */
		void getManifest(U32 baseNode, tvManifest ***mfs, int *nMfs, tvNodeRef ***ref, int *nRef); // these are pointers to an array of pointers
		
		/** get all the nodes whose IDs are in the table (saved as tableSize U32 values) and put them in the array.
		    Remember to free the node table and delete all its elements! */
		void fetchNodes(U32 *table, int tableSize, tvNode ***nodes, int *nNodes); // this is a pointer to an array of pointers
		void fetchNodes(tMBuf &table, int tableSize, tvNode ***nodes, int *nNodes); // this is a pointer to an array of pointers
		
		/** checks if this node exists */
		bool checkNode(U32 node);
		
		/** gets a list of all direct and indirect MGRS (parent nodes with a type <= 7) of this node
		    Remember to free the table */
		void getMGRs(U32 baseNode, U32 **table, U32 *tableSize);
		
		/** removes a node ref */
		void removeNodeRef(U32 parent, U32 son, bool cautious = true);
		
		/** remove a node and all sub-nodes which are not used elsewhere */
		void removeNodeTree(U32 node, bool cautious = true);
		
		/** creates a node ref
		    \returns true if the ref was added, false when this is a duplicate */
		bool addNodeRef(tvNodeRef &ref);
		
		/** updates the "seen" flag of that ref */
		void setSeen(U32 parent, U32 son, Byte seen);
		
		/** saves a list of direct parent nodes */
		void getParentNodes(U32 node, U32 **table, int *tableSize);
		
		/** get all references (direct and indirect) of this node */
		void getReferences(U32 node, tvNodeRef ***ref, int *nRef); // this is a pointer to an array of pointers
		
		/** removes all lost nodes and their subnodes */
		void clean(bool cleanAges);
	private:
		/** creates a query to SELECT (isUpdate = false) or UPDATE (isUpdate = true) that vault node
		    \returns a pointer to the query which you have to free */
		tString createNodeQuery(tvNode &node, bool isUpdate);
		
		bool prepare(void);
		int getVersion(void);
		void migrateVersion2to3(void);
		void migrateVersion3to4(void);
		void convertIntToTimestamp(const char *table, const char *intColumn, const char *timestampColumn);
		void convertIntToDouble(const char *table, const char *intColumn, const char *doubleColumn);
		void removeInvalidRefs(void);
		bool isLostAge(int id);
		
		tSQL *sql;
		tLog *log;
	};


} //End alc namespace

#endif
