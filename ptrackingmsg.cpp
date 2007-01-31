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
#include <string.h>

#ifndef __MSVC__
#  include <unistd.h>
#endif

#include "data_types.h"
#include "stdebug.h"
#include "urunet.h"
#include "protocol.h"
#include "prot.h"

#include "conv_funs.h"

//#include "uru.h"
//#include "config_parser.h"
#include "lobbysubsys.h"

//#include "gbasicmsg.h"
#include "globbymsg.h"

#include "ptrackingmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_tracking_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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
		case NetMsgCustomServerFound:
			int port;
			Byte address[200];
			Byte guid[21];
			Byte age_fname[100];
			port=*(U16 *)(buf+off);
			off+=2;
			off+=decode_urustring(address,buf+off,199);
			off+=2;
			off+=decode_urustring(guid,buf+off,20);
			off+=2;
			off+=decode_urustring(age_fname,buf+off,99);
			off+=2;
			print2log(f_uru,"<RCV> NetMsgCustomServerFound for %i, %s:%i,%s,%s\n",u->hmsg.ki,address,port,guid,age_fname);

			//Route the packet to the addient client
			//search by KI
			dsid=plNetClientSearchByKI(net,u->hmsg.ki);
			if(dsid>=0) {
				net->s[dsid].hmsg.x=u->hmsg.x; //set the X
				plNetMsgFindAgeReply(net,age_fname,address,port,guid,dsid);
			} else {
				print2log(f_uru,"Player not found!, to send the FindAgeReply!!\n");
			}
			n=1;
			break;
		case NetMsgCustomForkServer:
			print2log(f_uru,"<RCV> NetMsgCustomForkServer ");
			U16 f_port;
			Byte loadstate;
			char f_strport[7];
			char f_guid[20];
			char f_name[100];
			char f_logs[300];
			if(load_on_demand==0 || net->whoami!=KLobby) {  //disallow forking
				n=1;
				print2log(f_uru," ignored, because I'm in manual mode, or I'm not a Lobby\n");
				break;
			}
			f_port=*(U16 *)(buf+off);
			off+=2;
			sprintf(f_strport,"%i",f_port);
			off+=decode_urustring((Byte *)f_guid,buf+off,19);
			off+=2;
			off+=decode_urustring((Byte *)f_name,buf+off,99);
			off+=2;
			loadstate = *(buf+off);
			off++;
			//Never trust the user input!!
			str_filter((Byte *)f_guid);
			str_filter((Byte *)f_name);
			if(strlen(f_guid)!=16 || strlen(f_name)<=0) {
				plog(f_err,"Invalid guid or age filename! (but is not tracking already checking this?)\n");
				n=1;
			} else {
				sprintf(f_logs,"%s/%s/%s/",globby_game_log_path,f_name,f_guid);
				//do the forking here
				int pid;
#ifdef __WIN32__
				pid=1;
				int ret=0;
				char wthatb[200],params[300];
				sprintf(wthatb,"%suru_game.exe",globby_bin);
				sprintf(params,"-p %s -guid %s -name %s -log %s -c %s%s",f_strport,f_guid,\
				f_name,f_logs,globby_game_config,loadstate?" -L":"");
				ret=(int)ShellExecute(0,"open",wthatb,params,0,SW_SHOW);
				plog(f_err,"ShellExecute %s %s with ret:%i\n",wthatb,params,ret);
				//and (I hate windows)
				//plog(f_err,"ERROR_FILE_NOT_FOUND %i\n",ERROR_FILE_NOT_FOUND);
#else
				pid=fork();
				if(pid==0) { //we are the new server process
					close(net->sock); //close the socket (or we will not be able to restart the lobby without stopping all game servers)
					log_shutdown_silent();
					//TODO close and destroy anything else that we forgot to destroy.

					char wthatb[200];
					//allow blank binary path
					sprintf(wthatb,"%suru_game",globby_bin);
					if(loadstate) {
						execlp(wthatb,f_name,"-p",f_strport,"-guid",f_guid,"-name",f_name,\
									 "-log",f_logs,"-c",globby_game_config,"-L",NULL);
					}
					else {
						execlp(wthatb,f_name,"-p",f_strport,"-guid",f_guid,"-name",f_name,\
									 "-log",f_logs,"-c",globby_game_config,NULL);
					}
					//on error, dump an error message
					log_init();
					//warning here..
					stdebug_config->path=f_logs;
					st_log * fork_err=NULL;
					fork_err=open_log("fork_err.log",2,DF_APPEND);
					plog(fork_err,"Fatal, failed forking %s [%s] on port %i\n",f_name,f_guid,f_port);
					logerr(fork_err,"execlp()");
					log_shutdown();
					exit(-1);
				}
#endif
				if(pid<0) {
					print2log(f_err,"ERR: Cannot fork server %s [%s] at port %i\n",f_name,f_guid,f_port);
					lognl(f_uru);
				} else {
					print2log(f_uru," server %s [%s] forked at port %i\n",f_name,f_guid,f_port);
				}
				n=1;
			}
			break;
		default:
			n=UNET_OK;
			break;
	}
	return n;
}

