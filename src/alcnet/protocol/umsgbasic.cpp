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

/* CVS tag - DON'T TOUCH*/
#define __U_UMSGBASIC_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"
#include "unet.h"

#include "alcdebug.h"

namespace alc {

tmTerminated::tmTerminated(tNetSession * u,U32 who,Byte what,bool ack)
 :tmMsgBase(NetMsgTerminated,plNetKi | plNetCustom,u) {
	DBG(5,"tmTerminated() who:%i,what:%i,ack:%i\n",who,what,ack);
	if(ack)
		setFlags(plNetAck);
	ki=who;
	reason=what;
}
void tmTerminated::store(tBBuf &t) {
	tmMsgBase::store(t);
	reason=t.getByte();
}
int tmTerminated::stream(tBBuf &t) {
	int off;
	off=tmMsgBase::stream(t);
	t.putByte(reason);
	off++;
	return off;
}
void tmTerminated::additionalFields() {
	dbg.printf("\n Reason [%i] %s ",reason,alcUnetGetReasonCode(reason));
}


tmLeave::tmLeave(tNetSession * u,U32 ki,Byte reason)
 :tmMsgBase(NetMsgLeave,plNetKi | plNetAck | plNetCustom,u) {
	this->ki=ki;
	this->reason=reason;
}
void tmLeave::store(tBBuf &t) {
	tmMsgBase::store(t);
	reason=t.getByte();
}
int tmLeave::stream(tBBuf &t) {
	int off;
	off=tmMsgBase::stream(t);
	t.putByte(reason);
	off++;
	return off;
}
void tmLeave::additionalFields() {
	dbg.printf("\n Reason [%i] %s ",reason,alcUnetGetReasonCode(reason));
}


tmPlayerTerminated::tmPlayerTerminated(tNetSession * u,U32 ki,Byte reason)
 :tmMsgBase(NetMsgPlayerTerminated,plNetKi | plNetCustom,u) {
	this->ki=ki;
	this->reason=reason;
}
tmPlayerTerminated::tmPlayerTerminated(tNetSession * u,tNetSessionIte &ite,Byte reason)
 :tmMsgBase(NetMsgPlayerTerminated,plNetIP | plNetSid | plNetCustom,u) {
	ip=ite.ip;
	port=ite.port;
	sid=ite.sid;
	this->reason=reason;
}
void tmPlayerTerminated::store(tBBuf &t) {
	tmMsgBase::store(t);
	reason=t.getByte();
}
int tmPlayerTerminated::stream(tBBuf &t) {
	int off;
	if((flags & plNetIP) && u && u->proto!=0 && u->proto<3) throw txProtocolError(_WHERE("Unsuported message in Alcugs protocol <3"));
	off=tmMsgBase::stream(t);
	t.putByte(reason);
	off++;
	return off;
}
void tmPlayerTerminated::additionalFields() {
	dbg.printf("\n Reason [%i] %s ",reason,alcUnetGetReasonCode(reason));
}


tmPing::tmPing(tNetSession * u,double mtime,U32 ki,U32 x,Byte dst,bool ack)
 :tmMsgBase(NetMsgPing,plNetKi | plNetX | plNetCustom,u) {
	this->ki=ki;
	this->x=x;
	this->mtime=mtime;
	this->destination=dst;
	if(u && u->min_version>=6) {
		setFlags(plNetTimestamp);
	}
	if(ack) setFlags(plNetAck);
}
void tmPing::store(tBBuf &t) {
	tmMsgBase::store(t);
	mtime=t.getDouble();
	destination=t.getByte();
}
void tmPing::setReply() {
	unsetFlags(plNetAck);
}
void tmPing::setRouteInfo(tNetSessionIte &ite) {
	ip=ite.ip;
	port=ite.port;
	sid=ite.sid;
	setFlags(plNetIP || plNetSid);
}
void tmPing::unsetRouteInfo() {
	unsetFlags(plNetIP || plNetSid);
}
int tmPing::stream(tBBuf &t) {
	int off;
	if((flags & plNetSid) && u && u->proto!=0 && u->proto<3) unsetFlags(plNetSid);
	off=tmMsgBase::stream(t);
	t.putDouble(mtime);
	t.putByte(destination);
	off+=9;
	return off;
}
void tmPing::additionalFields() {
	dbg.printf("\n t:%e, dst:%i %s ",mtime,destination,alcUnetGetDestination(destination));
}


} //namespace
