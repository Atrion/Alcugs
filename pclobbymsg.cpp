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
#define __U_PCLOBBY_MSG_ID "$Id$"

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
#include "config_parser.h"

#include "gbasicmsg.h"
#include "gcsbvaultmsg.h"
#include "gctrackingmsg.h"
#include "globbymsg.h"

#include "urustructs.h"
#include "vaultstrs.h"
#include "vault_obj.h"

#include "pdefaultmsg.h"

#include "pclobbymsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_clobby_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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

	int i,n=0,off=0,dsid=-1;
	st_uru_client * vault=NULL;
	st_uru_client * track=NULL;

	switch(u->hmsg.cmd) {
		case NetMsgSetMyActivePlayer: //0x0374:
			print2log(f_uru,"<RCV> SetActivePlayer ");
			off+=decode_urustring((Byte *)u->name,buf+off,100);
			off+=2;
			Byte rcode;
			rcode=*(Byte *)(buf+off);
			off++;
			print2log(f_uru," %s, unk2=%02X\n",u->name,rcode);
			n=1;
			if(rcode!=0) {
				nlog(f_une,net,sid,"Recieved an unexpected NetMsgSetMyActivePlayer, rcode is %02X\n",rcode);
				//plNetMsgTerminated(net,RUnknown,sid);
				n=-3;
				break;
			}
			//first check the KI and player ownership
			if(u->access_level>AcAdmin) {
				dsid=plNetServerSearch(net,KVault);
				if(dsid>=0) {
					vault=&net->s[dsid];
					vault->hmsg.x=sid; //routing
					vault->hmsg.ki=u->hmsg.ki;
					u->x=u->hmsg.x;
					u->bussy=1; //set bussy (wait to vault answer)
					plNetMsgCustomVaultCheckKi(net,dsid,sid,net->pro_vault);
				} else { plog(f_err,"ERR: Vault seems to be down!!!\n"); }
			} else {
				//anti multi login protection
				for(i=0; i<(int)net->n; i++) {
					//if(net->s[i].flag==0) { break; }
					if(net->s[i].authenticated==1 && net->s[i].ki==(S32)u->hmsg.ki) {
						plog(f_uru,"An age cannot host the same avatar more than two times, disconnecting..\n");
						//plNetMsgTerminated(net,RLoggedInElsewhere,i);
						kill_player(net,RLoggedInElsewhere,i,0);
					}
				}
				if(net->whoami==KGame && strlen((char *)u->name)<=0) {
					//plNetMsgTerminated(net,RKickedOff,sid);
					n=-1;
				} else {
					dsid=plNetServerSearch(net,KTracking);
					if(dsid>=0) {
						track=&net->s[dsid];
						track->hmsg.ki=u->hmsg.ki;
						u->ki=(S32)u->hmsg.ki; //now set the ki
						u->status=RJoining;
						plNetMsgCustomPlayerStatus(net,2,RJoining,dsid,sid,net->pro_tracking);
						plNetMsgActivePlayerSet(net,sid);
						dsid=plNetServerSearch(net,KVault);
						if(dsid>=0) {
							vault=&net->s[dsid];
							vault->hmsg.ki=u->ki;
							U32 timer;
							time((time_t *)&timer);
							timer=timer-u->nego_stamp;
							plNetMsgCustomVaultPlayerStatus(net,net->name,net->guid,1,timer,dsid,net->pro_vault);
						} else { plog(f_err,"ERR: Vault seems to be down!!!\n"); }
					} else { plog(f_err,"ERR: Tracking seems to be down!!!\n"); }
					n=1;
				}
			}
			break;
		case NetMsgFindAge: { //0x037A
			n=1;
			print2log(f_uru,"<RCV> NetMsgFindAge ");
			Byte aguid[19];
			t_AgeLinkStruct * as;
			//--- store
			static Byte fage_mes_rot=0;
			dump_msg("fage",u->hmsg.cmd,fage_mes_rot++,buf,size);
			//--- end store
			//parse
			n=storeAgeLinkStruct((void **)&as,buf+off,size);
			if(n<0) {
				nlog(f_une,net,sid,"Parse error on NetMsgFindAge message!\n");
				n=-3;
				destroyAgeLinkStruct((void **)&as);
				break;
			}
			n=1;
			hex2ascii2(aguid,as->ainfo.guid,8);
			print2log(f_uru," ainfo[Fname:%s,Iname:%s,Guid:%s,Uname:%s,Dname:%s,Lan:%i],\
unk:%i,rules:%i,spw[t:%s,n:%s,c:%s],ccr:%i\n",as->ainfo.filename,\
as->ainfo.instance_name,aguid,as->ainfo.user_name,as->ainfo.display_name,\
as->ainfo.language,as->unk,as->rules,as->spoint.title,as->spoint.name,\
as->spoint.camera,as->ccr);
			//linking rules check
			if(as->unk!=KOriginalBook && as->unk!=KOwnedBook) {
				nlog(f_err,net,sid,"KICKED: Linking rules %i\n",as->unk);
				n=-2;
				break;
			}
			if(as->ccr!=0) {
				nlog(f_une,net,sid,"Kicking of someone that is attempting to link as a CCR?\n");
				//n=plNetMsgTerminated(sock,RKickedOff,u);
				n=-1;
				break;
			} else {
				dsid=plNetServerSearch(net,KTracking);
				if(dsid>=0) {
					track=&net->s[dsid];
					track->hmsg.ki=u->ki;
					track->hmsg.x=u->hmsg.x;
					plNetMsgCustomFindServer(net,(char *)aguid,(char *)as->ainfo.filename,dsid,sid,net->pro_tracking);
					} else { plog(f_err,"ERR: Tracking seems to be down!!!\n"); n=-4; }
				u->status=RInroute; //set player in route
			}
			destroyAgeLinkStruct((void **)&as);
		} break;
		default:
			n=UNET_OK;
			break;
	}

	return n;
}


