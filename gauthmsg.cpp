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
#define __U_URUGAUTHMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include "stdebug.h"
#include "protocol.h"
#include "prot.h"
#include "urunet.h"

#include "md5.h"
#include "useful.h"
#include "conv_funs.h"

#include "gauthmsg.h"

#include "debug.h"

/**
	Sends the Challenge (it automatically calcules a challenge)
	for authresult, see authresponse for a complete list (server)
*/
int plNetMsgAuthenticateChallenge(st_unet * net,Byte authresult,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	DBG(5,"result:%i,sid:%i\n",authresult,sid);

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgAuthenticateChallenge;
	u->hmsg.flags=(plNetKi | plNetAck | plNetX | plNetVersion | plNetCustom);

	Byte aux_hash[60]; //challenge aux hash

	print2log(f_uru,"<SND> AuthenticateChallenge v%i,%i \n",u->hmsg.max_version,\
	u->hmsg.min_version);

	//put the reason code
	*(buf+off)=authresult; //helloResult
	off++;

	//init the challenge to the MD5 of the current system time, and other garbage
	time((time_t *)(aux_hash));
	*(U32 *)(aux_hash+4)=(u->client.cs);
	*(U32 *)(aux_hash+8)=get_microseconds();
	*(U32 *)(aux_hash+12)=(u->server.cs);
	srandom(*(U32 *)(aux_hash));
	*(U32 *)(aux_hash+16)=(U32)random();
	memcpy(aux_hash+20,u->acct,8);
	memcpy(aux_hash+28,net->name,4);

	MD5(aux_hash,32,u->challenge);
	hex2ascii2(aux_hash,u->challenge,16);

	print2log(f_uru," helloResult:%i,challenge:%s\n",authresult,aux_hash);

	//put the challenge
	off+=encode_urustring(buf+off,u->challenge,0x10,0x00);

	DBG(5,"before plNetSend size:%i bytes sid:%i\n",off,sid);
	ret=plNetSendMsg(net,buf,off,sid,0);
	DBG(5,"after plNetSend ret:%i\n",ret);

	return ret;
}

/**
	Sends the Auth response (server)
*/
int plNetMsgAccountAuthenticated(st_unet * net,Byte authresult,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgAccountAuthenticated;  //code1=0x0394;
	u->hmsg.flags=(plNetAck | plNetCustom | plNetX | plNetKi | plNetGUI);

	print2log(f_uru,"<SND> AccountAuthenticated ");
	print2log(f_uru,"auth_result: %i %s\n",authresult,unet_get_auth_code(authresult));

	//account identifier
	// ID1(4 bytes)-ID2(2 bytes)-ID3(2 bytes)-ID4(2 bytes)-ID5(6 bytes)
	//memcpy(buf+off,u->hmsg.guid,16);
	//off+=16;

	// authResult
	*(buf+off)=authresult;
	off++;
	// serverGuid (8 bytes)
	if(authresult==0x00) {
		ascii2hex2(buf+off,(Byte *)net->guid,8);
		off+=8;
	} else {
		memset(buf+off,0,8);
		off+=8;
	}

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

