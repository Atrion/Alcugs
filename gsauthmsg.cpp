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
#define __U_URUGSAUTHMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "stdebug.h"
#include "protocol.h"
#include "prot.h"
#include "urunet.h"

#include "conv_funs.h"

#include "gsauthmsg.h"

#include "debug.h"

/** Asks the auth server to verify an user
		proto: 0x01 old protocol
*/
int plNetMsgCustomAuthResponse(st_unet * net,Byte * login,Byte result,Byte * passwd,Byte access_level,int sid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomAuthResponse;

	u->hmsg.flags=(plNetAck | plNetCustom);

	if(proto==1) {
		u->hmsg.flags |= plNetVersion | plNetX;
	} else {
		u->hmsg.flags |= plNetIP | plNetGUI | plNetX;
		if(!(net->flags & UNET_NETAUTH)) {
			u->hmsg.flags |= plNetVersion;
		}
	}

	//login
	off+=encode_urustring(buf+off,login,strlen((char *)login),0x00);
	//result
	*(Byte *)(buf+off)=result;
	off++;
	//passwd
	off+=encode_urustring(buf+off,passwd,32,0x00);
	if(proto==1) {
		//GUID
		memcpy(buf+off,u->hmsg.guid,16);
		off+=16;
	}
	//access_level
	*(Byte *)(buf+off)=access_level;
	off++;

	print2log(f_uru,"<SND> NetMsgCustomAuthResponse \n");

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}


