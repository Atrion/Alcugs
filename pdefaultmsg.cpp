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

/*
	Basic messages parser
*/

/* CVS tag - DON'T TOUCH*/
#define __U_PDEFAULT_MSG_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef __WIN32__
#  include "windoze.h"
#endif

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

#include "files.h"

#include "uru.h"
#include "config_parser.h"

#include "pdefaultmsg.h"

#include "debug.h"

/**
	processes basic plNetMsg's
*/
int process_default_plNetMsg(st_unet * net,Byte * buf,int size,int sid) {
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

	int n=0;

	print2log(f_uru,"<RCV> Uknown Type of packet! %04X\n",u->hmsg.cmd);
	nlog(f_une,net,sid,"Unimplemented message %04X %s recieved!\n",u->hmsg.cmd,unet_get_msg_code(u->hmsg.cmd));
	//---store
	static Byte unk_mes_rot=0;
	if(u->authenticated==1) {
		dump_msg("unk",u->hmsg.cmd,unk_mes_rot++,buf,size);
	}
	//---end store
	n=-3;

	return n;
}


void dump_msg(char * name,U16 cmd,Byte rot,Byte * buf,int size) {
	static char dumps_path[200];
	static char dumps_path2[300];
	sprintf(dumps_path,"%s/dumps",stdebug_config->path);
	mkdir(dumps_path,00750);
	sprintf(dumps_path2,"%s/%s%04X_%02X.raw",dumps_path,name,cmd,rot);
	savefile((char *)buf,size,dumps_path2);
}

