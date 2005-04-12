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
#define __U_URUGBASICMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stdebug.h"
#include "protocol.h"
#include "prot.h"
#include "urunet.h"

#include "gbasicmsg.h"

#include "debug.h"

/**
	\brief Sends a ping, to the host specified in the session struct (client <--> server)
	\param flags 0x01 echo reply,
				 0x00 new ping request
*/
int plNetMsgPing(st_unet * net,U16 flags,double mtime,Byte destination,int sid,int ssid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	if(net_check_address(net,ssid)!=0) { ssid=sid; }
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgPing; //0x025D

	switch (flags) {
		case 0x01: //echo reply (must be the same packet echoed)
			if(ssid!=sid) { return -1; }
			u->hmsg.flags &= ~plNetAck; //dissable the ack
			//u->hmsg.flags &= ~plNetIP; //avoid the ip flag
			break;
		case 0x02: //route the packet to another host with some routing information
			if(ssid==sid) { return -1; }
			//u->hmsg.flags = s->hmsg.flags;
			copy_plNetMsg_header(net,sid,ssid,s->hmsg.flags);
			u->hmsg.flags |= plNetIP; //set the peer address as routing
			u->hmsg.ip = ntohl(s->ip);
			u->hmsg.port = ntohs(s->port);
			break;
		default: //Start a normal ping query
			//u->hmsg.flags |= plNetAck; //set ack
			if(u->min_version>=0x06) {
				DBG(2,"V2 ping packet 12.%i\n",u->min_version);
				u->hmsg.flags=plNetTimestamp | plNetAck | plNetCustom | plNetKi | plNetX;
			} else {
				DBG(2,"V1 ping packet 12.%i\n",u->min_version);
				u->hmsg.flags=plNetAck | plNetCustom | plNetKi | plNetX;
			}
			break;
	}

	print2log(net->log,"<SND> Ping ");
	print2log(net->log," t:%e,dst:%i %s\n",mtime,destination,\
	unet_get_destination(destination));

	*(double *)(buf+off)=mtime; //put that time in the packet
	off+=8;
	*(Byte *)(buf+off)=destination; //put the destination
	off++;

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

/**
	Sends a MsgLeave, to the host specified in the session struct
	 ((seems useless for a server, but useful for a client)) (client)
*/
int plNetMsgLeave(st_unet * net,Byte reason,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgLeave; //0x025C: //plNetMsgLeave
	u->hmsg.flags=(plNetKi | plNetAck | plNetCustom);

	//put the reason code
	*(Byte *)(buf+off)=reason;
	off++;

	print2log(net->log,"<SND> Leave for %i reason %i\n",u->hmsg.ki,reason);

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}


/**
	Sends the TERMINATED message
*/
int plNetMsgTerminated(st_unet * net,Byte code,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgTerminated; //0x02C6
	u->hmsg.flags=(plNetKi | plNetCustom);

	//put the reason code
	*(Byte *)(buf+off)=code;
	off++;

	print2log(f_uru,"<SND> Terminated. Reason [%i] %s ",\
	code,unet_get_reason_code(code));

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

/**
	Sends the TERMINATED message
*/
int plNetMsgPlayerTerminated(st_unet * net,Byte code,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgPlayerTerminated; //0x02C6
	u->hmsg.flags=(plNetKi | plNetCustom);

	//put the reason code
	*(Byte *)(buf+off)=code;
	off++;

	print2log(f_uru,"<SND> Terminated. Reason [%i] %s ",\
	code,unet_get_reason_code(code));

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}


/**
	Send the alive message (client)
*/
int plNetMsgAlive(st_unet * net,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgAlive;
	u->hmsg.flags=(plNetKi | plNetAck | plNetCustom);

	if(u->min_version>=0x06) {
		//0x00061000; 0x00061001;
		u->hmsg.flags|=plNetTimestamp;
	}

	print2log(f_uru,"<SND> Alive \n");

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}


