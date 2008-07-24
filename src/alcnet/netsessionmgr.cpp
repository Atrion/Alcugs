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
	URUNET 3
*/

/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSIONMGR_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "alcnet.h"

#include "alcdebug.h"

namespace alc {

/* Session List */
tNetSessionList::tNetSessionList(void)
{
	off=0;
	size=count=0;
	table=NULL;
}
tNetSessionList::~tNetSessionList()
{
	if(table!=NULL)
		free((void *)table);
}
tNetSession *tNetSessionList::search(U32 ip, U16 port)
{
	for(int i=0; i<size; i++) {
		if(table[i]!=NULL && table[i]->getIp()==ip && table[i]->getPort()==port)
			return table[i];
	}
	return NULL; // not found
}
int tNetSessionList::add(tNetSession *u)
{
	int empty = findFreeSlot();
	if (empty >= 0) {
		table[empty] = u;
		++count;
		return empty;
	}
	// we have to resize the table
	DBG(5, "growing to %d\n", size+1);
	tNetSession **ntable=(tNetSession **)realloc((void *)table,sizeof(tNetSession*) * (size+1));
	if(ntable==NULL) throw txNoMem(_WHERE(""));
	table=ntable;
	++size;
	++count;
	table[size-1] = u;
	return size-1;
}
int tNetSessionList::findFreeSlot(void)
{
	for (int i = 0; i < size; ++i) {
		if (!table[i]) return i;
	}
	assert(size == count); // when we get here (i.e. there's no free slot), size and count must be the same
	return -1;
}
void tNetSessionList::remove(tNetSession *u)
{
	int found = -1;
	for (int i = 0; i < size; ++i) {
		if (table[i] && table[i] == u) {
			table[i] = NULL;
			--count;
			found = i;
			break;
		}
	}
	if (found < 0) return; // session is not part of the list
	// let's look if we can shrink... search for NULL entries before this one
	while (found > 0 && table[found-1] == NULL) --found;
	// look if all session after the found one have been destroyed
	for(int i=found; i<size; i++) {
		if(table[i]!=NULL) return; // we can't shrink :(
	}
	if(found!=-1) { // if that's the case, shrink
		DBG(5, "shrinking to %d\n", found);
		table=(tNetSession **)realloc(table,sizeof(tNetSession*) * found); // it's not a bug if we get NULL here - the size might be 0
		size=found;
	}
	if (count == 0) { assert(size == 0); }
}

void tNetSessionList::rewind() { off=0; }
void tNetSessionList::end() { off=size; }
tNetSession * tNetSessionList::getNext() {
	tNetSession * k=NULL;
	if(off>=size) { off=0; return NULL; }

	while((k=table[off])==NULL) {
		off++;
		if(off>=size) { off=0; return NULL; }
	}
	k=table[off]; off++;
	
	return k;
}
tNetSession *tNetSessionList::find(U32 ki)
{
	if (ki == 0) return NULL;
	for (int i = 0; i < size; ++i) {
		if (table[i] && table[i]->ki == ki) return table[i];
	}
	return NULL;
}

/* Sesion Mgr */
tNetSessionMgr::tNetSessionMgr(tUnet * net,int limit) : tNetSessionList()
{
	this->max=limit;
	this->net=net;
}
tNetSessionMgr::~tNetSessionMgr()
{
	if(table!=NULL) {
		for(int i=0; i<size; i++) {
			if(table[i]!=NULL) delete table[i];
		}
	}
	// the table itself will be freed in ~tNetSessionList
}
tNetSession * tNetSessionMgr::search(tNetSessionIte &ite,bool create) {
	if(ite.sid!=-1 && ite.sid<size && table[ite.sid]!=NULL) {
		if(table[ite.sid]->getIp()==ite.ip && table[ite.sid]->getPort()==ite.port) {
			return table[ite.sid];
		}
	}
	//then search
	tNetSession *u = tNetSessionList::search(ite.ip, ite.port);
	if (u) {
		ite.sid=u->getSid();
		return u;
	}
	if(!create) return NULL;

	// not found, create and add
	if (max != 0 && count >= max) // too many connections, ouch
		throw txToMCons(_WHERE("Too many connections (already having the maximum of %i)",max));
	// use add with a NULL pointer. That'll reserve the place for us and we can still create the new session witht the correct sid
	int sid = add(NULL);
	ite.sid = sid;
	table[sid] = new tNetSession(net,ite.ip,ite.port,ite.sid);
	return table[sid];
}

void tNetSessionMgr::destroy(tNetSessionIte &ite) {
	tNetSession *u = search(ite, false);
	if (!u) return;
	delete u;
	remove(u);
}
/* End session mgr */

}


