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

namespace alc {

	////DEFINITIONS
	class tVaultBackend {
	public:
		tVaultBackend(tUnet *net);
		~tVaultBackend(void);
		void unload(void);
		void load(void);
		
		void sendPlayerList(tmCustomVaultAskPlayerList &askPlayerList);
		void checkKi(tmCustomVaultCheckKi &checkKi);
		void updatePlayerStatus(tmCustomVaultPlayerStatus &status);
		void processVaultMsg(tvMessage &msg, tNetSession *u, U32 ki);
		void processVaultTask(tvMessage &msg, tNetSession *u, U32 ki, U32 x);
		void deletePlayer(tmCustomVaultDeletePlayer &deletePlayer);
		U32 createPlayer(tmCustomVaultCreatePlayer &createPlayer); //!< \returns KI number of created player or 0 if name already exists
		inline int getNumberOfPlayers(Byte *uid) {
			return vaultDB->getPlayerList(uid);
		}
	private:
		/** send a vault message */
		void send(tvMessage &msg, tNetSession *u, U32 ki, U32 x = 0);
		
		/** finds the vault mgr with that data and updates its session ite
		    \returns the number of that vmgr or -1 */
		int findVmgr(tNetSession *u, U32 ki, U32 mgr);
		
		/** creates the main vault nodes like AllPlayers and GlobalInbox */
		void createVault(void);
		
		/** searches the age by filename and creates it if necessary, returns the ID of the age info node */
		U32 getAge(tvAgeInfoStruct &ageInfo, bool create);
		
		/** creates an age, returns the ID of the age info node */
		U32 createAge(tvAgeInfoStruct &ageInfo);
		
		/** gives the player a link to that age and adds the linking point, returns the ID of the link node */
		U32 addAgeLinkToPlayer(U32 ki, U32 ageInfoNode, tvSpawnPoint &spawnPoint);
		
		/** adds that player to the owners of this age */
		void addOwnerToAge(U32 ageInfoNode, U32 ki);
		
		/** get the ID of that node. If it doesn't exist, create it (with the given parent and saver) and broadcast the update */
		U32 getNode(U32 saver, U32 parent, tvNode &node, bool bcast);
		
		/** create the node and add a ref to it. If told so, broadcast the update */
		U32 createNode(U32 saver, U32 parent, tvNode &node, bool bcast);
		
		/** add a ref and perhaps broadcast the update */
		void addRef(U32 saver, U32 parent, U32 son, bool bcast);
		
		/** broadcast that this node has been changed, ecxlude the vmgr with the given data */
		void broadcastNodeUpdate(tvNode &node, U32 origKi = 0, U32 origMgr = 0);
		
		/** broadcast that this node ref has been changed (set remove=true if it has been removed),
		    ecxlude the vmgr with the given data. The passed tvNodeRef instance will be deleted.  */
		void broadcastNodeRefUpdate(tvNodeRef *ref, bool remove, U32 origKi = 0, U32 origMgr = 0);
		
		/** broadcast the online state of this player node */
		void broadcastOnlineState(tvNode &node);
		
		/** broadcast the message to all vmgrs interested in that node, except for the one with origKi and origMgr
		    (i.e. the one who did the change) */
		void broadcast(tvMessage &msg, U32 node, U32 origKi = 0, U32 origMgr = 0);
	
		tVaultDB *vaultDB;
		Byte vaultFolderName[17];
		
		tGuidGen *guidGen;
		
		tLog *log, *logHtml;
		bool shortHtml;
		tUnet *net;
		
		// the list of vmgrs
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
		int nVmgrs;
		tVmgr **vmgrs;
	};


} //End alc namespace

#endif
