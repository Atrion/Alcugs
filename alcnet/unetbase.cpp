/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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
//#define _DBG_LEVEL_ 5
#include <alcdefs.h>
#include "unetbase.h"

#include "unetmain.h"
#include "netexception.h"
#include "netmsgq.h"
#include "protocol/umsgbasic.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cassert>
#include <netdb.h>
#include <pthread.h>


namespace alc {

tUnetBase::tUnetBase(uint8_t whoami) :tUnet(whoami), configured(false), running(true), workerThread(this) {
	alcUnetGetMain()->setNet(this);
	tString var;
	tConfig * cfg;
	cfg=alcGetMain()->config();
	var=cfg->getVar("port","global");
	if(!var.isEmpty()) {
		setBindPort(var.asUInt());
	}
	var=cfg->getVar("bind","global");
	if(!var.isEmpty()) {
		setBindAddress(var);
	}
	
	
}

tUnetBase::~tUnetBase() {
	forcestop(); // could happen if an exception is flying... but we wont be of much help then
	alcUnetGetMain()->setNet(NULL);
}

void tUnetBase::applyConfig() {
	configured = true;
	openLogfiles(); // (re-)open basic logfiles
	// re-load configuration
	tString var;
	tConfig * cfg;
	cfg=alcGetMain()->config();
	var=cfg->getVar("net.stop.timeout","global");
	if(var.isEmpty()) {
		stop_timeout=15*1000*1000;
	} else {
		stop_timeout=var.asUInt()*1000*1000;
	}
	//Sets the idle timer
	var=cfg->getVar("net.timer","global");
	if(!var.isEmpty()) {
		max_sleep = var.asUInt()*1000*1000;
	}
	var=cfg->getVar("net.maxconnections","global");
	if(!var.isEmpty()) {
		max=var.get32();
	}
	var=cfg->getVar("net.timeout","global");
	if(!var.isEmpty()) {
		conn_timeout=var.asUInt();
	}
	var=cfg->getVar("net.up","global");
	if(!var.isEmpty()) {
		nat_up=var.asUInt();
	}
	var=cfg->getVar("net.down","global");
	if(!var.isEmpty()) {
		nat_down=var.asUInt();
	}
	var=cfg->getVar("net.lan.up","global");
	if(!var.isEmpty()) {
		lan_up=var.asUInt();
	}
	var=cfg->getVar("net.lan.down","global");
	if(!var.isEmpty()) {
		lan_down=var.asUInt();
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
		if(var.asUInt()) {
			setFlags(UNET_FLOODCTR);
		} else {
			unsetFlags(UNET_FLOODCTR);
		}
	}
	var=cfg->getVar("net.log.ack","global");
	if(!var.isEmpty()) {
		if(var.asUInt()) {
			setFlags(UNET_EACKLOG);
		} else {
			unsetFlags(UNET_EACKLOG);
		}
	}
	//Other DEVEL vars (dangerous to touch)
	var=cfg->getVar("net.flood_check_sec","global");
	if(!var.isEmpty()) {
		flood_check_interval=var.asUInt()*1000*1000;
	}
	var=cfg->getVar("net.max_flood_pkts","global");
	if(!var.isEmpty()) {
		max_flood_pkts=var.asUInt();
	}
	var=cfg->getVar("net.receive_ahead","global");
	if(!var.isEmpty()) {
		receiveAhead=var.asUInt();
	}
	#ifdef ENABLE_NETDEBUG
	var=cfg->getVar("net.lim_down_cap","global");
	if(!var.isEmpty()) {
		lim_down_cap=var.asUInt();
	}
	var=cfg->getVar("net.lim_up_cap","global");
	if(!var.isEmpty()) {
		lim_up_cap=var.asUInt();
	}
	var=cfg->getVar("net.in_noise","global");
	if(!var.isEmpty()) {
		in_noise=var.asUInt();
	}
	var=cfg->getVar("net.out_noise","global");
	if(!var.isEmpty()) {
		out_noise=var.asUInt();
	}
	var=cfg->getVar("net.latency","global");
	if(!var.isEmpty()) {
		latency=var.asUInt();
	}
	var=cfg->getVar("net.quota_check_interval","global");
	if(!var.isEmpty()) {
		quota_check_interval=var.asUInt();
	}
	#endif
	// forward to sub-classes
	onApplyConfig();
}

void tUnetBase::stop(tNetTimeDiff timeout) {
	tMutexLock lock(runModeMutex);
	if(timeout != static_cast<tNetTimeDiff>(-1))
		stop_timeout=timeout*1000*1000;
	running=false;
}

bool tUnetBase::isRunning(void)
{
	tMutexLock lock(runModeMutex);
	return running;
}

void tUnetBase::terminate(tNetSession *u, uint8_t reason, bool gotEndMsg)
{
	DBG(5, "%s is being terminated\n", u->str().c_str());
	if (u->isTerminated()) {
		log->log("%s is already terminated, speeding up disconnect...\n", u->str().c_str());
		u->setTimeout(0); // make sure this session goes down ASAP
		return;
	}
	if (!reason) reason = u->isClient() ? RKickedOff : RQuitting;
	if (!gotEndMsg) { // don't send message again if we already sent it, or if we got the message from the other side
		if (u->isClient() || u->getPeerType() == KClient) { // a KClient will ignore us sending a leave, so send a terminated even if the roles changed
			tReadLock lock(u->pubDataMutex); // we might be in main thread
			tmTerminated terminated(u,reason);
			send(terminated);
		}
		else {
			tReadLock lock(u->pubDataMutex); // we might be in main thread
			tmLeave leave(u,reason);
			send(leave);
		}
		// Now that we sent that message, the other side must ack it, then tNetSession will be deleted
	}
	
	addEvent(new tNetEvent(u, UNET_CONNCLS, new tContainer<uint8_t>(reason))); // trigger the event in the worker
	
	// tell the session it's terminating
	u->terminating();
}

bool tUnetBase::terminateAll(bool playersOnly)
{
	bool anyTerminated = false;
	tReadLock lock(smgrMutex);
	for (tNetSessionMgr::tIterator it(smgr); it.next();) {
		if (!it->isTerminated() && (!playersOnly ||  it->getPeerType() == KClient)) { // avoid sending a NetMsgLeave or NetMsgTerminate to terminated peers
			terminate(*it);
			anyTerminated = true;
		}
	}
	return anyTerminated;
}

// main thread loop: handles the socket, fills the working queue, starts and stops threads
void tUnetBase::run() {
	if (!configured) applyConfig(); // make sure applyConfig() was called at least once
	startOp();
	onStart();
	
	workerThread.spawn();
	
	while(isRunning()) {
		if (sendAndWait())
			onIdle();
		flushLogs();
	}
	max_sleep = 100*1000; // from now on, do not wait longer than 0.1 seconds so that we do not miss the stop timeout, and to speed up session deletion
	
	// Uru clients need to be kicked first - messages might be sent to other servers as a reaction
	if (terminatePlayers()) {
		sendAndWait(); // do one round of sending waiting
		sendAndWait(); // and another round of sending, so that the messages to the other servers actually go out
		flushLogs();
	}
	
	// kick remaining peers
	terminateAll();
	
	tNetTime shutdownInitTime = getNetTime();
	while(!sessionListEmpty() && (getNetTime()-shutdownInitTime)<stop_timeout) {
		sendAndWait();
		flushLogs();
	}
	
	// stop the worker thread
	DBG(5, "Worker sanely terminating\n");
	clearEventQueue();
	workerThread.stop();
	
	// shutdown
	onStop();
	stopOp();
	log->log("INF: Service sanely terminated\n");
}

// worker thread dispatch function
void tUnetBase::processEvent(tNetEvent *evt)
{
	tNetSession *u=*evt->u;
	DBG(5, "Processing event %d for %s\n", evt->id, u->str().c_str());
	switch(evt->id) {
		case UNET_NEWCONN:
			if (!isRunning())
				terminate(u);
			else
				onNewConnection(u);
			return;
		case UNET_TIMEOUT:
			if (!isRunning() || u->isTerminated() || onConnectionTimeout(u))
				terminate(u, RTimedOut);
			return;
		case UNET_FLOOD:
			if (!isRunning() || onConnectionFlood(u)) {
				alcGetMain()->err()->log("%s kicked due to a Flood Attack\n", u->str().c_str());
				terminate(u);
			}
			return;
		case UNET_CONNCLS:
		{
			uint8_t reason = dynamic_cast<tContainer<uint8_t> *>(evt->data)->getData();
			onConnectionClosing(u, reason);
			return;
		}
		case UNET_MSGRCV:
		{
			tUnetMsg * msg = dynamic_cast<tUnetMsg *>(evt->data);
			assert(msg!=NULL);
			int ret = 0; // 0 - non parsed; 1 - parsed; 2 - ignored; -1 - parse error; -2 - hack attempt
			#ifdef ENABLE_MSGDEBUG
			log->log("%s New MSG Recieved\n",u->str().c_str());
			#endif
			try {
				ret = parseBasicMsg(msg, u, !isRunning());
				if (ret != 1 && !u->isTerminated()) // if this is an active connection, look for other messages
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
				if (u->isTerminated()) {
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
			return;
		}
		default:
			throw txBase(_WHERE("%s Unknown Event id %i\n",u->str().c_str(),evt->id));
	}
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
			/* Ack the current message and terminate the connection */
			terminate(u, msgleave.reason, /*gotEndMsg*/true); // this will delete the session ASAP
			return 1;
		}
		case NetMsgTerminated:
		{
			// accept it even if it IS a client - in that case, the peer obviously thinks it is a server, so lets respect its wish, it doesn't harm
			tmTerminated msgterminated(u);
			msg->data.get(msgterminated);
			log->log("<RCV> [%d] %s\n",msg->sn,msgterminated.str().c_str());
			/* Ack the current message and terminate the connection */
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

void tUnetBase::flushLogs()
{
	log->flush();
	err->flush();
	sec->flush();
}

void tUnetBase::tUnetWorkerThread::stop()
{
	if (!isSpawned()) return; // worker not even running
	DBG(5, "Stopping worker...\n");
	net->addEvent(new tNetEvent(UNET_KILL_WORKER));
	join(); // wait till thread really stopped
	DBG(5, "Worker stopped\n");
}

void tUnetBase::tUnetWorkerThread::main(void)
{
	DBG(5, "Worker spawned\n");
	tNetEvent *evt;
	while (true) {
		while ((evt=net->getEvent())) {
			switch (evt->id) {
				case UNET_KILL_WORKER:
					delete evt; // do not leak memory
					return;
				default:
					net->processEvent(evt);
			}
			delete evt;
		}
		// emptied event queue
		net->onWorkerIdle();
		// I don't know how much perforcmance this costs, but without flushing it's not possible to follow the server logs using tail -F
		net->flushLogs();
		// wait till e got events again
		tMutexLock lock(net->eventsMutex);
		if (!net->events.empty()) continue; // an event got added since we last checked, fine!
		// queue is empty, so wait for an event to be added
		net->workerWaiting = true;
		DBG(5, "Worker waiting\n");
		pthread_cond_wait(&net->eventAddedCond, net->eventsMutex.getMutex());
		DBG(5, "Worker finished waiting\n");
		net->workerWaiting = false;
	}
}

} //end namespace
