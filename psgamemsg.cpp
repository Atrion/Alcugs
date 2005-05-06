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

//server side message parser

#ifndef __U_GAMESERVER_MSG_
#define __U_GAMESERVER_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_GAMESERVER_MSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "config_parser.h"
#include "stdebug.h"
#include "conv_funs.h"
#include "protocol.h"
#include "urunet.h"

#include "vaultstrs.h"
#include "vault_obj.h"

#include "adv_gamemsgparser.h"
#include "urumsg.h"
#include "uru.h"

#include "msg_parser.h" //parse advanced messages

#include "psgamemsg.h"

/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_sgame_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		if(size>0) {
			stamp2log(f_uru);
			ip2log(f_uru,u);
			print2log(f_uru,"SGAME - Recieved a packet of %i bytes [%s,%s]...\n",size,u->login,u->avatar_name);
			dump_packet(f_uru,buf,size,0,7);
			print2log(f_uru,"\n-------------\n");
		}
		if(size>1024*256) {
			print2log(f_err,"Attention Packet is bigger than 256KBytes, that Is impossible!\n");
			DBG(5,"Abort Condition");
			abort();
			return -1;
		}
	}

	int offset=0;
	int n;
	int i;
	int aux1;

	int rcode;

	//Put here broken messages that the client sends, when it has logged out :/

	switch(u->adv_msg.cmd) {
		case NetMsgPlayerPage:
			//Put here the NetMsgPlayerPage
			n=0;
			break;
		default:
			n=-7;
			break;
	}

	//THE REALLY BIG VERY VERY BIG Switch <--- AT THIS POINT, YOU MUST BE LOGGED -->

	if(n==-7 && (u->access_level>=AcNotActivated || (u->authenticated!=1))) { // && (u->adv_msg.cmd!=NetMsgVault && u->adv_msg.cmd!=NetMsgVault2)))) {
		//go out (send go out)
		//if(u->adv_msg.cmd!=NetMsgPlayerPage) {
		///	plNetMsgTerminated(sock,RNotAuthenticated,u);
		//}

		//put here broken messages to don't send an auth failed
		switch(u->adv_msg.cmd) {
			case NetMsgPlayerPage:
			case NetMsgGameMessage:
			//case NetMsgGameMessageDirected:
			case NetMsgLoadClone:
				n=0;
				break;
			default:
				n=-15;
				break;
		}
		//n=-15;
		return n;
	} else {
		if(n!=-7) { return n; }
	}

	switch (u->adv_msg.cmd) {
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
			plNetMsgJoinAck(sock,u); //<-- working on this one (we need to send the age state)

			if(track!=NULL) {
				track->adv_msg.ki=u->adv_msg.ki;
				n=plNetMsgCustomPlayerStatus(sock,u->guid,u->login,u->avatar_name,2,RInGame,track);
				u->status=RInGame;
			}
			n=0;
			u->joined=1;
			u->paged=1; //needs to be moved...
			break;
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

			format=*(U32 *)(buf+offset);
			offset+=4;
			print2log(f_uru,"format %08X ",format);
			if(format & 0x01) { //with the page thingie
				page_id=*(U32 *)(buf+offset);
				offset+=4;
				print2log(f_uru,"page id %04X ",page_id);
				page_type=*(U16 *)(buf+offset);
				offset+=2;
				print2log(f_uru,"page type %02X ",page_type);
#if 0
				if(page_type!=0x00) {
					ip2log(f_une,u);
					plog(f_une,"UNE: Unexpected page type in plNetMsgPagingRoom %04X\n",page_type);
					rcode=1;
				}
#endif
				offset+=decode_urustring(district,buf+offset,190);
				offset+=2;
				print2log(f_uru,"page %s ",district);
			} else if(format!=0x00) { //minimized
				rcode=1;
			}
			page_flags=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru,"page flags %02X\n",page_flags);
			if(page_flags!=0x01 && page_flags!=0x00) {
				rcode=1;
			}

			if(rcode==1) {
				//---store
				char proom[100];
				static Byte proom_rot=0;
				mkdir("dumps",00750);
				sprintf(proom,"dumps/pagingroom%02X.raw",proom_rot++);
				savefile((char *)buf,size,proom);
				//---end store
				print2log(f_uru,"Unknonw Message recieved, ending session\n");
				plNetMsgTerminated(sock,RUnknown,u);
				n=0;
				break;
			}

			if(page_flags==0x00) { //page in that page
				plNetMsgGroupOwner(sock,page_id,page_type,1,u);
			} else if(page_flags==0x01) { //page out that page
				//plNetMsgGroupOwner(sock,page_id,page_type,0,u);
				//Seems that is not necessary to send this message, it only causes *problems*
			}
			n=0;
			break;
#if 1 //Enabled again
		case NetMsgLoadClone: //0x03AE
			print2log(f_uru,"<RCV> NetMsgLoadClone ");
			/*if(!check_plnetmsg(u->msg,0,0x0410,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			//re-write this stuff to be nicer

#if 1
			st_uru_adv_msg * adv_msg;
			adv_msg=&u->adv_msg;

			rcode=parse_adv_msg(buf,size,&u->adv_msg,u);

			//if(rcode<0) {
				print2log(f_uru,"failed parsing an advanced message\n");
				//---store
				char clone[100];
				static Byte clone_rot=0;
				mkdir("dumps",00750);
				sprintf((char *)clone,"dumps/clone%02X.raw",clone_rot++);
				savefile((char *)buf,size,clone);
				//---end store
				//plNetMsgTerminated(sock,RKickedOff,u);
				//abort();
			//}
			if(rcode<0) { n=0; break; }

			print2log(f_uru," page %i ptype %i name %s p_type %i, page %i ptype %i name %s p_type %i\n",adv_msg->msg.object.page_id,adv_msg->msg.object.page_type,adv_msg->msg.object.name,adv_msg->msg.object.type,adv_msg->object.page_id,adv_msg->object.page_type,adv_msg->object.name,adv_msg->object.type);

			//theorically this needs to be broadcasted to all players
			adv_msg->unk6=0x01;
			rcode=send_response(sock,adv_msg,u);

			if(global_broadcast==1) {
				for(i=global_the_bar+1; i<=global_client_count; i++) {
					if(all_players[i].paged==1 && all_players[i].ki!=u->ki) {
						plNetMsgMemberUpdate(sock,&all_players[i],u,1);
					}
				}
			}

			//now load all players
			if(u->paged==1) {
				//send_load_player_list(sock,u);
			}
			n=0;
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
			n=0;
			//u->paged=1; //the player is now paged in
			break;
#endif
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
				mkdir("dumps",00750);
				sprintf(state,"dumps/state%02X.raw",state_rot++);
				savefile((char *)buf,size,state);
				//---end store
				//plNetMsgTerminated(sock,RKickedOff,u);
				//break;
				//abort();
			}
			plNetMsgSDLStateEspecial(sock,u); //HA!
			send_load_player_list(sock,u); //well, let's see
			//Bah, this sucks, the client needs this hateful redundant message 3-4 times!!
			if(bunk1==0) {
				plNetMsgInitialAgeStateSent(sock,u);
			}
			n=0;
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
			n=0;
			break;
		case NetMsgTestAndSet: //0x0278
			print2log(f_uru,"<RCV> NetMsgTestAndSet ");
			/*if(!check_plnetmsg(u->msg,0,0x0410,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				break;
			}*/
			//Baah, preliminar, very preliminar
			st_UruObjectDesc o;
			bzero(&o,sizeof(st_UruObjectDesc));
			n=storeUruObjectDesc(buf,&o,size);
			dumpUruObjectDesc(f_uru,&o);

			int ret;

			Byte flag1; //0x00
			//U32 unk1; //0x00
			U32 unk2; //0x1D
			Byte trigger[100]; //TrigState
			U32 unk3; //0x01
			Byte state1; //0x00 | 0x01
			Byte triggered[100]; //Triggered
			Byte flag2; //0x02
			Byte state2; //0x01 | 0x00
			Byte state3; //0x01 | 0x00

			ret=0;
			if(n>0) {
				offset+=n;
				//preliminar stuff in a test and set message
				DBG(5,"Step 1\n");
				flag1=*(Byte *)(buf+offset);
				offset++;
				print2log(f_uru," flag1:%02X",flag1);
				DBG(5,"Step 4\n");
				if(flag1!=0x00) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				unk1=*(U32 *)(buf+offset);
				offset+=4;
				print2log(f_uru," unk1:%02X",unk1);
				if(unk1!=0x00) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				DBG(5,"Step 8\n");
				unk2=*(U32 *)(buf+offset);
				offset+=4;
				print2log(f_uru," unk2:%02X",unk2);
				DBG(5,"Step 10\n");
				if(unk2!=0x1D) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				DBG(5,"Step 12\n");
				offset+=decode_urustring(trigger,buf+offset,99);
				offset+=2;
				print2log(f_uru," trigger:%s",trigger);
				DBG(5,"Step 13\n");
				if(strcmp("TrigState",(char *)trigger)) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				DBG(5,"Step 14\n");
				unk3=*(U32 *)(buf+offset);
				offset+=4;
				print2log(f_uru," unk3:%02X",unk3);
				if(unk3!=0x01) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				state1=*(Byte *)(buf+offset);
				offset++;
				print2log(f_uru," state1:%02X",state1);
				if(state1!=0x00 && state1!=0x01) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				DBG(5,"Step 20\n");
				offset+=decode_urustring(triggered,buf+offset,99);
				offset+=2;
				print2log(f_uru," triggered:%s",triggered);
				if(strcmp("Triggered",(char *)triggered)) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				flag2=*(Byte *)(buf+offset);
				offset++;
				print2log(f_uru," flag2:%02X",flag2);
				if(flag2!=0x02) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				state2=*(Byte *)(buf+offset);
				offset++;
				print2log(f_uru," state2:%02X",state2);
				if(state2!=0x00 && state2!=0x01) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				DBG(5,"Step 40\n");
				state3=*(Byte *)(buf+offset);
				offset++;
				print2log(f_uru," state3:%02X",state3);
				if(state3!=0x00 && state3!=0x01) {
					ret=-1;
					print2log(f_uru," (unexpected!)");
				}
				print2log(f_uru,"\n");
			} else {
				print2log(f_uru,"ERROR, parsing an UruObjectDesc!\n");
				ret=-1;
			}
			DBG(5,"Step 45\n");
			if(ret>=0 && state1==0x00) {
				print2log(f_uru,"Sending a GameMessage for the requested state\n");
				Byte r_buf[512];
				int doff=0;
				int size_offset;

				*(U32 *)(r_buf+doff)=0x00;
				doff+=4;
				*(Byte *)(r_buf+doff)=0x00;
				doff++;
				//*(U32 *)(r_buf+doff)=0x35; //msg size
				size_offset=doff; //where the msg size needs to be stored
				doff+=4;
				//msg
				*(U16 *)(r_buf+doff)=0x026A; //msg cmd (plServerReplyMsg)
				doff+=2;
				*(Byte *)(r_buf+doff)=0x00;
				doff++;
				*(U32 *)(r_buf+doff)=0x01;
				doff+=4;
				*(Byte *)(r_buf+doff)=0x01;
				doff++;
				//put the uruobject
				doff+=streamUruObjectDesc(r_buf+doff,&o);
				//-->
				*(Byte *)(r_buf+doff)=0x01;
				doff++;
				*(U32 *)(r_buf+doff)=0x00;
				doff+=4;
				*(U32 *)(r_buf+doff)=0x00;
				doff+=4;
				*(U16 *)(r_buf+doff)=0x08;
				doff+=2;
				*(Byte *)(r_buf+doff)=0x00;
				doff++;
				*(U32 *)(r_buf+doff)=0x01;
				doff+=4;
				//end msg
				//store the size
				*(U32 *)(r_buf+size_offset)=((doff)-(size_offset+4));
				*(Byte *)(r_buf+doff)=0x00;
				doff++;

				plNetMsgGameMessage69(sock,r_buf,doff,u);
				n=0;
			}
			DBG(5,"Step 100\n");
			//---store
			char ts[100];
			static Byte ts_rot=0;
			mkdir("dumps",00750);
			sprintf(ts,"dumps/ts%02X.raw",ts_rot++);
			savefile((char *)buf,size,ts);
			n=0;
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
			mkdir("dumps",00750);
			sprintf(gm,"dumps/gm%02X.raw",gm_rot++);
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
			n=0;
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
			mkdir("dumps",00750);
			sprintf(sdlb,"dumps/sdlBCast%02X.raw",sdlb_rot++);
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
			n=0;
			//abort();
			break;
		case NetMsgSetTimeout: //0x0464
			print2log(f_uru,"<RCV> NetMsgSetTimeout ");
			char stime[100];
			static Byte stime_rot=0;
			mkdir("dumps",00750);
			sprintf(stime,"dumps/settime%02X.raw",stime_rot++);
			savefile((char *)buf,size,stime);
			n=0;
			break;
		case NetMsgSDLState: //0x02C8
			print2log(f_uru,"<RCV> NetMsgSDLState ");
			char sdlstate[100];
			static Byte sdlstate_rot=0;
			mkdir("dumps",00750);
			sprintf(sdlstate,"dumps/sdlstate%02X.raw",sdlstate_rot++);
			savefile((char *)buf,size,sdlstate);
			n=0;
			break;
		case NetMsgVoice: //0x0274
			print2log(f_uru,"<RCV> NetMsgVoice ");
			char voice[100];
			static Byte voice_rot=0;
			mkdir("dumps",00750);
			sprintf(voice,"dumps/voice%02X.raw",voice_rot++);
			savefile((char *)buf,size,voice);
			if(size>1023) {
				plog(f_err,"FATAL, NetMsgVoice too big, this is not normal, something went wrong in the netcore!\n");
				n=0;
				break;
			}
			n=0;
			//do the bcast (it must be filtered!! (in the future))
			if(global_broadcast==1) {
				for(i=global_the_bar+1; i<=global_client_count; i++) {
					if(all_players[i].client_ip!=0 && all_players[i].paged==1 && all_players[i].ki!=u->ki) {
						plNetMsgVoice(sock,buf,size,&all_players[i],u->ki);
					}
				}
			}
			//and now cross the fingers ....
			break;
#endif
		default:
			print2log(f_uru,"<RCV> Uknown Type of packet! %04X\n",u->adv_msg.cmd);
			//---store
			//char unk_mes[100];
			//static Byte unk_mes_rot=0;
			//sprintf(unk_mes,"dumps/unk%04X_%02X.raw",u->adv_msg.cmd,unk_mes_rot++);
			//savefile((char *)buf,size,unk_mes);
			//---end store
			//plNetMsgTerminated(sock,RKickedOff,u);
			//abort();
			n=-7;
			break;
	}

	logflush(f_uru);

	return n;
}
#endif

