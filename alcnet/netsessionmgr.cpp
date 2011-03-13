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
#include <alcdefs.h>
#include "netsessionmgr.h"

#include "netexception.h"

#include <cassert>
#include <cstdlib>

namespace alc {

/* Session List */
tNetSessionMgr::tNetSessionMgr(tUnet * net,size_t limit) : off(0), size(0), count(0), table(NULL), max(limit), net(net)
{}
tNetSessionMgr::~tNetSessionMgr()
{
	if(table!=NULL) {
		for(size_t i=0; i<size; i++) {
			delete table[i];
		}
	}
	free(table);
}
tNetSession *tNetSessionMgr::searchAndCreate(uint32_t ip, uint16_t port)
{
	for(size_t i=0; i<size; i++) {
		if(table[i]!=NULL && table[i]->getIp()==ip && table[i]->getPort()==port)
			return table[i];
	}
	// not found, create and add
	if (max != 0 && count >= max) // too many connections, ouch
		throw txToMCons(_WHERE("Too many connections (already having the maximum of %i)",max));
	// find a place for us
	size_t sid = findFreeSlot();
	table[sid] = new tNetSession(net,ip,port,sid);
	return table[sid];
}
size_t tNetSessionMgr::findFreeSlot(void)
{
	for (size_t i = 0; i < size; ++i) {
		if (!table[i]) return i;
	}
	assert(size == count); // when we get here (i.e. there's no free slot), size and count must be the same
	// we have to resize the table
	DBG(5, "growing to %Zd\n", size+1);
	tNetSession **ntable=static_cast<tNetSession **>(realloc(table,sizeof(tNetSession*) * (size+1)));
	if(ntable==NULL) throw txNoMem(_WHERE(""));
	table=ntable;
	++size;
	++count;
	return size-1; // == table-1
}
void tNetSessionMgr::remove(tNetSession *u)
{
	size_t found = npos;
	for (size_t i = 0; i < size; ++i) {
		if (table[i] && table[i] == u) {
			table[i] = NULL;
			--count;
			found = i;
			break;
		}
	}
	if (found == npos) return; // session is not part of the list
	// let's look if we can shrink... search for NULL entries before this one
	while (found > 0 && table[found-1] == NULL) --found;
	// look if all session after the found one have been destroyed
	for(size_t i=found; i<size; i++) {
		if(table[i]!=NULL) return; // we can't shrink :(
	}
	if(found!=npos) { // if that's the case, shrink
		DBG(5, "shrinking to %Zd\n", found);
		table=static_cast<tNetSession **>(realloc(table,sizeof(tNetSession*) * found));
		if (found && table==NULL) throw txNoMem(_WHERE("NoMem"));
		size=found;
	}
	if (count == 0) { assert(size == 0); }
}

tNetSession * tNetSessionMgr::getNext() {
	tNetSession * k=NULL;
	if(off>=size) { off=0; return NULL; }

	while((k=table[off])==NULL) {
		off++;
		if(off>=size) { off=0; return NULL; }
	}
	k=table[off]; off++;
	
	return k;
}
tNetSession *tNetSessionMgr::findByKi(uint32_t ki)
{
	if (ki == 0) return NULL;
	for (size_t i = 0; i < size; ++i) {
		if (table[i] && table[i]->ki == ki) return table[i];
	}
	return NULL;
}



void tNetSessionMgr::destroy(tNetSession *u) {
	assert(u && u->getSid() < size && table[u->getSid()] == u);
	remove(u);
	u->decRefs();
}
/* End session mgr */

}


