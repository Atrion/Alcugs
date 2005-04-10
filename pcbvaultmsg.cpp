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
#define __U_PCBVAULT_MSG_ID "$Id: pservermsg.cpp,v 1.7 2004/11/28 01:33:03 almlys Exp $"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "data_types.h"
#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "uru.h"
#include "config_parser.h"

#include "gbasicmsg.h"
#include "gcsbvaultmsg.h"

#include "pcbvaultmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_cbvault_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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
	st_uru_client * vault;

	//int proto=0;
	//proto=cnf_getByte(proto,"vault.oldprotocol","global",global_config);
	//if(proto!=0 && proto!=1) proto=1;

	/*if(proto==0) {
		printf("Remove abort call from pcbvaultmsg.cpp...\n");
		fflush(0);
		abort();
	}*/

	switch(u->hmsg.cmd) {
		case NetMsgRequestMyVaultPlayerList:
			print2log(f_uru,"<RCV> RequestMyVaultPlayerList\n");
			dsid=plNetServerSearch(net,KVault); //get vault address
			if(dsid>=0) {
				vault=&net->s[dsid];
				vault->hmsg.x=sid; //for routing
				u->x=u->hmsg.x;
				plNetMsgCustomVaultAskPlayerList(net,dsid,sid,net->pro_vault);
			} else { plog(f_err,"ERR: Vault seems to be down!!!\n"); }
			n=1;
			break;
		case NetMsgCreatePlayer:
			print2log(f_uru,"<RCV> CreatePlayer size %i\n",size);
			dsid=plNetServerSearch(net,KVault); //get vault address
			if(dsid>=0) {
				vault=&net->s[dsid];
				vault->hmsg.x=sid; //for routing
				u->x=u->hmsg.x;
				plNetMsgCustomVaultCreatePlayer(net,buf+off,size-off,dsid,sid,net->pro_vault);
			} else { plog(f_err,"ERR: Vault seems to be down!!!\n"); }
			n=1;
			break;
		case NetMsgDeletePlayer: //0x0384
			print2log(f_uru,"<RCV> DeletePlayer ");
			U16 aux1;
			aux1=*(U16 *)(buf+off);
			print2log(f_uru,"id %i unk1=%04X\n",u->hmsg.ki,aux1);
			if(aux1!=0) {
				print2log(f_une,"Recieved an unexpected NetMsgDeletePlayer, aux1 is %04X\n",aux1);
				//plNetMsgTerminated(net,RUnknown,sid);
				n=-3;
				break;
			}
			dsid=plNetServerSearch(net,KVault); //get vault address
			if(dsid>=0) {
				vault=&net->s[dsid];
				vault->hmsg.ki=u->hmsg.ki; //Player that will be deleted
				plNetMsgCustomVaultDeletePlayer(net,dsid,sid,net->pro_vault);
			} else { plog(f_err,"ERR: Vault seems to be down!!!\n"); }
			n=1;
			break;
		default:
			n=UNET_OK;
			break;
	}

	return n;
}

