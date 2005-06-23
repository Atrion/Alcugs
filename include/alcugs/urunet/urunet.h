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
#define UNET_FLOOD 5 /* !< This event occurs when a player is flooding the server */
//note events UNET_FLOOD, UNET_TERMINATED, UNET_NEWCONN, and UNET_MSGRCV always contain
// a new incoming message (from the affected peer), if the size of it is non-zero.
//other events don't contain an incomming message, and the message size will be always zero

typedef U16 tUnetFlags;

//! Urunet flags
#define UNET_NBLOCK   0x001 /* non-blocking socket */
#define UNET_ELOG     0x002 /* enable netcore logging */
#define UNET_ECRC     0x004 /* crc check enabled */
#define UNET_AUTOSP   0x008 /* auto speed correction */
#define UNET_NOFLOOD  0x010 /* enable flooding control */
#define UNET_BCAST    0x020 /* enable broadcast */
#define UNET_FLOG     0x040 /* enable file based logging, (also, 0x02 must be present) */
#define UNET_NETAUTH  0x080 /* Enable authentication through servers */
#define UNET_DLACK    0x100 /* Dissable ack trace */
#define UNET_DLCHK    0x200 /* Dissable chk log */
#define UNET_DLUNE    0x400 /* Dissable une log */
#define UNET_DLSEC    0x800 /* Dissable sec log */

#define UNET_DEFAULT_FLAGS UNET_NBLOCK | UNET_ELOG | UNET_ECRC | UNET_AUTOSP | UNET_NOFLOOD | UNET_FLOG | UNET_NETAUTH

class tUnet {
public:
	tUnet(char * lhost="0.0.0.0",U16 lport=0);
	void setFlags(tUnetFlags flags);
	void unsetFlags(tUnetFlags flags);
	void startOp();
	void stopOp();
	virtual ~tUnet();
	void dump(tLog * f=NULL,Byte flags=0x01);
private:
	void init();
	
	int Recv(int * sid);
	
	void neterror(char * msg);

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
	tUnetFlags flags; //explained -^
	U16 unet_sec; //netcore timeout to do another loop (seconds)
	U32 unet_usec; //netcore timeout to do another loop (microseconds)

	Byte max_version; //Default protocol version
	Byte min_version;

	U32 ntime_sec; //current time (in secs)
	U32 ntime_usec; //(in usecs)

	U32 conn_timeout; //default timeout (to disconnect a session) (seconds) [3 secs]
	U32 timeout; //default timeout when the send clock expires (re-transmission) (milliseconds)

	U32 max; //<! Maxium number of connections (default 0, unlimited)
	//this number should be always bigger than 2 times the maxium number of players
	tNetSessionMgr * smgr; //Session MGR

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
	tLog * log; //stdout
	tLog * err; //stderr
	tLog * unx; //unexpected
	tLog * ack; //ack drawing
	tLog * chk; //checksum results
	tLog * sec; //security log
};

}

#if 0

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

char * get_ip(U32 ip);

void nlog(st_log * log,st_unet * net,int sid,char * msg,...);

int plNetConnect(st_unet * net,int * sid,char * address,U16 port,Byte flags);
#define UNET_VAL0 0x02
#define UNET_VAL1 0x01
#define UNET_VAL2 0x00
#define UNET_VAL3 0x03

int plNetSendMsg(st_unet * net,Byte * msg,int size,int sid,Byte flags);

int plNetGetMsg(st_unet * net,int sid,Byte ** msg);

char plNetIsFini(st_unet * net,int sid);

int plNetClientSearchByKI(st_unet * net,U32 ki);
int plNetClientSearchByIp(st_unet * net,U32 ip,U16 port);
int plNetServerSearch(st_unet * net,Byte type);

void plNetEndConnection(st_unet * net,int sid);

#endif

#endif
