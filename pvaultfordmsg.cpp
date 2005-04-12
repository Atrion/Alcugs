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
#define __U_PVAULTFORD_MSG_ID "$Id: pservermsg.cpp,v 1.7 2004/11/28 01:33:03 almlys Exp $"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "data_types.h"

#include "vaultstrs.h"

#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "gbasicmsg.h"

#include "gvaultmsg.h"

//#include "pvaultroutermsg.h"

#include "vault_obj.h"
#include "htmldumper.h"

#include "pdefaultmsg.h"

#include "pvaultfordmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_vaultford_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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
	st_uru_client * vault=NULL;

	switch(u->hmsg.cmd) {
		case NetMsgVault2: //tpots mod
		case NetMsgVault: //0x0428
			n=1;
			if(u->hmsg.cmd==NetMsgVault2) {
				u->tpots=1;
			} else {
				u->tpots=2;
			}
			print2log(f_uru,"<RCV> Vault for %i (%i)\n",u->hmsg.ki,u->ki);
			if(u->ki<=0) {
				//plNetMsgTerminated(net,RKickedOff,sid);
				n=-1;
				break;
			}
			//Route the packet to the vault
			dsid=plNetServerSearch(net,KVault);
			if(dsid>=0) {
				vault=&net->s[dsid];
				vault->hmsg.ki=u->ki;
#if 0
				vault->hmsg.flags=(plNetAck | plNetKi | plNetCustom);

				plNetSendMsg(net,buf+off,size-off,dsid,0);
#else
				vault->hmsg.cmd=NetMsgVault;
				u->hmsg.cmd=NetMsgVault;
				t_vault_mos vault_s;
				DBG(6,"plMainVaultInit()\n");
				plMainVaultInit(&vault_s);
				DBG(6,"plVaultUnpack()\n");
				plVaultUnpack(buf+off,&vault_s,size-off,u,0x01);
				DBG(6,"htmlVaultParse()\n");
				htmlVaultParse(net,&vault_s,sid,1);
				DBG(6,"plNetMsgVault()\n");
				n=plNetMsgVault(net,&vault_s,dsid);
				DBG(6,"plMainVaultDestroy()\n");
				plMainVaultDestroy(&vault_s,0);
				n=1;
#endif
			} else { plog(f_err,"ERR: Vault seems to be down!\n"); }
			break;
		case NetMsgVaultTask: {
			n=1;
			print2log(f_uru,"<RCV> NetMsgVaultTask for %i (%i) \n",u->hmsg.ki,u->ki);
			//--- store
			static Byte vtask_rot=0;
			dump_msg("vtask",u->hmsg.cmd,vtask_rot++,buf,size);
			//--- end store
			if(u->ki<=0) {
				n=-3;
				break;
			}
			dsid=plNetServerSearch(net,KVault);
			if(dsid>=0) {
				vault=&net->s[dsid];
				vault->hmsg.ki=u->ki;
				//Route the packet to the vault
				vault->hmsg.cmd=NetMsgVaultTask;
				u->hmsg.cmd=NetMsgVaultTask;
				t_vault_mos vault_task;
				plMainVaultInit(&vault_task);
				plVaultUnpack(buf+off,&vault_task,size-off,u,0x02);
				htmlVaultParse(net,&vault_task,sid,1);
				plNetMsgVaultTask(net,&vault_task,dsid);
				plMainVaultDestroy(&vault_task,0);
				n=1;
			}  else { plog(f_err,"ERR: Vault seems to be down!\n"); }
		} break;
		default:
			n=UNET_OK;
			break;
	}

	return n;
}
