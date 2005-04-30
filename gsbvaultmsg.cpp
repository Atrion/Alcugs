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
#define __U_URUGSBVAULTMSG_ID "$Id$"

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

#include "gsbvaultmsg.h"

#include "debug.h"

/**
	Sends the vault list response message (vault->server)
*/
int plNetMsgCustomVaultPlayerList(st_unet * net,Byte * data,int data_size,int sid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomVaultPlayerList;
	u->hmsg.flags=(plNetX | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}
	if(proto==0) { //new
		u->hmsg.flags |= plNetGUI;
		//a guid should already be in hmsg
	} else { //old protocol
		memcpy(buf+off,u->hmsg.guid,16);
		off+=16;
	}

	if(data==NULL) {
		*(U16 *)(buf+off)=0x01; //set 1 players
		*(S32 *)(buf+off)=-1;
		off+=encode_urustring(buf+off,(Byte *)"#Database Error!",16,0x00);
		*(Byte *)(buf+off)=0x01;
	} else {
		memcpy(buf+off,data,data_size);
		off+=data_size;
	}

	print2log(f_uru,"<SND> NetMsgCustomVaultPlayerList \n");

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

/**
	Sends the KI of the new created player, and status of the creation
*/
int plNetMsgCustomVaultPlayerCreated(st_unet * net,Byte status,int sid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomVaultPlayerCreated;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}
	//the user id
	if(proto==0) { //new
		u->hmsg.flags |= plNetGUI;
		//a guid should already be in hmsg
	} else { //old protocol
		memcpy(buf+off,u->hmsg.guid,16);
		off+=16;
	}

	*(Byte *)(buf+off)=status;
	off++;

	print2log(f_uru,"<SND>NetMsgCustomVaultPlayerCreated, new ki id %i with result %i \n",u->hmsg.ki,status);

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

/** Asks the vault for the player ownership (server->vault)
		//status
		0x01 OK
		elsewhere failed!
*/
int plNetMsgCustomVaultKiChecked(st_unet * net,Byte status,Byte * avie,int sid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomVaultKiChecked;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}
	//the user id
	if(proto==0) { //new
		u->hmsg.flags |= plNetGUI;
		//a guid should already be in hmsg
	} else { //old protocol
		memcpy(buf+off,u->hmsg.guid,16);
		off+=16;
	}

	*(Byte *)(buf+off)=status;
	off++;
	off+=encode_urustring(buf+off,avie,strlen((char *)avie),0);

	print2log(f_uru,"<SND> NetMsgCustomVaultKiChecked id %i\n",u->hmsg.ki);
	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

