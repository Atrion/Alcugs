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

#ifndef __U_URUNET_H_
#define __U_URUNET_H_
/* CVS tag - DON'T TOUCH*/
#define __U_URUNET_H_ID "$Id$"

/*this number, is where all the server permonance resides, more clients implies more memory,
  more resources, more load, more dropped packets, more problems, more bugs, more crashes...
	Put a big number and you will see what I'm saying...
*/

//udp packet max buffer size (0xFFFF) - any packet should be bigger.
#define BUFFER_SIZE 65535
#define OUT_BUFFER_SIZE 1024

/* Includes */
#include<netdb.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>
#include <unistd.h>
#include <fcntl.h>


#include "config_parser.h"
#include "tmp_config.h"
#include "stdebug.h"
#include "conv_funs.h"
#include "protocol.h"
#include "files.h"


void unet_set_recv_host(st_uru_client * u);
int unet_set_host_info(char * hostname,int port,st_uru_client * u);
int plNetCheckAckDone(st_uru_client * u);
/*------------------------------------------------------------------------
  Sends and encrypts(if necessary) the correct packet
	Returns size if all gone well
	returns <0 if something went wrong
	(this is a low level function)
------------------------------------------------------------------------*/
int uru_net_send(int sock,Byte * buf,int n,st_uru_client * u);

/****************************************************************
High level functions

****************************************************************/
/*------------------------------------------------------
 Initialitzes the session default values
------------------------------------------------------*/
void u_init_session(st_uru_client * session,int max);

/*----------------------------------------------------
  Starts the network operation
	returns -1 on error
----------------------------------------------------*/
int plNetStartOp(U16 port,char * hostname);

/*----------------------------------------------------*/
/** Substititon of the recvmsg, the high level one
	(one step more and we will do a class)

	stores the session id in the sid.

	\return
	 returns -1 if error\n
	 returns size of packet if success
*/
/*-----------------------------------------------------*/
int plNetRecv(int sock,int * sid, st_uru_client ** session);

int plNetConnect(int sock,int * sid, st_uru_client ** v_session,Byte * address,U16 port);

/*---------------------------------------------------------------------
	Unimplemented, TODO: this one must store the packets in a send cache
---------------------------------------------------------------------*/
int plNetSend(int sock, Byte * buf, int n,st_uru_client * u);

/*-----------------------------------------------
	Sends a negotation response
	(returns the error code returned by net_send)
------------------------------------------------*/
int plNetClientComm(int sock, st_uru_client * u);
int plNetClientCommResponse(int sock, st_uru_client * u);
// Sends a message with the adv msg sender
int plNetAdvMsgSender(int sock,Byte * buf,int size,st_uru_client * u);

int plNetMsgTerminated(int sock, int code, st_uru_client * u);
int plNetMsgPlayerTerminated(int sock, int code, st_uru_client * u);
#endif

