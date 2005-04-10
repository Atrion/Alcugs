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

/* Don't touch - NEVER */
const char * ID = "$Id: uruping.cpp,v 1.6 2004/12/01 10:17:06 almlys Exp $";
const char * BUILD =  __DATE__ " " __TIME__;
const char * SNAME = "UruTestingSuit";
const char * VERSION = "1.2b";
/* */

#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#ifdef __WIN32__
#include <windows.h>
#include "windoze.h"
#endif

#include "license.h"
#include "version.h"

#include "data_types.h" //for data types used in this file
#include "stdebug.h" //for debugging

/* netcore */
#include "urunet.h" //network functions
#include "protocol.h" //protocol stuff
#include "prot.h"

#include "useful.h"
#include "files.h"

/* msg generator */
#include "gbasicmsg.h" //basic msg's

#include "debug.h"


//check admin?
#define __VTC 0

//netcore status
int __state_running=1;

void parameters_usage() {
	version(stdout);
	printf("Usage: urumsgtest peer:port [options]\n\n\
 -file x: Set the file to upload\n\
 -val x: set validation level (0-3) (default 2)\n\
 -nl: enable netcore logs\n\
 -V: show version and end\n\
 -h: Show short help and end\n\
 -v x: Set the verbose level\n\
 -lp x: Set the local port\n\
 -rp x: Set the remote port\n\
 -lh x: Set the local host\n\
 -rh x: Set the remote host\n\
 -lm: Set listenning mode\n\n");
}

//handler
void s_handler(int s) {
	plog(f_uru,"INF: Catch up signal %i\n",s);
	if(__state_running==0) {
		plog(f_err,"killed\n");
		log_shutdown();
		exit(-1);
	}
	__state_running=0;
}

/*---------------------------
 Install the handlers
 ------------------------*/
void install_handlers() {
	signal(SIGTERM, s_handler);
	signal(SIGINT, s_handler);
}

int main(int argc, char * argv[]) {

	int ret,i,sid,isid; //sid session identifier
	st_unet net;

	Byte silent=3; //0; //3

	//destination settings...
	char hostname[100]="";
	char username[100]="";
	char avie[100]="";
	int port=5000;

	char file[500]="";

	int val=2; //validation level
	char flags=0;

	//local settings...
	char l_hostname[100]="0.0.0.0";
	int l_port=0;

	int size, off=0; //msg size & offset
	Byte * msg=NULL; //pointer to the message
	int listen=0; //listenning mode

	plNetInitStruct(&net);

	net.flags &= (~UNET_ELOG & ~UNET_FLOG); //disable logging
	//if(license_check(stdout,argc,argv)) exit(0);

	//parse parameters
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			version(stdout);
			//show_disclaimer();
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-lm")) { listen=1; }
		else if(!strcmp(argv[i],"-nl")) { net.flags |= UNET_FLOG | UNET_ELOG; /* enable logging */ }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-v") && argc>i+1) { i++; ret=atoi(argv[i]);
			//stdebug_config->silent=
/*			global_verbose_level=(Byte)ret; */
			switch (ret) {
				case 0:
					silent=3;
					break;
				case 1:
					silent=2;
					break;
				case 2:
					silent=1;
					break;
				default:
					silent=0;
			}
		}
		else if(!strcmp(argv[i],"-file") && argc>i+1) {
			i++;
			strcpy(file,argv[i]);
		}
		else if(!strcmp(argv[i],"-lh") && argc>i+1) {
			i++;
			strcpy(l_hostname,argv[i]);
		}
		else if(!strcmp(argv[i],"-l")) {
			version(stdout);
			show_bigdisclaimer();
			exit(0);
		}
		else if(!strcmp(argv[i],"-rh") && argc>i+1) {
			i++;
			strcpy(hostname,argv[i]);
		}
		else {
			if(i==1) {
				if(get_host_info(argv[1],hostname,username,((U16 *)&port),avie)!=1) {
					parameters_usage();
					return -1;
				}
			} else {
				parameters_usage();
				return -1;
			}
		}
	}

	switch(val) {
		case 0:
			flags |=UNET_VAL0;
			break;
		case 1:
			flags |=UNET_VAL1;
			break;
		case 2:
			//default
			flags |=UNET_VAL2;
			break;
		case 3:
			flags |=UNET_VAL3; // 0x02 | 0x01
			break;
		default:
			printf("\nError: Unimplemented validation level %i requested.\n",val);
			exit(0);
	}

	if(listen==0 && !strcmp(file,"")) {
		printf("No input file specified!\n");
		exit(0);
	}

	//end params
	log_init(); //automatically started by the netcore, but the log_shutdown call is mandatory
	stdebug_config->silent=silent; //log_init() is a must before this call
	//set other stdebug_config params __here__

	//set other net params here

	if(silent!=3) {
		net.flags |= UNET_ELOG;
	}

	while(listen==0 && !strcmp(hostname,"")) {
		printf("\nHostname not set, please enter destination host: ");
		strcpy(hostname,ask());
	}

	if(listen==0) {
		printf("Connecting to %s#%s@%s:%i...\n",username,avie,hostname,port);
		printf("Sending file...\n");
	}

	install_handlers(); /* Set up the signal handlers */

	ret=plNetStartOp(l_port,l_hostname,&net);

	DBG(2,"plNetStartOp res:%i\n",ret);

	if(ret!=UNET_OK) {
		fprintf(stderr,"Urunet startup failed, with return code n: %i!\n",ret);
		exit(-1);
	}

	if(listen==0) {
		net.whoami = KClient;

		if(plNetConnect(&net,&sid,hostname,port,flags)!=UNET_OK || sid<0) {
			fprintf(stderr,"ERR: Failed connection to %s:%i\n",hostname,port);
			return -1;
		}
		isid=sid;

		net.s[sid].whoami = KLobby;
		//send the message here
		//TODO sendmsghere
		size=readfile((char **)&msg, file);

		net.s[sid].hmsg.cmd=0x1313;
		net.s[sid].hmsg.flags=0x00000000 | plNetAck | plNetTimestamp;

		plNetSendMsg(&net,msg,size,isid,0);

		if(msg!=NULL) { free((void *)msg); }
		__state_running=0; //se fini

	} else { //listenning mode
		net.whoami = KLobby;
		printf("Waiting for messages... CTR+C stops\n");
		fflush(0);
	}

	DBG(3,"__state_running is %i\n",__state_running);
	while(__state_running) { // && (listen!=0 || num==0)) { //rcvn<num ||
		ret=plNetRecv(&net,&sid);
		DBG(2,"plNetRcv event:%i from peer:%i\n",ret,sid);

		if(sid>=0) {

			switch(ret) {
				case UNET_NEWCONN: //this event never happens, but just in case
				case UNET_MSGRCV:
				case UNET_FLOOD:
					DBG(3,"ret:%i peer:%i, new message from %s:%i\n",ret,sid,get_ip(net.s[sid].ip),htons(net.s[sid].port));
					DBG(5,"now I'm going to parse the message...\n");
					size=plNetGetMsg(&net,sid,&msg); //get the message
					DBG(5,"Got a message from sid:%i of %i bytes\n",sid,size);
					//parse the message
					#if _DBG_LEVEL_ > 3
					dumpbuf(f_uru,msg,size);
					lognl(f_uru);
					#endif
					off=parse_plNet_msg(&net,msg,size,sid); //parse the header

					//TODO store here the msg
					DBG(5,"I'm going to store a message of %i bytes\n",size);
					savefile((char *)msg+off,size-off, "out.raw");

					if(msg!=NULL) free((void *)msg); //<- vip
					DBG(5,"the message has been succesfully parsed...\n");
					break;
				case UNET_TIMEOUT:
				case UNET_TERMINATED: //this event is not implemented, yet. It must be handled in the app layer
					DBG(3,"ret:%i peer:%i, connection terminated/timeout from %s:%i\n",ret,sid,get_ip(net.s[sid].ip),htons(net.s[sid].port));
					if(listen!=0) { //Only on listen mode
						printf("Timeout from %s:%i\n",get_ip(net.s[sid].ip),htons(net.s[sid].port));
						//plNetMsgTerminated(&net,RTimedOut,sid);
						plNetDestroySession(&net,sid);
					}
					break;
				default: //ignore the other events
					DBG(4,"ret:%i peer:%i, address:%s:%i, error:%s\n",ret,sid,get_ip(net.s[sid].ip),htons(net.s[sid].port),get_unet_error(ret));
					break;
			}
		} else { //ignore the generic events
			DBG(4,"ret:%i error:%s\n",ret,get_unet_error(ret));
		}
	} //while (main loop)

	if(listen==0) {

		double rcv,current;

		rcv = get_current_time();
		while(plNetIsFini(&net,isid)==0) {
			ret=plNetRecv(&net,&sid);
			DBG(2,"plNetRcv event:%i from peer:%i\n",ret,sid);
			current = get_current_time();
			if((current-rcv) > 20) {
				printf("\nQuitting after a timeout of 20 seconds...\n");
				break;
			}
		}
	}

	plNetStopOp(&net);
	DBG(3,"INF: Service sanely terminated\n");
	log_shutdown();

	return 0;
}

