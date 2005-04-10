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

/*
	This file is subject to dissapear/change in the future.

	Currently the purpose is to generate the correct guid for an age.
	New customized ages should be added here.
	Please see servers.list for more info

*/

#ifndef _GUID_GENERATOR_
#define _GUID_GENERATOR_
#define _GUID_GENERATOR_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <string.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include "data_types.h"
#include "conv_funs.h"
//#include "config_parser.h"
//#include "tmp_config.h"
//#include "urunet.h"
//#include "uru.h"

#include "stdebug.h"
#include "ageparser.h"
//#include "vaultsubsys.h"

#include "guid_gen.h"

#include "debug.h"

//This vars are created or defined in the tracking/game/vault subsystems
extern char * global_private_ages;
extern int global_muinstance_mode;
extern t_age_def * global_age_def; //age struct
extern int global_age_def_n; //total number of ages

int find_age_sequence_number(Byte * age_name) {

	int i,ret;

	ret=0;

	for(i=0; i<global_age_def_n; i++) {
		if(!strcmp((char *)global_age_def[i].name,(char *)age_name)) {
			ret=global_age_def[i].SequencePrefix;
			break;
		}
	}

	if(ret<0) { ret=0; } //disallow someone to attempt a link to a invalid age like GUI, GlobalAvatars, GlobalAnimations, CustomAvatars, GlobalMarkers, and so on...

	return ret;
}

int check_if_age_is_private(Byte * age_name) {

	char bufeta[1024];
	strcpy(bufeta,(const char *)global_private_ages);

	char *buf=bufeta;
	char *p=NULL;

	p=strsep(&buf,",");

	while(p!=NULL) {
		DBG(5,"p:%s,an:%s,b:%s\n",p,age_name,buf);
		if(p!=NULL && !strcmp(p,(const char *)age_name)) {
			return 1;
		}
		p=strsep(&buf,",");
	}
	//abort();
	return 0;

#if 0
	int i,e;
	Byte buf[200];
	int i=0;
	int ret=0;
	for(e=0; e<(int)strlen((char *)global_private_ages); e++) {
		if(global_private_ages[i]!=',') {
			buf[i]=global_private_ages[e];
			i++;
		} else {
			i++;
			buf[i]='\0';
			i=0;
			if(!strcmp((char *)buf,(char *)age_name)) {
				ret=1;
				break;
			}
		}
	}
	if(ret==0) {
		i++;
		buf[i]='\0';
		i=0;
		if(!strcmp((char *)buf,(char *)age_name)) {
			ret=1;
		}
	}*/

	return ret;
#endif
}

/* Note this code will cause that for example:
   private_ages = "Neighborhood02"
   age to check = "Neighborhood"
	 The function will return always 1, and we don't want this to happen.
=======
int check_if_age_is_private(Byte * age_name)
{
//	int i=0,e;
//	Byte buf[200];
//	int ret=0;

	DBG(5,"check_age_private %s\n", age_name);
  if (strstr((const char *)global_private_ages, (const char *)age_name))
	  return(1);
  else
	  return(0);

//	for(e=0; e<(int)strlen((char *)global_private_ages); e++) {
//		if(global_private_ages[i]!=',') {
//			buf[i]=global_private_ages[e];
//			i++;
//		} else {
//			i++;
//			buf[i]='\0';
//			i=0;
//			if(!strcmp((char *)buf,(char *)age_name)) {
//				ret=1;
//				break;
//			}
//		}
//	}
//	i++;
//	buf[i]='\0';
//	i=0;
//	if(!strcmp((char *)buf,(char *)age_name)) {
//		ret=1;
//	}
//
//	return ret;
>>>>>>> 1.8
}
*/


void generate_newguid(Byte * guid2,Byte * age_name,U32 ki) {
	Byte guid[8];
	memset(guid,0,8);
	int i=6;

	/*
	//so we have " The server GUID, aka age guid"
	---------------------------------
	| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
	--------------------------------
	| 0 | ki here       | 0 | s | 0 |
	--------------------------------

	Where s, is the sequence prefix.
	This is only a preliminar usage of the age guid. Using the player id, as part of the age,
	we will be completely sure, that all players, at least will have only one instance for his
	 own age.
	The 5 byte is reserved for a random number for the hoods, and any other age (for the future)
	And the 1st bit of the 4 byte, should be always 0 (since the Ki number is a signed value, this
	Will happen always.
	(7th byte is reserver for a possible expansion of the current sequence prefix, perhaps, some
	day in the future we will have more than 256 ages, but I'm pretty sure that this code will
	be completely different when this happens)

	*/

	if(global_muinstance_mode==1) { //multiple instance mode
		guid[i]=(Byte)find_age_sequence_number(age_name); //current limitation, is that only 256 ages are allowed

		if(check_if_age_is_private(age_name)) {
			*(U32 *)(guid+1)=ki; //nice eh!
		}
	} else if(global_muinstance_mode==0) { //single instance mode, new standard
		guid[i]=(Byte)find_age_sequence_number(age_name); //current limitation, is that only 256 ages are allowed
	} else { //single instance mode, old standard #2

		if(!strcmp((char *)age_name,"AvatarCustomization")) {
			guid[i]=0x01;
		} else if(!strcmp((char *)age_name,"Cleft")) {
			guid[i]=0x02;
		} else if(!strcmp((char *)age_name,"Personal")) {
			guid[i]=0x03;
		} else if(!strcmp((char *)age_name,"Nexus")) {
			guid[i]=0x04;
		} else if(!strcmp((char *)age_name,"city")) {
			guid[i]=0x05;
		} else if(!strcmp((char *)age_name,"Neighborhood")) {
			guid[i]=0x06;
		} else if(!strcmp((char *)age_name,"Teledahn")) {
			guid[i]=0x07;
		} else if(!strcmp((char *)age_name,"Garrison")) {
			guid[i]=0x08;
		} else if(!strcmp((char *)age_name,"Gira")) {
			guid[i]=0x09;
		} else if(!strcmp((char *)age_name,"Kadish")) {
			guid[i]=0x0A;
		} else if(!strcmp((char *)age_name,"BaronCityOffice")) {
			guid[i]=0x0B;
		} else if(!strcmp((char *)age_name,"BahroCave")) {
			guid[i]=0x0C;
		} else if(!strcmp((char *)age_name,"spyroom")) {
			guid[i]=0x0D;
		} else if(!strcmp((char *)age_name,"Personal02")) {
			guid[i]=0x0E;
		} else if(!strcmp((char *)age_name,"Neighborhood02")) {
			guid[i]=0x0F;
		} else if(!strcmp((char *)age_name,"Descent")) {
			guid[i]=0x10;
		} else if(!strcmp((char *)age_name,"Garden")) {
			guid[i]=0x11;
		} else if(!strcmp((char *)age_name,"GreatZero")) {
			guid[i]=0x12;
		} else if(!strcmp((char *)age_name,"RestorationGuild")) {
			guid[i]=0x13;
		} else if(!strcmp((char *)age_name,"Myst")) {
			guid[i]=0x14;
		} else if(!strcmp((char *)age_name,"Kveer")) {
			guid[i]=0x15;
		} else if(!strcmp((char *)age_name,"ErcanaCitySilo")) {
			guid[i]=0x16;
		} else if(!strcmp((char *)age_name,"Ercana")) {
			guid[i]=0x17;
		} else if(!strcmp((char *)age_name,"DniCityX2Finale")) {
			guid[i]=0x18;
		} else if(!strcmp((char *)age_name,"BahroCave02")) {
			guid[i]=0x19;
		} else if(!strcmp((char *)age_name,"Ahnonay")) {
			guid[i]=0x20;
		} else if(!strcmp((char *)age_name,"AhnySphere01")) {
			guid[i]=0x21;
		} else if(!strcmp((char *)age_name,"AhnySphere02")) {
			guid[i]=0x22;
		} else if(!strcmp((char *)age_name,"AhnySphere03")) {
			guid[i]=0x23;
		} else if(!strcmp((char *)age_name,"AhnySphere04")) {
			guid[i]=0x24;
		} else {
			guid[i]=0x00;
		}
	}

	hex2ascii2(guid2,guid,8);
}


#endif


