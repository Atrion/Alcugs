/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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

#ifndef __U_NETSESSIONMGR_H
#define __U_NETSESSIONMGR_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSIONMGR_H_ID "$Id$"

#include "netsession.h"

namespace alc {
	
	class tUnet;
	class tUnetMsg;
	
	const uint32_t nosid = -1;

class tNetSessionIte {
public:
	uint32_t ip; //network order
	uint16_t port; //network order
	uint32_t sid;
	tNetSessionIte() {
		ip=0; port=0; sid=nosid;
	}
	tNetSessionIte(uint32_t ip,uint16_t port,uint32_t sid=nosid) {
		this->ip=ip;
		this->port=port;
		this->sid=sid;
	}
	bool operator==(const tNetSessionIte &t) const {
		return(ip==t.ip && port==t.port);
	}
	bool operator==(tNetSession *u) const {
		return(ip==u->getIp() && port==u->getPort());
	}
};

/** tNetSessionList saves a list of sessions */
class tNetSessionList {
public:
	tNetSessionList(void);
	virtual ~tNetSessionList();
	
	//! find the session with the given ip and port and return it
	tNetSession *search(uint32_t ip, uint16_t port);
	//! add that session and return the place where it's saved (to be used by tNetSessionMgr::search)
	int add(tNetSession *u);
	//! removes the given session from the table and shrinks if possible
	void remove(tNetSession *u);
	//! return the nth session of our table
	tNetSession *get(size_t n) {
		if (n < size) return table[n];
		return NULL;
	}
	//! find a session by it's ki
	tNetSession *findByKi(uint32_t ki);
	void rewind() { off=0; }
	void end() { off=size; }
	tNetSession * getNext();
	bool isEmpty() { return count==0; }
	size_t getCount() { return count; }
protected:
	size_t findFreeSlot(void);

	size_t off; //!< current offset/position
	size_t size; //!< the size of the table
	size_t count; //!< the number of session stored in the table (there can be holes, so it can be smaller than size)
	tNetSession ** table;
};

/** tNetSessionMgr is meant to be the one and only session manager of a server. The number of a session in it's table defines
it's global sid. It uses tNetSessionIte for searching and for "caching" the sid. It can also create new and delete existing sessions
(in fact, it's the only part of alcugs which does that) and it implements a limit for a max. number of sessions */
class tNetSessionMgr : public tNetSessionList {
public:
	tNetSessionMgr(tUnet * net,size_t limit=0) : tNetSessionList(), max(limit), net(net) {}
	virtual ~tNetSessionMgr();
	tNetSession * search(tNetSessionIte &ite,bool create = false);
	void destroy(tNetSessionIte ite);
private:
	size_t max;
	tUnet * net;
	
	FORBID_CLASS_COPY(tNetSessionMgr)
};

class tNetEvent {
public:
	tNetEvent(tNetSessionIte who,int what,tUnetMsg *mymsg=NULL) { sid=who; id=what; msg=mymsg; }
	tNetEvent(int what) { id=what; msg=NULL; }
	tNetEvent() { id=0; msg=NULL; }
	~tNetEvent() { if (msg) delete msg; }
	tNetSessionIte sid;
	int id;
	tUnetMsg *msg;

	FORBID_CLASS_COPY(tNetEvent)
};

}

#endif
