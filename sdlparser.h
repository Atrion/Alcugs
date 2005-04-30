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

#ifndef __U_SDL_PARSER_H_
#define __U_SDL_PARSER_H_
#define __U_SDL_PARSER_H_ID "$Id$"

//Utility to uncompress/decompress uru files (works!)

#define SSTR 100

typedef struct {
	U16 u16k1; //seen always 0x00, but sometimes seen 0x0001
	Byte static1; //seen always 0x06
	Byte n_vals; //Number of stored values (or index?)
	//TODOt_sdl_bin_var * vars; //Each stored value
	Byte n_strs; //Number of stored structs
	//TODOt_sdl_bin_var * vars; //Each stored struct

} t_sdl_binary;

//for sdl binary thingyes
typedef struct {
	Byte object; //0x00 no UruObjectDesc, 0x01 UruObjectDesc
	//Byte sdl_magic 0x080
	//----
	Byte name[SSTR+1]; //The sdl name (IUSTR)
	U16 version; //The sdl version
	st_UruObjectDesc o; //The associated object (only if object==0x01)
	t_sdl_binary bin; //The sdl binary data
} t_sdl_head;


int sdl_parse_sub_data(FILE * f,Byte * buf,int totalsize,t_sdl_def * sdl,int sdl_id);
//to parse sdl files
int sdl_parse_binary_data(FILE * f,Byte * buf,int totalsize,t_sdl_def * sdl,int n_sdl);


#endif
