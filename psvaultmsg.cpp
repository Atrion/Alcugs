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

//server side message parser

#ifndef __U_PSVAULT_MSG_
#define __U_PSVAULT_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_PSVAULT_MSG_ID "$Id$"

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

#include "htmldumper.h"
#include "psvaultmsg.h"
#include "vault_db.h"
#include "vault_obj.h"
#include "vault_tasks.h"

#include "vault_advp.h"

#include "psvaultmsg.h"

/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_svault_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		dump_packet(f_uru,buf,size,0,7);
		print2log(f_uru,"\n-------------\n");
	}

	int offset=0;
	int n;
	//int i;

	int rcode;

	//THE BIG VERY VERY BIG Switch
	switch (u->adv_msg.cmd) {
		case NetMsgCustomVaultAskPlayerList:
			print2log(f_uru,"<RCV> NetMsgCustomVaultAskPlayerList\n");
			//player guid
			memcpy(u->guid,buf+offset,16);
			offset+=16;
			DBG(5,"Quering db for player list\n");
			rcode=plVaultQueryPlayerList(u->guid,&u->p_list);
			DBG(5,"Db queried with rcode=%i\n",rcode);
			if(rcode<0) {
				u->p_num=0;
			} else {
				u->p_num=rcode;
			}
			n=plNetMsgCustomVaultPlayerList(sock,u);
			if(rcode>0) {
				DBG(5,"I'm before a free");
				free((void *)u->p_list);
				u->p_list=NULL;
				DBG(5,"I'm after a free");
			}
			break;
		case NetMsgCustomVaultCreatePlayer:
			print2log(f_uru,"<RCV> NetMsgCustomVaultCreatePlayer ");
			Byte avie[100];
			Byte gender[100];
			Byte friend_name[100];
			Byte passkey[100];
			U16 aux1;
			int rcode;
			int ki;

			offset+=decode_urustring(u->login,buf+offset,100);
			offset+=2;

			memcpy(u->guid,buf+offset,16);
			offset+=16;

			u->access_level=*(Byte *)(buf+offset);
			offset++;

			offset+=decode_urustring(avie,buf+offset,50);
			offset+=2;
			offset+=decode_urustring(gender,buf+offset,50);
			offset+=2;
			offset+=decode_urustring(friend_name,buf+offset,50);
			offset+=2;
			offset+=decode_urustring(passkey,buf+offset,50);
			offset+=2;

			aux1=*(U16 *)(buf+offset); //vault !??!?!
			offset+=2;

			print2log(f_uru," Avie: %s, %s Friend's: %s, Pass: %s, vault_c: %i\n",avie,gender,friend_name,passkey,aux1);

			rcode=AUnspecifiedServerError; //by default

			print2log(f_uru,"access level=%i\n",u->access_level);
			int num;

			num=0;
			if(u->access_level>AcCCR) {
				num=plVaultGetNumberOfPlayers(u->guid);
				if(num<0) {
					u->p_num=0;
				} else {
					u->p_num=num;
				}
			} else {
				u->p_num=0;
			}

			u->adv_msg.ki=0;

			if(num<0) {
				rcode=AUnspecifiedServerError;
			} else if(u->p_num>=global_max_players_per_account) {
				rcode=AMaxNumberPerAccountReached;
			} else if(strlen((const char *)avie)<3) {
				rcode=ANameIsTooShort;
			} else if(strlen((const char *)avie)>30) {
				rcode=ANameIsTooLong;
			} else if(strlen((const char *)friend_name)>0) {
				rcode=AInvitationNotFound;
			} else if(aux1!=0) {
				rcode=AUnspecifiedServerError;
				//store the packet for further study
				//store_packet(buf+offset,n-offset,(const char *)avie);
				mkdir("uploads",0750);
				char upload_dir[200];
				static unsigned char upload_count=0;
				str_filter((Byte *)avie);
				sprintf(upload_dir,"uploads/%s%03i.vault",avie,upload_count++);
				savefile((char *)buf,size,upload_dir);
			} else {
				if(strcmp((char *)gender,"Male") && strcmp((char *)gender,"Female") && u->access_level>AcCCR) {
					if(!strcmp((char *)gender,"Yeesha") || !strcmp((char *)gender,"YeeshaNoGlow") || !strcmp((char *)gender,"Shuterland")) {
						strcpy((char *)gender,"Female");
					} else {
						strcpy((char *)gender,"Male");
					}
				}
				ki=plVaultCreatePlayer(sock,u->login,u->guid,avie,gender,u->access_level);
				//ki=plVaultCreatePlayer(u->guid,avie,gender);
				if(ki<0) rcode=AUnspecifiedServerError;
				else if(ki==0) rcode=ANameIsAlreadyInUse;
				else {
					rcode=0; //ok
					u->adv_msg.ki=ki;
				}
			}
			plNetMsgCustomVaultPlayerCreated(sock,rcode,u);
			break;
		case NetMsgCustomVaultDeletePlayer:
			print2log(f_uru,"<RCV> NetMsgCustomVaultDeletePlayer %i\n",u->adv_msg.ki);
			memcpy(u->guid,buf+offset,16);
			offset+=16;
			u->access_level=*(Byte *)(buf+offset);
			offset++;
			plVaultDeletePlayer(sock,u->guid,u->adv_msg.ki,u->access_level);
			n=0;
			break;
		case NetMsgCustomVaultCheckKi:
			memcpy(u->guid,buf+offset,16);
			offset+=16;
			rcode=plVaultCheckKi(u->guid,u->adv_msg.ki,u->avatar_name);
			n=plNetMsgCustomVaultKiChecked(sock,u->guid,rcode,u->avatar_name,u);
			break;
		case NetMsgVault:
			print2log(f_uru,"<RCV> NetMsgVault for %i\n",u->adv_msg.ki);
			t_vault_mos vault_s;
			plMainVaultInit(&vault_s);
			plVaultUnpack(buf+offset,&vault_s,size-offset,u,1); //0x01 vault packet
			u->ki=u->adv_msg.ki;
			htmlVaultParse(&vault_s,u,1);
			plAdvVaultParser(sock,&vault_s,u); //generate answers
			//n=plNetMsgVault(sock,&vault_s,u,plNetVersion);
			plMainVaultDestroy(&vault_s,0);
			u->ki=0;
			break;
		case NetMsgVault2:
			print2log(f_uru,"<RCV> NetMsgVault(tpots) for %i (Ignored...)\n",u->adv_msg.ki);
			break;
		case NetMsgVaultTask:
			print2log(f_uru,"<RCV> NetMsgVaultTask for %i\n",u->adv_msg.ki);
			t_vault_mos vault_task;
			plMainVaultInit(&vault_task);
			plVaultUnpack(buf+offset,&vault_task,size-offset,u,2); //vtask packet
			u->ki=u->adv_msg.ki;
			u->x=u->adv_msg.x;
			htmlVaultParse(&vault_task,u,1);
			plAdvVaultTaskParser(sock,&vault_task,u); //generate answers
			DBG(5,"Attempting to destroy the vault task\n");
			plMainVaultDestroy(&vault_task,0);
			DBG(5,"Vault task succesfully destroyed\n");
			u->ki=0;
			u->x=0;
			break;
		case NetMsgCustomVaultPlayerStatus:
			print2log(f_uru,"<RCV> NetMsgCustomVaultPlayerStatus for %i\n",u->adv_msg.ki);
			Byte age[200];
			Byte guid[20];
			Byte state;
			U32 online_time;
			offset+=decode_urustring(age,buf+offset,100);
			offset+=2;
			offset+=decode_urustring(guid,buf+offset,19);
			offset+=2;
			state=*(Byte *)(buf+offset);
			offset++;
			online_time=*(U32 *)(buf+offset);
			offset+=4;
			n=plVaultUpdatePlayerStatus(sock,u->adv_msg.ki,age,guid,state,online_time,u);
			break;
		default:
			n=-7;
			break;
	}

	logflush(f_uru);

	return n;

}
#endif

