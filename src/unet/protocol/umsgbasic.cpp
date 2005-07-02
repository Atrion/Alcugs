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
#include "urunet/unet.h"

#include "alcdebug.h"

namespace alc {

tmTerminated::tmTerminated(tNetSession * u,U32 who,Byte what,bool ack)
 :tmMsgBase(NetMsgTerminated,plNetKi | plNetCustom,u) {
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
Byte * tmTerminated::str() {
	#ifdef _UNET_MSGDBG_
	tmMsgBase::str();
	dbg.end();
	dbg.seek(-1);
	dbg.printf("Reason [%i] %s ",reason,alcUnetGetReasonCode(reason));
	dbg.putByte(0);
	dbg.rewind();
	return dbg.read();
	#else
	return tmMsgBase::str();
	#endif
}



} //namespace
