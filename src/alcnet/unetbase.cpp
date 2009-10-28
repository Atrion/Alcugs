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
#define __U_UNETBASE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcnet.h"

#ifndef __WIN32__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <alcdebug.h>

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
	reconfigure();
}

tUnetBase::~tUnetBase() {
	forcestop();
}

void tUnetBase::reconfigure() {
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
		setIdleTimer(var.asByte());
	}
#ifdef ENABLE_THREADS
	//Set pool size
	var=cfg->getVar("net.pool.size","global");
	if(var.isNull()) {
		pool_size=4; //Set up 4 worker threads by default (may be changed)
	} else {
		pool_size=var.asByte();
		if(pool_size<=0) pool_size=1;
	}
#endif
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
	var=cfg->getVar("private_mask","global");
	if(!var.isNull()) {
		lan_mask=(U32)inet_addr(var.c_str());
	}
	var=cfg->getVar("private_network","global");
	if(!var.isNull()) {
		lan_addr=(U32)inet_addr(var.c_str());
	} else {
		struct hostent *host;
		host=gethostbyname(bindaddr);
		if(host!=NULL) {
			lan_addr=*(U32 *)host->h_addr_list[0] & lan_mask;
		} else {
			lan_addr=0;
		}
	}
	var=cfg->getVar("net.noflood","global");
	if(!var.isNull()) {
		if(var.asByte()) {
			setFlags(UNET_NOFLOOD);
		} else {
			unsetFlags(UNET_NOFLOOD);
		}
	}
	var=cfg->getVar("net.log.ack","global");
	if(!var.isNull()) {
		if(var.asByte()) {
			setFlags(UNET_ACKLOG);
		} else {
			unsetFlags(UNET_ACKLOG);
		}
	}
	//Other DEVEL vars (dangerous to touch)
	var=cfg->getVar("broadcast","global");
	if(!var.isNull()) {
		if(var.asByte()) {
			unsetFlags(UNET_BCAST);
		} else {
			setFlags(UNET_BCAST);
		}
	}
	var=cfg->getVar("net.flood_check_sec","global");
	if(!var.isNull()) {
		flood_check_sec=var.asU32();
	}
	var=cfg->getVar("net.max_flood_pkts","global");
	if(!var.isNull()) {
		max_flood_pkts=var.asU32();
	}
	var=cfg->getVar("net.receive_ahead","global");
	if(!var.isNull()) {
		receiveAhead=var.asU32();
	}
	#ifdef ENABLE_NETDEBUG
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

void tUnetBase::terminate(tNetSession *u, Byte reason, bool gotLeave)
{
	bool destroy = gotLeave || u->isTerminated();
	if (!reason) reason = u->isClient() ? RKickedOff : RQuitting;
	if (!destroy) { // don't send message again if we already sent it
		if (u->isClient()) {
			tmTerminated terminated(u,u->ki,reason);
			send(terminated);
			/* We sent a NetMsgTerminated, the peer should answer with a NetMsgLeave which will correctly end the connection. */
		}
		else {
			tmLeave leave(u,u->ki,reason);
			send(leave);
			/* We left, nothing to be done for us anymore */
			destroy = true; // destroy this session ASAP
		}
	}
	
	if (destroy) { // if the session should be destroyed, do that ASAP
		u->terminate(0/*seconds*/);
	} else { // otherwise, give the session one second to send remaining messages
		u->terminate(1/*second*/);
	}
}

void tUnetBase::terminateAll(void)
{
	tNetSession *u;
	smgr->rewind();
	while ((u=smgr->getNext())) { // double brackets to suppress gcc warning
		if (!u->isTerminated()) // avoid sending a NetMsgLeave or NetMsgTerminate to terminated peers
			terminate(u);
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
				terminate(u);
			else
				onNewConnection(evt, u);
			sec->log("%s New Connection\n",u->str());
			break;
		case UNET_TIMEOUT:
			if (!u->isTerminated() && !shutdown) {
				onConnectionTimeout(evt,u);
				if(!evt->veto)
					terminate(u, RTimedOut);
			}
			else { // a destroyed session, close it
				closeConnection(u);
			}
			break;
		case UNET_FLOOD:
			sec->log("%s Flood Attack\n",u->str());
			if (!shutdown)
				onConnectionFlood(evt,u);
			if (shutdown || !evt->veto) {
				terminate(u);
				lerr->log("%s kicked due to a Flood Attack\n", u->str());
			}
			break;
		case UNET_MSGRCV:
		{
			tUnetMsg * msg = evt->msg;
			int ret = 0; // 0 - non parsed; 1 - parsed; 2 - ignored; -1 - parse error; -2 - hack attempt
			#ifdef ENABLE_MSGDEBUG
			log->log("%s New MSG Recieved\n",u->str());
			#endif
			assert(msg!=NULL);
			try {
				ret=parseBasicMsg(evt,msg,u,shutdown);
				// terminated sessions can be deleted here - either it was a NetMsgLeave and everything is fine, or it was an invalid message
				if (u->isTerminated() || shutdown) {
					if (ret != 1) {
						err->log("%s is terminated and sent a non-NetMsgLeave message 0x%04X (%s)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
						terminate(u); // this will delete the session ASAP as it already is terminated
					}
					else if (ret == 1 && !msg->data.eof() > 0) { // packet was processed and there are bytes left
						err->log("%s Recieved a message 0x%04X (%s) which was too long (%d Bytes remaining after parsing)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd), msg->data.remaining());
						terminate(u); // this will delete the session ASAP as it already is terminated
					}
					break;
				}
				// this part can never be reached on shutdown, so messages are only processed when the server is still fully running
				if (ret == 0) ret=onMsgRecieved(evt,msg,u);
				if (ret == 1 && !msg->data.eof() > 0) { // packet was processed and there are bytes left, obiously invalid, terminate the client
					err->log("%s Recieved a message 0x%04X (%s) which was too long (%d Bytes remaining after parsing) - kicking player\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd), msg->data.remaining());
					ret=-1;
				}
			}
			catch (txBase &t) { // if there was an error parsing the message, kick the responsible player
				err->log("%s Recieved invalid 0x%04X (%s) - kicking player\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
				err->log(" Exception details: %s\n",t.what());
				//err->log(" Backtrace: %s\n", t.backtrace());
				ret=-1;
			}
			if(u->isClient()==1) {
				if(ret==0) {
					err->log("%s Unexpected message %04X (%s) - kicking player\n",u->str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
					sec->log("%s Unexpected message %04X (%s)\n",u->str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
					terminate(u, RUnimplemented);
				}
				else if(ret==-1) {
					// the problem already got printed to the error log wherever this return value was set
					sec->log("%s Kicked off due to a parse error in a previus message 0x%04X (%s)\n", u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
					terminate(u, RParseError);
				}
				else if(ret==-2) {
					// the problem already got printed to the error log wherever this return value was set
					sec->log("%s Kicked off due to cracking 0x%04X (%s)\n",u->str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
					terminate(u, RHackAttempt);
				}
			} else {
				if(ret<=0) {
					err->log("%s Error code %i parsing message 0x%04X (%s)\n",u->str(),ret,msg->cmd,alcUnetGetMsgCode(msg->cmd));
				}
			}
			break;
		}
		default:
			throw txBase(_WHERE("%s Unknown Event id %i\n",u->str(),evt->id));
	}
	log->flush(); err->flush(); sec->flush(); // I don't know how much perforcmance this costs, but without flushing it's not possible to follow the server logs using tail -F
}

// main event processing loop - blocks
void tUnetBase::run() {
	onLoadConfig();
	startOp();
	onStart();
	
	tNetEvent * evt;
	tNetSession * u;

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
		updateTimerRelative(100000); // make sure we don't wait longer than this
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
	onUnloadConfig();
	log->log("INF: Service sanely terminated\n"); // stopOp closes the log files, so print this before calling stopOp
	stopOp();
}

int tUnetBase::parseBasicMsg(tNetEvent * ev,tUnetMsg * msg,tNetSession * u,bool shutdown)
{
	switch(msg->cmd) {
		case NetMsgLeave:
		{
			// accept it even if it is NOT a client - in that case, the peer obviously thinks it is a client, so lets respect its wish, it doesn't harm
			tmLeave msgleave(u);
			msg->data.get(msgleave);
			log->log("<RCV> [%d] %s\n",msg->sn,msgleave.str());
			if (!shutdown && !u->isTerminated()) {
				ev->id=UNET_TERMINATED;
				onLeave(ev,msgleave.reason,u);
			}
			/* The peer left, so there is nothing we have to do anymore, just remove it */
			terminate(u, msgleave.reason, /*gotLeave*/true); // this will delete the session ASAP
			return 1;
		}
		case NetMsgTerminated:
		{
			// accept it even if it IS a client - in that case, the peer obviously thinks it is a server, so lets respect its wish, it doesn't harm
			tmTerminated msgterminated(u);
			msg->data.get(msgterminated);
			log->log("<RCV> [%d] %s\n",msg->sn,msgterminated.str());
			ev->id=UNET_TERMINATED;
			onTerminated(ev,msgterminated.reason,u);
			/* The peer wants to terminate, so tell him we are ready to leave */
			terminate(u);
			return 1;
		}
		case NetMsgAlive:
		{
			if (u->isTerminated() || shutdown) return 0; // don't accept a NetMsgAlive on already terminated sessions
			tmAlive alive(u);
			msg->data.get(alive);
			log->log("<RCV> [%d] %s\n",msg->sn,alive.str());
			return 1;
		}
	}
	return 0;
}

} //end namespace
