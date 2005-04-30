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

#ifndef __U_SDL_DESC_H_
#define __U_SDL_DESC_H_
#define __U_SDL_DESC_H_ID "$Id$"

#include "sdlstrs.h"

//init & destroy
void init_sdl_def(t_sdl_def * sdl,int n);
void destroy_sdl_def(t_sdl_def * sdl,int n);

char * sdl_get_var_type(Byte type);
char * sdl_get_var_type_nspc(Byte type);
int sdl_get_var_type_from_name(char * buf);

//internal only
int sdl_parse_default_options(st_log * f,char * buf,t_sdl_var * var);
int sdl_compile(st_log * f,Byte * buf,int totalsize,t_sdl_def * sdl);
int update_sdl_version_structs(st_log * f,t_sdl_def * sdl,int n_sdl);
//--

//You must use these ones, the reader and the streamer
//Reads states from plain to struct.
int sdl_statedesc_reader(st_log * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl);
int sdl_statedesc_reader_fast(st_log * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl);
//Streams states from struct to buffer.
int sdl_statedesc_streamer(st_log * f,Byte ** buf2,t_sdl_def * sdl,int n_sdl);
//Puts streams from buffer to struct.
int sdl_parse_contents(st_log * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl);

//Returns -1, if not found, elsewhere the index in the sdl struct where is it
// requires the struct, and the number of elements
int find_sdl_descriptor(Byte * name,U16 version,t_sdl_def * sdl,int n);
int find_sdl_descriptor_by_name(Byte * name,t_sdl_def * sdl,int n);

//Creates a new list, but without dupped descriptors, only the latests versions
int sdl_get_last_descriptors(st_log * f,t_sdl_def ** sdlo,int * n_sdlo,t_sdl_def * sdl,int n_sdl);

//Reads all descriptors from the specified sdl folder
int read_sdl_files(st_log * f,char * address,t_sdl_def ** sdl,int * n_sdl);

#endif

