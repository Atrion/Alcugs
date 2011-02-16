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
#define __U_AUTHSERVER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <unetserverbase.h>

////extra includes
#include "authserver.h"
#include <protocol/authmsg.h>
#include <netlog.h>

namespace alc {
	
	const char *alcNetName = "Auth";
	tUnetServerBase *alcServerInstance(void) { return new tUnetAuthServer(); }

	////IMPLEMENTATION
	int tUnetAuthServer::onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgCustomAuthAsk:
			{
				tmCustomAuthAsk authAsk(u);
				tString passwd;
				Byte hexUid[16];
				Byte accessLevel;
				int authResult;
				
				// get the data out of the packet
				msg->data.get(authAsk);
				log->log("<RCV> [%d] %s\n", msg->sn, authAsk.str().c_str());
				
				// authenticate player
				tString challenge = alcHex2Ascii(tMBuf(authAsk.challenge, 16));
				tString hash = alcHex2Ascii(tMBuf(authAsk.hash, 16));
				authResult = authBackend->authenticatePlayer(u, authAsk.login, challenge, hash, authAsk.release, alcGetStrIp(authAsk.ip), &passwd, hexUid, &accessLevel);
				
				// send answer to client
				tmCustomAuthResponse authResponse(u, authAsk, hexUid, passwd, authResult, accessLevel);
				send(authResponse);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
