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
#define __U_URUGCBVAULTMSG_ID "$Id: urumsg.cpp,v 1.11 2004/11/28 01:33:03 almlys Exp $"

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

#include "gcbvaultmsg.h"

#include "debug.h"


/**
	Sends the vault list request message (client)
*/
int plNetMsgRequestMyVaultPlayerList(st_unet * net,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgRequestMyVaultPlayerList;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	print2log(f_uru,"<SND> RequestMyVaultPlayerList \n");

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

/**
	Create player petition (client)
	//data contains the vault block
*/
int plNetMsgCreatePlayer(st_unet * net,Byte * avie,Byte * gender, Byte * friendn, Byte * key, Byte * data,int data_size,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte * buf; //the out buffer

	//allocate buffer
	buf=(Byte *)malloc(sizeof(Byte) * (data_size + 500));

	u->hmsg.cmd=NetMsgCreatePlayer;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	//avie
	off+=encode_urustring(buf+off,avie,strlen((char *)avie),0x00);
	//gender
	off+=encode_urustring(buf+off,gender,strlen((char *)gender),0x00);
	//friend
	off+=encode_urustring(buf+off,friendn,strlen((char *)friendn),0x00);
	//key
	off+=encode_urustring(buf+off,key,strlen((char *)key),0x00);

	memcpy(buf+off,data,data_size);
	off+=data_size;

	print2log(f_uru,"<SND> NetMsgCreatePlayer\n");

	ret=plNetSendMsg(net,buf,off,sid,0);

	free((void *)buf);
	return ret;
}

/**
 Player deletion message (client)
*/
int plNetMsgDeletePlayer(st_unet * net,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgDeletePlayer;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	print2log(f_uru,"<SND> NetMsgDeletePlayer id %i\n",u->hmsg.ki);

	*(U16 *)(buf+off)=0x00; //unknonw zero value
	off+=2;

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}


