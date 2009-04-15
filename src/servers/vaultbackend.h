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

#ifndef __U_VAULTBACKEND_H
#define __U_VAULTBACKEND_H
/* CVS tag - DON'T TOUCH*/
#define __U_VAULTBACKEND_H_ID "$Id$"

#include <protocol/vaultmsg.h>
#include <protocol/vaultproto.h>

#include "vaultdb.h"
#include "guidgen.h"

#include <list>

namespace alc {

	////DEFINITIONS
	
	class tVaultBackend {
	public:
		tVaultBackend(tUnet *net);
		~tVaultBackend(void);
		void reload(void);
		
		void sendPlayerList(tmCustomVaultAskPlayerList &askPlayerList);
		void checkKi(tmCustomVaultCheckKi &checkKi);
		void updatePlayerStatus(tmCustomVaultPlayerStatus &status);
		void processVaultMsg(tvMessage &msg, tNetSession *u, U32 ki);
		void processVaultTask(tvMessage &msg, tNetSession *u, U32 ki, U32 x);
		void deletePlayer(tmCustomVaultDeletePlayer &deletePlayer);
		U32 createPlayer(tmCustomVaultCreatePlayer &createPlayer); //!< \returns KI number of created player or 0 if name already exists
		void cleanVault(bool cleanAges);
		inline int getNumberOfPlayers(const Byte *uid) {
			return vaultDB->getPlayerList(uid);
		}
		inline int getMaxPlayers(void) { return maxPlayers; }
	private:
		// first the new types
		struct tVmgr {
			tVmgr(U32 ki, U32 mgr, tNetSessionIte session) {
				this->ki = ki;
				this->mgr = mgr;
				this->session = session;
			}
			U32 ki;
			U32 mgr;
			tNetSessionIte session;
		};
		
		typedef std::list<tVmgr> tVmgrList;
	
		void unload(void);
		void load(void);
		
		/** send a vault message */
		void send(tvMessage &msg, tNetSession *u, U32 ki, U32 x = 0);
		
		/** finds the vault mgr with that data and updates its session ite
		    \returns the iterator of that vmgr or vmgrs.end() */
		tVmgrList::iterator findVmgr(tNetSession *u, U32 ki, U32 mgr);
		
		// high-level vault tasks
		
		/** creates the main vault nodes like AllPlayers and GlobalInbox */
		void createVault(void);
		
		/** ensures that the main vault nodes (AllPlayers, System) exist and are children of the AdminMGR */
		void checkMainNodes(void);
		
		/** searches the age by filename and creates it if necessary and wished, returns the ID of the age info node.
		    If the age should not be created and it is not found, returns 0 */
		U32 getAge(tvAgeInfoStruct &ageInfo, bool create = true);
		
		/** creates an age, returns the ID of the age info node */
		U32 createAge(tvAgeInfoStruct &ageInfo);
		
		/** finds this age in the list of ages this player owns/can visit. The ID of the folder the link is in is saved in linkedAgesFolder
		    \returns the ID of the link or 0 if not found */
		U32 findAgeLink(U32 ki, U32 ageInfoNode, U32 *linkedAgesFolder, bool visitedAge = false);
		
		/** gives the player a link to that age and adds the linking point
		    if noUpdate is true, the spawn point is only added if the link did not yet exist
		    \returns the ID of the link node or 0 if it's an invite and the player owns that age */
		U32 addAgeLinkToPlayer(U32 ki, U32 ageInfoNode, tvSpawnPoint &spawnPoint, bool noUpdate = false, bool visitedAge = false);
		
		/** removes the link to that age from the given player */
		void removeAgeLinkFromPlayer(U32 ki, U32 ageInfoNode, bool visitedAge = false);
		
		/** adds/removes that player to/from the owners/visitors of this age */
		void addRemovePlayerToAge(U32 ageInfoNode, U32 ki, bool visitor = false, bool remove = false);
		
		// mid-level vault functions (doing broadcasts)
		//  the low-level functions are in tVaultDB
		
		/** get the ID of that node. If it doesn't exist, create it (with the given parent and saver) and broadcast the update */
		U32 getChildNodeBCasted(U32 saver, U32 parent, tvNode &node);
		
		/** create that node (with the given parent and saver) and broadcast the update */
		U32 createChildNodeBCasted(U32 saver, U32 parent, tvNode &node);
		
		/** add a ref and broadcast the update */
		void addRefBCasted(U32 saver, U32 parent, U32 son);
		
		// helper functions for broadcasting
		
		/** broadcast that this node has been changed, ecxlude the vmgr with the given data */
		void broadcastNodeUpdate(tvNode &node, U32 origKi = 0, U32 origMgr = 0);
		
		/** broadcast that this node ref has been changed (set remove=true if it has been removed),
		    ecxlude the vmgr with the given data. The passed tvNodeRef instance will be deleted.  */
		void broadcastNodeRefUpdate(tvNodeRef *ref, bool remove, U32 origKi = 0, U32 origMgr = 0);
		
		/** broadcast the online state of this player info node */
		void broadcastOnlineState(tvNode &node, U32 origKi = 0, U32 origMgr = 0);
		
		/** broadcast the message to all vmgrs interested in that node, except for the one with origKi and origMgr
		    (i.e. the one who did the change) */
		void broadcast(tvMessage &msg, U32 node, U32 origKi = 0, U32 origMgr = 0);
	
		tVaultDB *vaultDB;
		char vaultFolderName[17];
		U32 adminNode;
		
		tGuidGen *guidGen;
		
		tLog *log, *logHtml;
		bool shortHtml;
		tUnet *net;
		
		// settings
		int maxPlayers;
		char welcomeMsgTitle[512], welcomeMsgText[4096];
		char hoodName[512], hoodDesc[512];
		bool linkingRulesHack;
		bool autoRemoveMgrs;
		
		// the list of vmgrs
		tVmgrList vmgrs;
	};


} //End alc namespace

#endif
