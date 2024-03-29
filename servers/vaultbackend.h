/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

#include <netsessionmgr.h>
#include <protocol/vaultproto.h>
#include <protocol/vaultmsg.h>
#include <alcutil/alclog.h>

#include <list>

namespace alc {

	class tvAgeLink;
	class tVaultDB;
	
	class tVaultBackend {
	public:
		tVaultBackend(tUnet *net);
		~tVaultBackend(void);
		void applyConfig(void);
		
		void sendPlayerList(tmRequestMyVaultPlayerList &askPlayerList);
		void sendAgeList(tmGetPublicAgeList &getAgeList);
		void createPublicAge(tmCreatePublicAge &createAge);
		void removePublicAge(tmRemovePublicAge &removeAge);
		void checkKi(tmCustomVaultCheckKi &checkKi);
		void updatePlayerStatus(tmCustomVaultPlayerStatus &status);
		void processVaultMsg(tvMessage &msg, tNetSession *u, uint32_t ki);
		void processVaultTask(tvMessage &msg, tNetSession *u, uint32_t ki, uint32_t x);
		void deletePlayer(tmCustomVaultDeletePlayer &deletePlayer);
		uint32_t createPlayer(tmCustomVaultCreatePlayer &createPlayer); //!< \returns KI number of created player or 0 if name already exists
		void cleanVault(bool cleanAges);
		int getNumberOfPlayers(const uint8_t *uid);
		int getMaxPlayers(void) { return maxPlayers; }
		bool setAgeGuid(tAgeLinkStruct *link, uint32_t ownerKi);
	private:
		// first the new types
		struct tVmgr {
			tVmgr(uint32_t ki, uint32_t mgr, tNetSession* session) : ki(ki), mgr(mgr), session(session) { }
			uint32_t ki;
			uint32_t mgr;
			tNetSessionRef session;
		};
		
		typedef std::list<tVmgr> tVmgrList;
		
		/** send a vault message */
		void send(tvMessage &msg, tNetSession *u, uint32_t ki, uint32_t x = 0);
		
		/** finds the vault mgr with that data and updates its session ite
		    \returns the iterator of that vmgr or vmgrs.end() */
		tVmgrList::iterator findVmgr(tNetSession *u, uint32_t ki, uint32_t mgr);
		
		// high-level vault tasks
		
		/** creates the main vault nodes like AllPlayers and GlobalInbox */
		void createVault(void);
		
		/** ensures that the main vault nodes (AllPlayers, System) exist and are children of the AdminMGR */
		void checkMainNodes(void);
		
		/** searches the age by filename and creates it if necessary and wished, returns the ID of the age info node.
		    If the age should not be created and it is not found, returns 0 */
		uint32_t getAge(const tAgeInfoStruct &ageInfo, bool create = true);
		
		/** creates an age, returns the ID of the age info node */
		uint32_t createAge(const tAgeInfoStruct &ageInfo);
		
		/** finds this age in the list of ages this player owns/can visit. The ID of the folder the link is in is saved in linkedAgesFolder
		    \returns the ID of the link or 0 if not found */
		uint32_t findAgeLink(uint32_t ki, uint32_t ageInfoNode, uint32_t *linkedAgesFolder, bool visitedAge = false);
		
		/** gives the player a link to that age and adds the linking point
		    if noUpdate is true, the spawn point is only added if the link did not yet exist
		    \returns the ID of the link node or 0 if it's an invite and the player owns that age */
		uint32_t addAgeLinkToPlayer(uint32_t ki, uint32_t ageInfoNode, tSpawnPoint &spawnPoint, bool noUpdate = false, bool visitedAge = false);
		
		/** removes the link to that age from the given player */
		void removeAgeLinkFromPlayer(uint32_t ki, uint32_t ageInfoNode, bool visitedAge = false);
		
		/** adds/removes that player to/from the owners/visitors of this age */
		void addRemovePlayerToAge(uint32_t ageInfoNode, uint32_t ki, bool visitor = false, bool remove = false);
		
		// mid-level vault functions (doing broadcasts)
		//  the low-level functions are in tVaultDB
		
		/** get the ID of that node. If it doesn't exist, create it (with the given parent and saver) and broadcast the update */
		uint32_t getChildNodeBCasted(uint32_t saver, uint32_t parent, tvNode &node);
		
		/** create that node (with the given parent and saver) and broadcast the update */
		uint32_t createChildNodeBCasted(uint32_t saver, uint32_t parent, tvNode &node);
		
		/** add a ref and broadcast the update */
		void addRefBCasted(uint32_t saver, uint32_t parent, uint32_t son);
		
		// helper functions for broadcasting
		
		/** broadcast that this node has been changed, ecxlude the vmgr with the given data */
		void broadcastNodeUpdate(tvNode &node, uint32_t origKi = 0, uint32_t origMgr = 0);
		
		/** broadcast that this node ref has been changed (set remove=true if it has been removed),
		    ecxlude the vmgr with the given data. The passed tvNodeRef instance will be deleted.  */
		void broadcastNodeRefUpdate(tvNodeRef *ref, bool remove, uint32_t origKi = 0, uint32_t origMgr = 0);
		
		/** broadcast the online state of this player info node */
		void broadcastOnlineState(tvNode &node, uint32_t origKi = 0, uint32_t origMgr = 0);
		
		/** broadcast the message to all vmgrs interested in that node, except for the one with origKi and origMgr
		    (i.e. the one who did the change) */
		void broadcast(tvMessage &msg, uint32_t node, uint32_t origKi = 0, uint32_t origMgr = 0);
		
		/** Find oit if an age is private (used by GUID generation) */
		bool isAgePrivate(const tString &age) const;
		
		/** Generate the GUID for an age */
		bool generateGuid(uint8_t *guid, const tString &age, uint32_t ki);
	
		tVaultDB *vaultDB;
		tString vaultFolderName;
		uint32_t publicAgesFolder;
		
		int instanceMode;
		tString privateAges; // msut start and end with a comma
		tString ageFileDir;
		
		tLog log, logHtml;
		bool shortHtml;
		tUnet *net;
		
		// settings
		int maxPlayers;
		tString welcomeMsgTitle, welcomeMsgText;
		tString hoodName, hoodDesc;
		bool linkingRulesHack;
		
		// the list of vmgrs
		tVmgrList vmgrs;
	};


} //End alc namespace

#endif
