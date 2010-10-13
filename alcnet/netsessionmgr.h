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

/**
	URUNET 3+
*/

#ifndef __U_NETSESSIONMGR_H
#define __U_NETSESSIONMGR_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSIONMGR_H_ID "$Id$"

namespace alc {

class tNetSessionIte {
public:
	U32 ip; //network order
	U16 port; //network order
	int sid;
	tNetSessionIte() {
		ip=0; port=0; sid=-1;
	}
	tNetSessionIte(U32 ip,U16 port,int sid=-1) {
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
	tNetSession *search(U32 ip, U16 port);
	//! add that session and return the place where it's saved (to be used by tNetSessionMgr::search)
	int add(tNetSession *u);
	//! removes the given session from the table and shrinks if possible
	void remove(tNetSession *u);
	//! return the nth session of our table
	tNetSession *get(int n) {
		if (n < size) return table[n];
		return NULL;
	}
	//! find a session by it's ki
	tNetSession *find(U32 ki);
	void rewind();
	void end();
	tNetSession * getNext();
	bool empty() { return count==0; }
	inline int getCount() { return count; }
protected:
	int findFreeSlot(void);

	int off;
	int size; //!< the size of the table
	int count; //!< the number of session stored in the table (there can be holes, so it can be smaller than size)
	tNetSession ** table;
};

/** tNetSessionMgr is meant to be the one and only session manager of a server. The number of a session in it's table defines
it's global sid. It uses tNetSessionIte for searching and for "caching" the sid. It can also create new and delete existing sessions
(in fact, it's the only part of alcugs which does that) and it implements a limit for a max. number of sessions */
class tNetSessionMgr : public tNetSessionList {
public:
	tNetSessionMgr(tUnet * net,int limit=0);
	virtual ~tNetSessionMgr();
	tNetSession * search(tNetSessionIte &ite,bool create = false);
	void destroy(tNetSessionIte &ite);
private:
	int max;
	tUnet * net;
	
	FORBID_CLASS_COPY(tNetSessionMgr)
};

class tNetEvent {
public:
	tNetEvent(tNetSessionIte who,int what,tUnetMsg *mymsg=NULL) { sid=who; id=what; next=NULL; msg=mymsg; }
	tNetEvent() { next=NULL; id=0; msg=NULL; }
	~tNetEvent() { if (msg) delete msg; }
	tNetSessionIte sid;
	int id;
	tNetEvent * next;
	tUnetMsg *msg;
private:
	// make copying impossible
	tNetEvent(const tNetEvent &);
	const tNetEvent &operator=(const tNetEvent &);
};

}

#endif