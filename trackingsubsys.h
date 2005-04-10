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

#ifndef __U_TRACKINGSUBSYS_H_
#define __U_TRACKINGSUBSYS_H_
/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGSUBSYS_H_ID "$Id: psauthmsg_internal.cpp,v 1.1 2004/10/24 11:20:27 almlys Exp $"

#include "ageparser.h"

//!Tracking
typedef struct {
	Byte uid[17];
	int x; //client X
	int ki; //the client //players ki number, unical uid
	//link with _home_ server peer
	int sid; //the unique session identifier
	U32 ip; //the server session, where this player is
	U16 port; //the server session, where this player is
	//end link
	Byte flag; // 0-> delete, 1-> set invisible, 2-> set visible, 3-> set only buddies
	Byte status; //RStopResponding 0x00, 0x01 ok, RInroute 0x16, RArriving 0x17, RJoining 0x18, RLeaving 0x19, RQuitting 0x1A
	Byte avie[200];
	Byte login[200];
	Byte guid[31]; //Age guid where the player is
	Byte age_name[101]; //Age name where the player is
	Byte waiting; //Waiting var (1 if is waiting to the FindAgeReply, elsewhere nothing...)
	U32 client_ip; //clients ip address
	U16 client_port; //clients port address
} st_tracking;

#define TR_FORK 0x01 //game forking allowed
#define TR_FAKE 0x02 //fake lobby
#define TR_PROT 0x04 //protected

typedef struct {
	//link with session identifier
	int sid; //sid link
	Byte flags; //flags (0x01 forking allowed on peer, 0x02 fake lobby, 0x04 protected lobby)
	//end link
	U16 port_start; //first port
	U16 port_end; //last port
	int * childs; //child peers
	int n_childs; //number of child peers
	char fakeguid[30]; //just a fake random guid for uruvision
} st_track_peer;


int init_tracking_subsys();
void stop_tracking_subsys();
void reload_tracking_subsys();
void update_tracking_subsys();
void tracking_subsys_idle_operation(st_unet * net);

int tracking_subsys_update(st_unet * net,int ki,char * guid,char * acct,char * avie,Byte flag,Byte status,int sid);
int tracking_subsys_addgame(st_unet * net,int sid);
int tracking_notify_waiting_players(st_unet * net, int sid);
int tracking_subsys_findserver(st_unet * net,char * guid,char * name,U32 cip,int sid);
int tracking_subsys_serverfound(st_unet * net,int cli,int dsid,int sid);

void tracking_dump_players_list(st_unet * net);

void xml_dump_players(st_unet * net,st_log * f_xml,int sid);
void xml_dump_agents(st_unet * net,st_log * f_xml);

extern int tracking_initialitzed;

extern char * global_private_ages;
extern int global_muinstance_mode;

extern t_age_def * global_age_def;
extern int global_age_def_n;

#endif

