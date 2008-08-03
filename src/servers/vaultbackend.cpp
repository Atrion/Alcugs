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
		if (nVmgrs > 0) lerr->log("ERR: The vault server is quitting and I still have %d vmgrs left\n", nVmgrs);
		if (vmgrs) {
			for (int i = 0; i < nVmgrs; ++i) {
				if (vmgrs[i]) delete vmgrs[i];
			}
			free((void *)vmgrs);
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
		S32 nodeType = -1, id = -1, nodeSon = -1, nodeParent = -1;
		U16 tableSize = 0;
		tMBuf table;
		tvNode *savedNode = NULL;
		
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
					id = itm->asInt();
					break;
				case 5: // a single vault node
					savedNode = itm->asNode(); // we don't have to free it, tvMessage does that
					break;
				case 9: // FoundNode Index / Son of a NodeRef / Old Node Index (saveNode)
					nodeSon = itm->asInt();
					break;
				case 10: // Stream containing a table of ints
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
				case 13: // Parent of a NodeRef
					nodeParent = itm->asInt();
					break;
				case 16: // GenericValue.Int: must always be the same (seen in FindNode)
					if (itm->asInt() != 0)
						throw txProtocolError(_WHERE("a vault item with ID 16 must always have a value of 0 but I got %d", itm->asInt()));
					break;
				case 20: // GenericValue.Int: must always be the same
					if (itm->asInt() != -1)
						throw txProtocolError(_WHERE("a vault item with ID 20 must always have a value of -1 but I got %d", itm->asInt()));
					break;
				// these are not sent to servers
				case 6: // a stream of Vault Nodes
				case 11: // new Node Index (saveNode)
				case 14: // stream of manifests
				case 15: // stream of NodeRefs
				case 23: // Vault folder
				case 24: // Timestamp (double format)
				case 25: // number of vault nodes
				case 31: // EOF of a FetchNode
				default:
					throw txProtocolError(_WHERE("vault item has invalid id %d", itm->id));
			}
		}
		
		if (msg.cmd != VConnect && findVmgr(u, ki, msg.vmgr) < 0)
			throw txProtocolError(_WHERE("player did not yet send his VConnect"));
		switch (msg.cmd) {
			case VConnect:
			{
				log->log("Vault Connect request for %d (Type: %d)\n", ki, nodeType);
				U32 mgr;
				if (nodeType == 2) { // player node
					mgr = id;
					if (mgr != ki)
						throw txProtocolError(_WHERE("Player with KI %d wants to VConnect as %d\n", ki, mgr));
					// create reply
					tvMessage reply(msg, 2);
					reply.items[0] = new tvItem(/*id*/2, /*mgr node id*/(S32)mgr);
					reply.items[1] = new tvItem(/*id*/23, /*folder name*/vaultFolderName);
					send(reply, u, ki);
				}
				else if (nodeType == 5) { // admin node
					tvNode mgrNode;
					mgrNode.setType(5);
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
				log->log("Vault Disconnect request for %d (Type: %d)\n", ki, nodeType);
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
			case VRemoveNodeRef:
			{
				if (nodeSon < 0 || nodeParent < 0) throw txProtocolError(_WHERE("got a VRemoveNodeRef where parent or son have not been set"));
				vaultDB->removeNodeRef(nodeParent, nodeSon);
				
				// broadcast the change
				tvMessage bcast(VRemoveNodeRef, 1);
				bcast.items[0] = new tvItem(/*id*/7, new tvNodeRef(nodeParent, nodeSon));
				break;
			}
			case VNegotiateManifest:
			{
				if (tableSize != 1)
					throw txProtocolError(_WHERE("Getting %d manifests at once is not supported\n", tableSize));
				tvManifest **mfs;
				tvNodeRef **ref;
				int nMfs, nRef;
				vaultDB->getManifest(table.getU32(), &mfs, &nMfs, &ref, &nRef);
				
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
					
					// reply to the sender
					tvMessage reply(msg, 2);
					reply.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)oldIndex);
					reply.items[1] = new tvItem(/*id*/11, /*old node index*/(S32)newIndex);
					send(reply, u, ki);
				}
				else {
					vaultDB->updateNode(*savedNode);
					
					// create the reply to the sender
					tvMessage reply(msg, 1);
					reply.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)savedNode->index);
					send(reply, u, ki);
					// create the broadcast reply
					tvMessage bcast(VSaveNode, 2);
					bcast.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)savedNode->index);
					bcast.items[1] = new tvItem(/*id*/24, /*timestamp*/(double)savedNode->modTime);
					// and broadcast it
					broadcast(bcast, savedNode->index, ki, msg.vmgr);
				}
				
				break;
			}
			case VFindNode:
			{
				tvManifest mfs;
				if (vaultDB->findNode(*savedNode, &mfs, /*create*/false)) {
					// create and send the reply
					tvMessage reply(msg, 2);
					reply.items[0] = new tvItem(/*id*/9, /*old node index*/(S32)mfs.id);
					reply.items[1] = new tvItem(/*id*/24, /*timestamp*/(double)mfs.time);
					send(reply, u, ki);
				}
			}
			case VFetchNode:
			{
				if (tableSize <= 0) break;
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
			default:
				throw txProtocolError(_WHERE("Unknown vault command 0x%02X (%s)\n", msg.cmd, alcVaultGetCmd(msg.cmd)));
		}
	}
	
	void tVaultBackend::broadcast(tvMessage &msg, U32 node, U32 origKi, U32 origMgr)
	{
		U32 *table, tableSize;
		tNetSession *session;
		vaultDB->getMGRs(node, &table, &tableSize);
		// now let's see who gets notified
		for (int i = 0; i < nVmgrs; ++i) {
			if (vmgrs[i]->mgr == 0 || (vmgrs[i]->ki == origKi && vmgrs[i]->mgr == origMgr)) continue;
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

