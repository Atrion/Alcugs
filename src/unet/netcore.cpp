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
	setTimer(2);
}

tUnetBase::~tUnetBase() {
	stop(5);
}

void tUnetBase::stop(Byte timeout) {
	stop_timeout=timeout;
	state_running=false;
}

void tUnetBase::terminate(tNetSessionIte & who,bool silent,Byte reason) {
	//tNetEvent * ev=new tNetEvent(who,UNET_CLOSSING);
	tNetSession * u=getSession(who);
	//onConnectionClossing(ev);
	if(!silent) {
		tmTerminated * terminated=new tmTerminated(u,u->ki,reason,true);
		u->send(*terminated);
		delete terminated;
	}

	u->setPeerType(0);
	u->setTimeout(3);
	u->timestamp.seconds=alcGetTime();
	//delete ev;
}

//Blocks
void tUnetBase::run() {
	startOp();
	
	tNetEvent * evt;
	tNetSession * u;
	int ret=0;
	/*
		0 - non parsed
		1 - parsed
		-1 - hack attempt
		-2 - parse error
	*/

	while(state_running) {
		Recv();
		
		//tmTerminated * terminated;
		
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
					onNewConnection(evt,u);
					//u->setPeerType(1);
					break;
				case UNET_TIMEOUT:
					sec->log("%s Timeout\n",u->str());
					onConnectionTimeout(evt,u);
					if(!evt->veto) {
						if(u->getPeerType()==0) {
							sec->log("%s Ended\n",u->str());
							evt->id=UNET_TERMINATED;
							onConnectionClossed(evt,u);
							destroySession(evt->sid);
						} else {
							terminate(evt->sid,false,RTimedOut);
						}
					}
					break;
				case UNET_FLOOD:
					sec->log("%s Flood Attack\n",u->str());
					onConnectionFlood(evt,u);
					if(!evt->veto) {
						//SND terminated
						terminate(evt->sid,false,RKickedOff);
					}
					break;
				case UNET_MSGRCV:
					dmalloc_verify(NULL);
					log->log("%s New MSG Recieved\n",u->str());
					u->rcvq->rewind();
					tUnetMsg * msg;
					msg=u->rcvq->getNext();
					while(msg!=NULL && msg->completed!=1)
						msg=u->rcvq->getNext();
					if(msg==NULL) break;
					ret=parseBasicMsg(evt,msg,u);
					if(ret==0) {
						ret=onMsgRecieved(evt,msg,u);
					}
					if(ret==0 || ret==-1) {
						err->log("%s Kicked off due to a parse error in a previus message\n",u->str());
						terminate(evt->sid,false,RKickedOff);
					}
					if(ret==-2) {
						sec->log("%s Kicked off due to cracking\n",u->str());
						terminate(evt->sid,false,RKickedOff);
					}
					u->rcvq->deleteCurrent();
					break;
				default:
					err->log("%s Unknown Event id %i\n",u->str(),evt->id);
					break;
			}
			
			delete evt;
		}

	}
	//terminating the service
	//U32 i;
	tNetSessionIte ite;
	smgr->rewind();
	while((u=smgr->getNext())) {
		ite=u->getIte();
		terminate(ite,false,RKickedOff);
	}
	
	U32 startup=getTime();
	idle=false;
	while(!idle && !smgr->empty() && (getTime()-startup)<stop_timeout) {
		updatetimer(100000);
		Recv();
		while((evt=getEvent())) {
			u=getSession(evt->sid);
			if(u==NULL) {
				delete evt;
				continue;
			}
			switch(evt->id) {
				case UNET_MSGRCV:
					u->rcvq->clear();
				case UNET_NEWCONN:
				case UNET_FLOOD:
					//terminate(evt->sid,false,RKickedOff);
					break;
				case UNET_TIMEOUT:
					sec->log("%s Ended\n",u->str());
					evt->id=UNET_TERMINATED;
					onConnectionClossed(evt,u);
					destroySession(evt->sid);
					break;
				default:
					err->log("%s Unknown Event id %i\n",u->str(),evt->id);
					break;
			}
			delete evt;
		}
	}
	
	if(!smgr->empty()) {
		err->log("Fatal, smgr is not empty!\n");
		smgr->rewind();
		while((u=smgr->getNext())) {
			evt=new tNetEvent(u->getIte(),UNET_TERMINATED);
			onConnectionClossed(evt,u);
			destroySession(evt->sid);
			delete evt;
		}
	}
	
	stopOp();
}

int tUnetBase::parseBasicMsg(tNetEvent * ev,tUnetMsg * msg,tNetSession * u) {

	int ret=0;
	
	tmLeave msgleave;

	switch(msg->cmd) {
		case NetMsgLeave:
			msg->data->get(msgleave);
			log->log("<RCV> %s\n",msgleave.str());
			terminate(ev->sid,true,msgleave.reason);
			ret=1;
			break;
		default:
			ret=0;
			break;
	}
	return ret;
}


} //end namespace



