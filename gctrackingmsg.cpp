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
#define __U_URUGCTRACKINGMSG_ID "$Id$"

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

#include "gctrackingmsg.h"

#include "debug.h"

/** Sends player status to the tracking server (server->tracking)
	Flag
	0-> delete
	1-> set invisible
	2-> set visible
	3-> set only buddies
	<---->
	Status
	RStopResponding 0x00
	RInroute 0x16
	RArriving 0x17
	RJoining 0x18
	RLeaving 0x19
	RQuitting 0x1A
	<----->
*/
int plNetMsgCustomPlayerStatus(st_unet * net,Byte flag,Byte status,int sid,int ssid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	if(net_check_address(net,ssid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomPlayerStatus;
	u->hmsg.flags=(plNetX | plNetKi | plNetAck | plNetCustom);

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
	//remember to update the KI number!
	u->hmsg.ki=s->ki; //don't trus never (source->hmsg.ki), but the server should be killing anyone that just sends a different ki number that the one issued and validated on the SetPlayer

	print2log(f_uru,"<SND> NetMsgCustomPlayerStatus id %i,%s,%s, %02X,%02X\n",s->ki,s->acct,s->name,flag,status);

	off+=encode_urustring(buf+off,(Byte *)s->acct,strlen((char *)s->acct),0);
	off+=encode_urustring(buf+off,(Byte *)s->name,strlen((char *)s->name),0);
	*(Byte *)(buf+off)=flag;
	off++;
	*(Byte *)(buf+off)=status;
	off++;

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}

/** --- Set the server guid (game-->tracking)
*/
int plNetMsgCustomSetGuid(st_unet * net,int sid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomSetGuid;
	u->hmsg.flags=(plNetX | plNetKi | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}

	print2log(f_uru,"<SND> NetMsgCustomSetGuid\n");

	off+=encode_urustring(buf+off,(Byte *)net->guid,strlen((char *)net->guid),1);
	off+=encode_urustring(buf+off,(Byte *)net->name,strlen((char *)net->name),0);

	if(proto==1) {
		char * aux;
		aux=cnf_getString("255.255.255.0","private_mask","global",global_config);
		off+=encode_urustring(buf+off,(Byte *)aux,strlen((char*)aux),0);
		off+=encode_urustring(buf+off,(Byte *)net->address,strlen((char*)net->address),0);
	}

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}

/** ---- Find server (game-->tracking)
*/
int plNetMsgCustomFindServer(st_unet * net,char * guid,char * age_fname,int sid,int ssid,Byte proto) {
	if(net_check_address(net,sid)!=0) { return -1; }
	if(net_check_address(net,ssid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgCustomFindServer;
	u->hmsg.flags=(plNetX | plNetKi | plNetAck | plNetCustom);

	if(proto==1 || !(net->flags & UNET_NETAUTH)) {
		u->hmsg.flags |= plNetVersion;
	}

	print2log(f_uru,"<SND> NetMsgCustomFindServer %s,%s\n",guid,age_fname);

	off+=encode_urustring(buf+off,(Byte *)guid,strlen((char *)guid),1);
	off+=encode_urustring(buf+off,(Byte *)age_fname,strlen((char *)age_fname),0);

	if(proto==1) {
		*(U32 *)(buf+off)=s->ip;
		off+=4;
	} else {
		u->hmsg.flags |= plNetIP;
		u->hmsg.ip=s->ip;
		u->hmsg.port=s->port;
	}

	ret=plNetSendMsg(net,buf,off,sid,0);
	return ret;
}

