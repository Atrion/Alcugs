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
const char * ID = "$Id$";
const char * BUILD =  __DATE__ " " __TIME__;
const char * SNAME = "UruPing";
const char * VERSION = "1.4";
/* */

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"


//paranoic dbg level
#define _DBG_PARANOIC

#include <signal.h>

/*Includes */

#include "debug.h" //for debugging
#include "data_types.h" //for data types used in this file
#include "config_parser.h" //for globals
#include "stdebug.h" //for debugging
#include "protocol.h" //all protocol specs
#include "urunet.h" //network functions
#include "useful.h" //useful functions

//global variable that points to all players struct
st_uru_client * all_players;
st_uru_client * auth=NULL; //points to the auth server
st_uru_client * vault=NULL; //points to the vault server
st_uru_client * track=NULL; //poinst to the tracking server
st_uru_client * meta=NULL; 


#include "sdlparser.h"
#include "ageparser.h"

//yes, yes I know, I need to fix this shit

//full sdl headers only
t_sdl_def * global_sdl=NULL; //sdl struct
int global_sdl_n=0; //number of global sdl records
//we need TODO..
//  -- minimized only sdl headers
//  -- dynamic game objects memory struct (read/write it from disk on server start/stop)

t_age_def * global_age_def=NULL; //age struct

int whoami;

#include "urumsg.h" //generic messages

//status
int __state_running=1;

const char * __uru_disclaimer9 = "\n\
    This program is distributed in the hope that it will be useful,\n\
    but WITHOUT ANY WARRANTY; without even the implied warranty of\n\
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n\
    This program has been specially designed for the H'uru server project.\n\
    Use it at your own risk.\n\n\
    You may get some technical/help assistance at COBBS forums:\n\
            http://www.cobbs.ca\n\n\
    \n";


void version() {
	printf("%s %s - Build: %s\n",SNAME,VERSION,BUILD);
}

void disclaimer() {
	printf("%s",__uru_disclaimer9);
}

void parameters_usage() {
	version();
	printf("Usage: uruping server:port [options]\n\n\
 -nl: enabled netcore logs\n\
 -t: set the sleep time in seconds\n\
 -n: set the number of proves, 0=infinite (CTR+C stops), 5 if not set\n\
 -V: show version and end\n\
 -h: Show short help and end\n\
 -v: Set the verbose level\n\
 -lp: Set the local port\n\
 -rp: Set the remote port\n\
 -lh: Set the local host\n\
 -rh: Set the remote host\n\
 -l: Set listenning mode\n\
 -one: Does only one ping probe and displays that value\n\
 -d: Set the destination:\n\
  [1] Agent\n\
  [2] Lobby\n\
  [3] Game\n\
  [4] Vault\n\
  [5] Auth\n\
  [6] Admin\n\
  [7] Lookup\n\
  [8] Client\n\n");
}

/*******************************************************************
   check the parameters
	 i -> position, iterator
	 what -> the paramenter to check "-v", "-V"
	 type -> 0 check only the presence
	         1 is a string, get it
	         2 is an integer, get it
	 result -> if 1, then the pointer to the string is stored here
	 iresult -> if 2, then the pointer to the int is stored here

	 returns 1 if the value is present
	         0 if not
	         -1 on error

******************************************************************/
int parameters_parse(int argc, char * argv[], int i, char * what, char type, int * result) {
	if(!strcmp(argv[i],what)) {
		if(type!=0 && i==argc-1) {
			fprintf(stderr,"ERR: Incorrect number of arguments supplied for %s\n\n",what);
			parameters_usage();
			exit(-1);
			return -1;
		}
		if(type==0) { return 1; }
		if(type==1) { //string
			*result=i+1; //return the interator to that string
			return 1;
		}
		if(type==2 && isdigit(argv[i+1][0])) {
			*result=atoi(argv[i+1]);
			return 1;
		}
		fprintf(stderr,"ERR: Incorrect supplied arguments for %s\n\n",what);
		parameters_usage();
		exit(-1);
		return -1;
	}
	return 0;

}

//handler
void s_handler(int s) {
	if(__state_running==0) {
		if(global_logs_enabled==1) {
			close_log_files();
		}
		exit(-1);
	}
	__state_running=0;
}

/*---------------------------
 Install the handlers
 ------------------------*/
void install_handlers() {
	signal(SIGHUP, s_handler); //reload configuration
	signal(SIGTERM, s_handler); //terminate the server in a elegant way
	signal(SIGINT, s_handler); //terminate the server in a elegant way
	signal(SIGUSR1, s_handler); //Send an emergency "server will be down" message, start a countdown and terminate the server in a elegant way
	signal(SIGUSR2, s_handler); //Send a TERMINATED message to all loged players.
	//signal(SIGSEGV, s_handler);
	//note some things are still NOT implemented ;)
}

int get_host_info(char * argv,char * hostname,char * username,U16 * port) {
	unsigned int i;

	int a=0,b=0,c=0;
	int q=0;

	char left[100]="";
	char mid[100]="";
	char right[100]="";

	for(i=0; i<strlen(argv); i++) {

		if(argv[i]=='@') { q=1; }
		else if(argv[i]==':') { q=2; }
		else {

			switch (q) {
				case 0:
					left[a]=argv[i];
					a++;
					break;
				case 1:
					mid[b]=argv[i];
					b++;
					break;
				case 2:
					right[c]=argv[i];
					c++;
					break;
				default:
					return -1;
			}
		}

	}
	left[a]='\0';
	mid[b]='\0';
	right[c]='\0';

	switch (q) {
		case 0:
			strcpy(hostname,left);
			break;
		case 1:
			strcpy(username,left);
			strcpy(hostname,mid);
			break;
		case 2:
			if(b!=0) {
				strcpy(username,left);
				strcpy(hostname,mid);
			} else {
				strcpy(hostname,left);
			}
			*port=atoi(right);
			break;
	}
	return 1;
}


int main(int argc, char * argv[]) {

	int i,ret;

	int sock; //the socket

	//destination settings...
	char hostname[100]="";
	char username[100]="";
	int port=5000;

	//local settings...
	char l_hostname[100]="0.0.0.0";
	int l_port=0;

	//time
	int t=1;
	int destination=KLobby; //klobby
	//number of probes
	int num=5;
	int count=0;

	//avg latency time
	double avg=0;

	//session id
	int sid=0;
	//msg size
	int size;
	//offset
	int off=0;

	//listenning mode
	int listen=0;

	//for MRTG users
	int mrtg=0;

	st_uru_client * ses=NULL; //all sessions will be here
	st_uru_client * u=NULL; //pointer to the session handler

	//global parameters
	global_max_clients=1;
	global_client_count=1;
	//global_reserved_admin_slots=0;
	global_logs_enabled=0;
	global_verbose_level=0;
	silent=3;

	//parameters
	//if(argc==1) { parameters_usage(); return 0; }

	for (i=1; i<argc; i++) {
		if(parameters_parse(argc,argv,i,"-h",0,NULL)) { parameters_usage(); return -1; }
		else if(parameters_parse(argc,argv,i,"-V",0,NULL)) {
			version();
			disclaimer();
			return -1;
		} else if(parameters_parse(argc,argv,i,"-lp",2,&l_port)) i++;
		else if(parameters_parse(argc,argv,i,"-rp",2,&port)) i++;
		else if(parameters_parse(argc,argv,i,"-l",0,NULL)) {
			listen=1;
		}
		else if(parameters_parse(argc,argv,i,"-nl",0,NULL)) {
			global_logs_enabled=1;
		}
		else if(parameters_parse(argc,argv,i,"-t",2,&t)) i++;
		else if(parameters_parse(argc,argv,i,"-n",2,&num)) i++;
		else if(parameters_parse(argc,argv,i,"-d",2,&destination)) i++;
		else if(parameters_parse(argc,argv,i,"-one",0,NULL)) {
			mrtg=1;
		}
		else if(parameters_parse(argc,argv,i,"-v",2,&ret)) {
			i++;
			global_verbose_level=(Byte)ret;
			switch (global_verbose_level) {
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
		else if(parameters_parse(argc,argv,i,"-lh",1,&ret)) {
			strcpy(l_hostname,argv[ret]); i++;
		}
		else if(parameters_parse(argc,argv,i,"-rh",1,&ret)) {
			strcpy(hostname,argv[ret]); i++;
		}
		else {
			if(i==1) {
				if(get_host_info(argv[1],hostname,username,((U16 *)&port))!=1) {
					parameters_usage();
					return -1;
				}
			} else {
				parameters_usage();
				return -1;
			}
		}
	}
	//end params
	if(global_logs_enabled==1) {
		open_log_files();
	}

	//special mode
	if(mrtg==0) {
		version();
		disclaimer();
	}

	if(t<1) {
		printf("\nCrazy man!, Sorry that time is not allowed, setting to 1 second\n");
		t=1;
	}

	if(mrtg==1) {
		num=1;
	}

	while(listen==0 && !strcmp(hostname,"")) {
		printf("\nHostname not set, please enter destination host: ");
		strcpy(hostname,ask());
	}

	if(listen==0 && mrtg==0) {
	printf("Connecting to %s@%s:%i...\n",username,hostname,port);
	printf("Sending ping probe to %i %s...\n",destination,unet_get_destination(destination));
	}

	install_handlers();
	sock=plNetStartOp(l_port,l_hostname);

	if(sock<0) {
		fprintf(stderr,"Fatal: there was an error trying to bind to the specified address\n");
		return -1;
	}

	//allocate struct
	ses=(st_uru_client *)(malloc(sizeof(st_uru_client) * (global_client_count+1)));
	if(ses==NULL) {
		fprintf(stderr,"Fatal: Insufficient free memory to continue...\n");
		exit(-1);
	}
	DBG(2,"INF: Struct allocated\n");

	//init session vars
	u_init_session(ses,global_client_count);

	u=&ses[1];
	DBG(4,"1a\n")

	//get the host
	if(listen==0 && unet_set_host_info(hostname,port,u)<0) {
		printf("ERR: Invalid hostname specified!\n");
		parameters_usage(); return -1;
	}

	//set validation and version
	u->validation=0x02;
	u->minor_version=6;
	u->major_version=12;
	DBG(4,"3a\n");
	//time structs
	struct timeval tv;
	time_t timestamp;
	double shour;

	//init client & server headers (on udp server is this program & client is the remote host)
	uru_init_header(&u->server,u->validation);
	uru_init_header(&u->client,u->validation);
	DBG(4,"4a\n");

	//initial negotiation
	u->negotiated=0;
if(listen==0) {
	u->flag=1; //solved 234 bugs with this line (is very important) also added a SEGFAULT
	plNetClientComm(sock,u);
}

	DBG(4,"5a\n");
	//set the session initial time
	time(&u->timestamp);
	u->microseconds=get_microseconds();
	DBG(4,"5b\n");
	//and now create the msg
	u->adv_msg.timestamp=u->timestamp;
	u->adv_msg.microseconds=u->microseconds;
	DBG(4,"6a\n");
	//set player id to 0
	u->ki=0;
	u->adv_msg.ki=0;
	u->adv_msg.x=0; //set x
	DBG(4,"7a\n");
	//calculate ping initial time
	time(&timestamp);
	gettimeofday(&tv,NULL);
	shour=(double)(tv.tv_usec)/1000000 + (timestamp);
	DBG(4,"8a\n");
	//start a ping request
	if(listen==0) {
		plNetMsgPing(sock,0,shour,(Byte)destination,u);
	} else {
		printf("Waiting for packets... CTR+C stops\n");
		fflush(0);
		while(__state_running==1) {
			size=plNetRecv(sock,&sid,&ses);
			if(sid!=1 && size>0) {
				printf("Recived a message from another client, and this test only allows one peer\n");
			}
			if(sid==1 && size>0 && (u->client.t==0x02 || u->client.t==0x00)) {
				off=parse_plNet_msg(u->rcv_msg,u);

				if(u->adv_msg.cmd==NetMsgPing) {
					printf("<RCV> Ping... dst:%i %s, t:%0.2f \n",*(Byte *)(u->rcv_msg+off+8),unet_get_destination(*(Byte *)(u->rcv_msg+off+8)),*(double *)(u->rcv_msg+off));
					plNetMsgPing(sock,1,*(double *)(u->rcv_msg+off),*(Byte *)(u->rcv_msg+off+8),u);
					printf("<SND> Pong...\n");
				} else
				if(u->adv_msg.cmd==NetMsgLeave) {
					printf("Sender sent a leave message, ending listenner...\n");
					__state_running=0;
					exit(0);
				}
			}
		}
		printf("Sending terminated message...\n");
		plNetMsgTerminated(sock,RKickedOff,u);
		exit(0);
	}

	while(mrtg==1 && __state_running==1) {
		size=plNetRecv(sock,&sid,&ses);
		if(sid==1 && size>0 && (u->client.t==0x02 || u->client.t==0x00)) {
			off=parse_plNet_msg(u->rcv_msg,u);
			if(u->adv_msg.cmd==NetMsgPing) {
				time(&timestamp);
				gettimeofday(&tv,NULL);
				shour=(double)(tv.tv_usec)/1000000 + (timestamp);
				printf("%0.02f\n",(shour-(*(double *)(u->rcv_msg+off)))*1000);
				plNetMsgLeave(sock,RQuitting,u);
				__state_running=0;
				exit(0);
			} else {
				printf("0\n");
				exit(0);
			}
		}
		if(size==-1) {
				printf("0\n");
				exit(0);
		}
	}

	//for packet control
	int dropped=0;

	DBG(4,"9a\n");
	i=0;
	while((i<num || num==0) && __state_running==1) {
		size=plNetRecv(sock,&sid,&ses);
		if(sid!=1 && size>0) {
			printf("Warning recieving a reply from another host! Host:%s:%i\n",unet_get_str_ip(ses[sid].client_ip),ntohs(ses[sid].client_port));
		}

		if(sid==1 && size>0 && (u->client.t==0x02 || u->client.t==0x00)) {
			off=parse_plNet_msg(u->rcv_msg,u);

			if(u->adv_msg.cmd==NetMsgPing) {
				time(&timestamp);
				gettimeofday(&tv,NULL);
				shour=(double)(tv.tv_usec)/1000000 + (timestamp);
				printf("The server is alive. %0.02f ms\n",(shour-(*(double *)(u->rcv_msg+off)))*1000);
				avg+=(shour-(*(double *)(u->rcv_msg+off)))*1000;
				count++;
				//another request
				//set version
				//u->minor_version=6;
				//u->major_version=12;
				if(i<count) {
					sleep(t);
					time(&timestamp);
					gettimeofday(&tv,NULL);
					shour=(double)(tv.tv_usec)/1000000 + (timestamp);
					plNetMsgPing(sock,0,shour,(Byte)destination,u);
					i++;
				} else {
					dropped++;
				}
			} else if(u->adv_msg.cmd==NetMsgTerminated) {
				printf("The server has send a terminated message. Reason %i %s\n",*(Byte *)(u->rcv_msg+off),unet_get_reason_code(*(Byte *)(u->rcv_msg+off)));
				__state_running=0;
			} else {
				printf("Recieved an unknown message from server %04X\n",u->adv_msg.cmd);
			}
		}
		if(size==-1) {
			printf("Timeout...\n");
			//another request
			time(&timestamp);
			gettimeofday(&tv,NULL);
			shour=(double)(tv.tv_usec)/1000000 + (timestamp);
			//set version
			//u->minor_version=6;
			//u->major_version=12;
			sleep(t);
			time(&timestamp);
			gettimeofday(&tv,NULL);
			shour=(double)(tv.tv_usec)/1000000 + (timestamp);
			plNetMsgPing(sock,0,shour,(Byte)destination,u);
			i++;
			//exit(0);
		}
	}
	printf("Leaving...\n");
	plNetMsgLeave(sock,RQuitting,u);

	int l=0;

	while(l<3 && !plNetCheckAckDone(u)) {
		size=plNetRecv(sock,&sid,&ses);
		if(size==-1) {
			l++;
			//printf("l is now %i\n",l);
		}
	}
	if(l>=3) {
		printf("There was a problem disconnecting your session\n");
	}

	printf("I have sent %i packets, I have recieved %i, %i packets were dropped, %0.2f%% packet loss, average %0.2f ms\n",i,count,dropped,((float)(i-(count-dropped))*100)/(float)i,avg/count);

	if(global_logs_enabled==1) {
		close_log_files();
	}

	return 0;
}

