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

/**
	URUNET 3+
*/

#ifndef __U_URUNET_H
#define __U_URUNET_H
/* CVS tag - DON'T TOUCH*/
#define __U_URUNET_H_ID "$Id$"

namespace alc {

class tUnet {
public:
	tUnet(char * lhost="0.0.0.0",U16 lport=0);
	virtual ~tUnet();

};

}

#if 0

#include "data_types.h"
#include "stdebug.h"

#ifdef __WIN32__
//#include <winsock.h>
#include <winsock2.h>
#define socklen_t int
#else
#include <arpa/inet.h>
#endif

//udp packet max buffer size (0xFFFF) - any packet should be bigger.
#define INC_BUF_SIZE 65535
#define OUT_BUFFER_SIZE 1024

//! Urunet event table <0 errors, 0 ok, >0 events
#define UNET_PARSEERR -12 /* !< Error parsing a plNet Msg */
#define UNET_OUTOFRANGE -11 /* !<  Out of range */
#define UNET_NOTAUTHED -10 /* !< Not authenticated */
#define UNET_REJECTED -9 /* !< Connection was rejected */
#define UNET_TOMCONS -8 /* !< Too many connections, new connection ignored */
#define UNET_NONURU -7 /* !< Recieved a message of a unknown protocol */
#define UNET_CRCERR -6 /* !< checksum validation failed */
#define UNET_TOOBIG -5 /* !< message too big */
#define UNET_ERR -4 /* !< an error occurred */
#define UNET_NOBIND -3 /* !< Impossible to bind to address */
#define UNET_INHOST -2 /* !< Invalid host (cannot resolve or bind to address) */
#define UNET_FINIT -1  /* !< Cannot initizalize the netcore */
#define UNET_OK 0 /* !< Successfull operation (or netcore timeout loop) */
#define UNET_MSGRCV 1 /* !< A new message has been recieved */
#define UNET_NEWCONN 2 /* !< New incomming connection stablished */
#define UNET_TIMEOUT 3 /* !< Connection to peer has ended the timer */
#define UNET_TERMINATED 4 /* !< Connection to peer terminated */
#define UNET_FLOOD 5 /* !< This event occurs when a player is flooding the server,
                        aka a DOS attack, normally this happens when fly mode is used,
                        the vault can toggle this event very frequently, so it should
                        be ignored */
//note events UNET_FLOOD, UNET_TERMINATED, UNET_NEWCONN, and UNET_MSGRCV always contain
// a new incoming message (from the affected peer), if the size of it is non-zero.
//other events don't contain an incomming message, and the message size will be always zero

//! Uruent incoming message cue
typedef struct {
	Byte * buf; //The message
	int size; //The size of the message
	U32 sn; //The message number
	U32 stamp; //The message timestamp
	char check[32]; //The bitmap
	void * next; //Pointer to the next message
	Byte fr_count; //Number of fragments
	Byte completed; //It's the message completed, and prepared for the app layer?
} st_unet_rcvmsg;

//! Urunet outcomming message cue
//Only messages with the ack bit set are stored here, the others go directly to the clients
typedef struct {
	Byte * buf; //The raw message with all: header, timestamp, and packet counters
	int size; //The raw message size
	Byte tryes; //The number of times that this message has been send
	void * next; //The next message in the cue (ensure ordered delivery)
} st_unet_sndmsg;

//! Urunet header
typedef struct {
	Byte ch; //ch_byte 0x01 checksum, 0x00 no checksum
	U32 cs; //checksum (4 bytes)
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
	//-- advanced fields (negotation) -- ** depreceated **
	//U32 microseconds; //Store the last microseconds, or session variable
	//time_t timestamp; //Store the last timestamp
	//-- custom fields (for fragmentation building)
	//Byte fr_count;    //counts the number of processed fragmented packets.
} st_uru_head;

typedef struct {
	U16 cmd; //message code
	U32 flags; //message flags
	//--- header
	Byte max_version; //major version  //plNetVersion
	Byte min_version; //minor version
	U32 stamp; //plNetTimestamp
	U32 micros;
	U32 x; //session message identifier, (used in routing) //plNetX
	U32 ki; //player unique id (used in routing) //plNetKi
	Byte guid[17]; //player uid (in hex) (used in routing) //plNetGUI
	U32 ip; //peer ip (used in routing) //plNetIP
	U16 port; //peer port (used in routing)
} st_unet_hmsg;

//! Urunet kernel client header -
typedef struct {
	Byte flag; // 0x00 free, 0x01 in use, 0x02 deleted, 0x03 terminated/timeout
	int sid; //peer id
	st_uru_head client; //!<client session vars (remote)
	st_uru_head server; //!<server session vars (local)
	char sock_array[sizeof(struct sockaddr_in)];
	socklen_t a_client_size;
	U32 old_p_n; //previus p_n, used in the aging control
	char * w; //window
	U32 wite;
	//---
	int whoami; //peer type
	Byte validation; //store the validation level (0,1,2)
	Byte authenticated; //it's the peer authed? (0,1,2)
	//Byte cflags; **proposed, for next major unet changes **
	/* 0x01 UNET_CVALID message validated by the checksum test
		 0x02 UNET_CBUSSY bussy flag is up
	*/
	Byte validated; //has the checksum test passed? (0,1)
	Byte bussy; //bussy flag (0,1) If this flag is activated, messages are keept in the rcv buffer
	Byte max_version; //peer major version
	Byte min_version; //peer minor version
	Byte tpots; //tpots version 0=undefined, 1=tpots client, 2=non-tpots client
	U32 ip; //peer ip
	U16 port; //peer port
	/* Note IP/port pairs are in the network order, Future plans is to make all host order */
	/* Note2: proposed change, all timestamp pairs are going to be a single double like the ones used in uruping. */
	U32 timeout; //the peer timeout
	U32 timestamp; //last time that we recieved a packet from it
	U32 microseconds; //last time that ....
	U32 ack_stamp; //last time that we sent a packet to it
	U32 ack_micros;
	U32 nego_stamp; //negotiation timestamp (set up at beggining of connection)
	U32 nego_micros;
	U32 renego_stamp;
	U32 renego_micros;
	U32 alive_stamp; //last time that we send the NetMsgAlive
	st_unet_hmsg hmsg; //the message header
	char name[201]; //peer name (vault, lobby, AvatarCustomization) or player name (string)
	char acct[201]; //peer account name (string)
	char uid[41]; //peer uid (client) (in hex)
	char guid[20]; //peer guid (server) (string)
	Byte passwd[34]; //peer passwd hash (used in V2) (string)
	Byte challenge[16]; //peer challenge (used in auth) (hex)
	int ki; //player set and valid id
	int x; //x value
	Byte reason; //reason code
	Byte release; //type of client
	U16 maxPacketSz; //Maxium size of the packets. Must be 1024 (always)
	Byte access_level; //the access level of the peer
	Byte status; //the player status, defined inside a states machine (see the states machine doc)
	Byte paged; //0x00 non-paged player, 0x01 player is paged
	//flux control
	U32 bandwidth; //client reported bandwidth (negotiated technology)
	Byte window; //the peer window size
	//flood control
	U32 last_check; //time of last check
	int npkts; //number of packets since last check
	//inc messages
	st_unet_rcvmsg * rcvmsg; //incomming message cue
	//out messages
	st_unet_sndmsg * sndmsg; //outcomming message cue
	int success; //number of the total succesfully sent messages, reseted when ack is not recieved
	S16 vpos; //last packet sent
} st_uru_client;

//! Urunet flags
#define UNET_NBLOCK 0x01 /* non-blocking socket */
#define UNET_ELOG   0x02 /* enable netcore logging */
#define UNET_ECRC   0x04 /* crc check enabled */
#define UNET_AUTOSP 0x08 /* auto speed correction */
#define UNET_NOFLOOD 0x10 /* enable flooding control */
#define UNET_BCAST   0x20 /* enable broadcast */
#define UNET_FLOG    0x40 /* enable file based logging, (also, 0x02 must be present) */
#define UNET_NETAUTH 0x80 /* Enable authentication through servers */
#define UNET_DLACK    0x100 /* Dissable ack trace */
#define UNET_DLCHK    0x200 /* Dissable chk log */
#define UNET_DLUNE    0x400 /* Dissable une log */
#define UNET_DLSEC    0x800 /* Dissable sec log */

#define UNET_DEFAULT_FLAGS UNET_NBLOCK | UNET_ELOG | UNET_ECRC | UNET_AUTOSP | UNET_NOFLOOD | UNET_FLOG | UNET_NETAUTH

typedef U16 t_unet_flags;

//! Urunet handler
typedef struct {
#ifdef __WIN32__
	WSADATA ws; //<! The winsock stack
	SOCKET sock; //<! The socket
	u_long nNoBlock; //<! non-blocking (private)
#else
	int sock; //<! The socket
#endif
	int opt;
	struct sockaddr_in server; //<! Server sockaddr
	//!blocking socket
	t_unet_flags flags; /* 0x01 non-blocking socket
								 0x02 enable netcore logging
							   0x04 crc check enabled
								 0x08 auto speed correction
								 0x10 flooding control
								 0x20 enable broadcast
								 0x40 enable file based logging, (also, 0x02 must be present)
								 0x80 net auth - Enable authentication through servers
							*/
	U16 unet_sec; //netcore timeout to do another loop (seconds)
	U32 unet_usec; //netcore timeout to do another loop (milliseconds)

	Byte max_version; //Default protocol version
	Byte min_version;

	U32 timestamp; //current time
	U32 microseconds; //curent time

	U32 timeout; //default timeout (to disconnect a session)
	U32 ack_timeout; //default timeout when the ack clock expires.

	U32 n; //<! Current number of connections
	U32 max; //<! Maxium number of connections (default 0, unlimited)
	//this number should be always bigger than 2 times the maxium number of players
	st_uru_client * s;

	int whoami; //type of _this_ server
	//Byte clt; //0x00 auto, 0x01 unix socket, 0x02 lo, 0x03 LAN, 0x04 WAN

	U32 lan_addr; //<! LAN address, in network byte order
	U32 lan_mask; //<! LAN mask, in network byte order (default 255.255.255.0)
	//! Bandwidth speed (lo interface -> maxium)
	U32 lan_up;
	U32 lan_down;
	U32 nat_up;
	U32 nat_down;

	char name[200]; //<! The system/server name, normally the age filename
	char guid[18]; //<! This system guid (age guid) (in Ascii)

	char address[50]; //<! This system public address (in Ascii)

	U16 spawn_start; //first port to spawn
	U16 spawn_stop; //last port to spawn (gameservers)

	//peers
	int auth; // (Game/Lobby)
	int vault; // (Game/Lobby/Tracking)
	int tracking; // (Game/Lobby)
	int meta; // (Tracking(local meta)/Lobby(remote metas))
	int data; // (Meta) [hmm]

	//protocol versions (0=newest)
	int pro_auth;
	int pro_vault;
	int pro_tracking;
	int pro_meta;
	int pro_data;

	//!logging subsystem
	st_log * log; //stdout
	st_log * err; //stderr
	st_log * unx; //unexpected
	st_log * ack; //ack drawing
	st_log * chk; //checksum results
	st_log * sec; //security log
} st_unet;

char * get_ip(U32 ip);

void nlog(st_log * log,st_unet * net,int sid,char * msg,...);

void plNetInitStruct(st_unet * net);
int plNetStartOp(U16 port,char * hostname,st_unet * net);
void plNetStopOp(st_unet * net);

int plNetRecv(st_unet * net,int * sid);
int plNetConnect(st_unet * net,int * sid,char * address,U16 port,Byte flags);
#define UNET_VAL0 0x02
#define UNET_VAL1 0x01
#define UNET_VAL2 0x00
#define UNET_VAL3 0x03

int plNetSendMsg(st_unet * net,Byte * msg,int size,int sid,Byte flags);

void plNetDestroySession(st_unet * net,int sid);

int plNetGetMsg(st_unet * net,int sid,Byte ** msg);

void dumpSession(st_log * log,st_unet * net,int i);
void dumpSessions(st_log * log,st_unet * net);
void dumpBuffers(st_unet * net,Byte flags);

char * get_unet_error(S16 code);

char net_check_address(st_unet * net,int sid);

char plNetIsFini(st_unet * net,int sid);

int plNetClientSearchByKI(st_unet * net,U32 ki);
int plNetClientSearchByIp(st_unet * net,U32 ip,U16 port);
int plNetServerSearch(st_unet * net,Byte type);

void plNetEndConnection(st_unet * net,int sid);

#endif

#endif
