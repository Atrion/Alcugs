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
	this will contain all related with using the socket
*/

#ifndef __U_DEFAULT_MSG_
#define __U_DEFAULT_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_DEFAULT_MSG_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "config_parser.h"
#include "stdebug.h"
#include "conv_funs.h"
#include "protocol.h"
#include "urunet.h"

#include "vaultstrs.h"
#include "urumsg.h"

#include "pdefaultmsg.h"

/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_default_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		dump_packet(f_uru,buf,size,0,7);
		print2log(f_uru,"\n-------------\n");
	}

	//int offset=0;
	int n;
	//int i;

	print2log(f_uru,"<RCV> Uknown Type of packet! %04X\n",u->adv_msg.cmd);
	//---store
	char unk_mes[100];
	static Byte unk_mes_rot=0;
	mkdir("dumps",00750);
	sprintf(unk_mes,"dumps/unk%04X_%02X.raw",u->adv_msg.cmd,unk_mes_rot++);
	savefile((char *)buf,size,unk_mes);
	//---end store
	n=-8;

	return n;
}
#endif

