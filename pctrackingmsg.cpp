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

/*
	Basic messages parser
*/

/* CVS tag - DON'T TOUCH*/
#define __U_PCTRACKING_MSG_ID "$Id: pservermsg.cpp,v 1.7 2004/11/28 01:33:03 almlys Exp $"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_types.h"
#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "conv_funs.h"

#include "trackingsubsys.h"

#include "gbasicmsg.h"

#include "pctrackingmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_ctracking_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
	if(net_check_address(net,sid)!=0) return UNET_OUTOFRANGE;
	st_uru_client * u=&net->s[sid];

#if _DBG_LEVEL_ > 3
	if(size>0) {
		nlog(net->log,net,sid,"Recieved a packet of %i bytes...\n",size);
		dump_packet(net->log,buf,size,0,7);
		print2log(net->log,"\n-------------\n");
	}
	if(size>1024*256) {
		plog(net->err,"Attention Packet is bigger than 256KBytes, that Is impossible!\n");
		DBG(5,"Abort Condition");
		abort();
		return UNET_TOOBIG;
	}
#endif

	int n=0,off=0,proto=0;//dsid=-1;
	//int i;
	char name[100];

	switch(u->hmsg.cmd) {
		case NetMsgCustomPlayerStatus:
			Byte flag;
			Byte status;
			char acct[100];
			if(!(u->hmsg.flags & plNetGUI)) {
				memcpy(u->hmsg.guid,buf+off,16);
				off+=16;
				proto=1;
			}
			off+=decode_urustring((Byte *)acct,buf+off,99);
			off+=2;
			off+=decode_urustring((Byte *)name,buf+off,99);
			off+=2;
			flag=*(Byte *)(buf+off);
			off++;
			status=*(Byte *)(buf+off);
			off++;
			print2log(f_uru,"<RCV> NetMsgCustomPlayerStatus id %i,%s,%s, %02X,%02X\n",u->hmsg.ki,u->acct,u->name,flag,status);
			tracking_subsys_update(net,u->hmsg.ki,(char *)u->hmsg.guid,acct,name,flag,status,sid);
			n=1;
			break;
		case NetMsgCustomSetGuid:
			print2log(f_uru,"<RCV> NetMsgCustomSetGuid\n");
			Byte private_mask[51];
			U32 int_private_mask;
			off+=decode_urustring((Byte *)u->guid,buf+off,100); //guid
			off+=2;
			off+=decode_urustring((Byte *)u->name,buf+off,100); //age file name
			off+=2;
			off+=decode_urustring((Byte *)private_mask,buf+off,50);
			off+=2;
			//u->private_mask=(U32)inet_network((char *)private_mask); <-- WRONG!
			int_private_mask=(U32)inet_addr((char *)private_mask);
			off+=decode_urustring((Byte *)u->acct,buf+off,100); //public address
			off+=2;
			plog(f_uru,"Game server %s [%s] Says hello (%s(%s):%i)\n",u->name,u->guid,u->acct,\
			get_ip(u->ip),(ntohs(u->port)));
			if(!strcmp(u->name,"Lobby")) {
				u->whoami=KLobby;
			} else if(!strcmp(u->name,"Data")) {
				u->whoami=KData;
			} else if(!strcmp(u->name,"Meta")) {
				u->whoami=KMeta;
			} else if(!strcmp(u->name,"Admin")) {
				u->whoami=KAdmin;
			} else {
				u->whoami=KGame;
			}
			tracking_subsys_addgame(net,sid);
			n=1;
			break;
		case NetMsgCustomFindServer:
			print2log(f_uru,"<RCV> NetMsgCustomFindServer\n");
			char guid[30];
			U32 cip;
			off+=decode_urustring((Byte *)guid,buf+off,30);
			off+=2;
			off+=decode_urustring((Byte *)name,buf+off,99);
			off+=2;
			cip=*(U32 *)(buf+off);
			off+=4;
			tracking_subsys_findserver(net,guid,name,cip,sid);
			n=1;
			break;
		default:
			n=UNET_OK;
			break;
	}

	return n;
}

