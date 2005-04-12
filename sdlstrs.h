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

/**
	Basic data types
*/

#ifndef __U_SDLSTRS_H_
#define __U_SDLSTRS_H_
#define __U_SDLSTRS_H_ID $Id$

#include "data_types.h"
#include "urustructs.h"

#define SSTR 200

//*.sav file format
typedef struct {
	U32 cdataflag; //0x01 contains data, 0x00 void file
	U16 maxversion; //0x01 // 0x07
	U16 minversion; //0x01 // 0x02
	Byte flags; //0x1F
	//if flags & 0x1F
		Byte name[SSTR+1]; //USTR? age filename
		Byte whoami; //peer type, always kGame 0x03.
		Byte ip[SSTR+1]; //USTR? ip address
		U16 port;
		Byte guid[8]; //peer guid
	//--
	U32 realsize; //uncompressed size
	U32 compsize; //compressed size
	//Byte * buf; //data buffer
} t_sdl_sav_header;

//SDL Header
typedef struct {
	Byte flag; //0x00 normal variable, 0x01 embeded struct
	Byte static1; //0x03 (always);
	Byte name[SSTR+1]; //variable name (IUSTR)
	Byte display_options[SSTR+1]; //a coma separated list of display options (USTR)
	U16 array_size; //the size of the array 0x00 [] (variable), elsewhere [x]
	U16 u16k1; //0x00 always
	Byte type; //The var type
	Byte default_value[SSTR+1]; //The default value (IUSTR)
	U16 default_option; //The default Option (DEFAULTOPTION=VAULT)
	U16 u16k2; //0x00 always
	//if flag==0x00
		U16 n_itms;
		Byte itm_type;
	//if flag==0x01
		Byte struct_name[SSTR+1]; //The struct name (IUSTR)
		U16 struct_version; //The struct version
} t_sdl_var;

typedef struct {
	Byte flag; //0x01 (always)
	Byte name[SSTR+1]; //SDL name (IUSTR)
	U16 version; //sdl version
	U16 n_vars; //num of stored sdl variables
	t_sdl_var * vars; //the struct where variables are stored
} t_sdl_def; //This is ONE SDL Descriptor, (we have a tuple in memory with all of them)
//End SDL header


//SDL Streamming byte code



#endif

