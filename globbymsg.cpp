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
#define __U_URUGLOBBYMSG_ID "$Id$"

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

#include "globbymsg.h"

#include "debug.h"

/**
	Sends a response to the setmyactive player (server)
*/
int plNetMsgActivePlayerSet(st_unet * net,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgActivePlayerSet;
	if(u->tpots==1) {
		u->hmsg.cmd=NetMsgActivePlayerSet2;
	}
	u->hmsg.flags=(plNetAck | plNetCustom | plNetKi | plNetX);

	print2log(f_uru,"<SND> NetMsgActivePlayerSet id %i\n",u->hmsg.ki);

	ret=plNetSendMsg(net,buf,off,sid,0);
	if(u->tpots==0) {
		u->hmsg.cmd=NetMsgActivePlayerSet2;
		print2log(f_uru,"<SND> NetMsgActivePlayerSet2 (tpots) id %i\n",u->hmsg.ki);
		ret=plNetSendMsg(net,buf,off,sid,0);
	}

	return ret;
}


/**
	Sends a response to the plNetMsgFindAgeReply (server)
*/

int plNetMsgFindAgeReply(st_unet * net,Byte * age_name,Byte * address,int port,Byte * guid,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgFindAgeReply;
	u->hmsg.flags=(plNetAck | plNetCustom | plNetKi | plNetX); //0x00061200;

	print2log(f_uru,"<SND> NetMsgFindAgeReply id %i\n",u->hmsg.ki);

	*(Byte *)(buf+off)=0x1F; //seems a response code, I will try it later ;)
	off++;
	//put the age name
	off+=encode_urustring(buf+off,age_name,strlen((char *)age_name), 0);
	*(Byte *)(buf+off)=0x03; //seems a flag (perhaps), well.., I will try it later ;)
	off++;  //0x03 is kgame, and the log displays kgame as the server type, o well
	//Ip address, also I will try if a domain works..
	off+=encode_urustring(buf+off, (Byte *)address, strlen((char *)address), 0);
	//and now the port
	*(U16 *)(buf+off)=port;
	off+=2;
	//finally copy the guid
	ascii2hex2(buf+off,guid,8);
	off+=8;
	//that's all, nice :)

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}
