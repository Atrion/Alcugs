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

/******************************************************************
   This file, only contains basic data types and structures used
******************************************************************/

#ifndef __U_DATA_TYPES_
#define __U_DATA_TYPES_
#define __U_DATA_TYPES_ID $Id$

#include<time.h>
#include<netdb.h>
#include<netinet/in.h>
#include <arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>

/*Data types */
//1 byte
#define Byte unsigned char
//2 bytes
#define U16 unsigned short int
//2 bytes signed
#define S16 short int
//4 bytes
#define U32 unsigned int
//4 bytes signed
#define S32 int
/*End data types */

//udp packet max buffer size (0xFFFF) - any packet should be bigger.
#define BUFFER_SIZE 65535
#define OUT_BUFFER_SIZE 1024

#define MAX_PACKET_SIZE 1024 //defined to 1024 in all seen versions

//Maxium number of packets to store in the out buffer (it should be bigger than 30, more or less)
#define MAX_PACKETS_IN_BUFFER 255

//Maxium number of pending messages
#define MAX_PACKETS_PENDING 5

//!max string size in structs
#define STR_MAX_SIZE 254

//!some prp structures

//!UruObjectDesc
typedef struct {
	Byte flag; //2 extra, 01 associated vault block(s)
	U32 page_id; //the page id
	U16 page_type; //type of page
	Byte unk1; //only if flag=0x02
	U16 type; //the type of object
	Byte name[STR_MAX_SIZE+1]; //the object name (inverted ustring)
	U32 unk2; //only if flag is 0x01 (someone claims that is the number of nodes, but false)
	U32 node_id; //the node id, only if flag is 0x01
} st_uru_object_desc;

//!Another UruObjectDesc
typedef struct {
	Byte flag; //0x00
	U32 page_id; //page id
	U16 page_type; //type of page
	U16 type; //the type of object
	Byte name[STR_MAX_SIZE+1]; //the object name (inverted ustring)
	U32 ide; //the Unique flag
	U32 id; //the client id
} st_UruObjectDesc;

//! A sub msg
typedef struct {
	U16 type; //the type of message
	Byte unk0; //always 0x00
	U32 unk1; //always 0x01
	U32 unk2; //always 0x01
	U32 unk3; //always 0x00
	U16 k_type; //always 0x0052 (key type) plNetClientMgr
	Byte ks_type[STR_MAX_SIZE+1]; //key type inverted ustring16 (always plNetClientMgr)
	U32 unk4; //always 0x00
	U32 unk5; //always 0x00
	U32 unk6; //seen 0x00000840 and 0x00000040 (format?)
	Byte unk7; //object present? always 0x01
	st_uru_object_desc object; //an uru object
	U32 unk8; //0x01
	U32 unk9; //0x00
	U16 k_type_val; //always 0x00F4 (key val?) plAvatarMgr
	Byte ks_type_val[STR_MAX_SIZE+1]; //key type val inverted ustring16 (kAvatarMgr_KEY)
	U32 node_id; //another node id / Ki number
	U32 unk10; //0x00
	Byte unk11; //0x01
	U16 unk12; //0x01 load in // 0x00 load out (the object)
	U32 unk13; //0x00000180
} st_sub_msg;

typedef struct {
	U32 unk1; //always 0
	Byte flag; //always 0 (seen 0 sometimes)
	U32 size; //the msg size
	U16 type; //Type of message
	//Byte * data; //Message data
} st_uru_game_msg;


//an ADV msg
typedef struct {
	U16 cmd; //message command type
	U32 format; //message format
	//ack can be tested by 0x0000
	//--------------------------------------
	Byte max_version; //major version 0x00000100
	Byte min_version; //minor version 0x00000100
	U32 timestamp; //the time     0x00000001
	U32 microseconds; //the time  0x00000001
	U32 x; //the x value 0x00020000
	U32 ki; //the node - sender ki number 0x00100000
	Byte guid[18]; //a server guid
	//customized vars 0x
	Byte reason;
	Byte release;
	//---- old ----// (compatibility)
	U32 unk1; //always 0
	Byte unk2; //always 0
	U32 s_msg_size; //size in bytes of the message
	st_sub_msg msg; //the sub_message
	Byte unk3; //always 0
	st_uru_object_desc object; //and uru object
	Byte unk4; //seen 0x01
	Byte unk5; //always 0x01 in // 0x00 load out (the object)
	Byte unk6; //seen 0x01 and 0x00 (client 0x00 - server 0x01) (only in 12.06)
} st_uru_adv_msg;

//Uru protocol header
typedef struct {
	Byte ch;          //ch_byte 0x01 checksum, 0x00 no checksum
	U32 cs;           //checksum (4 bytes)
	//---- ACK control ----//
	U32 p_n;          //packet number (4 bytes)
	Byte t;           //type of packet (1 byte) followed by 4 blanks
	U32 unknownA;     //Unknown variable A (4 bytes)
	Byte fr_n;        //number of fragmented packet (1 byte)
	U32 sn;           //sequence number (3 bytes)
	Byte fr_t;        //total number of fragmented packets (1 byte) followed by 4 blanks
	U32 unknownB;     //Unknown variable B (4 bytes)
	Byte fr_ack;      //last fragmented packet acknowledged
	U32 ps;           //last acknowledged packet (3 bytes)
	U32 size;         //packet size (4 bytes)
	//-- advanced fields (negotation) --
	U32 microseconds; //Store the last microseconds, or session variable
	time_t timestamp; //Store the last timestamp
	//-- custom fields (for fragmentation building)
	Byte fr_count;    //counts the number of processed fragmented packets.
} uru_head;

#if 0
//plNetMsg header
typedef struct {
	U32 cmd1; //code 1 ?
	Byte sbool; //boolean variable?
	U32 cmd2; //code 2 ?
	Byte unknownA; //1 byte, unknown
	//12.3,12.2 & 12.0 vars
	time_t timestamp; //yes another redundant timestamp..
	U32 microseconds; //another redundant microseconds variable?
	//other vars
	U32 x; //packet number AGAIN?
	S32 ki; //p Player id
	Byte release; //Build internal release/external release, debug, ....
} st_plNetMsg;

//!a msg buffer
typedef struct {
	Byte * msg; //!< a pointer to the message
	Byte flag; //!< the flag used in the search algorithms
} st_msg;
#endif

//vault players list
typedef struct {
	Byte avatar[255]; //avatar name
	S32 ki; //the ki number
	Byte flags; //player flags?
	Byte access_level; //The player acccess level
} st_vault_player;

//!Pending
typedef struct {
	int flag;
	Byte * pnd_msg;
	int size;
	Byte t;
} st_uru_pending;


//!A message
typedef struct {
	Byte * buf; //the data
	int size; //the msg size
	void * next; //pointer to the next message
	U32 msg_n; //the msg number
	Byte t; //the msg type
	U32 stamp; //the msg timestamp
	int fr_count;
	char check[33];
} st_uru_msg_r;

//!Clients
typedef struct {
	//ack flags
	uru_head client;  //!< store client session vars
	uru_head server;  //!< store server session vars
	//pending messages
	st_uru_pending pending[MAX_PACKETS_PENDING];
	//store inet vars
	struct sockaddr_in a_client;
	char sock_array[sizeof(struct sockaddr_in)];
	socklen_t a_client_size;
	//internal flags
	Byte flag; //!< store status (used in the search session alg.)
	U32 old_p_n; //!< old packet number (aging), used to check old packets (negotation required to reset it)
	U32 old_sn; //!< the max sn of the latest packet recieved, basically to reset all stuff.
	Byte old_fr_t; //!< the max number of the latest fragment count (consistency checks)
	Byte fr_sum_check; //!< this var is used to code all recieved fragmented packets, and search any missing packet
	//low_level buffers
	st_uru_msg_r * current_msg; //pointer to the first msg of the qeue
	Byte bussy;

	Byte * rcv_msg; //pointer to a big buffer where we are building a fragmented stream
	Byte * snd_msg; //pointer to a big buffer where we are putting a new message
	//st_msg snd[NUM_MSGS]; //!the out messages
	//Byte snd_i; //!iterator for the out messages (points to the current message)
	Byte out[MAX_PACKETS_IN_BUFFER][OUT_BUFFER_SIZE]; //out send buffer per client
	//client session idemtifiers
	U32 client_ip;    //client ip adddress (saved in the <inet> side)
	U16 client_port;  //client port (saved in the <inet> side)
	//validation level
	Byte validation; //0x00 level 0, 0x01 level 1, 0x02 level 2
	Byte validated; //Has the packet passed the checksum and any other courtmeasures?
	//tpots
	Byte tpots; //tpots modifier
	//uru version used
	Byte minor_version;
	Byte major_version;
	//Session variables
	Byte negotiated; //!< Store if the client and server are in sync
	U32 bandwidth; //!< client bandwidth
	U16 maxPacketSz; //!< client maxpacket size
	time_t timestamp; //Date timestamp
	U32 microseconds; //session id?, microseconds?
	U32 nego_timestamp; //timestamp for negotation
	U32 nego_microseconds; //timestamp for microseconds
	//plNetMsg
	////st_plNetMsg msg;
	//account - auth fields -
	U32 uid; //the user id
	Byte guid[17]; //the player guid
	Byte login[255]; //Account Name
	Byte passwd[33]; //Passwd MD5 Hash 32 in ascii + \n
	Byte challenge[16]; //Challenge
	Byte authenticated; //Is the client authenticated?
	Byte access_level; //The user acccess level
	Byte release;
	int sid; //the sid number (session id)
	//player specific options
	Byte url[200]; //website url (nice)
	U16 p_num; //number of players per account
	st_vault_player * p_list; //a list of players
	S32 ki; //The KI number
	int x;
	Byte avatar_name[255]; //The avie name
	//age player specific ones
	Byte age_guid[17]; //!< the age guid
	Byte age_name[255]; //!<the age name
	//server specific ones
	U32 private_mask;
	Byte public_address[101];
	int load; //number of peers that has this host
	//age advanced status messages
	st_uru_adv_msg adv_msg; //and advanced message
	Byte paged; //is the player paged?
	st_uru_object_desc object; //the clone object that describes that avatar
	Byte joined; //is the player joined?
	U32 online_time; //Set the online time of the player
	Byte status; //InRoute, Joinning, Ingame...
	double ping;
} st_uru_client;

//specific ones


//!an age
typedef struct {
	Byte filename[200]; //the age filename
	Byte guid[20]; //the age guid
	Byte name[200]; //the age name
	U32 node; //the number of the node that defines *this* age
	double timestamp; //a complex timestamp

} st_uru_age;


//!vault managers
typedef struct {
	int id; //if >100 it's a normal node KI number for a player, elsewhere is a server manager node
	int node; //the node VMGR id that we are monitoring
	//U32 stamp; //a timestamp
	U32 ip; //ip that identifies the server session
	U16 port; //port that identifies the server session
	int sid; //the unique session identifier
} st_vault_mgrs;

//!Tracking
typedef struct {
	Byte uid[17];
	int x; //client X
	int ki; //the client //players ki number, unical uid
	U32 ip; //the server session, where this player is
	U16 port; //the server session, where this player is
	Byte flag; // 0-> delete, 1-> set invisible, 2-> set visible, 3-> set only buddies
	Byte status; //RStopResponding 0x00, 0x01 ok, RInroute 0x16, RArriving 0x17, RJoining 0x18, RLeaving 0x19, RQuitting 0x1A
	Byte avie[200];
	Byte login[200];
	Byte guid[31]; //Age guid where the player is
	Byte age_name[101]; //Age name where the player is
	Byte waiting; //Waiting var (1 if is waiting to the FindAgeReply, elsewhere nothing...)
	U32 client_ip; //clients ip address
	int sid; //the unique session identifier
} st_tracking;

#endif

