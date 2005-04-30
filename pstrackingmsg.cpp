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

#ifndef __U_TRACK_MSG_
#define __U_TRACK_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_TRACK_MSG_ID "$Id$"

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
#include "guid_gen.h"

#include "uru.h"

#include "pstrackingmsg.h"

st_tracking * tracks=NULL;
int n_tracks=0;

int meta_anounced=0;
//Have we send the anounce to the metaserver?

int gt_population=0;
//store here the global population count

/*
U32 * used_ports=NULL;
int nused=0;*/

void init_tracking_data(st_tracking ** t) {
	*t=(st_tracking *)malloc(sizeof(st_tracking) * 1);
	bzero(*t,sizeof(st_tracking));
}

int do_idle_operation() {
	int i;
	FILE * f_hstatus;
	f_hstatus=fopen("status.html","w");
	if(f_hstatus==NULL) {
		return -1;
	}

	fprintf(f_hstatus,"<h2>Currently Players Online</h2>");
	fprintf(f_hstatus,"<br><b>Total population: %i</b><br>",gt_population);
	fprintf(f_hstatus,"<table border=1>");
	fprintf(f_hstatus,"<tr><td>Avie</td><td>KI</td><td>Location</td><td>Status</td><td>Debug info</td></tr>");

	gt_population=0;
	for(i=0; i<n_tracks; i++) {
		if(tracks[i].ki!=0 && tracks[i].sid<=global_client_count) {
			fprintf(f_hstatus,"<tr><td>%s</td><td>%i</td><td>&nbsp;%s</td><td>%i \
%s</td><td>x:%i,f:%i,g:%s,n:%s,w:%i,sid:%i</td></tr>",tracks[i].avie,tracks[i].ki,all_players[tracks[i].sid].age_name,tracks[i].status,\
unet_get_reason_code(tracks[i].status),tracks[i].x,tracks[i].flag,tracks[i].guid,tracks[i].age_name,tracks[i].waiting,tracks[i].sid);
			gt_population++;
		} else {
			tracks[i].ki=0;
		}
	}

	fprintf(f_hstatus,"</table><br>");

	fprintf(f_hstatus,"<h2>Currently Running Servers</h2>");
	fprintf(f_hstatus,"<table border=1>");
	fprintf(f_hstatus,"<tr><td>Server</td><td>Guid</td><td>Port</td><td>Ip</td><td>Debug info</td></tr>");

	for(i=global_the_bar+1; i<=global_client_count; i++) {
		if(all_players[i].client_ip!=0) {
			fprintf(f_hstatus,"<tr><td>");
			if(all_players[i].age_name==0 || strlen((char *)all_players[i].age_name)<=0) {
				fprintf(f_hstatus,"%s","Unknown");
			} else {
				fprintf(f_hstatus,"%s",all_players[i].age_name);
			}
			fprintf(f_hstatus,"</td><td>%s</td><td>%i</td><td>%s</td><td>sid:%i,stamp:%i,l:%i</td></tr>",all_players[i].age_guid,\
ntohs(all_players[i].client_port),get_ip(all_players[i].client_ip),all_players[i].sid,(int)all_players[i].timestamp,all_players[i].load);
		}
	}
	fprintf(f_hstatus,"</table>");

	fclose(f_hstatus);
	return 0;
}


/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_stracking_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		dump_packet(f_uru,buf,size,0,7);
		print2log(f_uru,"\n-------------\n");
	}

	int offset=0;
	int i,n;

	Byte address[100];

	//int rcode;

	//THE BIG VERY VERY BIG Switch
	switch (u->adv_msg.cmd) {
		case NetMsgCustomPlayerStatus:
			Byte flag;
			Byte status;
			memcpy(u->guid,buf+offset,16);
			offset+=16;
			offset+=decode_urustring(u->login,buf+offset,100);
			offset+=2;
			offset+=decode_urustring(u->avatar_name,buf+offset,100);
			offset+=2;
			flag=*(Byte *)(buf+offset);
			offset++;
			status=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru,"<RCV> NetMsgCustomPlayerStatus id %i,%s,%s, %02X,%02X\n",u->adv_msg.ki,u->login,u->avatar_name,flag,status);
			switch(flag) {
				case 0: //delete
					if(n_tracks<=0) break;
					for(i=0; i<n_tracks; i++) {
						if(tracks[i].ki==(S32)u->adv_msg.ki) {
							tracks[i].ki=0;
							tracks[i].ip=0;
							tracks[i].port=0;
							tracks[i].flag=0;
							tracks[i].status=0;
							tracks[i].sid=0;
							tracks[i].waiting=0;
							strcpy((char *)tracks[i].guid,"");
							strcpy((char *)tracks[i].age_name,"");
							strcpy((char *)tracks[i].avie,"");
							strcpy((char *)tracks[i].login,"");
							if(i==n_tracks) {
								while(n_tracks>1 && tracks[n_tracks].ki==0) {
									n_tracks--;
									tracks=(st_tracking *)realloc((void *)tracks,sizeof(st_tracking) * n_tracks);
									if(tracks==NULL) {
										plog(f_err,"WARNING: Not enough memory to track a player\n");
									}
								}
							}
						}
					}
					n=0;
					break;
				case 1: //set invisible
				case 2: //set visible
				case 3: //set only buddies
					if(tracks==NULL && n_tracks==0) {
						init_tracking_data(&tracks);
						n_tracks++;
						tracks[0].ki=u->adv_msg.ki;
						tracks[0].ip=u->client_ip;
						tracks[0].port=u->client_port;
						tracks[0].flag=flag;
						tracks[0].status=status;
						tracks[0].sid=u->sid;
						tracks[0].waiting=0;
						strcpy((char *)tracks[0].guid,"");
						strcpy((char *)tracks[0].age_name,"");
						strcpy((char *)tracks[0].avie,(char *)u->avatar_name);
						strcpy((char *)tracks[0].login,(char *)u->login);
						memcpy(tracks[0].uid,u->guid,16);
					} else if(tracks!=NULL && status!=RLeaving) {
						int mt=-1;
						for(i=0; i<n_tracks; i++) {
							if(tracks[i].ki==(S32)u->adv_msg.ki) { mt=i; break; }
							plog(f_uru,"%s-%s-%i\n",(char *)tracks[i].login,(char *)u->login,global_allow_multiple_login);
							if(global_allow_multiple_login==0 && strcmp((char *)tracks[i].login,(char *)u->login)==0) {
									//printf("yuppy"); fflush(0);
									mt=i; break;
								}
						}
						//delete already logged players
						if(mt!=-1 && tracks[mt].status!=RInroute && tracks[mt].status!=RLeaving && (tracks[mt].ip!=u->client_ip || tracks[mt].port!=u->client_port)) {
						//if(mt!=-1 && tracks[mt].status!=RInroute) {
							if(all_players[tracks[mt].sid].client_ip==tracks[mt].ip &&\
 all_players[tracks[mt].sid].client_port==tracks[mt].port) {
								plog(f_uru,"Disconnecting an older player id %i...\n",tracks[mt].ki);
								all_players[tracks[mt].sid].adv_msg.ki=tracks[mt].ki;
								plNetMsgPlayerTerminated(sock,RLoggedInElsewhere,\
&all_players[tracks[mt].sid]);
							} else {
								plog(f_uru,"Killing a Ghost id %i...\n",tracks[mt].ki);
							}
							//delete track
							tracks[mt].ki=0;
							tracks[mt].ip=0;
							tracks[mt].port=0;
							tracks[mt].flag=0;
							tracks[mt].status=0;
							tracks[mt].sid=0;
							tracks[mt].waiting=0;
							strcpy((char *)tracks[mt].guid,"");
							strcpy((char *)tracks[mt].age_name,"");
							strcpy((char *)tracks[mt].avie,"");
							strcpy((char *)tracks[mt].login,"");
							/*if(mt==n_tracks) {
								while(n_tracks>1 && tracks[n_tracks].ki==0) {
									n_tracks--;
									tracks=(st_tracking *)realloc((void *)tracks,sizeof(st_tracking) * n_tracks);
									if(tracks==NULL) {
										plog(f_err,"WARNING: Not enough memory to track a player\n");
									}
								}
							}*/
							//end delete code
						}
						if(mt==-1) { //not found (find a free slot)
							for(i=0; i<n_tracks; i++) {
								if(tracks[i].ki==0) {
									mt=i;
									break;
								}
							}
						}
						if(mt==-1) { //not found (create a new one)
							n_tracks++;
							tracks=(st_tracking *)realloc((void *)tracks,sizeof(st_tracking) * n_tracks);
							mt=n_tracks-1;
						}
						tracks[mt].ki=u->adv_msg.ki;
						tracks[mt].ip=u->client_ip;
						tracks[mt].port=u->client_port;
						tracks[mt].flag=flag;
						tracks[mt].status=status;
						tracks[mt].sid=u->sid;
						tracks[mt].waiting=0;
						strcpy((char *)tracks[mt].guid,"");
						strcpy((char *)tracks[mt].age_name,"");
						strcpy((char *)tracks[mt].avie,(char *)u->avatar_name);
						strcpy((char *)tracks[mt].login,(char *)u->login);
						memcpy(tracks[mt].uid,u->guid,16);
					} else {
						if(tracks==NULL) {
							plog(f_err,"This should not be happening, something terribly has gone wrong, head for the cover!\n");
						}
					}
					break;
				default:
					plog(f_err,"Unexpected tracking command\n");
			}
			n=0;
			do_idle_operation(); //hmm (bad)
			break;
		case NetMsgCustomSetGuid:
			print2log(f_uru,"<RCV> NetMsgCustomSetGuid\n");
			Byte private_mask[51];
			offset+=decode_urustring(u->age_guid,buf+offset,100);
			offset+=2;
			offset+=decode_urustring(u->age_name,buf+offset,100);
			offset+=2;
			offset+=decode_urustring(private_mask,buf+offset,50);
			offset+=2;
			//u->private_mask=(U32)inet_network((char *)private_mask); <-- WRONG!
			u->private_mask=(U32)inet_addr((char *)private_mask);
			offset+=decode_urustring(u->public_address,buf+offset,100);
			offset+=2;
   plog(f_uru,"Game server %s [%s] Says hello (%s)\n",u->age_name,u->age_guid,u->public_address);
			n=0;
			if(global_enable_metaserver==1 && meta_anounced==0 && !strcmp((char *)u->age_name,"Lobby")) {
				plNetMsgCustomMetaRegister(sock,1,gt_population,u->public_address,global_shard_name,global_shard_website,global_shard_description,global_shard_passwd,global_shard_contact,global_dataset,global_dataset_tag,u);
				meta_anounced=1;
			}
			//now search for -- waiting players --
			for(i=0; i<n_tracks; i++) {
				if(tracks[i].ki>0 && tracks[i].waiting==1 && tracks[i].status==RInroute &&\
 !strcmp((char *)u->age_guid,(char *)tracks[i].guid) &&\
 !strcmp((char *)u->age_name,(char *)tracks[i].age_name)) {
					tracks[i].waiting=0;
					st_uru_client * wr;
					wr=&all_players[tracks[i].sid];
					wr->adv_msg.ki=tracks[i].ki;
					wr->adv_msg.x=tracks[i].x;
					//set the address
					if((u->private_mask & u->client_ip)==(tracks[i].client_ip & u->private_mask)) { //then it's inside the private LAN
						print2log(f_uru,"The client is inside the local lan, setting %s,%08X\n",get_ip(u->client_ip),u->client_ip);
						strcpy((char *)address,get_ip(u->client_ip));
					} else {
						strcpy((char *)address,(char *)u->public_address);
						print2log(f_uru,"The client is outside the local lan, setting %s\n",u->public_address);
					}
					n=plNetMsgCustomServerFound(sock,address,ntohs(u->client_port),\
u->age_guid,u->age_name,wr);
				}
			}
			do_idle_operation(); //hmm, bad!
			break;
		case NetMsgCustomFindServer:
			print2log(f_uru,"<RCV> NetMsgCustomFindServer\n");
			st_uru_client * s;
			s=all_players;
			int tut; //iternator (with player track)
			int game; //iterator (with found game server sid)
			//--- load setting --
			int lowest_sid; //The lowest occuped sid (to fork)
			lowest_sid=0;
			int sid_load; //The lowest registered load (to fork)
			sid_load=10000000;
			//--- end load settings --
			//---- Cutre Soft Presents --
			U16 * table1;
			table1=NULL;
			int table1_n;
			table1_n=0;
			U16 * table2;
			table2=NULL;
			int table2_n;
			table2_n=0;
			U16 low;
			U16 lowest; //candidate 1
			lowest=global_min_port+1;
			low=lowest;
			//int ret;
			//---- End Cutre soft --
			tut=-1;
			game=0;
			for(i=0; i<n_tracks; i++) {
				if(tracks[i].ki==(S32)u->adv_msg.ki) { tut=i; break; }
			}
			if(tut>=0) {
				offset+=decode_urustring(tracks[tut].guid,buf+offset,30);
				offset+=2;
				offset+=decode_urustring(tracks[tut].age_name,buf+offset,100);
				offset+=2;
				tracks[tut].client_ip=*(U32 *)(buf+offset);
				offset+=4;
				//set InRoute
				tracks[tut].status=RInroute;
				//guid generator here
				if(!strcmp((char *)tracks[tut].guid,"") || \
strlen((char *)tracks[tut].guid)!=16 || !strcmp((char *)tracks[tut].guid,"0000000000000000")) {
					//strcpy((char *)tracks[tut].guid,"0000000000000100");
					generate_newguid(tracks[tut].guid,tracks[tut].age_name,u->adv_msg.ki);
					if(!strcmp((char *)tracks[tut].guid,"0000000000000000")) {
						//Link attempted to a non-existent age! -> action kick the player out of the server
						plNetMsgPlayerTerminated(sock,RKickedOff,u);
						n=0;
						break;
					}
				}
				//now search the server
				for(i=global_the_bar+1; i<=global_client_count; i++) {
					if(s[i].client_ip!=0) {
						if(!strcmp((char *)s[i].age_guid,(char *)tracks[tut].guid) &&\
 !strcmp((char *)s[i].age_name,(char *)tracks[tut].age_name)) { //found!!! ALELUYA!!
							game=i;
							break;
						} else { //Not Found!!
							if(ntohs(s[i].client_port)==global_min_port && s[i].load<sid_load) {
								//peer with less load
								lowest_sid=i;
								sid_load=s[i].load;
							}
#if 0
							if(lowest==ntohl(s[i].client_port)) {
								lowest++;
							} else if(lowest<ntohl(s[i].client_port)) {
								table1_n++;
								if(table1==NULL) {
									table1=(U16 *)malloc(sizeof(U16) * table1_n);
								} else {
									table1=(U16 *)realloc((void *)table1,sizeof(U16) * table1_n);
								}
								table1[table1_n-1]=ntohl(s[i].client_port);
							}
#endif
						}
					}
				}

				if(game>0) { //found a free game server hosting the age
					print2log(f_uru,"INF: Game server found for client\n");
					DBG(5,"%08X,%08X----%08X,%08X\n",s[game].client_ip,s[game].private_mask,tracks[tut].client_ip,s[game].private_mask);
					//abort();
					if((s[game].private_mask & s[game].client_ip)==(tracks[tut].client_ip & s[game].private_mask)) { //then it's inside the private LAN
						print2log(f_uru,"The client is inside the local lan, setting %s,%08X\n",get_ip(s[game].client_ip),s[game].client_ip);
						strcpy((char *)address,get_ip(s[game].client_ip));
					} else {
						strcpy((char *)address,(char *)s[game].public_address);
						print2log(f_uru,"The client is outside the local lan, setting %s\n",s[game].public_address);
					}
					n=plNetMsgCustomServerFound(sock,address,ntohs(s[game].client_port),\
s[game].age_guid,s[game].age_name,u);
				} else { //Server Not Found, tell lobby or another game server to create one
					plog(f_uru,"INF: Game server not found, creating a new one\n");

					for(i=global_the_bar+1; i<=global_client_count; i++) {
						if(s[i].client_ip==s[lowest_sid].client_ip) {
							if(lowest==ntohs(s[i].client_port)) {
								lowest++;
							} else if(lowest<ntohs(s[i].client_port)) {
								table1_n++;
								if(table1==NULL) {
									table1=(U16 *)malloc(sizeof(U16) * table1_n);
								} else {
									table1=(U16 *)realloc((void *)table1,sizeof(U16) * table1_n);
								}
								table1[table1_n-1]=ntohs(s[i].client_port);
							}
						}
					}

					while(table1_n>0 && low!=lowest) {
						table2_n=0;
						low=lowest;
						for(i=0; i<table1_n; i++) {
							if(lowest==table1[i]) {
								lowest++;
							} else if(lowest<table1[i]) {
									table2_n++;
									if(table2==NULL) {
										table2=(U16 *)malloc(sizeof(U16) * table2_n);
									} else {
										table2=(U16 *)realloc((void *)table2,sizeof(U16) * table2_n);
									}
									table2[table2_n-1]=table1[i];
							}
						}
						table1_n=table2_n;
						free((void *)table1);
						table1=table2;
						table2=NULL;
					}

					if(table1!=NULL) { free((void *)table1); }
					if(table2!=NULL) { free((void *)table2); }
					//Now we need to tell the Lobby to spawn a new server
					if(lowest<=global_max_port) {
						plog(f_uru,"Telling lobby to fork a server...\n");
						n=plNetMsgCustomForkServer(sock,lowest,tracks[tut].guid,\
	tracks[tut].age_name,&s[lowest_sid]);
						s[lowest_sid].load++;

						/*
						nused++;
						if(used_ports==NUL) {
							used_ports=(U32 *)malloc(sizeof(U32) * 2 * nused);
						} else {
							used_ports=(U32 *)realloc((void *)used_ports,sizeof(U32) * 2 * nused);
						}*/

					} else {
						n=0;
						plog(f_uru,"Cannot fork server, port range full\n");
					}
					tracks[tut].waiting=1;
					tracks[tut].x=u->adv_msg.x;
				} //end else (Not found a server)
			} else {
				plog(f_err,"INFO: Ignored a NetMsgCustomFindServer because I can't find the player that requested it! p:%i\n",u->adv_msg.ki);
				n=0;
			}
			break;
		case NetMsgCustomMetaPing:
			double mtime;
			Byte destination;
			print2log(f_uru,"<RCV> CustomMetaPing ");
			mtime=*(double *)(buf+offset); //8 bytes (double)
			offset+=0x08;
			destination=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru," t:%e dst:%i\n",mtime,destination);
			//Send a mirror copy of the recieved ping msg
			if(global_enable_metaserver==1) {
				n=plNetMsgCustomMetaPing(sock,1,mtime,destination,gt_population,u);
			}

			if(global_enable_metaserver==0 && !strcmp((char *)u->age_name,"Lobby")) {
				plNetMsgCustomMetaRegister(sock,0,gt_population,u->public_address,global_shard_name,global_shard_website,global_shard_description,global_shard_passwd,global_shard_contact,global_dataset,global_dataset_tag,u);
			}

			break;
		default:
			n=-7;
			break;
	}

	logflush(f_uru);

	return n;
}
#endif

