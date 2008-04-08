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
	bool operator==(tNetSessionIte &t) {
		return(ip==t.ip && port==t.port);
	}
};

/** tNetSessionList saves a list of sessions. it can search for a session using a tNetSessionIte. also see tNetSessionMgr. */
class tNetSessionList {
public:
	tNetSessionList(void);
	virtual ~tNetSessionList();
	
	//! find the session with the given ip and port (ignore the sid) and return it
	tNetSession *findSession(tNetSessionIte &ite);
	//! add that session and return the place where it's saved (to be used by tNetSessionMgr::search)
	int addSession(tNetSession *u);
	//! removes the given session from the table and shrinks if possible
	void removeSession(tNetSession *u);
	//! return the nth session of our table
	tNetSession *getSession(int nr) {
		if (nr < n) return table[nr];
		return NULL;
	}
	void rewind();
	void end();
	tNetSession * getNext();
	bool empty() {
		return n==0;
	}
protected:
	int findFreeSlot(void);

	int off;
	int n;
	tNetSession ** table;
};

/** tNetSessionMgr is meant to be the one and only session manager of a server. The number of a session in it's table defines
it's global sid. It will use the sid of a tNetSessionIte for faster searching. It can also create new and delete existing sessions
(in fact, it's the only part of alcugs which does that) and it implements a limit for a max. number of sessions */
class tNetSessionMgr : public tNetSessionList {
public:
	tNetSessionMgr(tUnet * net,int limit=0);
	virtual ~tNetSessionMgr();
	tNetSession * search(tNetSessionIte &ite,bool create);
	void destroy(tNetSessionIte &ite);
private:
	int max;
	tUnet * net;
};

class tNetEvent {
public:
	tNetEvent(tNetSessionIte who,int what) { sid=who; id=what; next=NULL; veto=false; }
	tNetEvent(tNetEvent &t) { sid=t.sid; id=t.id; next=NULL; veto=false; }
	tNetEvent() { next=NULL; id=0; veto=false; }
	void Veto() { veto=true; }
	tNetSessionIte sid;
	int id;
	tNetEvent * next;
	bool veto;
	//U16 cmd;
};

}

#endif
