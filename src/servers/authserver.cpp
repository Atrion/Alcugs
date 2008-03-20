/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <unet.h>

////extra includes
#include "authserver.h"
#include "authmsg.h"

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
				ret = 1;
				tmAuthAsk authAsk(u);
				Byte guid[40], passwd[40], accessLevel;
				int authResult;
				
				// get the data out of the packet
				msg->data->get(authAsk);
				log->log("<RCV> %s\n", authAsk.str());
				
				// authenticate player
				authResult = authenticatePlayer(authAsk.login.str(), authAsk.challenge, authAsk.hash, authAsk.release, (Byte *)alcGetStrIp(ntohl(authAsk.ip)), passwd, guid, &accessLevel);
				
				// send answer to client
				tmAuthResponse authResponse(u, authAsk, guid, passwd, authResult, accessLevel);
				u->send(authResponse);
				
				break;
		}
		return ret;
	}
	
	void tUnetAuthServer::calculateHash(Byte *login, Byte *passwd, Byte *challenge, Byte *hash) {
		tMD5Buf md5buffer;
		md5buffer.write(challenge, strlen((char *)challenge));
		md5buffer.write(login, strlen((char *)login));
		md5buffer.write(passwd, strlen((char *)passwd));
		md5buffer.compute();
		alcHex2Ascii(hash, md5buffer.read(16), 16);
	}
	
	int tUnetAuthServer::authenticatePlayer(Byte *login, Byte *challenge, Byte *hash, Byte release, Byte *ip, Byte *passwd,
			Byte *guid, Byte *accessLevel)
	{
		Byte correctHash[50];
		// TODO: query database instead of hardcoding values
		strcpy((char *)passwd, "76A2173BE6393254E72FFA4D6DF1030A"); // the md5sum of "passwd"
		strcpy((char *)guid, "7a9131b6-9dff-4103-b231-4887db6035b8");
		*accessLevel = 15;
		
		calculateHash(login, passwd, challenge, correctHash);
		if(strcmp((char *)hash, (char *)correctHash)) {
			return AInvalidPasswd;
		}
		
		return AAuthSucceeded;
	}

} //end namespace alc
