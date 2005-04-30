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
#define __U_PBASIC_MSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include "data_types.h"
#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "gbasicmsg.h"

#include "pbasicmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_basic_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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

	switch(u->hmsg.cmd) {
		case NetMsgPing: //0x025D
			double mtime;
			Byte destination;
			print2log(net->log,"<RCV> Ping ");
			mtime=*(double *)(buf+off); //8 bytes (double)
			off+=0x08;
			destination=*(Byte *)(buf+off);
			off++;
			print2log(net->log," t:%e dst:%i\n",mtime,destination);
			if(net->whoami==destination || destination==KBcast) {
				//Send a mirror copy of the recieved ping msg
				plNetMsgPing(net,1,mtime,destination,sid,sid);
				n=1;
			} else {
				if(net->whoami==KLobby || net->whoami==KGame) {
					if(u->whoami==KVault || u->whoami==KTracking || u->whoami==KAuth) {
						if(u->hmsg.flags & plNetIP) {
							dsid=plNetClientSearchByIp(net,u->hmsg.ip,u->hmsg.port);
							if(dsid>=0) {
								u->hmsg.flags&= ~plNetIP;
								copy_plNetMsg_header(net,dsid,sid,u->hmsg.flags);
								plNetMsgPing(net,1,mtime,destination,dsid,dsid);
							}
						}
					} else {
						//route packet to the specific destination if exists
						switch(destination) {
							case KVault:
								dsid=net->vault;
								break;
							case KAuth:
								dsid=net->auth;
								break;
							case KTracking:
								dsid=net->tracking;
								break;
							default:
								dsid=-1;
								break;
						}
						if(dsid>=0) {
							plNetMsgPing(net,2,mtime,destination,dsid,sid);
						}
					}
				}
				//nothing
				n=1;
			}
			break;
		case NetMsgAlive:
			nlog(net->log,net,sid,"<RCV> Alive for %i\n",u->hmsg.ki);
			n=1; //only ack is required
			break;
		default:
			n=UNET_OK;
			break;
	}

	return n;
}
#if 0
#if 0
		case NetMsgJoinReq:
			//OBSERVATION:
			// After the NetMsgJoinAck the client starts to send GameMessages, TestAndSet, and others
			print2log(f_uru,"<RCV> NetMsgJoinReq ");
			//  normal                                                  p2p?
			//if(!check_plnetmsg(u->msg,0x10,0x0612,0) && !check_plnetmsg(u->msg,0x18,0x0612,0)) {
			//parse
			U32 unk1;
			U16 unkc;
			unk1=*(U32 *)(buf+offset); //ip
			offset+=4;
			print2log(f_uru,",ip:%08X(%s)",unk1,get_ip(unk1));
			unkc=*(U16 *)(buf+offset); //port
			offset+=2;
			print2log(f_uru,",port:%i\n",unkc);
			plNetMsgJoinAck(sock,u);
			n=0;
			u->joined=1;
			u->paged=1;
			break;
#endif
#if 0 //Enabled again
		case NetMsgPagingRoom: //0x0218
			print2log(f_uru,"<RCV> NetMsgPagingRoom ");
			/*if(!check_plnetmsg(u->msg,0,0x0610,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			Byte district[200];
			Byte page_flags;
			U32 format;
			U32 page_id;
			U16 page_type;

			rcode=0;
			offset=0;
			//u->msg.ki=*(U32 *)(buf+offset);
			//offset+=4;
			format=*(U32 *)(buf+offset);
			offset+=4;
			if(format==0x01) { //extended
				page_id=*(U32 *)(buf+offset);
				offset+=4;
				print2log(f_uru,"page id %04X ",page_id);
				page_type=*(U16 *)(buf+offset);
				offset+=2;
				print2log(f_uru,"page type %02X ",page_type);
				if(page_type!=0x00) {
					ip2log(f_une,u);
					plog(f_une,"UNE: Unexpected page type in plNetMsgPagingRoom %04X\n",page_type);
					rcode=1;
				}
				offset+=decode_urustring(district,buf+offset,190);
				offset+=2;
				print2log(f_uru,"page %s ",district);
			} else if(format!=0x00) { //minimized
				rcode=1;
			}
			page_flags=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru,"page flags %02X\n",page_flags);

			//if(rcode==1) {
				//---store
				char proom[100];
				static Byte proom_rot=0;
				sprintf(proom,"dumps/pagingroom%02X.raw",proom_rot++);
				savefile((char *)buf,size,proom);
				//---end store
				//plNetMsgTerminated(sock,RKickedOff,u);
			//}

			if(page_flags==0x00) { //page in that page
				plNetMsgGroupOwner(sock,page_id,page_type,1,u);
			} else if(page_flags==0x01) { //page out that page
				plNetMsgGroupOwner(sock,page_id,page_type,0,u);
			}

			break;
		case NetMsgLoadClone: //0x03AE
			print2log(f_uru,"<RCV> NetMsgLoadClone ");
			/*if(!check_plnetmsg(u->msg,0,0x0410,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			st_uru_adv_msg * adv_msg;
			adv_msg=&u->adv_msg;

			rcode=parse_adv_msg(buf,n,&u->adv_msg,u);

			if(rcode<0) {
				print2log(f_uru,"failed parsing an advanced message\n");
				//---store
				char clone[100];
				static Byte clone_rot=0;
				sprintf((char *)clone,"dumps/clone%02X.raw",clone_rot++);
				savefile((char *)buf,n,clone);
				//---end store
				//plNetMsgTerminated(sock,RKickedOff,u);
				//abort();
			}

			print2log(f_uru," page %i ptype %i name %s p_type %i, page %i ptype %i name %s p_type %i\n",adv_msg->msg.object.page_id,adv_msg->msg.object.page_type,adv_msg->msg.object.name,adv_msg->msg.object.type,adv_msg->object.page_id,adv_msg->object.page_type,adv_msg->object.name,adv_msg->object.type);

			//theorically this needs to be broadcasted to all players
			adv_msg->unk6=0x01;
			rcode=send_response(sock,adv_msg,u);

			//now load all players
			if(u->paged==1) {
				send_load_player_list(sock,u);
			}

			//u->paged=1; //??

			break;
		case NetMsgPlayerPage: //0x03AF
			print2log(f_uru,"<RCV> NetMsgPlayerPage ");
/*			if(!check_plnetmsg(u->msg,0,0x0010,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			rcode=0;
			offset=0;
			//u->msg.ki=*(U32 *)(buf+offset);
			//offset+=4;
			Byte byte_val_flag;

			byte_val_flag=*(Byte *)(buf+offset);
			offset++;
			if(byte_val_flag!=0x01 && byte_val_flag!=0x00) {
				print2log(f_une,"UNE: Byte_val_flag in player page %02X\n",byte_val_flag);
				rcode=-1;
			}

			st_uru_object_desc uru_object;
			aux1=parse_uru_object_desc(buf+offset,&uru_object);

			if((S32)aux1<0) {
				print2log(f_une,"UNE: parsing an uru object\n");
				rcode=-1;
			} else {
				offset+=aux1;
			}

			if(rcode<0) {
				//---store
				char page[100];
				static Byte page_rot=0;
				sprintf(page,"dumps/page%02X.raw",page_rot++);
				savefile((char *)buf,size,page);
				//---end store
				//plNetMsgTerminated(sock,RKickedOff,u);
				//break;
				//abort();
			}
			print2log(f_uru," [%i] %s flag %02X\n",u->adv_msg.ki,uru_object.name,byte_val_flag);
			//u->paged=1; //the player is now paged in
			break;
		case NetMsgGameStateRequest: //0x0260
			print2log(f_uru,"<RCV> NetMsgGameStateRequest ");
			/*if(!check_plnetmsg(u->msg,0,0x0618,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			offset=0;
			//u->msg.ki=*(U32 *)(buf+offset);
			//offset+=4;
			U32 bunk1;
			bunk1=*(U32 *)(buf+offset);
			offset+=4;
			if(bunk1!=0) {
				//---store
				char state[100];
				static Byte state_rot=0;
				sprintf(state,"dumps/state%02X.raw",state_rot++);
				savefile((char *)buf,size,state);
				//---end store
				//plNetMsgTerminated(sock,RKickedOff,u);
				//break;
				//abort();
			}
			plNetMsgInitialAgeStateSent(sock,u);
			break;
		case NetMsgMembersListReq: //0x02A8
			print2log(f_uru,"<RCV> NetMsgMembersListReq ");
			/*if(!check_plnetmsg(u->msg,0,0x0610,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			offset=0;
			//u->msg.ki=*(U32 *)(buf+offset);
			print2log(f_uru," for %i\n",u->adv_msg.ki);
			plNetMsgMembersList(sock,u);
			break;
		case NetMsgTestAndSet: //0x0278
			print2log(f_uru,"<RCV> NetMsgTestAndSet ");
			/*if(!check_plnetmsg(u->msg,0,0x0410,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			//---store
			char ts[100];
			static Byte ts_rot=0;
			sprintf(ts,"dumps/ts%02i.raw",ts_rot++);
			savefile((char *)buf,size,ts);
			//---end store
			//plNetMsgTerminated(sock,RKickedOff,u);
			//abort();
			break;
		case NetMsgGameMessage: //0x0266
			print2log(f_uru,"<RCV> NetMsgGameMessage ");
			/*if(!check_plnetmsg(u->msg,0,0x0030,0) && !check_plnetmsg(u->msg,0,0x0410,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			//---store
			char gm[100];
			static Byte gm_rot=0;
			sprintf(gm,"dumps/gm%02i.raw",gm_rot++);
			savefile((char *)buf,size,gm);
			//---end store
			//plNetMsgTerminated(sock,RKickedOff,u);
			int i;
			if(global_broadcast==1) {
				for(i=global_the_bar+1; i<=global_client_count; i++) {
					if(all_players[i].paged==1 && all_players[i].ki!=u->ki) {
						plNetMsgGameMessage(sock,buf,size,&all_players[i],u);
					}
				}
			}
			//abort();
			break;
		case NetMsgSDLStateBCast: //0x0324
			print2log(f_uru,"<RCV> NetMsgSDLStateBCast ");
			/*if(!check_plnetmsg(u->msg,0,0x0410,0) && !check_plnetmsg(u->msg,0,0x0414,0)&& !check_plnetmsg(u->msg,0,0x0434,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			//---store
			char sdlb[100];
			static Byte sdlb_rot=0;
			sprintf(sdlb,"dumps/sdlBCast%02i.raw",sdlb_rot++);
			savefile((char *)buf,size,sdlb);
			//---end store
			//plNetMsgTerminated(sock,RKickedOff,u);
			//broadcast it
			//int i;
			if(global_broadcast==1) {
				for(i=global_the_bar+1; i<=global_client_count; i++) {
					if(all_players[i].paged==1 && all_players[i].ki!=u->ki) {
						plNetMsgSDLStateBCast(sock,buf,size,&all_players[i],u);
					}
				}
			}
			//abort();
			break;
#endif
		default:
#if 0
			print2log(f_uru,"<RCV> Uknown Type of packet! %04X\n",u->adv_msg.cmd);
			//---store
			char unk_mes[100];
			static Byte unk_mes_rot=0;
			sprintf(unk_mes,"dumps/unk%04X_%02X.raw",u->adv_msg.cmd,unk_mes_rot++);
			savefile((char *)buf,size,unk_mes);
			//---end store
			//plNetMsgTerminated(sock,RKickedOff,u);
			//abort();
#endif
			n=-7;
			break;
	}

	logflush(f_uru);

	return n;
}
#endif

