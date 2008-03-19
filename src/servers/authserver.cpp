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

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <unet.h>

////extra includes
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
		
		tmMsgBase auth(NetMsgCustomAuthAsk, plNetKi | plNetX | plNetCustom); // TODO: get the correct flags
		
		switch(msg->cmd) {
			case NetMsgCustomAuthAsk:
				log->log("<RCV> NetMsgCustomAuthAsk\n");
				msg->data->get(auth);
				// here we can now parse the additional stuff for this type of package, with e.g. msg->data->getByte()
				break;
		}
		return ret;
	}

} //end namespace alc

