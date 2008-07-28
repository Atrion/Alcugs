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
	
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("vault.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			log = new tLog("vault.log", 4, 0);
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
	}
	
	void tVaultBackend::sendPlayerList(tmCustomVaultAskPlayerList &askPlayerList)
	{
		tmCustomVaultPlayerList list(askPlayerList.getSession(), askPlayerList.x, askPlayerList.uid);
		list.numberPlayers = vaultDB->getPlayerList(list.players, askPlayerList.uid);
		net->send(list);
	}

} //end namespace alc

