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

#ifndef __U_SDL_PARSER_H_
#define __U_SDL_PARSER_H_
#define __U_SDL_PARSER_H_ID "$Id$"

//Utility to uncompress/decompress uru files (works!)

#define SSTR 100

//Ok a SDL struct

typedef struct {
	Byte flag; //0x00 normal variable, 0x01 embeded struct
	Byte static1; //0x03 (always);
	Byte name[SSTR+1]; //variable name (IUSTR)
	Byte display_options[SSTR+1]; //a coma separated list of display options (USTR)
	U16 array_size; //the size of the array 0x00 [] (variable), elsewhere [x]
	U16 u16k1; //0x00 always
	Byte type; //The var type
	Byte default_value[SSTR+1]; //The default value
	U16 default_option; //The default Option (DEFAULTOPTION=VAULT)
	U16 u16k2; //0x00 always
	//if flag==0x00
		U16 n_itms;
		Byte itm_type;
	//if flag==0x01
		Byte struct_name[SSTR+1]; //The struct name
		U16 struct_version; //The struct version
} t_sdl_var;

typedef struct {
	Byte flag; //0x01 (always)
	Byte name[SSTR+1]; //SDL name
	U16 version; //sdl version
	U16 n_vars; //num of stored sdl variables
	t_sdl_var * vars; //the struct where variables are stored
} t_sdl_def;


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

void init_sdl_def(t_sdl_def * sdl,int n);
void destroy_sdl_def(t_sdl_def * sdl,int n);
char * get_var_type(Byte type);
char * get_var_type_nspc(Byte type);
int get_var_type_from_name(char * buf);
int sdl_parse_default_options(FILE * f,char * buf,t_sdl_var * var);
int sdl_compile(FILE * f,Byte * buf,int totalsize,t_sdl_def * sdl);
int update_sdl_version_structs(FILE * f,t_sdl_def * sdl,int n_sdl);
int sdl_statedesc_reader(FILE * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl);
int sdl_statedesc_streamer(FILE * f,Byte ** buf2,t_sdl_def * sdl,int n_sdl);
//to parse sdl files
int sdl_parse_contents(FILE * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl);
//Returns -1, if not found, elsewhere the index in the sdl struct where is it
// requires the struct, and the number of elements
int find_sdl_descriptor(Byte * name,U16 version,t_sdl_def * sdl,int n);
int sdl_parse_sub_data(FILE * f,Byte * buf,int totalsize,t_sdl_def * sdl,int sdl_id);
//to parse sdl files
int sdl_parse_binary_data(FILE * f,Byte * buf,int totalsize,t_sdl_def * sdl,int n_sdl);
int read_sdl_files(char * address,t_sdl_def ** sdl,int * n_sdl);

#endif
