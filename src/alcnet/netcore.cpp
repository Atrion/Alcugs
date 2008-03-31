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
	URUNET 3+
*/

/* CVS tag - DON'T TOUCH*/
#define __U_NETCORE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"

#ifndef __WIN32__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "alcdebug.h"

namespace alc {

tUnetBase::tUnetBase() :tUnet() {
	state_running=true;
	tStrBuf var;
	tConfig * cfg;
	cfg=alcGetConfig();
	var=cfg->getVar("port","global");
	if(!var.isNull()) {
		setBindPort(var.asU16());
	}
	var=cfg->getVar("bind","global");
	if(!var.isNull()) {
		setBindAddress(var.c_str());
	}
	_reconfigure();
}

tUnetBase::~tUnetBase() {
	stop(5);
}

void tUnetBase::_reconfigure() {
	tUnetSignalHandler * h = new tUnetSignalHandler(this);
	DBG(5,"tUnetBase - installing signal handler\n");
	alcInstallSignalHandler(h);
	// re-load configuration
	tStrBuf var;
	tConfig * cfg;
	cfg=alcGetConfig();
	//Sets the idle timer
	var=cfg->getVar("net.timer","global");
	if(var.isNull()) {
		setTimer(5); // it should be max. 10 seconds (default setting in the netcore, bigger timers have issues)
	} else {
		setTimer(var.asByte());
	}
	//Set pool size
	var=cfg->getVar("net.pool.size","global");
	if(var.isNull()) {
		pool_size=4; //Set up 4 worker threads by default (may be changed)
	} else {
		pool_size=var.asByte();
	}
	#ifndef ENABLE_THREADS
	pool_size=1;
	#endif
	if(pool_size==0) pool_size=1;
	var=cfg->getVar("net.maxconnections","global");
	if(!var.isNull()) {
		max=var.getU32();
	}
	var=cfg->getVar("net.timeout","global");
	if(!var.isNull()) {
		conn_timeout=var.asU32();
	}
	var=cfg->getVar("net.up","global");
	if(!var.isNull()) {
		nat_up=var.asU32();
	}
	var=cfg->getVar("net.down","global");
	if(!var.isNull()) {
		nat_down=var.asU32();
	}
	var=cfg->getVar("net.lan.up","global");
	if(!var.isNull()) {
		lan_up=var.asU32();
	}
	var=cfg->getVar("net.lan.down","global");
	if(!var.isNull()) {
		lan_down=var.asU32();
	}
	//"public_address"
	var=cfg->getVar("private_mask","global");
	if(!var.isNull()) {
		lan_mask=(U32)inet_addr((const char *)var.c_str());
	}
	var=cfg->getVar("private_network","global");
	if(!var.isNull()) {
		lan_addr=(U32)inet_addr((const char *)var.c_str());
	} else {
		struct hostent *host;
		host=gethostbyname((const char *)bindaddr);
		if(host!=NULL) {
			lan_addr=*(U32 *)host->h_addr_list[0] & lan_mask;
		} else {
			lan_addr=0;
		}
	}
	//"spawn.start"
	//"spawn.stop"
	var=cfg->getVar("net.noflood","global");
	if(!var.isNull()) {
		if(var.asByte()) {
			setFlags(UNET_NOFLOOD);
		} else {
			unsetFlags(UNET_NOFLOOD);
		}
	}
	//protocol (auth,vault,tracking)
	//Other DEVEL vars (dangerous to touch)
	var=cfg->getVar("net.flood_check_sec","global");
	if(!var.isNull()) {
		flood_check_sec=var.asU32();
	}
	var=cfg->getVar("net.max_flood_pkts","global");
	if(!var.isNull()) {
		max_flood_pkts=var.asU32();
	}
	var=cfg->getVar("net.snd_expire","global");
	if(!var.isNull()) {
		snd_expire=var.asU32();
	}
	#ifdef _UNET_DBG_
	var=cfg->getVar("net.lim_down_cap","global");
	if(!var.isNull()) {
		lim_down_cap=var.asU32();
	}
	var=cfg->getVar("net.lim_up_cap","global");
	if(!var.isNull()) {
		lim_up_cap=var.asU32();
	}
	var=cfg->getVar("net.in_noise","global");
	if(!var.isNull()) {
		in_noise=var.asU32();
	}
	var=cfg->getVar("net.out_noise","global");
	if(!var.isNull()) {
		out_noise=var.asU32();
	}
	var=cfg->getVar("net.latency","global");
	if(!var.isNull()) {
		latency=var.asU32();
	}
	var=cfg->getVar("net.quota_check_sec","global");
	if(!var.isNull()) {
		quota_check_sec=var.asU32();
	}
	var=cfg->getVar("net.quota_check_usec","global");
	if(!var.isNull()) {
		quota_check_usec=var.asU32();
	}
	#endif

}

void tUnetBase::stop(SByte timeout) {
	if(timeout<0) {
		tStrBuf var;
		tConfig * cfg=alcGetConfig();
		var=cfg->getVar("net.stop.timeout","global");
		if(var.isNull()) {
			stop_timeout=15;
		} else {
			stop_timeout=var.asU32();
		}
	}
	stop_timeout=timeout;
	state_running=false;
}

void tUnetBase::terminate(tNetSessionIte & who,Byte reason, bool silent) {
	tNetSession * u=getSession(who);
	if (!silent && u->client) {
		tmTerminated * terminated=new tmTerminated(u,u->ki,reason,true);
		u->send(*terminated);
		delete terminated;
		
		/* We sent a NetMsgTerminated, the peer should answer with a NetMsgLeave which will trigger below if block.
		However, the unet3 lobby doesn't do that. it sends a NetMsgAlive instead. To cope with that, this session will be remembered as
		terminated and if a message different than NetMsgLeave is recieved, it will be deleted ASAP. The same happens if there's a
		timeout, i.e. the peer sends nothing within 3 seconds. */
	}
	else if (!silent && !u->client) {
		tmLeave leave(u,u->ki,reason);
		u->send(leave);
	}
	
	if (u->client && !u->terminated) // give clients 3 seconds time to send their NetMsgLeave, but only if they haven't already got a NetMsgTerminated before
		u->setTimeout(3);
	else // otherwise, there's time to send remaining messages
		u->setTimeout(2);
	u->terminated = true;
	u->timestamp.seconds = alcGetTime();
}

void tUnetBase::closeConnection(tNetSession *u)
{
	sec->log("%s Ended\n",u->str());
	tNetEvent * ev=new tNetEvent(u->getIte(),UNET_TERMINATED);
	onConnectionClosed(ev,u);
	destroySession(ev->sid);
	delete ev;
}

//Blocks
void tUnetBase::run() {
	startOp();
	
	tNetEvent * evt;
	tNetSession * u;
	tUnetMsg * msg;
	int ret=0;
	/*
		0 - non parsed
		1 - parsed
		-1 - hack attempt
		-2 - parse error
	*/

	onStart();
	while(state_running) {
		Recv();
		
		while((evt=getEvent())) {
			u=getSession(evt->sid);
			if(u==NULL) {
				delete evt;
				continue;
			}
			
			switch(evt->id) {
				case UNET_NEWCONN:
					sec->log("%s New Connection\n",u->str());
					onNewConnection(evt,u);
					break;
				case UNET_TIMEOUT:
					if (!u->terminated) {
						sec->log("%s Timeout\n",u->str());
						onConnectionTimeout(evt,u);
						if(!evt->veto)
							terminate(evt->sid,RTimedOut);
					}
					else {
						if (u->client) err->log("ERR: NetMsgLeave not sent by peer %s\n", u->str());
						closeConnection(u);
					}
					break;
				case UNET_FLOOD:
					sec->log("%s Flood Attack\n",u->str());
					onConnectionFlood(evt,u);
					if(!evt->veto) {
						//SND terminated
						terminate(evt->sid,RKickedOff);
					}
					break;
				case UNET_MSGRCV:
					//dmalloc_verify(NULL);
					#ifdef _DEBUG_PACKETS_
					log->log("%s New MSG Recieved\n",u->str());
					#endif
					u->rcvq->rewind();
					msg=u->rcvq->getNext();
					while(msg!=NULL && msg->completed!=1)
						msg=u->rcvq->getNext();
					if(msg==NULL) break;
					
					ret=parseBasicMsg(evt,msg,u);
					// terminated sessions can be deleted here - either it was a NetMsgLeave and everything is fine, or it was an invalid message
					if (u->terminated) {
						if (ret == 0) err->log("ERR: Peer %s is terminated and sent a non-NetMsgLeave message %04X (%s)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
						u->rcvq->clear();
						if (u->sndq->isEmpty()) // delete only if send queue isn't empty
							closeConnection(u);
						break;
					}
					if(ret==0) {
						try {
							ret=onMsgRecieved(evt,msg,u);
							if (ret > 0 && msg->data->remaining() > 0) {
								err->log("%s Recieved a message %04X (%s) which was too long (%d Bytes remaining after parsing)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd), msg->data->remaining());
								ret=-2;
							}
						}
						catch (txOutOfRange &t) { // when there was an out of range error, don't crash the whole server (it would be easy to crash then...) but kick the responsible client
							err->log("%s Recieved a message %04X (%s) which was too short (error txOutOfRange)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
							ret=-2;
						}
					}
					if(u->client==1) {
						if(ret==0) {
							err->log("%s Unexpected message %04X (%s)\n",u->str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
							terminate(evt->sid,RUnimplemented);
						}
						else if(ret==-1) {
							err->log("%s Kicked off due to a parse error in a previus message %04X (%s)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
							terminate(evt->sid,RParseError);
						}
						else if(ret==-2) {
							sec->log("%s Kicked off due to cracking %04X (%s)\n",u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
							terminate(evt->sid,RHackAttempt);
						}
					} else {
						if(ret!=1) {
							err->log("%s Error code %i parsing message %04X (%s)\n",u->str(),ret,msg->cmd,alcUnetGetMsgCode(msg->cmd));
						}
					}
					u->rcvq->deleteCurrent();
					break;
				default:
					err->log("%s Unknown Event id %i\n",u->str(),evt->id);
					break;
			}
			log->flush(); err->flush(); sec->flush(); unx->flush(); // I don't know how much perforcmance this costs, but without flushing it's not possible to follow the server logs using tail -f
			delete evt;
		}
		onIdle(idle);
	}
	//terminating the service
	//U32 i;
	tNetSessionIte ite;
	smgr->rewind();
	while((u=smgr->getNext())) {
		ite=u->getIte();
		if (!u->terminated) { // avoid sending a NetMsgLeave or NetMsgTerminate to terminated peers
			terminate(ite, u->client ? RKickedOff : RQuitting);
		}
	}
	
	U32 startup=getTime();
	while(!smgr->empty() && (getTime()-startup)<stop_timeout) {
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
					u->rcvq->rewind();
					msg=u->rcvq->getNext();
					while(msg!=NULL && msg->completed!=1)
						msg=u->rcvq->getNext();
					if(msg!=NULL && msg->cmd==NetMsgLeave) {
						tmLeave msgleave;
						msg->data->get(msgleave);
						log->log("<RCV> %s\n",msgleave.str());
					}
					else if (msg!=NULL && u->terminated) // if the connection was already terminated and we got a message anyway, delete it
						err->log("ERR: Peer %s is terminated and sent non-NetMsgLeave message\n", u->str());
					u->rcvq->clear();
					if (u->sndq->isEmpty()) // delete only if send queue isn't empty
						closeConnection(u);
					break;
				case UNET_NEWCONN:
				case UNET_FLOOD:
					terminate(evt->sid,RKickedOff);
					break;
				case UNET_TIMEOUT:
					if (!u->terminated) {
						sec->log("%s Timeout\n",u->str());
						terminate(evt->sid,RTimedOut);
					}
					else {
						if (u->client) err->log("ERR: NetMsgLeave not sent by peer %s\n", u->str());
						closeConnection(u);
					}
					break;
				default:
					err->log("%s Unknown Event id %i\n",u->str(),evt->id);
					break;
			}
			delete evt;
		}
	}
	
	if(!smgr->empty()) {
		err->log("ERR: Session manager is not empty!\n");
		smgr->rewind();
		while((u=smgr->getNext())) {
			closeConnection(u);
		}
	}
	
	onStop();
	
	log->log("INF: Service sanely terminated\n");
	stopOp();

}

int tUnetBase::parseBasicMsg(tNetEvent * ev,tUnetMsg * msg,tNetSession * u)
{
	switch(msg->cmd) {
		case NetMsgLeave:
		{
			if (!u->client) return 1;
			tmLeave msgleave;
			msg->data->get(msgleave);
			log->log("<RCV> %s\n",msgleave.str());
			ev->id=UNET_TERMINATED;
			onLeave(ev,msgleave.reason,u);
			terminate(ev->sid, msgleave.reason, true);
			return 1;
		}
		case NetMsgTerminated:
		{
			if (u->client) return 1;
			tmTerminated msgterminated;
			msg->data->get(msgterminated);
			log->log("<RCV> %s\n",msgterminated.str());
			ev->id=UNET_TERMINATED;
			onTerminated(ev,msgterminated.reason,u);
			terminate(ev->sid,msgterminated.reason);
			return 1;
		}
	}
	return 0;
}
void tUnetBase::installSignalHandler() {
	
}

} //end namespace



