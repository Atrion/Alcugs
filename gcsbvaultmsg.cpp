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
#define __U_URUGCSBVAULTMSG_ID "$Id: urumsg.cpp,v 1.11 2004/11/28 01:33:03 almlys Exp $"

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

#include "gcsbvaultmsg.h"

#include "debug.h"


/**
	Sends the vault list request message (server->vault)
*/
int plNetMsgCustomVaultAskPlayerList(st_unet * net,int sid,int ssid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	if(net_check_address(net,ssid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomVaultAskPlayerList;
	u->hmsg.flags=(plNetX | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}
	if(proto==0) { //new
		u->hmsg.flags |= plNetGUI;
		memcpy(u->hmsg.guid,s->uid,16);
	} else { //old protocol
		memcpy(buf+off,s->uid,16);
		off+=16;
	}

	print2log(f_uru,"<SND> NetMsgCustomVaultAskPlayerList \n");

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}

/**
	Create player petition (server->vault)
	//data contains the vault block
*/
int plNetMsgCustomVaultCreatePlayer(st_unet * net,Byte * data,int data_size,int sid,int ssid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	if(net_check_address(net,ssid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	int off=0,ret=0;

	Byte * buf=NULL; //the out buffer

	//allocate buffer
	buf=(Byte *)malloc(sizeof(Byte) * (data_size + 500));

	u->hmsg.cmd=NetMsgCustomVaultCreatePlayer;
	u->hmsg.flags=(plNetX | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}
	if(proto==1) {
		u->hmsg.flags |= plNetKi;
	}

	print2log(f_uru,"<SND> NetMsgCustomVaultCreatePlayer ");

	//put the login
	off+=encode_urustring(buf+off,(Byte *)s->acct,strlen((char *)s->acct),1);

	if(proto==0) { //new
		u->hmsg.flags |= plNetGUI;
		memcpy(u->hmsg.guid,s->uid,16);
	} else { //old protocol
		memcpy(buf+off,s->uid,16);
		off+=16;
	}

	Byte guid_aux[50];
	hex2ascii2(guid_aux,(Byte *)s->uid,16);
	print2log(f_uru," guid:%s\n",guid_aux);

	//put access level
	*(Byte *)(buf+off)=s->access_level;
	off++;

	memcpy(buf+off,data,data_size);
	off+=data_size;

	ret=plNetSendMsg(net,buf,off,sid,0);
	free((void *)buf);
	return ret;
}


/**
	Sends the vault list request message (server->vault)
*/
int plNetMsgCustomVaultDeletePlayer(st_unet * net,int sid,int ssid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	if(net_check_address(net,ssid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomVaultDeletePlayer;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}
	if(proto==0) { //new
		u->hmsg.flags |= plNetGUI;
		memcpy(u->hmsg.guid,s->uid,16);
	} else { //old protocol
		memcpy(buf+off,s->uid,16);
		off+=16;
	}

	*(Byte *)(buf+off)=s->access_level;
	off++;

	print2log(f_uru,"<SND> NetMsCustomVaultDeletePlayer id %i\n",u->hmsg.ki);

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}


/** Asks the vault for the player ownership (game->vault)
*/
int plNetMsgCustomVaultCheckKi(st_unet * net,int sid,int ssid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	if(net_check_address(net,ssid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomVaultCheckKi;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}
	if(proto==0) { //new
		u->hmsg.flags |= plNetGUI;
		memcpy(u->hmsg.guid,s->uid,16);
	} else { //old protocol
		memcpy(buf+off,s->uid,16);
		off+=16;
	}

	print2log(f_uru,"<SND> NetMsgCustomVaultCheckKi id %i\n",u->hmsg.ki);

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}

/** Vault Update Player status (game-->vault)
*/
int plNetMsgCustomVaultPlayerStatus(st_unet * net,char * age,char * guid,Byte state,U32 online_time,int sid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomVaultPlayerStatus;
	u->hmsg.flags=(plNetKi | plNetX | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}

	off+=encode_urustring(buf+off,(Byte *)age,strlen((char *)age),0);
	off+=encode_urustring(buf+off,(Byte *)guid,strlen((char *)guid),0);
	*(Byte *)(buf+off)=state;
	off++;
	*(U32 *)(buf+off)=online_time;
	off+=4;

	print2log(f_uru,"<SND> NetMsgCustomVaultPlayerStatus p:%i\n",state);

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}


