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

tUnetBase::tUnetBase(Byte whoami) :tUnet(whoami), running(true) {
	alcUnetGetMain()->setNet(this);
	tString var;
	tConfig * cfg;
	cfg=alcGetMain()->config();
	var=cfg->getVar("port","global");
	if(!var.isEmpty()) {
		setBindPort(var.asU16());
	}
	var=cfg->getVar("bind","global");
	if(!var.isEmpty()) {
		setBindAddress(var.c_str());
	}
}

tUnetBase::~tUnetBase() {
	forcestop();
	alcUnetGetMain()->setNet(NULL);
}

void tUnetBase::applyConfig() {
	openLogfiles(); // (re-)open basic logfiles
	// re-load configuration
	tString var;
	tConfig * cfg;
	cfg=alcGetMain()->config();
	//Sets the idle timer
	var=cfg->getVar("net.timer","global");
	if(!var.isEmpty()) {
		setIdleTimer(var.asByte());
	}
#ifdef ENABLE_THREADS
	//Set pool size
	var=cfg->getVar("net.pool.size","global");
	if(var.isEmpty()) {
		pool_size=4; //Set up 4 worker threads by default (may be changed)
	} else {
		pool_size=var.asByte();
		if(pool_size<=0) pool_size=1;
	}
#endif
	var=cfg->getVar("net.maxconnections","global");
	if(!var.isEmpty()) {
		max=var.getU32();
	}
	var=cfg->getVar("net.timeout","global");
	if(!var.isEmpty()) {
		conn_timeout=var.asU32();
	}
	var=cfg->getVar("net.up","global");
	if(!var.isEmpty()) {
		nat_up=var.asU32();
	}
	var=cfg->getVar("net.down","global");
	if(!var.isEmpty()) {
		nat_down=var.asU32();
	}
	var=cfg->getVar("net.lan.up","global");
	if(!var.isEmpty()) {
		lan_up=var.asU32();
	}
	var=cfg->getVar("net.lan.down","global");
	if(!var.isEmpty()) {
		lan_down=var.asU32();
	}
	var=cfg->getVar("private_mask","global");
	if(!var.isEmpty()) {
		lan_mask=inet_addr(var.c_str());
	}
	var=cfg->getVar("private_network","global");
	if(!var.isEmpty()) {
		lan_addr=inet_addr(var.c_str());
	} else {
		struct hostent *host;
		host=gethostbyname(bindaddr.c_str());
		if(host!=NULL) {
			lan_addr=*host->h_addr_list[0] & lan_mask;
		} else {
			lan_addr=0;
		}
	}
	var=cfg->getVar("net.noflood","global");
	if(!var.isEmpty()) {
		if(var.asByte()) {
			setFlags(UNET_FLOODCTR);
		} else {
			unsetFlags(UNET_FLOODCTR);
		}
	}
	var=cfg->getVar("net.log.ack","global");
	if(!var.isEmpty()) {
		if(var.asByte()) {
			setFlags(UNET_EACKLOG);
		} else {
			unsetFlags(UNET_EACKLOG);
		}
	}
	//Other DEVEL vars (dangerous to touch)
	var=cfg->getVar("broadcast","global");
	if(!var.isEmpty()) {
		if(var.asByte()) {
			unsetFlags(UNET_BCAST);
		} else {
			setFlags(UNET_BCAST);
		}
	}
	var=cfg->getVar("net.flood_check_sec","global");
	if(!var.isEmpty()) {
		flood_check_sec=var.asU32();
	}
	var=cfg->getVar("net.max_flood_pkts","global");
	if(!var.isEmpty()) {
		max_flood_pkts=var.asU32();
	}
	var=cfg->getVar("net.receive_ahead","global");
	if(!var.isEmpty()) {
		receiveAhead=var.asU32();
	}
	#ifdef ENABLE_NETDEBUG
	var=cfg->getVar("net.lim_down_cap","global");
	if(!var.isEmpty()) {
		lim_down_cap=var.asU32();
	}
	var=cfg->getVar("net.lim_up_cap","global");
	if(!var.isEmpty()) {
		lim_up_cap=var.asU32();
	}
	var=cfg->getVar("net.in_noise","global");
	if(!var.isEmpty()) {
		in_noise=var.asU32();
	}
	var=cfg->getVar("net.out_noise","global");
	if(!var.isEmpty()) {
		out_noise=var.asU32();
	}
	var=cfg->getVar("net.latency","global");
	if(!var.isEmpty()) {
		latency=var.asU32();
	}
	var=cfg->getVar("net.quota_check_sec","global");
	if(!var.isEmpty()) {
		quota_check_sec=var.asU32();
	}
	var=cfg->getVar("net.quota_check_usec","global");
	if(!var.isEmpty()) {
		quota_check_usec=var.asU32();
	}
	#endif
	// forward to sub-classes
	onApplyConfig();
}

void tUnetBase::stop(SByte timeout) {
	if(timeout<0) {
		tString var;
		tConfig * cfg=alcGetMain()->config();
		var=cfg->getVar("net.stop.timeout","global");
		if(var.isEmpty()) {
			stop_timeout=15;
		} else {
			stop_timeout=var.asU32();
		}
	}
	else
		stop_timeout=timeout;
	running=false;
}

void tUnetBase::terminate(tNetSession *u, Byte reason, bool gotEndMsg)
{
	bool wasTerminated = u->isTerminated(); // if the client was already terminated, go ahead and kill it quickly
	if (!reason) reason = u->isClient() ? RKickedOff : RQuitting;
	if (!wasTerminated && !gotEndMsg) { // don't send message again if we already sent it, or if we got the message from the other side
		if (u->isClient() || u->getPeerType() == KClient) { // a KClient will ignore us sending a leave, so send a terminated even if the roles changed
			tmTerminated terminated(u,u->ki,reason);
			send(terminated);
		}
		else {
			tmLeave leave(u,u->ki,reason);
			send(leave);
		}
		// Now taht we sent that message, the other side must ack it, then tNetSession will set the timeout to 0
	}
	
	if (!wasTerminated) // don't trigger the event twice
		onConnectionClosing(u, reason);
	
	if (wasTerminated) { // see above, we already sent the message, so don't wait any longer
		log->log("%s This connection is already terminated, don't wait any longer\n", u->str().c_str());
		u->terminate(0/*seconds*/);
	} else { // otherwise, wait some time to send/receive the ack
		u->terminate(2/*second*/);
		// In the case that the leave is re-sent because the ack was not received, we are in trouble... the other side will see that as a new connection. tNetSession halves the timeout when the ack is sent, if we set it to 2, there is another second in which re-sends are accepted. */
	}
}

void tUnetBase::terminateAll(bool playersOnly)
{
	tNetSession *u;
	smgr->rewind();
	while ((u=smgr->getNext())) { // double brackets to suppress gcc warning
		if (!u->isTerminated() && (!playersOnly ||  u->getPeerType() == KClient)) // avoid sending a NetMsgLeave or NetMsgTerminate to terminated peers
			terminate(u);
	}
}

void tUnetBase::removeConnection(tNetSession *u)
{
	sec->log("%s Ended\n",u->str().c_str());
	tNetEvent * ev=new tNetEvent(u->getIte(),UNET_TERMINATED);
	destroySession(ev->sid);
	delete ev;
}

void tUnetBase::processEventQueue(bool shutdown)
{
	tNetEvent *evt;
	while ((evt=getEvent())) {
		tNetSession *u=getSession(evt->sid);
		if (u != NULL) {
			switch(evt->id) {
				case UNET_NEWCONN:
					sec->log("%s New Connection\n",u->str().c_str());
					if (shutdown)
						terminate(u);
					else
						onNewConnection(u);
					break;
				case UNET_TIMEOUT:
					if (u->isTerminated()) { // a destroyed session, close it
						removeConnection(u);
					}
					else {
						if (shutdown || onConnectionTimeout(u))
							terminate(u, RTimedOut);
					}
					break;
				case UNET_FLOOD:
					sec->log("%s Flood Attack\n",u->str().c_str());
					if (shutdown || onConnectionFlood(u)) {
						terminate(u);
						alcGetMain()->err()->log("%s kicked due to a Flood Attack\n", u->str().c_str());
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
						ret = parseBasicMsg(msg, u, shutdown);
						if (ret != 1 && !u->isTerminated() && !shutdown) // if this is an active connection, look for other messages
							ret = onMsgRecieved(msg, u);
						if (ret == 1 && !msg->data.eof() > 0) { // packet was processed and there are bytes left, obiously invalid, terminate the client
							err->log("%s Recieved a message 0x%04X (%s) which was too long (%d Bytes remaining after parsing) - kicking player\n", u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd), msg->data.remaining());
							ret = -1;
						}
					}
					catch (txBase &t) { // if there was an error parsing the message, kick the responsible player
						err->log("%s Recieved invalid 0x%04X (%s) - kicking peer\n", u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
						err->log(" Exception %s\n%s\n",t.what(),t.backtrace());
						ret=-1;
					}
					if(ret==0) {
						if (u->isTerminated() || shutdown) {
							err->log("%s is terminated and sent non-NetMsgLeave message 0x%04X (%s)\n", u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
							terminate(u);
						}
						else {
							err->log("%s Unexpected message 0x%04X (%s) - kicking peer\n",u->str().c_str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
							sec->log("%s Unexpected message 0x%04X (%s)\n",u->str().c_str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
							terminate(u, RUnimplemented);
						}
					}
					else if(ret==-1) {
						// the problem already got printed to the error log wherever this return value was set
						sec->log("%s Kicked off due to a parse error in a previus message 0x%04X (%s)\n", u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
						terminate(u, RParseError);
					}
					else if(ret==-2) {
						// the problem already got printed to the error log wherever this return value was set
						sec->log("%s Kicked off due to cracking 0x%04X (%s)\n",u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
						terminate(u, RHackAttempt);
					}
					else if (ret!=1 && ret!=2) {
						err->log("%s Unknown error in 0x%04X (%s) - kicking peer\n",u->str().c_str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
						sec->log("%s Unknown error in 0x%04X (%s)\n",u->str().c_str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
						terminate(u);
					}
					break;
				}
				default:
					throw txBase(_WHERE("%s Unknown Event id %i\n",u->str().c_str(),evt->id));
			}
		}
		delete evt;
	}
	// I don't know how much perforcmance this costs, but without flushing it's not possible to follow the server logs using tail -F
	log->flush();
	err->flush();
	sec->flush();
}

// main event processing loop - blocks
void tUnetBase::run() {
	applyConfig();
	startOp();
	onStart();

	while(running) {
		Recv();
		processEventQueue(/*shutdown*/false);
		onIdle(idle);
	}

	// Uru clients need to be kicked first - messages might be sent to other servers as a reaction
	terminatePlayers();
	processEventQueue(/*shutdown*/true);
	
	//terminating the service
	terminateAll();
	
	U32 startup=getTime();
	while(!smgr->empty() && (getTime()-startup)<stop_timeout) {
		updateTimerRelative(100000); // make sure we don't wait longer than this (0.1 seconds)
		Recv();
		processEventQueue(/*shutdown*/true);
	}
	
	if(!smgr->empty()) {
		err->log("ERR: Session manager is not empty!\n");
		smgr->rewind();
		tNetSession * u;
		while((u=smgr->getNext())) {
			removeConnection(u);
		}
	}
	
	onStop();
	stopOp();
	log->log("INF: Service sanely terminated\n");
}

int tUnetBase::parseBasicMsg(tUnetMsg * msg, tNetSession * u, bool shutdown)
{
	switch(msg->cmd) {
		/* I am not sure what the correct "end of connection" procedure is, but here are some observations:
		- When the client leaves, it sends a NetMsgLeaves and expects an ack
		- When I send the client a NetMsgTerminated, it sends an ack back and goes away 
		- When I send the client a NetMsgLeave, it acks and ignores the message
		So I conclude that the two are equal, but that the server must send a terminate, and the client a leave. Alcugs will treat both of them equally. */
		case NetMsgLeave:
		{
			// accept it even if it is NOT a client - in that case, the peer obviously thinks it is a client, so lets respect its wish, it doesn't harm
			tmLeave msgleave(u);
			msg->data.get(msgleave);
			log->log("<RCV> [%d] %s\n",msg->sn,msgleave.str().c_str());
			/* Ack the current message adn terminate the connection */
			terminate(u, msgleave.reason, /*gotEndMsg*/true); // this will delete the session ASAP
			return 1;
		}
		case NetMsgTerminated:
		{
			// accept it even if it IS a client - in that case, the peer obviously thinks it is a server, so lets respect its wish, it doesn't harm
			tmTerminated msgterminated(u);
			msg->data.get(msgterminated);
			log->log("<RCV> [%d] %s\n",msg->sn,msgterminated.str().c_str());
			/* Ack the current message adn terminate the connection */
			terminate(u, msgterminated.reason, /*gotEndMsg*/true);
			return 1;
		}
		case NetMsgAlive:
		{
			if (u->isTerminated() || shutdown) return 0; // don't accept a NetMsgAlive on already terminated sessions
			tmAlive alive(u);
			msg->data.get(alive);
			log->log("<RCV> [%d] %s\n",msg->sn,alive.str().c_str());
			return 1;
		}
	}
	return 0;
}

} //end namespace
