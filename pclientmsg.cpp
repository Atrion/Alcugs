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

// UruNet class implementation will be here (temporany, it will be a list of all, C functions)

/*
	this will contain all related with using the socket
*/

#ifndef __U_CLIENT_MSG_
#define __U_CLIENT_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_CLIENT_MSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "config_parser.h"
#include "stdebug.h"
#include "conv_funs.h"
#include "protocol.h"
#include "urunet.h"

#include "vaultstrs.h"
#include "urumsg.h"

#include "pclientmsg.h"

/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_client_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		dump_packet(f_uru,buf,size,0,7);
		print2log(f_uru,"\n-------------\n");
	}

	int offset=0;
	int i,n;

	//THE BIG VERY VERY BIG Switch
	switch (u->adv_msg.cmd) {
		case NetMsgTerminated:
			print2log(f_uru,"<RCV> Terminated for %i",u->adv_msg.ki);
			u->adv_msg.reason=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru," Reason: %i %s\n",u->adv_msg.reason,unet_get_reason_code(u->adv_msg.reason));
			n=-1;
			//return -1;
			break;
		case NetMsgPlayerTerminated:
			print2log(f_uru,"<RCV> PlayerTerminated for %i",u->adv_msg.ki);
			u->adv_msg.reason=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru," Reason: %i %s\n",u->adv_msg.reason,unet_get_reason_code(u->adv_msg.reason));
			n=-1;
			//return -1;
			break;
		case NetMsgAuthenticateChallenge:
			print2log(f_uru,"<RCV> AuthenticateChallenge ");
			if(u->authenticated>0) {
				print2log(f_uru,"Unexpected message, when we are authenticated!\n");
				return -6;
			}
			u->adv_msg.reason=*(Byte *)(buf+offset);
			offset++;
			offset+=decode_urustring(u->challenge,buf+offset,0x12);
			offset+=2;
			print2log(f_uru," auth result:%i %s\n",u->adv_msg.reason,unet_get_auth_code(u->adv_msg.reason));
			if((u->major_version>u->adv_msg.max_version) || (u->minor_version>u->adv_msg.min_version)) {
				print2log(f_uru,"ERR: protocol version mismatch, servers are older\n");
				return -2;
			} else if((u->major_version<u->adv_msg.max_version) || (u->minor_version<u->adv_msg.min_version)) {
				print2log(f_uru,"ERR: protocol version mismatch, servers are newer\n");
				return -3;
			}
			if(u->adv_msg.reason==AAuthHello) {
				n=plNetMsgAuthenticateResponse(sock,u);
				//return n;
			} else {
				print2log(f_uru,"ERR: Server returned code %i\n",u->adv_msg.reason);
				n=-4;
			}
			break;
		case NetMsgAccountAuthenticated:
			print2log(f_uru,"<RCV> AccountAuthenticated ");
			if(u->authenticated>0) {
				print2log(f_uru,"Unexpected message, when we are authenticated!\n");
				return -6;
			}
			//player UID
			memcpy(u->guid,buf+offset,16);
			offset+=16;
			//auth result
			u->adv_msg.reason=*(Byte *)(buf+offset);
			offset++;
			//server guid
			memcpy(u->adv_msg.guid,buf+offset,8);
			offset+=8;
			print2log(f_uru,"auth_result: %i %s\n",u->adv_msg.reason,unet_get_auth_code(u->adv_msg.reason));
			if(u->adv_msg.reason==AAuthSucceeded) {
				u->authenticated=1;
				n=0;
			} else {
				n=-5;
			}
			break;
		case NetMsgVaultPlayerList:
			print2log(f_uru,"<RCV> NetMsgVaultPlayerList ");
			DBG(2,"number of players is %i\n",u->p_num);
			u->p_num=*(U16 *)(buf+offset);
			DBG(2,"number of players is %i\n",u->p_num);
			offset+=2;
			u->p_list=(st_vault_player *)(malloc(sizeof(st_vault_player) * u->p_num));
			print2log(f_uru,"%i\n",u->p_num);
			for(i=0; i<u->p_num; i++) {
				u->p_list[i].ki=*(S32 *)(buf+offset);
				offset+=4;
				offset+=decode_urustring(u->p_list[i].avatar,buf+offset,100);
				offset+=2;
				u->p_list[i].flags=*(Byte *)(buf+offset);
				offset++;
				print2log(f_uru,"ki:%i name:%s flags:%02X\n",u->p_list[i].ki,u->p_list[i].avatar,u->p_list[i].flags);
			}
			DBG(2,"number of players is %i\n",u->p_num);
			if(u->minor_version>=7) {
				offset+=decode_urustring(u->url,buf+offset,200);
				offset+=2;
				print2log(f_uru,"url:%s\n",u->url);
			}
			DBG(2,"number of players is %i\n",u->p_num);
			n=offset;
			break;
		default:
			//print2log(f_uru,"<RCV> Uknown Type of packet! %04X\n",u->adv_msg.cmd);
			//---store
			//char unk_mes[100];
			//static Byte unk_mes_rot=0;
			//sprintf(unk_mes,"dumps/unk%02i.raw",unk_mes_rot);
			//savefile((char *)buf,n,unk_mes);
			//---end store
			n=-7;
			break;
	}
	logflush(f_uru);

	return n;
}
#endif

