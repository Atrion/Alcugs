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

/* -- __THIS__ IS A TEMPORANY FILE -
PROTOCOL SPECIFICATIONS WILL BE ADDED INTO THE URUNET CLASS !

No sockets here, Please!!

 --*/

#ifndef __U_PROTOCOL_H_
#define __U_PROTOCOL_H_
/* CVS tag - DON'T TOUCH*/
#define __U_PROTOCOL_H_ID "$Id$"
#define __U_PROTOCOL_V "1.1.1"
#define __U_PROTOCOL_VMAX "12.07"
#define __U_PROTOCOL_VMIN "12.00"

//disable checksum checks
#define _NO_CHECKSUM

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
//for the md5 checksum
#include<openssl/md5.h>

#include "data_types.h" //data types
#include "conv_funs.h" //conversion functions
#include "config_parser.h" //for parsing configuration files (all globals are here)
#include "stdebug.h"

void uru_print_header(FILE * f_dsc,uru_head * u);

/*---------------------------------------------------
 De/Encodes the specific packet into Uru protocol 2

 k = offset MOD 8

 cod: c = x * 2 ^ k MOD 255
 dec: x = c * 2 ^ (8-k) MOD 255
---------------------------------------------------*/
void encode_packet(unsigned char* buf, int n);
void decode_packet(unsigned char* buf, int n);

/*---------------------------------------------------------------------
Computes the Uru known checksums
  alg
	 0 -> prime sum
	 1 -> live md5 sum
	 2 -> live md5 + passwd sum
	 (REMEMBER THAT aux_hash must be 32 bytes and contain an ascii hash
----------------------------------------------------------------------*/
U32 uru_checksum(Byte* buf, int size, int alg, Byte * aux_hash);

/*--------------------------------------------------------------------------------
  1st- Gets the validation level
	2nd- Checks if is a valid Uru Protocol formated packet
	3th- compute the checksum
	4th- Decode the packet
	5th- put the results in the session struct

	Net - NO

	Return Values
	 0 -> Yes, all went OK! :)
	 1 -> Ok, but checksum failed :(
	 2 -> Validation level too high
	 3 -> Bogus packet!!
---------------------------------------------------------------------------------*/
int uru_validate_packet(Byte * buf,int n,st_uru_client * u);


/*------------------------------------------------------
assings the header structure
of the specified buffer (ACK flags)

if success returns the number of the first data byte
on any other case returns 0
----------------------------------------------------*/
int uru_get_header(unsigned char * buf,int n,uru_head * u);

/*-------------------------------------------------------
   Prints the Uru Encapsulation header of a packet
--------------------------------------------------------*/
void uru_print_header(FILE * f_dsc,uru_head * u);

/*--------------------------------------------------------
  Gets the size of the header, and where the data starts
---------------------------------------------------------*/
int uru_get_header_start(uru_head * u);

/*--------------------------------------------------------
  Increments the packet counter, and updates the buffer
---------------------------------------------------------*/
void uru_update_packet_counter(Byte * buf, uru_head * u);

/*-----------------------------------------------------------
  Increases the message counter, with the specified flags
------------------------------------------------------------*/
void uru_inc_msg_counter(uru_head * server);

/*------------------------------------------------------
assings the header structure to the specified buffer
returns the size of the header
----------------------------------------------------*/
int uru_put_header(unsigned char * buf,uru_head * u);

/*------------------------------------------------------
Initialitzes (zeroes) the uru header
----------------------------------------------------*/
void uru_init_header(uru_head * u, Byte validation);

/*---------------------------------------------------------------
  updates the ack flags, and generates a new ack packet
	that will  be put in the buffer
------------------------------------------------------------*/
int uru_get_ack_reply(Byte * buf, st_uru_client * u);

/*----------------------------------------------------
  Parses and ACK reply, and updates the flags
	also, it need to clear from the buffer the acked
	packet and do other stuff.
	Returns 0 if it fails, returns size on success
----------------------------------------------------*/
int uru_process_ack(Byte * buf,int n,int start,st_uru_client * u);

/*------------------------------------------------------
  Creates a valid negotation packet (I think)
	returns the size
-------------------------------------------------------*/
int uru_get_negotation_reply(Byte * buf,st_uru_client * u);

/*--------------------------------------------------------
   Process the (Re)Negotation packets
	 returns 0 for not sending a Negotation reply
	 returns -1 if an error occurs
---------------------------------------------------------*/
int uru_process_negotation(Byte * buf,int n,int start,st_uru_client * u);

/*-------------------------------------------------------------------------
   Strips out the plNetMsg header in the struct
	 returns the position where the data starts
--------------------------------------------------------------------------*/
////int get_plNetMsg_header(Byte * buf,int n,int start,st_uru_client * u);

/*-------------------------------------------------------------------------
   Puts in the buffer the plNetMsg header of the struct
	 returns the position where the data starts
	 (This checks V1 & V2 packets for the timestamp useless data)
--------------------------------------------------------------------------*/
////int put_plNetMsg_header(Byte * buf,int n,int start,st_uru_client * u);

/*------------------------------------------------
  Gets the minidata
	 KI, build, player and puts into the struct
	 returns the position of the next byte of data
------------------------------------------------*/
////int get_plNetMsg_microki(Byte * buf,int n,int start,st_uru_client * u);

/*------------------------------------------------
  Puts the minidata
	 KI, build, player and puts into the struct
	 returns the position of the next byte of data
------------------------------------------------*/
////int put_plNetMsg_microki(Byte * buf,int n,int start,st_uru_client * u);

/** Check msg codes

*/
//int check_plnetmsg(st_plNetMsg msg,Byte sbool,U32 cmd2,Byte unkA);

/** Gets all plNet msg header vars

*/
int parse_plNet_msg(Byte * buf,st_uru_client * u);

/** puts in the buffer all plnet msg header vars

*/
int put_plNetMsg_header(Byte * buf, st_uru_client * u);

//get chars of diferent code values
char * unet_get_release(int rel);
char * unet_get_destination(int dest);
char * unet_get_reason_code(int code);
char * unet_get_auth_code(int code);
char * unet_get_avatar_code(int code);
char * unet_get_str_ip(int ip);

#endif
