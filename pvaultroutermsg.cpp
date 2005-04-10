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
#define __U_PVAULTROUTER_MSG_ID "$Id: pservermsg.cpp,v 1.7 2004/11/28 01:33:03 almlys Exp $"

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

#include "vault_obj.h"
#include "htmldumper.h"

#include "pvaultroutermsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_vaultrouter_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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
	st_uru_client * cli=NULL;

	switch(u->hmsg.cmd) {
		case NetMsgVault: //0x0428
			n=1;
			print2log(f_uru,"<RCV> Vault for %i (%i)\n",u->hmsg.ki,u->ki);
			if(u->hmsg.ki<=0) {
				plog(f_err,"ERR: Invalid vault destination!\n");
				n=-3;
				break;
			}
			//Route the packet to the client
			dsid=plNetClientSearchByKI(net,u->hmsg.ki);
			if(dsid>=0) {
				cli=&net->s[dsid];
#if 0
				if(cli->tpots==1) {
					cli->hmsg.cmd=NetMsgVault2;
				} else {
					cli->hmsg.cmd=NetMsgVault;
				}
				//cli->hmsg.cmd=u->hmsg.cmd;
				cli->hmsg.ki=u->hmsg.ki;
				cli->hmsg.flags=(plNetAck | plNetKi | plNetCustom);
				plNetSendMsg(net,buf+off,size-off,dsid,0);
				if(cli->tpots==0) {
					cli->hmsg.cmd=NetMsgVault2;
					plNetSendMsg(net,buf+off,size-off,dsid,0);
				}
#else
				cli->hmsg.cmd=NetMsgVaultTask;
				u->hmsg.cmd=NetMsgVaultTask;
				t_vault_mos vault_s;
				DBG(6,"plMainVaultInit()\n");
				plMainVaultInit(&vault_s);
				DBG(6,"plVaultUnpack()\n");
				plVaultUnpack(buf+off,&vault_s,size-off,u,0x01);
				DBG(6,"htmlVaultParse()\n");
				cli->hmsg.cmd=NetMsgVault;
				cli->hmsg.ki=u->hmsg.ki;
				htmlVaultParse(net,&vault_s,dsid,0);
				DBG(6,"plNetMsgVault()\n");
				plNetMsgVault(net,&vault_s,dsid);
				DBG(6,"plMainVaultDestroy()\n");
				plMainVaultDestroy(&vault_s,0);
				n=1;
#endif
			} else {
				plog(f_err,"ERR: Cannot route vault packet to client %i, client not found!\n",u->hmsg.ki);
				//abort();
			}
			break;
		case NetMsgVaultTask:
			n=1;
			print2log(f_uru,"<RCV> VaultTask for %i (%i)\n",u->hmsg.ki,u->ki);
			if(u->hmsg.ki<=0) {
				plog(f_err,"ERR: Invalid vault destination!\n");
				n=-3;
				break;
			}
			//Route the packet to the client
			dsid=plNetClientSearchByKI(net,u->hmsg.ki);
			if(dsid>=0) {
				cli=&net->s[dsid];
				cli->hmsg.cmd=NetMsgVaultTask;
				u->hmsg.cmd=NetMsgVaultTask;
				t_vault_mos vault_task;
				DBG(6,"plMainVaultInit()\n");
				plMainVaultInit(&vault_task);
				DBG(6,"plVaultUnpack()\n");
				plVaultUnpack(buf+off,&vault_task,size-off,u,0x02);
				DBG(6,"htmlVaultParse()\n");
				cli->hmsg.cmd=NetMsgVaultTask;
				cli->hmsg.ki=u->hmsg.ki;
				htmlVaultParse(net,&vault_task,dsid,0);
				DBG(6,"plNetMsgVault()\n");
				plNetMsgVaultTask(net,&vault_task,dsid);
				DBG(6,"plMainVaultDestroy()\n");
				plMainVaultDestroy(&vault_task,0);
				n=1;
			} else {
				plog(f_err,"ERR: Cannot route vault packet to client %i, client not found!\n",u->hmsg.ki);
				//abort();
			}
			break;
		default:
			n=UNET_OK;
			break;
	}

	return n;
}
