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

#ifndef __MSG_SPARSER_S
#define __MSG_SPARSER_S

#define __MSG_SPARSER_S_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

//Another message parser
#include "stdebug.h"
#include "data_types.h"
#include "vaultstrs.h"
#include "urumsg.h"
#include "uru.h"

#include "msg_parser.h"

int parse_uru_object_desc(Byte * buf,st_uru_object_desc * o) {
	int offset=0;

	o->flag=*(Byte *)(buf+offset);
	offset++;
	if(o->flag!=0x01) {
		print2log(f_une,"UNE: invalid object flag %02X\n",o->flag);
		return -1;
	}
	o->page_id=*(U32 *)(buf+offset);
	offset+=4;
	o->page_type=*(U16 *)(buf+offset);
	offset+=2;
	//--
	if(o->flag==0x02) {
		o->unk1=*(Byte *)(buf+offset);
	}
	//--
	o->type=*(U16 *)(buf+offset);
	offset+=2;
	offset+=decode_urustring(o->name,buf+offset,STR_MAX_SIZE);
	offset+=2;
	o->unk2=*(U32 *)(buf+offset);
	offset+=4;
	//if(o->unk2!=0x01) { //also seen 0x04
		//print2log(f_une,"UNE: invalid object unk2 %08X\n",o->unk2);
		//return -1;
	//}
	o->node_id=*(U32 *)(buf+offset);
	offset+=4;

	return offset;
}


int parse_sub_msg(Byte * buf,int size,st_sub_msg * s) {
	int offset=0;
	int ret=0;

	s->type=*(U16 *)(buf+offset);
	offset+=2;
	if(s->type!=0x03AC) { //plNetLoadAvatar
		print2log(f_une,"UNE: type %04X - sub msg\n",s->type);
		return -1;
	}
	s->unk0=*(Byte *)(buf+offset);
	offset++;
	if(s->unk0!=0x00) {
		print2log(f_une,"UNE: unk0 %02X - sub msg\n",s->unk0);
		return -1;
	}
	s->unk1=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk1!=0x01) {
		print2log(f_une,"UNE: unk1 %08X - sub msg\n",s->unk1);
		return -1;
	}
	s->unk2=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk2!=0x01) {
		print2log(f_une,"UNE: unk2 %08X - sub msg\n",s->unk2);
		return -1;
	}
	s->unk3=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk3!=0x00) {
		print2log(f_une,"UNE: unk3 %08X - sub msg\n",s->unk3);
		return -1;
	}
	s->k_type=*(U16 *)(buf+offset);
	offset+=2;
	if(s->k_type!=0x0052) { //plNetClientMgr
		print2log(f_une,"UNE: k_type %04X - sub msg\n",s->k_type);
		return -1;
	}
	offset+=decode_urustring(s->ks_type,buf+offset,STR_MAX_SIZE);
	offset+=2;
	if(strcmp((char *)s->ks_type,"kNetClientMgr_KEY")) {
		print2log(f_une,"UNE: ks_type %s sub msg\n",s->ks_type);
		return -1;
	}
	s->unk4=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk4!=0x00) {
		print2log(f_une,"UNE: unk4 %08X - sub msg\n",s->unk4);
		return -1;
	}
	s->unk5=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk5!=0x00) {
		print2log(f_une,"UNE: unk5 %08X - sub msg\n",s->unk5);
		return -1;
	}
	s->unk6=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk6!=0x00000840 && s->unk6!=0x00000040) {
		print2log(f_une,"UNE: unk6 %08X - sub msg\n",s->unk6);
		return -1;
	}
	s->unk7=*(Byte *)(buf+offset);
	offset++;
	if(s->unk7!=0x01) {
		print2log(f_une,"UNE: unk7 %02X - sub msg\n",s->unk7);
		return -1;
	}

	//the object
	ret=parse_uru_object_desc(buf+offset,&(s->object));
	if(ret<0) {
		print2log(f_une,"UNE: error parsing an uru object\n");
		return -1;
	}
	offset+=ret;

	s->unk8=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk8!=0x01) {
		print2log(f_une,"UNE: unk8 %08X - sub msg\n",s->unk8);
		return -1;
	}
	s->unk9=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk9!=0x00) {
		print2log(f_une,"UNE: unk9 %08X - sub msg\n",s->unk9);
		return -1;
	}
	s->k_type_val=*(U16 *)(buf+offset);
	offset+=2;
	if(s->k_type_val!=0x00F4) { //plAvatarMgr
		print2log(f_une,"UNE: k_type_val %04X - sub msg\n",s->k_type_val);
		return -1;
	}
	offset+=decode_urustring(s->ks_type_val,buf+offset,STR_MAX_SIZE);
	offset+=2;
	if(strcmp((char *)s->ks_type_val,"kAvatarMgr_KEY")) {
		print2log(f_une,"UNE: ks_type %s - sub msg\n",s->ks_type);
		return -1;
	}
	s->node_id=*(U32 *)(buf+offset);
	offset+=4;

	s->unk10=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk10!=0x00) {
		print2log(f_une,"UNE: unk10 %08X - sub msg\n",s->unk10);
		return -1;
	}
	s->unk11=*(Byte *)(buf+offset);
	offset++;
	if(s->unk11!=0x01) {
		print2log(f_une,"UNE: unk11 %08X - sub msg\n",s->unk11);
		return -1;
	}
	s->unk12=*(U16 *)(buf+offset);
	offset+=2;
	if(s->unk12!=0x01 && s->unk12!=0x00) {
		print2log(f_une,"UNE: unk12 %02X - sub msg\n",s->unk12);
		return -1;
	}
	s->unk13=*(U32 *)(buf+offset);
	offset+=4;
	if(s->unk13!=0x0180) {
		print2log(f_une,"UNE: unk13 %08X - sub msg\n",s->unk13);
		return -1;
	}

	if(size!=offset) {
		print2log(f_une,"UNE: Unexpected size mismatch %i vs %i parsing a sub message!\n",size,offset);
		return -1;
	}

	return size;
}


int parse_adv_msg(Byte * buf,int size,st_uru_adv_msg * m,st_uru_client * u) {
	int offset=0;
	int ret=0;

	//m->cmd=*(U16 *)(buf+offset);
	//offset+=2;
	//m->format=*(U32 *)(buf+offset);
	//offset+=4;

	m->ki=u->adv_msg.ki;
	/*m->ki=*(U32 *)(buf+offset);
	offset+=4;*/
	m->unk1=*(U32 *)(buf+offset);
	offset+=4;
	if(m->unk1!=0) {
		print2log(f_une,"UNE: non-zero unk1 value on adv msg %08X\n",m->unk1);
		return -1;
	}
	m->unk2=*(Byte *)(buf+offset);
	offset++;
	if(m->unk2!=0) {
		print2log(f_une,"UNE: non-zero unk2 value on adv msg %02X\n",m->unk2);
		return -1;
	}
	m->s_msg_size=*(U32 *)(buf+offset);
	offset+=4;

	//the sub message
	ret=parse_sub_msg(buf+offset,m->s_msg_size,&(m->msg));
	if(ret<0) {
		print2log(f_une,"UNE: error parsing sub msg\n");
		return -1;
	}
	offset+=ret;

	m->unk3=*(Byte *)(buf+offset);
	offset++;
	if(m->unk3!=0) {
		print2log(f_une,"UNE: non-zero unk3 value on adv msg %02X\n",m->unk3);
		return -1;
	}

	//the object
	ret=parse_uru_object_desc(buf+offset,&(m->object));
	if(ret<0) {
		print2log(f_une,"UNE: error parsing an uru object\n");
		return -1;
	}
	offset+=ret;

	m->unk4=*(Byte *)(buf+offset);
	offset++;
	if(m->unk4!=1 && m->unk4!=1) {
		print2log(f_une,"UNE: unk4 value on adv msg %02X\n",m->unk4);
		return -1;
	}
	m->unk5=*(Byte *)(buf+offset);
	offset++;
	if(m->unk5!=1 && m->unk5!=0) {
		print2log(f_une,"UNE: unk5 value on adv msg %02X\n",m->unk5);
		return -1;
	}

	if(u->minor_version>=0x06) { //only in live, todni & tpots
		m->unk6=*(Byte *)(buf+offset);
		offset++;
		if(m->unk6!=0 && m->unk6!=1) {
			print2log(f_une,"UNE: unk6 value on adv msg %02X\n",m->unk6);
			return -1;
		}
	} else {
		m->unk6=0; //???
	}

	if(size!=offset) {
		print2log(f_une,"UNE: Unexpected size mismatch %i vs %i parsing an adv message!\n",size,offset);
		return -1;
	}

	return size;
}

int put_uru_object_desc(Byte * buf,st_uru_object_desc * o) {
	int offset=0;

	*(Byte *)(buf+offset)=o->flag;
	offset++;
	*(U32 *)(buf+offset)=o->page_id;
	offset+=4;
	*(U16 *)(buf+offset)=o->page_type;
	offset+=2;
	//--
	if(o->flag==0x02) {
		*(Byte *)(buf+offset)=o->unk1;
	}
	//--
	*(U16 *)(buf+offset)=o->type;
	offset+=2;
	offset+=encode_urustring(buf+offset,o->name,strlen((char *)o->name),0x01);
	*(U32 *)(buf+offset)=o->unk2;
	offset+=4;
	*(U32 *)(buf+offset)=o->node_id;
	offset+=4;

	return offset;
}


int create_sub_msg(Byte * buf,st_sub_msg * s) {

	int offset=0;

	*(U16 *)(buf+offset)=s->type;
	offset+=2;
	*(Byte *)(buf+offset)=s->unk0;
	offset++;
	*(U32 *)(buf+offset)=s->unk1;
	offset+=4;
	*(U32 *)(buf+offset)=s->unk2;
	offset+=4;
	*(U32 *)(buf+offset)=s->unk3;
	offset+=4;
	*(U16 *)(buf+offset)=s->k_type;
	offset+=2;
	offset+=encode_urustring(buf+offset,s->ks_type,strlen((char *)s->ks_type),0x01);
	*(U32 *)(buf+offset)=s->unk4;
	offset+=4;
	*(U32 *)(buf+offset)=s->unk5;
	offset+=4;
	*(U32 *)(buf+offset)=s->unk6;
	offset+=4;
	*(Byte *)(buf+offset)=s->unk7;
	offset++;

	//the object
	offset+=put_uru_object_desc(buf+offset,&(s->object));

	*(U32 *)(buf+offset)=s->unk8;
	offset+=4;
	*(U32 *)(buf+offset)=s->unk9;
	offset+=4;
	*(U16 *)(buf+offset)=s->k_type_val;
	offset+=2;
	offset+=encode_urustring(buf+offset,s->ks_type_val,strlen((char *)s->ks_type_val),0x01);
	*(U32 *)(buf+offset)=s->node_id;
	offset+=4;

	*(U32 *)(buf+offset)=s->unk10;
	offset+=4;
	*(Byte *)(buf+offset)=s->unk11;
	offset++;

	*(U16 *)(buf+offset)=s->unk12;
	offset+=2;
	*(U32 *)(buf+offset)=s->unk13;
	offset+=4;

	return offset;
}


int create_adv_msg(Byte * buf,st_uru_adv_msg * m,st_uru_client * u) {
	int offset=0;

	//m->cmd=*(U16 *)(buf+offset);
	//offset+=2;
	//m->format=*(U32 *)(buf+offset);
	//offset+=4;

	//tmp dissabled
	//*(U32 *)(buf+offset)=m->ki;
	//offset+=4;
	*(U32 *)(buf+offset)=m->unk1;
	offset+=4;
	*(Byte *)(buf+offset)=m->unk2;
	offset++;

	//the sub message
	m->s_msg_size=create_sub_msg(buf+offset+4,&(m->msg));

	*(U32 *)(buf+offset)=m->s_msg_size;
	offset+=4;

	offset+=m->s_msg_size;
	//end sub message with size

	*(Byte *)(buf+offset)=m->unk3;
	offset++;

	//the object
	offset+=put_uru_object_desc(buf+offset,&(m->object));

	*(Byte *)(buf+offset)=m->unk4;
	offset++;

	*(Byte *)(buf+offset)=m->unk5;
	offset++;

	*(Byte *)(buf+offset)=m->unk6; //(but we store it)
	//if(u->minor_version>=0x06) { //only in live, todni & tpots
		offset++;
	//}

	return offset;
}

int send_response(int sock,st_uru_adv_msg * m,st_uru_client * u) {

	Byte buf[OUT_BUFFER_SIZE]; //the out buffer
	int size=0,i;
	int sized;

	size=create_adv_msg(buf,m,u);
	sized=size-1; //for old clients (last byte not present)

	//send it to the sender
	if(u->minor_version>=0x06) {
		if(!strcmp((char *)global_age.name,"AvatarCustomization")) {
			plNetMsgLoadClone(sock,u,buf,size);
			printf("%s - \n",global_age.name);
			//abort();
		}
	} else {
		//plNetMsgLoadClone(sock,u,buf,sized);
	}

	//broadcast to all players
	if(global_broadcast==1) {
		for(i=global_the_bar+1;i<=global_client_count;i++) {
			//if(all_players[i].flag==0) { break; }
			if(all_players[i].paged==1 && all_players[i].ki!=u->ki) {
				//ki fix
				all_players[i].adv_msg.ki=u->ki;
				if(all_players[i].minor_version>=0x06) {
					plNetMsgLoadClone(sock,&all_players[i],buf,size); //12.06
				} else {
					plNetMsgLoadClone(sock,&all_players[i],buf,sized); //old versions
				}
			}
		}
	}

	return 0;
}


int send_load_player_list(int sock,st_uru_client * u) {

	//load all other players
	if(global_broadcast==1) {

		Byte buf[OUT_BUFFER_SIZE]; //the out buffer
		int i,size,sized;


		for(i=global_the_bar+1; i<=global_client_count; i++) {
			if(all_players[i].paged==1 && all_players[i].ki!=u->ki) {
			//if(all_players[i].ki!=u->ki) {
				//ki fix
				all_players[i].adv_msg.ki=u->ki;
				size=create_adv_msg(buf,&all_players[i].adv_msg,u);
				sized=size-1;
				if(u->minor_version>=0x06) {
					plNetMsgLoadClone(sock,u,buf,size);
				} else {
					plNetMsgLoadClone(sock,u,buf,sized);
				}
			}
		}
	}

	return 0;

}

#endif
