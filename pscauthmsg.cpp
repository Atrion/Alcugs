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

//server side message parser

/* CVS tag - DON'T TOUCH*/
#define __U_SCAUTH_MSG_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "data_types.h"
#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "conv_funs.h"

#include "auth.h"

#include "gsauthmsg.h"
#include "pscauthmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_scauth_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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

	int n=0,off=0;//,dsid=-1;

	int rcode=0xFF,proto=0;

	switch(u->hmsg.cmd) {
		case NetMsgCustomAuthAsk:
			U32 ip;
			Byte hash[33];
			Byte str_challenge[33];
			Byte str_hash[33];
			Byte guid[40];
			char * str_ip;
			print2log(f_uru,"<RCV> NetMsgCustomAuthAsk ");
			//login
			off+=decode_urustring((Byte *)u->acct,buf+off,100);
			off+=2;
			//challenge
			memcpy(u->challenge,buf+off,16);
			off+=16;
			hex2ascii2(str_challenge,u->challenge,16);
			print2log(f_uru,"challenge: %s ",str_challenge);
			//hash (client answer)
			memcpy(hash,buf+off,16);
			off+=16;
			hex2ascii2(str_hash,hash,16);
			print2log(f_uru,"hash: %s ",str_hash);
			//build
			u->release=*(Byte *)(buf+off);
			off++;
			print2log(f_uru,"build: %i,%s ",u->release,unet_get_release(u->release));

			if(!(u->hmsg.flags & plNetIP)) {
				proto=1; //old protocol
				//ip
				ip=*(U32 *)(buf+off);
				off+=4;
			} else {
				//new protocol
				ip=u->hmsg.ip;
			}
			str_ip=get_ip(ip);
			print2log(f_uru,"ip: %s ",str_ip);
			//WARNING!, THE MODIFICATION OF THIS CODE IN THE WHOLE OR PART WITH THE PURPOSE OF
			//BYPASSING THE CYAN'S GLOBAL AUTH SERVER GOES AGAINST THE URU CLIENT LICENSE
			rcode=authenticate_player((char *)u->acct,(char *)str_challenge,\
			(char *)str_hash,u->release,(char *)str_ip,(char *)u->passwd,\
			(char *)guid,(char *)&(u->access_level));
			memcpy((char *)u->hmsg.guid,(char *)str_guid_to_hex(guid),16);
			nlog(net->sec,net,sid,"Authresult %i %s for %s - level:%i guid:%s ip:%s\n",rcode,\
			unet_get_auth_code(rcode),u->acct,u->access_level,guid,str_ip);
			plNetMsgCustomAuthResponse(net,(Byte *)u->acct,rcode,u->passwd,\
			u->access_level,sid,proto);
			logflush(net->sec);
			n=1;
			break;
		default:
			n=0;
			break;
	}

	return n;
}
