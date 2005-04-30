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

#ifndef __U_META_MSG_
#define __U_META_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_META_MSG_ID "$Id$"

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

#include "meta_db.h"

#include "uru.h"

#include "psmetamsg.h"


int do_meta_idle_operation(int sock) {
	int i,n;
	t_meta_servers * servers;
	servers=NULL;
	int sid;
	double mtime;

	struct timeval tv;
	time_t timestamp;

	#if 0
	for(i=global_client_count; i>global_the_bar; i--) {
		if(plNetCheckAckDone(&all_players[i])!=1) {

		}
	}
	#endif

	n=plMetaGetServers(&servers);

	for(i=0; i<n; i++) {
		if(plNetConnect(sock,&sid,&all_players,servers->address,5000)==0) {
			all_players[sid].x=servers->id;
			all_players[sid].major_version=12;
			all_players[sid].minor_version=7;
			all_players[sid].adv_msg.max_version=12;
			all_players[sid].adv_msg.min_version=7;
			time(&timestamp);
			gettimeofday(&tv,NULL);
			mtime=(double)(tv.tv_usec)/1000000 + (timestamp);
			plNetMsgCustomMetaPing(sock,1,mtime,KLobby,0,&all_players[sid]);
		}
	}

	if(servers!=NULL) {
		free((void *)servers);
	}

	return 0;
}


/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_smeta_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		dump_packet(f_uru,buf,size,0,7);
		print2log(f_uru,"\n-------------\n");
	}

	int offset=0;
	int n;

	//int rcode;

	//THE BIG VERY VERY BIG Switch
	switch (u->adv_msg.cmd) {
		case NetMsgCustomMetaPing:
			struct timeval tv;
			time_t timestamp;
			double shour;
			double mtime;
			Byte destination;
			U32 pop,ver;
			print2log(f_uru,"<RCV> CustomMetaPing ");
			mtime=*(double *)(buf+offset); //8 bytes (double)
			offset+=0x08;
			destination=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru," t:%e dst:%i\n",mtime,destination);
			ver=*(U32 *)(buf+offset);
			offset+=4;
			if(ver!=1) { n=0; u->client_ip=0; break; }
			pop=*(U32 *)(buf+offset);
			offset+=4;

			time(&timestamp);
			gettimeofday(&tv,NULL);
			shour=(double)(tv.tv_usec)/1000000 + (timestamp);

			plMetaUpdateServer(u->x,(Byte *)get_ip(u->client_ip),1,shour-mtime,pop);
			u->client_ip=0; //free up session
			n=0;
			break;
		case NetMsgCustomMetaRegister:
			print2log(f_uru,"<RCV> NetMsgCustomMetaRegister \n");
			U32 cmd,dataset; //pop,ver
			Byte address[100];
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
				u->client_ip=0; //free up session
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

			if(cmd!=1) {
				cmd=0; //set unregister if unknown
				plMetaUpdateServer(u->x,(Byte *)get_ip(u->client_ip),-1,0,0);
				n=0;
				u->client_ip=0; //free up session
				break;
			}

			struct hostent *host;
			host=gethostbyname((char *)address);
			if(host==NULL) {
				print2log(f_uru,"INFO: Cannot resolve host %s\n",address);
				n=0; //cannot resolve, abandon
				u->client_ip=0; //free up session
				break;
			}
			if(u->client_ip!=*(U32 *)host->h_addr_list[0]) {
				print2log(f_uru,"INFO: Ip mismatch!\n");
				n=0; //abandon
				u->client_ip=0; //free up session
				break;
			}

			plMetaAddServer((Byte *)get_ip(u->client_ip),address,name,desc,contact,site,dataset,passwd);

			u->client_ip=0; //free up session
			n=0;
			break;

		default:
			n=-7;
			break;
	}

	logflush(f_uru);

	return n;
}
#endif

