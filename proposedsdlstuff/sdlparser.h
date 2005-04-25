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

#include "sdl_desc.h"
#include "sdl_obj.h"

#define SSTR 0x0FFF

typedef struct {
	int index; //!index in the associated t_sdl_def
	Byte type; //!type of tuple, 0x02 will make the string2 appear
	
	//if (type==0x02)
	  //Byte static2; //0x00
	    Byte string2[SSTR+1]; /** some information
	                              (for example, if the relto door is opened
	                              from the outside, this string contains "fromOutside") */
	//endif (type==0x02)

	Byte flags; /** 0x04 will make the timestamp (with microseconds) appear,
	                0x08 will make the data stuff disappear and the default value will be used */
	//if (flags and 0x04)
		U32 timestamp;
		U32 microseconds;
	//endif (flags and 0x04)
	//if !(flags and 0x08)
		int array_count; //!count of the entries in an array
		int data_size; //!size of the whole data
		void *data; //!the data
	//endif !(flags and 0x08)
} t_sdl_bin_tuple;

typedef struct {
	U16 u16k1; //0x01: server should save the value, 0x00: server shouldn't save the value
	//Byte static1; //seen always 0x06
	Byte n_values; //Number of stored values (or index?)
	t_sdl_bin_tuple * values; //Each stored value
	Byte n_structs; //Number of stored structs
	t_sdl_bin_tuple * structs; //Each stored struct
} t_sdl_binary;

//for sdl binary thingyes
typedef struct {
	Byte object_present; //0x00 no UruObjectDesc, 0x01 UruObjectDesc
	//Byte sdl_magic 0x80
	//----
	//the following part is compressed sometimes
	Byte name[SSTR+1]; //The sdl name (IUSTR)
	U16 version; //The sdl version
	//if object_present==0x01
		st_UruObjectDesc o; //The associated object
	//endif
	t_sdl_binary bin; //The sdl binary data
} t_sdl_head;

/**	\brief Frees the contents of a t_sdl_binary structure.
	\param binsdl the struture that should be freed
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param sdl_id number of the SDL definition of the structure 
*/	
int sdl_free_t_sdl_binary(t_sdl_binary * binsdl, t_sdl_def * sdl, int n_sdl, int sdl_id);

/**	\brief Parses a t_sdl_binary structure.
	\param log the output will be send to log
	\param buf pointer to the buffer which contains the unparsed structure
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param out pointer to the t_sdl_binary structure in which the result will be stored
*/
int sdl_parse_t_sdl_binary(st_log * log, Byte * buf,t_sdl_def * sdl,int n_sdl,int sdl_id,t_sdl_binary * out);

/**	\brief Streams a t_sdl_sdl_binary structure. Saves the result in a buffer.
	\param bin pointer to the t_sdl_binary structure that should be streamed
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param buf pointer to the buffer in which the result will be saved
*/
int sdl_stream_t_sdl_binary(t_sdl_binary * bin,t_sdl_def * sdl,int n_sdl,int sdl_id,Byte * buf);

/**	\brief creates a plain t_sdl_binary structure by using an SDL definition as a template
	\param bin the t_sdl_binary in which the data will be stored
	\param sdl Pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param sdl_id number of the SDL definition that should be used as a template
*/
int sdl_fill_t_sdl_binary_by_sdl_id(t_sdl_binary * bin,t_sdl_def * sdl,int n_sdl,int sdl_id);

/**	\brief updates a t_sdl_binary structure with the entries of another struct which contains some new tuples
	\param bin1 the structure that will be updated
	\param bin2 the strucutre which contains some new tuples
	\param how specifies how the structure should be updated, 0x01 updates the structure normally,
	           0x02 updates it and prepares it for a SDLState message by removing the string
*/
int sdl_update_t_sdl_binary(t_sdl_binary * bin1,t_sdl_binary * bin2,int how);

/**	\brief If necessary decompresses and parses a t_sdl_head structure.
	\param buf pointer to the buffer in which contains the unparsed structure
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param sdl_head pointer to the t_sdl_head structure in which the result will be stored
*/
int sdl_decompress_and_parse_t_sdl_head(Byte *buf,t_sdl_def * sdl,int n_sdl,t_sdl_head *out);

/**	\brief Streams and - if neccessary - compresses a t_sdl_head structure. Saves the result in a buffer.
	\param sdl_head pointer to the t_sdl_head structure that should be streamed and compressed
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param buf pointer to the buffer in which the result will be saved
*/
int sdl_stream_and_compress_t_sdl_head(t_sdl_head *sdl_head,t_sdl_def * sdl,int n_sdl,Byte *buf);









#endif
