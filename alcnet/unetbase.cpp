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

//#define _DBG_LEVEL_ 5
#include <alcdefs.h>
#include "unetbase.h"

#include "unetmain.h"
#include "netexception.h"
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
		setBindPort(var.asInt());
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
	// re-load configuration
	tString var;
	tConfig * cfg;
	cfg=alcGetMain()->config();
	var=cfg->getVar("net.stop.timeout","global");
	if(var.isEmpty()) {
		stop_timeout=10;
	} else {
		stop_timeout=var.asInt();
	}
	//Sets the idle timer
	var=cfg->getVar("net.timer","global");
	if(!var.isEmpty()) {
		max_sleep = var.asInt();
	}
	var=cfg->getVar("net.maxconnections","global");
	if(!var.isEmpty()) {
		max=var.get32();
	}
	var=cfg->getVar("net.timeout","global");
	if(!var.isEmpty()) {
		conn_timeout=var.asInt();
	}
	var=cfg->getVar("net.up","global");
	if(!var.isEmpty()) {
		nat_up=var.asInt();
	}
	var=cfg->getVar("net.down","global");
	if(!var.isEmpty()) {
		nat_down=var.asInt();
	}
	var=cfg->getVar("net.lan.up","global");
	if(!var.isEmpty()) {
		lan_up=var.asInt();
	}
	var=cfg->getVar("net.lan.down","global");
	if(!var.isEmpty()) {
		lan_down=var.asInt();
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
		if(var.asInt()) {
			setFlags(UNET_FLOODCTR);
		} else {
			unsetFlags(UNET_FLOODCTR);
		}
	}
	var=cfg->getVar("net.log.ack","global");
	if(!var.isEmpty()) {
		if(var.asInt()) {
			setFlags(UNET_EACKLOG);
		} else {
			unsetFlags(UNET_EACKLOG);
		}
	}
	//Other DEVEL vars (dangerous to touch)
	var=cfg->getVar("net.flood_check_sec","global");
	if(!var.isEmpty()) {
		flood_check_interval=var.asInt();
	}
	var=cfg->getVar("net.max_flood_pkts","global");
	if(!var.isEmpty()) {
		max_flood_pkts=var.asInt();
	}
	var=cfg->getVar("net.receive_ahead","global");
	if(!var.isEmpty()) {
		receiveAhead=var.asInt();
	}
	#ifdef ENABLE_NETDEBUG
	var=cfg->getVar("net.lim_down_cap","global");
	if(!var.isEmpty()) {
		lim_down_cap=var.asInt();
	}
	var=cfg->getVar("net.lim_up_cap","global");
	if(!var.isEmpty()) {
		lim_up_cap=var.asInt();
	}
	var=cfg->getVar("net.in_noise","global");
	if(!var.isEmpty()) {
		in_noise=var.asInt();
	}
	var=cfg->getVar("net.out_noise","global");
	if(!var.isEmpty()) {
		out_noise=var.asInt();
	}
	var=cfg->getVar("net.latency","global");
	if(!var.isEmpty()) {
		latency=var.asInt()/1000.0/1000.0;
	}
	var=cfg->getVar("net.quota_check_interval","global");
	if(!var.isEmpty()) {
		quota_check_interval=var.asInt();
	}
	#endif
	// (re-)open basic logfiles
	openLogfiles();
	// forward to sub-classes
	onApplyConfig();
}

void tUnetBase::stop()
{
	{
		tSpinLock lock(runModeMutex);
		running=false;
	}
	
	if (alcGetSelfThreadId() != alcGetMain()->threadId()) wakeUpMainThread();
}

void tUnetBase::forcestop()
{
	{
		tSpinLock lock(runModeMutex);
		stop_timeout = 0; // don't wait
		running=false;
	}
	
	if (alcGetSelfThreadId() != alcGetMain()->threadId()) wakeUpMainThread();
}

bool tUnetBase::isRunning(void)
{
	tSpinLock lock(runModeMutex);
	return running;
}

bool tUnetBase::terminateAll(bool playersOnly)
{
	bool anyTerminated = false;
	tReadLock lock(smgrMutex);
	for (tNetSessionMgr::tIterator it(smgr); it.next();) {
		if (!it->isTerminated() && (!playersOnly ||  it->isUruClient())) { // avoid sending a NetMsgLeave or NetMsgTerminate to terminated peers
			it->terminate();
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
	}
	max_sleep = 0.1; // from now on, do not wait longer than 0.1 seconds so that we do not miss the stop timeout, and to speed up session deletion
	
	// Uru clients need to be kicked first - messages might be sent to other servers as a reaction
	if (terminatePlayers()) {
		sendAndWait(); // do one round of sending waiting
		sendAndWait(); // and another round of sending, so that the messages to the other servers actually go out
	}
	
	// kick remaining peers
	terminateAll();
	
	tNetTime shutdownInitTime = getNetTime();
	while(!sessionListEmpty() && (getNetTime()-shutdownInitTime)<getStopTimeout()) {
		sendAndWait();
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
				u->terminate();
			else
				onNewConnection(u);
			return;
		case UNET_TIMEOUT:
			if (!isRunning() || onConnectionTimeout(u))
				u->terminate(RTimedOut);
			return;
		case UNET_FLOOD:
			if (!isRunning() || onConnectionFlood(u)) {
				alcGetMain()->err()->log("%s kicked due to a Flood Attack\n", u->str().c_str());
				u->terminate();
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
			if (u->isTerminated()) {
				err->log("%s is terminated and sent non-NetMsgLeave message 0x%04X (%s) - ignoring\n", u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
				return;
			}
			try {
				ret = onMsgRecieved(msg, u);
				if (ret == 1 && !msg->data.eof() > 0) { // packet was processed and there are bytes left, obiously invalid, terminate the client
					err->log("%s Recieved a message 0x%04X (%s) which was too long (%zd Bytes remaining after parsing) - kicking player\n", u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd), msg->data.remaining());
					ret = -1;
				}
			}
			catch (txBase &t) { // if there was an error parsing the message, kick the responsible player
				err->log("%s Recieved invalid 0x%04X (%s) - kicking peer\n", u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
				err->log(" Exception %s\n%s\n",t.what(),t.backtrace());
				ret=-1;
			}
			if(ret==0) {
				err->log("%s Unexpected message 0x%04X (%s) - kicking peer\n",u->str().c_str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
				sec->log("%s Unexpected message 0x%04X (%s)\n",u->str().c_str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
				u->terminate(RUnimplemented);
			}
			else if(ret==-1) {
				// the problem already got printed to the error log wherever this return value was set
				sec->log("%s Kicked off due to a parse error in a previus message 0x%04X (%s)\n", u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
				u->terminate(RParseError);
			}
			else if(ret==-2) {
				// the problem already got printed to the error log wherever this return value was set
				sec->log("%s Kicked off due to cracking 0x%04X (%s)\n",u->str().c_str(), msg->cmd, alcUnetGetMsgCode(msg->cmd));
				u->terminate(RHackAttempt);
			}
			else if (ret!=1 && ret!=2) {
				err->log("%s Unknown error in 0x%04X (%s) - kicking peer\n",u->str().c_str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
				sec->log("%s Unknown error in 0x%04X (%s)\n",u->str().c_str(),msg->cmd,alcUnetGetMsgCode(msg->cmd));
				u->terminate();
			}
			return;
		}
		default:
			throw txBase(_WHERE("%s Unknown Event id %i\n",u->str().c_str(),evt->id));
	}
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
	while (true) {
		tNetEvent *evt = net->getEvent();
		switch (evt->id) {
			case UNET_KILL_WORKER:
				delete evt; // do not leak memory
				return;
			default:
				net->processEvent(evt);
		}
		delete evt;
	}
}

} //end namespace
