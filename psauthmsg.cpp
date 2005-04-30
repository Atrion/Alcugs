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

// UruNet class implementation will be here (temporany, it will be a list of all, C functions)

/*
	this will contain all related with using the socket
*/

/* CVS tag - DON'T TOUCH*/
#define __U_PSAUTH_MSG_ID "$Id$"

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

#include "gbasicmsg.h"
#include "gauthmsg.h"

#include "psauthmsg.h"

#include "debug.h"


/**
	processes basic plNetMsg's
*/
int process_sauth_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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

	int n=0,off=0,dsid=-1,i;
	st_uru_client * d;

	Byte proto=0;
	int rcode=AUnspecifiedServerError; //0xFF

	switch(u->hmsg.cmd) {
		case NetMsgCustomAuthResponse:
			print2log(f_uru,"<RCV> NetMsgCustomAuthResponse \n");
			//login
			off+=decode_urustring((Byte *)u->acct,buf+off,100);
			off+=2;

			if(!(u->hmsg.flags & plNetGUI)) { //old protocol version
				proto=1;
				//search by login
				for(i=0; i<(int)net->n; i++) {
					DBG(3,"searching for %s - %s (%i,%i) -%i \n",u->acct,\
					net->s[i].acct,u->hmsg.x,net->s[i].ki,i);
					if(!strcmp((char *)net->s[i].acct,(char *)u->acct) && \
					u->hmsg.x==(U32)net->s[i].ki && net->s[i].whoami==0) {
						dsid=i;
						break;
					}
				}
			} else { //new protocol
				//search by sid, and check ip port
				if(net_check_address(net,u->hmsg.x)==0) {
					dsid=u->hmsg.x;
					if(!(net->s[dsid].ip==u->hmsg.ip && net->s[dsid].port==u->hmsg.port && \
					net->s[dsid].whoami==0)) {
						dsid=-1;
					}
				}
			}

			if(dsid!=-1) {
				d=&net->s[dsid];
				//result
				rcode=*(Byte *)(buf+off);
				off++;
				//passwd
				off+=decode_urustring(d->passwd,buf+off,33);
				off+=2;
				if(proto==1) {
					//GUID
					memcpy(d->uid,buf+off,16);
					off+=16;
				} else {
					memcpy(d->uid,u->hmsg.guid,16);
				}
				d->access_level=*(Byte *)(buf+off);
				off++;
			} else {
				print2log(f_err,"ERR: Player %s not found!\n",u->acct);
				n=1;
				break;
			}

			d->hmsg.x=d->x; //set the x, just in case

			if(rcode==AAuthSucceeded) { //0x00
				memcpy(d->hmsg.guid,d->uid,16);
				print2log(f_uru," Valid passwd auth\n");
				nlog(net->sec,net,dsid,"Succesfull login for %s\n",u->acct);
				plNetMsgAccountAuthenticated(net,0x00,dsid);
				d->authenticated=2;
				d->timeout=2*60; //set 2 minutes
				d->whoami=KClient; //set that the peer is a client
			} else {
				print2log(f_uru," Incorrect passwd auth\n");
				nlog(net->sec,net,dsid,"Auth failed for %s, reason %i,%s\n",u->acct,rcode,\
				unet_get_auth_code(rcode));
				memset(d->uid,0,16);
				plNetMsgAccountAuthenticated(net,rcode,dsid);
				plNetMsgTerminated(net,RNotAuthenticated,dsid);
				plNetEndConnection(net,dsid);
			}
			logflush(f_uru);
			logflush(net->sec);
			n=1;
			break;
		default:
			n=0;
			break;
	}
	logflush(f_uru);

	return n;
}

