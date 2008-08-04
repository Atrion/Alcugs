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

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>

////extra includes
#include "vaultbackend.h"

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	tVaultBackend::tVaultBackend(tUnet *net)
	{
		this->net = net;
		log = logHtml = lnull;
		vaultDB = NULL;
		nVmgrs = 0;
		vmgrs = NULL;
	}
	
	tVaultBackend::~tVaultBackend(void)
	{
		unload();
		int del = 0;
		if (vmgrs) {
			for (int i = 0; i < nVmgrs; ++i) {
				if (vmgrs[i]) {
					delete vmgrs[i];
					++del;
				}
			}
			free((void *)vmgrs);
			lerr->log("ERR: The vault server is quitting and I still have %d vmgrs left\n", del);
		}
	}
	
	void tVaultBackend::unload(void)
	{
		if (vaultDB != NULL) {
			delete vaultDB;
			vaultDB = NULL;
		}
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
			shortHtml = (!var.isNull() && var.asByte()); // per default, it's not short
		}
		
		var = cfg->getVar("vault.unstable");
		if (var.isNull() || !var.asByte()) {
			lerr->log("\n\nREALLY IMPORTANT WARNING:\n=========================\n\nThis vault sevrer uses a vault db layout which might CHANGE WITHOUT A POSSIBILITY TO MIGRATE since it's still in development.\nThis way, YOUR VAULT DATA WILL BE LOST.\nI hope you know what you are doing.\n\nIn that case, set vault.unstable=1 in your config file. But you'd better just use the old vault server.\n\n\n");
			throw txBase(_WHERE("refusing to use unstable vault database"));
		}
		
		log->log("Started VaultBackend (%s)\n", __U_VAULTBACKEND_ID);
		vaultDB = new tVaultDB(log);
		log->nl();
		log->flush();
		vaultDB->getVaultFolderName(vaultFolderName);
		DBG(5, "global vault folder name is %s\n", vaultFolderName);
	}
	
	void tVaultBackend::send(tvMessage &msg, tNetSession *u, U32 ki)
	{
		msg.print(logHtml, /*clientToServer:*/false, u, shortHtml, ki);
		tmVault vaultMsg(u, ki, &msg);
		net->send(vaultMsg);
	}
	
	void tVaultBackend::sendPlayerList(tmCustomVaultAskPlayerList &askPlayerList)
	{
		tmCustomVaultPlayerList list(askPlayerList.getSession(), askPlayerList.x, askPlayerList.uid);
		list.numberPlayers = vaultDB->getPlayerList(list.players, askPlayerList.uid);
		net->send(list);
	}
	
	void tVaultBackend::checkKi(tmCustomVaultCheckKi &checkKi)
	{
		Byte avatar[256], status;
		status = vaultDB->checkKi(checkKi.ki, checkKi.uid, avatar);
		tmCustomVaultKiChecked checked(checkKi.getSession(), checkKi.ki, checkKi.x, checkKi.uid, status, avatar);
		net->send(checked);
	}
	
	void tVaultBackend::updatePlayerStatus(tmCustomVaultPlayerStatus &status)
	{
		// update (when the palyer is online) or remove (when hes offline) all mgrs using this KI
		bool checkShrink = false;
		for (int i = 0; i < nVmgrs; ++i) {
			if (vmgrs[i] && vmgrs[i]->ki == status.ki) {
				if (status.state) vmgrs[i]->session = status.getSession()->getIte();
				else {
					delete vmgrs[i];
					vmgrs[i] = NULL;
					checkShrink = true; // check if the array can be srhunken
					log->log("WARN: Player with KI %d just went offline but still had a vmgr registered... removing it\n", status.ki);
				}
			}
		}
		if (checkShrink) {
			// shrink the array if possible
			int last = nVmgrs-1; // find the last vmgr
			while (last >= 0 && vmgrs[last] == NULL) --last;
			if (last < nVmgrs-1) { // there are some NULLs at the end, shrink the array
				nVmgrs=last+1;
				vmgrs=(tVmgr **)realloc(vmgrs, sizeof(tVmgr *) * nVmgrs); // it's not a bug if we get NULL here - the size might be 0
				DBG(9, "shrinking vmgr array to %d\n", nVmgrs);
			}
		}
		
		tvNode **nodes;
		int nNodes;
		tvNode node;
		
		// update online time
		if (status.onlineTime > 0) {
			vaultDB->fetchNodes(&status.ki, 1, &nodes, &nNodes);
			if (nNodes != 1) throw txProtocolError(_WHERE("KI %d doesn't have a player node (or several of them)?!?", status.ki));
			(*nodes)->uInt2 += status.onlineTime;
			vaultDB->updateNode(**nodes);
			broadcastNodeUpdate(**nodes);
			// free stuff
			delete *nodes;
			free((void *)nodes);
		}
		
		// find and update the info node
		node.flagB = MType | MOwner;
		node.type = KPlayerInfoNode;
		node.owner = status.ki;
		U32 infoNode = vaultDB->findNode(node, NULL, false);
		if (!infoNode) throw txProtocolError(_WHERE("KI %d doesn't have a info node?!?", status.ki));
		vaultDB->fetchNodes(&infoNode, 1, &nodes, &nNodes);
		(*nodes)->str1 = status.age;
		(*nodes)->str2 = status.serverGuid;
		(*nodes)->int1 = status.state;
		vaultDB->updateNode(**nodes);
		broadcastOnlineState(**nodes);
		// free stuff
		delete *nodes;
		free((void *)nodes);
	}
	
	int tVaultBackend::findVmgr(tNetSession *u, U32 ki, U32 mgr)
	{
		for (int i = 0; i < nVmgrs; ++i) {
			if (vmgrs[i] && vmgrs[i]->ki == ki && vmgrs[i]->mgr == mgr) {
				vmgrs[i]->session = u->getIte();
				return i;
			}
		}
		return -1;
	}
	
	void tVaultBackend::processVaultMsg(tvMessage &msg, tNetSession *u, U32 ki)
	{
		msg.print(logHtml, /*clientToServer:*/true, u, shortHtml, ki);
		
		// have everything on the stack as we can safely throw exceptions then
		S32 nodeType = -1, playerKi = -1, nodeSon = -1, nodeParent = -1, seenFlag = -1, rcvPlayer = -1;
		U16 tableSize = 0;
		tMBuf table;
		tvNode *savedNode = NULL;
		tvNodeRef *savedNodeRef = NULL;
		const Byte *ageName = NULL, *ageGuid = NULL;
		
		// read and verify the general vault items
		for (int i = 0; i < msg.numItems; ++i) {
			tvItem *itm = msg.items[i];
			switch (itm->id) {
				case 0: // GenericValue.Int: used in VaultManager, must always be the same
					if (itm->asInt() != (S32)0xC0AB3041)
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
				case 9: // GenericValue.Int: FoundNode Index / Son of a NodeRef / Old Node Index (saveNode)
					nodeSon = itm->asInt();
					break;
				case 10: // GenericStream: Stream containing a table of ints
				{
					if (itm->type != DCreatableStream) throw txProtocolError(_WHERE("a vault item with id 10 must always have a creatable generic stream"));
					tMBuf *buf = ((tvCreatableStream *)itm->data)->getData();
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
				case 16: // GenericValue.Int: must always be 0 or 1 (seen in FindNode)
					if (itm->asInt() != 0 && itm->asInt() != 1)
						throw txProtocolError(_WHERE("a vault item with ID 16 must always have a value of 0 or 1 but I got %d", itm->asInt()));
					break;
				case 19: // GenericValue.Int: Set Seen flag
					seenFlag = itm->asInt();
					break;
				case 20: // GenericValue.Int: must always be the same
					if (itm->asInt() != -1)
						throw txProtocolError(_WHERE("a vault item with ID 20 must always have a value of -1 but I got %d", itm->asInt()));
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
				case 24: // GenericValue.Timestamp
				case 25: // GenericValue.Int: number of vault nodes
				case 31: // GenericValue.Int: EOF of a FetchNode
				default:
					throw txProtocolError(_WHERE("vault item has invalid id %d", itm->id));
			}
		}
		
		if (msg.cmd != VConnect && findVmgr(u, ki, msg.vmgr) < 0)
			throw txProtocolError(_WHERE("player did not yet send his VConnect"));
		switch (msg.cmd) {
			case VConnect:
			{
				if (nodeType < 0)
					throw txProtocolError(_WHERE("got a VConnect where node type has not been set"));
				log->log("Vault Connect request for %d (Type: %d)\n", ki, nodeType);
				log->flush();
				U32 mgr;
				
				tvNode mgrNode;
				mgrNode.flagB = MType;
				mgrNode.type = nodeType;
				if (nodeType == 2) { // player node
					if (playerKi < 0) throw txProtocolError(_WHERE("VConnect for node type 2 must have playerKi set"));
					mgr = playerKi;
					if (mgr != ki)
						throw txProtocolError(_WHERE("Player with KI %d wants to VConnect as %d\n", ki, mgr));
					// create reply
					tvMessage reply(msg, 2);
					reply.items[0] = new tvItem(/*id*/2, /*mgr node id*/(S32)mgr);
					reply.items[1] = new tvItem(/*id*/23, /*folder name*/vaultFolderName);
					send(reply, u, ki);
				}
				else if (nodeType == 3) { // age node
					if (!ageName || !ageGuid) throw txProtocolError(_WHERE("VConnect for node type 3 must have ageGuid and ageName set"));
					mgrNode.flagB |= MStr64_1;
					mgrNode.str1.writeStr(alcGetStrGuid(ageGuid));
					mgr = vaultDB->findNode(mgrNode, NULL, /*create*/true);
					// create and send the reply
					tvMessage reply(msg, 3);
					reply.items[0] = new tvItem(/*id:*/2, /*mgr node id*/(S32)mgr);
					reply.items[1] = new tvItem(/*id*/21, /*age name*/ageName);
					reply.items[2] = new tvItem(/*id*/23, /*folder name*/vaultFolderName);
					send(reply, u, ki);
				}
				else if (nodeType == 5) { // admin node
					mgr = vaultDB->findNode(mgrNode, NULL, /*create*/true);
					// create and send the reply
					tvMessage reply(msg, 3);
					reply.items[0] = new tvItem(/*id:*/1, /*node type*/5);
					reply.items[1] = new tvItem(/*id*/2, /*mgr node id*/(S32)mgr);
					reply.items[2] = new tvItem(/*id*/23, /*folder name*/vaultFolderName);
					send(reply, u, ki);
				}
				else // wrong or no node type at all
					throw txProtocolError(_WHERE("Connect request for unknown node type %d from KI %d\n", nodeType));
				// now let's see where we save this... first look if we already have this one registered
				int nr = findVmgr(u, ki, msg.vmgr);
				if (nr >= 0) // it is already registered, and findVmgr updated the session, so we have nothing to do
					break;
				// if that's not the case, search for a free slot
				else {
					for (int i = 0; i < nVmgrs; ++i) {
						if (!vmgrs[i]) {
							nr = i;
							break; // breaks the loop, not the switch
						}
					}
				}
				// if there's none, we have to resize the table
				if (nr < 0) {
					nr = nVmgrs;
					++nVmgrs;
					vmgrs = (tVmgr **)realloc((void *)vmgrs, sizeof(tVmgr *)*nVmgrs);
				}
				vmgrs[nr] = new tVmgr(ki, mgr, u->getIte());
				// FIXME: make sure the vmgrs are somehow cleaned up when they're inactive even when a player does not send a VDisconnect... the old vault server doesn't do that
				break;
			}
			case VDisconnect:
			{
				if (nodeType < 0)
					throw txProtocolError(_WHERE("got a VDisconnect where the node type has not been set"));
				log->log("Vault Disconnect request for %d (Type: %d)\n", ki, nodeType);
				log->flush();
				// send reply
				tvMessage reply(msg, 0);
				send(reply, u, ki);
				// remove vmgr
				int nr = findVmgr(u, ki, msg.vmgr);
				delete vmgrs[nr];
				vmgrs[nr] = NULL;
				// shrink if possible
				int last = nVmgrs-1; // find the last vmgr
				while (last >= 0 && vmgrs[last] == NULL) --last;
				if (last < nVmgrs-1) { // there are some NULLs at the end, shrink the array
					nVmgrs=last+1;
					vmgrs=(tVmgr **)realloc(vmgrs, sizeof(tVmgr *) * nVmgrs); // it's not a bug if we get NULL here - the size might be 0
					DBG(9, "shrinking vmgr array to %d\n", nVmgrs);
				}
				break;
			}
			case VAddNodeRef:
			{
				if (!savedNodeRef) throw txProtocolError(_WHERE("got a VAddNodeRef without a node ref attached"));
				log->log("Vault Add Node Ref from %d to %d for %d\n", savedNodeRef->parent, savedNodeRef->child, ki);
				log->flush();
				if (!vaultDB->addNodeRef(*savedNodeRef)) return; // ignore duplicates
				
				// broadcast the change
				broadcastNodeRefUpdate(new tvNodeRef(*savedNodeRef), ki, msg.vmgr);
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
				broadcastNodeRefUpdate(new tvNodeRef(0, nodeParent, nodeSon), ki, msg.vmgr);
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
				tvMessage reply(msg, 2);
				reply.compressed = 3; // compressed
				reply.items[0] = new tvItem(new tvCreatableStream(/*id*/14, (tvBase **)mfs, nMfs)); // tvMessage will delete it for us
				reply.items[1] = new tvItem(new tvCreatableStream(/*id*/15, (tvBase **)ref, nRef)); // tvMessage will delete it for us
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
				if (savedNode->modTime == 0 || !(savedNode->flagB & MModTime)) throw txProtocolError(_WHERE("every saved node must have a timestamp"));
				
				if (savedNode->index < 19000) {
					U32 oldIndex = savedNode->index;
					U32 newIndex = vaultDB->createNode(*savedNode);
					log->log("Vault Save Node (created new node %d) for %d\n", newIndex, ki);
					log->flush();
					
					// reply to the sender
					tvMessage reply(msg, 2);
					reply.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)oldIndex);
					reply.items[1] = new tvItem(/*id*/11, /*old node index*/(S32)newIndex);
					send(reply, u, ki);
				}
				else {
					vaultDB->updateNode(*savedNode);
					log->log("Vault Save Node (updated node %d) for %d\n", savedNode->index, ki);
					log->flush();
					
					// create the reply to the sender
					tvMessage reply(msg, 1);
					reply.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)savedNode->index);
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
				if (!vaultDB->findNode(*savedNode, &mfs, /*create*/false))
					throw txProtocolError(_WHERE("got a VFindNode but can't find the node"));
				// create and send the reply
				tvMessage reply(msg, 2);
				reply.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)mfs.id);
				reply.items[1] = new tvItem(/*id*/24, /*timestamp*/(double)mfs.time);
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
				int num = 0;
				for (int i = 0; i < nNodes; ++i) {
					buf.put(*nodes[i]);
					++num;
					if (buf.size() > 128000 || i == (nNodes-1)) { // if size is already big enough or this is the last one
						// create reply
						bool eof = (i == (nNodes-1));
						tvMessage reply(msg, eof ? 3 : 2);
						reply.compressed = 3; // compressed
						reply.items[0] = new tvItem(new tvCreatableStream(/*id*/6, buf)); // tvMessage will delete it for us
						reply.items[1] = new tvItem(/*id*/25, /*number of nodes*/num);
						if (eof) reply.items[2] = new tvItem(/*id*/31, 0); // EOF mark (this is the last packet of nodes)
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
				tvNode *node; // make sure it is reset after being used, vaultDB might change it
				// find the sender's info node
				node = new tvNode;
				node->flagB = MType | MOwner;
				node->type = KPlayerInfoNode;
				node->owner = ki;
				U32 infoNode = vaultDB->findNode(*node);
				delete node;
				if (!infoNode) throw txProtocolError(_WHERE("Couldn't find a players info node?!?"));
				
				// find and create the PeopleIKnowAboutFolder
				node = new tvNode;
				node->flagB = MType | MOwner | MInt32_1;
				node->type = KPlayerInfoListNode;
				node->owner = rcvPlayer;
				node->int1 = KPeopleIKnowAboutFolder;
				U32 recentFolder = getNode(*node, rcvPlayer);
				delete node;
				
				// find and create the InboxFolder
				node = new tvNode;
				node->flagB = MType | MOwner | MInt32_1;
				node->type = KFolderNode;
				node->owner = rcvPlayer;
				node->int1 = KInboxFolder;
				U32 inbox = getNode(*node, rcvPlayer);
				delete node;
				
				// add sender to recent folder
				addRef(0, recentFolder, infoNode);
				
				// add what was sent to the inbox
				addRef(ki, inbox, savedNode->index);
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
	
	U32 tVaultBackend::getNode(tvNode &node, U32 parent)
	{
		U32 nodeId = vaultDB->findNode(node);
		if (nodeId) return nodeId;
		// it doesn't exist, create it
		nodeId = vaultDB->createNode(node);
		addRef(0, parent, nodeId);
		return nodeId;
	}
	
	void tVaultBackend::addRef(U32 saver, U32 parent, U32 son)
	{
		tvNodeRef *ref = new tvNodeRef(saver, parent, son); // will be deleted by broadcastNodeRefUpdate
		if (vaultDB->addNodeRef(*ref))
			broadcastNodeRefUpdate(ref);
		else
			delete ref;
	}
	
	void tVaultBackend::broadcastNodeUpdate(tvNode &node, U32 origKi, U32 origMgr)
	{
		// create the broadcast message
		tvMessage bcast(VSaveNode, 2);
		bcast.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)node.index);
		bcast.items[1] = new tvItem(/*id*/24, /*timestamp*/(double)node.modTime);
		// and send it
		broadcast(bcast, node.index, origKi, origMgr);
	}
	
	void tVaultBackend::broadcastNodeRefUpdate(tvNodeRef *ref, U32 origKi, U32 origMgr)
	{
		// create the broadcast message
		tvMessage bcast(VAddNodeRef, 1);
		bcast.items[0] = new tvItem(/*id*/7, ref);
		// and send it
		broadcast(bcast, ref->parent, origKi, origMgr);
	}
	
	void tVaultBackend::broadcastOnlineState(tvNode &node)
	{
		// create the broadcast message
		tvMessage bcast(VOnlineState, node.int1 ? 5 : 3);
		bcast.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)node.index);
		bcast.items[1] = new tvItem(/*id*/24, /*timestamp*/(double)node.modTime);
		if (node.int1) { // if he's online
			bcast.items[2] = new tvItem(/*id*/27, node.str1.c_str()); // age name
			bcast.items[3] = new tvItem(/*id*/28, node.str2.c_str()); // age guid
			bcast.items[4] = new tvItem(/*id*/29, (S32)node.int1); // online state
		}
		else { // if he's not online
			bcast.items[2] = new tvItem(/*id*/29, (S32)node.int1); // online state
		}
		// and send it
		broadcast(bcast, node.index);
	}
	
	void tVaultBackend::broadcast(tvMessage &msg, U32 node, U32 origKi, U32 origMgr)
	{
		U32 *table, tableSize;
		tNetSession *session;
		vaultDB->getMGRs(node, &table, &tableSize);
		// now let's see who gets notified
		for (int i = 0; i < nVmgrs; ++i) {
			if (!vmgrs[i] || vmgrs[i]->mgr == 0 || (vmgrs[i]->ki == origKi && vmgrs[i]->mgr == origMgr)) continue;
			session = net->getSession(vmgrs[i]->session);
			if (!session) continue;
			for (U32 j = 0; j < tableSize; ++j) {
				if (table[j] != vmgrs[i]->mgr) continue;
				msg.vmgr = vmgrs[i]->mgr;
				send(msg, session, vmgrs[i]->ki);
				break;
			}
		}
		free((void *)table);
	}

} //end namespace alc

