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

#ifndef __U_NETSESSIONMGR_H
#define __U_NETSESSIONMGR_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSIONMGR_H_ID "$Id$"

#include "netsession.h"

namespace alc {
	
	class tUnet;
	class tUnetMsg;

class tNetSessionRef {
public:
	tNetSession *u;
	tNetSessionRef(tNetSession *u) : u(u) {
		if (u) u->incRefs();
	}
	tNetSessionRef() : u(NULL) {}
	tNetSessionRef(const tNetSessionRef &r) : u(r.u) {
		if (u) u->incRefs();
	}
	~tNetSessionRef() {
		if (u) u->decRefs();
	}
	tNetSession *operator->() {
		return u;
	}
	tNetSession *operator*() {
		return u;
	}
	void operator=(tNetSession *new_u) {
		if (u) u->decRefs();
		u = new_u;
		if (u) u->incRefs();
	}
	void operator=(const tNetSessionRef &r) {
		if (u) u->decRefs();
		u = r.u;
		if (u) u->incRefs();
	}
};

/** tNetSessionMgr is meant to be THE one and only session manager of a server. The number of a session in it's table defines
it's global sid. It uses tNetSessionIte for searching and for "caching" the sid. It can also create new and delete existing sessions
(in fact, it's the only part of alcugs which does that) and it implements a limit for a max. number of sessions */
class tNetSessionMgr {
public:
	tNetSessionMgr(tUnet * net,size_t limit=0);
	~tNetSessionMgr();
	
	//! find the session with the given ip and port and return it, creating it if necessary
	tNetSession *searchAndCreate(uint32_t ip, uint16_t port, bool client = true, uint8_t validation = 0);
	//! return the nth session of our table
	tNetSession *get(size_t n) const {
		if (n < size) return table[n];
		return NULL;
	}
	//! find a session by it's ki
	tNetSession *findByKi(uint32_t ki) const;
	//! remove a session
	void destroy(tNetSession *u);
	// small helpers
	bool isEmpty() const { return count==0; }
	size_t getCount() const { return count; }
	
	// iteration: Manager must not be modified during iteration, or behaviour is undefined!
	class tIterator {
	public:
		tIterator(const tNetSessionMgr *smgr) : smgr(smgr), pos(npos) {}
		bool next();
		tNetSession *operator->() { return smgr->table[pos]; }
		tNetSession *operator*() { return smgr->table[pos]; }
	private:
		const tNetSessionMgr *smgr;
		size_t pos;
	};
private:
	//! find a free slot, resizing the table if necessary
	size_t findFreeSlot(void);

	size_t size; //!< the size of the table
	size_t count; //!< the number of session stored in the table (there can be holes, so it can be smaller than size)
	tNetSession ** table;
	
	size_t max;
	tUnet * net;
	
	FORBID_CLASS_COPY(tNetSessionMgr)
};

class tNetEvent {
public:
	tNetEvent(tNetSession *u,int what,tBaseType *data=NULL) : u(u), id(what), data(data) {}
	tNetEvent(int what) { id=what; data=NULL; }
	tNetEvent() { id=0; data=NULL; }
	~tNetEvent() { delete data; }
	tNetSessionRef u;
	int id;
	tBaseType *data;

	FORBID_CLASS_COPY(tNetEvent)
};

}

#endif
