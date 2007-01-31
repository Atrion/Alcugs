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
	Basic elemental Uru messages
*/

/* CVS tag - DON'T TOUCH*/
#define __U_URUGTRACKINGMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stdebug.h"
#include "protocol.h"
#include "prot.h"
#include "urunet.h"

#include "conv_funs.h"

#include "gtrackingmsg.h"

#include "debug.h"

/**
 Server Found (tracking-->game)
*/
int plNetMsgCustomServerFound(st_unet * net,Byte * address,int port,Byte * guid,Byte * age_fname,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomServerFound;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);
	if(!(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}

	//data
	*(U16 *)(buf+off)=port;
	off+=2;
	off+=encode_urustring(buf+off,address,strlen((char *)address),0);
	off+=encode_urustring(buf+off,guid,strlen((char *)guid),1);
	off+=encode_urustring(buf+off,age_fname,strlen((char *)age_fname),0);

	print2log(f_uru,"<SND> NetMsgCustomServerFound %s:%i,%s,%s\n",address,port,\
	guid,age_fname);

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

/**
	-- Fork Server (tracking-->lobby)
*/
int plNetMsgCustomForkServer(st_unet * net,int port,Byte * guid,Byte * age_fname,Byte loadstate,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomForkServer;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);
	if(!(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}

	//data
	*(U16 *)(buf+off)=port;
	off+=2;
	off+=encode_urustring(buf+off,guid,strlen((char *)guid),1);
	off+=encode_urustring(buf+off,age_fname,strlen((char *)age_fname),0);
	*(buf+off)=loadstate;
	off++;

	print2log(f_uru,"<SND> NetMsgCustomForkServer p:%i,g:%s,af:%s,ls:%i\n",port,guid,age_fname,loadstate);

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

/**
	-- DirectedFwd (tracking-->game)
*/
int plNetMsgCustomDirectedFwd(st_unet * net,Byte *buf,int size,int fromki,int sid) {
	int ret;

	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];

	u->hmsg.cmd=NetMsgCustomDirectedFwd;
	u->hmsg.flags=(plNetAck | plNetKi | plNetCustom);
	u->hmsg.ki=fromki;
	if(!(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}

	//data
	print2log(f_uru,"<SND> NetMsgCustomDirectedFwd\n");

	ret=plNetSendMsg(net,buf,size,sid,0);

	return ret;
}

