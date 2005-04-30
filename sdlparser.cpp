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

#ifndef __U_SDL_PARSER_
#define __U_SDL_PARSER_
#define __U_SDL_PARSER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"


#include <stdio.h>
#include <string.h>
//#include <math.h>
#include <zlib.h>
//#include <ctype.h>
//#include <dirent.h>
//#include <errno.h>
//#include <sys/types.h>

#include "data_types.h" //for U32,Byte and others
#include "conv_funs.h"
#include "stdebug.h"

#include "urustructs.h"
#include "sdl_desc.h"
#include "sdl_obj.h"

#include "sdlparser.h"

//TODO
//returns the size if all goes OK
//int stream_sdl_binary_data(FILE * f, char * buf,t_sdl_var * var
#if 0
			} else if(sdl[i].vars[e].flag==0x01) {
				sdl[i].vars[e].struct_version=*(U16 *)(buf+off);
				off+=2;
			}
#endif

int sdl_parse_sub_data(FILE * f,Byte * buf,int totalsize,t_sdl_def * sdl,int sdl_id) {

	int off;

	FILE * ferr;
	ferr=stderr;

	off=0;
	int ret;//,sdl_id;

	//U32 n_vars;
	int e,j,k; //i
	//char my_char;

	int ok=0;

	//per object
	//Byte flag; //0x00
	//Byte magic; //0x80
	//Byte string[SSTR+1];
	//U16 version;
	//st_UruObjectDesc o;
	U16 u16k1; //0x00
	Byte static1; //0x06
	Byte n_vals;

	//per value
	Byte type; //val type
	//if type==0x02 ?
		Byte static2; //0x00
		U16 u16k2; //0xF000 //some people says that is a null inverted urustring
	Byte flags; //the flags
		//if (flags and 0x04)
		U32 timestamp; //0x00
		U32 microseconds; //0x00
	//the value if !(flags and 0x08)
		int n; //number of values on dynamic structures
		Byte value; //for a Byte and Bolean values
		float floats[4]; //for POINT3, QUATERNION
		st_UruObjectDesc oo; //The PLKEY
		int int_val; //for int

	//so, flags are:
	//0x04 Timestamp (2 int's)
	//0x08 Default Value

	//-- more

	u16k1=*(U16 *)(buf+off);
	off+=2;
	if(u16k1!=0x00) {
		fprintf(ferr,"Unexpected u16k1:%04X\n",u16k1);
		ok=-1;
		return -1;
	}
	static1=*(Byte *)(buf+off);
	off++;
	if(static1!=0x06) {
		fprintf(ferr,"Unexpected static1:%02X\n",static1);
		ok=-1;
		return -1;
	}

	//values
	n_vals=*(Byte *)(buf+off);
	off++;
	fprintf(f,"\r\n  Found %i vals:\r\n",n_vals);

	//ret=find_sdl_descriptor(string,version,sdl,n_sdl);
	//if(ret<0) {
	//	fprintf(ferr,"FATAL: Can't find an SDL descriptor for %s v:%i in the header!\n",string,version);
	//	ok=-1;
	//	return -1;
	//}
	//sdl_id=ret;

	//the values
	if(n_vals>sdl[sdl_id].n_vars) {
		fprintf(ferr,"FATAL/TERRIBLE: Incorrect number of stored values, SDLDESC has %i when the AGESDL has %i\n",sdl[sdl_id].n_vars,n_vals);
		ok=-1;
		return -1;
	}

	e=-1;

	for(j=0; j<n_vals; j++) {
		e++;
		type=*(Byte *)(buf+off);
		off++;
		if(type!=0x00 && type!=0x02) {
			fprintf(ferr,"Unexpected type:%02X\n",type);
			ok=-1;
			break;
		}

		if(type==0x02) {
			static2=*(Byte *)(buf+off);
			off++;
			if(static1!=0x06) {
				fprintf(ferr,"Unexpected static2:%02X\n",static2);
				ok=-1;
				break;
			}
			u16k2=*(U16 *)(buf+off);
			off+=2;
			if(u16k2!=0xF000) {
				fprintf(ferr,"Unexpected u16k2:%04X\n",u16k2);
				ok=-1;
				break;
			}
		}

		flags=*(Byte *)(buf+off);
		off++;

		//Unknown behabiour, we don't know the explanation that causes this, but well, here goes
		if(flags==0x00 && type==0x00) {
			//then the next one are the real flags
			flags=*(Byte *)(buf+off);
			off++;
		}
		//OK; OK; OK, IF there are structs & vars mixed, then there is an extra byte with the
		// index

		while(sdl[sdl_id].vars[e].type==0x05 && e<sdl[sdl_id].n_vars) { //It's a struct? skip it
			e++;
		}

		fprintf(f,"    [%i] %s(%i,%s) Type: %02X(%i) flags:%02X ",e+1,\
		sdl[sdl_id].vars[e].name,sdl[sdl_id].vars[e].type,\
		get_var_type_nspc(sdl[sdl_id].vars[e].type),type,type,flags);

		if(!(flags & 0x0C) && flags!=0x00 || (flags & 0x00)!=0x00) {
			fprintf(ferr,"Unexpected flags:%02X!\n",flags);
			ok=-1;
			break;
		}

		if(flags & 0x04) { //0x04 is timestamp flag
			timestamp=*(U32 *)(buf+off);
			off+=4;
			microseconds=*(U32 *)(buf+off);
			off+=4;
			fprintf(f,"Stamp:%s ",get_stime(timestamp,microseconds));
		}

		if(!(flags & 0x08)) { //0x08 is default value flag
			//seems that really hard work needs to be done here :/
			n=sdl[sdl_id].vars[e].array_size;
			if(n==0) {
				n=*(U32 *)(buf+off);
				off+=4;
			}

			fprintf(f,"N_elems:%i ",n);

			fprintf(f,"Value:");
			//now fetch the array
			for(k=0;k<n;k++) {
				if(k>0) { fprintf(f,","); }
				switch(sdl[sdl_id].vars[e].type) {
					case 0: //INT (4 bytes)
						int_val=*(int *)(buf+off);
						off+=4;
						fprintf(f,"%i",int_val);
						break;
					case 1: //FLOAT (4 bytes)
						floats[0]=*(float *)(buf+off);
						off+=4;
						fprintf(f,"%f",floats[0]);
						break;
					case 2: //BOOL (1 byte)
					case 9: //BYTE (1 byte)
						value=*(Byte *)(buf+off);
						off++;
						fprintf(f,"%i",value);
						break;
					case 8: //TIME (4+4 bytes)
						timestamp=*(U32 *)(buf+off);
						off+=4;
						microseconds=*(U32 *)(buf+off);
						off+=4;
						fprintf(f,"%s",get_stime(timestamp,microseconds));
						break;
					case 50: //VECTOR3 (3 floats)
					case 51: //POINT3 (3 floats)
						floats[0]=*(float *)(buf+off);
						off+=4;
						floats[1]=*(float *)(buf+off);
						off+=4;
						floats[2]=*(float *)(buf+off);
						off+=4;
						fprintf(f,"(%f,%f,%f)",floats[0],floats[1],floats[2]);
						break;
					case 54: //QUATERNION (4 floats)
						floats[0]=*(float *)(buf+off);
						off+=4;
						floats[1]=*(float *)(buf+off);
						off+=4;
						floats[2]=*(float *)(buf+off);
						off+=4;
						floats[3]=*(float *)(buf+off);
						off+=4;
						fprintf(f,"(%f,%f,%f,%f)",floats[0],floats[1],floats[2],floats[3]);
						break;
					case 4: //PLKEY (UruObject)
						ret=storeUruObjectDesc(buf+off,&oo,totalsize-off);
						if(ret<0) {
							fprintf(ferr,"Problem parsing an UruObjectDesc\n");
							ok=-1;
							break;
						}
						off+=ret;
						dumpUruObjectDesc(f,&oo);
						break;
					default:
						fprintf(ferr,"Error, unexpected type %i\n",sdl[sdl_id].vars[e].type);
						ok=-1;
						break;
				}
			}
			fprintf(f," ");
			if(ok<0) { break; }
		}

		fprintf(f,"\r\n");
		//break;
	}
	if(ok<0) {
		return -1;
	}
	return off;
}


//to parse sdl files
int sdl_parse_binary_data(FILE * f,Byte * buf,int totalsize,t_sdl_def * sdl,int n_sdl) {
	int off;

	FILE * ferr;
	ferr=stderr;

	off=0;
	int ret,sdl_id,sdl_id2;

	U32 n_vars;
	int i,e,j,k;
	//char my_char;

	int ok=0;

	//let's start
	n_vars=*(U32 *)(buf+off);
	off+=4;
	fprintf(f,"\r\nTotal number of binary sdl records: %i\r\n\r\n",n_vars);

	//per object
	Byte flag; //0x00
	Byte magic; //0x80
	Byte string[SSTR+1];
	U16 version;
	st_UruObjectDesc o;
	//U16 u16k1; //0x00
	//Byte static1; //0x06
	Byte n_vals;

	//per value
	Byte type; //val type
	//if type==0x02 ?
		//Byte static2; //0x00
		//Byte string2[SSTR+1];
	Byte flags; //the flags
		//if (flags and 0x04)
		U32 timestamp; //0x00
		U32 microseconds; //0x00
	//the value if !(flags and 0x08)
		int n; //number of values on dynamic structures
		Byte value; //for a Byte and Bolean values
		//float floats[4]; //for POINT3, QUATERNION
		//st_UruObjectDesc oo; //The PLKEY
		//int int_val; //for int

	//so, flags are:
	//0x04 Timestamp (2 int's)
	//0x08 Default Value

	//-- more


	//go for each game object attached to a sdl
	for(i=0; i<(int)n_vars; i++) {
		flag=*(Byte *)(buf+off);
		off++;
		if(flag!=0x01) {
			fprintf(ferr,"Unexpected flag:%02X\n",flag);
			ok=-1;
			break;
		}
		magic=*(Byte *)(buf+off);
		off++;
		if(magic!=0x80) {
			fprintf(ferr,"Unexpected sdl magic:%02X\n",magic);
			ok=-1;
			break;
		}
		off+=decode_urustring(string,buf+off,SSTR);
		off+=2;
		version=*(U16 *)(buf+off);
		off+=2;
		fprintf(f,"[%i] SDL:%s Version:%i\r\n  ",i+1,string,version);
		//the UruObject
		ret=storeUruObjectDesc(buf+off,&o,totalsize-off);
		if(ret<=0) {
			fprintf(ferr,"Error parsing an UruObject\n");
			ok=-1;
			break;
		}
		off+=ret;
		dumpUruObjectDesc(f,&o);

		ret=find_sdl_descriptor(string,version,sdl,n_sdl);
		if(ret<0) {
			fprintf(ferr,"FATAL: Can't find an SDL descriptor for %s v:%i in the header!\n",string,version);
			ok=-1;
			return -1;
		}
		sdl_id=ret;


		//binary sub data
		ret=sdl_parse_sub_data(f,buf+off,totalsize-off,sdl,sdl_id);
		if(ret<0) {
			ok=-1;
		} else {
			off+=ret;
		}

		if(ok==-1) { break; }

		//structs
		n_vals=*(Byte *)(buf+off);
		off++;
		fprintf(f,"  struct found %i vals:\r\n",n_vals);

		//if(n_vals!=0) {
		//		fprintf(ferr,"Unexpected n_vals:%02X!\n",n_vals);
		//		ok=-1;
		//		break;
		//}
		e=-1;

		for(j=0; j<n_vals; j++) {
			e++;
			type=*(Byte *)(buf+off);
			off++;
			if(type!=0x00) {
				fprintf(ferr,"M:Unexpected type:%02X\n",type);
				ok=-1;
				break;
			}

			flags=*(Byte *)(buf+off);
			off++;

			while(sdl[sdl_id].vars[e].type!=0x05 && e<sdl[sdl_id].n_vars) { //It'snt struct? skp it
				e++;
			}

			fprintf(f,"    [%i] %s(%i,%s) Type: %02X(%i) flags:%02X ",e+1,\
			sdl[sdl_id].vars[e].name,sdl[sdl_id].vars[e].type,\
			sdl[sdl_id].vars[e].struct_name,type,type,flags);

			if(!(flags & 0x0C) && flags!=0x00 || (flags & 0x00)!=0x00) {
				fprintf(ferr,"M:Unexpected flags:%02X!\n",flags);
				ok=-1;
				break;
			}

			if(flags & 0x04) { //0x04 is timestamp flag
				timestamp=*(U32 *)(buf+off);
				off+=4;
				microseconds=*(U32 *)(buf+off);
				off+=4;
				fprintf(f,"Stamp:%s ",get_stime(timestamp,microseconds));
			}

			if(!(flags & 0x08)) { //0x08 is default value flag
				//seems that really hard work needs to be done here :/
				n=sdl[sdl_id].vars[e].array_size;
				if(n==0) {
					n=*(U32 *)(buf+off);
					off+=4;
				}

				ret=find_sdl_descriptor(sdl[sdl_id].vars[e].struct_name,\
				sdl[sdl_id].vars[e].struct_version,sdl,n_sdl);
				if(ret<0) {
					fprintf(ferr,"FATAL: Can't find an SDL descriptor for %s v:%i in the header!\n",string,version);
					ok=-1;
					return -1;
				}
				sdl_id2=ret;
				fprintf(f,"N_elems:%i ",n);


				fprintf(f,"Value:");
				//now fetch the array
				for(k=0;k<n;k++) {
					if(k>0) { fprintf(f,","); }
					value=*(Byte *)(buf+off);
					off++;
					if((k==0 && value!=n) || (k>0 && value!=0x00)) {
						fprintf(ferr,"Unexpected value:%02X at %i/%i!\n",value,k,n);
						ok=-1;
						break;
					}

					ret=sdl_parse_sub_data(f,buf+off,totalsize-off,sdl,sdl_id2);
					if(ret<0) { ok=-1; break; }
					else { off+=ret; }
				}
				fprintf(f," ");

				if(ok==-1) { break; }

				//another one, argh this is recursive..
				value=*(Byte *)(buf+off);
				off++;
				if(value!=0x00) {
					fprintf(ferr,"Too many recursion levels, code needs to be improbed to allow them!\n");
					ok=-1;
					break;
				}

			}

		}

		fprintf(f,"\r\n");

		//break;
		if(ok==-1) { break; }
	}


	fprintf(f,"\r\n\r\n");

	if(ok==-1) {
		fprintf(ferr,"Parse error at offset %i,%08X\n",off,off);
		return -1;
	}

	return off;
}

#endif
