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

#ifndef __U_SERVER_MSG_
#define __U_SERVER_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_SERVER_MSG_ID "$Id$"

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

#include "htmldumper.h"

#include "urumsg.h"
#include "uru.h"

#include "pservermsg.h"

/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_server_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		if(size>0) {
			stamp2log(f_uru);
			ip2log(f_uru,u);
			print2log(f_uru,"Recieved a packet of %i bytes [%s,%s]...\n",size,u->login,u->avatar_name);
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

	int rcode;

	//THE BIG VERY VERY BIG Switch
	switch (u->adv_msg.cmd) {
		case NetMsgPing://0x025D //plNetMsgPing
			double mtime;
			Byte destination;
			print2log(f_uru,"<RCV> Ping ");
			mtime=*(double *)(buf+offset); //8 bytes (double)
			offset+=0x08;
			destination=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru," t:%e dst:%i\n",mtime,destination);
			//Send a mirror copy of the recieved ping msg
			n=plNetMsgPing(sock,1,mtime,destination,u);
			break;
		case NetMsgLeave://0x025C: //plNetMsgLeave
			u->adv_msg.reason=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru,"<RCV> Leave for %i reason %i\n",u->adv_msg.ki,u->adv_msg.reason);
			u->status=u->adv_msg.reason;

			if(whoami==KVault || whoami==KAuth || whoami==KTracking) {
				u->ki=0;
			}

			//active player
			if(track!=NULL && u->ki>=500) {
				//send to the tracking server the unload petition
				Byte code;
				if(u->adv_msg.reason==RQuitting) {
					code=0;
				} else {
					code=1;
				}
				track->adv_msg.ki=u->ki; //select the victim
				plNetMsgCustomPlayerStatus(sock,u->guid,u->login,u->avatar_name,\
code,u->adv_msg.reason,track);
			}
			if(vault!=NULL && u->ki>=500 && u->adv_msg.reason==RQuitting) {
				vault->adv_msg.ki=u->ki;
				U32 timer;
				time((time_t *)&timer);
				timer=timer-u->online_time;
				plNetMsgCustomVaultPlayerStatus(sock,(Byte *)"",(Byte *)"0000000000000000",0,timer,vault);
			}

			//destroy the session
			u->client_ip=0; //this player is officially logged out
			u->authenticated=0; //solved bug #843X34jAS_W234 with this line
			u->paged=0; //solved bug #$$A8348_JJ232423324 with this other line ;)
			u->ki=0; //this should also contribute in solving the nasty problem
			strcpy((char *)u->login,"");
			bzero(u->guid,16);
			n=0; //only ack is required (but i need to check the logs!)
			break;
		case NetMsgAuthenticateHello://0x02AF: //plNetMsgAuthenticateHello
			//int rcode;
			if(u->authenticated==1) {
				//this is impossible
				print2log(f_uru,"Ignoring AutheHello, since the player is authed\n");
				n=0;
				break;
			}
			plNetClientComm(sock,u); //re-sync
			print2log(f_uru,"<RCV> AuthenticateHello v%i,%i ",u->adv_msg.max_version,u->adv_msg.min_version);
			//now let's go te get the account name
			offset+=decode_urustring(u->login,buf+offset,200);
			offset+=2;
			u->maxPacketSz=*(U16 *)(buf+offset); //remember the two additional bytes
			offset+=2;
			u->adv_msg.release=*(Byte *)(buf+offset);
			offset++;
			u->release=u->adv_msg.release;
			print2log(f_uru," acctName:%s,maxPacketSz:%i,rel:%i,%s\n",u->login,u->maxPacketSz,u->adv_msg.release,unet_get_release(u->adv_msg.release));
			if(u->access_level>AcNotRes) {
				rcode=0xFF; //dissallow login (sends a unes. server error)
			} else {
				rcode=0x01; //allow login (sends the authHello)
			}
			//version checker
			if(u->adv_msg.max_version<12) {
				u->adv_msg.max_version=12;
				rcode=AProtocolOlder;
			} else
			if(u->adv_msg.max_version>12) {
				u->adv_msg.max_version=12;
				rcode=AProtocolNewer;
			} else
			if(u->adv_msg.min_version>7) {
				u->adv_msg.max_version=7;
				rcode=AProtocolNewer;
			} else {
				u->major_version=u->adv_msg.max_version;
				u->minor_version=u->adv_msg.min_version;
			}
			//end version checker
			n=plNetMsgAuthenticateChallenge(sock,rcode,u);
			if(rcode==0x0FF) {
				//send terminated
				plNetMsgTerminated(sock,RKickedOff,u);
				n=-1;
			}
			break;
		case NetMsgAlive:
			print2log(f_uru,"<RCV> Alive for %i\n",u->adv_msg.ki);
			n=0; //only ack is required
			break;
		default:
			n=-7;
			break;
	}

	//THE BIG VERY VERY BIG 2nd Switch <--- AT THIS POINT, YOU MUST BE LOGGED -->

	if(n==-7 && u->access_level>AcNotRes) {
		//go out (send go out)
		//if(u->adv_msg.cmd!=NetMsgPlayerPage) {
			//plNetMsgTerminated(sock,RNotAuthenticated,u);
		//}
		n=-15;
		return n;
	} else {
		if(n!=-7) {
			return n;
		}
	}

	switch (u->adv_msg.cmd) {
		case NetMsgAuthenticateResponse: //0x0393: //plNetMsgAuthenticateResponse
			Byte hash[33];
			print2log(f_uru,"<RCV> AuthenticateResponse \n");
			offset+=decode_urustring(hash,buf+offset,0x10);
			offset+=2;
			//ask the auth server
			if(auth!=NULL) {
				auth->adv_msg.x=(random()%100);
				u->ki=auth->adv_msg.x;
				plNetMsgCustomAuthAsk(sock,u->login,u->challenge,hash,\
u->release,u->client_ip,auth);
			} else {
				DBG(4,"Auth Is null?, This is not possible!!, only lobby and game are supposed to run this code");
			}
			n=0;
			return n;
			break;
	}

	//THE REALLY BIG VERY VERY BIG 2nd Switch <--- AT THIS POINT, YOU MUST BE LOGGED -->

	if((u->access_level>=AcNotActivated || (u->authenticated!=1))) { // && (u->adv_msg.cmd!=NetMsgVault && u->adv_msg.cmd!=NetMsgVault2)))) {
		//go out (send go out)
		//if(u->adv_msg.cmd!=NetMsgPlayerPage) {
			//plNetMsgTerminated(sock,RNotAuthenticated,u);
		//}
		n=-15;
		return n;
	}

	switch (u->adv_msg.cmd) {
		case NetMsgRequestMyVaultPlayerList:
			print2log(f_uru,"<RCV> RequestMyVaultPlayerList ");
			print2log(f_uru,"\n");
			vault->adv_msg.x=(random()%100);
			u->ki=vault->adv_msg.x;
			u->x=u->adv_msg.x;
			n=plNetMsgCustomVaultAskPlayerList(sock,u->guid,vault);
			break;
		case NetMsgCreatePlayer: //0x02AE
			print2log(f_uru,"<RCV> CreatePlayer size %i\n",size);
			vault->adv_msg.x=(random()%100);
			u->ki=vault->adv_msg.x;
			u->x=u->adv_msg.x;
			n=plNetMsgCustomVaultCreatePlayer(sock,u->login,u->guid,\
u->access_level,buf+offset,size-offset,vault);
			break;
		case NetMsgDeletePlayer: //0x0384
			print2log(f_uru,"<RCV> DeletePlayer ");
			U16 aux1;
			aux1=*(U16 *)(buf+offset);
			print2log(f_uru,"id %i unk1=%04X\n",u->adv_msg.ki,aux1);
			if(aux1!=0) {
				print2log(f_une,"Recieved an unexpected NetMsgDeletePlayer, aux1 is %04X\n",aux1);
				plNetMsgTerminated(sock,RUnknown,u);
				n=0;
				break;
			}
			vault->adv_msg.ki=u->adv_msg.ki; //copy the ki of the player that will be deleted
			plNetMsgCustomVaultDeletePlayer(sock,u->guid,u->access_level,vault);
			n=0;
			break;
		case NetMsgSetMyActivePlayer: //0x0374:
			print2log(f_uru,"<RCV> SetActivePlayer ");
			Byte avie[101];
			offset+=decode_urustring(avie,buf+offset,100);
			offset+=2;
			rcode=*(Byte *)(buf+offset);
			//store it in u->avie (when validated)
			print2log(f_uru," %s, unk2=%02X\n",avie,rcode);
			n=0;
			if(rcode!=0) {
				print2log(f_une,"Recieved an unexpected NetMsgSetMyActivePlayer, rcode is %02X\n",rcode);
				plNetMsgTerminated(sock,RUnknown,u);
				n=0;
				break;
			}
			//first check the KI and player ownership
			if(u->access_level>AcAdmin) {
				vault->adv_msg.x=(random()%100);
				u->ki=vault->adv_msg.x;
				strcpy((char *)u->avatar_name,(char *)avie);
				vault->adv_msg.ki=u->adv_msg.ki;
				n=plNetMsgCustomVaultCheckKi(sock,u->guid,vault);
				u->bussy=1; //set bussy (wait to vault answer)
			} else {
				//anti multi login protection
				for(i=global_the_bar+1; i<=global_client_count; i++) {
					//if(all_players[i].flag==0) { break; }
					if(all_players[i].authenticated==1 && all_players[i].ki==(S32)u->adv_msg.ki) {
						plog(f_uru,"An age cannot host the same avatar more than two times, disconnecting..\n");
						plNetMsgTerminated(sock,RLoggedInElsewhere,&all_players[i]);
					} else
					if(global_allow_multiple_login==0 && strcmp((char *)all_players[i].login,(char *)u->login)==0 && all_players[i].sid!=u->sid) {
						plNetMsgTerminated(sock,RLoggedInElsewhere,&all_players[i]);
					}
				}
				if(whoami==KGame && strlen((char *)avie)<=0) {
					plNetMsgTerminated(sock,RKickedOff,&all_players[i]);
					n=0;
				} else {
					if(track!=NULL) {
						track->adv_msg.ki=u->adv_msg.ki;
						n=plNetMsgCustomPlayerStatus(sock,u->guid,u->login,avie,2,RJoining,track);
						u->ki=(S32)u->adv_msg.ki; //now set the ki
						u->status=RJoining;
						strcpy((char *)u->avatar_name,(char *)avie);
						plNetMsgActivePlayerSet(sock,u);
						if(vault!=NULL) {
							vault->adv_msg.ki=u->ki;
							U32 timer;
							time((time_t *)&timer);
							timer=timer-u->online_time;
							plNetMsgCustomVaultPlayerStatus(sock,global_age.name,\
global_age.guid,1,timer,vault);
						}
					}
					n=0;
				}
			}
			break;
		case NetMsgVault2: //tpots mod
		case NetMsgVault: //0x0428
			if(u->adv_msg.cmd==NetMsgVault2) {
				u->tpots=1;
			} else {
				u->tpots=2;
			}
			print2log(f_uru,"<RCV> Vault for %i (%i)\n",u->adv_msg.ki,u->ki);
			DBG(6,"u->ki<1000?\n");
			if(u->ki<1000) {
				plNetMsgTerminated(sock,RKickedOff,u);
				n=0;
				break;
			}
			/*if(!check_plnetmsg(u->msg,0,0x0610,0)) {
				plNetMsgTerminated(sock,RKickedOff,u);
				abort();
			}*/
			/*rcode=vault_parse_msg(sock,u,buf,n);
			//rcode=0;
			if(rcode<0) {
				print2log(f_uru,"ERR: Failed parsing a vault message\n");
				if(rcode==-1) {
					plNetMsgTerminated(sock,RKickedOff,u);
				} else {
					plNetMsgTerminated(sock,RUnknownReason,u);
				}
			}*/
			//Route the packet to the vault
			t_vault_mos vault_s;
			DBG(6,"plMainVaultInit()\n");
			plMainVaultInit(&vault_s);
			DBG(6,"plVaultUnpack()\n");
			plVaultUnpack(buf+offset,&vault_s,size-offset,u,0x01);
			DBG(6,"htmlVaultParse()\n");
			htmlVaultParse(&vault_s,u,1);
			DBG(6,"vault->adv_msg.ki=u->ki;");
			vault->adv_msg.ki=u->ki;
			if(u->ki<1000) {
				vault->adv_msg.ki=u->adv_msg.ki;
			}
			DBG(6,"plNetMsgVault()\n");
			n=plNetMsgVault(sock,&vault_s,vault,plNetVersion);
			DBG(6,"plMainVaultDestroy()\n");
			plMainVaultDestroy(&vault_s,0);
			break;
		case NetMsgFindAge: //0x037A
			//u->paged=0; //Should fix something
			print2log(f_uru,"<RCV> NetMsgFindAge ");
			//int unkc,unk3,ccr; //unkc??
			//Byte mask,byte_value2; //A mask
			Byte aguid[19];
			t_AgeLinkStruct * as;
			//bzero(&agestr,sizeof(agestr));
			//parse
			n=storeAgeLinkStruct((void **)&as,buf+offset,size);
			if(n<0) {
				//---store packet---
				mkdir("dumps",0750);
				char fage_mes[100];
				static Byte fage_mes_rot=0;
				sprintf(fage_mes,"dumps/fage%02X.raw",fage_mes_rot++);
				savefile((char *)buf,size,fage_mes);
				n=plNetMsgTerminated(sock,RUnknown,u);
				//---end store packet---
			}
			hex2ascii2(aguid,as->ainfo.guid,8);
			print2log(f_uru," ainfo[Fname:%s,Iname:%s,Guid:%s,Uname:%s,Dname:%s,Lan:%i],\
unk:%i,rules:%i,spw[t:%s,n:%s,c:%s],ccr:%i\n",as->ainfo.filename,\
as->ainfo.instance_name,aguid,as->ainfo.user_name,as->ainfo.display_name,\
as->ainfo.language,as->unk,as->rules,as->spoint.title,as->spoint.name,\
as->spoint.camera,as->ccr);
			if(as->ccr!=0) {
				print2log(f_uru,"Kicking of someone that is attempting to link as a CCR?\n");
				n=plNetMsgTerminated(sock,RKickedOff,u);
			} else {
				if(track!=NULL && u->ki>=500) {
					track->adv_msg.ki=u->ki;
					track->adv_msg.x=u->adv_msg.x;
					n=plNetMsgCustomFindServer(sock,aguid,as->ainfo.filename,u->client_ip,track);
				} else {
					n=0;
				}
#if 0
				if(vault!=NULL && u->ki>=500) {
					vault->adv_msg.ki=u->ki;
					U32 timer;
					time((time_t *)&timer);
					timer=timer-u->online_time;
					plNetMsgCustomVaultPlayerStatus(sock,as->ainfo.filename,aguid,1,timer,vault);
				}
#endif
				u->status=RInroute; //set player in route

			}

#if 0
			unkc=*(U16 *)(buf+offset); //always 0x33
			offset+=2;
			print2log(f_uru,",uc:%04X",unkc);
			if(unkc!=0x33) {
				plNetMsgTerminated(sock,RUnknown,u);
				break;
			}
			mask=*(Byte *)(buf+offset); //mesage format mask??? (seen 0x0B, 0x6F, 0x2F)
			offset++;
			print2log(f_uru,",mask:%02X",mask);
			if(mask!=0x0B && mask!=0x03) {
				plNetMsgTerminated(sock,RUnknown,u);
				break;
			}
			offset+=decode_urustring(agestr.ainfo.filename,buf+offset,150); //the age filename
			offset+=2;
			print2log(f_uru,",fage:%s",agestr.ainfo.filename);
			offset+=decode_urustring(agestr.ainfo.instance_name,buf+offset,150); //the age internal? name
			offset+=2;
			print2log(f_uru,",iage:%s",agestr.ainfo.instance_name);
/*			if(byte_value1!=0x0B) {
				memcpy(u->age_guid,buf+offset,8); //the guid
				offset+=8;
				hex2ascii(u->age_guid, guid, 8);
				print2log(f_uru,",guid:%s",guid);
			} else {
				//let's go to create a fake guid
				u->age_guid[0]=0x00;
				u->age_guid[1]=0x01;
				u->age_guid[2]=0x00;
			} */
			if(mask!=0x03) {
			offset+=decode_urustring(agestr.ainfo.user_name,buf+offset,150); //the owner
			offset+=2;
			print2log(f_uru,",own:%s",agestr.ainfo.user_name);
			}
/*			if(byte_value1!=0x0B) {
			aux1=decode_urustring(full_owner_name,buf+offset,150); //the owner
			offset=offset+aux1+2;
			print2log(f_uru,",fown:%s",full_owner_name);
			}
			if(byte_value1==0x6F) {
				unk1=*(U32 *)(buf+offset); //seen 0 (language?)
				offset+=4;
				print2log(f_uru,",lan:%i",unk1);
			}*/
			byte_value2=*(U16 *)(buf+offset); //link rules??
			offset+=1;
			print2log(f_uru,",lunk:%04X",byte_value2);
			if(byte_value2!=0x01) { //seen 0x00 linking to ahnonay
				//plNetMsgTerminated(sock,RUnknown,u);
				//break;
			}
			agestr.rules=*(U32 *)(buf+offset); //link rules?
			offset+=4;
			print2log(f_uru,",unk2:%08X",agestr.rules);
			unk3=*(U32 *)(buf+offset); //spawn point/link rules?
			offset+=4;
			print2log(f_uru,",unk3:%08X",unk3);
			if(unk3!=0x07) {
				plNetMsgTerminated(sock,RUnknown,u);
				break;
			}
			offset+=decode_urustring(agestr.spoint.title,buf+offset,150); //the spawn point
			offset+=2;
			print2log(f_uru,",spw:%s",agestr.spoint.title);
			offset+=decode_urustring(agestr.spoint.name,buf+offset,150); //the linking point
			offset+=2;
			print2log(f_uru,",link:%s",agestr.spoint.name);
			offset+=decode_urustring(agestr.spoint.camera,buf+offset,150); //the camera stack
			offset+=2;
			print2log(f_uru,",c:%s",agestr.spoint.camera);
			ccr=*(Byte *)(buf+offset); //ccr, I'm going to ban internal releases...
			offset+=1;
			print2log(f_uru,",ccr:%i\n",ccr);
			if(ccr!=0) {
				plNetMsgTerminated(sock,RUnknown,u);
				break;
			}
			//plNetMsgFindAgeReply(sock,u);
			if(track!=NULL) {
				track->adv_msg.ki=u->ki;
				track->adv_msg.x=u->adv_msg.x;
				n=plNetMsgCustomFindServer(sock,(Byte *)"",agestr.ainfo.filename,u->client_ip,track);
			} else {
				n=0;
			}
			//n=0;
#endif
			n=0;
			break;
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
		case NetMsgVaultTask:
			print2log(f_uru,"<RCV> NetMsgVaultTask for %i (%i) \n",u->adv_msg.ki,u->ki);
			//format 0x00061200 (normally)
			//--- store
			char vtask[100];
			static Byte vtask_rot=0;
			sprintf(vtask,"dumps/vtask%02X.raw",vtask_rot++);
			savefile((char *)buf,size,vtask);
			//--- end store
			if(u->ki<1000) {
				plNetMsgTerminated(sock,RKickedOff,u);
				n=0;
				break;
			}
			//Route the packet to the vault
			t_vault_mos vault_task;
			plMainVaultInit(&vault_task);
			plVaultUnpack(buf+offset,&vault_task,size-offset,u,0x02);
			htmlVaultParse(&vault_task,u,1);
			vault->adv_msg.ki=u->ki;
			vault->adv_msg.x=u->adv_msg.x;
			n=plNetMsgVaultTask(sock,&vault_task,vault,plNetVersion);
			plMainVaultDestroy(&vault_task,0);
			break;
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

