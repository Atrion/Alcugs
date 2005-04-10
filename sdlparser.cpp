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
#include "debug.h"


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>


#define DEBUG

#include "data_types.h" //for U32,Byte and others
#include "files.h" //to open close files into a buffer (nice eh!)
#include "conv_funs.h"
#include "adv_gamemsgparser.h"
#include "stdebug.h"

#include "sdlparser.h"

//Utility to uncompress/decompress uru files (works!)

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

char * get_var_type(Byte type) {
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

char * get_var_type_nspc(Byte type) {
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

int get_var_type_from_name(char * buf) {
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

//returns the size if all goes OK
//int stream_sdl_binary_data(FILE * f, char * buf,t_sdl_var * var


int sdl_parse_default_options(FILE * f,char * buf,t_sdl_var * var) {

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
				fprintf(f,"ERR: Parse error line %i, unexpected character '='\n",l);
				return -1;
			}
		}
		else if((c==' ' || isblank(c))) {
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
			fprintf(f,"ERR: Parse error at line %i, unexpected character\n",l);
			return -1;
		}

		//do more stuff
		if(mode==0 && sizeA!=0) {
			bufferA[sizeA]='\0';
			bufferB[sizeB]='\0';

			if(!strcmp(bufferA,"DEFAULT")) {
				if(default_parsed==1) {
					fprintf(f,"ERR: An SDL record can't contain more than one default value!\n");
					return -1;
				}
				default_parsed=1;
				strcpy((char *)var->default_value,bufferB);
			}
			else if(!strcmp(bufferA,"DEFAULTOPTION")) {
				if(default_option_parsed==1) {
					fprintf(f,"ERR: An SDL record can't contain more than one defaultoption value!\n");
					return -1;
				}
				default_option_parsed=1;
				if(!strcmp(bufferB,"VAULT")) {
					var->default_option=var->default_option | 2;
				} else {
					fprintf(f,"ERR: Unexpected DEFAULTOPTION!\n");
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

//to compile an sdl (adds, or creates items into the sdl2)

int sdl_compile(FILE * f,Byte * buf,int totalsize,t_sdl_def * sdl) {

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
				fprintf(f,"ERR: Parse error at line %i, unexpected \"}\"",l);
				return -1;
			} else {
				end=1;
			}
		}
		else if(c=='#') {
			if(mode==1 || mode==2 || mode==3 || ((mode==4 || mode==5) && version==1)) {
				fprintf(f,"ERR: Parse error at line %i, unexpected \"#\"",l);
				return -1;
			}
			comment=1; //activate flag
			win=0; //deactivate win flag
			mode=0;
		}
		else if(c=='\n' || c=='\r') {
			comment=0;
			if(mode==1 || mode==2 || ((mode==3 || mode==4) && version==1)) {
				fprintf(f,"ERR: Parse error at line %i, missing right member\n",l);
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
				fprintf(f,"ERR: Parse error line %i, is too long\n",l);
				return -1;
			}
			bufferA[i]=c;
			i++;
			mode=1;
		}
		else if(isprint(c) && comment==0 && (mode==1 || mode==2 || mode==3 || mode==4 || mode==5 || mode==6 || mode==7)) {
			if(i>=1024) {
				fprintf(f,"ERR: Parse error line %i, is too long\n",l);
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
			fprintf(f,"ERR: Parse error at line %i, unexpected character\n",l);
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
					fprintf(f,"ERR: Missing version number\n");
					return -1;
				}
				version=1;
				sdl->version=atoi(bufferB);
				sdl->n_vars=0; //initialize to 0
				sdl->vars=NULL; //no me fio ni un pelo
			} else {
				if(strcmp(bufferA,"VAR")) {
					fprintf(f,"ERR: Unexpected string at the begginning.\n");
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
				sdl->vars[sdl_id].type=get_var_type_from_name(bufferB);

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
					fprintf(f,"ERR: Expected ']' not found.\n");
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
		fprintf(f,"ERR: Parse error at line %i, missing characters\n",l);
		return -1;
	}

	return off;
}

#if 0
			} else if(sdl[i].vars[e].flag==0x01) {
				sdl[i].vars[e].struct_version=*(U16 *)(buf+off);
				off+=2;
			}
#endif

int update_sdl_version_structs(FILE * f,t_sdl_def * sdl,int n_sdl) {

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

#if 0
int sdl_get_last_descriptors(FILE * f,t_sdl_def * sdl,int n_sdl,t_sdl_def ** osdl,int * on_sdl) {
	//i hope that the memcpy will work

	int i,e;
	int max,max_e;

	t_sdl_def * sdl2;

	for(i=0; i<n_sdl; i++) {
		max=0;
		//check for the last version
		for(e=0; e<*on_sdl; e++) {
			if(!strcmp((char *)sdl[i].name,(char *)sdl2[e].name) && max<=sdl2[e].version) {
				max=sdl[e].version;
				max_e=e;
			}
		}
		if(max==0) { //not found, create a new one
			if(*osdl==NULL) {
				*on_sdl++;
				*osdl=(t_sdl_def *)malloc(sizeof(t_sdl_def) * *on_sdl);
				sdl2=*osdl;
			} else {
				*on_sdl++;
				*osdl=(t_sdl_def *)realloc((void *)*osdl,sizeof(t_sdl_def) * *on_sdl);
				sdl2=*osdl;
			}
			memcpy(sdl2[*on_sdl-1]
			//TODO
		}


	}


}
#endif

int sdl_statedesc_reader(FILE * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl) {

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
				fprintf(f,"ERR: Parse error at line %i, unexpected \"{\"\n",l);
				return -1;
			}
		}
		else if(c=='#') {
			if(mode==1 || mode==2 || mode==3) {
				fprintf(f,"ERR: Parse error at line %i, unexpected \"#\"\n",l);
				return -1;
			}
			comment=1; //activate flag
			win=0; //deactivate win flag
			mode=0;
		}
		else if(c=='\n' || c=='\r') {
			comment=0;
			if(mode==1 || mode==2) {
				fprintf(f,"ERR: Parse error at line %i, missing right member\n",l);
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
				fprintf(f,"ERR: Parse error line %i, is too long\n",l);
				return -1;
			}
			if(waiting_key==1) {
				fprintf(f,"ERR: Parse error line %i, waiting for a {, got an strange character\n",l);
				return -1;
			}
			left_buffer[i]=c;
			i++;
			mode=1;
		}
		else if(isprint(c) && comment==0 && (mode==1 || mode==2 || mode==3)) {
			if(i>=1024) {
				fprintf(f,"ERR: Parse error line %i, is too long\n",l);
				return -1;
			}
			if(waiting_key==1) {
				fprintf(f,"ERR: Parse error line %i, waiting for a {, got an strange character\n",l);
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
			fprintf(f,"ERR: Parse error at line %i, unexpected character\n",l);
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
				fprintf(f,"ERR: Unknown descriptor type\n");
				return -1;
			}

			fprintf(f,"Reading %s\n",right_buffer);
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

	update_sdl_version_structs(f,sdl,*n_sdl);

	if(key==1 || waiting_key==1 || mode==1 || mode==2 || mode==3) {
		fprintf(f,"ERR: Parse error at line %i, missing characters\n",l);
		return -1;
	}
#ifdef DBG_P_PARSER
	printf("Statedesc reader finished\n"); fflush(0);
#endif

	return off;
}


int sdl_statedesc_streamer(FILE * f,Byte ** buf2,t_sdl_def * sdl,int n_sdl) {
	int off;

	FILE * ferr;
	ferr=stderr;

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
	fprintf(f,"\r\n# total number of sdl records: %i\r\n\r\n",n_sdl);

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
			fprintf(ferr,"Unexpected flag:%02X\n",sdl[i].flag);
			ok=-1;
			break;
		}
		off+=encode_urustring(buf+off,sdl[i].name,strlen((char *)sdl[i].name),0x01);
		fprintf(f,"STATEDESC %s\r\n{\r\n",sdl[i].name);

		*(U16 *)(buf+off)=sdl[i].version;
		off+=2;
		fprintf(f,"    VERSION %i\r\n",sdl[i].version);

		*(U16 *)(buf+off)=sdl[i].n_vars;
		off+=2;
		fprintf(f,"    # Num vars: %i\r\n",sdl[i].n_vars);

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
				fprintf(ferr,"Unexpected sub_flag:%02X\n",sdl[i].vars[e].flag);
				ok=-1;
				break;
			}
			*(Byte *)(buf+off)=sdl[i].vars[e].static1;
			off++;
			if(sdl[i].vars[e].static1!=0x03) {
				fprintf(ferr,"Unexpected static1:%02X\n",sdl[i].vars[e].static1);
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
				fprintf(ferr,"Unexpected u16k1:%04X\n",sdl[i].vars[e].u16k1);
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
				fprintf(ferr,"Unexpected u16k2:%04X\n",sdl[i].vars[e].u16k2);
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
				fprintf(f,"    #n_itms:%i,type:%i,%s\r\n",sdl[i].vars[e].n_itms,\
				sdl[i].vars[e].itm_type,get_var_type(sdl[i].vars[e].itm_type));
#endif
			}
			if(sdl[i].vars[e].default_option!=0 && sdl[i].vars[e].default_option!=2) {
#ifdef DEBUG
				fprintf(f,"    #defaultoption:%i\r\n",sdl[i].vars[e].default_option);
#endif
			}
			if(sdl[i].vars[e].flag==0x01) {
#ifdef DEBUG
				fprintf(f,"    #struct_version:%i\r\n",sdl[i].vars[e].struct_version);
#endif
			}
			if(sdl[i].vars[e].flag==0x00) {
				fprintf(f,"    VAR %s ",get_var_type(sdl[i].vars[e].type));
			} else if(sdl[i].vars[e].flag==0x01) {
				fprintf(f,"    VAR $%s",sdl[i].vars[e].struct_name);
			}
			fprintf(f,"  %s",sdl[i].vars[e].name);
			if(sdl[i].vars[e].array_size==0) {
				fprintf(f,"[]");
			} else {
				fprintf(f,"[%i]",sdl[i].vars[e].array_size);
			}

			if(strlen((char *)sdl[i].vars[e].default_value)>0) {
				fprintf(f,"  DEFAULT=%s",sdl[i].vars[e].default_value);
			}
			if(sdl[i].vars[e].default_option==2) {
				fprintf(f,"  DEFAULTOPTION=VAULT");
			}
			if(strlen((char *)sdl[i].vars[e].display_options)>0) {
				my_char=',';
				j=0;
				while(j<=(int)strlen((char *)sdl[i].vars[e].display_options)) {
					if(my_char==',') {
						fprintf(f," DISPLAYOPTION=");
					} else {
						fprintf(f,"%c",my_char);
					}
					my_char=sdl[i].vars[e].display_options[j];
					j++;
				}
			}

			fprintf(f,"\r\n");

			//break;
		}
		if(ok==-1) { break; }

		fprintf(f,"}\r\n\r\n");

		//break;

	}

	if(ok==-1) {
		fprintf(ferr,"Parse error at offset %i,%08X\n",off,off);
		return -1;
	}

	return off;
}

//to parse sdl files
int sdl_parse_contents(FILE * f,Byte * buf,int totalsize,t_sdl_def ** sdl2,int * n_sdl) {
	int off;

	FILE * ferr;
	ferr=stderr;

	off=0;

	t_sdl_def * sdl;

	U16 n_vars;
	int i,e,j;
	char my_char;

	int ok=0;

	//let's start
	n_vars=*(U16 *)(buf+off);
	off+=2;
	fprintf(f,"\r\n# total number of sdl records: %i\r\n\r\n",n_vars);

	*sdl2=(t_sdl_def *)malloc(sizeof(t_sdl_def) * *n_sdl);
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

	for(i=0; i<n_vars; i++) {
		sdl[i].flag=*(Byte *)(buf+off);
		off++;
		if(sdl[i].flag!=0x01) {
			fprintf(ferr,"Unexpected flag:%02X\n",sdl[i].flag);
			ok=-1;
			break;
		}
		off+=decode_urustring(sdl[i].name,buf+off,SSTR);
		off+=2;
		fprintf(f,"STATEDESC %s\r\n{\r\n",sdl[i].name);

		sdl[i].version=*(U16 *)(buf+off);
		off+=2;
		fprintf(f,"    VERSION %i\r\n",sdl[i].version);

		sdl[i].n_vars=*(U16 *)(buf+off);
		off+=2;
		fprintf(f,"    # Num vars: %i\r\n",sdl[i].n_vars);

		sdl[i].vars=(t_sdl_var *)malloc(sizeof(t_sdl_var) * sdl[i].n_vars);

		for(e=0; e<sdl[i].n_vars; e++) {
			sdl[i].vars[e].flag=*(Byte *)(buf+off);
			off++;
			if(sdl[i].vars[e].flag!=0x00 && sdl[i].vars[e].flag!=0x01) {
				fprintf(ferr,"Unexpected sub_flag:%02X\n",sdl[i].vars[e].flag);
				ok=-1;
				break;
			}
			sdl[i].vars[e].static1=*(Byte *)(buf+off);
			off++;
			if(sdl[i].vars[e].static1!=0x03) {
				fprintf(ferr,"Unexpected static1:%02X\n",sdl[i].vars[e].static1);
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
				fprintf(ferr,"Unexpected u16k1:%04X\n",sdl[i].vars[e].u16k1);
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
				fprintf(ferr,"Unexpected u16k2:%04X\n",sdl[i].vars[e].u16k2);
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
				fprintf(f,"    #n_itms:%i,type:%i,%s\r\n",sdl[i].vars[e].n_itms,\
				sdl[i].vars[e].itm_type,get_var_type(sdl[i].vars[e].itm_type));
#endif
			}
			if(sdl[i].vars[e].default_option!=0 && sdl[i].vars[e].default_option!=2) {
#ifdef DEBUG
				fprintf(f,"    #defaultoption:%i\r\n",sdl[i].vars[e].default_option);
#endif
			}
			if(sdl[i].vars[e].flag==0x01) {
#ifdef DEBUG
				fprintf(f,"    #struct_version:%i\r\n",sdl[i].vars[e].struct_version);
#endif
			}
			if(sdl[i].vars[e].flag==0x00) {
				fprintf(f,"    VAR %s ",get_var_type(sdl[i].vars[e].type));
			} else if(sdl[i].vars[e].flag==0x01) {
				fprintf(f,"    VAR $%s",sdl[i].vars[e].struct_name);
			}
			fprintf(f,"  %s",sdl[i].vars[e].name);
			if(sdl[i].vars[e].array_size==0) {
				fprintf(f,"[]");
			} else {
				fprintf(f,"[%i]",sdl[i].vars[e].array_size);
			}

			if(strlen((char *)sdl[i].vars[e].default_value)>0) {
				fprintf(f,"  DEFAULT=%s",sdl[i].vars[e].default_value);
			}
			if(sdl[i].vars[e].default_option==2) {
				fprintf(f,"  DEFAULTOPTION=VAULT");
			}
			if(strlen((char *)sdl[i].vars[e].display_options)>0) {
				my_char=',';
				j=0;
				while(j<=(int)strlen((char *)sdl[i].vars[e].display_options)) {
					if(my_char==',') {
						fprintf(f," DISPLAYOPTION=");
					} else {
						fprintf(f,"%c",my_char);
					}
					my_char=sdl[i].vars[e].display_options[j];
					j++;
				}
			}

			fprintf(f,"\r\n");

			//break;
		}
		if(ok==-1) { break; }

		fprintf(f,"}\r\n\r\n");

		//break;

	}

	if(ok==-1) {
		fprintf(ferr,"Parse error at offset %i,%08X\n",off,off);
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


int read_sdl_files(char * address,t_sdl_def ** sdl,int * n_sdl) {

	DIR *dir;
	struct dirent *entry;
	int totalsize,off;
	Byte * buf=NULL;

	char loopy[500];

	if((dir=opendir(address))== NULL) {
		print2log(f_err,"ERR: Opening %s\n",address);
		perror("opendir()");
		return -1;
	} else {
		while((entry=readdir(dir))!= NULL) {
			if(!strcasecmp(entry->d_name+(strlen(entry->d_name)-4),".sdl")) {
				print2log(f_sdl,"Reading %s...\n",entry->d_name);
				sprintf(loopy,"%s/%s",address,entry->d_name);
				totalsize=readWDYS((char **)&buf,loopy);
				if(totalsize<0) {
					totalsize=readfile((char **)&buf,loopy);
				}
				if(totalsize<0) {
					fprintf(f_err,"ERR: A problem ocurred reading %s\n",loopy);
					return -1;
				}
				off=0;
				if(sdl_statedesc_reader(f_sdl,buf+off,totalsize-off,sdl,n_sdl)<0) {
					fprintf(f_err,"ERR: SDL error in statedesc reader call, Something has gone wrong, head for the cover!\n");
					return -1;
				}
				if(buf!=NULL) {
					free((void *)buf);
				}
			}
		}
		closedir(dir);
	}
	//dump the SDL binary info
	Byte * dbuf=NULL;
	totalsize=sdl_statedesc_streamer(f_sdl,&dbuf,*sdl,*n_sdl);
	mkdir("dumps",0700);
	savefile((char *)dbuf,totalsize,"dumps/sdl_dump.raw");

	return 1;
}

#endif
