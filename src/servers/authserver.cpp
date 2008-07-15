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

/**
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

/* CVS tag - DON'T TOUCH*/
#define __U_AUTHSERVER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <unet.h>

////extra includes
#include <protocol/authmsg.h>
#include "authserver.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	const char * alcNetName="Auth";
	Byte alcWhoami=KAuth;
	
	int tUnetAuthServer::onMsgRecieved(alc::tNetEvent *ev, alc::tUnetMsg *msg, alc::tNetSession *u)
	{
		int ret = tUnetServerBase::onMsgRecieved(ev, msg, u); // first let tUnetServerBase process the message
		if (ret != 0) return ret; // cancel if it was processed, otherwise it's our turn
		
		switch(msg->cmd) {
			case NetMsgCustomAuthAsk:
			{
				tmCustomAuthAsk authAsk(u);
				Byte guid[50], passwd[50], challenge[50], hash[50], ip[50], accessLevel;
				int authResult;
				
				// get the data out of the packet
				msg->data->get(authAsk);
				log->log("<RCV> %s\n", authAsk.str());
				
				// authenticate player
				alcHex2Ascii(challenge, authAsk.challenge, 16);
				alcHex2Ascii(hash, authAsk.hash, 16);
				strncpy(ip, alcGetStrIp(authAsk.ip), 49); // alcGetStrIp uses a local static array so we have to copy it
				authResult = authBackend->authenticatePlayer(u, (Byte *)authAsk.login.c_str(), challenge, hash, authAsk.release, ip, passwd, guid, &accessLevel);
				
				// send answer to client
				tmCustomAuthResponse authResponse(u, authAsk, alcGetHexUid(guid), passwd, authResult, accessLevel);
				send(authResponse);
				
				return 1;
			}
		}
		return 0;
	}

} //end namespace alc
