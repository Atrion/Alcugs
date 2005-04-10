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

/* Plasma GATEWAY driver

   Please, carefully read the DISCLAIMER and client EULA before doing changes
   to this code.
*/

#ifndef __U_CGAS_AUTH_
#define __U_CGAS_AUTH_
#define __U_CGAS_AUTH_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

#include "data_types.h"

#include "cgas_auth.h"

extern char * VERSION;

int CGAS_parse_response(char * msg,int size,char * hash,char * passwd,char * uid);

/** Parse out the Auth Credentials [INTERNAL USE ONLY]
*/
int CGAS_parse_response(char * msg,int size,char * hash,char * passwd,char * uid) {
	//Let's go to use some type of weird Tunning machine for this
	int c; //current_char
	char left_buffer[1024]; //Max size of a variable
	char right_buffer[1024]; //Max size of a value
	int sizeA=0;
	int sizeB=0;
	int i; //an iterator
	int l; //line counter

	Byte comment=0; //commentary
	Byte mode=0; //parse mode 0 none, 1 left, 2 mid, 3 right, 4 end
	Byte quote=0; //quote mode
	Byte slash=0; //slash mode

	int j=0; //stream position

	int ret=0;

	strcpy(hash,"");
	strcpy(passwd,"");
	strcpy(uid,"");

	//parse
	i=0; l=1;
	while(j<size) {
		c=(int)msg[j];
		j++;
		//printf("char:'%c' mode:%i\n",c,mode); fflush(0);
		if(c=='#' && quote==0) {
			if(mode==1 || mode==2 || mode==3) {
				//fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"#\"",conf,l);
				//fclose(f);
				//return 1;
			}
			comment=1;  //activate comment flag
		}
		else if(c=='\n') {
			comment=0;
			if(mode==1 || mode==2) {
				//fprintf(dsc,"ERR: Parse error reading %s at line %i, missing right member\n",conf,l);
				//fclose(f);
				//return 1;
			}
			l++;
			if((mode==3 && quote==0) || mode==4) {
				sizeB=i;
				i=0;
				mode=0;
			}
		}
		else if(comment==0 && c=='\"' && slash==0) {
			if(quote==1) { quote=0; mode=4; }
			else if(mode==2) { mode=3; quote=1; }
			else {
				//fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected '\"'\n",conf,l);
				//fclose(f);
				//return (1);
			}
		}
		else if(slash==1) {
			slash=0;
			if(c=='n') { right_buffer[i]='\n'; }
			else { right_buffer[i]=c; }
			i++;
		}
		else if(c=='\\' && slash==0) {
			if(quote==1) {
				slash=1;
			} else {
				//fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected '\\'\n",conf,l);
				//fclose(f);
				//return 1;
			}
		}
		else if(comment==0 && quote==0 && (c==' ' || c==':' || isblank(c))) {
			if(mode==1) {
				mode=2;
				sizeA=i;
				i=0;
			}
			else if(mode==3) {
				if(c==':') { //on config files it was "=" --^
					//fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"=\"\n",conf,l);
					//fclose(f);
					//return(1);
				}
				mode=4;
			}
		} else if(isalpha(c) && comment==0 && mode==0) { //first character of a config directive
			if(i>=1024) {
				//fprintf(dsc,"ERR: Parse error reading %s at line %i, line too long\n",conf,l);
				//fclose(f);
				return(-1);
			}
			left_buffer[i]=c;
			i++;
			mode=1;
			//printf("<mode-is-now-%i>",mode);
		} else if(isprint(c) && comment==0 && (mode==1 || mode==2 || mode==3)) {
			if(i>=1024) {
				//fprintf(dsc,"ERR: Parse error reading %s at line %i, line too long\n",conf,l);
				//fclose(f);
				return(-1);
			}
			if(mode==1) {
				left_buffer[i]=tolower(c);
			}
			else if(mode==3 || mode==2) {
				mode=3;
				right_buffer[i]=c;
			}
			i++;
		//} else if(comment==0 && !feof(f)) {
			//fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected character '%c'\n",conf,l,c);
			//fclose(f);
			//return(1);
		}

		if(mode==0 && sizeA!=0) {
			left_buffer[sizeA]='\0';
			right_buffer[sizeB]='\0';
			//printf("->%s<-\n",left_buffer);
			if(!strcasecmp(left_buffer,"X-plHashReply")) {
				strcpy((char *)hash,(char *)right_buffer);
				ret++;
			} else if(!strcasecmp(left_buffer,"X-plHashed")) {
				strcpy((char *)passwd,(char *)right_buffer);
				ret++;
			} else if(!strcasecmp(left_buffer,"X-plGuid")) {
				strcpy((char *)uid,(char *)right_buffer);
				ret++;
			} else {
				//fprintf(dsc,"ERR: Unknown configuration directive %s reading %s at line %i\n",left_buffer,conf,l-1);
				//fclose(f);
				//return(1);
			}
			sizeA=0;
			sizeB=0;
		}
	}
	if(ret==3) {
		ret=1;
	} else {
		ret=0;
	}
	return ret;
}


/** Querys the Auth credentials to the Cyan's Global Auth Server
*/
int CGAS_query(char * login,char * challenge,char * client_ip,char * hash,char * passwd,char * uid) {
	//WARNING - YOU MUST NEVER EDIT THESE VARS
	char * cgas_host="auth.plasma.corelands.com";
	unsigned short int cgas_port=80;
	char * cgas_path="/info";
	//BYPASSING CYAN'S GLOBAL AUTH SERVER GOES AGAINST THE CLIENT EULA.
	//HOSTING A SERVER THAT BYPASSES THE CYAN'S AUTH SERVER IS ILLEGAL.
	//YOU MUST NEVER BYPASS CYAN'S GLOBAL AUTH SERVER FOR ANY PURPOSE
	char * agent="H'uruHTTP";
	int version=1; //put version information with the user agent?
	// 0 -- mimic the plasma auth server query
	// 1 -- minimal identification
	// 2 -- full identification, including server build

	int ret=0;
	//Auth result
	// <=0 -> Failed
	// 1 -> OK

	int sock; //the socket
	struct sockaddr_in server; // server address struct

#define HTTP_BUFFER_SIZE 1024

	char buf[HTTP_BUFFER_SIZE]; //Response buffer
	char msg[HTTP_BUFFER_SIZE]; //Message to send
	char user_agent[200];

	int msg_size;  //size of the message
	int rcv_size;  //size of recieved message
	int ip; //server ip

	struct hostent *host; //Host struct
	host=gethostbyname(cgas_host); //The hostname resolution
	if(host==NULL) {
		return -1; //failed
	}
	ip=*(int *)host->h_addr_list[0];

	//set the user agent
	if(version==0) {
		sprintf(user_agent,"UruHTTP");
	} else if(version==1) {
		strcpy(user_agent,agent);
	} else {
		sprintf(user_agent,"%s %s",agent,VERSION);
	}

	//Additional sanity check
	static char last_challenge[50]="";

	if(!strcmp(last_challenge,challenge)) { return -1; }
	strcpy(last_challenge,challenge);
	//two consequtive challenges meants that something is going wrong, or someone is attempting some type of weird hack
	
	//Perform sanity checks
	if(strlen(user_agent)>=100 || strlen(login)>=100 || strlen(challenge)!=32 || strlen(client_ip)>=40 || strlen(login)==0 || strlen(client_ip)==0 || strlen(user_agent)==0) {
		return -1; //Avoid to send a malformed (hacked) petition to the server.
	}

	sprintf(msg,"\
GET %s HTTP/1.1\r\n\
Host: %s\r\n\
Connection: close\r\n\
User-Agent: %s\r\n\
X-plLogin: %s\r\n\
X-plHash: %s\r\n\
X-plClientIP: %s\r\n\r\n\
",cgas_path,cgas_host,user_agent,login,challenge,client_ip);

	// Create a TCP socket
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0) {
		perror("socket");
		return -1;
	}

	//The server (destination) struct
	memset(&server,0,sizeof(server)); //Delete it (zero it)
	server.sin_family=AF_INET; //The family
	server.sin_addr.s_addr=ip; //The ip address
	server.sin_port=htons(cgas_port); //The port

	//Start connection to the auth server
	if(connect(sock,(struct sockaddr *)&server,sizeof(server))<0) {
		perror("connect");
		//fprintf(f_cgas,"Error on connect()\n");
		return -1;
	}

	msg_size=strlen(msg); //Get the msg size

	FILE * f_cgas;
	f_cgas=fopen("CGASauth.log","a");
	fprintf(f_cgas,"Sent:\n%s\n",msg);

	//Send the msg to the server
	if(send(sock,msg,msg_size,0)!=msg_size) {
			perror("sended number of bytes!=msg_size?");
			fprintf(f_cgas,"Error on send()\n");
			fclose(f_cgas);
			return -1;
	}

	//Now check the server answer
	fprintf(f_cgas,"Received:\n");
	//Recieve the message
	if((rcv_size=recv(sock,buf,HTTP_BUFFER_SIZE-1,0))<=0) {
		perror("recv");
		fprintf(f_cgas,"Error on recv()\n");
		fclose(f_cgas);
		return -1;
	}
	buf[rcv_size] = '\0';
	fprintf(f_cgas,"%s\n",buf);
	fclose(f_cgas);

	close(sock);

	//Now parse out the credentials
	ret=CGAS_parse_response(buf,rcv_size,hash,passwd,uid);

	return ret;
}

#endif
