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

#ifndef __U_AGE_PARSER_
#define __U_AGE_PARSER_
#define __U_AGE_PARSER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>

#define DEBUG

#include "data_types.h" //for U32,Byte and others
#include "files.h" //to open close files into a buffer (nice eh!)
#include "conv_funs.h"

#include "ageparser.h"

//parse out age structures


void init_age_def(t_age_def * age) {
	bzero(age,sizeof(t_age_def));
	age->page=NULL;
}

void destroy_age_def(t_age_def * age) {
	if(age!=NULL) {
		if(age->page!=NULL) {
			free((void *)age->page);
		}
		free((void *)age);
	}
}

int age_add_page(char * right_buffer,t_page_def ** page,S32 * n_pages) {

	if(*page==NULL) {
		//printf("initial assigment\n"); fflush(0);
		(*n_pages)=1;
		*page=(t_page_def *)malloc(sizeof(t_page_def) * *n_pages);
	} else {
		//printf("n_pages is %i\n",*n_pages);
		(*n_pages)++;
		*page=(t_page_def *)realloc((void *)*page,sizeof(t_page_def) * *n_pages);
	}
	if(*page==NULL) {
		fprintf(stderr,"FATAL, not enough memory!!!\n");
		abort();
		return -1;
	}

	Byte q; //machine status
	char c;
	char buffer[255];
	int i,n,t;

	n=strlen(right_buffer);

	t=0;
	q=0;

	for(i=0; i<n; i++) {
		c=right_buffer[i];
		//printf("[%c](%i,%i)\n",c,i,q);

		if(c!=',') {
			buffer[t]=c;
			t++;
		} else {
			buffer[t]='\0';
			if(q==0) {
				strcpy((char *)(*page)[(*n_pages)-1].name,buffer);
			} else if(q==1) {
				(*page)[(*n_pages)-1].id=(U16)atoi(buffer);
			} else {
				(*page)[(*n_pages)-1].sdl=(Byte)atoi(buffer);
			}
			q++;
			t=0;
		}

		if(q==0) {
			strcpy((char *)(*page)[(*n_pages)-1].name,buffer);
			(*page)[(*n_pages)-1].id=0;
			(*page)[(*n_pages)-1].sdl=0;
		} else if(q==1) {
			(*page)[(*n_pages)-1].id=(U16)atoi(buffer);
			(*page)[(*n_pages)-1].sdl=0;
		} else {
			(*page)[(*n_pages)-1].sdl=(Byte)atoi(buffer);
		}

	}

	//so that was some type of weird turing machine..
	return 0;

}

int parse_age_descriptor(FILE * dsc,char * buf,int size,t_age_def * age) {
	int c; //current_char
	char left_buffer[1024]; //Max size of a variable
	char right_buffer[1024]; //Max size of a value
	int sizeA=0;
	int sizeB=0;
	int i,n; //an iterator
	int l; //line counter

	Byte comment=0; //commentary
	Byte mode=0; //parse mode 0 none, 1 left, 2 mid, 3 right, 4 end
	Byte quote=0; //quote mode
	Byte slash=0; //slash mode
	Byte win=0; //windoze mode

	int parse_error=0; //there was a parse error?

	//parse
	i=0; l=1; n=0;
	while(n<size) {
		c=buf[n];
		//printf("[%c](%i)",c,mode); fflush(0);
		if(c=='#' && quote==0) {
			if(mode==1 || mode==2 || mode==3) {
				fprintf(dsc,"ERR: Parse error at line %i, unexpected \"#\"",l);
				parse_error=1;
			}
			comment=1;  //activate comment flag
			win=0;
		}
		else if(c=='\n' || c=='\r') {
			comment=0;
			if(mode==1 || mode==2) {
				fprintf(dsc,"ERR: Parse error at line %i, missing right member\n",l);
				parse_error=1;
			}
			if(c=='\r') {
				win=1;
				l++;
			}
			if(c=='\n' && win==0) {
				l++;
				win=0;
			}
			if((mode==3 && quote==0) || mode==4) {
				sizeB=i;
				i=0;
				mode=0;
			}
		}
		else if(comment==0 && c=='\"' && slash==0) {
			if(quote==1) { quote=0; mode=4; }
			else if(mode==2) { mode=3; quote=1; }
			else {
				fprintf(dsc,"ERR: Parse error at line %i, unexpected '\"'\n",l);
				parse_error=1;
			}
			win=0;
		}
		else if(slash==1) {
			slash=0;
			if(c=='n') { right_buffer[i]='\n'; }
			else { right_buffer[i]=c; }
			i++;
			win=0;
		}
		else if(c=='\\' && slash==0) {
			if(quote==1) {
				slash=1;
			} else {
				fprintf(dsc,"ERR: Parse error at line %i, unexpected '\\'\n",l);
				parse_error=1;
			}
			win=0;
		}
		else if(comment==0 && quote==0 && (c==' ' || c=='=' || isblank(c))) {
			if(mode==1) {
				mode=2;
				sizeA=i;
				i=0;
			}
			else if(mode==3) {
				if(c=='=') {
					fprintf(dsc,"ERR: Parse error at line %i, unexpected \"=\"\n",l);
					parse_error=1;
				}
				mode=4;
			}
			win=0;
		} else if(isalpha(c) && comment==0 && mode==0) { //first character of a config directive
			if(i>=1024) {
				fprintf(dsc,"ERR: Parse error at line %i, line too long\n",l);
				parse_error=1;
			}
			left_buffer[i]=tolower(c);
			i++;
			mode=1;
			win=0;
			//printf("<mode-is-now-%i>",mode);
		} else if(isprint(c) && comment==0 && (mode==1 || mode==2 || mode==3)) {
			if(i>=1024) {
				fprintf(dsc,"ERR: Parse error at line %i, line too long\n",l);
				parse_error=1;
			}
			if(mode==1) {
				left_buffer[i]=tolower(c);
			}
			else if(mode==3 || mode==2) {
				mode=3;
				right_buffer[i]=c;
			}
			i++;
			win=0;
		} else if(comment==0 && n<size) {
			printf("n:%isize:%i\n",n,size); fflush(0);
			fprintf(dsc,"ERR: Parse error at line %i, unexpected character '%c'\n",l,c);
			parse_error=1;
		}
		n++;

		if(mode==0 && sizeA!=0) {
			left_buffer[sizeA]='\0';
			right_buffer[sizeB]='\0';

			if(!strcmp(left_buffer,"startdatetime")) {
				if(!isdigit(right_buffer[0])) {
					fprintf(dsc,"ERR: Parse error at line %i, invalid type %s\n",l-1,right_buffer);
					parse_error=1;
				} else {
					age->StartDateTime = atoi(right_buffer);
				}
			} else if(!strcmp(left_buffer,"daylength")) {
				if(!isdigit(right_buffer[0])) {
					fprintf(dsc,"ERR: Parse error at line %i, invalid type %s\n",l-1,right_buffer);
					parse_error=1;
				} else {
					age->DayLength= (float)atof(right_buffer);
				}
			} else if(!strcmp(left_buffer,"maxcapacity")) {
				if(!isdigit(right_buffer[0])) {
					fprintf(dsc,"ERR: Parse error at line %i, invalid type %s\n",l-1,right_buffer);
					parse_error=1;
				} else {
					age->MaxCapacity= (U16)atoi(right_buffer);
				}
			} else if(!strcmp(left_buffer,"lingertime")) {
				if(!isdigit(right_buffer[0])) {
					fprintf(dsc,"ERR: Parse error at line %i, invalid type %s\n",l-1,right_buffer);
					parse_error=1;
				} else {
					age->LingerTime= (U16)atoi(right_buffer);
				}
			} else if(!strcmp(left_buffer,"sequenceprefix")) {
				if(!isdigit(right_buffer[0]) && (right_buffer[0]!='-')) {
					fprintf(dsc,"ERR: Parse error at line %i, invalid type %s\n",l-1,right_buffer);
					parse_error=1;
				} else {
					age->SequencePrefix= (U16)atoi(right_buffer);
				}
			} else if(!strcmp(left_buffer,"releaseversion")) {
				if(!isdigit(right_buffer[0])) {
					fprintf(dsc,"ERR: Parse error at line %i, invalid type %s\n",l-1,right_buffer);
					parse_error=1;
				} else {
					age->ReleaseVersion= (U16)atoi(right_buffer);
				}
			} else if(!strcmp(left_buffer,"page")) {
				if(age_add_page(right_buffer,&age->page,&age->n_pages)<0) {
					fprintf(dsc,"ERR: Parse error at line %i, invalid syntax\n",l-1);
					parse_error=1;
				}
			} else {
				fprintf(dsc,"ERR: Unknown configuration directive %s at line %i\n",left_buffer,l-1);
				parse_error=1;
			}
			sizeA=0;
			sizeB=0;
		}
	}

	if(parse_error==1) {
		return -1;
	}

	return (0);
}

int read_age_descriptor(FILE * f,char * address,t_age_def ** p_age) {

	t_age_def * age=NULL;
	char * buf=NULL;
	int filesize;
	int ret=0;

	*p_age=(t_age_def *)malloc(sizeof(t_age_def) * 1);
	if(*p_age==NULL) {
		fprintf(f,"ERR: Not enought memory!\n");
		return -1;
	}
	age=*p_age;
	init_age_def(age);

	filesize=readWDYS(&buf,address);
	if(filesize<=0) {
		filesize=readfile(&buf,address);
	}
	if(filesize<=0) {
		ret=-2;
	}

	if(ret==0) {
		ret=parse_age_descriptor(stdout,buf,filesize,age);
	}

	if(buf!=NULL) {
		free((void *)buf);
	}

	return ret;
}

int read_all_age_descriptors(FILE * f,char * address,t_age_def ** p_age) {

	t_age_def * age=NULL;
	char * buf=NULL;
	int filesize;
	int ret=0;

	DIR *dir;
	struct dirent *entry;

	char loopy[500];
	int n_ages=0;

	if((dir=opendir(address))== NULL) {
		fprintf(f,"ERR: Opening %s\n",address);
		perror("opendir()");
		return -1;
	} else {
		while((entry=readdir(dir))!= NULL) {
			if(!strcasecmp(entry->d_name+(strlen(entry->d_name)-4),".age")) {
				fprintf(f,"Reading %s...\n",entry->d_name);
				sprintf(loopy,"%s/%s",address,entry->d_name);

				n_ages++;
				if(*p_age==NULL) {
					*p_age=(t_age_def *)malloc(sizeof(t_age_def) * n_ages);
				} else {
					*p_age=(t_age_def *)realloc((void *)*p_age,sizeof(t_age_def) * n_ages);
				}
				if(*p_age==NULL) {
					fprintf(f,"ERR: Not enought memory!\n");
					return -1;
				}
				age=&(*p_age)[n_ages-1];
				init_age_def(age);

				strncpy((char *)age->name,entry->d_name,strlen(entry->d_name)-4);

				filesize=readWDYS(&buf,loopy);
				if(filesize<=0) {
					filesize=readfile(&buf,loopy);
				}
				if(filesize<=0) {
					ret=-2;
				}

				if(ret==0) {
					ret=parse_age_descriptor(stdout,buf,filesize,age);
				}

				if(buf!=NULL) {
					free((void *)buf);
				}

				dump_age_descriptor(f,*age);

				if(ret<0) {
					fprintf(f,"ERR: A problem ocurred reading %s\n",loopy);
					return -1;
				}
			}
		}
		closedir(dir);
	}

	return n_ages;
}


void dump_age_descriptor(FILE * f,t_age_def age) {
	fprintf(f,"StartDateTime=%i\n",age.StartDateTime);
	fprintf(f,"DayLength=%f\n",age.DayLength);
	fprintf(f,"MaxCapacity=%i\n",age.MaxCapacity);
	fprintf(f,"LingerTime=%i\n",age.LingerTime);
	fprintf(f,"SequencePrefix=%i\n",age.SequencePrefix);
	fprintf(f,"ReleaseVersion=%i\n",age.ReleaseVersion);

	int i;
	for(i=0; i<age.n_pages; i++) {
		fprintf(f,"Page=%s,%i,%i\n",age.page[i].name,age.page[i].id,age.page[i].sdl);
	}

}


#ifdef _AGE_PARSER_TEST_

int main(int argc, char * argv[]) {

	DIR *dir;
	struct dirent *entry;
	char loopy[500];

	if(argc!=2) {
		printf("Usage %s age_dir\n",argv[0]);
		return -1;
	}

	t_age_def * age=NULL; //The age struct

	if((dir=opendir(argv[1]))== NULL) {
		fprintf(stderr,"ERR: Opening %s\n",argv[1]);
		perror("opendir()");
		return -1;
	} else {
		while((entry=readdir(dir))!= NULL) {
			if(!strcasecmp(entry->d_name+(strlen(entry->d_name)-4),".age")) {
				printf("Reading %s...\n",entry->d_name);
				sprintf(loopy,"%s/%s",argv[1],entry->d_name);

				read_age_descriptor(stdout,loopy,&age);

				dump_age_descriptor(stdout,*age);

				if(age!=NULL) {
					destroy_age_def(age);
				}
			}
		}
		closedir(dir);
	}
}

#endif

#endif
