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
	
	int tVaultBackend::findVmgr(tNetSession *u, U32 ki, U32 node)
	{
		for (int i = 0; i < nVmgrs; ++i) {
			if (vmgrs[i] && vmgrs[i]->ki == ki && vmgrs[i]->node == node) {
				vmgrs[i]->session = u->getIte();
				return i;
			}
		}
		return -1;
	}
	
	void tVaultBackend::processVaultMsg(tvMessage &msg, tNetSession *u, U32 ki)
	{
		msg.print(logHtml, /*clientToServer:*/true, u, shortHtml, ki);
		
		S32 nodeType = -1, id = -1;
		U32 *table = NULL, tableSize; // make sure to free the table
		
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
				case 10: // Stream containing a table of ints
				{
					if (itm->type != DCreatableStream) throw txProtocolError(_WHERE("a vault item with id 10 must always have a creatable generic stream"));
					tMBuf *buf = ((tvCreatableStream *)itm->data)->getData();
					tableSize = buf->getU16();
					table = (U32 *)malloc(tableSize*sizeof(int));
					for (U32 i = 0; i < tableSize; ++i) table[i] = buf->getU32();
					bool eof = buf->eof();
					delete buf;
					if (!eof) throw txProtocolError(_WHERE("the stream is too long"));
					break;
				}
				case 20: // GenericValue.Int: must always be the same
					if (itm->asInt() != -1)
						throw txProtocolError(_WHERE("a vault item with ID 20 must always have a value of -1 but I got %d", itm->asInt()));
					break;
				default:
					throw txProtocolError(_WHERE("vault item has invalid id %d", itm->id));
			}
		}
		
		if (msg.cmd == VConnect || findVmgr(u, ki, msg.vmgr) >= 0) {
			switch (msg.cmd) {
				case VConnect:
				{
					log->log("Vault Connect request for %d (Type: %d)\n", ki, nodeType);
					tvNode node;
					node.setType(nodeType);
					U32 nodeId;
					if (nodeType == 5) { // admin node
						nodeId = vaultDB->findNode(node, true);
						// create and send the reply
						tvMessage reply(msg, 3);
						reply.items[0] = new tvItem(/*id:*/1, /*node type*/5);
						reply.items[1] = new tvItem(/*id*/2, /*node id*/nodeId);
						reply.items[2] = new tvItem(/*id*/23, /*folder name*/vaultFolderName);
						send(reply, u, ki);
					}
					else { // wrong or no node type at all
						lerr->log("%s [KI: %d] Connect request for unknown node type %d from KI %d\n", u->str(), ki, nodeType);
						break; // the switch
					}
					// now let's see where we save this... first look if we already have this one registered
					int nr = findVmgr(u, ki, msg.vmgr);
					if (nr >= 0) // it is already registered, and findVmgr updated the session, so we have nothing to do
						break; // the switch
					// if that's not the case, search for a free slot
					else {
						for (int i = 0; i < nVmgrs; ++i) {
							if (!vmgrs[i]) {
								nr = i;
								break; // the loop
							}
						}
					}
					// if there's none, we have to resize the table
					if (nr < 0) {
						nr = nVmgrs;
						++nVmgrs;
						vmgrs = (tVmgr **)realloc((void *)vmgrs, sizeof(tVmgr *)*nVmgrs);
					}
					vmgrs[nr] = new tVmgr(ki, nodeId, u->getIte());
					// FIXME: make sure the vmgrs are somehow cleaned up when they're inactive even when a player does not send a VDisconnect... the old vault server doesn't do that
					break;
				}
				default:
					lerr->log("%s [KI: %d] Unknown vault command 0x%02X (%s)\n", u->str(), ki, msg.cmd, alcVaultGetCmd(msg.cmd));
					break;
			}
		}
		else
			lerr->log("%s [KI: %d] Player sent a 0x%02X (%s) but did not yet send the VConnect\n", u->str(), ki, msg.cmd, alcVaultGetCmd(msg.cmd));
		if (table) free(table);
	}

} //end namespace alc

