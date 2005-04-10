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

/**
	Basic elemental Uru messages
*/

/* CVS tag - DON'T TOUCH*/
#define __U_URUGVAULTMSG_ID "$Id: urumsg.cpp,v 1.11 2004/11/28 01:33:03 almlys Exp $"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "stdebug.h"
#include "protocol.h"
#include "prot.h"
#include "urunet.h"

#include "vaultstrs.h"

#include "gvaultmsg.h"

#include "vault_obj.h"

#include "debug.h"

/**
	--- Vault Messages -- (client<-->game<-->vault)
*/
int plNetMsgVault(st_unet * net,t_vault_mos * v,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int size,off=0,ret=0;

	Byte * buf=NULL; //the out buffer
	Byte * vbuf=NULL;

	if(u->tpots==1) {
		u->hmsg.cmd=NetMsgVault2;
	} else {
		u->hmsg.cmd=NetMsgVault;
	}
	u->hmsg.flags=(plNetAck | plNetKi);

	DBG(5,"Calling to plVaultPack...\n");
	size=plVaultPack(&vbuf,v,u,1); //0x01 vault msg
	DBG(5,"Vault succesfully packed...\n");

	if(size>0) {
		buf=(Byte *)malloc(sizeof(Byte) * (size+1000));
	} else {
		return size;
	}

	print2log(f_uru,"<SND> NetMsgVault\n");

	memcpy(buf+off,vbuf,size);
	off+=size;

#if _DBG_LEVEL_ > 3
	plog(f_uru,"Vault dump\n");
	dumpbuf(f_uru,buf,off);
	lognl(f_uru);
#endif

	ret=plNetSendMsg(net,buf,off,sid,0);

	if(u->tpots==0 && sid!=net->vault) { //tpots patch
		//tpots
		u->hmsg.cmd=NetMsgVault2;

		u->tpots=1;
		free((void *)vbuf);
		off-=size;
		size=plVaultPack(&vbuf,v,u,1); //0x01 vault msg
		memcpy(buf+off,vbuf,size);
		off+=size;
		u->tpots=0;

		print2log(f_uru,"<SND> NetMsgVault2 (tpots) id %i\n",u->hmsg.ki);

		ret=plNetSendMsg(net,buf,off,sid,0);
	}

	free((void *)buf);
	free((void *)vbuf);
	return ret;
}

/**
--- Vault Task Messages -- (client<-->game<-->vault)
*/
int plNetMsgVaultTask(st_unet * net,t_vault_mos * v,int sid) {
	if(net_check_address(net,sid)!=0) { return -1; }
	st_uru_client * u=&net->s[sid];
	int size,off=0,ret=0;

	Byte * buf=NULL; //the out buffer
	Byte * vbuf=NULL;

	u->hmsg.cmd=NetMsgVaultTask;
	u->hmsg.flags=(plNetAck | plNetKi | plNetX);

	DBG(5,"Calling to plVaultPack...\n");
	size=plVaultPack(&vbuf,v,u,2); //0x02 //vtask
	DBG(5,"Vault succesfully packed...\n");

	if(size>0) {
		buf=(Byte *)malloc(sizeof(Byte) * (size+1000));
	} else {
		return size;
	}

	print2log(f_uru,"<SND> NetMsgVaultTask\n");

	memcpy(buf+off,vbuf,size);
	off+=size;

	ret=plNetSendMsg(net,buf,off,sid,0);

	free((void *)buf);
	free((void *)vbuf);
	DBG(5,"Returning from plNetMsgVaultTask RET:%i\n",off);
	return ret;
}
