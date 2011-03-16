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
#define __U_NETSESSIONMGR_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "netsessionmgr.h"

#include "netexception.h"

#include <cassert>
#include <cstdlib>

namespace alc {

tNetSessionMgr::tNetSessionMgr(tUnet * net,size_t limit) : size(0), count(0), table(NULL), max(limit), net(net)
{}
tNetSessionMgr::~tNetSessionMgr()
{
	if(table!=NULL) {
		for(size_t i=0; i<size; i++) {
			if (table[i]) table[i]->decRefs(); // no longer referenced here
		}
	}
	free(table);
}
tNetSession *tNetSessionMgr::searchAndCreate(uint32_t ip, uint16_t port, bool client, uint8_t validation)
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
	table[sid] = new tNetSession(net,ip,port,sid,client,validation);
	++count;
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
	return size-1;
}

bool tNetSessionMgr::tIterator::next() {
	do {
		++pos;
	} while(pos < smgr->size && smgr->table[pos] == NULL);
	return pos < smgr->size;
}
tNetSession *tNetSessionMgr::findByKi(uint32_t ki) const
{
	if (ki == 0) return NULL;
	for (size_t i = 0; i < size; ++i) {
		if (table[i] && table[i]->ki == ki) return table[i];
	}
	return NULL;
}
void tNetSessionMgr::destroy(tNetSession *u) {
	size_t pos = u->getSid();
	assert(u && pos < size && table[pos] == u); // this implies "size > 0"
	table[pos] = NULL;
	u->decRefs();
	--count;
	// let's look if we can shrink... search for NULL entries at the end
	if (pos != size-1) return; // it's not the last slot that was freed, so we can not shrink
	while (pos > 0 && table[pos-1] == NULL) --pos; // set pos to the last session that's still NULL
	// now we can shrink the table to size pos
	assert(count <= pos);
	DBG(5, "shrinking to %Zd\n", pos);
	table=static_cast<tNetSession **>(realloc(table,sizeof(tNetSession*) * pos));
	if (pos > 0 && table==NULL) throw txNoMem(_WHERE("NoMem"));
	size=pos;
	if (count == 0) { assert(size == 0); }
}

}


