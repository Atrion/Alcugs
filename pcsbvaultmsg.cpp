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
#define __U_PCSBVAULT_MSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifndef __MSVC__
#  include <unistd.h>
#else
#  include <direct.h> //mkdir
#endif

#include "data_types.h"
#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "conv_funs.h"
#include "files.h"

//#include "gbasicmsg.h"

#include "vserversys.h"
#include "vault_db.h"

#include "vault_tasks.h"

#include "gsbvaultmsg.h"

#include "pcsbvaultmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_csbvault_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
	if(net_check_address(net,sid)!=0) return UNET_OUTOFRANGE;
	st_uru_client * u=&net->s[sid];

	if(vault_server_initialitzed!=1) {
		if(!init_vault_server_subsys()) { return -3; }
	}

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

	int n=0,off=0,rcode=0; //,dsid=-1;

	Byte * data=NULL;
	int data_size=0;
	Byte proto=0;

	switch(u->hmsg.cmd) {
		case NetMsgCustomVaultAskPlayerList:
			print2log(f_uru,"<RCV> NetMsgCustomVaultAskPlayerList\n");
			//player guid
			if(!(u->hmsg.flags & plNetGUI)) { //old protocol
				proto=1;
				memcpy(u->hmsg.guid,buf+off,16);
				off+=16;
			}
			DBG(5,"Quering db for player list\n");
			rcode=plVaultQueryPlayerList(u->hmsg.guid,&data,&data_size,&db);
			DBG(5,"Db queried with rcode=%i\n",rcode);
			plNetMsgCustomVaultPlayerList(net,data,data_size,sid,proto);
			if(data!=NULL) { free((void *)data); data=NULL; }
			n=1;
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

			off+=decode_urustring((Byte *)u->acct,buf+off,100);
			off+=2;
			//player guid
			if(!(u->hmsg.flags & plNetGUI)) { //old protocol
				proto=1;
				memcpy(u->hmsg.guid,buf+off,16);
				off+=16;
			}
			u->access_level=*(Byte *)(buf+off);
			off++;

			//Create Player Message contents
			off+=decode_urustring(avie,buf+off,50);
			off+=2;
			off+=decode_urustring(gender,buf+off,50);
			off+=2;
			off+=decode_urustring(friend_name,buf+off,50);
			off+=2;
			off+=decode_urustring(passkey,buf+off,50);
			off+=2;

			aux1=*(U16 *)(buf+off); //vault !??!?!
			off+=2;

			print2log(f_uru," Avie: %s, %s Friend's: %s, Pass: %s, vault_c: %i\n",avie,gender,friend_name,passkey,aux1);

			rcode=AUnspecifiedServerError; //by default

			print2log(f_uru,"access level=%i\n",u->access_level);
			int num;

			num=0;
			if(u->access_level>AcCCR) {
				num=plVaultGetNumberOfPlayers(u->hmsg.guid,&db); //warning num can be -1 or <0
			}
			u->hmsg.ki=0;

			if(num<0) {
				rcode=AUnspecifiedServerError;
			} else if(num>=max_players_per_account) {
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
				static unsigned char upload_count=0;
				str_filter((Byte *)avie);
				static char dumps_path[300];
				static char dumps_path2[300];
				sprintf(dumps_path,"%s/dumps",stdebug_config->path);
				mkdir(dumps_path,00750);
				sprintf(dumps_path2,"%s/uploads",dumps_path);
				mkdir(dumps_path2,00750);
				sprintf(dumps_path,"%s/%s%03i.vault",dumps_path2,avie,upload_count++);
				savefile((char *)buf,size,dumps_path);
			} else {
				if(strcmp((char *)gender,"Male") && strcmp((char *)gender,"Female") && u->access_level>AcCCR) {
					if(!strcmp((char *)gender,"Yeesha") || !strcmp((char *)gender,"YeeshaNoGlow") || !strcmp((char *)gender,"Shuterland")) {
						strcpy((char *)gender,"Female");
					} else {
						strcpy((char *)gender,"Male");
					}
				}
				ki=plVaultCreatePlayer(net,(Byte *)u->acct,u->hmsg.guid,avie,gender,u->access_level);
				if(ki<0) rcode=AUnspecifiedServerError;
				else if(ki==0) rcode=ANameIsAlreadyInUse;
				else {
					rcode=0; //ok
					u->hmsg.ki=ki;
				}
			}
			plNetMsgCustomVaultPlayerCreated(net,rcode,sid,proto);
			n=1;
			break;
		case NetMsgCustomVaultDeletePlayer:
			print2log(f_uru,"<RCV> NetMsgCustomVaultDeletePlayer %i\n",u->hmsg.ki);
			if(!(u->hmsg.flags & plNetGUI)) { //old protocol
				proto=1;
				memcpy(u->hmsg.guid,buf+off,16);
				off+=16;
			}
			u->access_level=*(Byte *)(buf+off);
			off++;
			plVaultDeletePlayer(net,u->hmsg.guid,u->hmsg.ki,u->access_level);
			n=1;
			break;
		case NetMsgCustomVaultCheckKi:
			if(!(u->hmsg.flags & plNetGUI)) { //old protocol
				proto=1;
				memcpy(u->hmsg.guid,buf+off,16);
				off+=16;
			}
			rcode=plVaultCheckKi((Byte *)u->hmsg.guid,u->hmsg.ki,(Byte *)u->name,&db);
			plNetMsgCustomVaultKiChecked(net,rcode,(Byte *)u->name,sid,proto);
			n=1;
			break;
		case NetMsgCustomVaultPlayerStatus:
			print2log(f_uru,"<RCV> NetMsgCustomVaultPlayerStatus for %i\n",u->hmsg.ki);
			Byte age[200];
			Byte guid[20];
			Byte state;
			Byte level;
			U32 online_time;
			off+=decode_urustring(age,buf+off,100);
			off+=2;
			off+=decode_urustring(guid,buf+off,19);
			off+=2;
			state=*(Byte *)(buf+off);
			off++;
			online_time=*(U32 *)(buf+off);
			off+=4;
			//level=*(Byte *)(buf+off);
			//off++;
			level=0;
			plVaultUpdatePlayerStatus(net,u->hmsg.ki,age,guid,state,online_time,sid);
			n=1;
			break;
		default:
			n=UNET_OK;
			break;
	}

	return n;
}

