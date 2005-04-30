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
#define __U_URUGSCAUTHMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "stdebug.h"
#include "protocol.h"
#include "prot.h"
#include "urunet.h"

#include "conv_funs.h"

#include "gscauthmsg.h"

#include "debug.h"

/** Asks the auth server to verify an user
		proto: 0x01 old protocol
*/
int plNetMsgCustomAuthAsk(st_unet * net,Byte * hash,int sid,int ssid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	if(net_check_address(net,ssid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomAuthAsk;
	u->hmsg.flags=(plNetAck | plNetCustom);

	if(proto==1) {
		u->hmsg.flags |= plNetVersion | plNetX;
	} else {
		u->hmsg.flags |= plNetIP | plNetX;
		if(!(net->flags & UNET_NETAUTH)) {
			u->hmsg.flags |= plNetVersion;
		}
	}

	u->hmsg.max_version=12;
	u->hmsg.min_version=6;

	//used for routing.. and validation..
	u->hmsg.ip=s->ip;
	u->hmsg.port=s->port;

	//login
	off+=encode_urustring(buf+off,(Byte *)s->acct,strlen((char *)s->acct),0x00);
	//challenge
	memcpy(buf+off,s->challenge,16);
	off+=16;
	//hash (client answer)
	memcpy(buf+off,hash,16);
	off+=16;
	//build
	*(Byte *)(buf+off)=s->release;
	off++;
	//ip
	if(proto==1) {
		*(U32 *)(buf+off)=s->ip;
		off+=4;
	}

	print2log(net->log,"<SND> NetMsgCustomAuthAsk \n");

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

