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

//This is the vault subsystem required to parse those vault messages

/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGSUBSYS_ID "$Id$"
const char * _tracking_driver_ver="1.1.1c";

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef __WIN32__

#  include "windoze.h"

#endif


#include "data_types.h"
#include "stdebug.h"
#include "prot.h"
#include "protocol.h"

#include "useful.h"
#include "conv_funs.h"
#include "version.h"

#include "config_parser.h"
#include "uru.h"

#include "ageparser.h"

#include "gbasicmsg.h"
#include "gtrackingmsg.h"
#include "guid_gen.h"

#include "trackingsubsys.h"

#include "debug.h"

int tracking_initialitzed=0;

st_log * f_track=NULL;

char * global_private_ages=NULL;
int global_muinstance_mode=1;

Byte xml_e=1,status1_e=1,status2_e=1;
char * xml=NULL;
char * status1=NULL;
char * status2=NULL;

t_age_def * global_age_def=NULL; //age struct
int global_age_def_n=0; //total number of ages

//tracks
int n_tracks=0;
st_tracking * tracks=NULL;

int n_lobbys=0;
st_track_peer * lobbys=NULL;

int gt_population=0;
int gt_population2=0;

char * system_addr=NULL;
U16 system_port=0;

//parse age files
int vsys_parse_age_files() {
	print2log(f_track,"Parsing AGE descriptors...\n");

	char * where;
	where = (char *)cnf_getString("./age/","age","global",global_config);

	//1st destroy if them already exist
	int i;
	for(i=0; i<global_age_def_n; i++) {
		destroy_age_def(&global_age_def[i]);
	}
	if(global_age_def!=NULL) {
		free((void *)global_age_def);
		global_age_def=NULL;
	}
	global_age_def=0;

	global_age_def_n=read_all_age_descriptors(f_track,(char *)where,&global_age_def);
	if(global_age_def_n<=0) {
		print2log(f_track,"FATAL, failed to read AGE descriptors from %s...\n",where);
		return -1;
	}
	return 0;
}


int init_tracking_subsys() {
	int ret=0;
	if(tracking_initialitzed) return 0;
	tracking_initialitzed=1;

	DBG(5,"Initialiting tracking subsystem...\n");

	char * tmp=NULL;
	global_muinstance_mode=cnf_getByte(global_muinstance_mode,"instance_mode","global",\
	global_config);
	tmp = (char *)cnf_getString("AvatarCustomization,Personal,Nexus,BahroCave","private_ages","global",global_config);
	global_private_ages=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(global_private_ages,tmp);

	tmp = (char *)cnf_getString("log/status.xml","track.xml.path","global",global_config);
	xml=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(xml,tmp);
	tmp = (char *)cnf_getString("log/status.html","track.html.path","global",global_config);
	status1=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(status1,tmp);
	tmp = (char *)cnf_getString("log/stdbg.html","track.htmldbg.path","global",global_config);
	status2=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(status2,tmp);

	xml_e = cnf_getByte(xml_e,"track.xml","global",global_config);
	status1_e = cnf_getByte(status1_e,"track.html","global",global_config);
	status2_e = cnf_getByte(status2_e,"track.htmldbg","global",global_config);

	if(cnf_getByte(1,"tracking.log","global",global_config)==1) {
		f_track=open_log("tracking.log",2,DF_STDOUT);
	}

	vsys_parse_age_files();

	tmp=cnf_getString("localhost","bind","global",global_config);
	system_addr=(char *)malloc(sizeof(char) * (strlen(tmp)+1));
	strcpy(system_addr,tmp);

	system_port=cnf_getU16(5000,"port","global",global_config);

	plog(f_uru,"INF: Tracking subsystem v %s started\n",_tracking_driver_ver);
	plog(f_uru,"Log Path: %s\n",stdebug_config->path);
	logflush(f_uru);

	return ret;
}

void stop_tracking_subsys() {
	if(tracking_initialitzed!=1) return;
	plog(f_uru,"INF: Tracking subsystem stopped\n");
	if(global_private_ages!=NULL) free((void *)global_private_ages);
	if(xml!=NULL) free((void *)xml);
	if(status1!=NULL) free((void *)status1);
	if(status2!=NULL) free((void *)status2);
	if(system_addr!=NULL) free((void *)system_addr);
	global_private_ages=NULL;
	xml=NULL;
	status1=NULL;
	status2=NULL;
	system_addr=NULL;

	int i;
	for(i=0; i<global_age_def_n; i++) {
		DBG(5,"destroying age %i of %i\n",i,global_age_def_n);
		DBG(5,"dmalloc_verify()\n");
		dmalloc_verify(NULL);
		destroy_age_def(&global_age_def[i]);
	}
	if(global_age_def!=NULL) {
		free((void *)global_age_def);
		global_age_def=NULL;
	}
	global_age_def=0;

	tracking_initialitzed=0;
}

//Needs to be done on SIGHUP to be able to apply any changes performed in the config files
void reload_tracking_subsys() {
	plog(f_uru,"INF: Reloading tracking subsystem...\n");
	stop_tracking_subsys();
	init_tracking_subsys();
}

//note, reload will kill all connected MGRS, we should use this one instead
void update_tracking_subsys() {
	plog(f_track,"INF: Updating tracking server subsystem...\n");
	vsys_parse_age_files();
}

void tracking_subsys_idle_operation(st_unet * net) {
	// Do idle tasks here

#if 0
	cur=lobbys[i].childs[e];
	if(cur==-1 || net_check_address(net,cur)!=0 || net->s[cur].ip==0)
		//!TODO cleanup!!!
#endif

	tracking_dump_players_list(net);
}

///

int tracking_subsys_update(st_unet * net,int ki,char * guid,char * acct,char * avie,Byte flag,Byte status,int sid) {
	st_uru_client * u=&net->s[sid];
	int i;

	switch(flag) {
		case 0: //delete
			if(n_tracks<=0) break;
			for(i=0; i<n_tracks; i++) {
				if(tracks[i].ki==ki) {
					gt_population--;
					if(gt_population<=0) gt_population=0;
					memset(&tracks[i],0,sizeof(st_tracking));
					if(i==n_tracks) {
						while(n_tracks>0 && tracks[n_tracks].ki==0) {
							n_tracks--;
							tracks=(st_tracking *)realloc((void *)tracks,sizeof(st_tracking) * n_tracks);
							if(tracks==NULL && n_tracks!=0) {
								plog(f_err,"WARNING: realloc returned NULL!\n");
							}
						}
					}
				}
			}
			break;
		case 1: //set invisible
		case 2: //set visible
		case 3: //set only buddies
			if(status==RLeaving) break;
			int mt=-1;
			for(i=0; i<n_tracks; i++) {
				if(tracks[i].ki==ki) { mt=i; break; }
				//plog(f_track,"%s-%s-%i\n",(char *)tracks[i].login,(char *)acct,1);
				if(1==0 && strcmp((char *)tracks[i].login,(char *)acct)==0) {
					mt=i; break; //to dissallow multiple login, TODO.
				}
			}
			//delete already logged players
			if(mt!=-1 && tracks[mt].status!=RInroute && tracks[mt].status!=RLeaving && (tracks[mt].ip!=u->ip || tracks[mt].port!=u->port)) {
			//if(mt!=-1 && tracks[mt].status!=RInroute) {
				if(net->s[tracks[mt].sid].ip==tracks[mt].ip &&\
				net->s[tracks[mt].sid].port==tracks[mt].port) {
					plog(f_track,"Disconnecting an older player id %i...\n",tracks[mt].ki);
					net->s[tracks[mt].sid].hmsg.ki=tracks[mt].ki;
					plNetMsgPlayerTerminated(net,RLoggedInElsewhere,tracks[mt].sid);
					gt_population--;
				} else {
					plog(f_track,"Killing a Ghost id %i...\n",tracks[mt].ki);
					gt_population--;
				}
				memset(&tracks[mt],0,sizeof(st_tracking));
			}
			//end delete
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
			memset((void *)&tracks[mt],0,sizeof(st_tracking));
			tracks[mt].ki=ki;
			tracks[mt].ip=u->ip;
			tracks[mt].port=u->port;
			tracks[mt].flag=flag;
			tracks[mt].status=status;
			tracks[mt].sid=sid;
			tracks[mt].waiting=0;
			strcpy((char *)tracks[mt].guid,"");
			strcpy((char *)tracks[mt].age_name,"");
			strcpy((char *)tracks[mt].avie,(char *)avie);
			strcpy((char *)tracks[mt].login,(char *)acct);
			memcpy(tracks[mt].uid,guid,16);
			gt_population++;
	}
	tracking_dump_players_list(net);
	return 0;
}

int tracking_subsys_addgame(st_unet * net,int sid) {
	int i;//,e;
	st_uru_client * u=&net->s[sid];
	st_uru_client * p=NULL;

	int fl=-1; //found lobby
	char il=0; //init lobby
	int fp=-1; //found peer
	int can=-1; //candidate peer
	for(i=0; i<n_lobbys; i++) {
		if(lobbys[i].sid==-1) { if(can==-1) { can=i; } }
		else if(net_check_address(net,lobbys[i].sid)==0) {
			p=&net->s[lobbys[i].sid];
			if(u->ip == p->ip) { //same source peer (same node)
				fl=i; //we found a lobby
				break;
			}
		}
	}

	//check candidate
	if(fl==-1 && can!=-1) {
		memset(&lobbys[can],0,sizeof(st_track_peer));
		fl=can;
		il=1;
	}
	//
	/*
	if(fl==-1) { //Can't find lobby, create a host node.
		for(i=0; i<n_lobbys; i++) {
			if(lobbys[i].sid==-1) {
				fl=i;
				il=1;
				memset(&lobbys[i],0,sizeof(st_track_peer));
				break;
			}
		}
	}*/
	if(fl==-1) { //Still not found (create it)
		fl=n_lobbys;
		il=1;
		n_lobbys++;
		lobbys=(st_track_peer *)realloc((void *)lobbys,sizeof(st_track_peer) * n_lobbys);
		if(lobbys==NULL) { plog(f_err,"ERR: FATAL not enough memory!\n"); return -1; }
		memset(&lobbys[fl],0,sizeof(st_track_peer));
	}

	if(il==1 || (p!=NULL && u->port==p->port)) {
		//update set settings to this lobby
		char fguid[10];
		*(U16 *)(fguid)=0xFFFF;
		*(U32 *)(fguid+2)=(U32)random();
		*(Byte *)(fguid+6)=(Byte)get_microseconds();
		*(Byte *)(fguid+7)=0x02;
		lobbys[fl].sid=sid;
		lobbys[fl].port_start=5001;
		lobbys[fl].port_end=6000; //last port
		hex2ascii2((Byte *)lobbys[fl].fakeguid,(Byte *)fguid,8);
		if(u->whoami==KLobby) {
			DBG(5,"port:%i\n",u->port);
			if(u->port==htons(5000)) { //idiot
				lobbys[fl].flags=TR_FORK;
			}
			//abort();
		} else {
			plog(f_track,"WARNING: New Game server from lobbyless unknown peer %s(%s):%i\n",u->acct,get_ip(u->ip),ntohs(u->port));
			lobbys[fl].flags=TR_FAKE;
		}
	}

	if(u->whoami!=KLobby) {
		//search for the game server
		st_track_peer * lob=&lobbys[fl];
		can=-1;

		for(i=0; i<lob->n_childs; i++) {
			if(lob->childs[i]==-1) { if(can==-1) { can=i; } }
			else if(net_check_address(net,lob->childs[i])==0) {
				p=&net->s[lob->childs[i]];
				if(u->ip == p->ip && u->port==p->port) { //same source peer (same node)
					fp=i; //we found a game server (peer)
					break;
				}
			}
		}
		if(fp==-1 && can!=-1) {
			fp=can;
		}
		if(fp==-1) {
			fp=lob->n_childs;
			lob->n_childs++;
			lob->childs=(int *)realloc((void *)lob->childs,sizeof(int) * lob->n_childs);
		}
		lob->childs[fp]=sid;
		tracking_notify_waiting_players(net,sid); //send a ServerFound to all waiting players
	}
	tracking_dump_players_list(net);
	return 0;
}

int tracking_notify_waiting_players(st_unet * net, int sid) {
	int i;
	//char address[200];
	st_uru_client * u=&net->s[sid];
	//now search for -- waiting players --
	for(i=0; i<n_tracks; i++) {
		if(tracks[i].ki>0 && tracks[i].waiting==1 && tracks[i].status==RInroute &&\
 !strcmp((char *)u->guid,(char *)tracks[i].guid) &&\
 !strcmp((char *)u->name,(char *)tracks[i].age_name)) {
			tracks[i].waiting=0;
			st_uru_client * wr;
			wr=&net->s[tracks[i].sid];
			wr->hmsg.ki=tracks[i].ki;
			wr->hmsg.x=tracks[i].x;
			//set the address server address
			#if 0
			plog(f_track,"client:%08X(%s), net:%08X(%s) mask:%08X(%s)\n",\
			tracks[i].client_ip,get_ip(tracks[i].client_ip),\
			net->lan_addr,get_ip(net->lan_addr),net->lan_mask,get_ip(net->lan_mask));
			if((tracks[i].client_ip == 0x0100007F)) {
				plog(f_track,"The client is using the loopback address\n");
				strcpy((char *)address,"127.0.0.1");
			} else if((tracks[i].client_ip & net->lan_mask)==net->lan_addr) {
				//then it's inside the private LAN
				plog(f_track,"The client is inside the local lan, setting %s,%08X\n",\
				get_ip(u->ip),u->ip);
				strcpy((char *)address,get_ip(u->ip));
			} else {
				strcpy((char *)address,(char *)u->acct); //public address host stored in acct
				plog(f_uru,"The client is outside the local lan, setting %s\n",u->acct);
			}
			plNetMsgCustomServerFound(net,(Byte *)address,ntohs(u->port),\
			(Byte *)u->guid,(Byte *)u->name,wr->sid);
			#endif
			tracking_subsys_serverfound(net,i,wr->sid,sid);
		}
	}
	tracking_dump_players_list(net);
	return 0;
}


int tracking_subsys_findserver(st_unet * net,char * guid,char * name,U32 cip,int sid) {
	st_uru_client * u=&net->s[sid]; //peer that requested the FindServer Message
	int i;
	st_uru_client * s;
	s=net->s;
	int tut=-1; //iternator (with player track)
	int game=-1; //iterator (with found game server sid)
	int lobby=-1; //iterator (with found lobby server sid)
	int load=10000;

	//Check the player
	for(i=0; i<n_tracks; i++) {
		DBG(5,"pl:%i==%i? %08X==%08X?\n",tracks[i].ki,(S32)u->hmsg.ki,tracks[i].client_ip,cip);
		if(tracks[i].ki==(S32)u->hmsg.ki) { /* && tracks[i].client_ip==cip) { */
			tracks[i].client_ip=cip; //a considerable idiot
			tut=i; break;
		}
	}
	if(tut==-1) {
		plog(net->err,"INFO: Ignored a NetMsgCustomFindServer because I can't find the player that requested it! p:%i\n",u->hmsg.ki);
		return -1;
	}
	//continue - Search the peer
	//set InRoute
	tracks[tut].status=RInroute;
	strcpy((char *)tracks[tut].guid,guid);
	strcpy((char *)tracks[tut].age_name,name);
	//Get the age GUID
	if(!strcmp(guid,"") || strlen(guid)!=16 || !strcmp(guid,"0000000000000000")) {
		generate_newguid((Byte *)guid,(Byte *)name,u->hmsg.ki);
		strcpy((char *)tracks[tut].guid,guid);
		if(!strcmp((char *)tracks[tut].guid,"0000000000000000")) {
			//Link attempted to a non-existent age! -> action kick the player out of the server
			//
			//plNetKIMsg(net,"hAxOr",sid);
			//
			plog(net->err,"HACK: Player %i attempted access to an unauthorized non-existent age and/or server process: %s. The player is using a hacked/manipulated client\n",u->hmsg.ki,name);
			plNetMsgPlayerTerminated(net,RKickedOff,sid);
			return -1;
		}
	}
	//now search the server
	for(i=0; i<(int)net->n; i++) {
		if(s[i].ip!=0) {
			if(!strcmp((char *)s[i].guid,(char *)tracks[tut].guid) && \
			!strcmp((char *)s[i].name,(char *)tracks[tut].age_name)) {
				game=i; //found a game
				break;
			} // else
		}
	}
	if(game!=-1) { //found a free game server hosting the age
		plog(f_uru,"INF: Game server found for client %i\n",u->hmsg.ki);
		//set the address server address
		tracking_subsys_serverfound(net,tut,sid,game);
	} else { //Server Not Found, tell lobby or another game server to create one
		plog(f_uru,"INF: Game server not found, creating a new one\n");
		//search lobby peer with the lowest load
		for(i=0; i<n_lobbys; i++) {
			DBG(5,"load:%i,i:%i,n_childs:%i,flags:%i\n",load,i,lobbys[i].n_childs,lobbys[i].flags);
			if(lobbys[i].n_childs<load && (lobbys[i].flags & TR_FORK)) {
				load=lobbys[i].n_childs;
				lobby=i;
			}
		}
		if(lobby!=-1) {
			st_track_peer * lob=&lobbys[lobby];
			//Search for the lowest available port
			int cur;
			int lowest,low;
			//tables
			int table1_n=0;
			U16 * table1=NULL;
			int table2_n=0;
			U16 * table2=NULL;

			lowest=lob->port_start; //set lowest port
			low=lob->port_start;

			for(i=0; i<lob->n_childs; i++) {
				cur=lob->childs[i];
				if(cur!=-1 && net_check_address(net,cur)==0) {
					if(lowest==ntohs(s[cur].port)) {
						lowest++;
					} else if(lowest<ntohs(s[cur].port)) {
						table1_n++;
						table1=(U16 *)realloc((void *)table1,sizeof(U16) * table1_n);
						table1[table1_n-1]=ntohs(s[cur].port); //store the port
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
						table2=(U16 *)realloc((void *)table2,sizeof(U16) * table2_n);
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
			if(lowest<=lob->port_end) {
				plog(f_uru,"Telling lobby to fork a server...\n");
				plNetMsgCustomForkServer(net,lowest,tracks[tut].guid,\
				tracks[tut].age_name,lobby);
			} else {
				plog(f_uru,"Cannot fork server, port range full\n");
			}
			tracks[tut].waiting=1;
			tracks[tut].x=u->hmsg.x;
		} else {
			plog(f_err,"FATAL: No available lobby server found to spawn a game server!\n");
		}
	}
	tracking_dump_players_list(net);
	return 1;
}

int tracking_subsys_serverfound(st_unet * net,int cli,int dsid,int sid) {
	st_uru_client * u=&net->s[sid];
	char address[200];
	plog(f_track,"client:%08X(%s)",tracks[cli].client_ip,get_ip(tracks[cli].client_ip));
	print2log(f_track,",net:%08X(%s)",net->lan_addr,get_ip(net->lan_addr));
	print2log(f_track,",mask:%08X(%s)\n",net->lan_mask,get_ip(net->lan_mask));
	//if((tracks[cli].client_ip == 0x0100007F)) {
		//plog(f_track,"The client is using the loopback address\n");
		//strcpy((char *)address,"127.0.0.1");
	/*} else*/ if((tracks[cli].client_ip & net->lan_mask)==net->lan_addr) {
		//then it's inside the private LAN
		plog(f_track,"The client is inside the local lan, setting %s,%08X\n",\
		get_ip(u->ip),u->ip);
		strcpy((char *)address,get_ip(u->ip));
	} else {
		strcpy((char *)address,(char *)u->acct); //public address host stored in acct
		plog(f_uru,"The client is outside the local lan, setting %s\n",u->acct);
	}
	plNetMsgCustomServerFound(net,(Byte *)address,ntohs(u->port),\
	(Byte *)u->guid,(Byte *)u->name,dsid);
	return 1;
}


//dumps the players list to the filesystem
void tracking_dump_players_list(st_unet * net) {
	int i;
	st_log * f_xml, * f_hstatus, * f_hstatus2;

	if(xml_e==1) {
		f_xml=open_log(xml,2,DF_NODUMP);
	} else {
		f_xml=NULL;
	}
	if(status1_e==1) {
		f_hstatus=open_log(status1,2,DF_NODUMP);
	} else {
		f_hstatus=NULL;
	}
	//if(status2_e==1) {
		//f_hstatus2=open_log(status2,DF_NODUMP);
	//} else {
		f_hstatus2=NULL;
	//}

	//standard output
	if(status1_e==1 || status2_e==1) {
		print2log(f_hstatus,"Last Update: %s<br>",\
		get_stime(net->timestamp,net->microseconds));
		print2log(f_hstatus,"<h2>Current Online Players</h2>");
		print2log(f_hstatus,"<b>Total population: %i</b><br>\n",gt_population);
		print2log(f_hstatus,"<table border=1>");
		print2log(f_hstatus,"<tr><td>Avie</td><td>KI</td><td>Location</td>\
<td>Status</td><td>Debug info</td></tr>");

		gt_population2=0;

		for(i=0; i<n_tracks; i++) {
			if(tracks[i].ki!=0 && tracks[i].sid<(int)net->n) {
				print2log(f_hstatus,"<tr><td>%s</td><td>%i</td><td>&nbsp;%s</td><td>%i \
	%s</td><td>x:%i,f:%i,g:%s,n:%s,w:%i,sid:%i</td></tr>\n",tracks[i].avie,\
				tracks[i].ki,net->s[tracks[i].sid].name,tracks[i].status,\
				unet_get_reason_code(tracks[i].status),tracks[i].x,tracks[i].flag,\
				tracks[i].guid,tracks[i].age_name,tracks[i].waiting,tracks[i].sid);
				gt_population2++;
			} else {
				tracks[i].ki=0;
				/*print2log(f_hstatus,"<tr><td><font color=red>GHOST->(%s)</font></td><td>%i</td>\
	<td>&nbsp;%s</td><td>%i \
	%s</td><td>x:%i,f:%i,g:%s,n:%s,w:%i,sid:%i</td></tr>",tracks[i].avie,\
				tracks[i].ki,"GhostTown",tracks[i].status,\
				unet_get_reason_code(tracks[i].status),tracks[i].x,tracks[i].flag,\
				tracks[i].guid,tracks[i].age_name,tracks[i].waiting,tracks[i].sid);*/
			}
		}

		if(gt_population2!=gt_population) {
			plog(f_err,"Warning population incongruence! %i vs %i!\n",gt_population,gt_population2);
			gt_population=gt_population2;
		}

		print2log(f_hstatus,"</table><br>\n");

		print2log(f_hstatus,"<h2>Current Server Instances</h2>");
		print2log(f_hstatus,"<table border=1>");
		print2log(f_hstatus,"<tr><td>Server</td><td>Guid</td><td>Port</td><td>Ip</td><td>Debug info</td></tr>");

		for(i=0; i<(int)net->n; i++) {
			if(net->s[i].ip!=0) {
				print2log(f_hstatus,"<tr><td>");
				if(strlen((char *)net->s[i].name)<=0) {
					print2log(f_hstatus,"%s","Unknown");
				} else {
					print2log(f_hstatus,"%s",net->s[i].name);
				}
				print2log(f_hstatus,"</td><td>%s</td><td>%i</td><td>%s</td>\
	<td>sid:%i,stamp:%i</td></tr>\n",net->s[i].guid,\
				ntohs(net->s[i].port),get_ip(net->s[i].ip),\
				net->s[i].sid,(int)net->s[i].timestamp);
			}
		}
		print2log(f_hstatus,"</table>\n");
		print2log(f_hstatus,"<br><b>Servers Build info:</b><br>");
		print2log(f_hstatus,"<pre>");
		short_version_info(f_hstatus);
		print2log(f_hstatus,"</pre>");
	}

	//XML Uruvision compatible output
	print2log(f_xml,"<?xml version='1.0'?>\n");
	print2log(f_xml,"<SystemView>\n");
		print2log(f_xml,"<Version>2.0</Version>\n");
		print2log(f_xml,"<Lookup>\n");
			print2log(f_xml,"<Server>\n");
				print2log(f_xml,"<ServerInfo>\n");
					print2log(f_xml,"<Name>Tracking</Name>\n");
					print2log(f_xml,"<Type>7</Type>\n");
					print2log(f_xml,"<Addr>%s</Addr>\n",system_addr);
					print2log(f_xml,"<Port>%i</Port>\n",system_port);
					print2log(f_xml,"<Guid>0000000000000000</Guid>\n");
				print2log(f_xml,"</ServerInfo>\n");
			print2log(f_xml,"</Server>\n");
			print2log(f_xml,"<Agents>\n");
				xml_dump_agents(net,f_xml);
			print2log(f_xml,"</Agents>\n");
		print2log(f_xml,"</Lookup>\n");
	print2log(f_xml,"</SystemView>\n");

	close_log(f_hstatus2);
	close_log(f_hstatus);
	close_log(f_xml);
}

void xml_dump_agents(st_unet * net,st_log * f_xml) {
	int i,e,cur;
	char guid[20];
	for(i=0; i<n_lobbys; i++) {
		print2log(f_xml,"<Agent>\n");
			print2log(f_xml,"<ServerInfo>\n");
				print2log(f_xml,"<Name>Node</Name>\n");
				print2log(f_xml,"<Type>1</Type>\n");
				if(lobbys[i].sid!=-1 && net_check_address(net,lobbys[i].sid)==0) {
					print2log(f_xml,"<Addr>%s</Addr>\n",get_ip(net->s[lobbys[i].sid].ip));
				} else {
					print2log(f_xml,"<Addr>Fake Agent</Addr>\n");
				}
				print2log(f_xml,"<Port>0</Port>\n");
				strcpy(guid,lobbys[i].fakeguid);
				strcpy(guid+14,"00");
				print2log(f_xml,"<Guid>%s</Guid>\n",guid);
			print2log(f_xml,"</ServerInfo>\n");
			if(lobbys[i].sid!=-1 && net_check_address(net,lobbys[i].sid)==0) {
				print2log(f_xml,"<ExternalAddr>%s</ExternalAddr>\n",net->s[lobbys[i].sid].acct);
			} else {
				print2log(f_xml,"<ExternalAddr>Fake Agent</ExternalAddr>\n");
			}
			print2log(f_xml,"<PlayerLimit>-1</PlayerLimit>\n");
			print2log(f_xml,"<GameLimit>-1</GameLimit>\n");
			//the Lobby (if there is one)
			print2log(f_xml,"<Lobby><Process>\n");
				print2log(f_xml,"<Server>\n");
					print2log(f_xml,"<ServerInfo>\n");
						print2log(f_xml,"<Name>Lobby</Name>\n");
						print2log(f_xml,"<Type>2</Type>\n");
						if(lobbys[i].sid!=-1 && net_check_address(net,lobbys[i].sid)==0) {
							print2log(f_xml,"<Addr>%s</Addr>\n",get_ip(net->s[lobbys[i].sid].ip));
							print2log(f_xml,"<Port>%i</Port>\n",ntohs(net->s[lobbys[i].sid].port));
						} else {
							print2log(f_xml,"<Addr>Fake Lobby</Addr>\n");
							print2log(f_xml,"<Port>0</Port>\n");
						}
						print2log(f_xml,"<Guid>%s</Guid>\n",lobbys[i].fakeguid);
					print2log(f_xml,"</ServerInfo>\n");
				print2log(f_xml,"</Server>\n");
				print2log(f_xml,"<Players>\n");
					xml_dump_players(net,f_xml,lobbys[i].sid);
				print2log(f_xml,"</Players>\n");
			print2log(f_xml,"</Process></Lobby>\n");
			print2log(f_xml,"<Games>\n");
			for(e=0; e<lobbys[i].n_childs; e++) {
				cur=lobbys[i].childs[e];
				if(cur!=-1 && net_check_address(net,cur)==0 && net->s[cur].ip!=0) {
					print2log(f_xml,"<Game>\n");
						print2log(f_xml,"<Process>\n");
							print2log(f_xml,"<Server>\n");
								print2log(f_xml,"<ServerInfo>\n");
									print2log(f_xml,"<Name>%s</Name>\n",net->s[cur].name);
									print2log(f_xml,"<Type>3</Type>\n");
									print2log(f_xml,"<Addr>%s</Addr>\n",get_ip(net->s[cur].ip));
									print2log(f_xml,"<Port>%i</Port>\n",ntohs(net->s[cur].port));
									print2log(f_xml,"<Guid>%s</Guid>\n",net->s[cur].guid);
								print2log(f_xml,"</ServerInfo>\n");
							print2log(f_xml,"</Server>\n");
							print2log(f_xml,"<Players>\n");
								xml_dump_players(net,f_xml,cur);
							print2log(f_xml,"</Players>\n");
						print2log(f_xml,"</Process>\n");
						print2log(f_xml,"<AgeLink>\n");
							print2log(f_xml,"<AgeInfo>\n");
								print2log(f_xml,"<AgeFilename>%s</AgeFilename>\n",net->s[cur].name);
								print2log(f_xml,"<AgeInstanceName>%s</AgeInstanceName>\n",net->s[cur].name);
								print2log(f_xml,"<AgeInstanceGuid>%s</AgeInstanceGuid>\n",net->s[cur].guid);
								print2log(f_xml,"<AgeUserDefinedName> </AgeUserDefinedName>\n");
								print2log(f_xml,"<AgeSequenceNumber>0</AgeSequenceNumber>\n");
							print2log(f_xml,"</AgeInfo>\n");
							print2log(f_xml,"<LinkingRules>kOwnedBook</LinkingRules>\n");
							print2log(f_xml,"<SpawnPoint>t:Default,n:LinkInPointDefault,c:(nil)</SpawnPoint>\n");
						print2log(f_xml,"</AgeLink>\n");
						print2log(f_xml,"<State>3</State>\n");
						print2log(f_xml,"<StartTime>0</StartTime>\n");
					print2log(f_xml,"</Game>\n");
				}
			}
			print2log(f_xml,"</Games>\n");
		print2log(f_xml,"</Agent>\n");
	}
}


void xml_dump_players(st_unet * net,st_log * f_xml,int sid) {
	if(sid==-1 || net_check_address(net,sid)!=0) return;
	int i;
	for(i=0; i<n_tracks; i++) {
		if(tracks[i].sid==sid && tracks[i].ki!=0) {
			print2log(f_xml,"<Player>\n");
				print2log(f_xml,"<ClientGuid>\n");
					print2log(f_xml,"<AcctName>%s</AcctName>\n",tracks[i].login);
					print2log(f_xml,"<PlayerID>%i</PlayerID>\n",tracks[i].ki);
					print2log(f_xml,"<PlayerName>%s</PlayerName>\n",tracks[i].avie);
					print2log(f_xml,"<AccountUUID>Not Available</AccountUUID>\n");
					print2log(f_xml,"<SrcAddr>%s</SrcAddr>\n",get_ip(tracks[i].client_ip));
					print2log(f_xml,"<SrcPort>%i</SrcPort>\n",tracks[i].client_port);
					print2log(f_xml,"<BuildType>WhoCares?</BuildType>\n");
					print2log(f_xml,"<CRCLevel>God</CRCLevel>\n");
					print2log(f_xml,"<Protected>Nope</Protected>\n");
				print2log(f_xml,"</ClientGuid>\n");
				print2log(f_xml,"<ProcessGuid>%s</ProcessGuid>\n",tracks[i].guid);
				print2log(f_xml,"<State>%s</State>\n",unet_get_reason_code(tracks[i].status));
			print2log(f_xml,"</Player>\n");
		}
	}
}

int tracking_subsys_directed_fwd(st_unet * net,Byte *buf,int size,int fromki,int sid) {
	// I sure dislike having the same code in multiple files, but alas unet3+
	// is in suspended animation
	
	int offset=0;
	if (size < 9) {
		//messages from game servers shouldn't be mangled
		print2log(f_err,"ERR: forwarded Directed message too short!\n");
		return 1;
	}
	offset+=5;
	S32 bodylen=*(S32 *)(buf+offset);
	offset+=4;
	if (size < offset+bodylen+2) {
		print2log(f_err,"ERR: forwarded Directed message too short!\n");
		return 1;
	}
	offset+=bodylen;
	offset+=1;
	Byte recips=*(Byte *)(buf+offset);
	if (size < offset+(4*recips)) {
		print2log(f_err,"ERR: forwarded Directed message too short!\n");
		return 1;
	}
	offset+=1;

	// if I didn't do it by server first I'd have the same chat "spam" bug
	for(int i=0; i<(int)net->n; i++) {
		if(i!=sid && net->s[i].ip!=0) {
			// it's a server but let's not send messages to people in the lobby
			if(strcmp(((char *)net->s[i].name),"Lobby")) {
				// game server
				int send=0;
				for(int j=0; j<n_tracks; j++) {
					if(tracks[j].ki!=0 && tracks[j].sid==i) {
						// player in said age
						int off=offset;
						for (int k=0; k<recips; k++) {
							S32 recip=*(S32 *)(buf+off);
							off+=4;
							if (recip==tracks[j].ki) {
								send=1;
								break;
							}
						}
					}
					if(send) {
						break;
					}
				}
				if(send) {
					plNetMsgCustomDirectedFwd(net,buf,size,fromki,i);
				}
			}
		}
	} // end of servers
	return 0;
}
