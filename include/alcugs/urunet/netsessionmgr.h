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
	U32 ip;
	U16 port;
	int sid;
	tNetSessionIte() {
		ip=0; port=0; sid=-1;
	}
	tNetSessionIte(U32 ip,U16 port,int sid=-1) {
		this->ip=ip;
		this->port=port;
		this->sid=sid;
	}
};

class tNetSessionMgr {
public:
	tNetSessionMgr(tUnet * net,int limit=0);
	~tNetSessionMgr();
	tNetSession * search(tNetSessionIte &ite,bool create=true);
	void rewind();
	void end();
	void destroy(tNetSessionIte &ite);
	tNetSession * getNext();
private:
	int off;
	int n;
	int max;
	tNetSession ** table;
	tUnet * net;
};

}

#endif
