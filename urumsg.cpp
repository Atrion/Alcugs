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

// UruNet class implementation will be here (temporany, it will be a list of all, C functions)

/*
	this will contain all related with using the socket
*/

#ifndef __U_URUMSG_
#define __U_URUMSG_
/* CVS tag - DON'T TOUCH*/
#define __U_URUMSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "config_parser.h"
#include "stdebug.h"
#include "conv_funs.h"
#include "protocol.h"
#include "urunet.h"

//-->
#include "vnodes.h"
#include "vault_obj.h"
#include "vaultstrs.h"

#include "msg_parser.h"

#include "sdlparser.h"
#include "ageparser.h"

#include "uru.h"

#include "urumsg.h"

extern st_uru_client *vault;

/*-----------------------------------------------------------
	Sends a ping, to the host specified in the session struct (client/server)
------------------------------------------------------------*/
int plNetMsgPing(int sock,U16 flags,double mtime,Byte destination,st_uru_client * u) {
	int off;
	int start;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgPing; //0x025D

	switch (flags) {
		case 1: //echo reply (must be the same packet echoed)
			u->server.t=0x00; //no ack
			/*if(u->adv_msg.format & plNetAck) {
				u->adv_msg.format=(u->adv_msg.format & ~plNetAck);
			}*/
			break;
		default: //Start a normal ping query
			u->server.t=0x02; //ack
			if(u->minor_version>=0x06) {
				DBG(2,"V2 ping packet 12.%i\n",u->minor_version);
				u->adv_msg.format=plNetTimestamp | plNetAck | plNetCustom | plNetKi | plNetX;
			} else {
				DBG(2,"V1 ping packet 12.%i\n",u->minor_version);
				u->adv_msg.format=plNetCustom | plNetKi | plNetX;
			}
	}

	u->server.ch=u->validation; //set the validation level of the packet

	//get the size
	start=uru_get_header_start(&u->server);
	off=start;

	print2log(f_uru,"<SND> Ping ");

	//puts the NetMsg cmd's into the buffer
	off+=put_plNetMsg_header(buf+off,u);

	print2log(f_uru," t:%e,dst:%i %s\n",mtime,destination,unet_get_destination(destination));

	*(double *)(buf+off)=mtime; //put that time in the packet
	off+=8;
	*(Byte *)(buf+off)=destination; //put the destination
	off++;

	//set total raw size
	u->server.size=off-start;

	//send the packet
	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*-----------------------------------------------------------
	Sends a ping, to the host specified in the session struct (client/server)
------------------------------------------------------------*/
int plNetMsgCustomMetaPing(int sock,U16 flags,double mtime,Byte destination,int population,st_uru_client * u) {
	int off;
	int start;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomMetaPing;

	//switch (flags) {
		//case 0: //echo reply (must be the same packet echoed, with the population)
			//u->server.t=0x00; //no ack
			/*if(u->adv_msg.format & plNetAck) {
				u->adv_msg.format=(u->adv_msg.format & ~plNetAck);
			}*/
			//break;
		//default: //Start a normal ping query
			u->server.t=0x02; //ack
			u->adv_msg.format=plNetTimestamp | plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion;
	//}

	u->server.ch=u->validation; //set the validation level of the packet

	//get the size
	start=uru_get_header_start(&u->server);
	off=start;

	print2log(f_uru,"<SND> MetaPing ");

	//puts the NetMsg cmd's into the buffer
	off+=put_plNetMsg_header(buf+off,u);

	print2log(f_uru," t:%e,dst:%i %s\n",mtime,destination,unet_get_destination(destination));

	*(double *)(buf+off)=mtime; //put that time in the packet
	off+=8;
	*(Byte *)(buf+off)=destination; //put the destination
	off++;
	*(U32 *)(buf+off)=1; //version, (there could be some minor protocol changes in the future)
	off+=4;
	*(U32 *)(buf+off)=population;
	off+=4;

	//set total raw size
	u->server.size=off-start;

	//send the packet
	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}


/*---------------------------------------------------------------
	Sends a MsgLeave, to the host specified in the session struct
	 ((seems useless for a server, but useful for a client)) (client)
----------------------------------------------------------------*/
int plNetMsgLeave(int sock,Byte reason,st_uru_client * u) {
	int off;
	int start;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgLeave; //0x025C: //plNetMsgLeave
	u->adv_msg.format=(plNetKi | plNetAck | plNetCustom);
	u->server.t=0x02;
	u->server.ch=u->validation; //set the validation level of the packet

	///gets the size of the header
	start=uru_get_header_start(&u->server);
	off=start;

	//puts the NetMsg cmd's into the buffer
	off+=put_plNetMsg_header(buf+off,u);

	//put the reason code
	*(Byte *)(buf+off)=reason;
	off++;

	print2log(f_uru,"<SND> Leave for %i reason %i\n",u->adv_msg.ki,reason);

	//set total raw size
	u->server.size=off-start;

	//send the packet
	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}


/*---------------------------------------------------------------
	Sends the AuthenticateHello (client)
----------------------------------------------------------------*/
int plNetAuthenticateHello(int sock,Byte * account,st_uru_client * u) {
	int off;
	int start;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgAuthenticateHello;
	u->adv_msg.format=plNetKi | plNetX | plNetCustom | plNetVersion; //0x00031200
	u->server.t=0x00;
	u->server.ch=u->validation; //set the validation level of the packet

	///gets the size of the header
	start=uru_get_header_start(&u->server);
	off=start;
	//puts the NetMsg cmd's into the buffer
	off+=put_plNetMsg_header(buf+off,u);

	print2log(f_uru,"<SND> AuthenticateHello v%i,%i ",u->adv_msg.max_version,u->adv_msg.min_version);

	//store the login name
	off+=encode_urustring(buf+off,account,strlen((const char *)account),0);
	*(U16 *)(buf+off)=u->maxPacketSz; //put the maxPacketSz
	off+=2;
	*(Byte *)(buf+off)=u->adv_msg.release; //put the build
	off++;

	print2log(f_uru," acctName:%s,maxPacketSz:%i,build:%i %s\n",account,u->maxPacketSz,u->adv_msg.release,unet_get_release((int)u->adv_msg.release));

	u->server.size=off-start; //Set msg size

	//set total raw size
	u->server.size=off-start;

	//send the packet
	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Sends the Challenge (it automatically calcules a challenge)
	for authresult, see authresponse for a complete list (server)
----------------------------------------------------------------*/
int plNetMsgAuthenticateChallenge(int sock, int authresult, st_uru_client * u) {
	int start;
	int off;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer
	Byte aux_hash[33]; //challenge aux hash

	u->adv_msg.cmd=NetMsgAuthenticateChallenge;
	u->adv_msg.format=(plNetAck | plNetVersion | plNetCustom | plNetKi | plNetX);
	//0x00071200
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	///gets the size of the header
	start=uru_get_header_start(&u->server);
	off=start;

	//puts the NetMsg cmd's into the buffer
	off+=put_plNetMsg_header(buf+off,u);

	print2log(f_uru,"<SND> AuthenticateChallenge v%i,%i \n",u->major_version,u->minor_version);

	*(buf+off)=authresult; //helloResult
	off++;

	//init the challenge to the MD5 of the current system time, and other garbage
	time((time_t *)(aux_hash));
	aux_hash[4]=0xFF;
	*(U32 *)(aux_hash+5)=(u->client.cs); //I'm too evil :D
	aux_hash[9]=0x01;
	aux_hash[10]=0x92;
	*(U32 *)(aux_hash+10)=get_microseconds();
	srandom(*(U32 *)(aux_hash));
	*(U32 *)(aux_hash+14)=(U32)random();

	MD5(aux_hash,18,u->challenge);
	hex2ascii2(aux_hash,u->challenge,16);

	print2log(f_uru," helloResult:%i,challenge:%s\n",authresult,aux_hash);

	//put the challenge
	off+=encode_urustring(buf+off,u->challenge,0x10,0x00);

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Sends the Response AuthenticateResponse (client)
----------------------------------------------------------------*/
int plNetMsgAuthenticateResponse(int sock, st_uru_client * u) {
	int start;
	int off;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer
	Byte aux_hash[33]; //auth response hash

	u->adv_msg.cmd=NetMsgAuthenticateResponse;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX);
	//0x00061200
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	///gets the size of the header
	start=uru_get_header_start(&u->server);
	off=start;

	//puts the NetMsg cmd's into the buffer
	off+=put_plNetMsg_header(buf+off,u);

	print2log(f_uru,"<SND> AuthenticateResponse \n");

	//calculate response hash
	Byte * md5buffer;

	md5buffer=(Byte *)malloc((strlen((const char *)u->login)+32+32+1)*sizeof(Byte));
	hex2ascii2(md5buffer,u->challenge,16);
	strcat((char *)md5buffer,(char *)u->login);
	strcat((char *)md5buffer,(char *)u->passwd);
	MD5(md5buffer,32+32+strlen((const char *)u->login),aux_hash);
	free((void *)md5buffer);

	//put the challenge
	off+=encode_urustring(buf+off,aux_hash,0x10,0x00);

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/* Asks the auth server to verify an user
*/
int plNetMsgCustomAuthAsk(int sock,Byte * login,Byte * challenge,Byte * hash,Byte release,U32 ip,st_uru_client * u) {
	int start;
	int off;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomAuthAsk;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetVersion | plNetX);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	///gets the size of the header
	start=uru_get_header_start(&u->server);
	off=start;

	//puts the NetMsg cmd's into the buffer
	off+=put_plNetMsg_header(buf+off,u);

	print2log(f_uru,"<SND> NetMsgCustomAuthAsk \n");

	//login
	off+=encode_urustring(buf+off,login,strlen((char *)login),0x00);
	//challenge
	memcpy(buf+off,challenge,16);
	off+=16;
	//hash (client answer)
	memcpy(buf+off,hash,16);
	off+=16;
	//build
	*(Byte *)(buf+off)=release;
	off++;
	//ip
	*(U32 *)(buf+off)=ip;
	off+=4;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/* Asks the auth server to verify an user
*/
int plNetMsgCustomAuthResponse(int sock,Byte * login,Byte result,Byte * passwd,Byte access_level,st_uru_client * u) {
	int start;
	int off;

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomAuthResponse;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetVersion | plNetX);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	///gets the size of the header
	start=uru_get_header_start(&u->server);
	off=start;

	//puts the NetMsg cmd's into the buffer
	off+=put_plNetMsg_header(buf+off,u);

	print2log(f_uru,"<SND> NetMsgCustomAuthResponse \n");

	//login
	off+=encode_urustring(buf+off,login,strlen((char *)login),0x00);
	//result
	*(Byte *)(buf+off)=result;
	off++;
	//passwd
	off+=encode_urustring(buf+off,passwd,strlen((char *)passwd),0x00);
	//GUID
	memcpy(buf+off,u->guid,16);
	off+=16;
	//access_level
	*(Byte *)(buf+off)=access_level;
	off++;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Sends the Auth response (server)
----------------------------------------------------------------*/
int plNetMsgAccountAuthenticated(int sock,Byte authresult,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgAccountAuthenticated; //code1=0x0394;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetGUI);
	//0x00065200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> AccountAuthenticated ");

	//account identifier
	// ID1(4 bytes)-ID2(2 bytes)-ID3(2 bytes)-ID4(2 bytes)-ID5(6 bytes)
	memcpy(buf+off,u->adv_msg.guid,16);
	off+=16;

	// authResult
	*(buf+off)=authresult;
	off++;

	print2log(f_uru,"auth_result: %i %s\n",authresult,unet_get_auth_code(authresult));

	// serverGuid (8 bytes)
	ascii2hex2(buf+off,global_age.guid,8);
	off+=8;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Send the alive message (client)
----------------------------------------------------------------*/
int plNetMsgAlive(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgAlive;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi);
	if(u->minor_version>=0x06) {
		//0x00061000; 0x00061001;
		u->adv_msg.format|=plNetTimestamp;
	}
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> Alive \n");

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Sends the vault list request message (client)
----------------------------------------------------------------*/
int plNetMsgRequestMyVaultPlayerList(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgRequestMyVaultPlayerList;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX);
	//0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> RequestMyVaultPlayerList \n");

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Sends the vault list request message (server->vault)
----------------------------------------------------------------*/
int plNetMsgCustomVaultAskPlayerList(int sock,Byte * guid,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomVaultAskPlayerList;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomVaultAskPlayerList \n");

	//put the guid
	memcpy(buf+off,guid,16);
	off+=16;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Sends the vault list response message (vault->server)
----------------------------------------------------------------*/
int plNetMsgCustomVaultPlayerList(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomVaultPlayerList;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomVaultPlayerList \n");

	//put the guid
	memcpy(buf+off,u->guid,16);
	off+=16;

	//put number of players
	*(U16 *)(buf+off)=u->p_num;
	off+=2;

	//put the players list
	int i;
	for(i=0; i<u->p_num; i++) {
		//ki
		*(S32 *)(buf+off)=u->p_list[i].ki;
		off+=4;
		//player
		off+=encode_urustring(buf+off,u->p_list[i].avatar,\
strlen((char *)u->p_list[i].avatar),0x00);
		//flags
		*(Byte *)(buf+off)=u->p_list[i].flags;
		off++;
	}

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Sends the Players list (server)
----------------------------------------------------------------*/
int plNetMsgVaultPlayerList(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgVaultPlayerList; //0x0373;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgVaultPlayerList to %s \n",u->login);

	//put number of players
	*(U16 *)(buf+off)=u->p_num;
	off+=2;

	//put the players list
	int i;
	for(i=0; i<u->p_num; i++) {
		//ki
		*(S32 *)(buf+off)=u->p_list[i].ki;
		off+=4;
		//player
		off+=encode_urustring(buf+off,u->p_list[i].avatar,\
strlen((char *)u->p_list[i].avatar),0x00);
		//flags
		*(Byte *)(buf+off)=u->p_list[i].flags;
		off++;
	}

	//nice added in version 12.7 :P
	if(u->minor_version>=0x07) {
		off+=encode_urustring(buf+off,global_url,strlen((char *)global_url),0);
	}

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*---------------------------------------------------------------
	Create player petition (client)
	//data contains the vault block
----------------------------------------------------------------*/
int plNetMsgCreatePlayer(int sock,Byte * avie, Byte * gender, Byte * friendn, Byte * key, Byte * data,int data_size,st_uru_client * u) {
	int start;
	int off;
	Byte * buf;

	u->adv_msg.cmd=NetMsgCreatePlayer; //0x02AE;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header

	//allocate buffer
	buf=(Byte *)malloc(sizeof(Byte) * (data_size + start + 300));

	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCreatePlayer\n");

	//put number of players
	//avie
	off+=encode_urustring(buf+off,avie,strlen((char *)avie),0x00);
	//gender
	off+=encode_urustring(buf+off,gender,strlen((char *)gender),0x00);
	//friend
	off+=encode_urustring(buf+off,friendn,strlen((char *)friendn),0x00);
	//key
	off+=encode_urustring(buf+off,key,strlen((char *)key),0x00);

	memcpy(buf+off,data,data_size);
	off+=data_size;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	free((void *)buf);

	return off;
}

/*---------------------------------------------------------------
	Create player petition (server->vault)
	//data contains the vault block
----------------------------------------------------------------*/
int plNetMsgCustomVaultCreatePlayer(int sock,Byte * login,Byte * guid,Byte access_level,Byte * data,int data_size,st_uru_client * u) {
	int start;
	int off;
	Byte * buf;

	u->adv_msg.cmd=NetMsgCustomVaultCreatePlayer; //0x02AE;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header

	//allocate buffer
	buf=(Byte *)malloc(sizeof(Byte) * (data_size + start + 300));

	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomVaultCreatePlayer ");

	//put the login
	off+=encode_urustring(buf+off,login,strlen((char *)login),1);

	//put the user id
	memcpy(buf+off,guid,16);
	off+=16;

	Byte guid_aux[50];
	hex2ascii2(guid_aux,guid,16);
	print2log(f_uru," guid:%s\n",guid_aux);

	//put access level
	*(Byte *)(buf+off)=access_level;
	off++;

	memcpy(buf+off,data,data_size);
	off+=data_size;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	free((void *)buf);

	return off;
}

/*-------------------------------------------------------------------
	Sends the KI of the new created player, and status of the creation
--------------------------------------------------------------------*/
int plNetMsgCustomVaultPlayerCreated(int sock,Byte status,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomVaultPlayerCreated; //0x0377;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND>NetMsgCustomVaultPlayerCreated, new ki id %i with result %i \n",u->adv_msg.ki,status);

	//put the user id
	memcpy(buf+off,u->guid,16);
	off+=16;

	*(Byte *)(buf+off)=status;
	off++;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*-------------------------------------------------------------------
	Sends the KI of the new created player, and status of the creation
--------------------------------------------------------------------*/
int plNetMsgPlayerCreated(int sock,Byte status,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgPlayerCreated; //0x0377;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> Player created, new ki id %i with result %i \n",u->adv_msg.ki,status);

	*(Byte *)(buf+off)=status;
	off++;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*-------------------------------------------------------------------
 Player deletion message (client)
--------------------------------------------------------------------*/
int plNetMsgDeletePlayer(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgDeletePlayer; //0x0384
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgDeletePlayer id %i\n",u->adv_msg.ki);

	*(U16 *)(buf+off)=0x00; //unknonw zero value
	off+=2;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*-------------------------------------------------------------------
 Asks vault for player deletion (server->vault)
--------------------------------------------------------------------*/
int plNetMsgCustomVaultDeletePlayer(int sock,Byte * guid,Byte access_level,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomVaultDeletePlayer;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsCustomVaultDeletePlayer id %i\n",u->adv_msg.ki);

	memcpy(buf+off,guid,16);
	off+=16;
	*(Byte *)(buf+off)=access_level;
	off++;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	return off;
}

/*-------------------------------------------------------------------
	Sends set my active player request (client)
--------------------------------------------------------------------*/
int plNetMsgSetMyActivePlayer(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgSetMyActivePlayer;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgSetMyActivePlayer id %i,%s\n",u->adv_msg.ki,u->avatar_name);

	off+=encode_urustring(buf+off,u->avatar_name,strlen((char *)u->avatar_name),0);
	*(Byte *)(buf+off)=0x00; //Unknown
	off++;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/*-------------------------------------------------------------------
	Sends a response to the setmyactive player (server)
--------------------------------------------------------------------*/
int plNetMsgActivePlayerSet(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgActivePlayerSet; //0x0465; plNetMsgActivePlayerSet
	if(u->tpots==1) {
		u->adv_msg.cmd=NetMsgActivePlayerSet2; //0x0466; plNetMsgActivePlayerSet
	}
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgActivePlayerSet id %i\n",u->adv_msg.ki);

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	if(u->tpots==0 && u->major_version==12 && u->minor_version==6) { //tpots patch
		//tpots
		off=start;
		u->adv_msg.cmd=NetMsgActivePlayerSet2;
		off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

		print2log(f_uru,"<SND> NetMsgActivePlayerSet2 (tpots) id %i\n",u->adv_msg.ki);

		u->server.size=off-start; //Set total raw size

		off=plNetAdvMsgSender(sock,buf,off,u);
	}

	return off;
}

/** Sends player status to the tracking server (server->tracking)
	Flag
	0-> delete
	1-> set invisible
	2-> set visible
	3-> set only buddies
	<---->
	Status
	RStopResponding 0x00
	RInroute 0x16
	RArriving 0x17
	RJoining 0x18
	RLeaving 0x19
	RQuitting 0x1A
	<----->
*/
int plNetMsgCustomPlayerStatus(int sock,Byte * uid,Byte * name,Byte * avie,Byte flag,Byte status,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomPlayerStatus;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomPlayerStatus id %i,%s,%s, %02X,%02X\n",u->adv_msg.ki,name,avie,flag,status);

	memcpy(buf+off,uid,16);
	off+=16;
	off+=encode_urustring(buf+off,name,strlen((char *)name),0);
	off+=encode_urustring(buf+off,avie,strlen((char *)avie),0);
	*(Byte *)(buf+off)=flag;
	off++;
	*(Byte *)(buf+off)=status;
	off++;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/** Asks the vault for the player ownership (game->vault)
*/
int plNetMsgCustomVaultCheckKi(int sock,Byte * uid,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomVaultCheckKi;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomVaultCheckKi id %i\n",u->adv_msg.ki);

	memcpy(buf+off,uid,16);
	off+=16;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/** Asks the vault for the player ownership (server->vault)
		//status
		0x01 OK
		elsewhere failed!
*/
int plNetMsgCustomVaultKiChecked(int sock,Byte * uid,Byte status,Byte * avie,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomVaultKiChecked;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomVaultKiChecked id %i\n",u->adv_msg.ki);

	memcpy(buf+off,uid,16);
	off+=16;
	*(Byte *)(buf+off)=status;
	off++;
	off+=encode_urustring(buf+off,avie,strlen((char *)avie),0);

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

//--- Set the server guid (game-->tracking)
int plNetMsgCustomSetGuid(int sock,Byte * guid,Byte * age_fname,Byte * public_address,Byte * private_mask,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomSetGuid;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomSetGuid\n");

	off+=encode_urustring(buf+off,guid,strlen((char *)guid),1);
	off+=encode_urustring(buf+off,age_fname,strlen((char *)age_fname),0);
	off+=encode_urustring(buf+off,private_mask,strlen((char*)private_mask),0);
	off+=encode_urustring(buf+off,public_address,strlen((char*)public_address),0);

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

//---- Publicy a shard (lobby-->meta) (tracking-->meta)
int plNetMsgCustomMetaRegister(int sock,int cmd,int pop,Byte * address,Byte * name,Byte * site,Byte * desc,Byte * passwd,Byte * contact,int dataset,Byte * dset,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomMetaRegister;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomMetaRegister v:%i,cmd:%i,pop:%i,addr:%s,name:%s,site:%s,desc:%s,ct:%s,d:%s\n",1,cmd,pop,address,name,site,desc,contact,dset);

	*(U32 *)(buf+off)=1; //version
	off+=4;
	*(U32 *)(buf+off)=cmd; //command
	off+=4;
	//1 -> register
	//0 -> unregister
	*(U32 *)(buf+off)=pop; //population
	off+=4;

	off+=encode_urustring(buf+off,address,strlen((char *)address),1);
	off+=encode_urustring(buf+off,name,strlen((char *)name),0);
	off+=encode_urustring(buf+off,site,strlen((char *)site),0);
	off+=encode_urustring(buf+off,desc,strlen((char *)desc),0);
	off+=encode_urustring(buf+off,passwd,strlen((char *)passwd),1);
	off+=encode_urustring(buf+off,contact,strlen((char *)contact),1);
	*(U32 *)(buf+off)=dataset; //dat
	off+=4;

	off+=encode_urustring(buf+off,dset,strlen((char *)dset),1);

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

//---- Find server (game-->tracking)
int plNetMsgCustomFindServer(int sock,Byte * guid,Byte * age_fname,U32 client_ip,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomFindServer;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomFindServer %s,%s\n",guid,age_fname);

	off+=encode_urustring(buf+off,guid,strlen((char *)guid),1);
	off+=encode_urustring(buf+off,age_fname,strlen((char *)age_fname),0);

	*(U32 *)(buf+off)=client_ip;
	off+=4;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

//--- Server Found (tracking-->game)
int plNetMsgCustomServerFound(int sock,Byte * address,int port,Byte * guid,Byte * age_fname,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomServerFound;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomServerFound %s:%i,%s,%s\n",address,port,guid,age_fname);

	*(U16 *)(buf+off)=port;
	off+=2;
	off+=encode_urustring(buf+off,address,strlen((char *)address),0);
	off+=encode_urustring(buf+off,guid,strlen((char *)guid),1);
	off+=encode_urustring(buf+off,age_fname,strlen((char *)age_fname),0);

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

//--- Server Found (tracking-->game)
int plNetMsgCustomForkServer(int sock,int port,Byte * guid,Byte * age_fname,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomForkServer;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomForkServer p:%i,g:%s,af:%s\n",port,guid,age_fname);

	*(U16 *)(buf+off)=port;
	off+=2;
	off+=encode_urustring(buf+off,guid,strlen((char *)guid),1);
	off+=encode_urustring(buf+off,age_fname,strlen((char *)age_fname),0);

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

//--- Vault Messages -- (client<-->game<-->vault)
int plNetMsgVault(int sock,t_vault_mos * v,st_uru_client * u,U32 flags) {
	int start;
	int off;
	Byte * buf; //the out buffer
	Byte * vbuf; //another buffer
	int size;

	if(u->tpots==1) {
		u->adv_msg.cmd=NetMsgVault2;
	} else {
		u->adv_msg.cmd=NetMsgVault;
	}
	u->adv_msg.format=(plNetAck | plNetKi | flags);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;

	DBG(5,"Calling to plVaultPack...\n");
	size=plVaultPack(&vbuf,v,u,1); //0x01 vault msg
	DBG(5,"Vault succesfully packed...\n");

	if(size>0) {
		buf=(Byte *)malloc(sizeof(Byte) * (size+100));
	} else {
		return size;
	}

	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgVault\n");

	memcpy(buf+off,vbuf,size);
	off+=size;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);

	if(u->tpots==0 && u->major_version==12 && u->minor_version==6 && u!=vault) { //tpots patch
		//tpots
		off=start;
		u->adv_msg.cmd=NetMsgVault2;
		off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

		print2log(f_uru,"<SND> NetMsgVault2 (tpots) id %i\n",u->adv_msg.ki);

		memcpy(buf+off,vbuf,size);
		off+=size;

		u->server.size=off-start; //Set total raw size

		off=plNetAdvMsgSender(sock,buf,off,u);
	}

	free((void *)buf);
	free((void *)vbuf);
	return off;
}


/*-------------------------------------------------------------------
	Sends a request to the age (client)
--------------------------------------------------------------------*/
int plNetMsgFindAge(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgFindAge;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgFindAge id %i\n",u->adv_msg.ki);

	//TODO add here the data contents

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/*-------------------------------------------------------------------
	Sends a response to the plNetMsgFindAgeReply (server)
--------------------------------------------------------------------*/
int plNetMsgFindAgeReply(int sock,Byte * age_name,Byte * address,int port,Byte * guid,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgFindAgeReply;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX); //0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgFindAgeReply id %i\n",u->adv_msg.ki);

	*(Byte *)(buf+off)=0x1F; //seems a response code, I will try it later ;)
	off++;
	//put the age name
	off+=encode_urustring(buf+off,age_name,strlen((char *)age_name), 0);
	*(Byte *)(buf+off)=0x03; //seems a flag (perhaps), well.., I will try it later ;)
	off++;  //0x03 is kgame, and the log displays kgame as the server type, o well
	//Ip address, also I will try if a domain works..
	off+=encode_urustring(buf+off, (Byte *)address, strlen((char *)address), 0);
	//and now the port
	*(U16 *)(buf+off)=port;
	off+=2;
	//finally copy the guid
	ascii2hex2(buf+off,guid,8);
	off+=8;
	//that's all, nice :)

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}


/*-------------------------------------------------------------------
	Sends a request to Join (client)
--------------------------------------------------------------------*/
int plNetMsgJoinReq(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgJoinReq;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX);
	//0x00061200;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgJoinReq id %i\n",u->adv_msg.ki);

	//TODO put here the message contents

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}


/*-------------------------------------------------------------------
	Sends a response to the plNetMsgJoinReq (server)
--------------------------------------------------------------------*/
int plNetMsgJoinAck(int sock,st_uru_client * u) { //TODO,t_sdl_binary * agehook) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	int off_size_start;

	u->adv_msg.cmd=NetMsgJoinAck; //0x025B
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetFirewalled);
	//0x00061220;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgJoinAck id %i\n",u->adv_msg.ki);

	/* WARNING about possible nasty buffer overflow
		remember that the maxium size of a message is <1024 bytes (we need to substract the header)
		This means, that if someone is enough crazy to create an AgeSDLHook object of
		several entryes, the server will explode...
	*/

	//copy the 11 blank spcs
	//bzero(buf+off,11);
	//off+=11;
	//that's all, nice :)

	*(U16 *)(buf+off)=0x00; //unk flag
	off+=2;
	*(U32 *)(buf+off)=0x00; //The uncompressed size, always 0 if flag is 0x00
	off+=4;
	*(Byte *)(buf+off)=0x00; //flag 0x00 if it is uncompressed, 0x02 for a compressed one
	off++;
	off_size_start=off;
	*(U32 *)(buf+off)=0x00; //SDL size
	off+=4;

	//now Cutresoft software enterprises TM LTD Inc. S.A. S.L, R
	//presents this horrible patented and unical way to do things ;P

	int i,e,max;
	max=0;
	e=-1;

	for(i=0; i<global_sdl_n; i++) {
		if(!strcmp((char *)global_sdl[i].name,(char *)global_age.name)) {
			if(global_sdl[i].version>max) {
				max=global_sdl[i].version;
				e=i;
			}
		}
	}

	if(e==-1) {

		plog(f_uru,"Fatal, no age descriptor found!!! arghhh!!, all is going to explode!! Panic!! aiieeeeeeee...\n");

	} else {

		*(Byte *)(buf+off)=0x00;
		off++;
		*(Byte *)(buf+off)=0x80;
		off++;
		off+=encode_urustring(buf+off,global_sdl[e].name,strlen((char *)global_sdl[e].name),1);
		*(U16 *)(buf+off)=max;
		off+=2;
		*(U16 *)(buf+off)=0x00;
		off+=2;
		*(Byte *)(buf+off)=0x06;
		off++;
		*(Byte *)(buf+off)=(Byte)global_sdl[e].n_vars;
		off++;

		//do it
		for(i=0; i<global_sdl[e].n_vars; i++) {
			//why, Am I too lazzy?
			*(Byte *)(buf+off)=0x00;
			off++;
			/*
			*(Byte *)(buf+off)=0x02;
			off++;
			*(Byte *)(buf+off)=0x00;
			off++;
			*(U16 *)(buf+off)=0xf000;
			off+=2; */
			*(Byte *)(buf+off)=0x08; //toma ya!!
			off++;
		}

		*(Byte *)(buf+off)=0x00;
		off++;

		*(U32 *)(buf+off_size_start)=off-(off_size_start+4); //SDL size

		//That should work, but I'm not 100% sure, so, let's see what happens, perhaps
		//the universe explodes, or, well, I can't wait more, let's go...


	}

	//TODO: improbe this big shit


	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

//--- Vault Task Messages -- (client<-->game<-->vault)
int plNetMsgVaultTask(int sock,t_vault_mos * v,st_uru_client * u,U32 flags) {
	int start;
	int off;
	Byte * buf; //the out buffer
	Byte * vbuf; //another buffer
	int size;

	u->adv_msg.cmd=NetMsgVaultTask;
	u->adv_msg.format=(plNetAck | plNetKi | plNetX | flags);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;

	DBG(5,"Calling to plVaultPack...\n");
	size=plVaultPack(&vbuf,v,u,2); //0x02 //vtask
	DBG(5,"Vault succesfully packed...\n");

	if(size>0) {
		buf=(Byte *)malloc(sizeof(Byte) * (size+100));
	} else {
		return size;
	}

	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgVaultTask\n");

	memcpy(buf+off,vbuf,size);
	off+=size;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	DBG(5,"Sent vault task response\n");
	free((void *)buf);
	free((void *)vbuf);
	DBG(5,"Returning from plNetMsgVaultTask RET:%i\n",off);
	return off;
}


//--- Vault Update Player status (game-->vault)
int plNetMsgCustomVaultPlayerStatus(int sock,Byte * age,Byte * guid,Byte state,U32 online_time,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgCustomVaultPlayerStatus;
	u->adv_msg.format=(plNetAck | plNetCustom | plNetKi | plNetX | plNetVersion);
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgCustomVaultPlayerStatus p:%i,g:%s,af:%s\n",state,guid,age);

	off+=encode_urustring(buf+off,age,strlen((char *)age),0);
	off+=encode_urustring(buf+off,guid,strlen((char *)guid),0);
	*(Byte *)(buf+off)=state;
	off++;
	*(U32 *)(buf+off)=online_time;
	off+=4;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/*-------------------------------------------------------------------
	Sends the GroupOwner response
--------------------------------------------------------------------*/
int plNetMsgGroupOwner(int sock,U32 page_id,U16 page_type, Byte page_flag,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgGroupOwner; //0x025F
	u->adv_msg.format=(plNetAck | plNetCustom);
	//0x00060000;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgGroupOwner\n");

	*(U32 *)(buf+off)=0x01; //formater? (mask)
	off+=4;
	*(U32 *)(buf+off)=page_id;
	off+=4;
	*(U16 *)(buf+off)=page_type;
	off+=2;
	*(Byte *)(buf+off)=0x00; //always zero?
	off++;
	*(Byte *)(buf+off)=page_flag;
	//0x01 page in acked
	//0x00 page out acked
	off++;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

#if 1

/*-------------------------------------------------------------------
	Sends the LoadClone response
--------------------------------------------------------------------*/
int plNetMsgLoadClone(int sock,st_uru_client * u,Byte * in_buf,int size) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgLoadClone; //0x025F
	u->adv_msg.format=(plNetAck | plNetKi);
	//0x00041000;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgLoadClone\n");

	memcpy(buf+off,in_buf,size);
	off+=size;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/*-------------------------------------------------------------------
	Sends the InitialAgeStateSent response
--------------------------------------------------------------------*/
int plNetMsgInitialAgeStateSent(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgInitialAgeStateSent; //0x02B3
	u->adv_msg.format=(plNetAck | plNetCustom);
	//0x00060000;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgInitialAgeStateSent\n");

	*(U32 *)(buf+off)=0x01; //seen several values, uknown purpose?
	off+=4;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}


/*-------------------------------------------------------------------
	Sends the plNetMsgMembersList response
--------------------------------------------------------------------*/
int plNetMsgMembersList(int sock,st_uru_client * u) {
	int start;
	int off;
	int size_pos;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgMembersList; //0x02A9
	u->adv_msg.format=(plNetAck | plNetCustom);
	//0x00060000;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgMembersList\n");

	//now we need to put all members here... it will be a little hard :(

	size_pos=off;
	off+=2; //the size of the list

	int e,i;
	e=0;
	if(global_broadcast==1) {

		for(i=global_the_bar+1; i<=global_client_count; i++) {
			//if(all_players[i].flag==0) { break; } //I hate when this things happen
			if(all_players[i].paged==1 && all_players[i].ki!=u->ki) {
				*(U32 *)(buf+off)=0x00000020; //unknown seen 0x20 and 0x22 (seems a flag)
				off+=4;
				*(U16 *)(buf+off)=0x03EA; //always seen that value
				off+=2;
				*(U32 *)(buf+off)=(U32)all_players[i].ki;
				off+=4;
				off+=encode_urustring(buf+off,all_players[i].avatar_name,strlen((char *)all_players[i].avatar_name),0x00);
				*(Byte *)(buf+off)=0x00; //always
				off++;
				*(Byte *)(buf+off)=0x03; //always
				off++;
				//ip
				*(U32 *)(buf+off)=ntohl(all_players[i].client_ip);
				off+=4;
				//port
				*(U16 *)(buf+off)=ntohs(all_players[i].client_port);
				off+=2;
				*(Byte *)(buf+off)=0x00; //always 0?
				off++;
				//the uruobject
				off+=put_uru_object_desc(buf+off,&(all_players[i].adv_msg.object));
				e++;
			}
		}
	}

	*(U16 *)(buf+size_pos)=(U16)e; //store the size of the members list

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/*-------------------------------------------------------------------
	Sends the plNetMsgMembersList response
--------------------------------------------------------------------*/
int plNetMsgMemberUpdate(int sock,st_uru_client * u,st_uru_client * copy,Byte flag) {
	int start;
	int off;
	int size_pos;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgMemberUpdate; //0x02AC
	u->adv_msg.format=(plNetAck | plNetCustom);
	//0x00060000;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgMemberUpdate\n");

	//now we need to put all members here... it will be a little hard :(

	size_pos=off;
	//off+=2; //the size of the list

	int e;
	//,i;
	e=0;
	//if(global_broadcast==1) {

		//for(i=global_the_bar+1; i<=global_client_count; i++) {
			//if(all_players[i].flag==0) { break; } //I hate when this things happen
			//if(all_players[i].paged==1 && all_players[i].ki!=u->ki) {
				*(U32 *)(buf+off)=0x00000020; //unknown seen 0x20 and 0x22 (seems a flag)
				off+=4;
				*(U16 *)(buf+off)=0x03EA; //always seen that value
				off+=2;
				*(U32 *)(buf+off)=(U32)copy->ki;
				off+=4;
				off+=encode_urustring(buf+off,copy->avatar_name,strlen((char *)copy->avatar_name),0x00);
				*(Byte *)(buf+off)=0x00; //always
				off++;
				*(Byte *)(buf+off)=0x03; //always
				off++;
				//ip
				*(U32 *)(buf+off)=ntohl(copy->client_ip);
				off+=4;
				//port
				*(U16 *)(buf+off)=ntohs(copy->client_port);
				off+=2;
				*(Byte *)(buf+off)=0x00; //always 0?
				off++;
				//the uruobject
				//off+=put_uru_object_desc(buf+off,&(copy->adv_msg.object));
				//-->
				*(Byte *)(buf+off)=0x00;
				off++;
				*(U32 *)(buf+off)=0xFFFFFFFF;
				off+=4;
				*(U32 *)(buf+off)=0x00;
				off+=4;
				*(Byte *)(buf+off)=0x00;
				off++;
				*(Byte *)(buf+off)=0xf0;
				off++;
				e++;
			//}
		//}
	//}
	*(Byte *)(buf+off)=0x01; //flag;
	off++;

	//*(U16 *)(buf+size_pos)=(U16)e; //store the size of the members list

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}



/*-------------------------------------------------------------------
	Sends the NetMsgSDLStateBCast response
--------------------------------------------------------------------*/
int plNetMsgSDLStateBCast(int sock,Byte * in_buf,int size,st_uru_client * u,st_uru_client * copy) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgSDLStateBCast; //0x0324
	//u->adv_msg.format=(plNetAck | plNetCustom);
	u->adv_msg.format=copy->adv_msg.format;
	u->adv_msg.ki=copy->adv_msg.ki;
	//0x00060000; <- changes a lot
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgSDLStateBCast\n");

	memcpy(buf+off,in_buf,size);
	off+=size;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}


/*-------------------------------------------------------------------
	Sends the plNetMsgGameMessage response
--------------------------------------------------------------------*/
int plNetMsgGameMessage(int sock,Byte * in_buf,int size,st_uru_client * u,st_uru_client * copy) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgGameMessage; //0x0324
	//u->adv_msg.format=(plNetAck | plNetCustom);
	u->adv_msg.format=copy->adv_msg.format;
	u->adv_msg.ki=copy->adv_msg.ki;
	//0x00060000; <- changes a lot
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgGameMessage\n");

	memcpy(buf+off,in_buf,size);
	off+=size;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/*-------------------------------------------------------------------
	Sends the plNetMsgGameMessage response
--------------------------------------------------------------------*/
int plNetMsgGameMessage69(int sock,Byte * in_buf,int size,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgGameMessage; //0x0324
	u->adv_msg.format=(plNetAck | plNetKi); //0x00041000
	u->adv_msg.ki=u->ki;

	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgGameMessage\n");

	memcpy(buf+off,in_buf,size);
	off+=size;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/*-------------------------------------------------------------------
	Sends the plNetMsgVoiceChat!!!
--------------------------------------------------------------------*/
int plNetMsgVoice(int sock,Byte * in_buf,int size,st_uru_client * u,U32 snd_ki) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	u->adv_msg.cmd=NetMsgVoice; //0x0274
	u->adv_msg.format=(plNetKi); //0x00001000
	//u->adv_msg.ki=u->ki; (please set it according to sender)
	u->adv_msg.ki=snd_ki;

	u->server.t=0x00; //require ACK, NO!
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgVoiceChat of %i bytes to %i\n",size,snd_ki);
	logflush(f_uru);

	memcpy(buf+off,in_buf,size);
	off+=size;

	//*(U32 *)(buf+(off-4))=u->ki;

	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}

/// Fake AgeSDLHook SDLState sender, let's see if this thingye works or not
int plNetMsgSDLStateEspecial(int sock,st_uru_client * u) {
	int start;
	int off;
	Byte buf[OUT_BUFFER_SIZE]; //the out buffer

	int off_size_start;

	u->adv_msg.cmd=NetMsgSDLState; //0x02C8
	u->adv_msg.format=(plNetAck);
	//0x00040000;
	u->server.t=0x02; //require ACK, YES?
	u->server.ch=u->validation; //set the validation level of the packet
	if(u->validation==0x01) u->server.ch=0x00; //disable level 1, because the checksum is not working

	start=uru_get_header_start(&u->server); 	//gets the size of the header
	off=start;
	off+=put_plNetMsg_header(buf+off,u); //puts the NetMsg cmd's into the buffer

	print2log(f_uru,"<SND> NetMsgSDLState id %i\n",u->adv_msg.ki);

	/* WARNING about possible nasty buffer overflow
		remember that the maxium size of a message is <1024 bytes (we need to substract the header)
		This means, that if someone is enough crazy to create an AgeSDLHook object of
		several entryes, the server will explode...
	*/

	//put here the uruobject
	*(Byte *)(buf+off)=0x00;
	off++;

	//the page id
	*(Byte *)(buf+off)=0x1F; //lsb
	off++;
	if(global_age_def!=NULL) {
		*(Byte *)(buf+off)=(Byte)(global_age_def->SequencePrefix+1);
	} else {
		*(Byte *)(buf+off)=0x00;
		print2log(f_err,"FATAL, TERRIBLE, HORRIBLE ERROR: global_age_def IS NULL?, what the ##@#~, where are your sdl files?, that should not be happening, never...\n");
	}
	off++;
	*(U16 *)(buf+off)=0x00;
	off+=2;
	//well page id finished--

	//please, cross your fingers that the page type is always 0x08
	*(U16 *)(buf+off)=0x08;
	off+=2;
	*(U16 *)(buf+off)=0x01; //yep, is A SceneObject
	off+=2;
	off+=encode_urustring(buf+off,(Byte *)"AgeSDLHook",10,1);
	//end the weird uruobject desc now the other stuff, hmm, copy and paste ;)


	*(U32 *)(buf+off)=0x00; //The uncompressed size, always 0 if flag is 0x00
	off+=4;
	*(Byte *)(buf+off)=0x00; //flag 0x00 if it is uncompressed, 0x02 for a compressed one
	off++;
	off_size_start=off;
	*(U32 *)(buf+off)=0x00; //SDL size
	off+=4;

	//now Cutresoft software enterprises TM LTD Inc. S.A. S.L, R
	//presents this horrible patented and unical way to do things ;P

	int i,e,max;
	max=0;
	e=-1;

	for(i=0; i<global_sdl_n; i++) {
		if(!strcmp((char *)global_sdl[i].name,(char *)global_age.name)) {
			if(global_sdl[i].version>max) {
				max=global_sdl[i].version;
				e=i;
			}
		}
	}

	if(e==-1) {

		plog(f_uru,"Fatal, no age descriptor found!!! arghhh!!, all is going to explode!! Panic!! aiieeeeeeee...\n");
		//STOP HERE!
		return 0;

	} else {

		*(Byte *)(buf+off)=0x00;
		off++;
		*(Byte *)(buf+off)=0x80;
		off++;
		off+=encode_urustring(buf+off,global_sdl[e].name,strlen((char *)global_sdl[e].name),1);
		*(U16 *)(buf+off)=max;
		off+=2;
		*(U16 *)(buf+off)=0x00;
		off+=2;
		*(Byte *)(buf+off)=0x06;
		off++;
		*(Byte *)(buf+off)=(Byte)global_sdl[e].n_vars;
		off++;

		//do it
		for(i=0; i<global_sdl[e].n_vars; i++) {
			//why, Am I too lazzy?
			*(Byte *)(buf+off)=0x00;
			off++;
			/**(Byte *)(buf+off)=0x02;
			off++;
			*(Byte *)(buf+off)=0x00;
			off++;
			*(U16 *)(buf+off)=0xf000;
			off+=2; */
			if((global_sdl[e].vars[i].type==0x02 || global_sdl[e].vars[i].type==0x09) && global_sdl[e].vars[i].flag==0x00 && global_sdl[e].vars[i].array_size==0x01) {
				*(Byte *)(buf+off)=0x00; //toma ya!!
				off++;
				*(Byte *)(buf+off)=(Byte)atoi((const char *)global_sdl[e].vars[i].default_value);
				off++;
			} else {
				*(Byte *)(buf+off)=0x08; //toma ya!!
				off++;
			}
			//*(U32 *)(buf+off)=0x00;
			//off+=4;
			//*(U32 *)(buf+off)=0x00;
			//off+=4;
		}

		*(Byte *)(buf+off)=0x00;
		off++;

		*(U32 *)(buf+off_size_start)=off-(off_size_start+4); //SDL size

		*(Byte *)(buf+off)=0x01;
		off++;


		//That should work, but I'm not 100% sure, so, let's see what happens, perhaps
		//the universe explodes, or, well, I can't wait more, let's go...


	}

	//TODO: improbe this big shit


	u->server.size=off-start; //Set total raw size

	off=plNetAdvMsgSender(sock,buf,off,u);
	return off;
}


#endif

#endif


