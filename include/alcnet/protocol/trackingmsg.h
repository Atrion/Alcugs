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

#ifndef __U_TRACKINGMSG_H
#define __U_TRACKINGMSG_H
/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGMSG_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	class tmCustomSetGuid : public tmMsgBase {
	public:
		tmCustomSetGuid(tNetSession *u);
		tmCustomSetGuid(tNetSession *u, const Byte *serverGuid, const Byte *age, const Byte *externalIp);
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
		// format
		tUStr serverGuid, age, externalIp;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomPlayerStatus : public tmMsgBase {
	public:
		tmCustomPlayerStatus(tNetSession *u);
		tmCustomPlayerStatus(tNetSession *u, U32 ki, U32 x, const Byte *uid, const Byte *account, const Byte *avatar, Byte playerFlag, Byte playerStatus);
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
		// format
		tUStr account, avatar;
		Byte playerFlag, playerStatus;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomFindServer : public tmMsgBase {
	public:
		tmCustomFindServer(tNetSession *u);
		virtual void store(tBBuf &t);
		// format
		tUStr serverGuid, age;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomForkServer : public tmMsgBase {
	public:
		tmCustomForkServer(tNetSession *u, U32 ki, U32 x, U16 port, const Byte *serverGuid, const Byte *name, bool loadSDL);
		virtual int stream(tBBuf &t);
		// format
		U16 fork_port;
		tUStr serverGuid, age;
		Byte loadSDL;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomServerFound : public tmMsgBase {
	public:
		tmCustomServerFound(tNetSession *u, U32 ki, U32 x, U16 port, const Byte *ip_str, const Byte *serverGuid, const Byte *name);
		virtual int stream(tBBuf &t);
		// format
		U16 server_port;
		tUStr ip_str, serverGuid, age;
	protected:
		virtual void additionalFields();
	};
	
	class tmCustomDirectedFwd : public tmMsgBase {
	public:
		tmCustomDirectedFwd(tNetSession *u);
		tmCustomDirectedFwd(tNetSession *u, tmCustomDirectedFwd &directedFwd);
		virtual ~tmCustomDirectedFwd(void);
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
		// format
		tMBuf gameMessage; // saves the complete message
		Byte nRecipients;
		U32 *recipients;
	};
	
} //End alc namespace

#endif
