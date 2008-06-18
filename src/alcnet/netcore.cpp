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
	if(!var.isNull()) {
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
	else
		stop_timeout=timeout;
	state_running=false;
}

void tUnetBase::terminate(tNetSession *u,Byte reason, bool destroyOnly)
{
	if (!destroyOnly && u->client) {
		tmTerminated terminated(u,u->ki,reason);
		send(terminated);
		
		/* We sent a NetMsgTerminated, the peer should answer with a NetMsgLeave which will trigger below if block.
		However, the unet3 lobby doesn't do that. it sends a NetMsgAlive instead. To cope with that, this session will be remembered as
		terminated and if a message different than NetMsgLeave is recieved, it will be deleted ASAP. The same happens if there's a
		timeout, i.e. the peer sends nothing within 3 seconds. */
	}
	else if (!destroyOnly && !u->client) {
		tmLeave leave(u,u->ki,reason);
		send(leave);
	}
	
	if (destroyOnly) // if the session should be destroyed, do that ASAP
		u->setTimeout(0);
	else if (u->client && !u->terminated) // give clients 3 seconds time to send their NetMsgLeave, but only if they haven't already got a NetMsgTerminated before
		u->setTimeout(3);
	else // otherwise, give the session one second to send remaining messages
		u->setTimeout(1);
	u->terminated = true;
	u->timestamp.seconds = alcGetTime();
}

void tUnetBase::terminateAll(void)
{
	tNetSessionIte ite;
	tNetSession *u;
	smgr->rewind();
	while ((u=smgr->getNext())) { // double brackets to suppress gcc warning
		if (!u->terminated) // avoid sending a NetMsgLeave or NetMsgTerminate to terminated peers
			terminate(u, u->client ? RKickedOff : RQuitting);
	}
}

void tUnetBase::closeConnection(tNetSession *u)
{
	sec->log("%s Ended\n",u->str());
	tNetEvent * ev=new tNetEvent(u->getIte(),UNET_TERMINATED);
	onConnectionClosed(ev,u);
	destroySession(ev->sid);
	delete ev;
}

void tUnetBase::processEvent(tNetEvent *evt, tNetSession *u, bool shutdown)
{
	switch(evt->id) {
		case UNET_NEWCONN:
			if (shutdown)
				terminate(u, RKickedOff);
			else
				onNewConnection(evt, u);
			sec->log("%s New Connection\n",u->str());
			break;
		case UNET_TIMEOUT:
			if (!u->terminated && !shutdown) {
				sec->log("%s Timeout\n",u->str());
				onConnectionTimeout(evt,u);
				if(!evt->veto)
					terminate(u, RTimedOut);
			}
			else { // a destroyed session, close it ASAP
				if (u->sndq->isEmpty() && u->ackq->isEmpty())
					closeConnection(u);
				else // if the send or ack queue isn't empty, send remaining messages
					u->doWork();
					// I know it's ugly to call this here, but I found no other way: tUnet::doWork will create another timeout event instead of sending the messages
			}
			break;
		case UNET_FLOOD:
			sec->log("%s Flood Attack\n",u->str());
			if (!shutdown)
				onConnectionFlood(evt,u);
			if (shutdown || !evt->veto) {
				terminate(u, RKickedOff);
			}
			break;
		case UNET_MSGRCV:
		{
			tUnetMsg * msg;
			int ret = 0; // 0 - non parsed; 1 - parsed; -1 - hack attempt; -2 - parse error
			#ifdef _DEBUG_PACKETS_
			log->log("%s New MSG Recieved\n",u->str());
			#endif
			u->rcvq->rewind();
			msg=u->rcvq->getNext();
			while(msg!=NULL && msg->completed!=1)
				msg=u->rcvq->getNext();
			if(msg==NULL) break;
			
			try {
				ret=parseBasicMsg(evt,msg,u,shutdown);
				// terminated sessions can be deleted here - either it was a NetMsgLeave and everything is fine, or it was an invalid message
				if (u->terminated || shutdown) {
					if (ret == 0) err->log("%s is terminated and sent a non-NetMsgLeave message %04X (%s)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
					u->rcvq->deleteCurrent();
					terminate(u, RKickedOff, true); // delete the session ASAP
					break;
				}
				// this part can never be reached on shutdown, so messages are only processed when the server is still fully running
				if (ret == 0) ret=onMsgRecieved(evt,msg,u);
				if (ret > 0 && !msg->data->eof() > 0) { // when the packet was processed and there are bytes left, it is obiously invalid, terminate the client (ret = -2, hack attempt, processed below)
					err->log("%s Recieved a message %04X (%s) which was too long (%d Bytes remaining after parsing)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd), msg->data->remaining());
					ret=-2;
				}
			}
			catch (txOutOfRange &t) { // when there was an out of range error, don't crash the whole server (it would be easy to crash then...) but kick the responsible client
				err->log("%s Recieved a message %04X (%s) which was too short (error txOutOfRange)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
				err->log(" Exception details: %s\n%s\n",t.what(),t.backtrace());
				ret=-2;
			}
			catch (txProtocolError &t) {
				err->log("%s Recieved invalid %04X (%s)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
				err->log(" Exception details: %s\n%s\n",t.what(),t.backtrace());
				ret=-2;
			}
			if(u->client==1) {
				if(ret==0) {
					err->log("%s Unexpected message %04X (%s)\n",u->str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
					terminate(u, RUnimplemented);
				}
				else if(ret==-1) {
					err->log("%s Kicked off due to a parse error in a previus message %04X (%s)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
					terminate(u, RParseError);
				}
				else if(ret==-2) {
					sec->log("%s Kicked off due to cracking %04X (%s)\n",u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
					terminate(u, RHackAttempt);
				}
			} else {
				if(ret!=1) {
					err->log("%s Error code %i parsing message %04X (%s)\n",u->str(),ret,msg->cmd,alcUnetGetMsgCode(msg->cmd));
				}
			}
			u->rcvq->deleteCurrent();
			break;
		}
		default:
			err->log("%s Unknown Event id %i\n",u->str(),evt->id);
			break;
	}
	log->flush(); err->flush(); sec->flush(); // I don't know how much perforcmance this costs, but without flushing it's not possible to follow the server logs using tail -F
}

// main event processing loop - blocks
void tUnetBase::run() {
	startOp();
	
	tNetEvent * evt;
	tNetSession * u;

	onStart();
	while(state_running) {
		Recv();
		while((evt=getEvent())) {
			u=getSession(evt->sid);
			if (u != NULL)
				processEvent(evt, u);
			delete evt;
		}
		onIdle(idle);
	}
	
	//terminating the service
	terminateAll();
	
	U32 startup=getTime();
	while(!smgr->empty() && (getTime()-startup)<stop_timeout) {
		updatetimer(100000);
		Recv();
		while((evt=getEvent())) {
			u=getSession(evt->sid);
			if (u != NULL)
				processEvent(evt, u, true);
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

int tUnetBase::parseBasicMsg(tNetEvent * ev,tUnetMsg * msg,tNetSession * u,bool shutdown)
{
	switch(msg->cmd) {
		case NetMsgLeave:
		{
			if (!u->client) return 1;
			tmLeave msgleave(u);
			msg->data->get(msgleave);
			log->log("<RCV> %s\n",msgleave.str());
			ev->id=UNET_TERMINATED;
			if (!shutdown && !u->terminated)
				onLeave(ev,msgleave.reason,u);
			terminate(u, msgleave.reason, true); // delete the session ASAP
			return 1;
		}
		case NetMsgTerminated:
		{
			if (u->terminated || shutdown) return 0; // don't accept a NetMsgTerminated on already terminated sessions
			if (u->client) return 1;
			tmTerminated msgterminated(u);
			msg->data->get(msgterminated);
			log->log("<RCV> %s\n",msgterminated.str());
			ev->id=UNET_TERMINATED;
			onTerminated(ev,msgterminated.reason,u);
			terminate(u,msgterminated.reason);
			return 1;
		}
	}
	return 0;
}

} //end namespace



