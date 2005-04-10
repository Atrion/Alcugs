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

#ifndef __U_SCAUTH_MSG_
#define __U_SCAUTH_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_SCAUTH_MSG_ID "$Id$"

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

#include "pscauthmsg.h"

/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_scauth_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		dump_packet(f_uru,buf,size,0,7);
		print2log(f_uru,"\n-------------\n");
	}

	int offset=0;
	int i,n;

	int sid;

	//THE BIG VERY VERY BIG Switch
	switch (u->adv_msg.cmd) {
		case NetMsgCustomAuthResponse:
			//int sid;
			int rcode;
			sid=0;
			print2log(f_uru,"<RCV> NetMsgCustomAuthResponse \n");
			//login
			offset+=decode_urustring(u->login,buf+offset,100);
			offset+=2;

			//search by login
			DBG(4,"for(i=%i; i<=%i i++)\n",global_the_bar+1,global_client_count);
			for(i=global_the_bar+1; i<=global_client_count; i++) {
				DBG(3,"searching for %s - %s (%i,%i) -%i \n",u->login,all_players[i].login,u->adv_msg.x,all_players[i].ki,i);
				if(!strcmp((char *)all_players[i].login,(char *)u->login) && u->adv_msg.x==(U32)all_players[i].ki) {
					sid=i;
					u=&all_players[i];
					break;
				}
			}

			if(sid!=0) {
				//result
				u->adv_msg.reason=*(Byte *)(buf+offset);
				offset++;
				//passwd
				offset+=decode_urustring(u->passwd,buf+offset,100);
				offset+=2;
				//GUID
				memcpy(u->guid,buf+offset,16);
				offset+=16;
				rcode=u->adv_msg.reason;
				u->access_level=*(Byte *)(buf+offset);
				offset++;
			} else {
				print2log(f_uru,"INF: Player not found! c:%i bar:%i i:%i\n",global_client_count,global_the_bar,i);
				//abort();
				n=0;
				break;
			}

			if(rcode==0x00) {
				print2log(f_uru," Valid passwd auth\n");
				print2log(f_acc,"Succesfull login for %s\n",u->login);
				memcpy(u->adv_msg.guid,u->guid,16); //set the uid
				plNetMsgAccountAuthenticated(sock,0x00,u);
				u->authenticated=1;
				//print2log(f_uru," I HAVE SETUP AUTHENTICATED WITH VALUE=2<---\n\n");
			} else {
				bzero(u->adv_msg.guid,16);
				//strcpy((char *)global_age.guid,"0000000000000000");
				print2log(f_uru," Incorrect passwd auth\n");
				print2log(f_acc,"Auth failed for %s, reason %i\n",u->login,rcode);
				plNetMsgAccountAuthenticated(sock,rcode,u);
				plNetMsgTerminated(sock,RNotAuthenticated,u);
				//plNetMsgTerminated(go out);
			}
			logflush(f_uru);
			logflush(f_acc);
			n=0;
			break;
		case NetMsgCustomVaultPlayerList:
			print2log(f_uru,"<RCV> NetMsgCustomVaultPlayerList ");
			memcpy(u->guid,buf+offset,16);
			offset+=16;

			//search by guid
			//int sid=0;
			sid=0;
			for(i=global_the_bar+1; i<=global_client_count; i++) {
				if(!memcmp((char *)all_players[i].guid,(char *)u->guid,16) && u->adv_msg.x==(U32)all_players[i].ki) {
					sid=i;
					u=&all_players[i];
					break;
				}
			}

			if(sid!=0) {
				u->p_num=*(U16 *)(buf+offset);
				offset+=2;
				u->p_list=(st_vault_player *)(malloc(sizeof(st_vault_player) * u->p_num));
				if(u->p_list==NULL) {
					plog(f_err,"FATAL: Not enough memory (in allowcating u->p_list)!\n");
					n=0;
					break;
				}
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
				plNetMsgVaultPlayerList(sock,u);
				free((void *)u->p_list); //important!!
				u->p_list=NULL; //arfgh!
			} else {
				print2log(f_uru,"INF: Player not found! c:%i bar:%i i:%i\n",global_client_count,global_the_bar,i);
				//abort();
			}
			n=0;
			break;
		case NetMsgCustomVaultPlayerCreated:
			print2log(f_uru,"<RCV> NetMsgCustomVaultPlayerCreated \n");
			memcpy(u->guid,buf+offset,16);
			offset+=16;

			//search by guid
			sid=0;
			for(i=global_the_bar+1; i<=global_client_count; i++) {
				if(!memcmp((char *)all_players[i].guid,(char *)u->guid,16) && u->adv_msg.x==(U32)all_players[i].ki) {
					sid=i;
					all_players[i].adv_msg.ki=u->adv_msg.ki;
					u=&all_players[i];
					break;
				}
			}
			if(sid!=0) {
				u->adv_msg.reason=*(Byte *)(buf+offset);
				offset++;
				print2log(f_uru,"ki: %i, result: %i\n",u->adv_msg.ki,u->adv_msg.reason);
				u->adv_msg.x=u->x;
				n=plNetMsgPlayerCreated(sock,u->adv_msg.reason,u);
			} else {
				print2log(f_uru,"INF: Player not found! c:%i bar:%i i:%i\n",global_client_count,global_the_bar,i);
				//abort();
				n=0;
			}
			break;
		case NetMsgCustomVaultKiChecked: //continuation of setmyactiveplayer
			print2log(f_uru,"<RCV> NetMsgCustomVaultKiChecked \n");
			memcpy(u->guid,buf+offset,16);
			offset+=16;

			Byte avie[200];

			n=0;
			//search by guid
			sid=0;
			for(i=global_the_bar+1; i<=global_client_count; i++) {
				if(!memcmp((char *)all_players[i].guid,(char *)u->guid,16) && u->adv_msg.x==(U32)all_players[i].ki) {
					sid=i;
					all_players[i].adv_msg.ki=u->adv_msg.ki;
					u=&all_players[i];
					break;
				}
			}
			if(sid!=0) {
				u->adv_msg.reason=*(Byte *)(buf+offset);
				offset++;
				offset+=decode_urustring(avie,buf+offset,100);
				offset+=2;
				print2log(f_uru,"result: %i,%s\n",u->adv_msg.reason,u->avatar_name);

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
				if(whoami==KGame && strcmp((char *)u->avatar_name,(char *)avie)) {
					plNetMsgTerminated(sock,RKickedOff,&all_players[i]);
					n=0;
				} else {
					if(track!=NULL) {
						track->adv_msg.ki=u->adv_msg.ki;
						n=plNetMsgCustomPlayerStatus(sock,u->guid,u->login,avie,2,RJoining,track);
						u->ki=(S32)u->adv_msg.ki; //now set the ki
						if(vault!=NULL) {
							vault->adv_msg.ki=u->ki;
							U32 timer;
							time((time_t *)&timer);
							timer=timer-u->online_time;
							plNetMsgCustomVaultPlayerStatus(sock,global_age.name,global_age.guid,1,\
							timer,vault);
						}
					}
					u->status=RJoining;
					//it was in the game server
					//strcpy((char *)u->avatar_name,(char *)avie);
					plNetMsgActivePlayerSet(sock,u);
					n=0;
				}
				//end multi login protection
				u->bussy=0;
			} else {
				print2log(f_uru,"INF: Player not found! c:%i bar:%i i:%i\n",global_client_count,global_the_bar,i);
				//abort();
				n=0;
			}
			break;
		case NetMsgVault:
			print2log(f_uru,"<RCV> NetMsgVault for %i \n",u->adv_msg.ki);
			t_vault_mos vault_s;
			plMainVaultInit(&vault_s);
			plVaultUnpack(buf+offset,&vault_s,size-offset,u,0x01);
			if(u->adv_msg.ki<1000) {
				//do game server vault operations
				n=0;
			} else {
				//Route the packet to the addient client
				//search by KI
				sid=0;
				for(i=global_the_bar+1; i<=global_client_count; i++) {
					if(u->adv_msg.ki==(U32)all_players[i].ki) { // && all_players[i].status!=RInroute) {
						sid=i;
						all_players[i].adv_msg.ki=u->adv_msg.ki;
						u=&all_players[i];
						break;
					}
				}
				if(sid>0) {
					u->adv_msg.cmd=NetMsgVault;
					htmlVaultParse(&vault_s,u,0);
					n=plNetMsgVault(sock,&vault_s,u,0);
				} else {
					plog(f_err,"Cannot Route Vault message to Client %i, since it's not in my client list\n",u->adv_msg.ki);
					n=0;
				}
			}
			plMainVaultDestroy(&vault_s,0);
			break;
		case NetMsgVaultTask:
			print2log(f_uru,"<RCV> NetMsgVaultTask for %i \n",u->adv_msg.ki);
			t_vault_mos vault_task;
			plMainVaultInit(&vault_task);
			plVaultUnpack(buf+offset,&vault_task,size-offset,u,0x02);
			if(u->adv_msg.ki<1000) {
				//do game server vault operations
				n=0;
			} else {
				//Route the packet to the addient client
				//search by KI
				sid=0;
				for(i=global_the_bar+1; i<=global_client_count; i++) {
					if(u->adv_msg.ki==(U32)all_players[i].ki) {
						sid=i;
						all_players[i].adv_msg.ki=u->adv_msg.ki;
						all_players[i].adv_msg.x=u->adv_msg.x;
						u=&all_players[i];
						break;
					}
				}
				if(sid>0) {
					u->adv_msg.cmd=NetMsgVaultTask;
					htmlVaultParse(&vault_task,u,0);
					n=plNetMsgVaultTask(sock,&vault_task,u,0);
				} else {
					plog(f_err,"Cannot Route Vault message to Client %i, since it's not in my client list\n",u->adv_msg.ki);
					n=0;
				}
			}
			plMainVaultDestroy(&vault_task,0);
			break;
		case NetMsgCustomServerFound:
			int port;
			Byte address[200];
			Byte guid[21];
			Byte age_fname[100];
			port=*(U16 *)(buf+offset);
			offset+=2;
			offset+=decode_urustring(address,buf+offset,199);
			offset+=2;
			offset+=decode_urustring(guid,buf+offset,20);
			offset+=2;
			offset+=decode_urustring(age_fname,buf+offset,99);
			offset+=2;
			print2log(f_uru,"<RCV> NetMsgCustomServerFound for %i, %s:%i,%s,%s\n",u->adv_msg.ki,address,port,guid,age_fname);

			//Route the packet to the addient client
			//search by KI
			sid=0;
			for(i=global_the_bar+1; i<=global_client_count; i++) {
				if(u->adv_msg.ki==(U32)all_players[i].ki) {
					sid=i;
					all_players[i].adv_msg.ki=u->adv_msg.ki;
					all_players[i].adv_msg.x=u->adv_msg.x;
					u=&all_players[i];
					break;
				}
			}
			if(sid!=0) {
				n=plNetMsgFindAgeReply(sock,age_fname,address,port,guid,u);
			} else {
				print2log(f_uru,"Player not found!, to send the FindAgeReply!!\n");
				n=0;
			}
			break;
		case NetMsgCustomForkServer:
			print2log(f_uru,"<RCV> NetMsgCustomForkServer ");
			U16 f_port;
			char f_strport[7];
			char f_guid[20];
			char f_name[100];
			char f_logs[300];
			if(global_fork==0) {  //disallow forking
				n=0;
				print2log(f_uru," ignored, because we are in manual mode\n");
				break;
			}

			f_port=*(U16 *)(buf+offset);
			offset+=2;
			sprintf(f_strport,"%i",f_port);
			offset+=decode_urustring((Byte *)f_guid,buf+offset,19);
			offset+=2;
			offset+=decode_urustring((Byte *)f_name,buf+offset,99);
			offset+=2;
			//Never trust the user input!!
			str_filter((Byte *)f_guid);
			str_filter((Byte *)f_name);
			if(strlen(f_guid)!=16 || strlen(f_name)<=0) {
				n=0;
			} else {
				sprintf(f_logs,"%s/%s/%s/",global_log_files_path,f_name,f_guid);
				//do the forking here
				int pid;
				pid=fork();
				if(pid==0) { //we are the new server process
					//hacer el puto recubrimiento de memoria
					close(sock); //close the socket (or we will not be able to restart the lobby without stopping all game servers)
					//TODO close logging files, but when the debugging system is changed.

					char * wthatb;
					wthatb=(char *)malloc(sizeof(char) * (strlen((const char *)global_binary_path)+20));
					//allow blank binary path
					if(strlen((const char *)global_binary_path)==0) {
						sprintf(wthatb,"uru_game");
					} else {
						sprintf(wthatb,"%s/uru_game",(const char *)global_binary_path);
					}

					if (global_config_alt) {
						execlp(wthatb,f_name,"-p",f_strport,"-guid",f_guid,"-c",\
						global_config_alt,"-name",f_name,"-log",f_logs,NULL);
					} else {
						execlp(wthatb,f_name,"-p",f_strport,"-guid",f_guid,"-name",f_name,\
						"-log",f_logs,NULL);
					}
					//on error exit(0)
					exit(-1);
				}
				print2log(f_uru," server %s [%s] forked at port %i\n",f_name,f_guid,f_port);
				n=0;
			}
			break;
		case NetMsgCustomMetaPing:
			double mtime;
			Byte destination;
			U32 pop,ver;
			print2log(f_uru,"<RCV> CustomMetaPing ");
			mtime=*(double *)(buf+offset); //8 bytes (double)
			offset+=0x08;
			destination=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru," t:%e dst:%i\n",mtime,destination);
			//Send a mirror copy of the recieved ping msg
			ver=*(U32 *)(buf+offset);
			offset+=4;
			pop=*(U32 *)(buf+offset);
			offset+=4;

			if(meta!=NULL & track!=NULL) {
				if(u==meta) {
					n=plNetMsgCustomMetaPing(sock,1,mtime,destination,pop,track);
				} else if(u==track) {
					n=plNetMsgCustomMetaPing(sock,1,mtime,destination,pop,meta);
				}
			}
			n=0;
			break;
		case NetMsgCustomMetaRegister:
			print2log(f_uru,"<RCV> NetMsgCustomMetaRegister \n");
			if(whoami!=KLobby || meta==NULL) {
				n=0;
				break;
			}
			U32 cmd,dataset; //pop,ver
			//Byte address[100];
			Byte name[100];
			Byte site[200];
			Byte desc[255];
			Byte passwd[100];
			Byte contact[100];
			Byte dset[50];

			ver=*(U32 *)(buf+offset); //version
			if(ver!=1) {
				print2log(f_err," NetMsgCustomMetaRegister version mismatch! %i\n",ver);
				n=0;
				break;
			}
			offset+=4;
			cmd=*(U32 *)(buf+offset); //command
			offset+=4;
			//1 -> register
			//0 -> unregister
			pop=*(U32 *)(buf+offset); //population
			offset+=4;

			offset+=decode_urustring(address,buf+offset,99);
			offset+=2;
			offset+=decode_urustring(name,buf+offset,99);
			offset+=2;
			offset+=decode_urustring(site,buf+offset,199);
			offset+=2;
			offset+=decode_urustring(desc,buf+offset,254);
			offset+=2;
			offset+=decode_urustring(passwd,buf+offset,99);
			offset+=2;
			offset+=decode_urustring(contact,buf+offset,99);
			offset+=2;
			dataset=*(U32 *)(buf+offset); //dat
			offset+=4;

			offset+=decode_urustring(dset,buf+offset,49);
			offset+=2;

			plNetMsgCustomMetaRegister(sock,cmd,pop,address,name,site,desc,\
			passwd,contact,dataset,dset,meta);

			n=0;
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

