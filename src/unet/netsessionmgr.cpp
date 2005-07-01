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
	URUNET 3
*/

/* CVS tag - DON'T TOUCH*/
#define __U_NETSESSIONMGR_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "urunet/unet.h"

#include "alcdebug.h"

namespace alc {

/* Sesion Mgr */
tNetSessionMgr::tNetSessionMgr(tUnet * net,int limit) {
	DBG(5,"tNetSessionMgr()\n");
	this->max=limit;
	this->net=net;
	off=0;
	n=0;
	table=NULL;
}
tNetSessionMgr::~tNetSessionMgr() {
	DBG(5,"~tNetSessionMgr()\n");
	int i;
	if(table!=NULL) {
		for(i=0; i<n; i++) {
			if(table[i]!=NULL) delete table[i];
		}
		free((void *)table);
	}
}
tNetSession * tNetSessionMgr::search(tNetSessionIte &ite,bool create) {
	if(ite.sid!=-1 && ite.sid<n && table[ite.sid]!=NULL) {
		if(table[ite.sid]->ip==ite.ip && table[ite.sid]->port==ite.port) {
			return table[ite.sid];
		}
	}
	if(!create) return NULL;
	//then create/search
	int i;
	int lsid=-1;
	for(i=0; i<n; i++) {
		if(table[i]==NULL) {
			if(lsid==-1) lsid=i;
		} else {
			if(table[i]->ip==ite.ip && table[i]->port==ite.port) {
				ite.sid=i;
				return table[i];
			}
		}
	}
	//not found
	if(lsid!=-1) {
		i=lsid;
		ite.sid=i;
		table[i] = new tNetSession(net,ite.ip,ite.port,ite.sid);
		/*table[i]->ip=ite.ip;
		table[i]->port=ite.port;
		table[i]->sid=ite.sid;*/
		return table[i];
	}
	//does not exist
	if(max!=0 && n>=max) throw txToMCons(_WHERE("To many connections %i:%i",n,max));
	tNetSession ** ntable;
	ntable=(tNetSession **)realloc((void *)table,sizeof(tNetSession) * (n+1));
	if(ntable==NULL) throw txNoMem(_WHERE(""));
	table=ntable;
	ite.sid=n;
	i=n;
	n++;
	table[i] = new tNetSession(net,ite.ip,ite.port,ite.sid);
	/*table[i]->ip=ite.ip;
	table[i]->port=ite.port;
	table[i]->sid=ite.sid;*/
	return table[i];
}
void tNetSessionMgr::destroy(tNetSessionIte &ite) {
	int found=-1;
	int i;
	if(ite.sid!=-1 && ite.sid<n && table[ite.sid]!=NULL) {
		if(table[ite.sid]->ip==ite.ip && table[ite.sid]->port==ite.port) {
			found=ite.sid;
		}
	}
	if(found==-1) {
		//then search
		for(i=0; i<n; i++) {
			if(table[i]!=NULL && table[i]->ip==ite.ip && table[i]->port==ite.port) {
				ite.sid=i;
				found=i;
				break;
			}
		}
	}
	if(found!=-1) {
		delete table[found];
		table[found]=NULL;
		for(i=found; i<n; i++) {
			if(table[i]!=NULL) { found=-1; break; }
		}
		if(found!=-1) { //then resize
			table=(tNetSession **)realloc(table,sizeof(tNetSession) * found);
			n=found;
		}
	}
}
void tNetSessionMgr::rewind() { off=0; }
void tNetSessionMgr::end() { off=max; }
tNetSession * tNetSessionMgr::getNext() {
	tNetSession * k=NULL;
	if(off>=n) { off=0; return NULL; }

	while((k=table[off])==NULL) {
		off++;
		if(off>=n) { off=0; return NULL; }
	}
	k=table[off]; off++;
	
	return k;
}
/* End session mgr */

}


