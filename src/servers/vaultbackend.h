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
		void processVaultMsg(tvMessage &msg, tNetSession *u, U32 ki);
	private:
		void send(tvMessage &msg, tNetSession *u, U32 ki);
		int findVmgr(tNetSession *u, U32 ki, U32 node); //!< \returns the number of that vmgr or -1
	
		tVaultDB *vaultDB;
		Byte vaultFolderName[17];
		
		tLog *log, *logHtml;
		bool shortHtml;
		tUnet *net;
		
		// the list of vmgrs
		struct tVmgr {
			tVmgr(U32 ki, U32 node, tNetSessionIte session) {
				this->ki = ki;
				this->node = node;
				this->session = session;
			}
			U32 ki;
			U32 node;
			tNetSessionIte session;
		};
		int nVmgrs;
		tVmgr **vmgrs;
	};


} //End alc namespace

#endif
