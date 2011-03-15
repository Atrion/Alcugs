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

/* The Uru protocol, is here */

#ifndef __U_UMSGBASIC_H
#define __U_UMSGBASIC_H
/* CVS tag - DON'T TOUCH*/
#define __U_UMSGBASIC_H_ID "$Id$"

#include "protocol.h"

namespace alc {

/** TERMINATED message */
class tmTerminated :public tmNetMsg {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmTerminated(tNetSession *u) : tmNetMsg(u) {}
	tmTerminated(tNetSession * u,uint8_t reason);
	virtual tString additionalFields(tString dbg) const;
	//format
	uint8_t reason;
};

/** Leave */
class tmLeave :public tmNetMsg {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmLeave(tNetSession *u) : tmNetMsg(u) {}
	tmLeave(tNetSession * u,uint8_t reason);
	virtual tString additionalFields(tString dbg) const;
	//format
	uint8_t reason;
};

/** Alive */
class tmAlive :public tmNetMsg {
public:
	tmAlive(tNetSession *u) : tmNetMsg(u) {}
	tmAlive(tNetSession * u,uint32_t ki)
	: tmNetMsg(NetMsgAlive, plNetKi | plNetAck | plNetTimestamp, u)
	{
		this->ki=ki;
	}
};

/** Player terminated */
class tmPlayerTerminated :public tmNetMsg {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmPlayerTerminated(tNetSession* u) : tmNetMsg(u) {}
	tmPlayerTerminated(tNetSession * u,uint32_t ki,uint8_t reason);
	virtual tString additionalFields(tString dbg) const;
	//format
	uint8_t reason;
};

/** Ping */
class tmPing :public tmNetMsg {
public:
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	tmPing(tNetSession *u) : tmNetMsg(u) {}
	tmPing(tNetSession * u, uint8_t dst);
	tmPing(tNetSession *u, tmPing &ping);
	virtual tString additionalFields(tString dbg) const;
	void setRouteInfo(tNetSession *u);
	void unsetRouteInfo();
	//format
	double mtime;
	uint8_t destination;
};

}

#endif
