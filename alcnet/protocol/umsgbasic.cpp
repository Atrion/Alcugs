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

/* CVS tag - DON'T TOUCH*/
#define __U_UMSGBASIC_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "umsgbasic.h"

#include "netexception.h"
#include "netsessionmgr.h"

namespace alc {

tmTerminated::tmTerminated(tNetSession * u,uint8_t what)
 : tmNetMsg(NetMsgTerminated,plNetKi | plNetAck,u) {
	ki=u->ki;
	reason=what;
	urgent = true; // We can not wait long after sending the message, so do it quickly!
}
void tmTerminated::store(tBBuf &t) {
	tmNetMsg::store(t);
	reason=t.get8();
}
void tmTerminated::stream(tBBuf &t) const {
	tmNetMsg::stream(t);
	t.put8(reason);
}
tString tmTerminated::additionalFields(tString dbg) const {
	dbg.printf("\n Reason [0x%02X] %s ",reason,alcUnetGetReasonCode(reason));
	return dbg;
}


tmLeave::tmLeave(tNetSession * u,uint8_t reason)
 :tmNetMsg(NetMsgLeave,plNetKi|plNetAck,u) {
 /* The Uru client sends the leaves with the ack flag enabled, so we do it, too - but this is problematic:
 The connection is dropped ASAP after sending this message, but the ack reply has to be sent, and it msut not trigger a new connection on the other end */
	this->ki=u->ki;
	this->reason=reason;
	urgent = true; // We can not wait long after sending the message, so do it quickly!
}
void tmLeave::store(tBBuf &t) {
	tmNetMsg::store(t);
	reason=t.get8();
}
void tmLeave::stream(tBBuf &t) const {
	tmNetMsg::stream(t);
	t.put8(reason);
}
tString tmLeave::additionalFields(tString dbg) const {
	dbg.printf("\n Reason [0x%02X] %s ",reason,alcUnetGetReasonCode(reason));
	return dbg;
}


tmPlayerTerminated::tmPlayerTerminated(tNetSession * u,uint32_t ki,uint8_t reason)
 :tmNetMsg(NetMsgPlayerTerminated,plNetKi | plNetAck,u) {
	this->ki=ki;
	this->reason=reason;
}
void tmPlayerTerminated::store(tBBuf &t) {
	tmNetMsg::store(t);
	if (!hasFlags(plNetKi)) throw txProtocolError(_WHERE("KI flag missing"));
	reason=t.get8();
}
void tmPlayerTerminated::stream(tBBuf &t) const {
	tmNetMsg::stream(t);
	t.put8(reason);
}
tString tmPlayerTerminated::additionalFields(tString dbg) const {
	dbg.printf("\n Reason [0x%02X] %s ",reason,alcUnetGetReasonCode(reason));
	return dbg;
}


tmPing::tmPing(tNetSession * u, uint8_t dst)
 : tmNetMsg(NetMsgPing,plNetKi | plNetX | plNetAck,u)
 // indeed acking a ping does not make much sense... however, this gives us some really good data when a player connects to a game server (there are pings exchanged then) to estimate the latency of the user's connection
{
	ki = x = 0;
	mtime = 0;
	destination = dst;
	if (u->max_version > 12 || (u->max_version == 12 && u->min_version >= 6)) {
		setFlags(plNetTimestamp);
	}
}
tmPing::tmPing(tNetSession *u, tmPing &ping)
 : tmNetMsg(NetMsgPing,ping.flags,u)
{
	ki = ping.ki;
	x = ping.x;
	destination = ping.destination;
	mtime = ping.mtime;
	if (hasFlags(plNetSid))
		sid = ping.sid;
}
void tmPing::store(tBBuf &t) {
	tmNetMsg::store(t);
	mtime=t.getDouble();
	destination=t.get8();
}
void tmPing::setRouteInfo(tNetSession *u) {
	sid=u->getSid();
	setFlags(plNetSid);
}
void tmPing::unsetRouteInfo() {
	unsetFlags(plNetSid);
}
void tmPing::stream(tBBuf &t) const {
	tmNetMsg::stream(t);
	t.putDouble(mtime);
	t.put8(destination);
}
tString tmPing::additionalFields(tString dbg) const {
	dbg.printf("\n t:%e, dst:%i %s ",mtime,destination,alcUnetGetDestination(destination));
	return dbg;
}


} //namespace
