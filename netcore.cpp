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

/* Build vars - Don't touch - NEVER */
#define __U_NETCORE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdlib.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include "data_types.h"
#include "conv_funs.h"

#include "netcore.h"

#include "debug.h"

/** Unet
*/

alUnet::alUnet() {
	alUnet::create(0,"0.0.0.0");
}

alUnet::alUnet(U16 port,char * hostname,t_unet_flags flags) {
	alUnet::create(port,hostname,flags);
}

alUnet::~alUnet() {
	printf("~alUnet()\n");
	plNetStopOp(&net);
	printf("Netcore succesfully stopped\n");
	log_shutdown();
}

void alUnet::create(U16 port,char * hostname,t_unet_flags flags) {
	ret=0;
	plNetInitStruct(&net);
	net.flags=flags;
	net.unet_sec=0;
	net.unet_usec=1000;
	ret=plNetStartOp(port,hostname,&net);
	if(ret!=UNET_OK) throw "Unet Startup failed!";
}

char * alUnet::GetStrTime() {
	//static char buf[200];
	return (char *)get_stime(net.timestamp,net.microseconds);
}

int alUnet::WaitForMessages() {
	ret=plNetRecv(&net,&sid);
	return ret;
}

int alUnet::IsFini(int isid) {
	return(plNetIsFini(&net,isid));
}

