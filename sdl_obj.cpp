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

#define __U_SDL_OBJ_ID "$Id: sdlparser.cpp,v 1.6 2004/11/20 03:37:32 almlys Exp $"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>

#include "data_types.h" //for U32,Byte and others
#include "conv_funs.h"
#include "stdebug.h"

#include "sdl_obj.h"

#include "debug.h"


//Parse URU Objects here (size is the remaining buffer size)
int storeUruObjectDesc(Byte * buf,st_UruObjectDesc * o,int size) {
	int offset=0;

	o->flag=*(Byte *)(buf+offset);
	offset++;
	if(o->flag!=0x00 && o->flag!=0x01) { //0x01 client id, 0x00 no client id
					//print2log(f_une,"UNE: invalid object flag %02X\n",o->flag);
					//dump_packet(f_une,buf,size,0,7);
					return -1;
	}
	o->page_id=*(U32 *)(buf+offset);
	offset+=4;
	o->page_type=*(U16 *)(buf+offset);
	offset+=2;
	o->type=*(U16 *)(buf+offset);
	offset+=2;
	offset+=decode_urustring(o->name,buf+offset,STR_MAX_SIZE);
	offset+=2;
	if(o->flag & 0x01) {
		o->cflags=*(U32 *)(buf+offset);
		offset+=4;
		o->id=*(U32 *)(buf+offset);
		offset+=4;
	} else {
		o->cflags=0;
		o->id=0;
	}
	return offset;
}

//Streams the object, returns the size of the streamed object
int streamUruObjectDesc(Byte * buf,st_UruObjectDesc * o) {
	int offset=0;
	*(Byte *)(buf+offset)=o->flag;
	offset++;
	*(U32 *)(buf+offset)=o->page_id;
	offset+=4;
	*(U16 *)(buf+offset)=o->page_type;
	offset+=2;
	*(U16 *)(buf+offset)=o->type;
	offset+=2;
	offset+=encode_urustring(buf+offset,o->name,strlen((char *)o->name),0x01);
	if(o->flag & 0x01) {
		*(U32 *)(buf+offset)=o->cflags;
		offset+=4;
		*(U32 *)(buf+offset)=o->id;
		offset+=4;
	}
	return offset;
}

//!Dumps to the object to the logs
void dumpUruObjectDesc(st_log * f,st_UruObjectDesc * o) {
	print2log(f,"UruObjectDesc:[flag:%i,pid:%08X,t:%04X,ot:%04X,n:%s,(cflags:%i,id:%i)]",\
	o->flag,o->page_id,o->page_type,o->type,o->name,o->cflags,o->id);
}

int compareUruObjectDesc(st_UruObjectDesc * a,st_UruObjectDesc * b) {
	if(a->flag!=b->flag || a->page_id!=b->page_id || a->page_type!=b->page_type ||
		strcmp((char *)a->name,(char *)b->name) || a->cflags!=b->cflags || a->id!=b->id) {
		return -1;
	}
	return 0;
}


