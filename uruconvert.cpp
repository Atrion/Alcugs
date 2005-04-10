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

/*******************************************************************************
* Uru Command line                                                             *
*                                                                              *
*    License will be added here                                                *
*                                                                              *
*******************************************************************************/

/* Don't touch - NEVER */
const char * ID = "$Id$";
const char * BUILD =  __DATE__ " " __TIME__;
const char * SNAME = "UruConvert";
const char * VERSION = "1.0";
/* */

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"


//paranoic dbg level
//#define _DBG_PARANOIC

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

/*Includes */
//#ifndef _DBG_LEVEL_
#define _DBG_LEVEL_ 0
//#endif

#include "useful.h" //useful generic functions
#include "debug.h" //for debugging
#include "data_types.h" //for data types used in this file
#include "config_parser.h" //for globals
#include "stdebug.h" //for debugging
#include "protocol.h" //all protocol specs
#include "urunet.h" //network functions

//global variable that points to all players struct
st_uru_client * all_players;
st_uru_client * auth=NULL; //points to the auth server
st_uru_client * vault=NULL; //points to the vault server
st_uru_client * track=NULL; //poinst to the tracking server
st_uru_client * meta=NULL;

#include "urumsg.h" //generic messages generator
#include "pclientmsg.h" //client messages parser

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

#include "mysql.h"
#include "log.h"
char *syslogname="URUCONVERT";
MYSQLINFO *mysqli;
void dothejob();
char u_hostname[100];
char u_username[30];
char u_password[30];
char u_database[30];


//status
volatile int __state_running=1;
volatile int __netcore_running=1;
volatile int __nths=1;

//pointer to all the client structs
st_uru_client * ses=NULL;
int sock;

//semaphore
sem_t msg_sem; //is there a message?
sem_t pmsg_sem; //has been the message processed?

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
	printf("Usage: urucmd user@server:port [options]\n\n\
 -V: show version and end\n\
 -h: Show short help and end\n\
 -v: Set the verbose level\n\
 -lp: Set the local port\n\
 -lr: Set the remote port\n\
 -lh: Set the local host\n\
 -rh: Set the remote host\n\n\
 -nl: Enable netcore logs\n");
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


//this seems a tunning machine ;) , well at least Models Abstractes de Càlcul was
//useful for something ;)
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


int main(int argc, char * argv[]) {

	int i,ret;

	//int sock; //the socket

	//destination settings...
	char hostname[100]="";
	char username[100]="";
	char password_hash[30]="";
	int port=5000;

	//local settings...
	int l_port=0;
	char l_hostname[100]="0.0.0.0";

	//session id
	int sid=0;
	//msg size
	int size;
	//offset
	int off=0;

	//st_uru_client * ses=NULL; //all sessions will be here
	st_uru_client * u=NULL; //pointer to the session handler
	ses=NULL;

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
			return -1;
		} else if(parameters_parse(argc,argv,i,"-lp",2,&l_port)) i++;
		else if(parameters_parse(argc,argv,i,"-rp",2,&port)) i++;
		else if(parameters_parse(argc,argv,i,"-nl",0,NULL)) {
			global_logs_enabled=1;
		}
		else if(parameters_parse(argc,argv,i,"-lh",1,&ret)) {
			strcpy(l_hostname,argv[ret]); i++;
		}
		else if(parameters_parse(argc,argv,i,"-rh",1,&ret)) {
			strcpy(hostname,argv[ret]); i++;
		}
    else if(parameters_parse(argc,argv,i,"-uh",1,&ret)) {
      strcpy(u_hostname,argv[ret]); i++;
    }
    else if(parameters_parse(argc,argv,i,"-uu",1,&ret)) {
      strcpy(u_username,argv[ret]); i++;
    }
    else if(parameters_parse(argc,argv,i,"-up",1,&ret)) {
      strcpy(u_password,argv[ret]); i++;
    }
    else if(parameters_parse(argc,argv,i,"-ud",1,&ret)) {
      strcpy(u_database,argv[ret]); i++;
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
		else {
			if(i==1) {
				if(get_host_info(argv[1],hostname,username,(U16 *)(&port))!=1) {
					printf("There was an error parsing the hostname\n");
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

	version();
	disclaimer();

	if(global_logs_enabled==1) {
		open_log_files();
	}

	printf("Please be sure that you are using the latest version!\n");

	if(!strcmp(hostname,"")) {
		printf("Please enter the destination host/ip: ");
		fflush(0);
		strcpy(hostname,ask());
	}

	printf("Connecting to %s@%s:%i...\n",username,hostname,port);

	install_handlers();
	sock=plNetStartOp(l_port,l_hostname);

	//create semaphores
	sem_init(&msg_sem,0,0);
	sem_init(&pmsg_sem,0,0);


	if(sock<0) {
		fprintf(stderr,"Fatal; I cannot start the Netcore!\n");
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

	//get the host
	if(unet_set_host_info(hostname,port,u)<0) {
		parameters_usage(); return -1;
	}

	//set validation and version
	u->validation=0x02;
	u->minor_version=7;
	u->major_version=12;
	u->maxPacketSz=1024; //set maxpacketsize
	u->release=0x07; //magic number 7//TDbg; //set debug release

	//init client & server headers (on udp server is this program & client is the remote host)
	uru_init_header(&u->server,u->validation);
	uru_init_header(&u->client,u->validation);


	//login here
	if(!strcmp(username,"")) {
		printf("Please enter your username: ");
		fflush(0);
		strcpy(username,ask());
	}

	//get password
	strcpy((char *)u->passwd,getpass("Please enter your password: "));
	//DBG(4,"passwd is: %s\n",u->passwd);
	MD5(u->passwd,strlen((const char *)u->passwd),(Byte *)password_hash);
	hex2ascii2(u->passwd,(Byte *)password_hash,16);

	DBG(3,"passwd hash: %s\n",u->passwd);

	//set login
	strcpy((char *)u->login,username);

	//initial negotiation
	u->authenticated=0;
	u->negotiated=0;
	u->flag=1; //solved 234 bugs with this line (is very important) also added a SEGFAULT
	ret=plNetClientComm(sock,u);
	if(ret<0) {
		host_error();
	}

	//set the session initial time
	time(&u->timestamp);
	u->microseconds=get_microseconds();

	//and now create the msg
	u->adv_msg.timestamp=u->timestamp;
	u->adv_msg.microseconds=u->microseconds;
	//set player id to 0
	u->ki=0;
	u->adv_msg.ki=0;
	u->adv_msg.x=0; //set x
	u->adv_msg.max_version=u->major_version;
	u->adv_msg.min_version=u->minor_version;
	u->adv_msg.release=u->release;
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
			Log(L_CO,NULL,"ERR: Error ocurred entering into the threaded mode\n");
			__state_running=0;
		} else {
			DBG(3,"Netcore Thread created.\n");
			__nths++;
		}
		//start the interactive console

		if(__state_running==1) {
			dothejob();
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

static int level;
char levelstr[21];

char *KNodeTypes[] = {
	"KInvalidNode",
	"",
	"KVNodeMgrPlayerNode",	// 2
	"KVNodeMgrAgeNode",
	"KVNodeMgrGameServerNode",
	"KVNodeMgrAdminNode",
	"KVNodeMgrServerNode",
	"KVNodeMgrCCRNode",			// 7
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"KFolderNode",				// 22
	"KPlayerInfoNode",		// 23
	"KSystem",
	"KImageNode",
	"KTextNoteNode",
	"KSDLNode",
	"KAgeLinkNode",
	"KChronicleNode",
	"KPlayerInfoListNode",
	"",
	"KMarkerNode",
	"KAgeInfoNode",
	"KAgeInfoListNode",
	"KMarkerListNode"			// 35
};

void
getchildsrecurs(MYSQLINFO *sock)
{
  MYSQLINFO *mysqlchildsi, *mysqlnodesi;

  if (((mysqlchildsi = dupSGBD(sock)) == NULL) || ((mysqlnodesi = dupSGBD(sock)) == NULL))
    return;
	if (mysql_request(mysqlchildsi, "SELECT ChildIdx from NodeRefs WHERE ParentIdx=%ld", atoi(sock->row[0])) == NULL)
		return;
	while (((mysqlchildsi->row = mysql_fetch_row(mysqlchildsi->res)) != NULL) && mysqlchildsi->row[0]) {
		  if (mysql_request(mysqlnodesi, "SELECT * from Nodes WHERE Idx=%ld", atoi(mysqlchildsi->row[0])) == NULL)
		    return;
			if ((mysqlnodesi->row = mysql_fetch_row(mysqlnodesi->res)) != NULL) {
			  strcpy(levelstr, "-                   ");
				levelstr[level] = '\0';
			  fprintf(stderr, "%s %ld %s\n", levelstr, atoi(mysqlchildsi->row[0]), KNodeTypes[atoi(mysqlnodesi->row[12])]);
			  level++;
			  getchildsrecurs(mysqlchildsi);
		  } else Log(L_ALL,NULL,"Problem while getting node info");
	}
	free(mysqlchildsi);
	free(mysqlnodesi);
	level--;
}

void
dothejob()
{
  // HURU -> SELECT index,
  char *queryfmt="SELECT n1.OwnerIdx,n1.Idx,n1.IString64_1,UPPER(n2.IString64_2),n2.String64_1,n1.String64_1 FROM Nodes AS n1 LEFT JOIN Nodes AS n2 ON n1.OwnerIdx=n2.Idx WHERE n1.NodeType=%2d AND n1.OwnerIdx !=0 ORDER BY n1.Idx;";
//  char *queryfmt="SELECT OwnerIdx,IString64_1 FROM Nodes WHERE NodeType=%2d ORDER BY Idx;";
  char *query=(char *)malloc(strlen(queryfmt)+1);

  Log(L_ALL,NULL,"Entering dothejob()");
  sprintf(query, queryfmt, KPlayerInfoNode);

  if ((mysqli = allocSGBD()) == NULL)
    return;
  if (!loginSGBD(mysqli, u_hostname, u_username, u_password, u_database, FALSE))
    return;
  if (mysql_request(mysqli, query) == NULL)
    return;
  free(query);

  while (((mysqli->row = mysql_fetch_row(mysqli->res)) != NULL) && mysqli->row[0]) {
//    Log(L_ALL, NULL, "Processing %s (%ld)", mysqli->row[1], atoi(mysqli->row[0]));
		Log(L_ALL, NULL, "plVaultCreatePlayer(sock, %s,%s,%s,%s,15) %ld",
			mysqli->row[2],mysqli->row[3],mysqli->row[2],mysqli->row[4], atoi(mysqli->row[0]));
		// get Child nodes
		level=1;
		getchildsrecurs(mysqli);  // OwnerIdx
    break;
//    if ((ret = plVaultCreatePlayer(sock,mysqli->row[1],mysqli->row[2],mysqli->row[1],mysqli->row[3],15)) > 0) {
//     printf("Created %s\n", vusername);
//    } else {
//      printf("ERR: couldn't create player %s (%s)",
//        vusername, ret<0?"database error":"already exist");
//    }
  }

  mysql_end_request(mysqli);
  logoutSGBD(mysqli);
  freeSGBD(mysqli);

//  while ( findallusers in untiluru) {
//
//    if ((ret = plVaultCreatePlayer(sock,vusername,vguid,vavie,vgender,acclev)) > 0) {
//      printf("Created %s\n", vusername);
//    } else {
//      printf("ERR: couldn't create player %s (%s)",
//        vusername, ret<0?"database error":"already exist");
//    }
//  }
}

