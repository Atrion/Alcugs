/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004  The Alcugs H'uru Server Team                          *
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
#define __U_URUGBVAULTMSG_ID "$Id: urumsg.cpp,v 1.11 2004/11/28 01:33:03 almlys Exp $"

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

#include "uru.h"
#include "config_parser.h"

#include "gbvaultmsg.h"

#include "debug.h"

/**
	Sends the Players list (server)
*/
int plNetMsgVaultPlayerList(st_unet * net,Byte * data,int data_size,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	char * url=NULL;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgVaultPlayerList;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	//put the reason code
	memcpy(buf+off,data,data_size);
	off+=data_size;

	//nice added in version 12.7 :P
	if(u->min_version>=0x07) {
		url=cnf_getString("http://huru.almlys.dyns.net/unconfigured.php",\
		"website","global",global_config);
		off+=encode_urustring(buf+off,(Byte *)url,strlen((char *)url),0);
	}

	print2log(f_uru,"<SND> NetMsgVaultPlayerList to %s \n",u->acct);

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}

/**
	Sends the KI of the new created player, and status of the creation
*/
int plNetMsgPlayerCreated(st_unet * net,Byte status,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgPlayerCreated;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	*(Byte *)(buf+off)=status;
	off++;

	print2log(f_uru,"<SND> Player created, new ki id %i with result %i \n",u->hmsg.ki,status);

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}

