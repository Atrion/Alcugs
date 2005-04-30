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

/*
	Basic messages parser
*/

/* CVS tag - DON'T TOUCH*/
#define __U_CAUTH_MSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include "data_types.h"
#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "conv_funs.h"

//
#include "config_parser.h"
#include "uru.h"
//

//#include "gbasicmsg.h"
#include "gauthmsg.h"
#include "gscauthmsg.h"

#include "pcauthmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_cauth_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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

	int n=0,off=0,dsid=-1;
	int rcode=0;

	switch(u->hmsg.cmd) {
		case NetMsgAuthenticateHello://0x02AF: //plNetMsgAuthenticateHello
			if(u->authenticated!=0) {
				//this is impossible
				nlog(f_err,net,sid,"Ignoring AutheHello, since the player is already authed\n");
				n=1;
				break;
			}
			print2log(f_uru,"<RCV> AuthenticateHello v%i,%i ",\
			u->hmsg.max_version,u->hmsg.min_version);
			//now let's go te get the account name
			off+=decode_urustring((Byte *)u->acct,buf+off,200);
			off+=2;
			u->maxPacketSz=*(U16 *)(buf+off);
			off+=2;
			u->release=*(Byte *)(buf+off);
			off++;
			print2log(f_uru," acctName:%s,maxPacketSz:%i,rel:%i,%s\n",u->acct,\
			u->maxPacketSz,u->release,unet_get_release(u->release));
			//rcode=0xFF; //dissallow login (sends a unes. server error)
			rcode=0x01; //allow login (sends the authHello)

			//version checker
			if(u->hmsg.max_version<12) {
				u->hmsg.max_version=12;
				rcode=AProtocolOlder;
			} else
			if(u->hmsg.max_version>12) {
				u->hmsg.max_version=12;
				rcode=AProtocolNewer;
			} else
			if(u->hmsg.min_version>7) {
				u->hmsg.min_version=7;
				rcode=AProtocolNewer;
			} else {
				u->max_version=u->hmsg.max_version;
				u->min_version=u->hmsg.min_version;
				if(u->hmsg.min_version!=6) { u->tpots=2; } //non-tpots client
			}
			//end version checker
			plNetMsgAuthenticateChallenge(net,rcode,sid);
			u->authenticated=10; //set up next auth level
			n=1;
			break;
		case NetMsgAuthenticateResponse: //0x0393: //plNetMsgAuthenticateResponse
			if(u->authenticated!=10) {
				//this is impossible
				nlog(f_err,net,sid,"Ignoring AuthResponse, since the player is already being authed\n");
				n=1;
				break;
			}
			Byte hash[33];
			print2log(f_uru,"<RCV> AuthenticateResponse \n");
			off+=decode_urustring(hash,buf+off,0x10);
			off+=2;
			dsid=plNetServerSearch(net,KAuth); //get Auth server address
			if(dsid!=-1) {
				st_uru_client * auth=&net->s[dsid]; //auth server
				//old protocol compatibility - Will be removed in the future!!
				Byte protocol=0;
				protocol=net->pro_auth;

				u->x=u->hmsg.x;
				if(protocol==1) {
					auth->hmsg.x=(random()%100);
					u->ki=auth->hmsg.x;
					plNetMsgCustomAuthAsk(net,hash,dsid,sid,1);
				} else { //new protocol
					//printf("remove the abort call on pcauthmsg.cpp...\n");
					//fflush(0);
					//abort(); //avoid a SEGFAULT on the auth servers...
					auth->hmsg.x=sid; //set sid (for routing purposes)
					plNetMsgCustomAuthAsk(net,hash,dsid,sid,0);
				}
				u->timeout=30; //set 30 seconds
			} else {
				plog(f_err,"ERR: Auth server is not responding!!\n");
				plNetMsgAccountAuthenticated(net,0xFF,sid);
				u->authenticated=0;
			}
			n=1;
			return n;
			break;
		default:
			n=UNET_OK;
			break;
	}
	return n;
}

