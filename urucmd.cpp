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

/*******************************************************************************
* Uru Command line                                                             *
*******************************************************************************/

/* Don't touch - NEVER */
const char * ID = "$Id$";
const char * BUILD =  __DATE__ " " __TIME__;
const char * SNAME = "UruCmd";
const char * VERSION = "1.5a";
/* */

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#if 0 //tbd
#include <signal.h>
//for threads
#include <pthread.h>
//for semaphores
#include <semaphore.h>
/*
//for semaphores
#include <sys/types.h>
#include <sys/ipc.h>
#inlcude <sys/sem.h>
*/
#endif

#ifdef __WIN32__
#  include "windoze.h"
#endif

#ifndef __MSVC__
#  include <unistd.h>
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

/* msg generator's */
#include "gbasicmsg.h" //basic msg's

/* msg parser's */
//#include "pbasicmsg.h" //basic server msg's

#include "debug.h"

//check admin?
#define __VTC 0

#if 0
//netcore status
int __state_running=1;

//status
volatile int __state_running=1;
volatile int __netcore_running=1;
volatile int __nths=1;

//semaphore
sem_t msg_sem; //is there a message?
sem_t pmsg_sem; //has been the message processed?
#endif


void parameters_usage() {
	version(stdout);
	printf("Usage: urucmd user#avie@server:port [options]\n\n\
 -l: Show license info\n\
 -V: show version and end\n\
 -h: Show short help and end\n\
 -v: Set the verbose level\n\
 -lp: Set the local port\n\
 -lr: Set the remote port\n\
 -lh: Set the local host\n\
 -rh: Set the remote host\n\n\
 -nl: Enable netcore logs\n");
}

#if 0
void install_handlers();
//handler
void s_handler(int s) {
	DBG(2,"signal %i recieved\n",s);
	DBG(3,"__state_running is %i\n",__state_running);
	if(__state_running==0 && __nths<=0) {
		if(global_logs_enabled==1) {
			close_log_files();
		}
		exit(-1);
	}
	__state_running=0;
	__netcore_running=0;
	__nths--;
	sem_post(&pmsg_sem);
	install_handlers();
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
#endif

#if 0
void host_error() {
	fprintf(stderr,"There was a problem trying to connect to the specified host! Host down?");
	exit(-1);
}

void netcore() {
	int size;
	int sid;

	st_uru_client * u;

	u=&ses[1];

	int off=0;
	int ret;

	//netcore loop
	while(__netcore_running==1) {
		//recieve messages
		size=plNetRecv(sock,&sid,&ses);
		if(sid!=1 && size>0) {
			printf("Warning recieving a reply from another host! Host:%s:%i\n",unet_get_str_ip(ses[sid].client_ip),ntohs(ses[sid].client_port));
		}
		else
		if(sid==1 && size>0 && (u->client.t==0x02 || u->client.t==0x00)) {
			DBG(4,"calling plNet_msg parser...\n");
			off=parse_plNet_msg(u->rcv_msg,u);
			DBG(4,"end plNet_msg parser call, now calling process_client_plNetMSG...\n");
			//do stuff here
			ret=process_client_plNetMsg(sock,u->rcv_msg+off,size-off,u);
			switch(u->adv_msg.cmd) {
				case NetMsgTerminated:
					printf("Recieved a terminated message. Reason (%i) %s\n",u->adv_msg.reason,unet_get_reason_code(u->adv_msg.reason));
					__state_running=0;
					__netcore_running=0;
					break;
				case NetMsgVaultPlayerList:
							DBG(2,"number of players is %i\n",u->p_num);
					sem_post(&msg_sem); //now the other thread may continue
								DBG(2,"number of players is %i\n",u->p_num);
					sem_wait(&pmsg_sem); //wait the other thread to process the msg
								DBG(2,"number of players is %i\n",u->p_num);
					break;
				default:
					printf("Recieved an unknow message from server %04X\n",u->adv_msg.cmd);
			}
		}
		else
		if(size==-1) {
			//nothing, it's a simple timeout
			DBG(3,"idle...");
			if(u->authenticated==1) {
				plNetMsgAlive(sock,u);
			}
		}
	}
	//alliberate resources
	sem_post(&msg_sem);
}

void print_player_menu(U16 n,st_vault_player * p) {
	int i;
	DBG(5,"printing player menu\n");
	for(i=0;i<n;i++) {
		DBG(5,"printing player %i\n",i);
		printf("[%i] %s (ki:%i,flags:%02X)\n",i+1,p[i].avatar,p[i].ki,p[i].flags);
	}
	DBG(5,"end the menu printing..\n");
}
#endif

int main(int argc, char * argv[]) {
	int ret,i,sid,isid; //sid session identifier
	st_unet net;

	Byte silent=3; //0; //3

	//destination settings...
	char hostname[100]="";
	char username[100]="";
	char password_hash[30]="";
	char password[100]="";
	char avie[100]="";
	int port=5000;

	int val=2; //validation level

	//local settings...
	char l_hostname[100]="0.0.0.0";
	int l_port=0;

	int size, off=0; //msg size & offset
	Byte * msg=NULL; //pointer to the message

	Byte flags=0;

	plNetInitStruct(&net);

	net.flags &= (~UNET_ELOG & ~UNET_FLOG); //disable logging

	//parse parameters
	for (i=1; i<argc; i++) {
		if(!strcmp(argv[i],"-h")) { parameters_usage(); return -1; }
		else if(!strcmp(argv[i],"-V")) {
			version(stdout);
			//show_disclaimer();
			return -1;
		} else if(!strcmp(argv[i],"-lp") && argc>i+1) { i++; l_port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-rp") && argc>i+1) { i++; port=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-nl")) { net.flags |= UNET_FLOG | UNET_ELOG; /* enable logging */ }
		else if(!strcmp(argv[i],"-val") && argc>i+1) { i++; val=atoi(argv[i]); }
		else if(!strcmp(argv[i],"-v") && argc>i+1) { i++; ret=atoi(argv[i]);
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

	version(stdout);

	//kk
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

	//end params
	log_init(); //automatically started by the netcore, but the log_shutdown call is mandatory
	stdebug_config->silent=silent; //log_init() is a must before this call
	//set other stdebug_config params __here__

	//set other net params here
	if(bcast==1) { //set broadcast address
		net.flags |= UNET_BCAST;
	}

	if(silent!=3) {
		net.flags |= UNET_ELOG;
	}
	
	printf("Please be sure that you are using the latest version!\n");

	while(!strcmp(hostname,"")) {
		printf("\nPlease enter destination host/ip: ");
		strcpy(hostname,ask());
	}

	while(!strcmp(username,"")) {
		printf("\nPlease enter your username: ");
		strcpy(username,ask());
	}

	strcpy((char *)password,getpass("Please enter your password: "));
	MD5((Byte *)password,strlen((const char *)password),(Byte *)password_hash);
	memset((char *)password,0,strlen((const char *)password)
	hex2ascii2((Byte *)password,(Byte *)password_hash,16);
	
	printf("Connecting to %s#%s@%s:%i...\n",username,avie,hostname,port);

	//install_handlers(); /* Set up the signal handlers */

	ret=plNetStartOp(l_port,l_hostname,&net);

	DBG(2,"plNetStartOp res:%i\n",ret);

	if(ret!=UNET_OK) {
		fprintf(stderr,"Urunet startup failed, with return code n: %i!\n",ret);
		exit(-1);
	}

	#if 0
	//create semaphores
	sem_init(&msg_sem,0,0);
	sem_init(&pmsg_sem,0,0);
	#endif

// ---- start stuff here
	printf("Authenticating...\n");
	ret=plNetAuthenticateHello(sock,(Byte *)username,u);
	if(ret<0) {
		host_error();
	}

	//end login here

	int tryes=0;

	int auth_flag=0;

	//login loop
	while(__state_running==1 && u->authenticated==0 && tryes<6) {
		//recieve messages
		size=plNetRecv(sock,&sid,&ses);
		if(sid!=1 && size>0) {
			printf("Warning recieving a reply from another host! Host:%s:%i\n",unet_get_str_ip(ses[sid].client_ip),ntohs(ses[sid].client_port));
		}
		else
		if(sid==1 && size>0 && (u->client.t==0x02 || u->client.t==0x00)) {
			DBG(4,"calling plNet_msg parser...\n");
			off=parse_plNet_msg(u->rcv_msg,u);
			DBG(4,"end plNet_msg parser call, now calling process_client_plNetMSG...\n");
			//do stuff here
			ret=process_client_plNetMsg(sock,u->rcv_msg+off,size-off,u);
			switch(u->adv_msg.cmd) {
				case NetMsgTerminated:
					printf("Recieved a terminated message. Reason (%i) %s\n",u->adv_msg.reason,unet_get_reason_code(u->adv_msg.reason));
					__state_running=0;
					break;
				case NetMsgAuthenticateChallenge:
					auth_flag=1; //set auth flag
					if(ret<0) {
						switch(ret) {
							case -2:
								printf("ERR: Protocol version mismatch, servers are older\n");
								__state_running=0;
								break;
							case -3:
								printf("ERR: Protocol version mismatch, servers are newer\n");
								__state_running=0;
								break;
							default:
								printf("ERR: Server code %i %s \n",u->adv_msg.reason,unet_get_auth_code(u->adv_msg.reason));
								__state_running=0;
								break;
						}
					}
					break;
				case NetMsgAuthenticateResponse:
					break; //do nothing the parser does the job
				case NetMsgAccountAuthenticated:
					if(ret<0) {
						__state_running=0;
						switch(u->adv_msg.reason) {
							case AProtocolOlder:
								printf("ERR: Protocol version mismatch, servers are older\n");
								break;
							case AProtocolNewer:
								printf("ERR: Protocol version mismatch, servers are newer\n");
								break;
							case AAccountExpired:
								printf("Notice: Your account has expired! ( ?:? )\n");
								break;
							case AAccountDisabled:
								printf("Notice: Your account is dissabled\n");
								break;
							case AInvalidPasswd:
							case AInvalidUser:
								printf("ERR: The entered Username/Password combination is invalid\n");
								break;
							default:
								printf("ERR: Unespicified server error code %i\n",u->adv_msg.reason);
								break;
						}
					}
					break;
				default:
					printf("ERR: Recieved an unknow message from server %04X\n",u->adv_msg.cmd);
			}
			//end do stuff
		} else
		if(size==-1 && auth_flag==0) {
			printf("Authenticating...\n");
			ret=plNetAuthenticateHello(sock,(Byte *)username,u);
			tryes++;
		}
	}
	//end login loop

	//now the user is logged in
	if(u->authenticated==1) {
		printf("Succesfully logged in, entering multi-threaded mode...\n");

		int ret;

		//create the net core thread
		DBG(3,"Creating thread...\n");
		pthread_t netcore_th;
		ret=pthread_create(&netcore_th,NULL,(void*(*)(void*))(&(netcore)),NULL);
		if(ret<0) {
			printf("ERR: Error ocurred entering into the threaded mode\n");
			__state_running=0;
		} else {
			DBG(3,"Netcore Thread created.\n");
			__nths++;
		}
		//start the interactive console

		DBG(4,"__state_running is %i\n",__state_running);
		if(__state_running==1) {
			DBG(3,"request vaultplayerlist\n");
			ret=plNetMsgRequestMyVaultPlayerList(sock,u);

			DBG(3,"waiting for the list\n");
			//wait for vault player list
			printf("Requesting player list from vault...\n");
			sem_wait(&msg_sem);

			DBG(3,"list processed telling netcore to continue operating...\n");
			//tell the netcore that the message has been processed
			sem_post(&pmsg_sem);


			char c;
			int sc=-1;

			while((sc<0 || sc>u->p_num) && __state_running==1) {
				DBG(3,"asking user for the menu...\n");
				//bring up the menu
				printf("Please select or create a player:\n");
				printf("[0] Create a new player...\n");
				DBG(2,"number of players is %i\n",u->p_num);
				print_player_menu(u->p_num,u->p_list);
				printf("Your choice is: "); fflush(0);
				c=getchar();
				sc=c-0x30;
				if(sc<0 || sc>u->p_num) {
					printf("Invalid choice!\n");
				}
			}
			if(sc==0) {


			}

		}

		//end actions
		sem_destroy(&msg_sem); //destroy semaphore
		sem_destroy(&pmsg_sem); //destroy semaphores

	}

// ---- end
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

	if(global_logs_enabled==1) {
		close_log_files();
	}

	return 0;
}

