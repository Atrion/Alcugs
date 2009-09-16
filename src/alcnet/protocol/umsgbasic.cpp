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

/* The Uru protocol, is here */

/* CVS tag - DON'T TOUCH*/
#define __U_UMSGBASIC_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcnet.h"

#include <alcdebug.h>

namespace alc {

tmTerminated::tmTerminated(tNetSession * u,U32 who,Byte what)
 :tmMsgBase(NetMsgTerminated,plNetKi | plNetAck,u) {
	DBG(5,"tmTerminated() who:%i,what:%i\n",who,what);
	ki=who;
	reason=what;
}
void tmTerminated::store(tBBuf &t) {
	tmMsgBase::store(t);
	reason=t.getByte();
}
void tmTerminated::stream(tBBuf &t) const {
	tmMsgBase::stream(t);
	t.putByte(reason);
}
void tmTerminated::additionalFields() {
	dbg.printf("\n Reason [0x%02X] %s ",reason,alcUnetGetReasonCode(reason));
}


tmLeave::tmLeave(tNetSession * u,U32 ki,Byte reason)
 :tmMsgBase(NetMsgLeave,plNetKi,u) {
 // the connection will be dropped immediately after sending this message, the ack reply would already trigger a new one
	this->ki=ki;
	this->reason=reason;
	setUrgent(); // We can not wait long after sending the message, so do it quickly!
}
void tmLeave::store(tBBuf &t) {
	tmMsgBase::store(t);
	reason=t.getByte();
}
void tmLeave::stream(tBBuf &t) const {
	tmMsgBase::stream(t);
	t.putByte(reason);
}
void tmLeave::additionalFields() {
	dbg.printf("\n Reason [0x%02X] %s ",reason,alcUnetGetReasonCode(reason));
}


tmPlayerTerminated::tmPlayerTerminated(tNetSession * u,U32 ki,Byte reason)
 :tmMsgBase(NetMsgPlayerTerminated,plNetKi | plNetAck,u) {
	this->ki=ki;
	this->reason=reason;
}
void tmPlayerTerminated::store(tBBuf &t) {
	tmMsgBase::store(t);
	if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
	reason=t.getByte();
}
void tmPlayerTerminated::stream(tBBuf &t) const {
	tmMsgBase::stream(t);
	t.putByte(reason);
}
void tmPlayerTerminated::additionalFields() {
	dbg.printf("\n Reason [0x%02X] %s ",reason,alcUnetGetReasonCode(reason));
}


tmPing::tmPing(tNetSession * u, Byte dst)
 : tmMsgBase(NetMsgPing,plNetKi | plNetX | plNetAck,u)
{
	ki = x = 0;
	mtime = 0;
	destination = dst;
	if (u->max_version > 12 || (u->max_version == 12 && u->min_version >= 6)) {
		setFlags(plNetTimestamp);
	}
}
tmPing::tmPing(tNetSession *u, tmPing &ping)
 : tmMsgBase(NetMsgPing,ping.flags,u)
{
	ki = ping.ki;
	x = ping.x;
	destination = ping.destination;
	mtime = ping.mtime;
	if (hasFlags(plNetIP | plNetSid)) {
		ip = ping.ip;
		port = ping.port;
		sid = ping.sid;
	}
}
void tmPing::store(tBBuf &t) {
	tmMsgBase::store(t);
	mtime=t.getDouble();
	destination=t.getByte();
}
void tmPing::setRouteInfo(const tNetSessionIte &ite) {
	ip=ite.ip;
	port=ite.port;
	sid=ite.sid;
	setFlags(plNetIP | plNetSid);
}
void tmPing::unsetRouteInfo() {
	unsetFlags(plNetIP | plNetSid);
}
void tmPing::stream(tBBuf &t) const {
	tmMsgBase::stream(t);
	t.putDouble(mtime);
	t.putByte(destination);
}
void tmPing::additionalFields() {
	dbg.printf("\n t:%e, dst:%i %s ",mtime,destination,alcUnetGetDestination(destination));
}


} //namespace
