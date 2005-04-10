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

/* The Uru protocol, is here */

/*
	This is only the protocol.
	No sockets here, Please!!
*/

#ifndef __U_PROTOCOL_H_
#define __U_PROTOCOL_H_
/* CVS tag - DON'T TOUCH*/
#define __U_PROTOCOL_H_ID "$Id$"
#define __U_PROTOCOL_V "1.3.1c"
#define __U_PROTOCOL_VMAX "12.07"
#define __U_PROTOCOL_VMIN "12.00"

//disable checksum checks
//#define _NO_CHECKSUM

#include "data_types.h" //data types
#include "stdebug.h"
#include "urunet.h"

extern st_log * f_chk;
extern st_log * f_une;

extern const int unet_max_version;
extern const int unet_min_version;

#if 0
//Version separator were some major protocol differences are seen (V1 vs V2)
#define _U_VERSION 0x05

//protocol info is here
#include "prot.h"

/*Includes */
#include<unistd.h>
#include<stdlib.h>
#include<math.h>
#include<ctype.h>
#include<time.h>

#include "conv_funs.h" //conversion functions
#include "config_parser.h" //for parsing configuration files (all globals are here)
#include "stdebug.h"
#endif

void encode_packet(unsigned char * buf2,unsigned char* buf, int n);
void decode_packet(unsigned char* buf, int n);

int uru_validate_packet(Byte * buf,int n,st_uru_client * u);
U32 uru_checksum(Byte* buf, int size, int alg, Byte * aux_hash);

void uru_print_header(st_log * f_dsc,st_uru_head * u);
int uru_get_header(unsigned char * buf,int n,st_uru_head * u);
int uru_put_header(unsigned char * buf,st_uru_head * u);
int uru_get_header_start(st_uru_head * u);

void htmlDumpHeader(st_log * log,st_uru_client c,st_uru_head h,Byte * buf,int size,int flux);
void htmlDumpHeaderRaw(st_unet * net,st_log * log,st_uru_client c,Byte * buf,int size,int flux);

int parse_plNet_msg(st_unet * net,Byte * buf,int size,int sid);
int put_plNetMsg_header(st_unet * net,Byte * buf,int size,int sid);

void copy_plNetMsg_header(st_unet * net,int sid,int ssid,int flags);

#if 0

//deleted moved or internal only

void uru_update_packet_counter(Byte * buf, uru_head * u);

void uru_inc_msg_counter(uru_head * server);

void uru_init_header(uru_head * u, Byte validation);

int uru_get_ack_reply(Byte * buf, st_uru_client * u);

int uru_process_ack(Byte * buf,int n,int start,st_uru_client * u);

int uru_get_negotation_reply(Byte * buf,st_uru_client * u);

int uru_process_negotation(Byte * buf,int n,int start,st_uru_client * u);

#endif

//get chars of diferent code values
char * unet_get_release(int rel);
char * unet_get_destination(int dest);
char * unet_get_reason_code(int code);
char * unet_get_auth_code(int code);
char * unet_get_avatar_code(int code);
char * unet_get_str_ip(int ip);
char * unet_get_msg_code(U16 code);

#endif
