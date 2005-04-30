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

#define __U_SDL_DESC_ID "$Id$"

//#define _DBG_LEVEL_ 10

//#define DEBUG

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef __WIN32__
#  include "windoze.h"
#endif

#ifndef __MSVC__
#  include <dirent.h>
#else
#  include <direct.h>
#endif

#include "data_types.h" //for U32,Byte and others
#include "sdlstrs.h"
#include "stdebug.h"
#include "conv_funs.h"
#include "files.h"

#include "sdl_desc.h"

#include "debug.h"

//Inits the sdl def
void init_sdl_def(t_sdl_def * sdl,int n) {
	int i;
	for(i=0; i<n; i++) {
		sdl[i].vars=NULL;
	}
}

void destroy_sdl_def(t_sdl_def * sdl,int n) {
	int i;
	for(i=0; i<n; i++) {
		if(sdl[i].vars!=NULL) {
			free((void *)sdl[i].vars);
			sdl[i].vars=NULL;
		}
	}
}

//Helpers
char * sdl_get_var_type(Byte type) {
	static char res[50];
	switch(type) {
		case 0x00: return "INT         ";
		case 0x01: return "FLOAT       ";
		case 0x02: return "BOOL        ";
		case 0x03: return "STRING32    ";
		case 0x04: return "PLKEY       ";
		case 0x06: return "CREATABLE   ";
		case 0x08: return "TIME        ";
		case 0x09: return "BYTE        ";
		case 0x0A: return "SHORT       ";
		case 0x0B: return "AGETIMEOFDAY";
		case 50:   return "VECTOR3     ";
		case 51:   return "POINT3      ";
		case 54:   return "QUATERNION  ";
		case 55:   return "RGB8        ";
		default: {
			sprintf(res,"Unknown%i",type);
			//return "Unknown";
			return res;
		}
	}
}

char * sdl_get_var_type_nspc(Byte type) {
	static char res[50];
	switch(type) {
		case 0x00: return "INT";
		case 0x01: return "FLOAT";
		case 0x02: return "BOOL";
		case 0x03: return "STRING32";
		case 0x04: return "PLKEY";
		case 0x06: return "CREATABLE";
		case 0x08: return "TIME";
		case 0x09: return "BYTE";
		case 0x0A: return "SHORT";
		case 0x0B: return "AGETIMEOFDAY";
		case 50:   return "VECTOR3";
		case 51:   return "POINT3";
		case 54:   return "QUATERNION";
		case 55:   return "RGB8";
		default: {
			sprintf(res,"!Unknown%i",type);
			//return "Unknown";
			return res;
		}
	}
}

int sdl_get_var_type_from_name(char * buf) {
	if(!strcmp("INT",buf)) { return 0; }
	if(!strcmp("FLOAT",buf)) { return 1; }
	if(!strcmp("BOOL",buf)) { return 2; }
	if(!strcmp("STRING32",buf)) { return 3; }
	if(!strcmp("PLKEY",buf)) { return 4; }
	if(!strcmp("CREATABLE",buf)) { return 6; }
	if(!strcmp("TIME",buf)) { return 8; }
	if(!strcmp("BYTE",buf)) { return 9; }
	if(!strcmp("SHORT",buf)) { return 0x0A; }
	if(!strcmp("AGETIMEOFDAY",buf)) { return 0x0B; }
	if(!strcmp("VECTOR3",buf)) { return 50; }
	if(!strcmp("POINT3",buf)) { return 51; }
	if(!strcmp("QUATERNION",buf)) { return 54; }
	if(!strcmp("RGB8",buf)) { return 55; }
	return -1;
}

/**
This code parses the default, default option and display options.
*/
int sdl_parse_default_options(st_log * f,char * buf,t_sdl_var * var) {
	int off=0; //offset interator
	int c; //current_char
	char bufferA[1024];
	char bufferB[1024];
	int sizeA=0;
	int sizeB=0;
	int i; //,j,k; //an iterator
	int l; //line counter
	//int n;

	Byte mode=0; //Pmode 0 none, 1 bufA, 2 midAB, 3 bufB, 4 end
	Byte default_parsed=0; //is default parsed?
	Byte default_option_parsed=0;
	Byte first_display_parsed=0;

	i=0;
	l=0;

	int totalsize=strlen(buf);

	while(off<totalsize) {
		//read the entire buffer and end when we have all the input
		c=buf[off];
#ifdef DBG_P_PARSER
		printf("%c%i",c,mode);
		printf("\n%s\n",buf); fflush(0);
#endif
		if(c=='=') {
			if(mode==1) {
				mode=2;
				sizeA=i;
				i=0;
			}
			else if(mode!=2) {
				print2log(f,"ERR: Parse error line %i, unexpected character '='\n",l);
				return -1;
			}
		}
		else if(isblank(c)) {
			if(mode==1) {
				mode=2;
				sizeA=i;
				i=0;
			}
			else if(mode==3) {
				sizeB=i;
				i=0;
				mode=0;
			}
		}
		else if(isalpha(c) && mode==0) { //first character of an entry
			bufferA[i]=c;
			i++;
			mode=1;
		}
		else if(isprint(c) && (mode==1 || mode==2 || mode==3)) {
			if(mode==1) {
				bufferA[i]=c;
			}
			else if(mode==3 || mode==2) {
				mode=3;
				bufferB[i]=c;
			}
			i++;
		}
		else {
			print2log(f,"ERR: Parse error at line %i, unexpected character\n",l);
			return -1;
		}

		//do more stuff
		if(mode==0 && sizeA!=0) {
			bufferA[sizeA]='\0';
			bufferB[sizeB]='\0';

			if(!strcmp(bufferA,"DEFAULT")) {
				if(default_parsed==1) {
					print2log(f,"ERR: An SDL record can't contain more than one default value!\n");
					return -1;
				}
				default_parsed=1;
				strcpy((char *)var->default_value,bufferB);
			}
			else if(!strcmp(bufferA,"DEFAULTOPTION")) {
				if(default_option_parsed==1) {
					print2log(f,"ERR: An SDL record can't contain more than one defaultoption value!\n");
					return -1;
				}
				default_option_parsed=1;
				if(!strcmp(bufferB,"VAULT")) {
					var->default_option=var->default_option | 2;
				} else {
					print2log(f,"ERR: Unexpected DEFAULTOPTION!\n");
					return -1;
				}
			}
			else if(!strcmp(bufferA,"DISPLAYOPTION")) {
				if(first_display_parsed==1) {
					strcat((char *)var->display_options,",");
				} else {
					first_display_parsed=1;
				}
				if(!strcmp(bufferB,"hidden")) {
					var->default_option=var->default_option | 1;
				}
				strcat((char *)var->display_options,bufferB);
			}
			sizeA=0;
			sizeB=0;
			i=0;
		}
		off++;

	}

	return off;
}

/**
to compile a single sdl descriptor (adds, or creates items into the sdl struct)
*/
int sdl_compile(st_log * f,Byte * buf,int totalsize,t_sdl_def * sdl) {
	int off=0; //offset interator
	int c; //current_char
	char bufferA[1024];
	char bufferB[1024];
	char bufferC[1024];
	char bufferD[1024];
	int sizeA=0;
	int sizeB=0;
	int sizeC=0;
	int sizeD=0;
	int i,j,k; //an iterator
	int l; //line counter
	//int n;

	Byte comment=0; //comentary mode
	Byte mode=0; //Pmode 0 none, 1 bufA, 2 midAB, 3 bufB, 4 midBC, 5 bufC, 6 midCD, 7 bufD, 8 end
	Byte end=0; //found a } ending key
	Byte win=0; //windoze mode
	Byte version=0; //version parsed?

	int sdl_id;

	i=0;
	l=0;

	while(end==0 && off<totalsize) {
		//read the entire buffer and end when we have all the input
		c=buf[off];
#ifdef DBG_P_PARSER
		printf("%c%i",c,mode);
#endif
		if(c=='}') {
			if(version==0 || mode==1 || mode==2 || mode==3 || mode==4 || mode==5) {
				print2log(f,"ERR: Parse error at line %i, unexpected \"}\"",l);
				return -1;
			} else {
				end=1;
			}
		}
		else if(c=='#') {
			if(mode==1 || mode==2 || mode==3 || ((mode==4 || mode==5) && version==1)) {
				print2log(f,"ERR: Parse error at line %i, unexpected \"#\"",l);
				return -1;
			}
			comment=1; //activate flag
			win=0; //deactivate win flag
			mode=0;
		}
		else if(c=='\n' || c=='\r') {
			comment=0;
			if(mode==1 || mode==2 || ((mode==3 || mode==4) && version==1)) {
				print2log(f,"ERR: Parse error at line %i, missing right member\n",l);
				return -1;
			}
			if(c=='\r') {
				win=1;
				l++;
			}
			if(c=='\n' && win==0) {
				l++;
				win=0;
			}
			if((mode==3 || mode==4)) {
				if(i!=0) {
					sizeB=i;
				}
				i=0;
				mode=0;
			}
			if((mode==5 || mode==6)) {
				if(i!=0) {
					sizeC=i;
				}
				i=0;
				mode=0;
			}
			if((mode==7 || mode==8)) {
				if(i!=0) {
					sizeD=i;
				}
				i=0;
				mode=0;
			}
		}
		else if(comment==0 && (c==' ' || isblank(c))) {
			if(mode==1) {
				mode=2;
				sizeA=i;
				i=0;
			}
			else if(mode==3) {
				sizeB=i;
				i=0;
				mode=4;
			}
			else if(mode==5) {
				sizeC=i;
				i=0;
				mode=6;
			}
			else if(mode==7) {
				bufferD[i]=c;
				i++;
			//	sizeD=i;
			//	i=0;
			//	mode=8;
			}
		}
		else if(isalpha(c) && comment==0 && mode==0) { //first character of an entry
			if(i>=1024) {
				print2log(f,"ERR: Parse error line %i, is too long\n",l);
				return -1;
			}
			bufferA[i]=c;
			i++;
			mode=1;
		}
		else if(isprint(c) && comment==0 && (mode==1 || mode==2 || mode==3 || mode==4 || mode==5 || mode==6 || mode==7)) {
			if(i>=1024) {
				print2log(f,"ERR: Parse error line %i, is too long\n",l);
				return -1;
			}
			if(mode==1) {
				bufferA[i]=c;
			}
			else if(mode==3 || mode==2) {
				mode=3;
				bufferB[i]=c;
			}
			else if(mode==5 || mode==4) {
				mode=5;
				bufferC[i]=c;
			}
			else if(mode==7 || mode==6) {
				mode=7;
				bufferD[i]=c;
			}
			i++;
		}
		else if(comment==0) {
			print2log(f,"ERR: Parse error at line %i, unexpected character\n",l);
			return -1;
		}

#ifdef DBG_P_PARSER
		printf("bufferC is %s[%i,%i(%i)]\n",bufferC,i,sizeC,mode); fflush(0);
#endif

		//do more stuff
		if(mode==0 && sizeA!=0) {
			bufferA[sizeA]='\0';
			bufferB[sizeB]='\0';
			bufferC[sizeC]='\0';
			bufferD[sizeD]='\0';

			if(version==0) {
				if(strcmp(bufferA,"VERSION")) {
					print2log(f,"ERR: Missing version number\n");
					return -1;
				}
				version=1;
				sdl->version=atoi(bufferB);
				sdl->n_vars=0; //initialize to 0
				sdl->vars=NULL; //no me fio ni un pelo
			} else {
				if(strcmp(bufferA,"VAR")) {
					print2log(f,"ERR: Unexpected string at the begginning.\n");
					return -1;
				}

				sdl->n_vars++;


				//printf("Checkpoint...\n"); fflush(0);

				if(sdl->n_vars<=1 || sdl->vars==NULL) { //the first allocation
					//printf("Checkpoint1.2a...\n"); fflush(0);
					sdl->vars=(t_sdl_var *)malloc(sizeof(t_sdl_var) * sdl->n_vars);
				} else {
					//printf("Checkpoint1.2b...\n"); fflush(0);
					sdl->vars=(t_sdl_var *)realloc((void *)sdl->vars,sizeof(t_sdl_var) * sdl->n_vars);
				}
				if(sdl->vars==NULL) { return -1; }
				sdl_id=sdl->n_vars-1;
#ifdef DBG_P_PARSER
				printf("Checkpoint1.5,%i,'%s'...\n",sdl_id,bufferC); fflush(0);
#endif
				//get the var type id
				sdl->vars[sdl_id].type=sdl_get_var_type_from_name(bufferB);

				//printf("Checkpoint2...,'%s'.\n",bufferC); fflush(0);

				strcpy((char *)sdl->vars[sdl_id].struct_name,"");
				sdl->vars[sdl_id].struct_version=0;

				switch(sdl->vars[sdl_id].type) {
					case 55: //RGB8
						sdl->vars[sdl_id].n_itms=3;
						sdl->vars[sdl_id].itm_type=9;
						break;
					case 50: //POINT3
					case 51: //VECTOR3
						sdl->vars[sdl_id].n_itms=3;
						sdl->vars[sdl_id].itm_type=1;
						break;
					case 54: //QUATERNION
						sdl->vars[sdl_id].n_itms=4;
						sdl->vars[sdl_id].itm_type=1;
						break;
					default:
						sdl->vars[sdl_id].n_itms=1;
						sdl->vars[sdl_id].itm_type=sdl->vars[sdl_id].type;
				}

				//now check if it is an struct
				if(bufferB[0]=='$') {
					sdl->vars[sdl_id].flag=0x01;
					sdl->vars[sdl_id].type=0x05;
					strcpy((char *)sdl->vars[sdl_id].struct_name,bufferB+1);
					//strcpy((char *)sdl->vars[sdl_id].name,bufferD+1);
					sdl->vars[sdl_id].default_option=4;
				} else {
					sdl->vars[sdl_id].flag=0x00;
					//strcpy((char *)sdl->vars[sdl_id].name,bufferD);
					sdl->vars[sdl_id].default_option=0;
				}

				//get var name
				k=0;
				for(j=0; j<(int)strlen(bufferC); j++) {
					if(bufferC[j]=='[') { break; }
					sdl->vars[sdl_id].name[k]=bufferC[j];
					k++;
				}
				sdl->vars[sdl_id].name[k]='\0';

#ifdef DBG_P_PARSER
				printf("'%s','%s'\n",sdl->vars[sdl_id].name,bufferC);
#endif

				//get struct size
				if(bufferC[strlen(bufferC)-1]!=']') {
					printf("'%c'..\n",bufferC[strlen(bufferC)-1]);
					print2log(f,"ERR: Expected ']' not found.\n");
					return -1;
				}

#ifdef DBG_P_PARSER
				printf("len:%i,j:%i\n",strlen(bufferC),j);
#endif

				if((int)strlen(bufferC)==j+1+1) {
					sdl->vars[sdl_id].array_size=0;
				} else {
					bufferC[strlen(bufferC)-1]='\0';
					sdl->vars[sdl_id].array_size=atoi(bufferC+j+1);
				}
#ifdef DBG_P_PARSER
				printf("DBG: Array size is %i\n",sdl->vars[sdl_id].array_size);
#endif

				sdl->vars[sdl_id].static1=0x03;
				sdl->vars[sdl_id].u16k1=0x00;
				sdl->vars[sdl_id].u16k2=0x00;

				//and now put the hard stuff
				strcpy((char *)sdl->vars[sdl_id].default_value,"");
				strcpy((char *)sdl->vars[sdl_id].display_options,"");
				strcat((char *)bufferD," ");
				sdl_parse_default_options(f,bufferD,&sdl->vars[sdl_id]);

			}
			sizeA=0;
			sizeB=0;
			sizeC=0;
			sizeD=0;
			i=0;
		}
		off++;

	}

	if(end==0) {
		print2log(f,"ERR: Parse error at line %i, missing characters\n",l);
		return -1;
	}

	return off;
}

/**
Updates the version of the embedded structs, it will set the maxium version to all
*/
int update_sdl_version_structs(st_log * f,t_sdl_def * sdl,int n_sdl) {

	int i,e,j;
	int max_v;
	for(i=0; i<n_sdl; i++) {
		for(e=0; e<sdl[i].n_vars; e++) {
			if(sdl[i].vars[e].type==0x05) { //It's a struct
				max_v=0;
				for(j=0; j<n_sdl; j++) {
					if(!strcmp((char *)sdl[j].name,(char *)sdl[i].vars[e].struct_name) && max_v<=sdl[j].version) {
						max_v=sdl[j].version;
						sdl[i].vars[e].struct_version=max_v;
					}
				}
			}
		}
	}
	return 0;
}

int sdl_statedesc_reader(st_log * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl) {

	int off=0; //offset interator
	int c; //current_char
	char left_buffer[1024];
	char right_buffer[1024];
	int sizeA=0;
	int sizeB=0;
	int i=0; //an iterator
	int l=0; //line counter

	Byte comment=0; //comentary mode
	Byte mode=0; //Parse mode 0 none, 1 left, 2 mid, 3 right, 4 end
	Byte waiting_key=0; //1 waiting for {, 0 startup
	Byte win=0; //windoze mode
	Byte key=0; //key mode

	int sdl_id;
	t_sdl_def * sdl=NULL;

	//*n_sdl=0;

	while(off<totalsize) {
		//read the entire buffer and end when we have all the input
		c=buf[off];
#ifdef DBG_P_PARSER
		printf("%c%i",c,mode); fflush(0);
#endif
		if(comment==0 && c=='{') {
			if(waiting_key==1) {
				key=1;
			} else {
				print2log(f,"ERR: Parse error at line %i, unexpected \"{\"\n",l);
				return -1;
			}
		}
		else if(c=='#') {
			if(mode==1 || mode==2 || mode==3) {
				print2log(f,"ERR: Parse error at line %i, unexpected \"#\"\n",l);
				return -1;
			}
			comment=1; //activate flag
			win=0; //deactivate win flag
			mode=0;
		}
		else if(c=='\n' || c=='\r') {
			comment=0;
			if(mode==1 || mode==2) {
				print2log(f,"ERR: Parse error at line %i, missing right member\n",l);
				return -1;
			}
			if(c=='\r') {
				win=1;
				l++;
			}
			if(c=='\n' && win==0) {
				l++;
				win=0;
			}
			if((mode==3 || mode==4)) {
				sizeB=i;
				i=0;
				mode=0;
			}
		}
		else if(comment==0 && (c==' ' || isblank(c))) {
			if(mode==1) {
				mode=2;
				sizeA=i;
				i=0;
			}
			else if(mode==3) {
				mode=4;
			}
		}
		else if(isalpha(c) && comment==0 && mode==0) { //first character of an entry
			if(i>=1024) {
				print2log(f,"ERR: Parse error line %i, is too long\n",l);
				return -1;
			}
			if(waiting_key==1) {
				print2log(f,"ERR: Parse error line %i, waiting for a {, got an strange character\n",l);
				return -1;
			}
			left_buffer[i]=c;
			i++;
			mode=1;
		}
		else if(isprint(c) && comment==0 && (mode==1 || mode==2 || mode==3)) {
			if(i>=1024) {
				print2log(f,"ERR: Parse error line %i, is too long\n",l);
				return -1;
			}
			if(waiting_key==1) {
				print2log(f,"ERR: Parse error line %i, waiting for a {, got an strange character\n",l);
				return -1;
			}
			if(mode==1) {
				left_buffer[i]=c;
			}
			else if(mode==3 || mode==2) {
				mode=3;
				right_buffer[i]=c;
			}
			i++;
		}
		else if(comment==0) {
			print2log(f,"ERR: Parse error at line %i, unexpected character\n",l);
			return -1;
		}

		//do more stuff
		if(mode==0 && sizeA!=0) {
			waiting_key=1; //now wait for the opening key
			left_buffer[sizeA]='\0';
			right_buffer[sizeB]='\0';
		}

		if(key==1) {
			key=0;
			waiting_key=0;

			if(strcmp(left_buffer,"STATEDESC")) {
				print2log(f,"ERR: Unknown descriptor type\n");
				return -1;
			}

			print2log(f,"Reading %s\n",right_buffer);
			//*n_sdl; //set current number of vars
			(*n_sdl)++;

			if(*n_sdl<=1 || *sdl2==NULL) { //the first allocation
				*sdl2=(t_sdl_def *)malloc(sizeof(t_sdl_def) * *n_sdl);
			} else {
				*sdl2=(t_sdl_def *)realloc((void *)*sdl2,sizeof(t_sdl_def) * *n_sdl);
			}
			if(*sdl2==NULL) { return -1; }
			sdl_id=(*n_sdl)-1;

			sdl=*sdl2;
			init_sdl_def(&sdl[sdl_id],1);

			sdl[sdl_id].flag=0x01;
			strcpy((char *)sdl[sdl_id].name,right_buffer);

			off++;
			off+=sdl_compile(f,buf+off,totalsize-off,&sdl[sdl_id]);
			//off++;
			sizeA=0;
			sizeB=0;
			i=0;
		} else {
			off++;
		}

	}

	//update_sdl_version_structs(f,sdl,*n_sdl);

	if(key==1 || waiting_key==1 || mode==1 || mode==2 || mode==3) {
		print2log(f,"ERR: Parse error at line %i, missing characters\n",l);
		return -1;
	}
#ifdef DBG_P_PARSER
	printf("Statedesc reader finished\n"); fflush(0);
#endif

	return off;
}


int sdl_statedesc_streamer(st_log * f,Byte ** buf2,t_sdl_def * sdl,int n_sdl) {
	int off;

	st_log * ferr;
	ferr=f;

	off=0;

	//t_sdl_def * sdl;

	int i,e,j;
	char my_char;

	Byte * buf=NULL;

	int current_buf_size;

	current_buf_size=1024*2;

	*buf2=(Byte *)malloc(current_buf_size);
	buf=*buf2;

	int ok=0;

	//let's start
	*(U16 *)(buf+off)=(U16)n_sdl;
	off+=2;
	print2log(f,"\r\n# total number of sdl records: %i\r\n\r\n",n_sdl);

	for(i=0; i<n_sdl; i++) {
		//avoid buffer overflow
		if((current_buf_size-1024)<=off) {
			current_buf_size+=1024;
			*buf2=(Byte *)realloc((Byte *)*buf2,current_buf_size);
			buf=*buf2;
		}
		*(Byte *)(buf+off)=sdl[i].flag;
		off++;
		if(sdl[i].flag!=0x01) {
			print2log(ferr,"Unexpected flag:%02X\n",sdl[i].flag);
			ok=-1;
			break;
		}
		off+=encode_urustring(buf+off,sdl[i].name,strlen((char *)sdl[i].name),0x01);
		print2log(f,"STATEDESC %s\r\n{\r\n",sdl[i].name);

		*(U16 *)(buf+off)=sdl[i].version;
		off+=2;
		print2log(f,"    VERSION %i\r\n",sdl[i].version);

		*(U16 *)(buf+off)=sdl[i].n_vars;
		off+=2;
		print2log(f,"    # Num vars: %i\r\n",sdl[i].n_vars);

		for(e=0; e<sdl[i].n_vars; e++) {
			//avoid buffer overflow
			if((current_buf_size-1024)<=off) {
				current_buf_size+=1024;
				*buf2=(Byte *)realloc((Byte *)*buf2,current_buf_size);
				buf=*buf2;
			}
			*(Byte *)(buf+off)=sdl[i].vars[e].flag;
			off++;
			if(sdl[i].vars[e].flag!=0x00 && sdl[i].vars[e].flag!=0x01) {
				print2log(ferr,"Unexpected sub_flag:%02X\n",sdl[i].vars[e].flag);
				ok=-1;
				break;
			}
			*(Byte *)(buf+off)=sdl[i].vars[e].static1;
			off++;
			if(sdl[i].vars[e].static1!=0x03) {
				print2log(ferr,"Unexpected static1:%02X\n",sdl[i].vars[e].static1);
				ok=-1;
				break;
			}
			off+=encode_urustring(buf+off,sdl[i].vars[e].name,strlen((char *)sdl[i].vars[e].name),0x01);
			off+=encode_urustring(buf+off,sdl[i].vars[e].display_options,strlen((char *)sdl[i].vars[e].display_options),0x00);
			*(U16 *)(buf+off)=sdl[i].vars[e].array_size;
			off+=2;
			*(U16 *)(buf+off)=sdl[i].vars[e].u16k1;
			off+=2;
			if(sdl[i].vars[e].u16k1!=0x00) {
				print2log(ferr,"Unexpected u16k1:%04X\n",sdl[i].vars[e].u16k1);
				ok=-1;
				break;
			}
			*(Byte *)(buf+off)=sdl[i].vars[e].type;
			off++;
			off+=encode_urustring(buf+off,sdl[i].vars[e].default_value,strlen((char *)sdl[i].vars[e].default_value),0x01);

			*(U16 *)(buf+off)=sdl[i].vars[e].default_option;
			off+=2;
			*(U16 *)(buf+off)=sdl[i].vars[e].u16k2;
			off+=2;
			if(sdl[i].vars[e].u16k2!=0x00) {
				print2log(ferr,"Unexpected u16k2:%04X\n",sdl[i].vars[e].u16k2);
				ok=-1;
				break;
			}
			if(sdl[i].vars[e].flag==0x00) {
				*(U16 *)(buf+off)=sdl[i].vars[e].n_itms;
				off+=2;
				*(Byte *)(buf+off)=sdl[i].vars[e].itm_type;
				off++;
			} else if(sdl[i].vars[e].flag==0x01) {
				off+=encode_urustring(buf+off,sdl[i].vars[e].struct_name,strlen((char *)sdl[i].vars[e].struct_name),0x01);
				*(U16 *)(buf+off)=sdl[i].vars[e].struct_version;
				off+=2;
			}

			//now dump this stuff
			if(sdl[i].vars[e].n_itms!=1 && sdl[i].vars[e].flag==0x00) {
#ifdef DEBUG
				print2log(f,"    #n_itms:%i,type:%i,%s\r\n",sdl[i].vars[e].n_itms,\
				sdl[i].vars[e].itm_type,sdl_get_var_type(sdl[i].vars[e].itm_type));
#endif
			}
			if(sdl[i].vars[e].default_option!=0 && sdl[i].vars[e].default_option!=2) {
#ifdef DEBUG
				print2log(f,"    #defaultoption:%i\r\n",sdl[i].vars[e].default_option);
#endif
			}
			if(sdl[i].vars[e].flag==0x01) {
#ifdef DEBUG
				print2log(f,"    #struct_version:%i\r\n",sdl[i].vars[e].struct_version);
#endif
			}
			if(sdl[i].vars[e].flag==0x00) {
				print2log(f,"    VAR %s ",sdl_get_var_type(sdl[i].vars[e].type));
			} else if(sdl[i].vars[e].flag==0x01) {
				print2log(f,"    VAR $%s",sdl[i].vars[e].struct_name);
			}
			print2log(f,"  %s",sdl[i].vars[e].name);
			if(sdl[i].vars[e].array_size==0) {
				print2log(f,"[]");
			} else {
				print2log(f,"[%i]",sdl[i].vars[e].array_size);
			}

			if(strlen((char *)sdl[i].vars[e].default_value)>0) {
				print2log(f,"  DEFAULT=%s",sdl[i].vars[e].default_value);
			}
			if(sdl[i].vars[e].default_option==2) {
				print2log(f,"  DEFAULTOPTION=VAULT");
			}
			if(strlen((char *)sdl[i].vars[e].display_options)>0) {
				my_char=',';
				j=0;
				while(j<=(int)strlen((char *)sdl[i].vars[e].display_options)) {
					if(my_char==',') {
						print2log(f," DISPLAYOPTION=");
					} else {
						print2log(f,"%c",my_char);
					}
					my_char=sdl[i].vars[e].display_options[j];
					j++;
				}
			}

			print2log(f,"\r\n");

			//break;
		}
		if(ok==-1) { break; }

		print2log(f,"}\r\n\r\n");

		//break;

	}

	if(ok==-1) {
		free(*buf2);
		print2log(ferr,"Parse error at offset %i,%08X\n",off,off);
		return -1;
	}

	return off;
}

//to parse sdl files
int sdl_parse_contents(st_log * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl) {
	int off;

	st_log * ferr;
	ferr=f;

	off=0;

	t_sdl_def * sdl;

	U16 n_vars;
	int i,e,j;
	char my_char;

	int ok=0;

	//let's start
	n_vars=*(U16 *)(buf+off);
	off+=2;
	print2log(f,"\r\n# total number of sdl records: %i\r\n\r\n",n_vars);

	*sdl2=(t_sdl_def *)malloc(sizeof(t_sdl_def) * n_vars);
	if(*sdl2==NULL) { return -1; }

	init_sdl_def(*sdl2,n_vars);
	*n_sdl=n_vars;
	sdl=*sdl2;

#if 0
	Byte flag;
	Byte string[SSTR+1];
	U16 version;
	U16 num;

	//Byte flag;
	Byte static1;
	Byte var_name[SSTR+1];
	Byte display_options[SSTR+1];
	U16 array_size;
	U16 u16k1;
	Byte type;
	Byte default_value[SSTR+1];
	U16 default_option;
	U16 u16k2;
	//-> if(flag==0x00)
	U16 n_itms;
	Byte itm_type;
	//-> if(flag==0x01)
	Byte struct_name[SSTR+1];
	U16 struct_version;
#endif

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	for(i=0; i<n_vars; i++) {
	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

		sdl[i].flag=*(Byte *)(buf+off);
		off++;
	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

		if(sdl[i].flag!=0x01) {
			print2log(ferr,"Unexpected flag:%02X\n",sdl[i].flag);
			ok=-1;
			break;
		}

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

		off+=decode_urustring(sdl[i].name,buf+off,SSTR);
		off+=2;
		print2log(f,"STATEDESC %s\r\n{\r\n",sdl[i].name);

		sdl[i].version=*(U16 *)(buf+off);
		off+=2;
		print2log(f,"    VERSION %i\r\n",sdl[i].version);

		sdl[i].n_vars=*(U16 *)(buf+off);
		off+=2;
		print2log(f,"    # Num vars: %i\r\n",sdl[i].n_vars);

		sdl[i].vars=(t_sdl_var *)malloc(sizeof(t_sdl_var) * sdl[i].n_vars);

		for(e=0; e<sdl[i].n_vars; e++) {
			sdl[i].vars[e].flag=*(Byte *)(buf+off);
			off++;
			if(sdl[i].vars[e].flag!=0x00 && sdl[i].vars[e].flag!=0x01) {
				print2log(ferr,"Unexpected sub_flag:%02X\n",sdl[i].vars[e].flag);
				ok=-1;
				break;
			}
			sdl[i].vars[e].static1=*(Byte *)(buf+off);
			off++;
			if(sdl[i].vars[e].static1!=0x03) {
				print2log(ferr,"Unexpected static1:%02X\n",sdl[i].vars[e].static1);
				ok=-1;
				break;
			}
			off+=decode_urustring(sdl[i].vars[e].name,buf+off,SSTR);
			off+=2;
			off+=decode_urustring(sdl[i].vars[e].display_options,buf+off,SSTR);
			off+=2;
			sdl[i].vars[e].array_size=*(U16 *)(buf+off);
			off+=2;
			sdl[i].vars[e].u16k1=*(U16 *)(buf+off);
			off+=2;
			if(sdl[i].vars[e].u16k1!=0x00) {
				print2log(ferr,"Unexpected u16k1:%04X\n",sdl[i].vars[e].u16k1);
				ok=-1;
				break;
			}
			sdl[i].vars[e].type=*(Byte *)(buf+off);
			off++;
			off+=decode_urustring(sdl[i].vars[e].default_value,buf+off,SSTR);
			off+=2;
			sdl[i].vars[e].default_option=*(U16 *)(buf+off);
			off+=2;
			sdl[i].vars[e].u16k2=*(U16 *)(buf+off);
			off+=2;
			if(sdl[i].vars[e].u16k2!=0x00) {
				print2log(ferr,"Unexpected u16k2:%04X\n",sdl[i].vars[e].u16k2);
				ok=-1;
				break;
			}
			sdl[i].vars[e].n_itms=0;
			sdl[i].vars[e].itm_type=0;
			strcpy((char *)sdl[i].vars[e].struct_name,"");
			sdl[i].vars[e].struct_version=0;
			if(sdl[i].vars[e].flag==0x00) {
				sdl[i].vars[e].n_itms=*(U16 *)(buf+off);
				off+=2;
				sdl[i].vars[e].itm_type=*(Byte *)(buf+off);
				off++;
			} else if(sdl[i].vars[e].flag==0x01) {
				off+=decode_urustring(sdl[i].vars[e].struct_name,buf+off,SSTR);
				off+=2;
				sdl[i].vars[e].struct_version=*(U16 *)(buf+off);
				off+=2;
			}

			//now dump this stuff
			if(sdl[i].vars[e].n_itms!=1 && sdl[i].vars[e].flag==0x00) {
#ifdef DEBUG
				print2log(f,"    #n_itms:%i,type:%i,%s\r\n",sdl[i].vars[e].n_itms,\
				sdl[i].vars[e].itm_type,sdl_get_var_type(sdl[i].vars[e].itm_type));
#endif
			}
			if(sdl[i].vars[e].default_option!=0 && sdl[i].vars[e].default_option!=2) {
#ifdef DEBUG
				print2log(f,"    #defaultoption:%i\r\n",sdl[i].vars[e].default_option);
#endif
			}
			if(sdl[i].vars[e].flag==0x01) {
#ifdef DEBUG
				print2log(f,"    #struct_version:%i\r\n",sdl[i].vars[e].struct_version);
#endif
			}
			if(sdl[i].vars[e].flag==0x00) {
				print2log(f,"    VAR %s ",sdl_get_var_type(sdl[i].vars[e].type));
			} else if(sdl[i].vars[e].flag==0x01) {
				print2log(f,"    VAR $%s",sdl[i].vars[e].struct_name);
			}
			print2log(f,"  %s",sdl[i].vars[e].name);
			if(sdl[i].vars[e].array_size==0) {
				print2log(f,"[]");
			} else {
				print2log(f,"[%i]",sdl[i].vars[e].array_size);
			}

			if(strlen((char *)sdl[i].vars[e].default_value)>0) {
				print2log(f,"  DEFAULT=%s",sdl[i].vars[e].default_value);
			}
			if(sdl[i].vars[e].default_option==2) {
				print2log(f,"  DEFAULTOPTION=VAULT");
			}
			if(strlen((char *)sdl[i].vars[e].display_options)>0) {
				my_char=',';
				j=0;
				while(j<=(int)strlen((char *)sdl[i].vars[e].display_options)) {
					if(my_char==',') {
						print2log(f," DISPLAYOPTION=");
					} else {
						print2log(f,"%c",my_char);
					}
					my_char=sdl[i].vars[e].display_options[j];
					j++;
				}
			}
			print2log(f,"\r\n");
			//break;
		}
		if(ok==-1) { break; }

		print2log(f,"}\r\n\r\n");
		//break;
	}

	if(ok==-1) {
		print2log(ferr,"Parse error at offset %i,%08X\n",off,off);
		return -1;
	}
	return off;
}


//Returns -1, if not found, elsewhere the index in the sdl struct where is it
// requires the struct, and the number of elements
int find_sdl_descriptor(Byte * name,U16 version,t_sdl_def * sdl,int n) {
	int i;
	for(i=0; i<n; i++) {
		if(sdl[i].version==version && !strcmp((char *)name,(char *)sdl[i].name)) {
			return i;
		}
	}
	return -1; //not found :(
}

/**	\brief Returns the index of an entry in a t_sdl_def list that has a
	        specified name and the highest found version number
*/
int find_sdl_descriptor_by_name(Byte * name,t_sdl_def * sdl,int n)
{
	int i;
	U16 last_ver=0;
	int ret=-1;


	for(i=0; i<n; i++)
	{
		if(!strcmp((char *)name,(char *)sdl[i].name))
		{
			if(last_ver<sdl[i].version)
				ret=i;
		}
	}
	return ret;
}


/**
Creates a new sdl_desc struct, but without duplicates, it will store only the latest
version. (The new descriptor will be osdl)
*/
int sdl_get_last_descriptors(st_log * f,t_sdl_def ** sdlo,int * n_sdlo,t_sdl_def * sdl,int n_sdl) {
	int i;
	int max,found;

	t_sdl_def * sdl2=NULL;
	t_sdl_def * sdlx=NULL;
	t_sdl_def * sdlc=NULL;
	int n_sdl2=0;
	int n_sdlx=0;
	int n_sdlc=0;

	//init
	sdlx=sdl;
	n_sdlx=n_sdl;

	while(n_sdlx>0) {
		max=0;
		found=0;
		for(i=1; i<n_sdlx; i++) {
			if(!strcmp((char *)sdlx[0].name,(char *)sdlx[i].name)) {
				if(max<=sdlx[i].version) {
					max=sdlx[i].version;
					found=i;
				}
			} else {
				n_sdlc++;
				sdlc=(t_sdl_def *)realloc((void *)sdlc,sizeof(t_sdl_def) * n_sdlc);
				memset(&sdlc[n_sdlc-1],0,sizeof(t_sdl_def));
				//init_sdl_def(&sdlc[n_sdlc-1],1);
				memcpy(&sdlc[n_sdlc-1],&sdlx[i],sizeof(t_sdl_def));
				sdlc[n_sdlc-1].vars=(t_sdl_var *)malloc(sizeof(t_sdl_var) * sdlc[n_sdlc-1].n_vars);
				memcpy(sdlc[n_sdlc-1].vars,sdlx[i].vars,sizeof(t_sdl_var) * sdlc[n_sdlc-1].n_vars);
			}
		}
		n_sdl2++;
		sdl2=(t_sdl_def *)realloc((void *)sdl2,sizeof(t_sdl_def) * n_sdl2);
		memset(&sdl2[n_sdl2-1],0,sizeof(t_sdl_def));
		//init_sdl_def(&sdlc[n_sdl2-1],1);
		memcpy(&sdl2[n_sdl2-1],&sdlx[found],sizeof(t_sdl_def));
		sdl2[n_sdl2-1].vars=(t_sdl_var *)malloc(sizeof(t_sdl_var) * sdl2[n_sdl2-1].n_vars);
		DBG(6,"n_sdl2:%i,found:%i,n_vars:%i\n",n_sdl2,found,sdl2[n_sdl2-1].n_vars);
		memcpy(sdl2[n_sdl2-1].vars,sdlx[found].vars,sizeof(t_sdl_var) * sdl2[n_sdl2-1].n_vars);

		if(sdlx!=sdl) {
			destroy_sdl_def(sdlx,n_sdlx);
			free((void *)sdlx);
			sdlx=NULL;
		}
		sdlx=sdlc;
		n_sdlx=n_sdlc;
		sdlc=NULL;
		n_sdlc=0;
	}

	*sdlo=sdl2;
	*n_sdlo=n_sdl2;

	return 0;
}

#ifndef __MSVC__

//Reads all descriptors from disk
int read_sdl_files(st_log * f,char * address,t_sdl_def ** sdl,int * n_sdl) {
	DIR *dir;
	struct dirent *entry;
	int totalsize,off;
	Byte * buf=NULL;

	st_log * err=NULL;
	err=f;

	char loopy[500];

	if((dir=opendir(address))== NULL) {
		print2log(err,"ERR: Opening %s\n",address);
		logerr(err,"opendir()");
		return -1;
	} else {
		while((entry=readdir(dir))!= NULL) {
			if(!strcasecmp(entry->d_name+(strlen(entry->d_name)-4),".sdl")) {
				print2log(f,"Reading %s...\n",entry->d_name);
				sprintf(loopy,"%s/%s",address,entry->d_name);
				totalsize=readWDYS((char **)&buf,loopy);
				if(totalsize<0) {
					totalsize=readfile((char **)&buf,loopy);
				}
				if(totalsize<0) {
					print2log(err,"ERR: A problem ocurred reading %s\n",loopy);
					return -1;
				}
				off=0;
				if(sdl_statedesc_reader_fast(f,buf+off,totalsize-off,sdl,n_sdl)<0) {
					print2log(err,"ERR: SDL error in statedesc reader call, Something has gone wrong, head for the cover!\n");
					return -1;
				}
				if(buf!=NULL) {
					free((void *)buf);
				}
			}
		}
		closedir(dir);
	}

	update_sdl_version_structs(f,*sdl,*n_sdl);

	//dump the SDL binary info
	Byte * dbuf=NULL;
	totalsize=sdl_statedesc_streamer(f,&dbuf,*sdl,*n_sdl);

	char dumps_path[200];
	sprintf(dumps_path,"%s/dumps",stdebug_config->path);
	mkdir(dumps_path,00750);
	strcat(dumps_path,"/sdl_dump.raw");
	savefile((char *)dbuf,totalsize,dumps_path);

	if(dbuf)
		free(dbuf);

	return 1;
}

#else
//using MSVC

int read_sdl_files(st_log * f,char * address,t_sdl_def ** sdl,int * n_sdl) {

    bool done;
    HANDLE filelist;
    char dir[MAX_PATH+1];
    WIN32_FIND_DATA filedata;

    sprintf(dir, "%s/*", address);

	char *buf=NULL;
	int totalsize,off;
	int ret=0;

	st_log *err=f;

	char loopy[500];


    filelist = FindFirstFile(dir, &filedata);
    if (filelist != INVALID_HANDLE_VALUE)
    {
        done=false;
        while(!done)
        {
			if((!lstrcmpi(filedata.cFileName+(strlen(filedata.cFileName)-4),".sdl")) && (!(filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))) {
				print2log(f,"Reading %s...\n",filedata.cFileName);
				sprintf(loopy,"%s/%s",address,filedata.cFileName);

				totalsize=readWDYS((char **)&buf,loopy);
				if(totalsize<0) {
					totalsize=readfile((char **)&buf,loopy);
				}
				if(totalsize<0) {
					print2log(err,"ERR: A problem ocurred reading %s\n",loopy);
					return -1;
				}
				off=0;
				if(sdl_statedesc_reader_fast(f,(Byte *)(buf+off),totalsize-off,sdl,n_sdl)<0) {
					print2log(err,"ERR: SDL error in statedesc reader call, Something has gone wrong, head for the cover!\n");
					return -1;
				}
				if(buf!=NULL) {
					free((void *)buf);
				}
			}

            if (!FindNextFile(filelist, &filedata))
            {
                if (GetLastError()==ERROR_NO_MORE_FILES)
                {
                    done=true;
                }
            }
        }
    }
	FindClose(filelist);

	update_sdl_version_structs(f,*sdl,*n_sdl);

	//dump the SDL binary info
	Byte * dbuf=NULL;
	totalsize=sdl_statedesc_streamer(f,&dbuf,*sdl,*n_sdl);

	char dumps_path[200];
	sprintf(dumps_path,"%s/dumps",stdebug_config->path);
	mkdir(dumps_path,00750);
	strcat(dumps_path,"/sdl_dump.raw");
	savefile((char *)dbuf,totalsize,dumps_path);

	if(dbuf)
		free(dbuf);

	return 1;
}

#endif //__MSVC__


#ifdef _SDL_TEST_

#include "files.h"

//g++ sdl_desc.cpp -o sdltest -D_SDL_TEST_ files.o stdebug.o whatdoyousee.o conv_funs.o

int main(int argc,char * argv[]) {

	int size;
	char * buf=NULL;

	t_sdl_def * sdl=NULL;
	int n_sdl=0;
	t_sdl_def * sdl2=NULL;
	int n_sdl2=0;

	log_init();
	log_openstdlogs();

	size=readfile(&buf,"Teledahn.sdl");
	sdl_statedesc_reader(f_uru,(Byte *)buf,size,&sdl,&n_sdl);
	free((void *)buf);
	buf=NULL;

	size=sdl_statedesc_streamer(f_uru,(Byte **)&buf,sdl,n_sdl);
	destroy_sdl_def(sdl,n_sdl);
	free((void *)sdl);
	sdl=NULL;
	n_sdl=0;

	sdl_parse_contents(f_uru,(Byte *)buf,size,&sdl,&n_sdl);
	free((void *)buf);
	buf=NULL;

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	sdl_get_last_descriptors(f_uru,&sdl2,&n_sdl2,sdl,n_sdl);

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	size=sdl_statedesc_streamer(f_uru,(Byte **)&buf,sdl2,n_sdl2);

	free((void *)buf);
	buf=NULL;

	destroy_sdl_def(sdl2,n_sdl2);
	free((void *)sdl2);
	sdl2=NULL;
	n_sdl2=0;

	destroy_sdl_def(sdl,n_sdl);
	free((void *)sdl);
	sdl=NULL;
	n_sdl=0;

	read_sdl_files(f_uru,"share/sdl.tpots",&sdl,&n_sdl);

	size=sdl_statedesc_streamer(f_uru,(Byte **)&buf,sdl,n_sdl);
	destroy_sdl_def(sdl,n_sdl);
	free((void *)sdl);
	sdl=NULL;
	n_sdl=0;

	sdl_parse_contents(f_uru,(Byte *)buf,size,&sdl,&n_sdl);
	free((void *)buf);
	buf=NULL;

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	sdl_get_last_descriptors(f_uru,&sdl2,&n_sdl2,sdl,n_sdl);

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	size=sdl_statedesc_streamer(f_uru,(Byte **)&buf,sdl2,n_sdl2);

	free((void *)buf);
	buf=NULL;

	destroy_sdl_def(sdl2,n_sdl2);
	free((void *)sdl2);
	sdl2=NULL;
	n_sdl2=0;

	destroy_sdl_def(sdl,n_sdl);
	free((void *)sdl);
	sdl=NULL;
	n_sdl=0;

	log_shutdown();
}

#endif

typedef struct _tmp_sdl_vars
{
	t_sdl_var content;
	_tmp_sdl_vars *next;
} tmp_sdl_vars;


typedef struct _tmp_sdl_defs
{
	t_sdl_def content;
	_tmp_sdl_defs *next;
} tmp_sdl_defs;

#define offchar_ptr ((char *)(buf+off))
#define offchar (*offchar_ptr)

#define sdl_reading_error "Error while parsing a SDL definition file\n	"

int sdl_statedesc_reader_fast(st_log * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl)
{
	int mode=0;

/*
	modes:
	//0: out of a statedesc block, wait for one or for a comment
	//1: found a statedesc block, search for name
	//2: found the name, search for end
	//3: search for the curly bracket
	//4: found curly bracket, search newline
	//-----------------------------------------------------------
	//5: in a statedesc block, search for VERSION
	//6: in a statedesc block after VERSION, search version number
	//7: in a statedesc block after VERSION, search end of version number
	//8: in a statedesc block after the version number, search for newline or comment
	//9: in a statedesc block, search for VAR
	//10: search for var type
	//11: read var type
	//12: search var name
	//13: search end of var name
	//14: find closing ]
	//15: find additional information (DEFAULT, DEFAULTOPTION, DISPLAYOPTION, INTERNAL, PHASED)
	//    or search for a line break
	//16: find equal sign after DEFAULT
	//17: find default data start
	//18: find end of default data
	//-----------------------------------------------------------
*/
	int off=0;
	int startpos;
	int endpos;
	int line=1;

	int comment=0;

	char tmpbuf[256];

	tmp_sdl_vars sdl_vars;
	tmp_sdl_defs sdl_defs;

	tmp_sdl_vars *var=&sdl_vars;
	tmp_sdl_defs *def=&sdl_defs;

	memset(var,0,sizeof(tmp_sdl_vars));
	memset(def,0,sizeof(tmp_sdl_defs));

	int varcount=0,defcount=0;

	while(off!=totalsize)
	{
		if(comment)
		{
			int newline=0;

			if(offchar=='\r')
			{
				newline=1;
				off++;
			}
			if(offchar=='\n')
			{
				newline=1;
				off++;
			}

			if(newline)
			{
				line++;
				comment=0;
			}
			else
			{
				off++;
				continue;
			}
		}

		if(mode==0)
		{
			//0: out of a statedesc block, wait for one or for a comment
			if(offchar=='\r' || offchar=='\n')
			{
				if(offchar=='\r')
					off++;
				if(offchar=='\n')
					off++;

				line++;
			}
			else if(offchar=='#')
			{
				comment=1;
			}
			else if(!memcmp("STATEDESC",offchar_ptr,sizeof("STATEDESC")-1))
			{
				//found statedesc block
				mode=1;
				off+=sizeof("STATEDESC")-1;
				if(!isspace(offchar) || offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after STATEDESC (line %i)\n",line);
					return -1;
				}

				def->content.flag=0x01;
			}
			else if(!isspace(offchar))
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected data out of STATEDESC (line %i)\n",line);
				return -1;
			}
		}
		else if(mode==1)
		{
			//1: found a statedesc block, search for name
			if(offchar=='\r' || offchar=='\n')
			{
				print2log(f,sdl_reading_error);
				print2log(f,"found a linebreak after STATEDESC (line %i)\n",line);
				return -1;
			}
			if(!isspace(offchar))
			{
				startpos=off;
				mode=2;
			}
			
			off++;
		}
		else if(mode==2)
		{
			//2: found the name, search for end
			if(isspace(offchar))
			{

				//found end of the name
				endpos=off;

				//store it
				memcpy(&def->content.name,buf+startpos,endpos-startpos);
				def->content.name[endpos-startpos]=0;

				mode=3;
			}
			else
				off++;
		}
		else if(mode==3)
		{
			//3: search for the curly bracket
			if(offchar=='#')
			{
				comment=1;
				continue;
			}

			if(offchar=='\r' || offchar=='\n')
			{
				if(offchar=='\r')
					off++;
				if(offchar=='\n')
					off++;

				line++;
			}
			if(offchar=='{')
			{
				mode=4;
			}
			off++;
		}
		else if(mode==4)
		{
			if(offchar=='#')
			{
				comment=1;
				mode=5;
				continue;
			}

			int newline=0;
			//4: found curly bracket, search newline
			if(offchar=='\r')
			{
				newline=1;
				off++;
			}

			if(offchar=='\n')
			{
				newline=1;
				off++;
			}

			if(newline)
			{
				//found newline
				line++;
				mode=5;
			}
			else
				off++;
		}
		//-----------------------------------------------------------
		else if(mode==5)
		{
			//5: in a statedesc block, search version
			if(!memcmp("VERSION",offchar_ptr,sizeof("VERSION")-1))
			{
				//found version
				mode=6;
				off+=sizeof("VERSION")-1;
				if(!isspace(offchar) || offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after VERSION (line %i)\n",line);
					return -1;
				}
			}
			else if(offchar=='#')
			{
				comment=1;
			}
			else if(isspace(offchar))
			{
				int newline=0;
				if(offchar=='\n')
				{
					off++;
					newline=1;
				}
				if(offchar=='\r')
				{
					off++;
					newline=1;
				}
				if(newline)
					line++;
				else
					off++;
			}
			else
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected data after the opening curly bracket (line %i)\n",line);
				return -1;
			}
		}
		else if(mode==6)
		{
			//6: in a statedesc block after VERSION, search version number
			if(offchar=='\n' || offchar=='\r')
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected line break after VERSION (lines %i/%i)\n",line,line+1);
				return -1;
			}

			if(!isspace(offchar))
			{
				//found version number start point
				startpos=off;
				mode=7;
			}
			off++;
		}
		else if(mode==7)
		{
			//7: in a statedesc block after VERSION, search end of version number
			if(isspace(offchar))
			{
				endpos=off;

				memcpy(&tmpbuf,buf+startpos,endpos-startpos);
				tmpbuf[endpos-startpos]=0;

				def->content.version=atoi((char *)&tmpbuf);

				mode=8;
			}
			else
				off++;
		}
		else if(mode==8)
		{
			//8: in a statedesc block after the version, search for newline or comment

			if(offchar=='\r' || offchar=='\n')
			{
				if(offchar=='\r')
					off++;
				if(offchar=='\n')
					off++;

				line++;
				mode=9;
			}
			else if(offchar=='#')
			{
				comment=1;
				mode=9;
			}
			else if(!isspace(offchar))
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected data after the version (line %i)\n",line);
				return -1;
			}
			else
				off++;
		}
		else if(mode==9)
		{
			//9: search for VAR
			if(!memcmp("VAR",offchar_ptr,sizeof("VAR")-1))
			{
				//found version
				mode=10;
				off+=sizeof("VAR")-1;
				if(!isspace(offchar) || offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after VAR (line %i)\n",line);
					return -1;
				}
			}
			else if(offchar=='#')
			{
				comment=1;
			}
			else if(isspace(offchar))
			{
				int newline=0;
				if(offchar=='\r')
				{
					off++;
					newline=1;
				}
				if(offchar=='\n')
				{
					off++;
					newline=1;
				}
				if(newline)
				{
					line++;
				}
				else
					off++;
			}
			else if(offchar=='}')
			{

				def->content.vars=(t_sdl_var *)malloc(sizeof(t_sdl_var)*varcount);
				var=&sdl_vars;

				tmp_sdl_vars *tmp_var;
				for(int i=0; i<varcount; i++)
				{
					memcpy(&def->content.vars[i],&var->content,sizeof(t_sdl_var));
					tmp_var=var;
					var=var->next;
					if(tmp_var!=&sdl_vars)
						free(tmp_var);
				}
				if(var!=&sdl_vars)
					free(var);


				var=&sdl_vars;

				memset(var,0,sizeof(tmp_sdl_vars));

				def->content.n_vars=varcount;
				varcount=0;

				def->next=(tmp_sdl_defs *)malloc(sizeof(tmp_sdl_defs));
				def=def->next;

				defcount++;

				memset(def,0,sizeof(tmp_sdl_defs));

				mode=0;

				off++;
			}
			else
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected data in a STATEDESC block (line %i)\n",line);
				return -1;
			}
		}
		else if(mode==10)
		{
			//10: search for var type
			if(offchar=='\n' || offchar=='\r')
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected line break after VAR (lines %i/%i)\n",line,line+1);
				return -1;
			}

			if(!isspace(offchar))
			{
				startpos=off;
				mode=11;
			}
			off++;
		}
		else if(mode==11)
		{
			//11: read var type
			if(isspace(offchar))
			{
				if(offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected line break directly after the var type (lines %i/%i)\n",line,line+1);
					return -1;
				}

				endpos=off;

				memcpy(&tmpbuf,buf+startpos,endpos-startpos);
				tmpbuf[endpos-startpos]=0;

				int type;
				if(tmpbuf[0]=='$')
				{
					type=0x05;
					strcpy((char *)&var->content.struct_name,(char *)&tmpbuf+1);
					var->content.struct_version=0;
				}
				else
					type=sdl_get_var_type_from_name((char *)&tmpbuf);

				if(type<0)
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unknown type \"%s\" specified (line %i)\n",(char *)&tmpbuf,line);
					return -1;
				}
				var->content.type=type;

				mode=12;
			}
			else
				off++;
		}
		else if(mode==12)
		{
			//12: search var name

			if(offchar=='\n' || offchar=='\r')
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected line break after the var type (lines %i/%i)\n",line,line+1);
				return -1;
			}

			if(!isspace(offchar))
			{
				startpos=off;
				mode=13;
			}
			off++;
		}
		else if(mode==13)
		{
			//13... argh - the number of misfortune ;-)
			//13: search end of var name

			if(offchar=='[')
			{
				endpos=off;

				memcpy(&var->content.name,buf+startpos,endpos-startpos);
				var->content.name[endpos-startpos]=0;

				startpos=++off;

				mode=14;
			}
			else if(isspace(offchar))
			{
				//argh, should NOT happen
				if(offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected line break between the var name and the expected tuple item count (lines %i/%i)\n",line,line+1);
				}
				else
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected blank between the var name and the expected tuple item count (line %i)\n",line);
				}
				return -1;
			}
			else
				off++;
		}
		else if(mode==14)
		{
			//14: find closing ]
			if(isspace(offchar))
			{
				//argh, should NOT happen
				if(offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected line break in the tuple item count (lines %i/%i)\n",line,line+1);
				}
				else
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected blank in the tuple item count (line %i)\n",line);
				}
				return -1;
			}

			if(offchar==']')
			{
				endpos=off;

				if(endpos-startpos>0)
				{
					memcpy(&tmpbuf,buf+startpos,endpos-startpos);
					tmpbuf[endpos-startpos]=0;

					var->content.array_size=atoi((char *)&tmpbuf);
				}
				else
				{
					var->content.array_size=0;
				}

				off++;

				if(!isspace(offchar))
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after the closing ] (line %i)\n",line);
					return -1;
				}

				mode=15;
			}
			else
				off++;
		}
		else if(mode==15)
		{
			//15: find additional information (DEFAULT, DEFAULTOPTION, DISPLAYOPTION, INTERNAL, PHASED)
			//    or search for a line break

			if(!memcmp("PHASED",offchar_ptr,sizeof("PHASED")-1))
			{
				off+=sizeof("PHASED")-1;
				if(offchar=='\n' || offchar=='\r' || !isspace(offchar))
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after PHASED (line %i)\n",line);
					return -1;
				}
				var->content.default_option|=2; //VAULT flag
			}
			else if(!memcmp("INTERNAL",offchar_ptr,sizeof("INTERNAL")-1))
			{
				off+=sizeof("INTERNAL")-1;
				if(offchar=='\n' || offchar=='\r' || !isspace(offchar))
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after INTERNAL (line %i)\n",line);
					return -1;
				}
				var->content.default_option|=1; //hidden flag
			}
			else if(!memcmp("DISPLAYOPTION",offchar_ptr,sizeof("DISPLAYOPTION")-1))
			{
				mode=36;
				off+=sizeof("DISPLAYOPTION")-1;
				if(offchar=='\n' || offchar=='\r' || (!isspace(offchar) && offchar!='='))
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after DISPLAYOPTION (line %i)\n",line);
					return -1;
				}
			}
			else if(!memcmp("DEFAULTOPTION",offchar_ptr,sizeof("DEFAULTOPTION")-1))
			{
				mode=26;
				off+=sizeof("DEFAULTOPTION")-1;
				if(offchar=='\n' || offchar=='\r' || (!isspace(offchar) && offchar!='='))
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after DEFAULTOPTION (line %i)\n",line);
					return -1;
				}
			}
			else if(!memcmp("DEFAULT",offchar_ptr,sizeof("DEFAULT")-1))
			{
				mode=16;
				off+=sizeof("DEFAULT")-1;
				if(offchar=='\n' || offchar=='\r' || (!isspace(offchar) && offchar!='='))
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected data directly after DEFAULT (line %i)\n",line);
					return -1;
				}
			}
			else if(offchar=='#' || offchar=='\r' || offchar=='\n')
			{
				switch(var->content.type) {
					case 55: //RGB8
						var->content.n_itms=3;
						var->content.itm_type=9;
						break;
					case 50: //POINT3
					case 51: //VECTOR3
						var->content.n_itms=3;
						var->content.itm_type=1;
						break;
					case 54: //QUATERNION
						var->content.n_itms=4;
						var->content.itm_type=1;
						break;
					default:
						var->content.n_itms=1;
						var->content.itm_type=var->content.type;
				}

				//now check if it is an struct
				if(var->content.struct_name[0]!=0)
				{
					var->content.flag=0x01;
					var->content.default_option|=4;
				}

				var->content.static1=0x03;
				var->content.u16k1=0x00;
				var->content.u16k2=0x00;


				var->next=(tmp_sdl_vars *)malloc(sizeof(tmp_sdl_vars));
				var=var->next;
				varcount++;

				memset(var,0,sizeof(tmp_sdl_vars));



				mode=9;
				if(offchar=='#')
				{
					comment=1;
					continue;
				}
				if(offchar=='\r')
					off++;
				if(offchar=='\n')
					off++;

				line++;

			}
			else if(isspace(offchar))
			{
				off++;
			}
			else
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected data directly after a var definition (line %i)\n",line);
				return -1;
			}
		}
		//----------------------------------------------------------------------------------------------
		else if(mode==16)
		{
			//16: find equal sign after DEFAULT
			if(offchar=='=')
			{
				mode=17;
				off++;
			}
			else if(isspace(offchar))
			{
				if(offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected line break after DEFAULT (lines %i/%i)\n",line,line+1);
					return -1;
				}

				off++;
			}
			else
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected data directly after DEFAULT (line %i)\n",line);
				return -1;
			}
		}
		else if(mode==17)
		{
			//17: find default data start

			//!TODO: add the ability to parse default strings and vectors
			//format: (value[,value])
			//    or: [(]value[)]

			if(offchar=='\n' || offchar=='\r')
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected line break after DEFAULT (lines %i/%i)\n",line,line+1);
				return -1;
			}

			if(!isspace(offchar))
			{
				startpos=off;
				mode=18;
			}
			off++;
		}
		else if(mode==18)
		{
			//18: find end of default data
			if(isspace(offchar))
			{
				endpos=off;
				memcpy(&var->content.default_value,buf+startpos,endpos-startpos);
				var->content.default_value[endpos-startpos]=0;
				mode=15;
			}
			else
				off++;
		}
		//----------------------------------------------------------------------------------------------
		else if(mode==26)
		{
			//26: find end ofsign after DEFAULTOPTION
			if(offchar=='=')
			{
				mode=27;
				off++;
			}
			else if(isspace(offchar))
			{
				if(offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected line break after DEFAULTOPTION (lines %i/%i)\n",line,line+1);
					return -1;
				}

				off++;
			}
			else
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected data directly after DEFAULTOPTION (line %i)\n",line);
				return -1;
			}
		}
		else if(mode==27)
		{
			//27: find default option data start

			if(offchar=='\n' || offchar=='\r')
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected line break after DEFAULTOPTION (lines %i/%i)\n",line,line+1);
				return -1;
			}

			if(!isspace(offchar))
			{
				startpos=off;
				mode=28;
			}
			off++;
		}
		else if(mode==28)
		{
			//28: find end of default option data
			if(isspace(offchar))
			{
				endpos=off;
				memcpy(&tmpbuf,buf+startpos,endpos-startpos);
				tmpbuf[endpos-startpos]=0;

				if(!strcmp((char *)&tmpbuf,"VAULT"))
					var->content.default_option|=2; //VAULT flag
					
				mode=15;
			}
			else
				off++;
		}
		//----------------------------------------------------------------------------------------------
		else if(mode==36)
		{
			//36: find equal sign after DISPLAYOPTION
			if(offchar=='=')
			{
				mode=37;
				off++;
			}
			else if(isspace(offchar))
			{
				if(offchar=='\n' || offchar=='\r')
				{
					print2log(f,sdl_reading_error);
					print2log(f,"unexpected line break after DEFAULTOPTION (lines %i/%i)\n",line,line+1);
					return -1;
				}

				off++;
			}
			else
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected data directly after DEFAULTOPTION (line %i)\n",line);
				return -1;
			}
		}
		else if(mode==37)
		{
			//37: find display option data start

			if(offchar=='\n' || offchar=='\r')
			{
				print2log(f,sdl_reading_error);
				print2log(f,"unexpected line break after DEFAULTOPTION (lines %i/%i)\n",line,line+1);
				return -1;
			}

			if(!isspace(offchar))
			{
				startpos=off;
				mode=38;
			}
			off++;
		}
		else if(mode==38)
		{
			//38: find end of display option data
			if(isspace(offchar))
			{
				endpos=off;
				memcpy(&tmpbuf,buf+startpos,endpos-startpos);
				tmpbuf[endpos-startpos]=0;

				if(!strcmp((char *)&tmpbuf,"hidden"))
					var->content.default_option|=1; //hidden flag
				else
					strcpy((char *)&var->content.display_options,(char *)&tmpbuf);
					
				mode=15;
			}
			else
				off++;
		}
	};

	t_sdl_def *insertptr;

	if(*n_sdl==0)
	{
		insertptr=*sdl2=(t_sdl_def *)malloc(sizeof(t_sdl_def)*defcount);
	}
	else
	{
		insertptr=*sdl2=(t_sdl_def *)realloc(*sdl2,sizeof(t_sdl_def)*(*n_sdl+defcount));
		insertptr=&insertptr[*n_sdl];
	}

	def=&sdl_defs;

	tmp_sdl_defs *tmp_def;
	for(int i=0; i<defcount; i++)
	{
		memcpy(&insertptr[i],&def->content,sizeof(t_sdl_def));
		tmp_def=def;
		def=def->next;
		if(tmp_def!=&sdl_defs)
			free(tmp_def);
	}
	if(def!=&sdl_defs)
		free(def);

	(*n_sdl)+=defcount;

	return 0;
}