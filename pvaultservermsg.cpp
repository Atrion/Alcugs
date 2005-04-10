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
#define __U_PVAULTSERVER_MSG_ID "$Id: pservermsg.cpp,v 1.7 2004/11/28 01:33:03 almlys Exp $"

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

#include "vserversys.h"

#include "vault_obj.h"
#include "vault_tasks.h"
#include "vault_advp.h"
#include "htmldumper.h"

#include "pdefaultmsg.h"

#include "pvaultservermsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_vaultserver_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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

	u->tpots=2; //unset tpots on all incoming messages

	int n=0,off=0;//,dsid=-1;

	switch(u->hmsg.cmd) {
		case NetMsgVault:
			print2log(f_uru,"<RCV> NetMsgVault for %i\n",u->hmsg.ki);
			t_vault_mos vault_s;
			plMainVaultInit(&vault_s);
			plVaultUnpack(buf+off,&vault_s,size-off,u,1); //0x01 vault packet
			u->ki=u->hmsg.ki;
			htmlVaultParse(net,&vault_s,sid,1);
			plAdvVaultParser(net,&vault_s,sid); //generate answers
			//n=plNetMsgVault(sock,&vault_s,u,plNetVersion);
			plMainVaultDestroy(&vault_s,0);
			u->ki=0;
			n=1;
			break;
		case NetMsgVaultTask:
			print2log(f_uru,"<RCV> NetMsgVaultTask for %i\n",u->hmsg.ki);
			t_vault_mos vault_task;
			plMainVaultInit(&vault_task);
			plVaultUnpack(buf+off,&vault_task,size-off,u,2); //vtask packet
			u->ki=u->hmsg.ki;
			u->x=u->hmsg.x;
			htmlVaultParse(net,&vault_task,sid,1);
			plAdvVaultTaskParser(net,&vault_task,sid); //generate answers
			DBG(5,"Attempting to destroy the vault task\n");
			plMainVaultDestroy(&vault_task,0);
			DBG(5,"Vault task succesfully destroyed\n");
			u->ki=0;
			u->x=0;
			n=1;
			break;
		default:
			n=UNET_OK;
			break;
	}
	return n;
}
