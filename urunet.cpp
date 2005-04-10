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

// UruNet class implementation will be here (temporany, it will be a list of all, C functions)

/*
	this will contain all related with using the socket
*/

#ifndef __U_URUNET_
#define __U_URUNET_
/* CVS tag - DON'T TOUCH*/
#define __U_URUNET_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

//number of seconds to wait to re-send pending packets
#define ACK_TIMEOUT 1

//number of seconds & milliseconds to wait to the netcore to loop again
#define NETCORE_TIMEOUT_S 1
#define NETCORE_TIMEOUT_US 0
//500000

#include "debug.h" //for debugging

#include "urunet.h"

//--- external references ---//
extern st_uru_client * track;
extern st_uru_client * vault;
extern int plNetMsgCustomPlayerStatus(int sock,Byte * uid,Byte * name,Byte * avie,Byte flag,Byte status,st_uru_client * u);
extern int plNetMsgCustomVaultPlayerStatus(int sock,Byte * age,Byte * guid,Byte state,U32 online_time,st_uru_client * u);
//--- end external references --//

void unet_set_recv_host(st_uru_client * u) {
	struct sockaddr_in client;

	client.sin_family=AF_INET; //UDP IP
	client.sin_addr.s_addr=u->client_ip; //address
	client.sin_port=u->client_port; //port

	memcpy(u->sock_array,&client,sizeof(struct sockaddr_in));
}

int unet_set_host_info(char * hostname,int port,st_uru_client * u) {
	struct hostent *host;
	host=gethostbyname(hostname);
	if(host==NULL) {
		return -1;
	}
	u->client_ip=*(U32 *)host->h_addr_list[0];
	u->client_port=htons(port);
	unet_set_recv_host(u);
	return 0;
}

//check for the ack flags & status
int plNetCheckAckDone(st_uru_client * u) {
	int e;
	for(e=0; e<MAX_PACKETS_IN_BUFFER; e++) {
		if(u->out[e][0]==0x01) {
			DBG(4,"checker -> cached packet %i found\n",e);
			return 0;
		}
	}
	return 1;
}


/*------------------------------------------------------------------------
  Sends and encrypts(if necessary) the correct packet
	Returns size if all gone well
	returns <0 if something went wrong
	(this is a low level function)
------------------------------------------------------------------------*/
int uru_net_send(int sock,Byte * buf,int n,st_uru_client * u) {

	//inet
	struct sockaddr_in client; //client struct

	//copy the inet struct
	memcpy(&client,u->sock_array,sizeof(struct sockaddr_in));
	client.sin_family=AF_INET; //UDP IP (????)
	client.sin_addr.s_addr=u->client_ip; //address
	client.sin_port=u->client_port; //port


	print2log(f_uru,"<SND>");
	uru_print_header(f_uru,&u->server);
	print2log(f_uru,"\n");

	if(u->validation==2) {
		DBG(4,"Validation 2 packet found...\n");
		DBG(4,"Enconding packet of %i bytes...\n",n);
		encode_packet(buf,n);
		if(u->authenticated==1) {
			DBG(4,"Client is authenticated...\n");
			DBG(4,"Doing checksum...\n");
			*((U32 *)(buf+2))=uru_checksum(buf,n,2,u->passwd);
			DBG(4,"Checksum done!...\n");
		} else {
			DBG(4,"Client is not authenticated...\n");
			DBG(4,"Doing checksum...\n");
			*((U32 *)(buf+2))=uru_checksum(buf,n,1,NULL);
			DBG(4,"Checksum done!...\n");
		}
		if(u->authenticated==2) {
			DBG(4,"Auth phase 2...\n");
			u->authenticated=1;
			stamp2log(f_uru);
			ip2log(f_uru,u);
			print2log(f_uru,"DBG: I HAVE SETUP AUTHENTICATED WITH VALUE=1<---\n");
		}
		buf[1]=0x02;
	} else
	if(u->validation==1) {
		if(u->server.ch==1) {
			*((U32 *)(buf+2))=uru_checksum(buf,n,0,NULL);
			buf[1]=0x01;
		} else {
			buf[1]=0x00;
		}
	} else {
		buf[1]=0x00;
	}
	DBG(4,"Magic number added...\n");
	buf[0]=0x03; //magic number


	DBG(4,"Before the Sendto call...\n");
	n = sendto(sock,buf,n,0,(struct sockaddr *)&client,sizeof(struct sockaddr));
	if(n<0) {
		ERR(2,"n<0 ?, %i",n);
	}
	DBG(4,"After the Sendto call...\n");

	/* CutreSoft software presents, cutre patch for avoid problems */
	if(u->validation==2 and n>0) {
		//horrible software manipulation cpu stupid consuming cycles..
		decode_packet(buf,n); //cought!
		buf[1]=0x02; //recover validation level
	}

	DBG(2,"returning from uru_net_send RET:%i\n",n)

	return n;
}


/****************************************************************
High level functions

****************************************************************/

/*------------------------------------------------------
 Initialitzes the session default values
------------------------------------------------------*/
void u_init_session(st_uru_client * session,int max) {
	int i,e;
	//be sure that all sessions are clean
	for(i=0; i<=max; i++) {
		DBG(4,"%i - forin\n",i)
		session[i].client_ip=0; //session is free
		session[i].authenticated=0; //not authenticated
		session[i].flag=0; //flag used in the find algorithm
		DBG(4,"aide\n");
		time(&session[i].server.timestamp); //init timestamps to current time
		DBG(4,"aide\n");
		//session[i].server.timestamp=session[i].client.timestamp;
		session[i].server.microseconds=get_microseconds();
		DBG(4,"aide\n");
		//session[i].client.microseconds=session[i].server.microseconds;
		session[i].access_level=AcPlayer; //player level by default
		session[i].tpots=0; //tpots modifier
		session[i].paged=0; //player not paged in
		session[i].ki=0;
		session[i].major_version=0;
		session[i].minor_version=0;
		//session[i].snd_i=0; //iterator reset
		/*for(e=0; e<NUM_MSGS; e++) {
			session[i].snd[e].flag=0; //flag reset
		}*/
		DBG(4,"%i - step\n",i);
		for(e=0; e<MAX_PACKETS_IN_BUFFER; e++) {
			session[i].out[e][0]=0x00; //reset the flags
		}
		for(e=0; e<MAX_PACKETS_PENDING; e++) {
			session[i].pending[e].flag=0; //reset the flags
		}
		DBG(4,"%i - forout\n",i);
	}
	//Init the default 0 session, used when the server is OVERLOADED!!
	session[0].access_level=AcNotRes+5;
}


/*----------------------------------------------------
  Starts the network operation
	returns -1 on error
----------------------------------------------------*/
int plNetStartOp(U16 port,char * hostname) {

	int sock;

	struct sockaddr_in server; //server struct

	//creating the socket
	sock=socket(AF_INET,SOCK_DGRAM,0); //udp is listed as 17, but always 0 (ip) is used
	if(sock<0) { //lately we will migrate to a better logging system, and error control...
		stamp2log(f_err);
		print2log(f_err,"ERR: Fatal - Failed Creating socket\n");
		return -1; //size -1 (error)
	}
	stamp2log(f_uru);
	print2log(f_uru,"DBG: Socket created\n");


	//set non-blocking
	long arg;
	if( (arg = fcntl(sock,F_GETFL, NULL))<0) {
		stamp2log(f_err);
		print2log(f_err,"ERR: Fatal setting socket as non-blocking (fnctl F_GETFL)\n");
		return -2;
	}
	arg |= O_NONBLOCK;

	if(fcntl(sock, F_SETFL, arg)<0) {
		stamp2log(f_err);
		print2log(f_err,"ERR: Fatal setting socket as non-blocking\n");
		return -3;
	}

	//set network specific options
	server.sin_family=AF_INET; //UDP IP
	server.sin_addr.s_addr=htonl(INADDR_ANY); //any address

	struct hostent *host;
	host=gethostbyname(hostname);
	if(host==NULL) {
		return -4;
	}
	server.sin_addr.s_addr=*(U32 *)host->h_addr_list[0];

	//server.sin_addr.s_addr=htonl(0xAC1A000D);

	server.sin_port=htons(port); //port 5000 default

	//binding port
	if(bind(sock,(struct sockaddr *)&server,sizeof(server))<0) {
		stamp2log(f_err);
		print2log(f_err,"ERR: Fatal - Failed binding to port %i\n",port);
		return -5;
	}
	stamp2log(f_uru);
	print2log(f_uru,"DBG: Listening to incoming datagrams on %s port udp %i\n\n",hostname,port);
	logflush(f_uru);

	return sock; //return the socket
}

//* Resend old non acked packets
int plNetReSendAckPackets(int sock,st_uru_client * u) {
	//send a row of non acked packets
	int ret=0;
	stamp2log(f_uru);
	print2log(f_uru,"re-sending packets with pending ack flags\n");
	int e; int stats=0;

	for(e=0; e<MAX_PACKETS_IN_BUFFER; e++) {
		if(u->out[e][0]==0x01) {
			DBG(4,"cached packet %i found\n",e);
			ret=1;

			int size; int pn;
			DBG(4,"validation level is %i\n",u->out[e][1]);
			if(u->out[e][1]==0x02 || u->out[e][1]==0x01) {
				DBG(4,"validation 1-2 found\n");
				pn=*(U32 *)(&u->out[e][6]);
				//pn++;
				//*(U32 *)(&session[i].out[e][6])=pn;
				u->server.p_n++;
				DBG(4,"pn is %i, updating to %i\n",pn,u->server.p_n);
				*(U32 *)(&u->out[e][6])=u->server.p_n;

				size=*(U32 *)(&u->out[e][28])+32;
			}
			else {
				DBG(4,"validation 0 found\n");
				pn=*(U32 *)(&u->out[e][2]);
				//pn++;
				u->server.p_n++;
				DBG(4,"pn is %i, updating to %i\n",pn,u->server.p_n);
				//*(U32 *)(&session[i].out[e][2])=pn;
				*(U32 *)(&u->out[e][2])=u->server.p_n;

				size=*(U32 *)(&u->out[e][24])+28;
			}

			//ok let's go to dump out 10 packets out of the buffer
			if(_DBG_LEVEL_>2) {
				savefile((char *)u->out,10*1024,"buffer_dump.raw");
			}

			DBG(3,"attemting to send packet %i of %i bytes\n",e,size);
			uru_net_send(sock,u->out[e],size,u);
			DBG(3,"packet %i sent\n",e);

			stats++;

			u->out[e][0]=0x01;

			if(((U32)pn+256)<(u->server.p_n)) {
				print2log(f_uru,"INFO: Deleting old non-acked packets\n");
				u->out[e][0]=0x02;
			}
		}
	}
	print2log(f_uru,"%i packets were sent\n",stats);
	return ret;
}


/*----------------------------------------------------*/
/** Substititon of the recvmsg, the high level one
	(one step more and we will do a class)

	stores the session id in the sid.

	\return
	 returns -1 if error\n
	 returns size of packet if success
*/
/*-----------------------------------------------------*/
int plNetRecv(int sock,int * sid, st_uru_client ** v_session) {
	int n,s_n,i; //size of packet and iterator
	int ret; //joker variable

	int s_old; //identifier for and old session found
	int s_new; //identifier for a void slot session found

	//cutre fix
	st_uru_client * session;
	session=*v_session;
	//cutre fix

	//WARNING - WARNING - WARNING - BE CAREFULL WITH THESE STATICS...
	static Byte ack_buf[OUT_BUFFER_SIZE]; //buffer for ack packets
	static Byte buf[BUFFER_SIZE]; //internal only now

	struct sockaddr_in client; //client struct

	socklen_t client_len; //client len size

	client_len=sizeof(struct sockaddr_in); //get client struct size

	//waiting for packets - timeout
	fd_set rfds;
	struct timeval tv;
	int valret;

	/* Mirar socket para ver si tiene entrada */
	FD_ZERO(&rfds);
	FD_SET(sock, &rfds);
	/* Esperar hasta 5 s */
	tv.tv_sec = NETCORE_TIMEOUT_S;
	tv.tv_usec = NETCORE_TIMEOUT_US;

//#if 0
//	printf("Waiting....\n"); fflush(0);
	valret = select(sock+1, &rfds, NULL, NULL, &tv);
	/* ¡No confiar ahora en el valor de tv! */

#if 0
	if (valret)
			printf("Data recieved...\n");
			/* FD_ISSET(0, &rfds) será verdadero */
	else {
			printf("No data recieved...\n");
			fflush(0);
	}
#endif

	//waiting for packets
	n = recvfrom(sock,buf,BUFFER_SIZE,0,(struct sockaddr *)&client,&client_len);
	//The trick is to start a new thread with each new packet to process it.. but well, I will see what I can do.

	if(n<0 && valret) { //problems?
		stamp2log(f_err);
		print2log(f_err,"ERR: Fatal recieving...\n");
		//perror("what");
		//return -1;
		return -2;
	}


	/** \todo
		1) Check for clients those timestamp is old,
		and then resend old not acked packets.\n
		2) Firewall\n
		3) Argh!! re-write the entire engine!! :(
		\bug
		 If the first packet of a fragmented stream is dropped, then the entire stream
		will be ignored by the server.\n
		 If the same packet is drop several times, and the client sends a new one, then again
		the old one, or the entire stream will be ignored.
	*/


	/*
		if n=0 (this requires a non-blocking policy).
		TODO: Check for clients those timestamp is old,
		and then resend old not acked packets.
	*/

	static Byte randomizer=0;
	randomizer++;
	int e;

	st_uru_msg_r * my_msg;
	st_uru_msg_r * my_msg_aux;

	if(!valret || randomizer>50) {
		randomizer=0;

		time(&session[0].timestamp); //init default session timestamp to current time

		for(i=1; i<=global_client_count; i++) {
			if(session[i].flag==0) { break; }
			else if(session[i].client_ip!=0) {
				if((session[0].timestamp-session[i].timestamp)\
>(global_connection_timeout*60) && i>global_the_bar) {
					stamp2log(f_uru);
					print2log(f_uru,"Disconnecting dead client...\n");
					//send a TERMINATED:Timeout here if we are recycling an old session
					if(session[i].client_ip!=0) {
						plNetMsgTerminated(sock, RTimedOut, &session[i]);
					}
					//delete session vars
					//plNetDisconnect(&session[i]);
					session[i].client_ip=0; //delete the ip (client loged out)
//---
						//destroy messages
						DBG(3,"I'm going to do a free call...\n");
						if(session[i].rcv_msg!=NULL) {
							DBG(3,"before free call...\n");
							free((void *)session[i].rcv_msg);
							session[i].rcv_msg=NULL;
							DBG(3,"after free call...\n");
						}
						if(session[i].snd_msg!=NULL) {
							DBG(3,"before free call...\n");
							free((void *)session[i].snd_msg);
							session[i].snd_msg=NULL;
							DBG(3,"after free call...\n");
						}
						DBG(3,"before several free calls\n");
						for(e=0; e<MAX_PACKETS_PENDING; e++) {
							if(session[i].pending[e].pnd_msg!=NULL) {
								free((void *)session[i].pending[e].pnd_msg);
								session[i].pending[e].pnd_msg=NULL;
							}
						}
						if(session[i].p_list!=NULL) {
							free((void *)session[i].p_list);
							session[i].p_list=NULL;
						}

						DBG(3,"after several free calls\n");
						//now destroy the message cue
						DBG(3,"destroying message cue..\n");
						my_msg=session[i].current_msg;
						while(my_msg!=NULL) {
							if(my_msg->buf!=NULL) {
								free((void *)my_msg->buf);
								my_msg->buf=NULL;
							}
							my_msg_aux=my_msg;
							my_msg=(st_uru_msg_r *)my_msg->next; //the next message
							free((void *)my_msg_aux);
							my_msg_aux=NULL;
						}
						DBG(3,"message cue succesfully destroyed!\n");

//---
					if(i==global_client_count) { // || session[i+1].flag==0) {
						session[i].flag=0;
						global_client_count--;
						*v_session=(st_uru_client *)realloc((void *)(*v_session),sizeof(st_uru_client)*(global_client_count+1));
						session=*v_session;
						if(session==NULL) {
							print2log(f_uru,"Unexpected terrible exception occurred...");
							close_log_files();
							abort();
						}
					}
					//TODO: empty the send buffers
				}
				else
					//if(1==1) {
					if((session[0].timestamp-session[i].timestamp)\
>(ACK_TIMEOUT)) {
						//plNetSend(sock,NULL,0,&session[i]);
						plNetReSendAckPackets(sock,&session[i]);
#if 0
					//send a row of non acked packets
					stamp2log(f_uru);
					print2log(f_uru,"re-sending packets with pending ack flags\n");
					int e; int stats=0;
					for(e=0; e<MAX_PACKETS_IN_BUFFER; e++) {
						if(session[i].out[e][0]==0x01) {
							DBG(4,"cached packet %i found\n",e);

							int size; int pn;
							DBG(4,"validation level is %i\n",session[i].out[e][1]);
							if(session[i].out[e][1]==0x02 || session[i].out[e][1]==0x01) {
								DBG(4,"validation 1-2 found\n");
								pn=*(U32 *)(&session[i].out[e][6]);
								//pn++;
								//*(U32 *)(&session[i].out[e][6])=pn;
								session[i].server.p_n++;
								DBG(4,"pn is %i, updating to %i\n",pn,session[i].server.p_n);
								*(U32 *)(&session[i].out[e][6])=session[i].server.p_n;

								size=*(U32 *)(&session[i].out[e][28])+32;
							}
							else {
								DBG(4,"validation 0 found\n");
								pn=*(U32 *)(&session[i].out[e][2]);
								//pn++;
								session[i].server.p_n++;
								DBG(4,"pn is %i, updating to %i\n",pn,session[i].server.p_n);
								//*(U32 *)(&session[i].out[e][2])=pn;
								*(U32 *)(&session[i].out[e][2])=session[i].server.p_n;

								size=*(U32 *)(&session[i].out[e][24])+28;
							}

							//ok let's go to dump out 10 packets out of the buffer
							if(_DBG_LEVEL_>2) {
								savefile((char *)session[i].out,10*1024,"buffer_dump.raw");
							}

							DBG(3,"attemting to send packet %i of %i bytes to peer %i\n",e,size,i);
							uru_net_send(sock,session[i].out[e],size,&session[i]);
							DBG(3,"packet %i to peer %i sent\n",e,i);

							stats++;

							session[i].out[e][0]=0x01;

							if(((U32)pn+20)<(session[i].server.p_n)) {
								print2log(f_uru,"INFO: Deleting old non-acked packets\n");
								session[i].out[e][0]=0x02;
							}
						}
					}
					print2log(f_uru,"%i packets were sent\n",stats);
#endif
				}
			}
		}
		if(!valret) {
			//This code, should delete any void session (the last one)
			if(global_client_count>global_the_bar) {
				if(session[global_client_count].client_ip==0) {
					//we have a victim
					//argh this is hateful
					//hmmm - this should fix all problems
					DBG(4,"several free calls now\n");
					DBG(5,"free rcv_msg\n");
					if(session[global_client_count].rcv_msg!=NULL) {
						free((void *)session[global_client_count].rcv_msg);
						session[global_client_count].rcv_msg=NULL;
					}
					DBG(5,"free snd_msg\n");
					if(session[global_client_count].snd_msg!=NULL) {
						free((void *)session[global_client_count].snd_msg);
						session[global_client_count].snd_msg=NULL;
					}
					DBG(5,"free p_list\n");
					if(session[global_client_count].p_list!=NULL) {
						free((void *)session[global_client_count].p_list);
						session[global_client_count].p_list=NULL;
					}

					DBG(5,"free my_msg\n");
					my_msg=session[global_client_count].current_msg;
					while(my_msg!=NULL) {
						if(my_msg->buf!=NULL) {
							free((void *)my_msg->buf);
							my_msg->buf=NULL;
						}
						my_msg_aux=my_msg;
						my_msg=(st_uru_msg_r *)my_msg->next; //the next message
						free((void *)my_msg_aux);
						my_msg_aux=NULL;
					}
					//hmmm
					DBG(5,"free pending packets\n");
					for(e=0; e<MAX_PACKETS_PENDING; e++) {
						if(session[global_client_count].pending[e].pnd_msg!=NULL) {
							free(session[global_client_count].pending[e].pnd_msg);
							session[global_client_count].pending[e].pnd_msg=NULL;
						}
					}
					global_client_count--;
					DBG(3,"After a realloc...\n");
					*v_session=(st_uru_client *)realloc((void *)(*v_session),sizeof(st_uru_client)*(global_client_count+1));
					session=*v_session;
					DBG(3,"Before a realloc...\n");
					s_new=global_client_count;
				}
			}

			//end new piece of code
			return -1;
		}
	}


	//TODO: firewall rules
	/*if(strcmp(c_ip,"172.26.0.2")) {
		n=0;
		print2log(f_uru,"\nINF: Blocked connection from %s:%i\n",c_ip,c_port);
	}*/

	/*
		Uru protocol check
		Drop packet if it is not an Uru packet without wasting a session
	*/
	if(buf[0]!=0x03) { //not an Uru protocol packet don't waste an slot for it
		print2log(f_une,"\n");
		stamp2log(f_une);
		ip2log_old(f_une,client.sin_addr.s_addr,client.sin_port);
		print2log(f_une,"[sid:%i] ERR: Unexpected Non-Uru protocol packet found\n",-1);
		dump_packet(f_une,buf,n,0,5);
		print2log(f_une,"\n---------------------------------------------\n");
		//---------
		return -2;
	}

	/*
	find the session id, and control where to manage and save data
	yes, I know, the next version will use those other hash algorithms (TODO) that are more
	efficient, but for now, I will use this ineficient garbage

	Yes this will be changed by a new type of algorith, that searches for a ps and ns at the
	same time that is sending not acked packets... :)
	*/

	s_old=0;
	s_new=0;

	time(&session[0].timestamp); //init default session timestamp to current time

	for(i=1; i<=global_client_count; i++) {
		DBG(4,"\n###################Checking %i###############\n",i);
		if(session[i].flag==0) {
			if(s_new==0) { //new lowest session found
				DBG(4,"\n#############LOWEST NEW SID FOUNDm %i################\n",i);
				s_new=i;
				//session[i].flag=1;
			} else {
				if(session[i-1].client_ip==0 && i!=(global_the_bar+1)) session[i-1].flag=0; //go back..
			}
			break; //al job done!
		}
		DBG(3,"%08X - %04X VS %08X - %04X\n",session[i].client_ip,session[i].client_port,client.sin_addr.s_addr,client.sin_port);
		if(session[i].client_ip==client.sin_addr.s_addr && session[i].client_port==client.sin_port) {
			s_old=i;
			DBG(4,"\n###################OLD SID FOUND %i################\n",i);
		} else if(i>global_the_bar){
			//if(session[i].client_ip!=0 &&
			if((session[0].timestamp-session[i].timestamp)>(global_connection_timeout*60)) {
				//send a TERMINATED:Timeout here if we are recycling an old session
				if(session[i].client_ip!=0) {
					plNetMsgTerminated(sock, RTimedOut, &session[i]);
				}
				//delete session vars
				//plNetDisconnect(&session[i]);
				session[i].client_ip=0; //delete the ip (client loged out)
				if(i==global_client_count || session[i+1].flag==0) session[i].flag=0;
				//TODO: empty the send buffers
			}
			if(session[i].client_ip==0) {
				if(s_new==0) {
					s_new=i; //The lowest session
					DBG(4,"\n#############LOWEST NEW SID FOUND %i################\n",i);
				}
			}
		}
		if(session[i].client_ip!=0) {
			//TODO: RESEND NOT ACKED PACKETS *HERE*
			//check the timeout and the window size
		}
	}

	//patch-fix-cutresoft software
	if(s_old==0 && s_new==0) {
		if(global_client_count<global_max_clients) {
			global_client_count++;
			DBG(3,"After a realloc...\n");
			*v_session=(st_uru_client *)realloc((void *)(*v_session),sizeof(st_uru_client)*(global_client_count+1));
			session=*v_session;
			//hmmm - this should fix all problems
			DBG(4,"Setting rcv_msg to NULL...\n");
			session[global_client_count].rcv_msg=NULL;
			session[global_client_count].snd_msg=NULL;
			session[global_client_count].p_list=NULL;
			session[global_client_count].current_msg=NULL;
			//hmmm
			for(e=0; e<MAX_PACKETS_IN_BUFFER; e++) {
				session[i].out[e][0]=0x00; //reset the flags
			}
			for(e=0; e<MAX_PACKETS_PENDING; e++) {
				session[i].pending[e].flag=0; //reset the flags
				session[i].pending[e].pnd_msg=NULL;
			}
			DBG(3,"Before a realloc...\n");
			s_new=global_client_count;
		}
	}
	//end fix

	//now we have the session - init the vars -
	if(s_old!=0) {
		//old session found
		*sid=s_old;
		print2log(f_uru,"\n____________________________________________________\n");
		stamp2log(f_uru);
		ip2log(f_uru,&session[*sid]);
		print2log(f_uru,"[sid:%i] INF: Old Incoming connection\n",*sid);
		memcpy(session[*sid].sock_array,&client,sizeof(struct sockaddr_in));
		//time(&session[*sid].timestamp); //update the _session_ timestamp (is done above!)
		//also I will add the microseconds update!
		//session[*sid].microseconds=get_microseconds();
	} else if(s_new!=0) {
		//new session found
		session[s_new].flag=1;
		*sid=s_new;
		//Initialitze the data structures
		session[*sid].client_ip=client.sin_addr.s_addr; //ip
		session[*sid].client_port=client.sin_port; //port
		session[*sid].authenticated=0; //be sure that the client is not authenticated
		session[*sid].major_version=0;
		session[*sid].minor_version=0;

		session[*sid].sid=*sid; //<- store the unique session id

		//time(&session[*sid].client.timestamp); //init timestamps to current time
		//session[*sid].server.timestamp=session[*sid].client.timestamp;
		//session[*sid].timestamp=session[*sid].server.timestamp;
		//session[*sid].server.microseconds=get_microseconds();
		//session[*sid].client.microseconds=session[*sid].server.microseconds;
		print2log(f_uru,"\n____________________________________________________\n");
		stamp2log(f_uru);
		ip2log(f_uru,&session[*sid]);
		print2log(f_uru,"[sid:%i] INF: New Incoming connection\n",*sid);
		stamp2log(f_acc);
		ip2log(f_acc,&session[*sid]);
		print2log(f_acc,"[sid:%i] New Incoming connection\n",*sid);
		logflush(f_acc);
		//initzalitze the server structure //buf[1] -> ch_byte
		uru_init_header(&session[*sid].server,buf[1]); //this sets the server ack flags to 0
		session[*sid].old_p_n=0; //reset the agging control
		session[*sid].old_sn=0; //reset the max packet seq number recieved
		session[*sid].negotiated=0; //reset the negotation flag
		session[*sid].tpots=0; //tpots mod
		session[*sid].paged=0; //player not paged in
		session[*sid].ki=0;
		session[*sid].bussy=0; //it's bussy?
		session[*sid].load=0; //number of peers (used in the tracking server)
		session[*sid].joined=0; //Is the player joined?
		session[*sid].status=0;
		time((time_t *)&session[*sid].online_time); //Set the Online time

		for(i=0; i<MAX_PACKETS_IN_BUFFER; i++) {
			session[*sid].out[i][0]=0x00;
		}

		//memcpy(&session[*sid].a_client,&client,sizeof(struct sockaddr_in));
		memcpy(session[*sid].sock_array,&client,sizeof(struct sockaddr_in));

		//session[*sid].a_client=client;
		session[*sid].a_client_size=client_len;

//---
						//destroy messages
						i=*sid;
						DBG(3,"I'm going to do a free call...\n");
						if(session[i].rcv_msg!=NULL) {
							DBG(3,"before free call...\n");
							free((void *)session[i].rcv_msg);
							session[i].rcv_msg=NULL;
							DBG(3,"after free call...\n");
						}
						if(session[i].snd_msg!=NULL) {
							DBG(3,"before free call...\n");
							free((void *)session[i].snd_msg);
							session[i].snd_msg=NULL;
							DBG(3,"after free call...\n");
						}
						DBG(3,"before several free calls\n");
						for(e=0; e<MAX_PACKETS_PENDING; e++) {
							if(session[i].pending[e].pnd_msg!=NULL) {
								free((void *)session[i].pending[e].pnd_msg);
								session[i].pending[e].pnd_msg=NULL;
							}
						}
						DBG(3,"after several free calls\n");
						//now destroy the message cue
						DBG(3,"destroying message cue..\n");
						my_msg=session[i].current_msg;
						while(my_msg!=NULL) {
							if(my_msg->buf!=NULL) {
								free((void *)my_msg->buf);
							}
							my_msg_aux=my_msg;
							my_msg=(st_uru_msg_r *)my_msg->next; //the next message
							free((void *)my_msg_aux);
						}
						session[i].current_msg=NULL;
						DBG(3,"message cue succesfully destroyed!\n");
//---


	} else {
		//the server is kao
		*sid=0;
		if(session[0].client_ip!=client.sin_addr.s_addr) {
			stamp2log(f_acc);
			//ip2log(f_acc,&session[*sid]);
			ip2log_old(f_acc,client.sin_addr.s_addr,client.sin_port);
			print2log(f_acc,"[sid:%i] New Incoming connection (server overloaded)\n",*sid);
			logflush(f_acc);
			//initzialitze the server structure to '0' //buf[1] -> ch_byte
			uru_init_header(&session[*sid].server,buf[1]);
			session[*sid].old_p_n=0; //reset the agging control
			session[*sid].old_sn=0; //reset the max packet seq number recieved
			session[*sid].negotiated=0; //reset the negotation flag

			for(i=0; i<MAX_PACKETS_IN_BUFFER; i++) {
				session[*sid].out[i][0]=0x00;
			}
		}
		//Initialitze the data structures
		session[*sid].client_ip=client.sin_addr.s_addr; //ip
		session[*sid].client_port=client.sin_port; //port
		DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);
		session[*sid].authenticated=0; //be sure that the client is not authenticated
		session[*sid].major_version=0;
		session[*sid].minor_version=0;
		print2log(f_uru,"\n____________________________________________________\n");
		stamp2log(f_uru);
		ip2log(f_uru,&session[*sid]);
		print2log(f_uru,"[sid:%i] INF: Maxium number of clients reached, ignoring client\n",*sid);

		//memcpy(&session[*sid].a_client,&client,sizeof(struct sockaddr_in));
		memcpy(session[*sid].sock_array,&client,sizeof(struct sockaddr_in));

		//session[*sid].a_client=client;
		session[*sid].a_client_size=client_len;

		DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);
	}


	//---------------------------

	//First, get the packet
	//This gets the validation level, stablishes if is necessary to create a session, and
	//decrypts the level 2 packets, also checks the checksum for levels 1 & 2
	ret=uru_validate_packet(buf,n,&session[*sid]);
	DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);
	if(ret>=3) {
		print2log(f_une,"\n");
		stamp2log(f_une);
		ip2log(f_une,&session[*sid]);
		print2log(f_une,"[sid:%i] ERR: Unexpected Uru protocol packet found\n",*sid);
		dump_packet(f_une,buf,n,0,5);
		print2log(f_une,"\n---------------------------------------------\n");
		n=0; //avoid to do nothing
	} else if(ret==2) {
		print2log(f_une,"\n");
		stamp2log(f_une);
		ip2log(f_une,&session[*sid]);
		print2log(f_une,"[sid:%i] ERR: Validation schema %i requested to high!!\n",*sid,session[*sid].validation);
		dump_packet(f_une,buf,n,0,5);
		print2log(f_une,"\n---------------------------------------------\n");
		n=0; //avoid to do nothing
	} else if(ret==1) {
		print2log(f_une,"\n");
		stamp2log(f_une);
		ip2log(f_une,&session[*sid]);
		print2log(f_une,"[sid:%i] ERR: Unexpected Checksum result!\n",*sid);
		print2log(f_une,"Chk: %08X<-\n",*(U32 *)(buf+0x02));
		if(session[*sid].validation==0x02) {
			encode_packet(buf,n); //encode back the packet!! to dump it out!!
			buf[1]=0x02; //put the validation 2 flag on, again
			Byte tmp_hash[33];
			hex2ascii2(tmp_hash,session[*sid].passwd,16);
			print2log(f_une,"Chk: %08X\n",uru_checksum(buf,n,2,tmp_hash));
			print2log(f_une,"Chk: %08X\n",uru_checksum(buf,n,1,NULL));
		}
		print2log(f_une,"Chk: %08X\n",uru_checksum(buf,n,0,NULL));
		dump_packet(f_une,buf,n,0,5);
		if(session[*sid].validation==0x02) {
			decode_packet(buf,n); //decode back the packet!! to process it!!
			buf[1]=0x02; //put the validation 2 flag on, again
		}
		print2log(f_une,"\n---------------------------------------------\n");
		//n=0; //avoid to do nothing
		//NOTE: The server allows, incorrect checksums, but this could change in the future
		//TODO: I will put code here to allow only the bad built 12.0,12.3 auth packets checksums
	} //End of packet validation

//-----------------------ACK complicated stuff starts here -----

	if(n!=0) { //all went ok
		//get the client header (get the client ack flags)
		i=uru_get_header(buf,n,&session[*sid].client);
		DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);
		print2log(f_uru,"<RCV>");
		uru_print_header(f_uru,&session[*sid].client);
		print2log(f_uru,"\n");
		DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);
		//refresh the client session
		time(&session[*sid].timestamp); //update the timestamp
		session[*sid].microseconds=get_microseconds();
		//<-

		//ack colored drawing
		//stamp2log(f_ack);
		//print2log(f_ack,"[%s:%i sid:%i] ->",c_ip,c_port,sid);
		//-->

		if(session[*sid].client.p_n<=session[*sid].old_p_n && session[*sid].client.p_n!=0 && session[*sid].client.t!=0x42) {
			stamp2log(f_uru);
			print2log(f_uru,"INF: Discarded and old packet\n");
			//Discard OLD packet
			n=0;
		} else

		if(i!=0) { //we have something to process

			//what to do now?
			if(session[*sid].client.t==0x80) { //ack reply
				i=uru_process_ack(buf,n,i,&session[*sid]);
				//
				// TODO: Check the cache and delete acked packets,
				//       log strange packets
				//
				if(i==0 && *sid!=0) {
					print2log(f_une,"\n");
					stamp2log(f_une);
					ip2log(f_une,&session[*sid]);
					print2log(f_une,"[sid:%i] INF: Unexpected Ack packet\n",*sid);
					dump_packet(f_une,buf,n,0,5);
					print2log(f_une,"\n------------------------------------------\n");
				}
				n=0; //ack packets don't require a reply
			} else {
				if(session[*sid].client.t==0x02 || session[*sid].client.t==0x42 || session[*sid].client.t==NetCustMsg1) {

					print2log(f_uru,"->");
					uru_print_header(f_uru,&session[*sid].server);
					print2log(f_uru,"<-\n");

					logflush(f_uru);
					//this sends -always- the ACK response on required packets
					///if(session[sid].client.p_n%2==0) { //some dropped packets testing
					s_n=uru_get_ack_reply(ack_buf,&session[*sid]);
					//dump_packet(f_uru,ack_buf,s_n,0,5);
					//print2log(f_uru,"\n");
					//not always, muhahahaha
					//if(session[*sid].client.fr_n!=2) {
					DBG(4,"Sending ack reply...\n");
					uru_net_send(sock,ack_buf,s_n,&session[*sid]);
					DBG(4,"Ack reply sent...\n");
					//} else {
					//	n=0; //ups !:
					//}
					///}
					//printf("plNetRcv step0\n"); fflush(0);
				}
				//negotation ?
				//printf("plNetRcv step1\n"); fflush(0);
				if(session[*sid].client.t==0x42) { //It's a negotation packet?
					s_n=uru_process_negotation(buf,n,i,&session[*sid]);
					if(s_n!=0 && *sid!=0) {
						/* BUG: Problem with negotation packets, the time is always reset, and
							this shouldn't be happen. This causes a terrible endless loop.
						*/
						//we are going to send a (Re)Negotation request (this resets all internal counters)
						if(session[*sid].negotiated==1) {
							//don't resets the timestamps
							plNetClientCommResponse(sock,&session[*sid]);
						} else {
							//resets the timestamps
							plNetClientComm(sock,&session[*sid]);
						}
						//also reset client vars --
						session[*sid].old_p_n=session[*sid].client.p_n; //packet number
						session[*sid].old_sn=session[*sid].client.sn; //init the seq num
						session[*sid].negotiated=1; //set true
					}
					n=0; //normally with negotation, you only need to answer with an ack
				} else { //it's a normal 2nd classe packet != negotation
					//AGING FILTER ~deletes old packets~
#if 0
					if(session[*sid].client.p_n<session[*sid].old_p_n) {
						stamp2log(f_uru);
						print2log(f_uru,"INF: Discarded and old packet\n");
						//Discard OLD packet
						n=0;
					} else {
						//update the oldest packet number
						session[*sid].old_p_n=session[*sid].client.p_n;
					//end aging filter
						//printf("plNetRcv step3\n"); fflush(0);
						//consistency filter ~checks for packets caused
						//due to a dropped ack sent by server
						if(session[*sid].client.sn<session[*sid].old_sn) {
							stamp2log(f_uru);
							print2log(f_uru,"INF: Dropped old packet by sn\n");
							n=0;
						} else {
							session[*sid].old_sn=session[*sid].client.sn; //update it
				//printf("plNetRcv step4\n"); fflush(0);
							//number of fragments sanity check
							if(session[*sid].client.fr_n!=0 && session[*sid].client.fr_t!=session[*sid].old_fr_t) {
								stamp2log(f_uru);
								print2log(f_uru,"INF: Dropped invalid packet with unexpected fragment count\n");
							} else {
								session[*sid].old_fr_t=session[*sid].client.fr_t; //update & init it
							}
							//end of n_frg sanity check
						}
						//end consistency filter

					} //end of all filters
#endif
				}
				//printf("plNetRcv step2\n"); fflush(0);
			}
		}
		else { //Unexpected packet
			//print2log(f_une,"\n");
			stamp2log(f_une);
			ip2log(f_une,&session[*sid]);
			print2log(f_une,"[sid:%i] ",*sid);
			print2log(f_une,"ERR: Recieved an unexpected packet from client!\n");
			uru_print_header(f_une,&session[*sid].client);
			print2log(f_une,"\n");
		}
	}

	//printf("Negotiate check..."); fflush(0);
	//if the packet passed all quality checks then check if we have the negotiated var to true
	if(n!=0 && session[*sid].negotiated==0) {
		//then request a negotiation (that will reset any server var
		DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);
		DBG(3,"Sending a negotation packet, because we are not in sync with client\n");
		plNetClientComm(sock,&session[*sid]);
		DBG(3,"Negotation packet sent...\n");
		session[*sid].negotiated=1; //set true
	}

	/*
		BUG: Terrible problem/bug, if the first packet of a fragmented stream is dropped
		then the entire packet will be ignored by the server!!!
		Also another problem, is that if the same packet is dropped lots of times and
		the client sends a new fresh packet, then again the old packet will be ignored
		by the server.
	*/

	DBG(3,"before the fragmentator\n");

#if 0
	//Fragmented packets control
	if(n!=0) {
		//printf("frag step 1\n"); fflush(0);
		if(session[*sid].client.fr_n==0) {
			session[*sid].client.fr_count=0;
			session[*sid].fr_sum_check=0; //init
			//this is the first packet of a stream
			//delete any previous buffer, and create a new one (**EXPERIMENTAL**)
			DBG(3,"[%i] delete any previous buffer and create a new one...\n",*sid);
			if(session[*sid].rcv_msg!=NULL) {
				DBG(4,"before free\n");
				free((void *)session[*sid].rcv_msg); //aliberate the buffer allocation
				DBG(4,"after free\n");
				DBG(3,"old buffer deleted\n");
			}
			DBG(3,"frag step 2 - creating new buffer\n");
			//create the new buffer
			DBG(3,"just before a malloc\n");
			DBG(4,"attemtting to allocate %i bytes\n",((session[*sid].client.fr_t+1)*OUT_BUFFER_SIZE));
			session[*sid].rcv_msg=(Byte *)malloc(((session[*sid].client.fr_t+1)*OUT_BUFFER_SIZE)*sizeof(Byte));
			DBG(3,"just after a malloc\n");
			if(session[*sid].rcv_msg==NULL) {
				print2log(f_err,"WAR: Warning, insufficient memory!!!\n");
				logflush(f_err);
			}
		}
		DBG(3,"frag step 3\n");
		if(session[*sid].rcv_msg!=NULL && session[*sid].client.size<OUT_BUFFER_SIZE) {
		memcpy(session[*sid].rcv_msg+(session[*sid].client.fr_n*(OUT_BUFFER_SIZE-i)),buf+i,session[*sid].client.size);
		}
		DBG(3,"frag step 4\n"); fflush(0);
		if(session[*sid].client.fr_count==session[*sid].client.fr_t) {
			n=(session[*sid].client.fr_t*(OUT_BUFFER_SIZE-i))+session[*sid].client.size;
			DBG(3,"\n####################################n:%i###############\n",n);
			//dump_packet(f_uru,session[*sid].rcv_msg,(session[*sid].client.fr_t*(OUT_BUFFER_SIZE-i))+session[*sid].client.size,0,7);
		} else {
			n=0;
		}
				//printf("frag step 5\n"); fflush(0);
		//integrity check
		//TODO

		/*
		if(session[*sid].fr_sum_check== ((session[*sid].client.fr_n)*session[*sid].client.fr_n+1)/2) */

		session[*sid].fr_sum_check+=session[*sid].client.fr_n;
		session[*sid].client.fr_count++;
	}
#endif

#if 1
	//Fragmented packets control
	if(n!=0) {
		DBG(3,"frag step 1\n");

		DBG(4,"Doing Timestamp check...\n");
		//delete old msg's
		if(session[*sid].current_msg!=NULL && (session[0].timestamp-session[*sid].current_msg->stamp)>5) {
			my_msg=session[*sid].current_msg;
			//swap it
			session[*sid].current_msg=(st_uru_msg_r *)my_msg->next;
			DBG(5,"before a free...\n");
			if(my_msg->buf!=NULL) {
				free((void *)my_msg->buf);
			}
			free((void *)my_msg);
			DBG(5,"after a free...\n");
			print2log(f_uru,"INF: Dropped old packet by timestamp\n");
		}

		DBG(4,"checking if is the 1st message of the cue...\n");
		//is the 1st message of the cue
		if(session[*sid].current_msg==NULL) {
			DBG(4,"its the first message, creating it...\n");
			//create the first message
			session[*sid].current_msg=(st_uru_msg_r *)malloc(sizeof(st_uru_msg_r) * 1);
			if(session[*sid].current_msg==NULL) {
				print2log(f_err,"WAR: Warning, insufficient memory!!!\n");
				logflush(f_err);
			}
			my_msg=session[*sid].current_msg;
			my_msg->next=NULL;
			my_msg->buf=NULL;
			my_msg->msg_n=session[*sid].client.sn;
			my_msg->size=0;
		} else {
			DBG(4,"Nope, this is a partial completed message, so current_msg!=NULL is true\n");
			my_msg=session[*sid].current_msg;
			if(my_msg->msg_n>session[*sid].client.sn) {
				print2log(f_uru,"INF: Dropped old packet by sn\n");
				n=0;
			}
		}

		DBG(4,"Message Assembler, n:%i\n",n);
		if(n!=0) {

			DBG(5,"Starting message search loop...\n");
			while(my_msg!=NULL && my_msg->msg_n!=session[*sid].client.sn) {
				DBG(5,"Searching for a message...\n");
				my_msg_aux=my_msg;
				my_msg=(st_uru_msg_r *)my_msg->next; //next one
			}

			if(my_msg==NULL) { //not found!
				DBG(5,"Message not found, creating a new one...\n");
				//create it
				my_msg_aux->next=(void *)malloc(sizeof(st_uru_msg_r) * 1);
				my_msg=(st_uru_msg_r *)my_msg_aux->next;
				my_msg->next=NULL;
				my_msg->buf=NULL;
				my_msg->msg_n=session[*sid].client.sn;
				my_msg->size=0;
			} //else //found

			if(my_msg->buf==NULL) {
				//create the new buffer
				DBG(5,"my_msg->buf is NULL, creating a new one\n");
				DBG(3,"just before a malloc\n");
				//Found a big problem here, tracking server don't wants to allocate 1024 bytes
				//Endless loop inside the Malloc??
				DBG(4,"attemtting to allocate %i bytes\n",((session[*sid].client.fr_t+1)*OUT_BUFFER_SIZE));
				my_msg->buf=(Byte *)malloc(((session[*sid].client.fr_t+1)*OUT_BUFFER_SIZE)*sizeof(Byte));
				DBG(3,"just after a malloc\n");
				if(my_msg->buf==NULL) {
					print2log(f_err,"WAR: Warning, insufficient memory!!!\n");
					logflush(f_err);
				}
				my_msg->fr_count=0;
				bzero(my_msg->check,33);
			}
			//update timestamp
			DBG(5,"Updating timestamp...\n");
			time((time_t *)&my_msg->stamp);
			DBG(3,"frag step 3\n");

			DBG(5,"Meesage bitmap check...\n");
			if(!((my_msg->check[session[*sid].client.fr_n/8]>> (session[*sid].client.fr_n%8)) & 0x01)) {
				//update algorithm
				my_msg->check[session[*sid].client.fr_n/8] |= (0x01<<(session[*sid].client.fr_n%8));
				DBG(5,"Bitmap check success.., updated bitmap\n");

				memcpy(my_msg->buf+(session[*sid].client.fr_n*(OUT_BUFFER_SIZE-i)),\
buf+i,session[*sid].client.size);
				DBG(3,"frag step 4\n");

				DBG(6,"client.fr_t:%i,my_msg->fr_count:%i\n",session[*sid].client.fr_t,my_msg->fr_count);
				if(session[*sid].client.fr_t==my_msg->fr_count) {
					my_msg->size=(session[*sid].client.fr_t*(OUT_BUFFER_SIZE-i))\
	+session[*sid].client.size;
	DBG(3,"\n###############################n:%i##size:%i#############\n",n,my_msg->size);
	//dump_packet(f_uru,session[*sid].rcv_msg,(session[*sid].client.fr_t*(OUT_BUFFER_SIZE-i))+session[*sid].client.size,0,7);

					if(session[*sid].bussy==0) {
						DBG(4,"before some mallocs and frees\n");
						DBG(5,"my_msg->size is %i\n",my_msg->size);
						if(my_msg==session[*sid].current_msg) {
							session[*sid].current_msg=(st_uru_msg_r *)my_msg->next;
							DBG(5,"my_msg->size is %i\n",my_msg->size);
						} else {
							my_msg_aux->next=my_msg->next;
							DBG(5,"my_msg->size is %i\n",my_msg->size);
						}
						DBG(5,"my_msg->size is %i\n",my_msg->size);
						if(session[*sid].rcv_msg!=NULL) {
							DBG(5,"my_msg->size is %i\n",my_msg->size);
							free((void *)session[*sid].rcv_msg);
						}
						DBG(5,"my_msg->size is %i\n",my_msg->size);
						session[*sid].rcv_msg=my_msg->buf;
						DBG(5,"my_msg->size is %i\n",my_msg->size);
						n=my_msg->size;
						free((void *)my_msg);
						DBG(5,"my_msg->size is %i\n",my_msg->size);
						DBG(4,"after some mallocs and free's\n");
						DBG(5,"n is %i,my_msg->size is %i\n",n,my_msg->size);
						DBG(5,"n is %i,my_msg->size is %i\n",n,my_msg->size);
					} else {
						DBG(4,"The resource It's bussy...\n");
						//abort();
						n=0;
					}

				} else {
					DBG(4,"Message stream is still not completed...\n");
					n=0;
					my_msg->fr_count++;
				}
			} else {
				DBG(5,"A packet was discarded by bitmap check, this can only happen if it was repeated\n");
				print2log(f_uru,"INF: Discarded a fragmented packet by bitmap check\n");
				n=0;
			}
		}
	}
#endif

if(n==0) {
	for(i=1; i<=global_client_count; i++) {
		if(session[i].bussy==0) {
			my_msg=session[i].current_msg;
			while(my_msg!=NULL && my_msg->size==0) {
				if(my_msg->next==NULL) { break; }
				my_msg_aux=my_msg;
				my_msg=(st_uru_msg_r *)my_msg->next;
			}
			if(my_msg!=NULL && my_msg->size!=0) {
				DBG(4,"Found a message in the que, decuing it...\n");
				*sid=i;
				DBG(4,"before some mallocs and frees\n");
				if(my_msg==session[*sid].current_msg) {
					session[*sid].current_msg=(st_uru_msg_r *)my_msg->next;
				} else {
					my_msg_aux->next=my_msg->next;
				}
				if(session[*sid].rcv_msg!=NULL) {
					free((void *)session[*sid].rcv_msg);
				}
				session[*sid].rcv_msg=my_msg->buf;
				n=my_msg->size;
				free((void *)my_msg);
				DBG(4,"after some mallocs and free's\n");
			}
		}
	}
}


	//the process continues
	if(session[*sid].rcv_msg==NULL) {
		DBG(2,"NULL rcv_msg buffer?\n");
	}

	//DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);

	DBG(2,"returning from the message processor RET:%i\n",n);
	if(n>256 * 1024) {
		DBG(5,"Abort Condition, Message too big!\n");
		abort();
	}

	if(n!=0) {
		stamp2log(f_uru);
		ip2log(f_uru,&session[*sid]);
		print2log(f_uru,"<- Processing a message from this host...\n");
	}

	return n;
}

////------------->>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

int plNetConnect(int sock,int * sid, st_uru_client ** v_session,Byte * address,U16 port) {
	//int n,s_n,i; //size of packet and iterator
	int i;
	//int ret; //joker variable

	int s_old; //identifier for and old session found
	int s_new; //identifier for a void slot session found

	//cutre fix
	st_uru_client * session;
	session=*v_session;
	//cutre fix

	//WARNING - WARNING - WARNING - BE CAREFULL WITH THESE STATICS...
	//static Byte ack_buf[OUT_BUFFER_SIZE]; //buffer for ack packets
	static Byte buf[BUFFER_SIZE]; //internal only now

	struct sockaddr_in client; //client struct

	socklen_t client_len; //client len size

	client_len=sizeof(struct sockaddr_in); //get client struct size


	struct hostent *host;
	host=gethostbyname((char *)address);
	if(host==NULL) {
		return -1;
	}

	client.sin_family=AF_INET; //UDP IP
	client.sin_addr.s_addr=*(U32 *)host->h_addr_list[0];
	client.sin_port=htons(port);


	static Byte randomizer=0;
	randomizer++;
	int e;

	st_uru_msg_r * my_msg;
	st_uru_msg_r * my_msg_aux;

	s_old=0;
	s_new=0;

	for(i=1; i<=global_client_count; i++) {
		DBG(4,"\n###################Checking %i###############\n",i);
		if(session[i].flag==0) {
			if(s_new==0) { //new lowest session found
				DBG(4,"\n#############LOWEST NEW SID FOUNDm %i################\n",i);
				s_new=i;
				//session[i].flag=1;
			} else {
				if(session[i-1].client_ip==0 && i!=(global_the_bar+1)) session[i-1].flag=0; //go back..
			}
			break; //al job done!
		}
		DBG(3,"%08X - %04X VS %08X - %04X\n",session[i].client_ip,session[i].client_port,client.sin_addr.s_addr,client.sin_port);
		if(session[i].client_ip==client.sin_addr.s_addr && session[i].client_port==client.sin_port) {
			s_old=i;
			DBG(4,"\n###################OLD SID FOUND %i################\n",i);
		} else if(i>global_the_bar){
			//if(session[i].client_ip!=0 &&
			if((session[0].timestamp-session[i].timestamp)>(global_connection_timeout*60)) {
				//send a TERMINATED:Timeout here if we are recycling an old session
				if(session[i].client_ip!=0) {
					plNetMsgTerminated(sock, RTimedOut, &session[i]);
				}
				//delete session vars
				//plNetDisconnect(&session[i]);
				session[i].client_ip=0; //delete the ip (client loged out)
				if(i==global_client_count || session[i+1].flag==0) session[i].flag=0;
				//TODO: empty the send buffers
			}
			if(session[i].client_ip==0) {
				if(s_new==0) {
					s_new=i; //The lowest session
					DBG(4,"\n#############LOWEST NEW SID FOUND %i################\n",i);
				}
			}
		}
		if(session[i].client_ip!=0) {
			//TODO: RESEND NOT ACKED PACKETS *HERE*
			//check the timeout and the window size
		}
	}

	//patch-fix-cutresoft software
	if(s_old==0 && s_new==0) {
		if(global_client_count<global_max_clients) {
			global_client_count++;
			DBG(3,"After a realloc...\n");
			*v_session=(st_uru_client *)realloc((void *)(*v_session),sizeof(st_uru_client)*(global_client_count+1));
			session=*v_session;
			//hmmm - this should fix all problems
			DBG(4,"Setting rcv_msg to NULL...\n");
			session[global_client_count].rcv_msg=NULL;
			session[global_client_count].snd_msg=NULL;
			session[global_client_count].p_list=NULL;
			session[global_client_count].current_msg=NULL;
			//hmmm
			for(e=0; e<MAX_PACKETS_IN_BUFFER; e++) {
				session[i].out[e][0]=0x00; //reset the flags
			}
			for(e=0; e<MAX_PACKETS_PENDING; e++) {
				session[i].pending[e].flag=0; //reset the flags
				session[i].pending[e].pnd_msg=NULL;
			}
			DBG(3,"Before a realloc...\n");
			s_new=global_client_count;
		}
	}
	//end fix

	//put the correct ip and port stuff here


	//now we have the session - init the vars -
	if(s_old!=0) {
		//old session found
		*sid=s_old;
		print2log(f_uru,"\n____________________________________________________\n");
		stamp2log(f_uru);
		ip2log(f_uru,&session[*sid]);
		print2log(f_uru,"[sid:%i] INF: Old Outcomming connection\n",*sid);
		memcpy(session[*sid].sock_array,&client,sizeof(struct sockaddr_in));
		//time(&session[*sid].timestamp); //update the _session_ timestamp (is done above!)
		//also I will add the microseconds update!
		//session[*sid].microseconds=get_microseconds();
	} else if(s_new!=0) {
		//new session found
		session[s_new].flag=1;
		*sid=s_new;
		//Initialitze the data structures
		session[*sid].client_ip=client.sin_addr.s_addr; //ip
		session[*sid].client_port=client.sin_port; //port
		session[*sid].authenticated=0; //be sure that the client is not authenticated
		session[*sid].major_version=0;
		session[*sid].minor_version=0;

		session[*sid].sid=*sid; //<- store the unique session id

		//time(&session[*sid].client.timestamp); //init timestamps to current time
		//session[*sid].server.timestamp=session[*sid].client.timestamp;
		//session[*sid].timestamp=session[*sid].server.timestamp;
		//session[*sid].server.microseconds=get_microseconds();
		//session[*sid].client.microseconds=session[*sid].server.microseconds;
		print2log(f_uru,"\n____________________________________________________\n");
		stamp2log(f_uru);
		ip2log(f_uru,&session[*sid]);
		print2log(f_uru,"[sid:%i] INF: New Outcomming connection\n",*sid);
		stamp2log(f_acc);
		ip2log(f_acc,&session[*sid]);
		print2log(f_acc,"[sid:%i] New Outcomming connection\n",*sid);
		logflush(f_acc);
		//initzalitze the server structure //buf[1] -> ch_byte
		uru_init_header(&session[*sid].server,buf[1]); //this sets the server ack flags to 0
		session[*sid].old_p_n=0; //reset the agging control
		session[*sid].old_sn=0; //reset the max packet seq number recieved
		session[*sid].negotiated=0; //reset the negotation flag
		session[*sid].tpots=0; //tpots mod
		session[*sid].paged=0; //player not paged in
		session[*sid].ki=0;
		session[*sid].bussy=0; //it's bussy?
		session[*sid].load=0; //number of peers (used in the tracking server)
		session[*sid].joined=0; //Is the player joined?
		session[*sid].status=0;
		time((time_t *)&session[*sid].online_time); //Set the Online time

		for(i=0; i<MAX_PACKETS_IN_BUFFER; i++) {
			session[*sid].out[i][0]=0x00;
		}

		//memcpy(&session[*sid].a_client,&client,sizeof(struct sockaddr_in));
		memcpy(session[*sid].sock_array,&client,sizeof(struct sockaddr_in));

		//session[*sid].a_client=client;
		session[*sid].a_client_size=client_len;

//---
						//destroy messages
						i=*sid;
						DBG(3,"I'm going to do a free call...\n");
						if(session[i].rcv_msg!=NULL) {
							DBG(3,"before free call...\n");
							free((void *)session[i].rcv_msg);
							session[i].rcv_msg=NULL;
							DBG(3,"after free call...\n");
						}
						if(session[i].snd_msg!=NULL) {
							DBG(3,"before free call...\n");
							free((void *)session[i].snd_msg);
							session[i].snd_msg=NULL;
							DBG(3,"after free call...\n");
						}
						DBG(3,"before several free calls\n");
						for(e=0; e<MAX_PACKETS_PENDING; e++) {
							if(session[i].pending[e].pnd_msg!=NULL) {
								free((void *)session[i].pending[e].pnd_msg);
								session[i].pending[e].pnd_msg=NULL;
							}
						}
						DBG(3,"after several free calls\n");
						//now destroy the message cue
						DBG(3,"destroying message cue..\n");
						my_msg=session[i].current_msg;
						while(my_msg!=NULL) {
							if(my_msg->buf!=NULL) {
								free((void *)my_msg->buf);
							}
							my_msg_aux=my_msg;
							my_msg=(st_uru_msg_r *)my_msg->next; //the next message
							free((void *)my_msg_aux);
						}
						session[i].current_msg=NULL;
						DBG(3,"message cue succesfully destroyed!\n");
//---


	} else {
		//the server is kao
		*sid=0;
		if(session[0].client_ip!=client.sin_addr.s_addr) {
			stamp2log(f_acc);
			//ip2log(f_acc,&session[*sid]);
			ip2log_old(f_acc,client.sin_addr.s_addr,client.sin_port);
			print2log(f_acc,"[sid:%i] New Outcomming connection (server overloaded)\n",*sid);
			logflush(f_acc);
			//initzialitze the server structure to '0' //buf[1] -> ch_byte
			uru_init_header(&session[*sid].server,buf[1]);
			session[*sid].old_p_n=0; //reset the agging control
			session[*sid].old_sn=0; //reset the max packet seq number recieved
			session[*sid].negotiated=0; //reset the negotation flag

			for(i=0; i<MAX_PACKETS_IN_BUFFER; i++) {
				session[*sid].out[i][0]=0x00;
			}
		}
		//Initialitze the data structures
		session[*sid].client_ip=client.sin_addr.s_addr; //ip
		session[*sid].client_port=client.sin_port; //port
		DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);
		session[*sid].authenticated=0; //be sure that the client is not authenticated
		session[*sid].major_version=0;
		session[*sid].minor_version=0;
		print2log(f_uru,"\n____________________________________________________\n");
		stamp2log(f_uru);
		ip2log(f_uru,&session[*sid]);
		print2log(f_uru,"[sid:%i] INF: Maxium number of clients reached, ignoring client\n",*sid);

		//memcpy(&session[*sid].a_client,&client,sizeof(struct sockaddr_in));
		memcpy(session[*sid].sock_array,&client,sizeof(struct sockaddr_in));

		//session[*sid].a_client=client;
		session[*sid].a_client_size=client_len;

		DBG(6,"[%i] ip is %08X\n",*sid,session[*sid].client_ip);
	}

	time(&session[*sid].timestamp);
	plNetClientComm(sock,&session[*sid]);

	return 0;
}





////------------------->>>>>>>>>>>>>>>>><


/*---------------------------------------------------------------------
	Unimplemented, TODO: this one must store the packets in a send cache
---------------------------------------------------------------------*/
int plNetSend(int sock, Byte * buf, int n,st_uru_client * u) {
	//OK WE are going to do the hard stuff here
	int he_sz; //header size
	int pkt_sz; //packet size
	Byte n_packets; //number of fragmented packets
	Byte i,e;
	int num; //number of bytes to copy

	//first before sending a packet we need to check 2 things
	//1) non-acked packets
	//2) non-send old packets

	/*DBG(3,"1st Check non-acked packets...\n");

	if(plNetReSendAckPackets(sock,u)) { //n=0 fake packets, argh!!
		print2log(f_uru,"INF: There are pending packets to ack, storing packet in buffer\n");
		if(n>0) {
			for(e=0; e<MAX_PACKETS_PENDING; e++) {
				if(u->pending[e].flag==0) {
					//slot found, storing message
					DBG(3,"free slot found, storing message...\n");
					u->pending[e].flag=1;
					if(u->pending[e].pnd_msg!=NULL) {
						free((void *)u->pending[e].pnd_msg);
						u->pending[e].pnd_msg=NULL;
					}
					u->pending[e].pnd_msg=(Byte *)malloc(sizeof(Byte)*n);
					u->pending[e].size=n;
					u->pending[e].t=u->server.t;
					if(u->pending[e].pnd_msg==NULL) {
						print2log(f_err,"ERR: Fatal not enough memory to send a message\n");
						u->pending[e].flag=0;
					} else {
						memcpy(u->pending[e].pnd_msg,buf,n);
						u->pending[e].flag=1;
					}
					//break;
					return 0;
				}
			}
			print2log(f_err,"ERR: Fatal not enough space in buffer to store a message!\n");
		}
		return 0;
		//nothing else
	}

	int my_max=0;
	int my_min=1;
	int min_id=0;

	DBG(3,"2nd search for buffered packets...\n");
	//now search for a previous non-sended message and send it
	for(i=0; i<MAX_PACKETS_PENDING; i++) {
		if(u->pending[i].flag!=0) {
			if(u->pending[i].flag>my_max) {
				my_max=u->pending[i].flag;
			}
			if(u->pending[i].flag<=my_min) {
				my_min=u->pending[i].flag;
				min_id=i;
			}
		}
	}

	DBG(3,"results: my_max=%i,my_min=%i,min_id=%i\n",my_max,my_min,min_id);

	Byte * aux_buf;
	int aux_size;
	Byte aux_t;

	if(my_max>0) {
		//found a previous message
		DBG(4,"found a previous message\n");
		aux_buf=buf;
		aux_size=n;
		aux_t=u->server.t;
		//swap it
		n=u->pending[min_id].size;
		buf=u->pending[min_id].pnd_msg;
		u->server.t=u->pending[min_id].t;
		//--
		if(aux_size>0) {
			u->pending[min_id].pnd_msg=(Byte *)malloc(sizeof(Byte) * aux_size);
			u->pending[min_id].size=aux_size;
			u->pending[min_id].t=aux_t;
			if(u->pending[min_id].pnd_msg==NULL) {
				print2log(f_err,"ERR: Fatal not enough memory to send a message\n");
				u->pending[min_id].flag=0;
			} else {
				memcpy(u->pending[min_id].pnd_msg,aux_buf,aux_size);
				u->pending[min_id].flag=my_max+1;
			}
		} else {
			u->pending[min_id].flag=0;
		}
	} else {
		if(n<=0) {
			return 0; //don't do nothing.., was just a forzed re-send
		}
	}

	if(n<=0) {
		print2log(f_err,"WARNING: Sender Caught a zero sized packet!\n");
		return 0; //argh!!
	}*/

	//Byte bufe[MAX_PACKET_SIZE];

	//uru_inc_msg_counter(&u->server); //inc the p_n and the sn..

	he_sz=uru_get_header_start(&u->server); //get the header size

	//quick nasty hack
	//buf=buf+he_sz; //ummmm :/?

	pkt_sz=MAX_PACKET_SIZE-he_sz; //packet data size

	n_packets=(((n-he_sz)-1)/pkt_sz); //get the number of packets

	u->server.fr_t=n_packets; //put the number of frag packets

	u->server.sn++; //inc the seq num

	if(u->server.t==0x02) {

		i=0;

		//print2log(f_uru,"MARCO\n");
		//dump_packet(f_uru,buf,n,0,7);
		//print2log(f_uru,"\n");
		//logflush(f_uru);

		//search for a free slot to store the packet
		for(e=0; e<MAX_PACKETS_IN_BUFFER; e++) {
			//printf("----%i----\n",e);
			//fflush(0);
			if(u->out[e][0]==0x00 || u->out[e][0]==0x02) {
				//free slot found
				u->server.fr_n=i; //put the fragment number
				u->server.p_n++; //tmp inc the packet counter

				if(i==n_packets) { //some hacks here
					num=(n-he_sz)-(i*(MAX_PACKET_SIZE-he_sz)); //this should compute the rest of the last packet
				} else {
					num=MAX_PACKET_SIZE-he_sz; //this should compute the avg packet size
				}

				u->server.size=num; //update the size of the data

				uru_put_header(u->out[e],&u->server); //puts the header into the buffer

				//buf+he_sz
				//printf("I'm here, just before a memcpy\n"); fflush(0);
				memcpy(u->out[e]+he_sz,(buf+he_sz)+(i*(MAX_PACKET_SIZE-he_sz)),(num)*sizeof(Byte)); //copy the buffer
				//printf("I'm here, just after a memcpy\n"); fflush(0);

				print2log(f_uru,"Sending... %i\n",num+he_sz);
				//dump_packet(f_uru,u->out[e],num+he_sz,0,7);
				//print2log(f_uru,"\n");
				logflush(f_uru);
				uru_net_send(sock,u->out[e],num+he_sz,u); //and send it
				//printf("after sending it\n");
				//fflush(0);
				u->out[e][0]=0x01;

				if(i>=n_packets) break;

				i++;

			}
		}
		if(e>=MAX_PACKETS_IN_BUFFER) {
			stamp2log(f_uru);
			print2log(f_uru,"ERR: Warning, buffer overloaded!!\n");
		}
	} else {
			u->server.fr_n=0; //put the fragment number to 0,
			//this is the first packet, and there shouldn't be non ack packets of a size more than 1024 bytes
			//the next net core will allow fragmented streems for non-ack packets
			u->server.p_n++; //tmp inc the packet counter
			//num=n-(i*MAX_PACKET_SIZE); //this should compute the rest of the last packet
			u->server.size=n-he_sz; //update the size of the data
			uru_put_header(buf,&u->server); //puts the header into the buffer
			uru_net_send(sock,buf,n,u); //and send it
	}

		//printf("POLO\n");
		//fflush(0);


	/*
	for(i=0; i<=n_packets; i++) {
		u->server.fr_n=i; //put the fragment number
		//u->server.p_n++; //tmp inc the packet counter

		if(i==n_packets) {
			num=n-(i*MAX_PACKET_SIZE); //this should compute the rest of the last packet
		} else {
			num=MAX_PACKET_SIZE-he_sz; //this should compute the avg packet size
		}

		u->server.size=num; //update the size of the data


		uru_put_header(bufe,&u->server); //puts the header into the buffer

		memcpy(bufe,buf+(i*(MAX_PACKET_SIZE-he_sz)),(num+he_sz)*sizeof(Byte)); //copy the buffer

		//uru_net_send(sock,bufe,num,u); //and send it

	}
	*/


	//return(uru_net_send(sock,buf,n,u));
	return 0;
}

/*-----------------------------------------------
	Sends a negotation request
	(returns the error code returned by net_send)
------------------------------------------------*/
int plNetClientComm(int sock, st_uru_client * u) {
	int n;
	Byte buf[OUT_BUFFER_SIZE];

	//initzialitze the server flags to zero, this is the start of the transaction
	//if(u->negotiated==0) {
		time(&u->server.timestamp); //init timestamps to current time
		u->server.microseconds=get_microseconds();

		//
		//u->nego_timestamp=u->server.timestamp;
		//u->nego_microseconds=u->server.microseconds;
	//} else {
		//u->negotiated=0;
	//}
	//initzalitze the server structure to '0' //buf[1] -> ch_byte
	uru_init_header(&u->server,u->validation);
	u->old_p_n=0; //the agging packet counter protection
	u->old_sn=0; //bis
	n=uru_get_negotation_reply(buf,u);
	return(plNetSend(sock,buf,n,u));
}

/*-----------------------------------------------
	Sends a negotation response
	(returns the error code returned by net_send)
------------------------------------------------*/
int plNetClientCommResponse(int sock, st_uru_client * u) {
	int n;
	Byte buf[OUT_BUFFER_SIZE];

	//initzialitze the server flags to zero, this is the start of the transaction
	//if(u->negotiated==0) {
		//time(&u->server.timestamp); //init timestamps to current time
		//u->server.microseconds=get_microseconds();
	//} else {
		//u->negotiated=0;
	//}
	//initzalitze the server structure to '0' //buf[1] -> ch_byte
	//uru_init_header(&u->server,u->validation);
	//u->old_p_n=0; //the agging packet counter protection
	//u->old_sn=0; //bis
	n=uru_get_negotation_reply(buf,u);
	return(plNetSend(sock,buf,n,u));
}


int plNetAdvMsgSender(int sock,Byte * buf,int size,st_uru_client * u) {
	int n;

	/*if(u->adv_msg.format & 0x00040000) {
		u->server.t=0x02; //ack on
	} else {
		u->server.t=0x00; //ack off
	}*/

	//udpate the header flags with the correct size and all other data
	uru_put_header(buf,&u->server);

	stamp2log(f_uru);
	ip2log(f_uru,u);
	print2log(f_uru,"<- Sending packet to %s,%s\n",u->login,u->avatar_name);
#if 1
	print2log(f_uru,"\n");
	dump_packet(f_uru,buf,size,0,7);
	print2log(f_uru,"\n-------------\n");
#endif

	//and send the msg
	n=plNetSend(sock,buf,size,u);

	return n;
}


/*---------------------------------------------------------------
	Sends the TERMINATED message
----------------------------------------------------------------*/
int plNetMsgTerminated(int sock, int code, st_uru_client * u) {
	int start;
	int off;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgTerminated; //0x02C6
	u->adv_msg.format=0x00021000; //flags ki + custom
	u->server.t=0x00; //ack off

	//yes this one is really important
	u->server.ch=u->validation; //set the validation level of the packet

	///gets the size of the header
	start=uru_get_header_start(&u->server);
	off=start;

	print2log(f_uru,"<SND> Terminated. Reason [%i] %s ",\
	code,unet_get_reason_code(code));

	off+=put_plNetMsg_header(buf+off,u);

	*(buf+off)=(Byte)code; //the reason
	off++;

	//set total raw size
	u->server.size=off-start; //Now, we have the correct size of the packet

	off=plNetAdvMsgSender(sock,buf,off,u);

	//active player
	if(track!=NULL && u->ki>=500 && code!=RLoggedInElsewhere) {
		//send to the tracking server the unload petition
		track->adv_msg.ki=u->ki; //select the victim
		plNetMsgCustomPlayerStatus(sock,u->guid,u->login,u->avatar_name,0,0,track);
	}
	if(vault!=NULL && u->ki>=500) {
		vault->adv_msg.ki=u->ki;
		U32 timer;
		time((time_t *)&timer);
		timer=timer-u->online_time;
		plNetMsgCustomVaultPlayerStatus(sock,(Byte *)"",(Byte *)"0000000000000000",0,timer,vault);
	}

	//log out the client
	u->authenticated=0;
	u->client_ip=0;
	u->paged=0;
	u->ki=0;
	strcpy((char *)u->login,"");
	bzero(u->guid,16);
	//
	return off;
}

/*---------------------------------------------------------------
	Sends the TERMINATED message
----------------------------------------------------------------*/
int plNetMsgPlayerTerminated(int sock, int code, st_uru_client * u) {
	int start;
	int off;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgPlayerTerminated; //0x02C6
	u->adv_msg.format=0x00021000 | plNetVersion; //flags ki + custom
	u->server.t=0x00; //ack off

	//yes this one is really important
	u->server.ch=u->validation; //set the validation level of the packet

	///gets the size of the header
	start=uru_get_header_start(&u->server);
	off=start;

	print2log(f_uru,"<SND> NetMsgPlayerTerminated. Reason [%i] %s ",\
	code,unet_get_reason_code(code));

	off+=put_plNetMsg_header(buf+off,u);

	*(buf+off)=(Byte)code; //the reason
	off++;

	//set total raw size
	u->server.size=off-start; //Now, we have the correct size of the packet

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}


#endif

