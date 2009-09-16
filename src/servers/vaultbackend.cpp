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

/* CVS tag - DON'T TOUCH*/
#define __U_VAULTBACKEND_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcnet.h>
#include <protocol/ext-protocol.h>

////extra includes
#include "vaultbackend.h"

#include <alcdebug.h>

namespace alc {

	const char *defaultWelcomeMsgTitle = "Shorah b'shehmtee";
	const char *defaultWelcomeMsgText = "Shorah b'shehmtee, this Shard is running the Alcugs server software.\nThanks for your support!\n\nWelcome to the new adventure, feel free to explore Er'cana or any other age. Be careful if you see new books, some explorers have found some Kortee'nea and other ancient technology in a secret room in Kirel DRC neighborhood, and they are starting to learn the art of writting.\n";

	////IMPLEMENTATION
	tVaultBackend::tVaultBackend(tUnet *net)
	{
		this->net = net;
		log = logHtml = lnull;
		vaultDB = NULL;
		ageInfos = NULL;
		load();
	}
	
	tVaultBackend::~tVaultBackend(void)
	{
		unload();
		if (vmgrs.size() > 0)
			log->log("ERR: The vault server is quitting and I still have %d vmgrs left\n", vmgrs.size());
	}
	
	void tVaultBackend::unload(void)
	{
		delete vaultDB;
		vaultDB = NULL;
		delete ageInfos;
		ageInfos = NULL;
		if (log != lnull) {
			delete log;
			log = lnull;
		}
		if (logHtml != lnull) {
			delete logHtml;
			logHtml = lnull;
		}
	}
	
	void tVaultBackend::load(void)
	{
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("vault.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			log = new tLog("vault.log", 4, 0);
		}
		var = cfg->getVar("vault.html.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			logHtml = new tLog("vault.html", 2, DF_HTML);
			var = cfg->getVar("vault.html.log.short");
			shortHtml = (var.isNull() || var.asByte()); // per default, it *is* short
		}
		
		var = cfg->getVar("vault.maxplayers");
		if (!var.isNull()) maxPlayers = var.asU32();
		else maxPlayers = 5;
		
		var = cfg->getVar("vault.hood.name");
		if (!var.isNull()) strncpy(hoodName, var.c_str(), 511);
		else strncpy(hoodName, "Alcugs hood", 511);
		var = cfg->getVar("vault.hood.desc");
		if (!var.isNull()) strncpy(hoodDesc, var.c_str(), 511);
		else strncpy(hoodDesc, "This is a hood on an Alcugs server", 511);
		
		var = cfg->getVar("vault.wipe.msg.title");
		if (!var.isNull()) strncpy(welcomeMsgTitle, var.c_str(), 511);
		else strncpy(welcomeMsgTitle, defaultWelcomeMsgTitle, 511);
		var = cfg->getVar("vault.wipe.msg");
		if (!var.isNull()) strncpy(welcomeMsgText, var.c_str(), 4095);
		else strncpy(welcomeMsgText, defaultWelcomeMsgText, 4095);
		
		var = cfg->getVar("vault.tmp.hacks.linkrules");
		linkingRulesHack = (!var.isNull() && var.asByte()); // disabled per default
		
		// load the list of private ages
		var = cfg->getVar("private_ages");
		if (var.isNull()) strcpy(privateAges, "AvatarCustomization,Personal,Nexus,BahroCave,BahroCave02,LiveBahroCaves,DniCityX2Finale,Cleft,Kadish,Gira,Garrison,Garden,Teledahn,Ercana,Minkata,Jalak");
		else strncpy(privateAges, var.c_str(), 1023);
		// load instance mode setting
		var = cfg->getVar("instance_mode");
		if (var.isNull()) instanceMode = 1;
		else instanceMode = var.asByte();
		if (instanceMode != 0 && instanceMode != 1) throw txBase(_WHERE("instance_mode must be 0 or 1 but is %d", instanceMode));
		// Load age files
		if (!ageInfos) ageInfos = new tAgeInfoLoader;
		
		log->log("Started VaultBackend (%s)\n", __U_VAULTBACKEND_ID);
		vaultDB = new tVaultDB(log);
		log->nl();
		log->flush();
		vaultDB->getVaultFolderName(vaultFolderName);
		DBG(5, "global vault folder name is %s\n", vaultFolderName);
		checkMainNodes();
	}
	
	void tVaultBackend::reload(void)
	{
		unload();
		load();
	}
	
	void tVaultBackend::createVault(void)
	{
		tvNode *node;
		// create admin mgr (the other nodes will be a child of this so that they are not lost). This is what the Vault Manager does
		//  on first login
		node = new tvNode(MType);
		node->type = KVNodeMgrAdminNode;
		U32 adminNode = vaultDB->createNode(*node);
		delete node;
		
		// create AllPlayersFolder
		node = new tvNode(MType | MInt32_1);
		node->type = KFolderNode;
		node->int1 = KAllPlayersFolder;
		vaultDB->createChildNode(KVaultID, adminNode, *node);
		delete node;
		
#if 0
		// create PublicAgesFolder (what is this used for?)
		node = new tvNode(MType | MInt32_1);
		node->type = KFolderNode;
		node->int1 = KPublicAgesFolder;
		vaultDB->createChildNode(KVaultID, adminNode, *node);
		delete node;
#endif
		
		// create System node
		node = new tvNode(MType);
		node->type = KSystem;
		U32 systemNode = vaultDB->createChildNode(KVaultID, adminNode, *node);
		delete node;
		
		// create GlobalInboxFolder as a child of the system node
		node = new tvNode(MType | MInt32_1);
		node->type = KFolderNode;
		node->int1 = KGlobalInboxFolder;
		U32 globalInboxNode = vaultDB->createChildNode(KVaultID, systemNode, *node);
		delete node;
		
		// create welcome message as a child of the global inbox
		node = new tvNode(MType | MStr64_1 | MBlob1);
		node->type = KTextNoteNode;
		node->str1.writeStr(welcomeMsgTitle);
		node->blob1Size = strlen(welcomeMsgText)+1;
		node->blob1 = (Byte *)malloc(node->blob1Size * sizeof(Byte));
		if (node->blob1 == NULL) throw txNoMem(_WHERE("NoMem"));
		strcpy((char *)node->blob1, welcomeMsgText);
		vaultDB->createChildNode(KVaultID, globalInboxNode, *node);
		delete node;
	}
	
	void tVaultBackend::checkMainNodes(void)
	{
		tvNode *node;
		// get AdminMGR
		node = new tvNode(MType);
		node->type = KVNodeMgrAdminNode;
		U32 adminNode = vaultDB->findNode(*node);
		if (!adminNode) {
			createVault();
			adminNode = vaultDB->findNode(*node);
		}
		delete node;
		if (!adminNode) throw txDatabaseError(_WHERE("No admin node found"));
		
		// find the two main nodes and ensure they are connected to this one
		// AllPlayersFolder
		node = new tvNode(MType | MInt32_1);
		node->type = KFolderNode;
		node->int1 = KAllPlayersFolder;
		getChildNodeBCasted(KVaultID, adminNode, *node);
		delete node;
		
		// System node
		node = new tvNode(MType);
		node->type = KSystem;
		getChildNodeBCasted(KVaultID, adminNode, *node);
		delete node;
	}
	
	void tVaultBackend::send(tvMessage &msg, tNetSession *u, U32 ki, U32 x)
	{
		msg.print(logHtml, /*clientToServer:*/false, u, shortHtml, ki);
		tmVault vaultMsg(u, ki, x, msg.task, &msg);
		net->send(vaultMsg);
	}
	
	void tVaultBackend::sendPlayerList(tmCustomVaultAskPlayerList &askPlayerList)
	{
		tmCustomVaultPlayerList list(askPlayerList.getSession(), askPlayerList.x, askPlayerList.sid, askPlayerList.uid);
		list.numberPlayers = vaultDB->getPlayerList(askPlayerList.uid, &list.players);
		net->send(list);
	}
	
	void tVaultBackend::checkKi(tmCustomVaultCheckKi &checkKi)
	{
		char avatar[256];
		Byte status;
		status = vaultDB->checkKi(checkKi.ki, checkKi.uid, avatar);
		tmCustomVaultKiChecked checked(checkKi.getSession(), checkKi.ki, checkKi.x, checkKi.sid, checkKi.uid, status, avatar);
		net->send(checked);
	}
	
	void tVaultBackend::updatePlayerStatus(tmCustomVaultPlayerStatus &status)
	{
		if (status.state > 2) throw txProtocolError(_WHERE("recieved invalid online state of %d", status.state));
	
		// update (when the palyer is online) or remove (when he's offline) all mgrs using this KI
		tVmgrList::iterator it = vmgrs.begin();
		while (it != vmgrs.end()) {
			if (it->ki == status.ki) {
				if (status.state) it->session = status.getSession()->getIte();
				else {
					it = vmgrs.erase(it);
					log->log("WARN: Player with KI %d just went offline but still had a vmgr registered... removing it\n", status.ki);
					continue; // we already incremented
				}
			}
			++it;
		}
		
		tvNode *node;
		tvNode **nodes;
		int nNodes;
		
		/* status.state can have three values:
		0 => player is offline, remove his vmgrs, update both nodes and tell everyone
		1 => player is online, update both nodes
		2 => player just left, but he will come back so keep his vmgrs, update only the online time and don't send the bcast to this player */
		
		// update online time
		if (status.onlineTime > 0) {
			vaultDB->fetchNodes(&status.ki, 1, &nodes, &nNodes);
			if (nNodes != 1) throw txProtocolError(_WHERE("update for non-existing player"));
			node = *nodes; // save some *
			node->uInt2 += status.onlineTime;
			vaultDB->updateNode(*node);
			if (status.state == 2) // exclude player from bcast revievers
				broadcastNodeUpdate(*node, status.ki, status.ki);
			else
				broadcastNodeUpdate(*node);
			// free stuff
			delete node;
			free((void *)nodes);
		}
		
		// find and update the info node
		if (status.state == 0 || status.state == 1) { // don't update when status.state is 2 (see above)
			node = new tvNode(MType | MOwner);
			node->type = KPlayerInfoNode;
			node->owner = status.ki;
			U32 infoNode = vaultDB->findNode(*node);
			delete node;
			if (!infoNode) throw txUnet(_WHERE("KI %d doesn't have a info node?!?", status.ki));
			vaultDB->fetchNodes(&infoNode, 1, &nodes, &nNodes);
			assert(nNodes == 1);
			node = *nodes; // same some *
			node->str1 = status.age;
			node->str2 = status.serverGuid;
			node->int1 = status.state;
			vaultDB->updateNode(*node);
			broadcastOnlineState(*node);
			// free stuff
			delete node;
			free((void *)nodes);
		}
		
		log->log("Status update for KI %d: %d\n", status.ki, status.state);
		log->flush();
		
		if (!linkingRulesHack || status.state != 1) return;
		// linking rules hack
		// this hack was originally written by a'moaca' for the unet3 vault server. I just ported it to unet3+. Unlike the original hack,
		//  this one also creates the age if necessary as otherwise the first link there would be done without the hack
		// Here's the original comment:
			//NOTE I have not done enough testing to consider this hack safe,
			//     also the problem should be solved on the unet3+ game servers
			//     since the new game servers are years away, this hack can be a temporany
			//     solution for some people.
		if (status.age == "Ahnonay" || status.age == "Neighborhood02" || status.age == "Myst") {
			Byte guid[8];
			alcAscii2Hex(guid, status.serverGuid.c_str(), 8);
			tvAgeInfoStruct ageInfo(status.age.c_str(), guid);
			tvSpawnPoint spawnPoint("Default", "LinkInPointDefault");
			log->log("Linking rule hack: adding link to %s to player %d\n", ageInfo.filename.c_str(), status.ki);
			U32 ageInfoNode = getAge(ageInfo); // create if necessary
			addAgeLinkToPlayer(status.ki, ageInfoNode, spawnPoint, /*noUpdate*/true);
		}
	}
	
	tVaultBackend::tVmgrList::iterator tVaultBackend::findVmgr(tNetSession *u, U32 ki, U32 mgr)
	{
		for (tVmgrList::iterator it = vmgrs.begin(); it != vmgrs.end(); ++it) {
			if (it->ki == ki && it->mgr == mgr) {
				it->session = u->getIte();
				return it;
			}
		}
		return vmgrs.end();
	}
	
	void tVaultBackend::processVaultMsg(tvMessage &msg, tNetSession *u, U32 ki)
	{
		assert(!msg.task);
		msg.print(logHtml, /*clientToServer:*/true, u, shortHtml, ki);
		
		// have everything on the stack as we can safely throw exceptions then
		U32 nodeType = -1, playerKi = -1, nodeSon = -1, nodeParent = -1, seenFlag = -1, rcvPlayer = -1;
		U16 tableSize = 0;
		tMBuf table;
		tvNode *savedNode = NULL;
		tvNodeRef *savedNodeRef = NULL;
		const char *ageName = NULL;
		const Byte *ageGuid = NULL;
		
		// read and verify the general vault items
		for (tvMessage::tItemList::iterator it = msg.items.begin(); it != msg.items.end(); ++it) {
			tvItem *itm = *it;
			switch (itm->id) {
				case 0: // GenericValue.Int: used in VaultManager, must always be the same
					if (itm->asInt() != 0xC0AB3041)
						throw txProtocolError(_WHERE("a vault item with ID 0 must always have a value of 0xC0AB3041 but I got 0x%08X", itm->asInt()));
					break;
				case 1: // GenericValue.Int: node type
					nodeType = itm->asInt();
					break;
				case 2: // GenericValue.Int: unique id [ki number]
					playerKi = itm->asInt();
					break;
				case 4: // GenericValue.Int: reciever of a VSendNode
					rcvPlayer = itm->asInt();
					break;
				case 5: // VaultNode: a single vault node
					savedNode = itm->asNode(); // we don't have to free it, tvMessage does that
					break;
				case 7: // VaultNodeRef: a single vault node ref
					savedNodeRef = itm->asNodeRef(); // we don't have to free it, tvMessage does that
					break;
				case 9: // GenericValue.Int: Node Index / Son of a NodeRef
					nodeSon = itm->asInt();
					break;
				case 10: // GenericStream: Stream containing a table of ints
				{
					if (itm->type != plCreatableStream) throw txProtocolError(_WHERE("a vault item with id 10 must always be a plCreatableStream"));
					tMBuf *buf = static_cast<tvCreatableStream *>(itm->data)->getData();
					tableSize = buf->getU16();
					table.write(buf->read(tableSize*4), tableSize*4);
					bool eof = buf->eof();
					delete buf;
					if (!eof) throw txProtocolError(_WHERE("the stream is too long"));
					table.rewind();
					break;
				}
				case 13: // GenericValue.Int: Parent of a NodeRef
					nodeParent = itm->asInt();
					break;
				case 16: // GenericValue.Int: must always be 0 or 1 (1 seen in FindNode)
					if (itm->asInt() != 0 && itm->asInt() != 1)
						throw txProtocolError(_WHERE("a vault item with ID 16 must always have a value of 0 or 1 but I got %d", itm->asInt()));
					break;
				case 19: // GenericValue.Int: Set Seen flag
					seenFlag = itm->asInt();
					break;
				case 20: // GenericValue.Int: must always be the -1 or 0 (0 seen in NegotiateManifest when client gets Ahnonay SDL to update current Sphere)
					if (itm->asInt() != (U32)-1 && itm->asInt() != 0)
						throw txProtocolError(_WHERE("a vault item with ID 20 must always have a value of -1 or 0 but I got %d", itm->asInt()));
					break;
				case 21: // GenericValue.UruString: age name
					ageName = itm->asString();
					break;
				case 22: // ServerGuid: age guid
					ageGuid = itm->asGuid();
					break;
				// these are not sent to servers
				case 6: // GenericStream: a stream of Vault Nodes
				case 11: // GenericValue.Int: new Node Index (saveNode)
				case 14: // GenericStream: stream of manifests
				case 15: // GenericStream: stream of NodeRefs
				case 23: // GenericValue.String: Vault folder
				case 24: // GenericValue.Timestamp of found node
				case 25: // GenericValue.Int: number of vault nodes
				case 27: // GenericValue.String: current age of a player
				case 28: // GenericValue.String: current age GUID of a player
				case 29: // online status of a player
				case 31: // GenericValue.Int: EOF of a FetchNode
				default:
					throw txProtocolError(_WHERE("vault item has invalid id %d", itm->id));
			}
		}
		
		if (msg.cmd != VConnect && findVmgr(u, ki, msg.vmgr) == vmgrs.end())
			throw txProtocolError(_WHERE("player did not yet send his VConnect"));
		switch (msg.cmd) {
			case VConnect:
			{
				if (nodeType < 0)
					throw txProtocolError(_WHERE("got a VConnect where node type has not been set"));
				log->log("Vault Connect request for %d (Type: %d)\n", ki, nodeType);
				log->flush();
				U32 mgr;
				
				// we can be sure that the vault has already been initialized (main folders and the welcome message are created) since
				// a player has to be created to get here
				
				tvNode mgrNode(MType);
				mgrNode.type = nodeType;
				if (nodeType == KVNodeMgrPlayerNode) { // player node
					if (playerKi < 0) throw txProtocolError(_WHERE("VConnect for node type 2 must have playerKi set"));
					mgr = playerKi;
					if (mgr != ki)
						throw txProtocolError(_WHERE("Player with KI %d wants to VConnect as %d\n", ki, mgr));
					// create reply
					tvMessage reply(msg);
					reply.items.push_back(new tvItem(/*id*/2, /*mgr node id*/mgr));
					reply.items.push_back(new tvItem(/*id*/23, /*folder name*/vaultFolderName));
					send(reply, u, ki);
				}
				else if (nodeType == KVNodeMgrAgeNode) { // age node
					if (!ageName || !ageGuid) throw txProtocolError(_WHERE("VConnect for node type 3 must have ageGuid and ageName set"));
					mgrNode.flagB |= MStr64_1;
					mgrNode.str1.writeStr(alcGetStrGuid(ageGuid));
					mgr = vaultDB->findNode(mgrNode, /*create*/true);
					// create and send the reply
					tvMessage reply(msg);
					reply.items.push_back(new tvItem(/*id:*/2, /*mgr node id*/mgr));
					reply.items.push_back(new tvItem(/*id*/21, /*age name*/ageName));
					reply.items.push_back(new tvItem(/*id*/23, /*folder name*/vaultFolderName));
					send(reply, u, ki);
				}
				else if (nodeType == KVNodeMgrAdminNode) { // admin node
					mgr = vaultDB->findNode(mgrNode, /*create*/true); // creating it is needed for vaults which were created with an old vault server
					// create and send the reply
					tvMessage reply(msg);
					reply.items.push_back(new tvItem(/*id:*/1, /*node type*/5u));
					reply.items.push_back(new tvItem(/*id*/2, /*mgr node id*/mgr));
					reply.items.push_back(new tvItem(/*id*/23, /*folder name*/vaultFolderName));
					send(reply, u, ki);
				}
				else // wrong or no node type at all
					throw txProtocolError(_WHERE("Connect request for unknown node type %d from KI %d\n", nodeType));
				// now let's see where we save this... first look if we already have this one registered
				tVmgrList::iterator it = findVmgr(u, ki, mgr);
				if (it != vmgrs.end()) // it is already registered, and findVmgr updated the session, so we have nothing to do
					break;
				// if that's not the case, add a new one
				vmgrs.push_back(tVmgr(ki, mgr, u->getIte()));
				break;
			}
			case VDisconnect:
			{
				if (nodeType < 0)
					throw txProtocolError(_WHERE("got a VDisconnect where the node type has not been set"));
				log->log("Vault Disconnect request for %d (Type: %d)\n", ki, nodeType);
				log->flush();
				if (nodeType != KVNodeMgrAdminNode) { // VaultManager sometimes leaves too fast, and it doesn't need the VDisconnect reply
					// send reply
					tvMessage reply(msg);
					send(reply, u, ki);
				}
				// remove vmgr
				tVmgrList::iterator it = findVmgr(u, ki, msg.vmgr);
				vmgrs.erase(it);
				break;
			}
			case VAddNodeRef:
			{
				if (!savedNodeRef) throw txProtocolError(_WHERE("got a VAddNodeRef without a node ref attached"));
				log->log("Vault Add Node Ref from %d to %d for %d\n", savedNodeRef->parent, savedNodeRef->child, ki);
				log->flush();
				
				// check if both nodes exist
				if (!vaultDB->checkNode(savedNodeRef->parent) || !vaultDB->checkNode(savedNodeRef->child))
					throw txProtocolError(_WHERE("One of the nodes I should connect (%d -> %d) doesn't exist", savedNodeRef->parent, savedNodeRef->child));
				
				if (!vaultDB->addNodeRef(*savedNodeRef)) return; // ignore duplicates
				
				// broadcast the change
				broadcastNodeRefUpdate(new tvNodeRef(*savedNodeRef), /*remove:*/false, ki, msg.vmgr);
				break;
			}
			case VRemoveNodeRef:
			{
				if (nodeSon < 0 || nodeParent < 0)
					throw txProtocolError(_WHERE("got a VRemoveNodeRef where parent or son have not been set"));
				log->log("Vault Remove Node Ref from %d to %d for %d\n", nodeParent, nodeSon, ki);
				log->flush();
				vaultDB->removeNodeRef(nodeParent, nodeSon);
				
				// broadcast the change
				broadcastNodeRefUpdate(new tvNodeRef(0, nodeParent, nodeSon), /*remove:*/true, ki, msg.vmgr);
				break;
			}
			case VNegotiateManifest:
			{
				if (tableSize != 1)
					throw txProtocolError(_WHERE("Getting %d manifests at once is not supported\n", tableSize));
				U32 mgr = table.getU32();
				log->log("Vault Negoiate Manifest (MGR: %d) for %d\n", mgr, ki);
				log->flush();
				tvManifest **mfs;
				tvNodeRef **ref;
				int nMfs, nRef;
				vaultDB->getManifest(mgr, &mfs, &nMfs, &ref, &nRef);
				
				// create reply
				tvMessage reply(msg);
				reply.compress = true;
				reply.items.push_back(new tvItem(new tvCreatableStream(/*id*/14, reinterpret_cast<tvBase **>(mfs), nMfs))); // tvMessage will delete it for us
				reply.items.push_back(new tvItem(new tvCreatableStream(/*id*/15, reinterpret_cast<tvBase **>(ref), nRef))); // tvMessage will delete it for us
				send(reply, u, ki);
				
				// free stuff
				for (int i = 0; i < nMfs; ++i) delete mfs[i];
				free((void *)mfs);
				for (int i = 0; i < nRef; ++i) delete ref[i];
				free((void *)ref);
				break;
			}
			case VSaveNode:
			{
				if (!savedNode) throw txProtocolError(_WHERE("got a save node request without the node attached"));
				
				if (savedNode->index < KVaultID) {
					U32 oldIndex = savedNode->index;
					U32 newIndex = vaultDB->createNode(*savedNode);
					log->log("Vault Save Node (created new node %d) for %d\n", newIndex, ki);
					log->flush();
					
					// reply to the sender
					tvMessage reply(msg);
					reply.items.push_back(new tvItem(/*id*/9, /*old node index*/oldIndex));
					reply.items.push_back(new tvItem(/*id*/11, /*old node index*/newIndex));
					send(reply, u, ki);
				}
				else {
					vaultDB->updateNode(*savedNode);
					log->log("Vault Save Node (updated node %d) for %d\n", savedNode->index, ki);
					log->flush();
					
					// create the reply to the sender
					tvMessage reply(msg);
					reply.items.push_back(new tvItem(/*id*/9, /*old node index*/savedNode->index));
					send(reply, u, ki);
					// and the broadcast
					broadcastNodeUpdate(*savedNode, ki, msg.vmgr);
				}
				break;
			}
			case VFindNode:
			{
				if (!savedNode) throw txProtocolError(_WHERE("got a find node request without the node attached"));
				log->log("Vault Find Node (looking for node %d) for %d\n", savedNode->index, ki);
				log->flush();
				tvManifest mfs;
				bool create = (savedNode->type == KPlayerInfoNode || savedNode->type == KFolderNode || savedNode->type == KPlayerInfoListNode || savedNode->type == KSDLNode || savedNode->type == KAgeInfoNode);
				// It's necessary to allow creating these
				// FolderNode and PlayerInfoNode are requested when fetching a new player or an age
				// SDLNode is requested when fetching an age
				// AgeInfoNode is requested for AvatarCustomization and the Cleft (and perhaps more) for new players
				// PlayerInfoNode is requested for the saver of a KI messages and must be recreated if the player was deleted
				if (!vaultDB->findNode(*savedNode, create, &mfs))
					throw txProtocolError(_WHERE("got a VFindNode but can't find the node"));
				// create and send the reply
				tvMessage reply(msg);
				reply.items.push_back(new tvItem(/*id*/9, /*old node index*/mfs.id));
				reply.items.push_back(new tvItem(/*id*/24, /*timestamp*/(double)mfs.time));
				send(reply, u, ki);
			}
			case VFetchNode:
			{
				if (tableSize <= 0) break;
				log->log("Vault Fetch Node (fetching %d nodes) for %d\n", tableSize, ki);
				log->flush();
				tvNode **nodes;
				int nNodes;
				vaultDB->fetchNodes(table, tableSize, &nodes, &nNodes);
				
				// split the message into several ones to avoid it getting too big
				tMBuf buf;
				U32 num = 0;
				for (int i = 0; i < nNodes; ++i) {
					buf.put(*nodes[i]);
					++num;
					if (buf.size() > 128000 || i == (nNodes-1)) { // if size is already big enough or this is the last one
						// create reply
						bool eof = (i == (nNodes-1));
						tvMessage reply(msg);
						reply.compress = true;
						reply.items.push_back(new tvItem(new tvCreatableStream(/*id*/6, buf))); // tvMessage will delete it for us
						reply.items.push_back(new tvItem(/*id*/25, /*number of nodes*/num));
						if (eof) reply.items.push_back(new tvItem(/*id*/31, 0u)); // EOF mark (this is the last packet of nodes)
						send(reply, u, ki);
						buf.clear();
						num = 0;
					}
				}
				
				// free stuff
				for (int i = 0; i < nNodes; ++i) delete nodes[i];
				free((void *)nodes);
				break;
			}
			case VSendNode:
			{
				if (rcvPlayer < 0 || !savedNode) throw txProtocolError(_WHERE("Got a VSendNode without the reciever or the node"));
				log->log("Sending node %d from player %d to %d\n", savedNode->index, ki, rcvPlayer);
				log->flush();
				tvNode *node;
				tvNode **nodes;
				int nNodes;
				
				// check if node to send exists
				if (!vaultDB->checkNode(savedNode->index))
					throw txProtocolError(_WHERE("Not sending non-existing node %d", savedNode->index));
				
				// check if reciever exists
				U32 reciever = rcvPlayer;
				vaultDB->fetchNodes(&reciever, 1, &nodes, &nNodes);
				if (nNodes > 0) delete *nodes;
				free(nodes);
				if (nNodes == 0) {
					log->log("ERR: I should send a message to the non-existing player %d\n", rcvPlayer);
					log->flush();
					break;
				}
				
				// find the sender's info node
				node = new tvNode(MType | MOwner);
				node->type = KPlayerInfoNode;
				node->owner = ki;
				U32 infoNode = vaultDB->findNode(*node);
				delete node;
				if (!infoNode) throw txUnet(_WHERE("Couldn't find a players info node?!?"));
				
				// find and create the InboxFolder
				node = new tvNode(MType | MOwner | MInt32_1);
				node->type = KFolderNode;
				node->owner = rcvPlayer;
				node->int1 = KInboxFolder;
				U32 inbox = getChildNodeBCasted(rcvPlayer, rcvPlayer, *node);
				delete node;
				
				// add what was sent to the inbox
				addRefBCasted(ki, inbox, savedNode->index);
				break;
			}
			case VSetSeen:
			{
				if (nodeParent < 0 || nodeSon < 0 || seenFlag < 0)
					throw txProtocolError(_WHERE("parent, son and seen flag must be set for a VSetSeen"));
				log->log("Vault Set Seen (parent: %d, son: %d, seen: %d) for %d\n", nodeParent, nodeSon, seenFlag, ki);
				vaultDB->setSeen(nodeParent, nodeSon, seenFlag);
				break;
			}
			case VOnlineState: // not sent to servers
			default:
				throw txProtocolError(_WHERE("Unknown vault command 0x%02X (%s)\n", msg.cmd, alcVaultGetCmd(msg.cmd)));
		}
	}
	
	void tVaultBackend::processVaultTask(tvMessage &msg, tNetSession *u, U32 ki, U32 x)
	{
		assert(msg.task);
		msg.print(logHtml, /*clientToServer:*/true, u, shortHtml, ki);
		
		tvAgeLinkStruct *ageLink = NULL;
		const Byte *ageGuid = NULL;
		const char *ageName = NULL;
		
		// read and verify the general vault items
		for (tvMessage::tItemList::iterator it = msg.items.begin(); it != msg.items.end(); ++it) {
			tvItem *itm = *it;
			switch (itm->id) {
				case 11: // AgeLinkStruct: an age link
					ageLink = itm->asAgeLink(); // we don't have to free it, tvMessage does that
					break;
				case 12: // GenericValue.UruString: the filename of the age to remove
					ageName = itm->asString();
					break;
				case 13: // ServerGuid: the GUID of the age the invite should be removed from
					ageGuid = itm->asGuid();
					break;
				// these are not sent to servers
				case 1: // the ID of the created age link node
				default:
					throw txProtocolError(_WHERE("vault item has invalid id %d", itm->id));
			}
		}
		
		// now process the task
		switch (msg.cmd) {
			case TRegisterOwnedAge:
			{
				if (ki != msg.vmgr)
					throw txProtocolError(_WHERE("the vmgr of a TRegisterOwnedAge task must be the player's KI, but %d != %d", ki, msg.vmgr));
				if (!ageLink) throw txProtocolError(_WHERE("the age link must be set for a TRegisterOwnedAge"));
				log->log("TRegisterOwnedAge (age filename: %s) from %d\n", ageLink->ageInfo.filename.c_str(), msg.vmgr);
				
				// if necessary, generate the guid
				Byte zeroGuid[8];
				memset(zeroGuid, 0, 8);
				if (memcmp(ageLink->ageInfo.guid, zeroGuid, 8) == 0) {
					// this happens for the Watcher's Guild link which is created "on the fly" as Relto expects it, and
					// for the links to the 4 Ahnonay spheres which are created when linking to the Cathedral
					if (!generateGuid(ageLink->ageInfo.guid, ageLink->ageInfo.filename.c_str(), msg.vmgr))
						throw txProtocolError(_WHERE("could not generate GUID"));
				}
				
				// now find the age info node of the age we're looking for
				U32 ageInfoNode = getAge(ageLink->ageInfo);
				// we got it - now let's add it to the player
				U32 linkNode = addAgeLinkToPlayer(msg.vmgr, ageInfoNode, ageLink->spawnPoint);
				// and add that player to the age owner's list
				addRemovePlayerToAge(ageInfoNode, msg.vmgr);
				
				tvMessage reply(msg);
				reply.items.push_back(new tvItem(/*id*/1, linkNode));
				send(reply, u, ki, x);
				break;
			}
			case TUnRegisterOwnedAge:
			{
				if (ki != msg.vmgr)
					throw txProtocolError(_WHERE("the vmgr of a TRegisterOwnedAge task must be the player's KI, but %d != %d", ki, msg.vmgr));
				if (!ageName) throw txProtocolError(_WHERE("the age name must be set for a TUnRegisterOwnedAge"));
				log->log("TUnRegisterOwnedAge (age filename: %s) from %d\n", ageName, msg.vmgr);
				
				// generate the GUID
				Byte guid[8];
				if (!generateGuid(guid, ageName, msg.vmgr))
					throw txProtocolError(_WHERE("could not generate GUID"));
				
				// find age info node
				tvAgeInfoStruct ageInfo(ageName, guid);
				U32 ageInfoNode = getAge(ageInfo, /*create*/false);
				if (!ageInfoNode) throw txProtocolError(_WHERE("I should remove a non-existing owned age"));
				// remove the link from the player
				removeAgeLinkFromPlayer(msg.vmgr, ageInfoNode);
				// remove the player from that age
				addRemovePlayerToAge(ageInfoNode, msg.vmgr, /*visitor*/false, /*remove*/true);
				
				tvMessage reply(msg);
				send(reply, u, ki, x);
				break;
			}
			case TRegisterVisitAge:
			{
				if (!ageLink) throw txProtocolError(_WHERE("the age link must be set for a TRegisterVisitAge"));
				log->log("TRegisterVisitAge (age filename: %s) from %d to %d\n", ageLink->ageInfo.filename.c_str(), ki, msg.vmgr);
				
				// msg.vmgr is the reciever's KI, ki the sender's one
				
				// now find the age info node of the age we're looking for
				U32 ageInfoNode = getAge(ageLink->ageInfo, /*create*/false);
				if (!ageInfoNode) throw txProtocolError(_WHERE("I should send an invite for a non-existing age"));
				// we got it - now let's add it to the player
				U32 linkNode = addAgeLinkToPlayer(msg.vmgr, ageInfoNode, ageLink->spawnPoint, /*noUpdate*/true, /*visitedAge*/true);
				// and add that player to the age visitor's list
				if (linkNode) // the player does not own that age
					addRemovePlayerToAge(ageInfoNode, msg.vmgr, /*visitor*/true);
				
				tvMessage reply(msg);
				reply.items.push_back(new tvItem(/*id*/1, linkNode));
				send(reply, u, ki, x);
				break;
			}
			case TUnRegisterVisitAge:
			{
				if (!ageGuid) throw txProtocolError(_WHERE("the age GUID must be set for a TRegisterVisitAge"));
				log->log("TUnRegisterVisitAge from %d\n", ki);
				
				// msg.vmgr is the reciever's KI, ki the sender's one
				
				// now find the age info node of the age we're looking for
				tvAgeInfoStruct ageInfo("", ageGuid); // filename is unknown
				U32 ageInfoNode = getAge(ageInfo, /*create*/false);
				if (!ageInfoNode) throw txProtocolError(_WHERE("I should remove an invite for a non-existing age"));
				// remove the link from the player
				removeAgeLinkFromPlayer(msg.vmgr, ageInfoNode, /*visitedAge*/true);
				// remove the player from that age
				addRemovePlayerToAge(ageInfoNode, msg.vmgr, /*visitor*/true, /*remove*/true);
				
				tvMessage reply(msg);
				send(reply, u, ki, x);
				break;
			}
			default:
				throw txProtocolError(_WHERE("Unknown vault task 0x%02X (%s)\n", msg.cmd, alcVaultGetTask(msg.cmd)));
		}
	}
	
	U32 tVaultBackend::getAge(tvAgeInfoStruct &ageInfo, bool create)
	{
		tvNode *node;
		// search for the age
		node = new tvNode(MType | MStr64_4);
		node->type = KAgeInfoNode;
		if (ageInfo.filename.size() > 0) {
			node->flagB |= MStr64_1;
			node->str1 = ageInfo.filename;
		}
		node->str4.writeStr(alcGetStrGuid(ageInfo.guid));
		U32 ageInfoNode = vaultDB->findNode(*node);
		delete node;
		if (ageInfoNode) // we got it!
			return ageInfoNode;
		
		if (!create) return 0;
		// we have to create it
		return createAge(ageInfo);
	}
	
	U32 tVaultBackend::createAge(tvAgeInfoStruct &ageInfo)
	{
		tvNode *node;
		// first create the age mgr node
		node = new tvNode(MType | MStr64_1);
		node->type = KVNodeMgrAgeNode;
		node->str1.writeStr(alcGetStrGuid(ageInfo.guid));
		U32 ageMgrNode = vaultDB->createNode(*node);
		delete node;
		
		// now create the age info node as child of the age mgr
		node = new tvNode(MType | MUInt32_1 | MStr64_1 | MStr64_2 | MStr64_3 | MStr64_4 | MText_1);
		node->type = KAgeInfoNode;
		node->uInt1 = ageMgrNode;
		node->str1 = ageInfo.filename;
		node->str2 = ageInfo.instanceName;
		node->str3 = ageInfo.userDefName;
		node->str4.writeStr(alcGetStrGuid(ageInfo.guid));
		node->text1 = ageInfo.displayName;
		U32 ageInfoNode = vaultDB->createChildNode(ageMgrNode, ageMgrNode, *node);
		delete node;
		
		return ageInfoNode;
	}
	
	U32 tVaultBackend::findAgeLink(U32 ki, U32 ageInfoNode, U32 *linkedAgesFolder, bool visitedAge)
	{
		tvNode *node;
		// find (and create if necessary) AgesIOwnFolder
		node = new tvNode(MType | MOwner | MInt32_1);
		node->type = KFolderNode;
		node->owner = ki;
		node->int1 = visitedAge ? KAgesICanVisitFolder : KAgesIOwnFolder;
		*linkedAgesFolder = getChildNodeBCasted(ki, ki, *node);
		delete node;
		
		// now get all child nodes and look for our age
		tvNodeRef **ref;
		int nRef;
		U32 ageLinkNode = 0;
		vaultDB->getReferences(*linkedAgesFolder, &ref, &nRef);
		for (int i = 0; i < nRef; ++i) {
			if (ref[i]->child == ageInfoNode) ageLinkNode = ref[i]->parent;
			delete ref[i];
		}
		free(ref);
		return ageLinkNode;
	}
	
	U32 tVaultBackend::addAgeLinkToPlayer(U32 ki, U32 ageInfoNode, tvSpawnPoint &spawnPoint, bool noUpdate, bool visitedAge)
	{
		U32 linkedAgesFolder;
		U32 ageLinkNode = findAgeLink(ki, ageInfoNode, &linkedAgesFolder); // first check if the player owns that age
		
		if (visitedAge) {
			if (ageLinkNode) // the player owns that age, don't send an invite
				return 0;
			ageLinkNode = findAgeLink(ki, ageInfoNode, &linkedAgesFolder, /*visitedAge*/true);
		}
		
		// new spawn point info
		char spawnPnt[512];
		sprintf(spawnPnt, "%s:%s:%s;", spawnPoint.title.c_str(), spawnPoint.name.c_str(), spawnPoint.cameraStack.c_str());
		
		// if the link node exists, fetch and update it, otherwise, create it
		if (ageLinkNode) {
			if (noUpdate) // we're told not to update it, so let's go
				return ageLinkNode;
			tvNode **nodes;
			int nNodes;
			vaultDB->fetchNodes(&ageLinkNode, 1, &nodes, &nNodes);
			if (nNodes != 1) throw txUnet(_WHERE("cant find age link even though I just found the reference?!?"));
			tvNode *node = *nodes; // saves some * ;-)
			assert(node->type == KAgeLinkNode);
			assert(node->blob1 != NULL); // otherwise we would have to add a terminator...
			node->flagB = MBlob1; // only save what is really necessary, so unset all flags
			node->flagC = 0;
			// add spawn point info
			node->blob1Size += strlen(spawnPnt);
			node->blob1 = (Byte *)realloc(node->blob1, node->blob1Size*sizeof(Byte));
			if (node->blob1 == NULL) throw txNoMem(_WHERE("NoMem"));
			strcat((char *)node->blob1, spawnPnt);
			// update it and broadcast the update
			vaultDB->updateNode(*node);
			broadcastNodeUpdate(*node);
			// free stuff
			delete node;
			free(nodes);
		}
		else {
			tvNode *node = new tvNode(MType | MOwner | MInt32_1 | MInt32_2 | MBlob1);
			node->type = KAgeLinkNode;
			node->owner = ki;
			node->int1 = 1; // locked status: 0 = unlocked, 1 = locked
			node->int2 = 0; // volatile status: 0 = non-volatile, 1 = volatile
			// add spawn point info
			node->blob1Size = strlen(spawnPnt)+1; // one for the terminator
			node->blob1 = (Byte *)malloc(node->blob1Size*sizeof(Byte));
			if (node->blob1 == NULL) throw txNoMem(_WHERE("NoMem"));
			strcpy((char *)node->blob1, spawnPnt);
			// insert the age link node as child of the AgesIOwnFolder
			ageLinkNode = createChildNodeBCasted(ki, linkedAgesFolder, *node);
			// create link age link node -> age info node
			addRefBCasted(ki, ageLinkNode, ageInfoNode);
			// free stuff
			delete node;
		}
		return ageLinkNode;
	}
	
	void tVaultBackend::removeAgeLinkFromPlayer(U32 ki, U32 ageInfoNode, bool visitedAge)
	{
		// find the age link
		U32 linkedAgesFolder;
		U32 ageLinkNode = findAgeLink(ki, ageInfoNode, &linkedAgesFolder, visitedAge);
		if (!ageLinkNode) return; // the player does not have a link to this age
		
		// and now remove that node and broadcast the removal
		vaultDB->removeNodeRef(linkedAgesFolder, ageLinkNode, /*cautious*/false); // this will remove the node if this was the only ref
		broadcastNodeRefUpdate(new tvNodeRef(0, linkedAgesFolder, ageLinkNode), /*removal*/true);
	}
	
	void tVaultBackend::addRemovePlayerToAge(U32 ageInfoNode, U32 ki, bool visitor, bool remove)
	{
		tvNode *node;
		// find and create age owners/visitors folder
		node = new tvNode(MType | MOwner | MInt32_1);
		node->type = KPlayerInfoListNode;
		node->owner = ageInfoNode;
		node->int1 = visitor ? KCanVisitFolder : KAgeOwnersFolder;
		U32 agePlayersNode = getChildNodeBCasted(ki, ageInfoNode, *node);
		delete node;
		
		// find player info node
		node = new tvNode(MType | MOwner);
		node->type = KPlayerInfoNode;
		node->owner = ki;
		U32 infoNode = vaultDB->findNode(*node);
		delete node;
		if (!infoNode) throw txUnet(_WHERE("couldn't find player info node for KI %d", ki));
		
		if (remove) {
			// remove the reference and broadcast that
			vaultDB->removeNodeRef(agePlayersNode, infoNode);
			broadcastNodeRefUpdate(new tvNodeRef(0, agePlayersNode, infoNode), /*removal*/true);
		}
		else {
			// add link age owners -> player info
			addRefBCasted(ki, agePlayersNode, infoNode);
		}
	}
	
	U32 tVaultBackend::createPlayer(tmCustomVaultCreatePlayer &createPlayer)
	{
		tvNode *node;
		// check if the name is already in use
		node = new tvNode(MType | MlStr64_1);
		node->type = KVNodeMgrPlayerNode;
		node->lStr1 = createPlayer.avatar;
		U32 playerNode = vaultDB->findNode(*node);
		delete node;
		if (playerNode) return 0;
		
		// so now we have to create a new player
		// first, find the AllPlayersFolder
		node = new tvNode(MType | MInt32_1);
		node->type = KFolderNode;
		node->int1 = KAllPlayersFolder;
		U32 allPlayers = vaultDB->findNode(*node);
		delete node;
		if (!allPlayers) throw txUnet(_WHERE("could not find all players folder"));
		
		// create player node
		node = new tvNode(MType | MPerms | MStr64_1 | MlStr64_1 | MlStr64_2 | MText_1);
		node->type = KVNodeMgrPlayerNode;
		node->permissions = KDefaultPermissions;
		node->str1 = createPlayer.gender;
		node->lStr1 = createPlayer.avatar;
		node->lStr2.writeStr(alcGetStrUid(createPlayer.uid));
		node->text1 = createPlayer.login;
		U32 ki = vaultDB->createNode(*node);
		delete node;
		
		// create player info node as a child of the player node
		node = new tvNode(MType | MPerms | MOwner | MUInt32_1 | MlStr64_1);
		node->type = KPlayerInfoNode;
		node->permissions = KDefaultPermissions;
		node->owner = ki;
		node->uInt1 = ki; // used by client to find e.g. sender of a KI message
		node->lStr1 = createPlayer.avatar;
		U32 infoNode = vaultDB->createChildNode(ki, ki, *node);
		delete node;
		
		// link that player with Ae'gura, the hood and DniCityX2Final
		Byte guid[8];
		{
			if (!generateGuid(guid, "city", ki)) throw txProtocolError(_WHERE("error creating GUID"));
			tvAgeInfoStruct ageInfo("city", "Ae'gura", "Ae'gura", "Ae'gura", guid);
			tvSpawnPoint spawnPoint("FerryTerminal", "LinkInPointFerry");
			
			U32 ageNode = getAge(ageInfo);
			addAgeLinkToPlayer(ki, ageNode, spawnPoint);
		}
		
		{
			if (!generateGuid(guid, "Neighborhood", ki)) throw txProtocolError(_WHERE("error creating GUID"));
			tvAgeInfoStruct ageInfo("Neighborhood", "Neighborhood", hoodName, hoodDesc, guid);
			tvSpawnPoint spawnPoint("Default", "LinkInPointDefault");
			
			U32 ageNode = getAge(ageInfo);
			addAgeLinkToPlayer(ki, ageNode, spawnPoint);
			addRemovePlayerToAge(ageNode, ki);
		}
		
		{
			if (!generateGuid(guid, "DniCityX2Finale", ki)) throw txProtocolError(_WHERE("error creating GUID"));
			tvAgeInfoStruct ageInfo("DniCityX2Finale", "DniCityX2Finale", "", "", guid);
			tvSpawnPoint spawnPoint("Default", "LinkInPointDefault");
			
			U32 ageNode = getAge(ageInfo);
			addAgeLinkToPlayer(ki, ageNode, spawnPoint);
			// I don't know why, but the NEWS file clearly says that DniCityX2Finale should not have an owner *shrug*
		}
		
		// create link AllPlayersFolder -> info node (broadcasted)
		addRefBCasted(ki, allPlayers, infoNode);
		
		return ki;
	}
	
	void tVaultBackend::deletePlayer(tmCustomVaultDeletePlayer &deletePlayer)
	{
		// check if the account owns that avatar
		tvNode **nodes;
		int nNodes;
		vaultDB->fetchNodes(&deletePlayer.ki, 1, &nodes, &nNodes);
		if (nNodes != 1) throw txProtocolError(_WHERE("asked to remvoe non-existing player"));
		
		if (deletePlayer.accessLevel <= AcAdmin || strcmp((*nodes)->lStr2.c_str(), alcGetStrUid(deletePlayer.uid)) == 0) {
			int *table, tableSize;
			// find the  info node
			tvNode *node = new tvNode(MType | MOwner);
			node->type = KPlayerInfoNode;
			node->owner = deletePlayer.ki;
			U32 infoNode = vaultDB->findNode(*node);
			delete node;
			if (!infoNode) throw txUnet(_WHERE("Couldn't find a players info node?!?"));
			
			// remove player node and all sub-nodes
			vaultDB->getParentNodes(deletePlayer.ki, &table, &tableSize);
			// broadcast the removal (this must be done BEFORE removing the node because otherwise the references to it don't exist anymore)
			for (int i = 0; i < tableSize; ++i) {
				broadcastNodeRefUpdate(new tvNodeRef(0, table[i], deletePlayer.ki), /*remove:*/true);
			}
			// remove the node
			vaultDB->removeNodeTree(deletePlayer.ki, false); // don't be cautious
			// free stuff
			free((void *)table);
			
			// remove info node and all sub-nodes
			vaultDB->getParentNodes(infoNode, &table, &tableSize);
			// broadcast the removal (this must be done BEFORE removing the node because otherwise the references to it don't exist anymore)
			for (int i = 0; i < tableSize; ++i) {
				broadcastNodeRefUpdate(new tvNodeRef(0, table[i], infoNode), /*remove:*/true);
			}
			// remove the node
			vaultDB->removeNodeTree(infoNode, false); // don't be cautious
			// free stuff
			free((void *)table);
		}
		
		// free stuff
		delete *nodes;
		free((void *)nodes);
	}
	
	U32 tVaultBackend::getChildNodeBCasted(U32 saver, U32 parent, tvNode &node)
	{
		U32 nodeId = vaultDB->findNode(node);
		if (nodeId) {
			addRefBCasted(saver, parent, nodeId); // ensure that this is really a child node of the given parent
			return nodeId;
		}
		// it doesn't exist, create it
		return createChildNodeBCasted(saver, parent, node);
	}
	
	U32 tVaultBackend::createChildNodeBCasted(U32 saver, U32 parent, tvNode &node)
	{
		U32 nodeId = vaultDB->createChildNode(saver, parent, node);
		broadcastNodeRefUpdate(new tvNodeRef(saver, parent, nodeId), /*removal*/false);
		return nodeId;
	}
	
	void tVaultBackend::addRefBCasted(U32 saver, U32 parent, U32 son)
	{
		tvNodeRef *ref = new tvNodeRef(saver, parent, son);
		bool newRef = vaultDB->addNodeRef(*ref);
		if (newRef)
			broadcastNodeRefUpdate(ref, /*remove:*/false); // will delete the ref tvNodeRef instance
		else
			delete ref;
	}
	
	void tVaultBackend::broadcastNodeUpdate(tvNode &node, U32 origKi, U32 origMgr)
	{
		// create the broadcast message
		tvMessage bcast(VSaveNode);
		bcast.items.push_back(new tvItem(/*id*/9, /*old node index*/node.index));
		bcast.items.push_back(new tvItem(/*id*/24, /*timestamp*/node.modTime));
		// and send it
		broadcast(bcast, node.index, origKi, origMgr);
	}
	
	void tVaultBackend::broadcastNodeRefUpdate(tvNodeRef *ref, bool remove, U32 origKi, U32 origMgr)
	{
		// create the broadcast message
		tvMessage bcast(remove ? VRemoveNodeRef : VAddNodeRef);
		bcast.items.push_back(new tvItem(/*id*/7, ref));
		// and send it
		broadcast(bcast, ref->parent, origKi, origMgr);
	}
	
	void tVaultBackend::broadcastOnlineState(tvNode &node, U32 origKi, U32 origMgr)
	{
		// create the broadcast message
		tvMessage bcast(VOnlineState);
		bcast.items.push_back(new tvItem(/*id*/9, /*node index*/node.index));
		bcast.items.push_back(new tvItem(/*id*/24, /*timestamp*/node.modTime));
		if (node.int1) { // if he's online
			bcast.items.push_back(new tvItem(/*id*/27, node.str1.c_str())); // age name
			bcast.items.push_back(new tvItem(/*id*/28, node.str2.c_str())); // age guid
			bcast.items.push_back(new tvItem(/*id*/29, node.int1)); // online state
		}
		else { // if he's not online
			bcast.items.push_back(new tvItem(/*id*/29, node.int1)); // online state
		}
		// and send it
		broadcast(bcast, node.index, origKi, origMgr);
	}
	
	void tVaultBackend::broadcast(tvMessage &msg, U32 node, U32 origKi, U32 origMgr)
	{
		U32 *table, tableSize;
		tNetSession *session;
		vaultDB->getMGRs(node, &table, &tableSize);
		DBG(9, "Got interested MGRs\n");
		// now let's see who gets notified
		for (tVmgrList::iterator it = vmgrs.begin(); it != vmgrs.end(); ++it) {
			if (it->mgr == 0 || (it->ki == origKi && it->mgr == origMgr)) continue;
			session = net->getSession(it->session);
			if (!session) continue;
			for (U32 j = 0; j < tableSize; ++j) {
				if (table[j] > it->mgr) break; // the list is in order, we will not find this node in there
				else if (table[j] < it->mgr) continue;
				msg.vmgr = it->mgr;
				send(msg, session, it->ki);
				break;
			}
		}
		free((void *)table);
	}
	
	void tVaultBackend::cleanVault(bool cleanAges)
	{
		log->log("Cleaning up the vault. I hope you did a backup first! Now it's too late though...\n");
		vaultDB->clean(cleanAges);
	}
	
	bool tVaultBackend::isAgePrivate(const char *age) const
	{
		// local copy of private age list as strsep modifies it
		char ages[1024];
		strcpy(ages, privateAges);
		
		char *buf = ages;
		char *p = strsep(&buf, ",");
		while (p != 0) {
			if (strcmp(p, age) == 0) return true;
			p = strsep(&buf, ",");
		}
		return false;
	}
	
	bool tVaultBackend::generateGuid(Byte *guid, const char *age, U32 ki)
	{
		tAgeInfo *ageInfo = ageInfos->getAge(age);
		if (!ageInfo) return false;
		if (ageInfo->seqPrefix > 0x00FFFFFF) return false; // obviously he wants to link to an age like GlobalMarkers
		bool isPrivate = (instanceMode == 1) ? isAgePrivate(age) : false;
		if (isPrivate && ki > 0x0FFFFFFF) throw txBase(_WHERE("KI is too big!")); // ensure 1st bit of the 4 byte is 0 (see comment below)
		
		/* so we have "The server GUID, aka age guid"
		---------------------------------
		| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
		--------------------------------
		| 0 | ki here       | s | s | s |
		--------------------------------
		Where s is the sequence prefix, and the 1st bit of the 4 byte should be always 0.
		
		This ensures all GUIDs are unique as no player can have an age several times. That's what happened in Cyan's servers when you
		reset an age, but Alcugs will handle that differently to also avoid names like "Diafero's(1) Relto".
		*/
		tMBuf buf;
		buf.putByte(0);
		buf.putU32(isPrivate ? ki : 0);
		
		// 3 byte sequence prefix: First the one which usually is zero, to keep compatability
		buf.putByte(ageInfo->seqPrefix >> 16);
		// then the remaining two bytes
		buf.putU16(ageInfo->seqPrefix & 0x0000FFFF);
		
		buf.rewind();
		memcpy(guid, buf.read(8), 8);
		return true;
	}
	
	bool tVaultBackend::setAgeGuid(tvAgeLinkStruct *link, U32 ownerKi)
	{
		return generateGuid(link->ageInfo.guid, link->ageInfo.filename.c_str(), ownerKi);
	}

} //end namespace alc
