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

/* The Uru protocol, is here */

#ifndef __U_UMSGBASIC_H
#define __U_UMSGBASIC_H
/* CVS tag - DON'T TOUCH*/
#define __U_UMSGBASIC_H_ID "$Id$"

namespace alc {

/** TERMINATED message */
class tmTerminated :public tmMsgBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmTerminated(tNetSession * u=NULL,U32 ki=0,Byte reason=RKickedOff,bool ack=false);
	//format
	Byte reason;
protected:
	virtual void additionalFields();
};

/** Leave */
class tmLeave :public tmMsgBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmLeave(tNetSession * u=NULL,U32 ki=0,Byte reason=RQuitting);
	//format
	Byte reason;
protected:
	virtual void additionalFields();
};

/** Alive */
class tmAlive :public tmMsgBase {
public:
	tmAlive(tNetSession * u=NULL,U32 ki=0)
 :tmMsgBase(NetMsgAlive,plNetKi | plNetAck | plNetCustom,u) {
		this->ki=ki;
	}
	//format
};

/** Player terminated */
class tmPlayerTerminated :public tmMsgBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmPlayerTerminated(tNetSession * u=NULL,U32 ki=0,Byte reason=RKickedOff);
	tmPlayerTerminated(tNetSession * u,tNetSessionIte &ite,Byte reason=RKickedOff);
	//format
	Byte reason;
protected:
	virtual void additionalFields();
};

/** Ping */
class tmPing :public tmMsgBase {
public:
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	tmPing(tNetSession * u=NULL,double mtime=0,U32 ki=0,U32 x=0,Byte dst=KLobby,bool ack=true);
	void setReply();
	void setRouteInfo(tNetSessionIte &ite);
	void unsetRouteInfo();
	//format
	double mtime;
	Byte destination;
protected:
	virtual void additionalFields();
};

}

#endif
