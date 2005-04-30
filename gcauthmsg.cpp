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
	Client Auth messages generator
*/

/* CVS tag - DON'T TOUCH*/
#define __U_URUGCAUTHMSG_ID "$Id: urumsg.cpp,v 1.11 2004/11/28 01:33:03 almlys Exp $"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stdebug.h"
#include "protocol.h"
#include "prot.h"
#include "urunet.h"

#include "md5.h"
#include "conv_funs.h"

#include "gcauthmsg.h"

#include "debug.h"

/**
	Sends the AuthenticateHello (client)
*/
int plNetAuthenticateHello(st_unet * net,Byte * account,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgAuthenticateHello;
	u->hmsg.flags=(plNetKi | plNetX | plNetCustom | plNetVersion);

	//store the login name
	off+=encode_urustring(buf+off,account,strlen((const char *)account),0);
	*(U16 *)(buf+off)=u->maxPacketSz; //put the maxPacketSz
	off+=2;
	*(Byte *)(buf+off)=u->release; //put the build
	off++;

	print2log(f_uru,"<SND> AuthenticateHello v%i,%i ",\
	u->hmsg.max_version,u->hmsg.min_version);
	print2log(f_uru," acctName:%s,maxPacketSz:%i,build:%i %s\n",account,u->maxPacketSz,\
	u->release,unet_get_release((int)u->release));

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

/**
	Sends the Response AuthenticateResponse (client)
*/
int plNetMsgAuthenticateResponse(st_unet * net,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int off=0,ret=0;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->hmsg.cmd=NetMsgAuthenticateResponse;
	u->hmsg.flags=(plNetAck | plNetKi | plNetX | plNetCustom);

	Byte aux_hash[33]; //auth response hash

	//calculate response hash
	Byte * md5buffer;

	md5buffer=(Byte *)malloc((strlen((const char *)u->acct)+32+32+1)*sizeof(Byte));
	hex2ascii2(md5buffer,u->challenge,16);
	strcat((char *)md5buffer,(char *)u->acct);
	strcat((char *)md5buffer,(char *)u->passwd);
	MD5(md5buffer,32+32+strlen((const char *)u->acct),aux_hash);
	free((void *)md5buffer);

	//put the challenge
	off+=encode_urustring(buf+off,aux_hash,0x10,0x00);

	print2log(f_uru,"<SND> AuthenticateResponse \n");

	ret=plNetSendMsg(net,buf,off,sid,0);

	return ret;
}

