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

#ifndef __U_SDL_PARSER_
#define __U_SDL_PARSER_
#define __U_SDL_PARSER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
//#include "debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <math.h>
#include <zlib.h>
//#include <ctype.h>
//#include <dirent.h>
//#include <errno.h>
//#include <sys/types.h>

#ifdef __WIN32__
#  include "windoze.h"
#endif

#include "data_types.h" //for U32,Byte and others
#include "conv_funs.h"
#include "stdebug.h"

#include "urustructs.h"
#include "sdl_desc.h"
#include "sdl_obj.h"

#include "sdlparser.h"

/**	\brief Frees the contents of a t_sdl_binary structure.
	\param binsdl the struture that should be freed
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param sdl_id number of the SDL definition of the structure 
*/	
int sdl_free_t_sdl_binary(t_sdl_binary * binsdl, t_sdl_def * sdl, int n_sdl, int sdl_id)
{
	int i;
	for(i=0; i<binsdl->n_values; i++)
	{
		if(binsdl->values[i].data)
			free(binsdl->values[i].data);
	}
	if(binsdl->values)
		free(binsdl->values);



	for(i=0; i<binsdl->n_structs; i++)
	{
		int index = binsdl->structs[i].index;

		int count=sdl[sdl_id].vars[index].array_size;
		if(count == 0)
			count = binsdl->structs[i].array_count;

		t_sdl_binary *sub_binsdl = (t_sdl_binary *)binsdl->structs[i].data;
		for(int k=0; k<count; k++)
		{
			int sub_sdl_id=find_sdl_descriptor(sdl[sdl_id].vars[index].struct_name,sdl[sdl_id].vars[index].struct_version,sdl,n_sdl);
			if(sub_sdl_id<0)
			{
				//not found :(
				//argh - now there's a memory leak
				break;
			}

			sdl_free_t_sdl_binary(&sub_binsdl[k],sdl,n_sdl,sub_sdl_id);
		}

		free(binsdl->structs[i].data);
	}
	if(binsdl->structs)
		free(binsdl->structs);

	
	return 0;
}






/**	\brief Converts some SDL tuple indices between the SDL binary style and the SDL definition style.
	\param type specifies how the index will be converted,
	            0x00: convert binary value number to sdl definition index,
	            0x01: convert binary struct number to sdl definition index,
	            0x02: convert sdl definition index to binary value number,
	            0x03: convert sdl definition index to binary struct number
	\param number the index which will be converted
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param sdl_id number of the SDL definition where the tuple index will be converted
	\return -1 on error, otherwise the result of the conversion
*/
__inline int sdl_get_item_number(int type, int number, t_sdl_def * sdl, int n_sdl, int sdl_id)
{
	int value_counter=-1;
	int struct_counter=-1;
	for(int i=0; i<sdl[sdl_id].n_vars; i++)
	{
		if(sdl[sdl_id].vars[i].type == 0x05)
			struct_counter++;
		else
			value_counter++;

		switch(type)
		{
			case 0: //translation value number => sdl defintion index
				if(value_counter == number)
					return i;
				break;

			case 1: //translation struct number => sdl defintion index
				if(struct_counter == number)
					return i;
				break;

			case 2: //translation sdl definition index => value number
				if(i == number)
				{
					if(sdl[sdl_id].vars[i].type != 0x05)
						return value_counter;
					else
						return -1; //the index that should be translated doesn't point to a value
				}
				break;

			case 3: //translation sdl definition index => struct number
				if(i == number)
				{
					if(sdl[sdl_id].vars[i].type == 0x05)
						return struct_counter;
					else
						return -1; //the index that should be translated doesn't point to a struct
				}
				break;
		}
	}
	return -1;
}


/**	\brief Parses a t_sdl_binary structure.
	\param log the output will be send to log
	\param buf pointer to the buffer which contains the unparsed structure
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param out pointer to the t_sdl_binary structure in which the result will be stored
*/
int sdl_parse_t_sdl_binary(st_log * log, Byte * buf,t_sdl_def * sdl,int n_sdl,int sdl_id,t_sdl_binary * out)
{
	plog(log,"Parsing a t_sdl_binary ");

	memset(out,0,sizeof(t_sdl_binary));

	int off=3;
	int i;
	int indices;


	out->u16k1 = *(U16 *)buf;
	
	if(out->u16k1!=0x00 && out->u16k1!=0x01)
	{
		plog(log," The u16k1 is %02X instead of being 0x00 or 0x01\n",out->u16k1);
		return -1;
	}

	if(*(Byte *)(buf+2) != 0x06)
	{
		plog(log," The static that should be 0x06 is in fact 0x%02X\n",*(Byte *)(buf+2));
		return -1;
	}

	plog(log,"with u16k1=%i.\n",out->u16k1);
	
	out->n_values = *(Byte *)(buf+off);
	off++;

	int value_counter=0;
	int struct_counter=0;
	for(i=0; i<sdl[sdl_id].n_vars; i++)
	{
		if(sdl[sdl_id].vars[i].type == 0x05)
			struct_counter++;
		else
			value_counter++;
	}

	if(out->n_values != value_counter)
	{
		indices = 1;
	}
	else
	{
		indices = 0;
	}

#if 0
	//strange... it's like this in the .sav-files:
	if(out->u16k1 != 0) //this means: if(indices != 0)
	{
		//index flags are present
		out->n_values = *(Byte *)(buf+off);
		off++;
	}
	else
	{
		int value_counter=0;
		int struct_counter=0;
		for(i=0; i<sdl[sdl_id].n_vars; i++)
		{
			if(sdl[sdl_id].vars[i].type == 0x05)
				struct_counter++;
			else
				value_counter++;
		}

		out->n_values = value_counter;
	}
#endif

	plog(log,"  This t_sdl_binary contains %i value(s).\n",out->n_values);

	out->values = (t_sdl_bin_tuple *)malloc(sizeof(t_sdl_bin_tuple)*out->n_values);
	memset(out->values,0,sizeof(t_sdl_bin_tuple)*out->n_values); //security

	int index_number = -1;

	int realindex;

	for(i=0; i<out->n_values; i++)
	{
		//for each value...
		t_sdl_bin_tuple* bin_var = &out->values[i];


		if(indices)
		{
			index_number = *(Byte *)(buf+off);
			off++;
		}
		else
		{
			index_number++;
		}
		realindex = sdl_get_item_number(0,index_number,sdl,n_sdl,sdl_id);
		
		bin_var->index = realindex;





		bin_var->type = *(Byte *)(buf+off);
		off++;
		if(bin_var->type == 0x02)
		{
			//bin_var->static2 = *(Byte *)(buf+off); //should be always 0
			off++;
			off+=decode_urustring(bin_var->string2,buf+off,SSTR);
			off+=2;
		}
		bin_var->flags = *(Byte *)(buf+off);
		off++;

		//Unknown behaviour, we don't know the explanation that causes this, but well, here goes
		//The.Modificator: i didn't find this case anywhere... but i didn't delete the code just to be sure
		if(bin_var->flags==0x00 && bin_var->type==0x00) {
			//then the next one are the real flags
			bin_var->flags=*(Byte *)(buf+off);
			off++;
		}
		plog(log,"    The tuple with the name \"%s\" ",sdl[sdl_id].vars[realindex].name);
		plog(log,"with the type 0x%02X and the flags 0x%02X ",bin_var->type,bin_var->flags);

		if(bin_var->flags & 0x04)
		{
			bin_var->timestamp = *(U32 *)(buf+off);
			off+=4;
			bin_var->microseconds = *(U32 *)(buf+off);
			off+=4;
		}
		else
		{
			bin_var->timestamp = 0;
			bin_var->microseconds = 0;
		}

		if(!(bin_var->flags & 0x08))
		{
			bin_var->array_count=sdl[sdl_id].vars[realindex].array_size;
			if(bin_var->array_count==0) {
				bin_var->array_count=*(U32 *)(buf+off);
				off+=4;
			}


			plog(log,"contains %i variable(s).\n",bin_var->array_count);


			//plog(log,"Value(s):");


			//allocate the memory and copy the data
			switch(sdl[sdl_id].vars[realindex].type)
			{
				case 0: //INT (4 bytes)
					bin_var->data_size = sizeof(int)*bin_var->array_count;
					bin_var->data=malloc(sizeof(int)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),sizeof(int)*bin_var->array_count);
					off+=sizeof(int)*bin_var->array_count;
					break;
				case 1: //FLOAT (4 bytes)
					bin_var->data_size = sizeof(float)*bin_var->array_count;
					bin_var->data=malloc(sizeof(float)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),sizeof(float)*bin_var->array_count);
					off+=sizeof(float)*bin_var->array_count;
					break;
				case 3: //STRING32 (32 bytes)
					bin_var->data_size = 32*sizeof(Byte)*bin_var->array_count;
					bin_var->data=malloc(32*sizeof(Byte)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),32*sizeof(Byte)*bin_var->array_count);
					off+=32*sizeof(Byte)*bin_var->array_count;
					break;
				case 5: //Sub SDL
					plog(log,"   Error: There a struct in the value list.\n");
					return -1;
					break;
				case 2: //BOOL (1 byte)
				case 9: //BYTE (1 byte)
					bin_var->data_size = sizeof(Byte)*bin_var->array_count;
					bin_var->data=malloc(sizeof(Byte)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),sizeof(Byte)*bin_var->array_count);
					off+=sizeof(Byte)*bin_var->array_count;
					break;
				case 0xA: //SHORT (2 bytes)
					bin_var->data_size = sizeof(short)*bin_var->array_count;
					bin_var->data=malloc(sizeof(short)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),sizeof(short)*bin_var->array_count);
					off+=sizeof(short)*bin_var->array_count;
					break;
				case 8: //TIME (4+4 bytes)
					bin_var->data_size = 2*sizeof(int)*bin_var->array_count;
					bin_var->data=malloc(2*sizeof(int)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),2*sizeof(int)*bin_var->array_count);
					off+=2*sizeof(int)*bin_var->array_count;
					break;
				case 50: //VECTOR3 (3 floats)
				case 51: //POINT3 (3 floats)
					bin_var->data_size = 3*sizeof(float)*bin_var->array_count;
					bin_var->data=malloc(3*sizeof(float)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),3*sizeof(float)*bin_var->array_count);
					off+=3*sizeof(float)*bin_var->array_count;
					break;
				case 54: //QUATERNION (4 floats)
					bin_var->data_size = 4*sizeof(float)*bin_var->array_count;
					bin_var->data=malloc(4*sizeof(float)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),4*sizeof(float)*bin_var->array_count);
					off+=4*sizeof(float)*bin_var->array_count;
					break;
				case 55: //RGB8 (3 bytes)
					bin_var->data_size = 3*sizeof(Byte)*bin_var->array_count;
					bin_var->data=malloc(3*sizeof(Byte)*bin_var->array_count);
					memcpy(bin_var->data,(void *)(buf+off),3*sizeof(Byte)*bin_var->array_count);
					off+=3*sizeof(Byte)*bin_var->array_count;
					break;
				case 4: //PLKEY (UruObject)
				{
					bin_var->data_size = sizeof(st_UruObjectDesc)*bin_var->array_count;
					bin_var->data=malloc(sizeof(st_UruObjectDesc)*bin_var->array_count);
					for(int k=0;k<bin_var->array_count;k++)
					{
						int ret=storeUruObjectDesc(buf+off,(st_UruObjectDesc *)((size_t)bin_var->data + sizeof(st_UruObjectDesc)*k),0xfff);
						if(ret<0)
						{
							plog(log,"    Error while parsing an UruObjectDesc.\n");
							return -1;
						}
						off+=ret;
					}
					break;
				}
				default:
					plog(log,"    Error, unexpected type 0x%02X.\n",sdl[sdl_id].vars[index_number].type);
					return -1;
			}


		}
		else
		{
			plog(log,"has its default value.\n");
		}	
	} //for each value



	out->n_structs = *(Byte *)(buf+off);
	off++;

	if(out->n_structs != struct_counter)
	{
		indices = 1;
	}
	else
	{
		indices = 0;
	}

	plog(log,"  This t_sdl_binary contains %i structure(s).\n",out->n_structs);

	out->structs = (t_sdl_bin_tuple *)malloc(sizeof(t_sdl_bin_tuple)*out->n_structs);
	memset(out->structs,0,sizeof(t_sdl_bin_tuple)*out->n_structs); //security


	index_number=-1;

	for(i=0; i<out->n_structs; i++)
	{
		//for each struct...
		t_sdl_bin_tuple* bin_var = &out->structs[i];

		if(indices)
		{
			index_number = *(Byte *)(buf+off);
			off++;
		}
		else
		{
			index_number++;
		}
		realindex = sdl_get_item_number(1,index_number,sdl,n_sdl,sdl_id);
		
		bin_var->index = realindex;

		bin_var->type = *(Byte *)(buf+off);
		off++;
		if(bin_var->type == 0x02)
		{
			//bin_var->static2 = *(Byte *)(buf+off); //should be always 0
			off++;
			off+=decode_urustring(bin_var->string2,buf+off,SSTR);
			off+=2;
		}
		bin_var->flags = *(Byte *)(buf+off);
		off++;

		//Unknown behaviour, we don't know the explanation that causes this, but well, here goes
		if(bin_var->flags==0x00 && bin_var->type==0x00) {
			//then the next one are the real flags
			bin_var->flags=*(Byte *)(buf+off);
			off++;
		}

		plog(log,"    The tuple with the name \"%s\" ",sdl[sdl_id].vars[realindex].name);
		plog(log,"with the flags 0x%02X ",bin_var->flags);

		if(bin_var->flags & 0x04)
		{
			bin_var->timestamp = *(U32 *)(buf+off);
			off+=4;
			bin_var->microseconds = *(U32 *)(buf+off);
			off+=4;
		}
		if(!(bin_var->flags & 0x08))
		{
			bin_var->array_count=sdl[sdl_id].vars[realindex].array_size;
			if(bin_var->array_count==0) {
				bin_var->array_count=*(U32 *)(buf+off);
				off+=4;
			}
			off++; //extra byte for structures

			plog(log,"contains %i structure(s).\n",bin_var->array_count);
			
			//plog(log,"Value:");


			
			//allocate the memory and copy the data
			switch(sdl[sdl_id].vars[realindex].type)
			{
				case 5: //Sub SDL
				{
					int sub_sdl_id=find_sdl_descriptor(sdl[sdl_id].vars[realindex].struct_name,sdl[sdl_id].vars[realindex].struct_version,sdl,n_sdl);
					if(sub_sdl_id<0)
					{
						//not found :(
						plog(log,"    Error while parsing a sub-struct. Didn't find the correct SDL definition.\n");
						return -1;
					}
					bin_var->data_size = bin_var->array_count*sizeof(t_sdl_binary);
					bin_var->data = malloc(bin_var->array_count*sizeof(t_sdl_binary));
			
					plog(log,"\n    [sub-SDL list]\n");

					for(int k=0; k<bin_var->array_count; k++)
					{
						plog(log,"\n    -SDL:%s Version:%i\n\n",sdl[sub_sdl_id].name,sdl[sub_sdl_id].version);

						int ret=sdl_parse_t_sdl_binary(log,buf+off,sdl,n_sdl,sub_sdl_id,
						                               (t_sdl_binary *)((size_t)bin_var->data + k*sizeof(t_sdl_binary)) );
						
						if(ret < 0)
						{
							plog(log,"    Error while parsing a sub-struct.\n");
							return -1; //error
						}
						off+=ret;
					}

					plog(log,"\n    [/sub-SDL list]\n");
				} break;
				default:
					plog(log,"    Error, unexpected type 0x%02X in the structure list. (It should just contain tuples of the 0x05 type.)\n",sdl[sdl_id].vars[realindex].type);
					return -1;
			}
		}
		else
		{
			plog(log,"has its default... value... well, a struct with a value?\n");
		}

	}//for each structure

	plog(log,"...finished parsing the t_sdl_binary!\n");
	return off;
}



#if 0
//untested yet, don't trust this code
int sdl_copy_t_sdl_binary(t_sdl_binary * out,t_sdl_binary * tocopy)
{
	out->u16k1 = tocopy->u16k1;
	out->static1 = tocopy->static1;
	out->n_values = tocopy->n_values;
	out->n_structs = tocopy->n_structs;

	out->values = (t_sdl_bin_var *)malloc(sizeof(t_sdl_bin_var)*out->n_values);
	memcpy(out->values,tocopy->values,sizeof(t_sdl_bin_var)*out->n_values);

	for(int i=0; i<out->n_values; i++)
	{
		out->values[i].data = malloc(out->values[i].data_size);
		memcpy(out->values[i].data,tocopy->values[i].data,out->values[i].data_size);
	}

	for(i=0; i<out->n_structs; i++)
	{
		out->structs[i].data = malloc(out->structs[i].data_size);

		for(int j=0; j<out->structs[i].array_count; j++)
		{
			t_sdl_binary *zin = &((t_sdl_binary *)tocopy->structs[i].data)[j];
			t_sdl_binary *zout = &((t_sdl_binary *)out->structs[i].data)[j];
			sdl_copy_t_sdl_binary(zout,zin);
		}
	}

	return 0;
}
#endif




/**	\brief Streams a t_sdl_sdl_binary structure. Saves the result in a buffer.
	\param bin pointer to the t_sdl_binary structure that should be streamed
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param buf pointer to the buffer in which the result will be saved
*/
int sdl_stream_t_sdl_binary(t_sdl_binary * bin,t_sdl_def * sdl,int n_sdl,int sdl_id,Byte * buf)
{
	int off=0;
	int i;
	int indices;

	memcpy(buf,(void *)bin,2+1);
	off+=2+1; //u16k1, static1


	*(Byte *)(buf+off) = bin->n_values;
	off++;

	int value_counter=0;
	int struct_counter=0;
	for(i=0; i<sdl[sdl_id].n_vars; i++)
	{
		if(sdl[sdl_id].vars[i].type == 0x05)
			struct_counter++;
		else
			value_counter++;
	}

	if(bin->n_values != value_counter)
	{
		indices = 1;
	}
	else
	{
		indices = 0;
	}

#if 0
	//strange... it's like this in the .sav-files:
	if(out->u16k1 != 0) //(indices != 0)
	{
		//index flags are present
		out->n_values = *(Byte *)(buf+off);
		off++;
	}
	else
	{
		int value_counter=0;
		int struct_counter=0;
		for(i=0; i<sdl[sdl_id].n_vars; i++)
		{
			if(sdl[sdl_id].vars[i].type == 0x05)
				struct_counter++;
			else
				value_counter++;
		}

		out->n_values = value_counter;
	}
#endif

	int index_number = -1;

	for(i=0; i<bin->n_values; i++)
	{
		//for each value...
		t_sdl_bin_tuple* bin_var = &bin->values[i];


		if(indices)
		{
			index_number = sdl_get_item_number(2,bin_var->index,sdl,n_sdl,sdl_id);;
			*(Byte *)(buf+off) = index_number;
			off++;
		}
		else
		{
			index_number++;
		}



		*(Byte *)(buf+off) = bin_var->type;
		off++;
		if(bin_var->type == 0x02)
		{
			*(Byte *)(buf+off) = 0; //bin_var->static2;
			off++;
			off+=encode_urustring(buf+off,bin_var->string2,strlen((char *)&bin_var->string2),1);
		}
		*(Byte *)(buf+off) = bin_var->flags;
		off++;

		//Unknown behaviour, we don't know the explanation that causes this, but well, here goes
		if(bin_var->flags==0x00 && bin_var->type==0x00) {
			//then the next one are the real flags
			*(Byte *)(buf+off) = 0;
			off++;
		}

		if(bin_var->flags & 0x04)
		{
			*(U32 *)(buf+off) = bin_var->timestamp;
			off+=4;
			*(U32 *)(buf+off) = bin_var->microseconds;
			off+=4;
		}
		if(!(bin_var->flags & 0x08))
		{
			bin_var->array_count=sdl[sdl_id].vars[bin_var->index].array_size;
			if(bin_var->array_count==0) {
				*(U32 *)(buf+off) = bin_var->array_count;
				off+=4;
			}

			//allocate the memory and copy the data
			switch(sdl[sdl_id].vars[bin_var->index].type)
			{
				case 0: //INT (4 bytes)
					memcpy((void *)(buf+off),bin_var->data,sizeof(int)*bin_var->array_count);
					off+=sizeof(int)*bin_var->array_count;
					break;
				case 1: //FLOAT (4 bytes)
					memcpy((void *)(buf+off),bin_var->data,sizeof(float)*bin_var->array_count);
					off+=sizeof(float)*bin_var->array_count;
					break;
				case 3: //STRING32 (32 bytes)
					memcpy((void *)(buf+off),bin_var->data,32*bin_var->array_count);
					off+=32*bin_var->array_count;
					break;
				case 5: //Sub SDL
					return -1;
					break;
				case 2: //BOOL (1 byte)
				case 9: //BYTE (1 byte)
					memcpy((void *)(buf+off),bin_var->data,bin_var->array_count);
					off+=bin_var->array_count;
					break;
				case 8: //TIME (4+4 bytes)
					memcpy((void *)(buf+off),bin_var->data,2*sizeof(int)*bin_var->array_count);
					off+=2*sizeof(int)*bin_var->array_count;
					break;
				case 0xA: //SHORT (2 bytes)
					memcpy((void *)(buf+off),bin_var->data,sizeof(short)*bin_var->array_count);
					off+=sizeof(short)*bin_var->array_count;
					break;
				case 50: //VECTOR3 (3 floats)
				case 51: //POINT3 (3 floats)
					memcpy((void *)(buf+off),bin_var->data,3*sizeof(float)*bin_var->array_count);
					off+=3*sizeof(float)*bin_var->array_count;
					break;
				case 54: //QUATERNION (4 floats)
					memcpy((void *)(buf+off),bin_var->data,4*sizeof(float)*bin_var->array_count);
					off+=4*sizeof(float)*bin_var->array_count;
					break;
				case 55: //RGB8 (3 bytes)
					memcpy((void *)(buf+off),bin_var->data,3*bin_var->array_count);
					off+=3*bin_var->array_count;
					break;
				case 4: //PLKEY (UruObject)
				{
					for(int k=0;k<bin_var->array_count;k++)
					{
						int ret=streamUruObjectDesc(buf+off,(st_UruObjectDesc *)((size_t)bin_var->data + sizeof(st_UruObjectDesc)*k));
						if(ret<0) {
							return -1; //printf("Problem streaming an UruObjectDesc\n");
						}
						off+=ret;
					}
					break;
				}
				default:
					return -1; //printf("Error, unexpected type %i\n",sdl[sdl_id].vars[index_number].type);
					break;
			}


		}
		else
		{
			//default value
		}	
	} //for each value



	*(Byte *)(buf+off) = bin->n_structs;
	off++;

	if(bin->n_structs != struct_counter)
	{
		indices = 1;
	}
	else
	{
		indices = 0;
	}

	index_number=-1;

	for(i=0; i<bin->n_structs; i++)
	{
		//for each struct...
		t_sdl_bin_tuple* bin_var = &bin->structs[i];

		if(indices)
		{
			index_number = sdl_get_item_number(2,bin_var->index,sdl,n_sdl,sdl_id);;
			*(Byte *)(buf+off) = index_number;
			off++;
		}
		else
		{
			index_number++;
		}

		*(Byte *)(buf+off) = bin_var->type;
		off++;
		if(bin_var->type == 0x02)
		{
			*(Byte *)(buf+off) = 0; //bin_var->static2;
			off++;
			off+=encode_urustring(buf+off,bin_var->string2,strlen((char *)&bin_var->string2),1);
		}
		*(Byte *)(buf+off) = bin_var->flags;
		off++;

		//Unknown behaviour, we don't know the explanation that causes this, but well, here goes
		if(bin_var->flags==0x00 && bin_var->type==0x00) {
			//then the next one are the real flags
			*(Byte *)(buf+off) = 0;
			off++;
		}

		if(bin_var->flags & 0x04)
		{
			*(U32 *)(buf+off) = bin_var->timestamp;
			off+=4;
			*(U32 *)(buf+off) = bin_var->microseconds;
			off+=4;
		}
		if(!(bin_var->flags & 0x08))
		{
			bin_var->array_count=sdl[sdl_id].vars[bin_var->index].array_size;
			if(bin_var->array_count==0) {
				*(U32 *)(buf+off) = bin_var->array_count;
				off+=4;
			}
			*(Byte *)(buf+off) = (Byte)bin_var->array_count;
			off++; //extra byte for structures

		
			//copy the data
			switch(sdl[sdl_id].vars[bin_var->index].type)
			{
				case 5: //Sub SDL
				{
					int sub_sdl_id=find_sdl_descriptor(sdl[sdl_id].vars[bin_var->index].struct_name,sdl[sdl_id].vars[bin_var->index].struct_version,sdl,n_sdl);
					if(sub_sdl_id<0)
					{
						//not found :(
						return -1;
					}

					for(int k=0; k<bin_var->array_count; k++)
					{
						int ret=sdl_stream_t_sdl_binary((t_sdl_binary *)((size_t)bin_var->data + k*sizeof(t_sdl_binary)),
						                               sdl,n_sdl,sub_sdl_id,buf+off);
						if(ret < 0)
						{
							return -1; //error
						}
						off+=ret;
					}
				} break;
				default:
					return -1; //printf("Error, unexpected type %i\n",sdl[sdl_id].vars[index_number].type);
					break;
			}
		}
		else
		{
			//default value
		}

	}//for each structure

	return off;
}

/**	\brief creates a plain t_sdl_binary structure by using a SDL definition as a template
	\param bin the t_sdl_binary in which the data will be stored
	\param sdl Pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param sdl_id number of the SDL definition that should be used as a template
*/
int sdl_fill_t_sdl_binary_by_sdl_id(t_sdl_binary * bin,t_sdl_def * sdl,int n_sdl,int sdl_id)
{
	int value_counter=0;
	int struct_counter=0;

	int i;

	for(i=0; i<sdl[sdl_id].n_vars; i++)
	{
		if(sdl[sdl_id].vars[i].type == 0x05)
			struct_counter++;
		else
			value_counter++;
	}

	bin->n_values = value_counter;
	bin->n_structs = struct_counter;

	bin->values = (t_sdl_bin_tuple *)malloc(sizeof(t_sdl_bin_tuple)*value_counter);
	bin->structs = (t_sdl_bin_tuple *)malloc(sizeof(t_sdl_bin_tuple)*struct_counter);

	value_counter = struct_counter = 0;
	for(i=0; i<sdl[sdl_id].n_vars; i++)
	{
		if(sdl[sdl_id].vars[i].type == 0x05)
		{
			bin->structs[struct_counter].index = i;
			bin->structs[struct_counter].type = 2;
			//bin->structs[struct_counter].static2 = 0;
			bin->structs[struct_counter].string2[0] =0;
			bin->structs[struct_counter].flags = 0x08;
			bin->structs[struct_counter].timestamp = 0;
			bin->structs[struct_counter].microseconds = 0;
			bin->structs[struct_counter].array_count = 0;
			bin->structs[struct_counter].data_size = 0;
			bin->structs[struct_counter].data = 0;
			struct_counter++;
		}
		else
		{
			bin->values[value_counter].index = i;
			bin->values[value_counter].type = 2;
			//bin->values[value_counter].static2 = 0;
			bin->values[value_counter].string2[0] =0;
			bin->values[value_counter].flags = 0x08;
			bin->values[value_counter].timestamp = 0;
			bin->values[value_counter].microseconds = 0;
			bin->values[value_counter].array_count = 0;
			bin->values[value_counter].data_size = 0;
			bin->values[value_counter].data = 0;
			value_counter++;
		}
	}

	return 0;
}



/**	\brief updates a t_sdl_binary structure with the entries of another struct which contains some new tuples
	\param bin1 the structure that will be updated
	\param bin2 the strucutre which contains some new tuples
	\param how specifies how the structure should be updated, 0x01 updates the structure normally,
	           0x02 updates it and prepares it for a SDLState message by removing the string
*/
int sdl_update_t_sdl_binary(/*t_sdl_def * sdl,int n_sdl,int sdl_id,*/t_sdl_binary * bin1,t_sdl_binary * bin2,int how)
{
	int i;

	for(i=0; i<bin1->n_values; i++)
	{
		for(int j=0; j<bin2->n_values; j++)
		{
			if(bin2->values[j].index == bin1->values[i].index)
			{
				//update a value
				if(!(bin1->values[i].flags & 0x08))
					free(bin1->values[i].data);
				memcpy(&bin1->values[i],&bin2->values[j],sizeof(t_sdl_bin_tuple));
				if(!(bin1->values[i].flags & 0x08))
				{
					bin1->values[i].data = malloc(bin1->values[i].data_size);
					memcpy(bin1->values[i].data,bin2->values[j].data,bin1->values[i].data_size);
				}
#if 0
				gettimeofday((timeval *)&bin1->values[i].timestamp,0);
				//the plasma servers don't do something like that...
#endif

				break;
			}
		}

		if(how == 1)
		{
#if 0
			//add the flag 0x04 (timestamp)
			if(bin1->values[i].flags & 0x08)
				bin1->values[i].flags = 0x0C; //0x04|0x08;
			else
				bin1->values[i].flags = 0x04;
#endif

			bin1->values[i].string2[0]=0x00; //remove the string...
		}
	}


	//todo modi: hmm... updating structures might not work as expected. will have to be tested...
	for(i=0; i<bin1->n_structs; i++)
	{
		for(int j=0; j<bin2->n_structs; j++)
		{
			if(bin2->structs[j].index == bin1->structs[i].index)
			{
				//update a struct
				if(!(bin1->structs[i].flags & 0x08))
					free(bin1->structs[i].data);
				memcpy(&bin1->structs[i],&bin2->structs[j],sizeof(t_sdl_bin_tuple));
				if(!(bin1->structs[i].flags & 0x08))
				{
					bin1->structs[i].data = malloc(bin1->structs[i].data_size);
					memcpy(bin1->structs[i].data,bin2->structs[j].data,bin1->structs[i].data_size);
				}

#if 0
				gettimeofday((timeval *)&bin1->values[i].timestamp,0);
				//the plasma servers don't do something like that...
#endif

				break;
			}
		}
		if(how == 1)
		{
#if 0
			//add the flag 0x04 (timestamp)
			if(bin1->structs[i].flags & 0x08)
				bin1->structs[i].flags = 0x0C; //0x04|0x08;
			else
				bin1->structs[i].flags = 0x04;
#endif

			bin1->structs[i].string2[0]=0x00; //remove the string...
		}
	}

	return 0;
}















/**	\brief If necessary decompresses and parses a t_sdl_head structure.
	\param buf pointer to the buffer in which contains the unparsed structure
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param sdl_head pointer to the t_sdl_head structure in which the result will be stored
*/
int sdl_decompress_and_parse_t_sdl_head(st_log *log, Byte *buf,t_sdl_def * sdl,int n_sdl,t_sdl_head *out)
{
	int off;

	int ret;

	int compsize, realsize;
	Byte cflag; //compression flag, 0x02=>compressed, 0x00=>plain data


	realsize=*(U32 *)buf;
	off=4;
	cflag=*(Byte *)(buf+off);
	off++;

	compsize=*(U32 *)(buf+off);
	off+=4;
	//strip out 2 bytes of the compressed size
	compsize-=2;


	out->object_present = *(Byte *)(buf+off);
	off++;
	if((out->object_present!=0x01) && (out->object_present!=0x00))
	{
		plog(log,"Unexpected flag; object_present is %02X instead of 0x01 or 0x00\n",out->object_present);
		return -1;
	}

	if(*(Byte *)(buf+off)!=0x80)
	{
		plog(log,"Unexpected SDL magic: %02X instead of 0x80\n",*(Byte *)(buf+off));
		return -1;
	}
	off++;


	Byte* dbuf;
	if(cflag == 0x02)
	{
		//allocation dbuffer
		dbuf=(Byte *)malloc(realsize);
		ret = uncompress(dbuf,(uLongf*)&realsize,buf+off,compsize);
		if(ret)
		{
			plog(log,"Error while decompressing the data.\n");
			free(dbuf);
			return -1;
		}
		off+=compsize;
	} else {
		//allocation dbuffer
		dbuf=(Byte *)malloc(compsize);
		memcpy(dbuf,buf+off,compsize);
		off+=compsize;
	}

	int ret_off=off; //save it to returning it at the end
	off=0;

	//well, now we have a buffer filled with the uncompressed binary sdl data... ("dbuf")
	//let's fill the t_sdl_head with it

	off+=decode_urustring(out->name,dbuf+off,SSTR);
	off+=2;
	out->version=*(U16 *)(dbuf+off);
	off+=2;
	plog(log,"SDL:%s Version:%i\n",out->name,out->version);
	
	if(out->object_present==0x01)
	{
		//the UruObject
		ret=storeUruObjectDesc(dbuf+off,&out->o,0xfff);
		if(ret<0) {
			plog(log,"Error parsing an UruObject\n");
			free(dbuf);
			return -1;
		}
		off+=ret;
		dumpUruObjectDesc(log,&out->o);
	}

	ret=find_sdl_descriptor(out->name,out->version,sdl,n_sdl);
	if(ret<0) {
		plog(log,"FATAL: Can't find an SDL descriptor for %s v%i!\n",out->name,out->version);
		free(dbuf);
		return -1;
	}
	//out->sdl_descriptor=ret;


	//parse t_sdl_binary
	ret=sdl_parse_t_sdl_binary(log,dbuf+off,sdl,n_sdl,ret,&out->bin);
	if(ret<1)
	{
		plog(log,"Error while parsing the binary data.\n");
		free(dbuf);
		return -1;
	}
	//else
	//	off+=ret;
	//as the important data ends here, we do not have to increase the offset

	free(dbuf);

	return ret_off;
}


/**	\brief Streams and - if neccessary - compresses a t_sdl_head structure. Saves the result in a buffer.
	\param sdl_head pointer to the t_sdl_head structure that should be streamed and compressed
	\param sdl pointer to the SDL definitions
	\param n_sdl counter of the SDL definitions
	\param buf pointer to the buffer in which the result will be saved
*/
int sdl_stream_and_compress_t_sdl_head(t_sdl_head *sdl_head,t_sdl_def * sdl,int n_sdl,Byte *buf)
{
	int off;
	int ret;

	Byte* dbuf;
	dbuf=(Byte *)malloc(2*1024);

	off=encode_urustring(dbuf,sdl_head->name,strlen((char *)sdl_head->name),1);
	
	*(U16 *)(dbuf+off) = sdl_head->version;
	off+=2;

	if(sdl_head->object_present==0x01)
	{
		ret=streamUruObjectDesc(dbuf+off,&sdl_head->o);
		if(ret<0)
		{
			free(dbuf);
			return -1;
		}
		off+=ret;
	}

	ret=find_sdl_descriptor((Byte *)&sdl_head->name,sdl_head->version,sdl,n_sdl);
	if((ret<0)) // || (sdl_head->sdl_descriptor != ret))
	{
		free(dbuf);
		return -1;
	}
	//ret=sdl_head->sdl_descriptor; //should also work, but never trust the input of an user! :P

	
	ret=sdl_stream_t_sdl_binary(&sdl_head->bin,sdl,n_sdl,ret,dbuf+off);
	if(ret<0)
	{
		free(dbuf);
		return -1;
	}
	off+=ret;

	int realsize;
	realsize=off;

	off=0;

	Byte cflag; //compression flag, 0x02=>compressed, 0x00=>plain data

	if(realsize>0xFF) //the 0xFF is just a guess
	{
		//compress it
		cflag=0x02;
		*(U32 *)(buf+off) = realsize;
	}
	else
	{
		//just copy it - no compression
		cflag=0x00;
		*(U32 *)(buf+off) = 0;
	}
	off+=4;
	*(Byte *)(buf+off) = cflag;
	off++;
	int compression_size_off = off;
	off+=4;
	*(Byte *)(buf+off) = sdl_head->object_present;
	off++;
	*(Byte *)(buf+off) = 0x80;
	off++;

	if(cflag == 0x02)
	{
		uLongf outsize = realsize;
		ret = compress((Bytef *)(buf+off),&outsize,(Bytef *)dbuf,realsize);
		if(ret)
		{
			free(dbuf);
			return -1;
		}
		*(U32 *)(buf+compression_size_off) = outsize+2;
		off+=outsize;
	}
	else
	{
		memcpy((buf+off),dbuf,realsize);
		*(U32 *)(buf+compression_size_off) = realsize+2;
		off+=realsize;
	}

	free(dbuf);

	return off;
}

#endif
