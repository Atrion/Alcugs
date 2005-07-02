/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs H'uru Server Team                     *
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
	URUNET 3+
*/

/* CVS tag - DON'T TOUCH*/
#define __U_NETCORE_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "urunet/unet.h"

#include "alcdebug.h"

namespace alc {

tUnetBase::tUnetBase(char * lhost,U16 lport) :tUnet(lhost,lport) {
	state_running=true;
}

tUnetBase::~tUnetBase() {
	stop(5);
}

void tUnetBase::stop(Byte timeout) {
	stop_timeout=timeout;
	state_running=false;
}

void tUnetBase::terminate(tNetSessionIte & who) {
	//tNetEvent * ev=new tNetEvent(who,UNET_CLOSSING);
	tNetSession * u=getSession(who);
	//onConnectionClossing(ev);
	u->setPeerType(0);
	u->setTimeout(2);
	u->timestamp.seconds=alcGetTime();
	//delete ev;
}

//Blocks
void tUnetBase::run() {
	startOp();
	while(state_running) {
		Recv();
		
		tNetEvent * evt;
		tNetSession * u;
		
		tmTerminated * terminated;
		
		while((evt=getEvent())) {
			//lstd->log("Event id %i from host [%i]%s:%i\n",evt->id,evt->sid.sid,alcGetStrIp(evt->sid.ip),ntohs(evt->sid.port));+
			
			u=getSession(evt->sid);
			if(u==NULL) {
				delete evt;
				continue;
			}
			
			switch(evt->id) {
				case UNET_NEWCONN:
					sec->log("%s New Connection\n",u->str());
					onNewConnection(evt);
					u->setPeerType(1);
					break;
				case UNET_TIMEOUT:
					terminated=new tmTerminated(u,0,RKickedOff);
					u->send(*terminated);
					delete terminated;
					sec->log("%s Timeout\n",u->str());
					onConnectionTimeout(evt);
					if(!evt->veto) {
						if(u->getPeerType()==0) {
							sec->log("%s Ended\n",u->str());
							evt->id=UNET_TERMINATED;
							onConnectionClossed(evt);
							destroySession(evt->sid);
						} else {
							terminate(evt->sid);
						}
					}
					break;
				case UNET_FLOOD:
					sec->log("%s Flood Attack\n",u->str());
					onConnectionFlood(evt);
					if(!evt->veto) {
						//TODO SND terminated
					}
					break;
				case UNET_MSGRCV:
					dmalloc_verify(NULL);
					DBG(8,"SEGFAULT!??? BEGIN\n");
					log->log("%s New MSG Recieved\n",u->str());
					DBG(8,"SEGFAULT!??? END\n");
					u->rcvq->rewind();
					tUnetMsg * msg;
					msg=u->rcvq->getNext();
					while(msg!=NULL && msg->completed!=1)
						msg=u->rcvq->getNext();
					if(msg==NULL) break;
					onMsgRecieved(evt);
					u->rcvq->deleteCurrent();
					break;
				default:
					err->log("%s Unknown Event id %i\n",u->str(),evt->id);
					break;
			}
			
			delete evt;
		}

	}
	stopOp();
}




} //end namespace



