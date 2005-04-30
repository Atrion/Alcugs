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
#define __U_PSBVAULT_MSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "data_types.h"
#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "conv_funs.h"

#include "uru.h"

#include "gbasicmsg.h"
#include "gbvaultmsg.h"
#include "gctrackingmsg.h"
#include "globbymsg.h"
#include "gcsbvaultmsg.h"

#include "psbvaultmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_sbvault_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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

	int i,n=0,off=0,dsid=-1,tsid=-1;

	st_uru_client * d=NULL;
	st_uru_client * track=NULL;

	switch(u->hmsg.cmd) {
		case NetMsgCustomVaultPlayerList:
			print2log(f_uru,"<RCV> NetMsgCustomVaultPlayerList \n");
			if(!(u->hmsg.flags & plNetGUI)) { //old protocol
				memcpy(u->hmsg.guid,buf+off,16);
				off+=16;
			}

			if(net_check_address(net,u->hmsg.x)==0 && \
				!memcmp(net->s[u->hmsg.x].uid,u->hmsg.guid,16)) {
				dsid=u->hmsg.x; //set peer
				d=&net->s[u->hmsg.x];
				d->hmsg.x=d->x;
				plNetMsgVaultPlayerList(net,buf+off,size-off,dsid);
			} else {
				plog(f_err,"Cannot find client! on NetMsgCustomVaultPlayerList message!!\n");
			}
			n=1;
			break;
		case NetMsgCustomVaultPlayerCreated:
			print2log(f_uru,"<RCV> NetMsgCustomVaultPlayerCreated \n");
			if(!(u->hmsg.flags & plNetGUI)) { //old protocol
				memcpy(u->hmsg.guid,buf+off,16);
				off+=16;
			}

			Byte status;
			status=*(Byte *)(buf+off);
			off++;

			if(net_check_address(net,u->hmsg.x)==0 && \
				!memcmp(net->s[u->hmsg.x].uid,u->hmsg.guid,16)) {
				dsid=u->hmsg.x; //set peer
				d=&net->s[u->hmsg.x];
				d->hmsg.ki=u->hmsg.ki;
				d->hmsg.x=d->x;
				plNetMsgPlayerCreated(net,status,dsid);
			} else {
				plog(f_err,"Cannot find client! on NetMsgCustomVaultPlayerList message!!\n");
			}
			n=1;
			break;
		case NetMsgCustomVaultKiChecked: //continuation of setmyactiveplayer
			print2log(f_uru,"<RCV> NetMsgCustomVaultKiChecked \n");
			if(!(u->hmsg.flags & plNetGUI)) { //old protocol
				memcpy(u->hmsg.guid,buf+off,16);
				off+=16;
			}
			n=1;
			//search by sid
			if(net_check_address(net,u->hmsg.x)==0 && \
				!memcmp(net->s[u->hmsg.x].uid,u->hmsg.guid,16)) {
				dsid=u->hmsg.x; //set peer
				d=&net->s[u->hmsg.x];
				//d->hmsg.ki=u->hmsg.ki;
				d->hmsg.x=d->x;

				Byte avie[200];
				status=*(Byte *)(buf+off);
				off++;
				off+=decode_urustring(avie,buf+off,100);
				off+=2;
				print2log(f_uru,"result: %i,%s\n",status,avie);
				//anti multi login protection
				for(i=0; i<(int)net->n; i++) {
					//if(net->s[i].flag==0) { break; }
					if(net->s[i].authenticated==1 && net->s[i].ki==(S32)u->hmsg.ki) {
						plog(f_uru,"An age cannot host the same avatar more than two times, disconnecting..\n");
						//plNetMsgTerminated(net,RLoggedInElsewhere,i);
						kill_player(net,RLoggedInElsewhere,i,0);
					} else
					if(0 && strcmp((char *)net->s[i].acct,(char *)d->acct)==0 && i!=dsid) {
						//plNetMsgTerminated(net,RLoggedInElsewhere,i);
						kill_player(net,RLoggedInElsewhere,i,0);
					}
				}
				if(net->whoami==KGame && strcmp((char *)d->name,(char *)avie)) {
					//plNetMsgTerminated(net,RKickedOff,dsid);
					kill_player(net,RKickedOff,dsid,0);
				} else {
					tsid=plNetServerSearch(net,KTracking);
					if(tsid>=0) {
						track=&net->s[tsid];
						track->hmsg.ki=u->hmsg.ki;
						d->ki=(S32)u->hmsg.ki; //now set the ki
						plNetMsgCustomPlayerStatus(net,2,RJoining,tsid,dsid,net->pro_tracking);

						//vault is u, the source of the message. Sent back something to him...
						U32 timer;
						time((time_t *)&timer);
						timer=timer-d->nego_stamp;
						plNetMsgCustomVaultPlayerStatus(net,net->name,net->guid,1,timer,sid,net->pro_vault);
					} else { plog(f_err,"ERR: Seems that tracking is down!!!\n"); }
					d->status=RJoining;
					plNetMsgActivePlayerSet(net,dsid);
				}
				d->bussy=0;
			} else {
				plog(f_err,"Cannot find client! on NetMsgCustomVaultKiChecked message!!\n");
			}
			break;
		default:
			n=UNET_OK;
			break;
	}

	return n;
}

