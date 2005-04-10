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

#ifndef __U_PROTOCOL_
#define __U_PROTOCOL_
/* CVS tag - DON'T TOUCH*/
#define __U_PROTOCOL_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "protocol.h"

/*---------------------------------------------------
 De/Encodes the specific packet into Uru protocol 2

 k = offset MOD 8

 cod: c = x * 2 ^ k MOD 255
 dec: x = c * 2 ^ (8-k) MOD 255
---------------------------------------------------*/
void encode_packet(unsigned char* buf, int n) {
	int i;
	for(i=0; i<n; i++) {
		buf[i] = buf[i] << (i%8) | buf[i] >> (8-(i%8));
#if 0
		if(buf[i]!=0xFF)
		buf[i]=(unsigned char)(((int)buf[i] * (int)pow(2,(double)(i%8))) % 255);
#endif
	}
}

void decode_packet(unsigned char* buf, int n) {
	int i;
	for(i=0; i<n; i++) {
		buf[i] = buf[i] >> (i%8) | buf[i] << (8-(i%8));
#if 0
		if(buf[i]!=0xFF)
		buf[i]=(unsigned char)(((int)buf[i] * (int)pow(2,(double)(8-(i%8)))) % 255);
#endif
	}
}

/*---------------------------------------------------------------------
Computes the Uru known checksums
  alg
	 0 -> prime sum
	 1 -> live md5 sum
	 2 -> live md5 + passwd sum
	 (REMEMBER THAT aux_hash must be 32 bytes and contain an ascii hash
----------------------------------------------------------------------*/
U32 uru_checksum(Byte* buf, int size, int alg, Byte * aux_hash) {
	int i;
	U32 aux=0;
	Byte * md5buffer;
	Byte hash[16];
	int aux_size; //auxiliar size

	DBG(4,"Checksum %i requested, packet size is %i...\n",alg,size);
	switch(alg) {
		case 0:
			//be sure that the last chunck of bytes is zero
			for(i=size; i<size+4; i++) {
				if(i>=BUFFER_SIZE) { break; }
				buf[i]=0;
			}
			for(i=6; i<size; i=i+4) {
				aux = aux + *((U32 *)(buf+i)); //V1 - Checksum algorithm ~ it fails in some messages :/
				//print2log(f_chkal,"%08X - %08X\n",*((U32 *)(buf+i)),aux);
			}
			break;
		case 1:
		case 2:
			//code for the V2 - Checksum algorithm
			aux_size=size-6;
			if(alg==2) { aux_size+=32; }
			//allocate the space for the buffer
			DBG(4,"Allocating md5buffer - %i bytes...\n",aux_size);
			md5buffer = (Byte *)malloc(sizeof(Byte)*(aux_size));
			if(md5buffer==NULL) {
				DBG(4,"md5buffer is NULL?\n");
			}
			for(i=6; i<size; i++) {
				md5buffer[i-6]=buf[i];
			}
			//Concatenate the ASCII passwd md5 hash as required by V2 algorithm
			if(alg==2) {
				for(i=size; i<(aux_size+6); i++) {
					md5buffer[i-6]=aux_hash[i-size];
				}
				//print2log(f_chkal,"passwd_hash: %s\n",aux_hash);
			}
			//print2log(f_chkal,"to be md5sumed:\n");
			//dump_packet(f_chkal,md5buffer,aux_size,0,5);
			MD5(md5buffer, aux_size, hash);
			//print2log(f_chkal,"\n<-\n");

			aux = *((U32 *)hash);
			free(md5buffer);

			break;
		default:
			//print2log(f_chkal,"ERR: Unimplemented Checksum V%i algorithm requested\n",alg);
			print2log(f_err,"ERR: Uru Checksum V%i is currenlty not supported in this version of the server.\n\n",alg);
			aux = 0xFFFFFFFF;
	}
	return aux;
}


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
int uru_validate_packet(Byte * buf,int n,st_uru_client * u) {
	U32 checksum;
#ifndef _NO_CHECKSUM
	U32 aux_checksum;
#endif
	if(n>=2 && buf[0]==0x03) { //magix uru number
		u->validation=buf[1]; //store the validation level
		if(buf[1]==0x00) { u->validated=1; u->client.cs=0x00; return 0; } //All went OK with validation level 0
		checksum=*(U32 *)(buf+0x02); //store the checksum
		u->client.cs=checksum; //it's used to calculate the challenge (legacy)
#ifndef _NO_CHECKSUM
		if(buf[1]==0x01) { //validation level 1
			aux_checksum=uru_checksum(buf, n, 0, NULL);
		} else
#endif
		if(buf[1]==0x02) { //validation level 2
			//print2log(f_uru,"--- authenticated? %i\n",u->authenticated);
#ifndef _NO_CHECKSUM
			if(u->authenticated==1) { //authenticated?
				//hex2ascii(u->passwd,hash,16); //get the string of the hash
				aux_checksum=uru_checksum(buf, n, 2, u->passwd);
			} else {
			aux_checksum=uru_checksum(buf, n, 1, NULL);
			}
#endif
			decode_packet(buf,n); //Now the packet will be readable
			//be sure to take bak the 0x02 byte to the header
			buf[1]=0x02;
		}
#ifndef _NO_CHECKSUM
		else {
			return 2; //humm??
		}
		if(aux_checksum==checksum) { u->validated=1; return 0; } //All went ok with v1
		else { u->validated=0; return 1; } //:(
#else
		return 0; //allow all packets
#endif
	}
	return 3; //aiieee!!!
}


/*------------------------------------------------------
assings the header structure
of the specified buffer (ACK flags)

if success returns the number of the first data byte
on any other case returns 0
----------------------------------------------------*/
int uru_get_header(unsigned char * buf,int n,uru_head * u) {
	U32 i; //normal unsigned iterator
	int offset=2;
	unsigned char non_std=0; //if 1, non-standard packet found

	//ch_byte
	u->ch=(U16)buf[1];

	//checksum
	if(buf[1]==0x01 || buf[1]==0x02) { //0x01 (0x02 encoded) (chksum) or 0x00 (without chksum)
		offset=6;
	}

	//Generic Packet counter
	u->p_n=*((U32 *)(buf+offset));
	offset+=4; //U32

	//type of message
	u->t=buf[offset];
	offset++;

	//4 blank spaces (unkownA)
	u->unknownA=*((U32 *)(buf+offset));

	if(u->unknownA!=0) { non_std=1; }
	offset+=4;

	//Number of data fragment
	u->fr_n=*((Byte *)(buf+offset));
	offset++;
	//Current message number
	u->sn=(*((U32 *)(buf+offset)) & 0x00FFFFFF); //be sure that the msb is not the fr_t
	offset+=3;

	//Total number of fragments
	u->fr_t=*((Byte *)(buf+offset));
	offset++;

	//4 blank spaces (unkownB)
	u->unknownA=*((U32 *)(buf+offset));

	if(u->unknownA!=0) { non_std=1; }
	offset+=4;

	//Last fragmented packet ack.
	u->fr_ack=*((Byte *)(buf+offset));
	offset++;
	//Last ack recieved. (or last (re)negotation)
	u->ps=(*((U32 *)(buf+offset)) & 0x00FFFFFF);
	offset+=3;

	//packet size
	u->size=*((U32 *)(buf+offset));

	offset=offset+4;
	//iterator now at the beginging of the data

	//size consistency check, being 1024 the maxPacketSize
	if(u->size>1024) { non_std=1;	}

	if(non_std==1) {
		print2log(f_une,"<-------- Attention-UNKNOWN PACKET FOUND!! dump here: -------->\n");
		print2log(f_une,"\n");
		dump_packet(f_une,buf,n,0,5);
		print2log(f_une,"\n<-------------- End UNKNOWN -------------------------------->\n");
		return 0; //abandon to do nothing. (no response to client)
	}

	//now let's go to test the size theory
	if(u->ch==0x01 || u->ch==0x02) { //checksum
		i=n-32;
	} else {
		i=n-28;
	}

	if(i==u->size && u->t==0x80) {
		print2log(f_uru,"INF: Unexpected size mismatch for 0x80! %i = %i?\n",i,u->size);
		print2log(f_une,"ERR: Size mismatch!! Type: %i, Size: %i, Exp_Size: %i\n",i,u->size);
		dump_packet(f_une,buf,n,0,5);
		print2log(f_une,"\n-----------------------------------\n");
		return 0; //abandon to do nothing. (no response to client)
	}
	else {
		if(i!=u->size && u->t!=0x80) {
			print2log(f_uru,"INF: Unexpected size mismatch! %i = %i?\n",i,u->size);
			print2log(f_une,"ERR: Size mismatch!! Type: %i, Size: %i, Exp_Size: %i\n",i,u->size);
			dump_packet(f_une,buf,n,0,5);
			print2log(f_une,"\n-----------------------------------\n");
			return 0; //abandon to do nothing. (no response to client)
		}
	}
	return offset;
}

/*-------------------------------------------------------
   Prints the Uru Encapsulation header of a packet
--------------------------------------------------------*/
void uru_print_header(FILE * f_dsc,uru_head * u) {
	print2log(f_dsc,"[%i] ->%02X<- {%i,%i (%i) %i,%i} - %02X|%i bytes",u->p_n,u->t,u->fr_n,u->sn,u->fr_t,u->fr_ack,u->ps,u->size,u->size);
}

/*--------------------------------------------------------
  Gets the size of the header, and where the data starts
---------------------------------------------------------*/
int uru_get_header_start(uru_head * u) {
	if(u->ch==0x01 || u->ch==0x02) return 32; //with checksum
	else return 28; //with no-checksum
}

/*--------------------------------------------------------
  Increments the packet counter, and updates the buffer
---------------------------------------------------------*/
void uru_update_packet_counter(Byte * buf, uru_head * u) {
	u->p_n++;
	if(buf[1]==0x01 || buf[1]==0x02) {
		*((U32 *)(buf+6))=u->p_n;
	} else {
		*((U32 *)(buf+2))=u->p_n;
	}
}

/*-----------------------------------------------------------
  Increases the message counter, with the specified flags
------------------------------------------------------------*/
void uru_inc_msg_counter(uru_head * server) {
		//update flags
		server->p_n++;
		server->sn++;
}

/*------------------------------------------------------
assings the header structure to the specified buffer
returns the size of the header
----------------------------------------------------*/
int uru_put_header(unsigned char * buf,uru_head * u) {
	int offset=2;

	//ch_byte (validation level)
	buf[1]=u->ch;

	//checksum
	if(buf[1]==0x01 || buf[1]==0x02) { //0x01 (0x02 encoded) (chksum) or 0x00 (without chksum)
		*((U32 *)(buf+2))=0xFFFFFFFF;
		offset=6;
	}

	//Generic Packet counter
	*((U32 *)(buf+offset))=u->p_n; //we are going to put it in the net_snd
	offset+=4; //U32

	//type of message
	buf[offset]=u->t;
	offset++;

	//4 blank spaces (unkownA)
	*(U32 *)(buf+offset)=0;
	offset+=4;

	//Number of data fragment
	*((Byte *)(buf+offset))=u->fr_n;
	offset++;
	//Current message number
	*((U32 *)(buf+offset))=u->sn;
	offset+=3;

	//Total number of fragments
	*((Byte *)(buf+offset))=u->fr_t;
	offset++;

	//4 blank spaces
	*(U32 *)(buf+offset)=0;
	offset+=4;

	//Last fragmented packet ack.
	*((Byte *)(buf+offset))=u->fr_ack;
	offset++;
	//Last ack recieved. (or last (re)negotation)
	*((U32 *)(buf+offset))=u->ps;

	offset+=3;

	//packet size
	*((U32 *)(buf+offset))=u->size;

	offset=offset+4;
	//iterator now at the beginging of the data

	return offset;
}

/*------------------------------------------------------
Initialitzes (zeroes) the uru header
----------------------------------------------------*/
void uru_init_header(uru_head * u, Byte validation) {
	u->ch = validation; //(outdated legacy)
	u->p_n=0;
	u->t=0x00;
	u->fr_n=0;
	u->sn=0;
	u->fr_t=0;
	u->fr_ack=0;
	u->ps=0;
	u->size=0x00;
}

/*---------------------------------------------------------------
  updates the ack flags, and generates a new ack packet
	that will  be put in the buffer
------------------------------------------------------------*/
int uru_get_ack_reply(Byte * buf, st_uru_client * u) {
	int n;

	//update flags
	u->server.fr_n=0; //put the fragment number to zero
	u->server.p_n++;
	u->server.t=0x80;
	u->server.sn++;
	u->server.size=0x01;

	if(u->validation<=0x01) {
		u->server.ch=0x00; //disable checksum on all V1 packets
		//(Negotation and Ack packets don't have checksum)
	}

	//puts the header in the buffer
	n=uru_put_header(buf,&u->server);

	//create ack_packet
	//two blanks
	*(U16 *)(buf+n)=0x00;
	//fragment ack
	buf[n+2]=u->client.fr_n;
	//sn ack
	*(U32 *)(buf+n+3)=u->client.sn;
	//4 blanks
	*(U32 *)(buf+n+6)=0x00;

	// fr ack
	buf[n+0x0A]=u->client.fr_ack;
	// ps ack
	*(U32 *)(buf+n+0x0B)=u->client.ps;

	//4 blanks
	*(U32 *)(buf+n+0x0E)=0x00;

	print2log(f_uru,"<SND> Ack (%i,%i) (%i,%i)\n",u->client.fr_n,u->client.sn,u->client.fr_ack,u->client.ps);

	return 0x12+n;
}

/*----------------------------------------------------
  Parses and ACK reply, and updates the flags
	also, it need to clear from the buffer the acked
	packet and do other stuff.
	Returns 0 if it fails, returns size on success
----------------------------------------------------*/
int uru_process_ack(Byte * buf,int n,int start,st_uru_client * u) {
	U32 i;
	int non_std=0;

	Byte fr_n;
	U16 sn;

	Byte fr_ack;
	U16 ps;

	U16 unknownZ;
	U32 unknownA;
	U32 unknownB;

	unknownZ=*(U16 *)(buf+start);
	if(unknownZ!=0) { non_std=1; }

	print2log(f_uru,"<RCV> Ack");

	int stats_n=0, del_n=0;

	//Do for each ack.
	for(i=0; i<u->client.size; i++) {
		fr_n=*(Byte *)(buf+start+2+(i*16));
		sn=*(U32 *)(buf+start+3+(i*16));
		unknownA=*(U32 *)(buf+start+6+(i*16));
		fr_ack=*(Byte *)(buf+start+0x0A+(i*16));
		ps=*(U32 *)(buf+start+0x0B+(i*16));
		unknownB=*(U32 *)(buf+start+14+(i*16));
		if(i!=0) { print2log(f_uru," |"); }
		print2log(f_uru," (%i,%i) (%i,%i)",fr_n,sn,fr_ack,ps);

		if(unknownA!=0 || unknownB!=0) { non_std=1; }

		/*
		TODO: This need to change, search in the buffer for the acked packets
		and delete them, if something strange happens log the unexpected packets.
		*/

		/* temporany patch */ //erres un chapussero
		Byte e;
		int n_start;
		Byte ack_fr;
		U32 ack_sn;

		stats_n=0;

		for(e=0; e<MAX_PACKETS_IN_BUFFER; e++) {
			if(u->out[e][0]==0x01) { //the slot is in use
				if(u->out[e][1]==0x00) { n_start=11; } //28
				else { n_start=15; } //32
				ack_sn=(*(U32 *)(u->out[e]+n_start+1)) & 0x00FFFFFF;
				ack_fr=*(Byte *)(u->out[e]+n_start);
				//printf("\n@@@@@@@@@@\nack_sn %i, ack_fr %i\n sn %i, fr_n %i, ps %i, fr_ack %i\n",ack_sn,ack_fr,sn,fr_n,ps,fr_ack);
				//if((ack_sn<=sn && ack_fr<=fr_n) && (ack_sn>ps && (ack_fr>fr_ack || ack_fr==0))) {
				////if((ack_sn<=sn && ack_fr<=fr_n)) { //<-- old
				if((ack_sn==sn && ack_fr==fr_n)) { //<-- new
					//ok
					u->out[e][0]=0x02; //reset acked packet
					//printf("\n##############DELETED PACKET FROM BUFFER!!\n");
					del_n++;
				} else {
					stats_n++;
				}
				//dump_packet(stdout,u->out[e],30,0,5);
			}
		}


		/* end of temporany patch */

		if(fr_n==u->server.fr_n && sn==u->server.sn) {  //perfect it's an ack of the last packet sent
			if(fr_ack!=u->server.fr_ack || ps!=u->server.ps) {
				print2log(f_uru," ERR: Found an ack packet that breaks the rules!\n");
				non_std=1;
			}
			u->server.fr_ack=u->server.fr_n; //update flags
			u->server.ps=u->server.sn;
		} else if(fr_n>u->server.fr_n || sn>u->server.sn) {
			print2log(f_uru," ERR: Recieved an ack of a packet that I have not send!? %i - %i\n",fr_n,u->server.fr_n);
			//non_std=1;
		} else {
			print2log(f_uru," INF: Discarding old ack packet?\n");
			//non_std=1;
		}
	}

	print2log(f_uru,"\nINF: Deleted %i packets, pending %i pkts",del_n,stats_n);

	print2log(f_uru,"\n");

	if(non_std==1) return 0;

	return (u->client.size + start);
}

/*------------------------------------------------------
  Creates a valid negotation packet (I think)
	returns the size
-------------------------------------------------------*/
int uru_get_negotation_reply(Byte * buf,st_uru_client * u) {
	int n;
	char * timestamp;

	//update flags
	u->server.fr_n=0; //put the fragment number to zero
	//u->server.p_n++;
	u->server.t=0x42;
	//u->server.sn++;
	u->server.size=0x0C;

	if(u->validation<=0x01) {
		u->server.ch=0x00; //disable checksum on all V1 packets
		//(Negotation and Ack packets don't have checksum)
	}

	n=uru_put_header(buf,&u->server);

	//server bandwidth
	*(U32 *)(buf+n)=global_bandwidth;

	//time(&u->timestamp);
	timestamp=ctime((const time_t *)&u->server.timestamp);

	print2log(f_uru,"<SND> (Re)Negotation us: %i bandwidth: %i bps time: %s",u->server.microseconds,global_bandwidth,timestamp);
	//free(timestamp);

	//store timestamp
	*(time_t *)(buf+n+4)=u->server.timestamp;
	//*(time_t *)(buf+n+4)=u->nego_timestamp;
	//microseconds?
	*(U32 *)(buf+n+8)=u->server.microseconds; //what will do this?
	//*(U32 *)(buf+n+8)=u->nego_microseconds;

	return(n+u->server.size);
}

/*--------------------------------------------------------
   Process the (Re)Negotation packets
	 returns 0 for not sending a Negotation reply
	 returns -1 if an error occurs
---------------------------------------------------------*/
int uru_process_negotation(Byte * buf,int n,int start,st_uru_client * u) {
	char * timestamp;
	U32 microseconds;
	time_t stamp;

	u->bandwidth=*(U32 *)(buf+start);
	stamp=*(time_t *)(buf+4+start);
	microseconds=*(U32 *)(buf+8+start); //this is a session idemtifier, I'm very sure

	timestamp=ctime((const time_t *)&stamp);

	print2log(f_uru,"<RCV> (Re)Negotation us: %i bandwidth: %i bps time: %s",microseconds,u->bandwidth,timestamp);
	//free(timestamp);

	if(microseconds>=1000000) { //if microseconds if bigger than 1 second, then something is going wrong..
		stamp2log(f_une);
		ip2log_old(f_une,u->client_ip,u->client_port);
		print2log(f_une,"ERR: Strange microseconds value found in Negotation packet!\n");
		dump_packet(f_une,buf,n,start,5);
		print2log(f_une,"\n");
	}

	//That's all for now about negotation packets
	//printf("<- %i,%i -- %i,%i -->\n",u->client.microseconds,microseconds,(U32)u->client.timestamp,(U32)stamp);
	if(u->client.microseconds==microseconds && u->client.timestamp==stamp) {
		print2log(f_uru,"INF: Discarding old (Re)Negotation packet\n");
		return 0;
	}
	u->client.microseconds=microseconds;
	u->client.timestamp=stamp;

	//2nd test
	/*if(u->nego_microseconds==(U32)microseconds && u->nego_timestamp==(U32)stamp) {
		print2log(f_uru,"INF: 2nd Discarding old (Re)Negotation packet\n");
		return 0;
	}
	u->nego_microseconds=microseconds;
	u->nego_timestamp=stamp;*/

	return (start+u->client.size);
}

/* the next version of the plnetmsg processor
	puts all stuff in a struct... well

*/

/** Gets all plNet msg header vars

*/
int parse_plNet_msg(Byte * buf,st_uru_client * u) {
	int off=0;
	if(buf==NULL) {
		DBG(3,"Null buffer?\n");
	}
	u->adv_msg.cmd=*(U16 *)(buf+off);
	off+=2;
	u->adv_msg.format=*(U32 *)(buf+off);
	off+=4;

	//put here a cmd printer
	print2log(f_uru," %04X %08X\n",u->adv_msg.cmd,u->adv_msg.format);

	if(u->adv_msg.format & plNetAck) {
		print2log(f_uru," Ack flag on\n");
	} else {
		print2log(f_uru," Ack flag off\n");
	}
	if(u->adv_msg.format & plNetFirewalled) {
		print2log(f_uru," Firewalled flag ON\n");
	}

	if(u->adv_msg.format & 0x00010000) {
		u->adv_msg.max_version=*(Byte *)(buf+off);
		off++;
		u->adv_msg.min_version=*(Byte *)(buf+off);
		off++;
		print2log(f_uru," Version: %i.%i\n",u->adv_msg.max_version,u->adv_msg.min_version);
		//now set up version in u var (compatibility)
		//IA-guess
		if(u->major_version==0) {
			u->minor_version=u->adv_msg.min_version;
			u->major_version=u->adv_msg.max_version;
		}
		// END IA
	} else {
		//u->adv_msg.min_version=0;
		//u->adv_msg.max_version=0;
	}

	//IA-guess the protocol version from behaviours
	if(u->adv_msg.format & 0x00000001 && u->major_version==0) {
		u->major_version=0x12; //sure (normally on ping proves)
		u->minor_version=0x06;
	} else if(u->major_version==0) {
		u->major_version=0x12;
		u->minor_version=0x00;
	}// END IA

	//NetMsgPing should have always the timestamp enabled in new versions.
	if(u->adv_msg.format & 0x00000001 || u->minor_version<0x06 || u->adv_msg.cmd==NetMsgPing) {
		u->adv_msg.timestamp=*(U32 *)(buf+off);
		off+=4;
		u->adv_msg.microseconds=*(U32 *)(buf+off);
		off+=4;
		print2log(f_uru," Timestamp: %s\n",get_stime(u->adv_msg.timestamp,u->adv_msg.microseconds));
	} else {
		u->adv_msg.timestamp=0;
		u->adv_msg.microseconds=0;
	}
	if(u->adv_msg.format & 0x00000200) {
		u->adv_msg.x=*(U32 *)(buf+off);
		off+=4;
		print2log(f_uru," x: %i\n",u->adv_msg.x);
	} else {
		u->adv_msg.x=0;
	}
	if(u->adv_msg.format & 0x00001000) {
		u->adv_msg.ki=*(U32 *)(buf+off);
		off+=4;
		print2log(f_uru," ki: %i\n",u->adv_msg.ki);
	} else {
		u->adv_msg.ki=0;
	}
	if(u->adv_msg.format & 0x00004000) {
		print2log(f_uru,"\nThis plNetMsg should have a player guid, not used in our server\n");
	}
	if(u->adv_msg.format & 0x00000200) {
		print2log(f_uru,"\bCustom msg vars present?\n");
	}
	if(u->adv_msg.format & plNetIP) {
		print2log(f_uru," This msg seems to have an ip address attached to it\n");
	}
	//now catch undocumented protocol flags
	if(u->adv_msg.format & 0xEFF8ADDE) {  //0xFFF8ADFE
		stamp2log(f_une);
		ip2log(f_une,u);
		print2log(f_une,"[%s,%s]",u->login,u->avatar_name);
		print2log(f_une,"\nProblem parsing a plNetMsg header format mask %08X\n",u->adv_msg.format & 0xFFF8ADFE);
		dump_packet(f_une,buf,u->client.size,0,0x07);
		print2log(f_une,"\n\n");
	}

	return off;
}

/** puts in the buffer all plnet msg header vars

*/
int put_plNetMsg_header(Byte * buf, st_uru_client * u) {
	int off=0;

	//the msg command
	*(U16 *)(buf+off)=u->adv_msg.cmd;
	off+=2;
	//format flags
	*(U32 *)(buf+off)=u->adv_msg.format;
	off+=4;

	//put here a cmd printer
	print2log(f_uru," %04X %08X\n",u->adv_msg.cmd,u->adv_msg.format);

	if(u->adv_msg.format & plNetAck) {
		print2log(f_uru," Ack flag on\n");
	} else {
		print2log(f_uru," Ack flag off\n");
	}
	if(u->adv_msg.format & plNetFirewalled) {
		print2log(f_uru," Firewalled flag ON\n");
	}

	//put things
	if(u->adv_msg.format & 0x00010000) {
		print2log(f_uru," Version: %i.%i\n",u->adv_msg.max_version,u->adv_msg.min_version);
		*(Byte *)(buf+off)=u->adv_msg.max_version;
		off++;
		*(Byte *)(buf+off)=u->adv_msg.min_version;
		off++;
	}

	//IA-guess the protocol version from behaviours
	if(u->adv_msg.format & 0x00000001 && u->major_version==0) {
		u->major_version=0x12; //sure (normally on ping proves)
		u->minor_version=0x06;
	} else if(u->major_version==0) {
		u->major_version=0x12;
		u->minor_version=0x00;
	}// END IA
	//NetMsgPing should have always the timestamp enabled in new versions.
	if(u->adv_msg.format & 0x00000001 || (u->minor_version<6 && u->major_version==12) || u->adv_msg.cmd==NetMsgPing) {
		DBG(2,"minor version is %i\n",u->minor_version);
		if(u->adv_msg.timestamp==0) {
			time((time_t *)&u->adv_msg.timestamp);
			u->adv_msg.microseconds=0;
		}
		print2log(f_uru," Timestamp: %s\n",get_stime(u->adv_msg.timestamp,u->adv_msg.microseconds));
		*(U32 *)(buf+off)=u->adv_msg.timestamp;
		off+=4;
		*(U32 *)(buf+off)=u->adv_msg.microseconds;
		off+=4;
	}
	if(u->adv_msg.format & 0x00000200) {
		print2log(f_uru," x: %i\n",u->adv_msg.x);
		*(U32 *)(buf+off)=u->adv_msg.x;
		off+=4;
	}
	if(u->adv_msg.format & 0x00001000) {
		print2log(f_uru," ki: %i\n",u->adv_msg.ki);
		*(U32 *)(buf+off)=u->adv_msg.ki;
		off+=4;
	}
	if(u->adv_msg.format & 0x00004000) {
		print2log(f_uru," This plNetMsg should have a player guid, not used in our server\n");
	}
	if(u->adv_msg.format & 0x00020000) {
		print2log(f_uru," Custom msg vars present?\n");
	}
	if(u->adv_msg.format & plNetIP) {
		print2log(f_uru," This msg seems to have an ip address attached to it\n");
	}
	//now catch undocumented protocol flags
	if(u->adv_msg.format & 0xEFF8ADDE) { //0xFFF8ADFE
		stamp2log(f_une);
		ip2log(f_une,u);
		plog(f_une,"[%s,%s]",u->login,u->avatar_name);
		print2log(f_une,"Problem parsing a plNetMsg header format mask %08X\n",u->adv_msg.format & 0xFFF8ADFE);
		//dump_packet(f_une,buf,u->client.size,0,0x07);
		//print2log(f_une,"\n");
		//off=-1;
	}

	return off;
}

char * unet_get_release(int rel) {
	static char * ret;
	switch(rel) {
		case 0x03:
			ret="ExtRel";
			break;
		case 0x02:
			ret="IntRel";
			break;
		case 0x01:
			ret="Dbg";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * unet_get_destination(int dest) {
	static char * ret;
	switch(dest) {
		case 0x01:
			ret="kAgent";
			break;
		case 0x02:
			ret="kLobby";
			break;
		case 0x03:
			ret="kGame";
			break;
		case 0x04:
			ret="kVault";
			break;
		case 0x05:
			ret="kAuth";
			break;
		case 0x06:
			ret="kAdmin";
			break;
		case 0x07:
			ret="kLookup";
			break;
		case 0x08:
			ret="kClient";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * unet_get_reason_code(int code) {
	static char * ret;
	switch(code) {
		case 0x00:
			ret="StopResponding";
			break;
		case 0x14:
			ret="InGame";
			break;
		case 0x16:
			ret="InRoute";
			break;
		case 0x17:
			ret="Arriving";
			break;
		case 0x18:
			ret="Joining";
			break;
		case 0x19:
			ret="Leaving";
			break;
		case 0x1A:
			ret="Quitting";
			break;
		case 0x01:
			ret="Unknown";
			break;
		case 0x02:
			ret="KickedOff";
			break;
		case 0x03:
			ret="TimedOut";
			break;
		case 0x04:
			ret="LoggedInElsewhere";
			break;
		case 0x05:
			ret="NotAuthenticated";
			break;
		case 0x06:
			ret="UnprotectedCCR";
			break;
		case 0x07:
			ret="IllegalCCRClient";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * unet_get_auth_code(int code) {
	static char * ret;
	switch(code) {
		case 0x00:
			ret="AuthSucceeded";
			break;
		case 0x01:
			ret="AuthHello";
			break;
		case 0xF7:
			ret="ProtocolOlder";
			break;
		case 0xF8:
			ret="ProtocolNewer";
			break;
		case 0xFB:
			ret="AccountExpired";
			break;
		case 0xFC:
			ret="AccountDisabled";
			break;
		case 0xFD:
			ret="InvalidPasswd";
			break;
		case 0xFE:
			ret="InvalidUser";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}

char * unet_get_avatar_code(int code) {
	static char * ret;
	switch(code) {
		case 0x00:
			ret="Ok";
			break;
		case 0xF8:
			ret="NameDoesNotHaveEnoughLetters";
			break;
		case 0xF9:
			ret="NameIsTooShort";
			break;
		case 0xFA:
			ret="NameIsTooLong";
			break;
		case 0xFB:
			ret="InvitationNotFound";
			break;
		case 0xFC:
			ret="NameIsAlreadyInUse";
			break;
		case 0xFD:
			ret="NameIsNotAllowed";
			break;
		case 0xFE:
			ret="MaxNumberPerAccountReached";
			break;
		case 0xFF:
			ret="UnspecifiedServerError";
			break;
		default:
			ret="Unknown";
			break;
	}
	return ret;
}


char * unet_get_str_ip(int ip) {
	in_addr cip;
	static char mip[16];
	cip.s_addr=(unsigned long)ip;
	strcpy(mip, inet_ntoa(cip));
	return mip;
}

#endif
