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

#ifndef _URU_STRUCTS
#define _URU_STRUCTS
#define _URU_STRUCTS_ID "$Id: vaultstrs.h,v 1.1 2004/11/05 02:21:18 almlys Exp $"


typedef struct {
	Byte mask; //The mask
	Byte filename[STR_MAX_SIZE+1]; //age filename
	Byte instance_name[STR_MAX_SIZE+1]; //age instance name
	Byte guid[8]; //Age Guid
	Byte user_name[STR_MAX_SIZE+1]; //user defined name
	Byte display_name[STR_MAX_SIZE+1]; //The display name
	U32 language; //Age Language, seen always 0
} t_AgeInfoStruct;

typedef struct {
	U32 mask; //A mask, always 0x00000007
	Byte title[STR_MAX_SIZE+1]; //Title of the link (Default)
	Byte name[STR_MAX_SIZE+1]; //Name of the link (LinkingPointDefault)
	Byte camera[STR_MAX_SIZE+1]; //Camera stack of the link (Null)
} t_SpawnPoint;

typedef struct {
	U16 mask; //The mask
	t_AgeInfoStruct ainfo; //age information
	Byte unk; //Linking rules
	U32 rules; //An unknown U32 that is always 0x01 (code needs to be fixed!)
	t_SpawnPoint spoint; //the spawn point
	Byte ccr; //It's a CCR?
} t_AgeLinkStruct; //0x02BF

//SOME SDL vars
//PLKEY (UruObject)
//!UruObjectDesc
typedef struct {
	Byte flag; //Seen always (0x00 and 0x01, there is also a 0x02?, no examples or proof found)
	U32 page_id; //page id
	U16 page_type; //type of page
	//if flag & 0x02
	//Byte unk1; ??
	//--
	U16 type; //the type of object
	Byte name[STR_MAX_SIZE+1]; //IUSTR the object name (inverted ustring)
	//if flag & 0x01
		U32 cflags; //the Unique flag (seen, 0x01, 0x02, depending of the LoadClone message)
		U32 id; //the client id (a MGR player node)
	//--
} st_UruObjectDesc;



#endif
