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

/* The Uru protocol, is here */

/*

Uru Protocol
No sockets please

 --*/

/* CVS tag - DON'T TOUCH*/
#define __U_PROTOCOL_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//for the md5 checksum
#include "md5.h" //but we get the function from the crypto library, on windows we use
                 // that implementation

#include "stdebug.h"
#include "urunet.h"
#include "conv_funs.h"
#include "useful.h"
#include "prot.h"

#include "protocol.h"

#include "debug.h"

st_log * f_chk=NULL;
st_log * f_une=NULL;

/* These numbers must never be touched */
const int unet_max_version = 1;
const int unet_min_version = 2; //this number is increassed on protocol changes.
/* Backwards compatibility is a must */

/**
 \brief Encodes the specific packet into Uru protocol 2
*/

/* k = offset MOD 8
   enc: c = x * 2 ^ k MOD 255
*/
void encode_packet(unsigned char* buf2,unsigned char* buf, int n) {
	int i;
	for(i=0; i<n; i++) {
		buf2[i] = buf[i] << (i%8) | buf[i] >> (8-(i%8));
	}
}

/**
 Decodes the specific packet from Uru protocol 2
*/

/* k = offset MOD 8
   dec: x = c * 2 ^ (8-k) MOD 255
*/
void decode_packet(unsigned char* buf, int n) {
	int i;
	for(i=0; i<n; i++) {
		buf[i] = buf[i] >> (i%8) | buf[i] << (8-(i%8));
	}
}

/** Debug the V1 checksum */
U32 uru_checksum1trace(Byte * buf, int size) {
	int i;
	U32 aux=0;
	//S32 saux=0;
	//Byte * md5buffer;
	//Byte hash[16];
	//int aux_size; //auxiliar size
	int whoi=0;

	DBG(4,"Checksum %i requested, packet size is %i...\n",1,size);
	//be sure that the last chunck of bytes is zero
	for(i=size; i<size+4; i++) {
		if(i>=INC_BUF_SIZE) { break; }
		buf[i]=0;
	}
	for(i=6; i<size; i=i+4) {
		aux = aux + *((U32 *)(buf+i)); //V1 - Checksum algorithm ~ it fails in some messages :/
		print2log(f_chk," %08X   -  %08X\n",*((U32 *)(buf+i)),aux);
	}
	//extension <--
	whoi=((size-6)%4);
	print2log(f_chk,"------------- lst chunck %i bytes\n",whoi);
	print2log(f_chk," %08X  -->>",aux);
	if(whoi==2) {
		aux -= *((U32 *)(buf+size-2));
		aux += *((Byte *)(buf+size-1));
	} else if(whoi==3) {
		aux -= *((U32 *)(buf+size-3));
	}
	//aux=saux;
	print2log(f_chk," %08X\n\n",aux);
	return aux;
}

/**
  \brief Computes the Uru known checksums
  \param alg
	 0 -> prime sum,
	 1 -> live md5 sum,
	 2 -> live md5 + passwd sum,
  \note REMEMBER THAT aux_hash must be 32 bytes and contain an ascii hash
*/
U32 uru_checksum(Byte* buf, int size, int alg, Byte * aux_hash) {
	int i;
	U32 aux=0;
	//S32 saux=0;
	Byte * md5buffer;
	Byte hash[16];
	int aux_size; //auxiliar size
	int whoi=0;

	DBG(4,"Checksum %i requested, packet size is %i...\n",alg,size);
	switch(alg) {
		case 0:
			//be sure that the last chunck of bytes is zero
			//for(i=size; i<size+4; i++) {
			//	if(i>=INC_BUF_SIZE) { break; }
			//	buf[i]=0;
			//}  HA HA HA HA HA HA HA HA HA HA HA HA HA HA HA HA HA HA HA...
			for(i=6; i<(size-4); i=i+4) {
				aux = aux + *((U32 *)(buf+i)); //V1 - working Checksum algorithm
			}

			//extension <--
			whoi=((size-6)%4);
			if(whoi==1) {
				aux += *((Byte *)(buf+size-1));
			} else if(whoi==2) {
				//aux -= *((U32 *)(buf+size-2));
				aux += *((Byte *)(buf+size-1));
			} else if(whoi==3) {
				//aux -= *((U32 *)(buf+size-3));
			} else if(whoi==0) {
				aux += *((U32 *)(buf+size-4));
			}

			//aux=saux;
			break;
		case 1:
		case 2:
			//code for the V2 - Checksum algorithm
			aux_size=size-6;
			if(alg==2) { aux_size+=32; }
			//allocate the space for the buffer
			DBG(4,"Allocating md5buffer - %i bytes...\n",aux_size);
			md5buffer = (Byte *)malloc(sizeof(Byte)*(aux_size+10));
			if(md5buffer==NULL) {
				DBG(4,"md5buffer is NULL?\n");
				return 0xFFFFFFFF;
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
			/*if(alg==2) {
				dumpbuf(f_uru,md5buffer,aux_size);
				abort();
			}*/
			DBG(5,"computing md5...\n");
			MD5(md5buffer, aux_size, hash);
			//print2log(f_chkal,"\n<-\n");

			aux = *((U32 *)hash);
			DBG(5,"after a free call\n");
			free(md5buffer);
			DBG(5,"before a free call\n");

			break;
		default:
			//print2log(f_chkal,"ERR: Unimplemented Checksum V%i algorithm requested\n",alg);
			plog(f_err,"ERR: Uru Checksum V%i is currenlty not supported in this version of the server.\n\n",alg);
			aux = 0xFFFFFFFF;
	}
	return aux;
}

/**
\brief Validates a packet
\note 
	1st- Gets the validation level,
	2nd- Checks if is a valid Uru Protocol formated packet,
	3th- compute the checksum,
	4th- Decode the packet,
	5th- put the results in the session struct,

	Net - NO

\return
	 0 -> Yes, all went OK! :)
	 1 -> Ok, but checksum failed :(
	 2 -> Validation level too high
	 3 -> Bogus packet!!
*/
int uru_validate_packet(Byte * buf,int n,st_uru_client * u) {
	U32 checksum;
#ifndef _NO_CHECKSUM
	U32 aux_checksum;
#endif
	if(n>=2 && buf[0]==0x03) { //magix uru number
		if(buf[1]!=0) {
			u->validation=buf[1]; //store the validation level
		}
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
		else {
			//work around (swap the checksum algorithms)
			if(buf[1]==0x02) { //validation level 2
				encode_packet(buf,buf,n);
				buf[1]=0x02;
				if(u->authenticated==1) { //authenticated?
					//hex2ascii(u->passwd,hash,16); //get the string of the hash
					aux_checksum=uru_checksum(buf, n, 1, NULL);
				} else {
					aux_checksum=uru_checksum(buf, n, 2, u->passwd);
				}
				decode_packet(buf,n); //Now the packet will be readable
				//be sure to take bak the 0x02 byte to the header
				buf[1]=0x02;
				if(aux_checksum==checksum) { u->validated=1; return 0; } //All went ok with v1
			}

			plog(f_chk,"ERR: Validation level %i, got: %08X exp: %08X\n",u->validation,u->client.cs,aux_checksum);
			if(u->validation==1) {
				uru_checksum1trace(buf, n);
			}
			dumpbuf(f_chk,buf,n);
			lognl(f_chk);
			u->validated=0;
			return 1;
		} //:(
#else
		return 0; //allow all packets
#endif
	}
	return 3; //aiieee!!!
}


/**
\brief assings the header structure
of the specified buffer (ACK flags)

\return if success returns the number of the first data byte
on any other case returns 0
*/
int uru_get_header(unsigned char * buf,int n,st_uru_head * u) {
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

	#if _DBG_LEVEL_ > 5
	if(u->p_n>50000) {
		DBG(5,"invalid pn %i\n",u->p_n);
		dumpbuf(f_err,buf,n);
		//abort();
	}
	#endif

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
	u->unknownB=*((U32 *)(buf+offset));

	if(u->unknownB!=0) { non_std=1; }
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
		//abort();
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

/**
   \brief Prints the Uru Encapsulation header of a packet
*/
void uru_print_header(st_log * f_dsc,st_uru_head * u) {
	print2log(f_dsc,"[%i] ->%02X<- {%i,%i (%i) %i,%i} - %02X|%i bytes",u->p_n,u->t,u->fr_n,u->sn,u->fr_t,u->fr_ack,u->ps,u->size,u->size);
}

/**
  \brief Gets the size of the header, and where the data starts
*/
int uru_get_header_start(st_uru_head * u) {
	if(u->ch==0x01 || u->ch==0x02) return 32; //with checksum
	else return 28; //with no-checksum
}

#if 0

/**
  Increments the packet counter, and updates the buffer
*/
void uru_update_packet_counter(Byte * buf, st_uru_head * u) {
	u->p_n++;
	if(buf[1]==0x01 || buf[1]==0x02) {
		*((U32 *)(buf+6))=u->p_n;
	} else {
		*((U32 *)(buf+2))=u->p_n;
	}
}

/**
  Increases the message counter, with the specified flags
*/
void uru_inc_msg_counter(uru_head * server) {
		//update flags
		server->p_n++;
		server->sn++;
}

#endif

/**
\brief assings the header structure to the specified buffer
\return returns the size of the header
*/
int uru_put_header(unsigned char * buf,st_uru_head * u) {
	int offset=2;

	//if(u->ch==0x01 && u->t==0x42) abort();

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

#if 0

/**
Initialitzes (zeroes) the uru header
*/
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

/**
  updates the ack flags, and generates a new ack packet
	that will  be put in the buffer
*/
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

/**
  \brief Parses and ACK reply, and updates the flags
	also, it need to clear from the buffer the acked
	packet and do other stuff.
  \return Returns 0 if it fails, returns size on success
*/
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

/**
  \brief Creates a valid negotation packet (I think)
  \return returns the size
*/
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

/**
   \brief Process the (Re)Negotation packets
   \return returns 0 for not sending a Negotation reply
   \return returns -1 if an error occurs
*/
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

#endif

/** Gets all plNet msg header vars
		/return returns the size
*/
int parse_plNet_msg(st_unet * net,Byte * buf,int size,int sid) {
	st_uru_client * u=&net->s[sid];
	int off=0;
	if(buf==NULL) {
		DBG(3,"Null buffer? (size:%i,sid:%i)\n",size,sid);
		return -1;
	}
	u->hmsg.cmd=*(U16 *)(buf+off);
	off+=2;
	u->hmsg.flags=*(U32 *)(buf+off);
	off+=4;

	//put here a cmd printer
	print2log(net->log,"\n %04X %08X\n",u->hmsg.cmd,u->hmsg.flags);

	if(u->hmsg.flags & plNetAck) {
		print2log(net->log," Ack flag on\n");
	} else {
		print2log(net->log," Ack flag off\n");
	}
	if(u->hmsg.flags & plNetFirewalled) {
		print2log(net->log," Firewalled flag ON\n");
	}
	if(u->hmsg.flags & plNetBcast) {
		print2log(net->log," Bcast flag ON\n");
	}
	if(u->hmsg.flags & plNetCustom) {
		print2log(net->log," UCPNPI flag ON\n");
	}

	if(u->hmsg.flags & plNetVersion) {
		u->hmsg.max_version=*(Byte *)(buf+off);
		off++;
		u->hmsg.min_version=*(Byte *)(buf+off);
		off++;
		print2log(net->log," Version: %i.%i\n",u->hmsg.max_version,u->hmsg.min_version);
		//now set up version in u var (compatibility)
		//IA-guess
		if(u->max_version==0) {
			u->max_version=u->hmsg.max_version;
			u->min_version=u->hmsg.min_version;
		}
		// END IA
	}// else {
		//u->adv_msg.min_version=0;
		//u->adv_msg.max_version=0;
	//}

	//IA-guess the protocol version from behaviours
	// The first message is always an auth hello that contains the version numbers
	if(u->hmsg.flags & plNetTimestamp && u->max_version==0) {
		u->max_version=12; //sure (normally on ping proves)
		u->min_version=6;
	} else if(u->max_version==0) {
		u->max_version=12;
		u->min_version=0;
	}// END IA

	DBG(5,"Detected version is %i.%i\n",u->max_version,u->min_version);
	//abort();

	//NetMsgPing should have always the timestamp enabled in new versions.
	if(u->hmsg.flags & plNetTimestamp || u->min_version<0x06) { // || u->hmsg.cmd==NetMsgPing) {
		u->hmsg.stamp=*(U32 *)(buf+off);
		off+=4;
		u->hmsg.micros=*(U32 *)(buf+off);
		off+=4;
		print2log(f_uru," Timestamp: %s\n",get_stime(u->hmsg.stamp,u->hmsg.micros));
	} else {
		u->hmsg.stamp=0;
		u->hmsg.micros=0;
	}
	if(u->hmsg.flags & plNetX) {
		u->hmsg.x=*(U32 *)(buf+off);
		off+=4;
		print2log(net->log," x: %i\n",u->hmsg.x);
	} else {
		u->hmsg.x=0;
	}
	if(u->hmsg.flags & plNetKi) {
		u->hmsg.ki=*(U32 *)(buf+off);
		off+=4;
		print2log(net->log," ki: %i\n",u->hmsg.ki);
	} else {
		u->hmsg.ki=0;
	}
	if(u->hmsg.flags & plNetGUI) {
		memcpy(u->hmsg.guid,buf+off,16);
		off+=16;
		print2log(net->log," guid: %s\n",create_str_guid(u->hmsg.guid));
	}
	if(u->hmsg.flags & plNetIP) {
		u->hmsg.ip=*(U32 *)(buf+off);
		off+=4;
		u->hmsg.port=*(U16 *)(buf+off);
		off+=2;
		print2log(f_uru," ip: %s:%i\n",get_ip(u->hmsg.ip),u->hmsg.port);
	}
	//now catch undocumented protocol flags

	if(u->hmsg.flags & ~(plNetAck | plNetBcast | plNetVersion | plNetTimestamp | \
	plNetX | plNetKi | plNetGUI | plNetIP | plNetCustom)) {
		nlog(net->unx,net,sid,"Problem parsing a plNetMsg header format mask %08X\n",u->hmsg.flags & ~(plNetAck | plNetBcast | plNetVersion | plNetTimestamp | \
	plNetX | plNetKi | plNetGUI | plNetIP | plNetCustom));
		dumpbuf(net->unx,buf,size);
		lognl(net->unx);
		lognl(net->unx);
	}

	return off;
}

/**
	puts in the buffer all plnet msg header vars
*/
int put_plNetMsg_header(st_unet * net,Byte * buf,int size,int sid) {
	st_uru_client * u=&net->s[sid];
	int off=0;

	//the msg command
	*(U16 *)(buf+off)=u->hmsg.cmd;
	off+=2;
	//format flags
	*(U32 *)(buf+off)=u->hmsg.flags;
	off+=4;

	//put here a cmd printer
	print2log(net->log,"\n %04X %08X\n",u->hmsg.cmd,u->hmsg.flags);

	if(u->hmsg.flags & plNetAck) {
		print2log(net->log," Ack flag on\n");
	} else {
		print2log(net->log," Ack flag off\n");
	}
	if(u->hmsg.flags & plNetFirewalled) {
		print2log(net->log," Firewalled flag ON\n");
	}
	if(u->hmsg.flags & plNetBcast) {
		print2log(net->log," Bcast flag ON\n");
	}
	if(u->hmsg.flags & plNetCustom) {
		print2log(net->log," UCPNPI flag ON\n");
	}

	//put things
	if(u->hmsg.flags & plNetVersion) {
		print2log(net->log," Version: %i.%i\n",u->hmsg.max_version,u->hmsg.min_version);
		*(Byte *)(buf+off)=u->hmsg.max_version;
		off++;
		*(Byte *)(buf+off)=u->hmsg.min_version;
		off++;
	}

	//we must specify a valid version number after sending the first message
#if 0
	//IA-guess the protocol version from behaviours
	if(u->hmsg.flags & plNetTimestamp && u->max_version==0) {
		u->max_version=12; //sure (normally on ping proves)
		u->min_version=6;
	} else if(u->max_version==0) {
		u->max_version=12;
		u->min_version=0;
	}// END IA
#endif

	//NetMsgPing should have always the timestamp enabled in new versions.
	if(u->hmsg.flags & plNetTimestamp || (u->min_version<6 && u->max_version==12)) { // || u->adv_msg.cmd==NetMsgPing) {
		DBG(2,"minor version is %i\n",u->min_version);
		if(u->hmsg.stamp==0) {
			time((time_t *)&u->hmsg.stamp);
			u->hmsg.micros=get_microseconds();
		}
		print2log(net->log," Timestamp: %s\n",get_stime(u->hmsg.stamp,u->hmsg.micros));
		*(U32 *)(buf+off)=u->hmsg.stamp;
		off+=4;
		*(U32 *)(buf+off)=u->hmsg.micros;
		off+=4;
	}
	if(u->hmsg.flags & plNetX) {
		print2log(net->log," x: %i\n",u->hmsg.x);
		*(U32 *)(buf+off)=u->hmsg.x;
		off+=4;
	}
	if(u->hmsg.flags & plNetKi) {
		print2log(net->log," ki: %i\n",u->hmsg.ki);
		*(U32 *)(buf+off)=u->hmsg.ki;
		off+=4;
	}

	if(u->hmsg.flags & plNetGUI) {
		memcpy(buf+off,u->hmsg.guid,16);
		off+=16;
		print2log(net->log," guid: %s\n",create_str_guid(u->hmsg.guid));
	}
	if(u->hmsg.flags & plNetIP) {
		*(U32 *)(buf+off)=u->hmsg.ip;
		off+=4;
		*(U16 *)(buf+off)=u->hmsg.port;
		off+=2;
		print2log(f_uru," ip: %s:%i\n",get_ip(u->hmsg.ip),u->hmsg.port);
	}

#if 0
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
#endif //this was useless

	return off;
}


/**
	copies one plNetMsg header to another one
	sid=destination
	ssid=source
*/
void copy_plNetMsg_header(st_unet * net,int sid,int ssid,int flags) {
	if(net_check_address(net,sid)!=0) return;
	if(net_check_address(net,ssid)!=0) return;
	st_uru_client * u=&net->s[sid];
	st_uru_client * s=&net->s[ssid];
	//int off=0;

	//the msg command
	u->hmsg.cmd=s->hmsg.cmd;
	//format flags
	u->hmsg.flags=s->hmsg.flags;

	//put here a cmd printer
	//print2log(net->log," %04X %08X\n",u->hmsg.cmd,u->hmsg.flags);

	/*
	if(flags & plNetAck) {
		print2log(net->log," Ack flag on\n");
	} else {
		print2log(net->log," Ack flag off\n");
	}
	if(u->hmsg.flags & plNetFirewalled) {
		print2log(net->log," Firewalled flag ON\n");
	}
	if(u->hmsg.flags & plNetBcast) {
		print2log(net->log," Bcast flag ON\n");
	}
	if(u->hmsg.flags & plNetCustom) {
		print2log(net->log," UCPNPI flag ON\n");
	}
	*/

	//put things
	if(flags & plNetVersion) {
		//print2log(net->log," Version: %i.%i\n",u->hmsg.max_version,u->hmsg.min_version);
		u->hmsg.max_version=s->hmsg.max_version;
		u->hmsg.min_version=s->hmsg.min_version;
	}

	//NetMsgPing should have always the timestamp enabled in new versions.
	if(flags & plNetTimestamp) {
		//DBG(2,"minor version is %i\n",u->min_version);
		//print2log(net->log," Timestamp: %s\n",get_stime(u->hmsg.stamp,u->hmsg.micros));
		u->hmsg.stamp=s->hmsg.stamp;
		u->hmsg.micros=s->hmsg.stamp;
	}
	if(flags & plNetX) {
		//print2log(net->log," x: %i\n",u->hmsg.x);
		u->hmsg.x=s->hmsg.x;
	}
	if(flags & plNetKi) {
		//print2log(net->log," ki: %i\n",u->hmsg.ki);
		u->hmsg.ki=s->hmsg.ki;
	}

	if(flags & plNetGUI) {
		memcpy(u->hmsg.guid,s->hmsg.guid,16);
		//print2log(net->log,"\n guid: %s\n",get_guid(u->hmsg.guid));
	}
	if(flags & plNetIP) {
		u->hmsg.ip=s->hmsg.ip;
		u->hmsg.port=s->hmsg.port;
		//print2log(f_uru," ip: %s:%i\n",get_ip(u->hmsg.ip),u->hmsg.port);
	}

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
		case KMeta:
			ret="kMeta";
			break;
		case KData:
			ret="kData";
			break;
		case KBcast:
			ret="kBcast";
			break;
		case KTest:
			ret="kTest";
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
			ret="Active";
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
			ret="AccountDissabled";
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

char * unet_get_msg_code(U16 code) {
	static char * ret;
	switch(code) {
		case 0x0218:
			ret="NetMsgPagingRoom";
			break;
		case 0x025A:
			ret="NetMsgJoinReq";
			break;
		case 0x025B:
			ret="NetMsgJoinAck";
			break;
		case 0x025C:
			ret="NetMsgLeave";
			break;
		case 0x025D:
			ret="NetMsgPing";
			break;
		case 0x025F:
			ret="NetMsgGroupOwner";
			break;
		case 0x0260:
			ret="NetMsgGameStateRequest";
			break;
		case 0x0266:
			ret="NetMsgGameMessage";
			break;
		case 0x0274:
			ret="NetMsgVoice";
			break;
		case 0x0278:
			ret="NetMsgTestAndSet";
			break;
		case 0x02A8:
			ret="NetMsgMembersListReq";
			break;
		case 0x02A9:
			ret="NetMsgMembersList";
			break;
		case 0x02AC:
			ret="NetMsgMemberUpdate";
			break;
		case 0x02AE:
			ret="NetMsgCreatePlayer";
			break;
		case 0x02AF:
			ret="NetMsgAuthenticateHello";
			break;
		case 0x02B0:
			ret="NetMsgAuthenticateChallenge";
			break;
		case 0x02B3:
			ret="NetMsgInitialAgeStateSent";
			break;
		case 0x02BE:
			ret="NetMsgVaultTask";
			break;
		case 0x02C5:
			ret="NetMsgAlive";
			break;
		case 0x02C6:
			ret="NetMsgTerminated";
			break;
		case 0x02C8:
			ret="NetMsgSDLState";
			break;
		case 0x0324:
			ret="NetMsgSDLStateBCast";
			break;
		case 0x034E:
			ret="NetMsgRequestMyVaultPlayerList";
			break;
		case 0x0373:
			ret="NetMsgVaultPlayerList";
			break;
		case 0x0374:
			ret="NetMsgSetMyActivePlayer";
			break;
		case 0x0377:
			ret="NetMsgPlayerCreated";
			break;
		case 0x037A:
			ret="NetMsgFindAge";
			break;
		case 0x037B:
			ret="NetMsgFindAgeReply";
			break;
		case 0x0384:
			ret="NetMsgDeletePlayer";
			break;
		case 0x0393:
			ret="NetMsgAuthenticateResponse";
			break;
		case 0x0394:
			ret="NetMsgAccountAuthenticated";
			break;
		case 0x03AE:
			ret="NetMsgLoadClone";
			break;
		case 0x03AF:
			ret="NetMsgPlayerPage";
			break;
		case 0x0428:
			ret="NetMsgVault";
			break;
		case 0x0429:
			ret="NetMsgVault2";
			break;
		case 0x0464:
			ret="NetMsgSetTimeout";
			break;
		case 0x0465:
			ret="NetMsgActivePlayerSet_NetMsgSetTimeout2";
			break;
		case 0x0466:
			ret="NetMsgActivePlayerSet2";
			break;
		case 0x1001:
			ret="NetMsgCustomAuthAsk";
			break;
		case 0x1002:
			ret="NetMsgCustomAuthResponse";
			break;
		case 0x1003:
			ret="NetMsgCustomVaultAskPlayerList";
			break;
		case 0x1004:
			ret="NetMsgCustomVaultPlayerList";
			break;
		case 0x1005:
			ret="NetMsgCustomVaultCreatePlayer";
			break;
		case 0x1006:
			ret="NetMsgCustomVaultPlayerCreated";
			break;
		case 0x1007:
			ret="NetMsgCustomVaultDeletePlayer";
			break;
		case 0x1008:
			ret="NetMsgCustomPlayerStatus";
			break;
		case 0x1009:
			ret="NetMsgCustomVaultCheckKi";
			break;
		case 0x100A:
			ret="NetMsgCustomVaultKiChecked";
			break;
		case 0x100B:
			ret="NetMsgCustomRequestAllPlStatus";
			break;
		case 0x100C:
			ret="NetMsgCustomAllPlayerStatus";
			break;
		case 0x100D:
			ret="NetMsgCustomSetGuid";
			break;
		case 0x100E:
			ret="NetMsgCustomFindServer";
			break;
		case 0x100F:
			ret="NetMsgCustomServerFound";
			break;
		case 0x1010:
			ret="NetMsgCustomForkServer";
			break;
		case 0x1011:
			ret="NetMsgPlayerTerminated";
			break;
		case 0x1012:
			ret="NetMsgCustomVaultPlayerStatus";
			break;
		case 0x1013:
			ret="NetMsgCustomMetaRegister";
			break;
		case 0x1014:
			ret="NetMsgCustomMetaPing";
			break;
		default:
			ret="fix_me";
			break;
	}
	return ret;
}


//flux 0 client -> server, 1 server -> client
void htmlDumpHeader(st_log * log,st_uru_client c,st_uru_head h,Byte * buf,int size,int flux) {
	if(log==NULL) return;

	static int count=0;
	count++;

	print2log(log,"%i: ",count);
	stamp2log(log);

	switch(h.t) {
		case NetAck: //0x80
			print2log(log,"<font color=red>");
			break;
		case NetClientComm: //0x42
			print2log(log,"<font color=green>");
			break;
		case NetMsg0: //0x00
			print2log(log,"<font color=blue>");
			break;
		case NetMsg1: //0x02
			print2log(log,"<font color=black>");
			break;
		default:
			print2log(log,"<font color=pink>");
			break;
	}


	if(flux==0) {
		print2log(log,"<b> me <- ");
	} else {
		print2log(log," me -> ");
	}
	print2log(log,"%i %i,%i (%i) %i,%i ",h.p_n,h.fr_n,h.sn,h.fr_t,h.fr_ack,h.ps);

	if(flux==0) {
	print2log(log," <- %s:%i</b> ",get_ip(c.ip),ntohs(c.port));
	} else {
	print2log(log," -> %s:%i ",get_ip(c.ip),ntohs(c.port));
	}

	int i;

	switch(h.t) {
		case NetAck: //0x80
			print2log(log,"ack");
			for(i=0; i<(int)h.size; i++) {
				if(i!=0) { print2log(log," |"); }
				print2log(log," %i,%i %i,%i",*(Byte *)((buf+2)+i*0x10),*(U32 *)((buf+3)+i*0x10),*(Byte *)((buf+2+8)+i*0x10),*(U32 *)((buf+3+8)+i*0x10));
			}
			break;
		case NetClientComm: //0x42
			print2log(log,"Negotiation ");
			char * times;
			times=ctime((const time_t *)(buf+4));
			print2log(log,"%i bps, %s %i ms",*(U32 *)(buf),times,*(U32 *)(buf+8));
			break;
		case NetMsg0: //0x00
			print2log(log,"plNetMsg0 ");
			break;
		case NetMsg1: //0x02
			print2log(log,"plNetMsg1 ");
			break;
		default:
			print2log(log,"ERROR! ");
			break;
	}

	print2log(log,"</font>");

	if((h.t==NetMsg0 || h.t==NetMsg1)) {
		if(h.fr_n==0) {
			print2log(log,"(%04X) %s %08X",*(U16 *)(buf),unet_get_msg_code(*(U16 *)(buf)),*(U32 *)(buf+2));
		} else {
			print2log(log,"frg..");
		}
	}
	print2log(log,"<br>\n");

	logflush(log);

}

void htmlDumpHeaderRaw(st_unet * net,st_log * log,st_uru_client c,Byte * buf,int size,int flux) {

	st_uru_head h;
	int aux;

	aux=uru_get_header(buf,size,&h);

	if(flux==1) {
		print2log(net->log,"<SND>");
	} else {
		print2log(net->log,"<RCV>");
	}
	uru_print_header(net->log,&h);
	print2log(net->log,"\n");

	htmlDumpHeader(log,c,h,buf+aux,size,flux);
}

#if 0

char * unet_get_str_ip(int ip) {
	in_addr cip;
	static char mip[16];
	cip.s_addr=(unsigned long)ip;
	strcpy(mip, inet_ntoa(cip));
	return mip;
}

#endif
