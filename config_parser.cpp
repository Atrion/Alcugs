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

/**********************************************
  For parsing the configuration URU files
*************************************************/

#ifndef __U_CONFIG_PARSER_
#define __U_CONFIG_PARSER_
#define __U_CONFIG_PARSER_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "config_parser.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// We don't want sockets, network, protocol, and nothing else here, it's only a parser
// and it should be a parser

//configuration variables moved to tmp_config.cpp and tmp_config.h, since the new
// netcore is not using them.

/** Handle nicely foreign languages. setlocale() is not sufficient on systems which didn't installed all locales,
 so we better use our own function. Feel free to add other accents according to your language.
*/
long my_isalpha(int c) {
  return((long)index("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ����\
����������������", c));
}

/**
	Parse the configuration file(s)
	Returns -4 terrible error, execution should stop
	Returns -3 if it fails to open the file
	Returns -2 if a parser error happens
	Returns -1 if a warning ocurred
	Retruns 0 if success
	conf should be NULL at first time, the function can
	be called several times, and will override any previous
	value(s)
*/
int read_config(FILE * dsc, char * conf,st_config ** cfg2, char ** allowed, int nall) {
	FILE * f;

	st_config * cfg=*cfg2;

	int c; //current_char
	char left_buffer[1024]; //Max size of a variable
	char right_buffer[1024]; //Max size of a value
	int sizeA=0;
	int sizeB=0;

	int i,e; //an iterator
	int l; //line counter

	Byte comment=0; //commentary
	Byte mode=0; //parse mode 0 none, 1 left, 2 mid, 3 right, 4 end
	Byte quote=0; //quote mode
	Byte slash=0; //slash mode
	Byte win=0; //windoze mode
	Byte check=0; //check if valid
	Byte key=0; //are we in key mode?

	Byte store_key=0; //store the key?
	Byte first_key_done=0; //we have a current key?

	int found_key=0;
	int current_key=0; //the current key (default global)

	int parse_error=0; //there was a parse error?
	int warning=0; //there was a warning?

	int ret=0;

	f=fopen((const char *)conf,"r");
	if(f==NULL) {
		return -3;
	}

	//parse
	i=0; l=1;
	while(!feof(f)) {
		c=fgetc(f);
		//printf("%c%i",c,mode); fflush(0);
		if(c=='#' && quote==0) {
			if(mode==1 || mode==2 || mode==3) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"#\"",conf,l);
				parse_error=1;
			}
			comment=1;  //activate comment flag
			win=0;
		}
		else if(c=='[' && quote==0 && comment==0) {
			if(mode==1 || mode==2 || mode==3 || key==1) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"[\"",conf,l);
				parse_error=1;
			}
			key=1; //set key flag
			win=0;
			i=0;
		}
		else if(c==']' && quote==0 && comment==0) {
			if(mode==1 || mode==2 || mode==3 || key==0) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"]\"",conf,l);
				parse_error=1;
			}
			key=0; //unset key flag
			win=0;
			left_buffer[i]='\0';
			sizeA=i;
			i=0;
			store_key=1;
			mode=0;
		}
		else if(key==1) {
			left_buffer[i]=tolower(c);
			i++;
		}
		else if(c=='\n' || c=='\r') {
			comment=0;
			if(mode==1 || mode==2) {
				fprintf(dsc,"WAR: Parse error reading %s at line %i, missing right member\n",conf,l);
				warning=1;
				if(mode==1) {
					sizeA=i;
					strcpy((char *)right_buffer,"");
				}
				i=0;
				mode=0;
			}
			if(key==1) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, missing bracket\n",conf,l);
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
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected '\"'\n",conf,l);
				parse_error=1;
			}
			win=0;
		}
		else if(slash==1) {
			slash=0;
			if(c=='n') { right_buffer[i]='\n'; }
			else if(c=='\n' || c=='\r') { i--; } //do nothing
			else { right_buffer[i]=c; }
			i++;
			win=0;
		}
		else if(c=='\\' && slash==0) {
			if(quote==1) {
				slash=1;
			} else {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected '\\'\n",conf,l);
				parse_error=1;
			}
			win=0;
		}
		else if(comment==0 && key==0 && quote==0 && (c==' ' || c=='=' || isblank(c))) {
			if(mode==1) {
				mode=2;
				sizeA=i;
				i=0;
			}
			else if(mode==3) {
				if(c=='=') {
					fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"=\"\n",conf,l);
					parse_error=1;
				}
				mode=4;
			}
			win=0;
		} else if(isalpha(c) && comment==0 && key==0 && mode==0) { //first character of a config directive
			if(i>=1024) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, line too long\n",conf,l);
				parse_error=1;
			}
			left_buffer[i]=tolower(c);
			i++;
			mode=1;
			win=0;
			//printf("<mode-is-now-%i>",mode);
		} else if((my_isalpha(c) || isprint(c)) && key==0 && comment==0 && (mode==1 || mode==2 || mode==3)) {
			if(i>=1024) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, line too long\n",conf,l);
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
		} else if(comment==0 && !feof(f)) {
			fprintf(dsc,"WAR: Parse error reading %s at line %i, unexpected character '%c'\n",conf,l,c);
			warning=1;
		}

		if(store_key || first_key_done==0) {
			if(store_key==1) {
				store_key=2;
				sizeA=0;
			}

			if(first_key_done==0) {
				strcpy((char *)left_buffer,"global");
			}

			first_key_done=1;

			if(*cfg2==NULL) {
				*cfg2=(st_config *)malloc(sizeof(st_config));
				cfg=*cfg2;
				cfg->n=0;
				cfg->keys=NULL;
				if(cfg==NULL) {
					return -4;
				}
			//} else {
			}
				found_key=-1;
				if(cfg->keys==NULL) {
					cfg->n++;
					cfg->keys=(st_config_keys *)malloc(sizeof(st_config_keys) * cfg->n);
				} else {
					//search a previous key
					for(e=0; e<cfg->n; e++) {
						if(!strcmp((const char *)cfg->keys[e].key,(const char *)left_buffer)) {
							found_key=e;
							break;
						}
					}
					if(found_key!=-1) { current_key=found_key; }
					else { //then create it
						cfg->n++;
						cfg->keys=(st_config_keys *)realloc((void *)cfg->keys,\
						sizeof(st_config_keys) * cfg->n);
					}
				}
				if(cfg->keys==NULL) {
					return -4;
				}
				if(found_key==-1) {
					current_key=cfg->n-1;
					strcpy((char *)cfg->keys[current_key].key,(char *)left_buffer);
					cfg->keys[current_key].n=0;
					cfg->keys[current_key].config=NULL;
				}
			//}
		}

		if(store_key==0 && mode==0 && key==0 && sizeA!=0) { //store inside correct key
			left_buffer[sizeA]='\0';
			right_buffer[sizeB]='\0';

			if(!strcmp((const char *)left_buffer,"read_config")) {
				ret=read_config(dsc,(char *)right_buffer,&cfg,allowed,nall);
				if(ret<=-4) {
					fprintf(dsc,"ERR: Fatal error reading include file %s at line %i of %s\n",right_buffer,l-1,conf);
					return -4;
				} else if(ret>-4 && ret<=-1) {
					fprintf(dsc,"WAR: Error reading include file %s at line %i of %s\n",right_buffer,l-1,conf);
					warning=1;
				}
			} else {

				check=0;
				for(e=0; e<nall; e++) {
					if(!strcmp((const char *)allowed[e],left_buffer)) {
						check=1;
						break;
					}
				}
				if(check!=1) {
					fprintf(dsc,"WAR: Unknown configuration directive %s reading %s at line %i\n",left_buffer,conf,l);
					warning=1;
				} else {
					found_key=-1;
					if(cfg->keys[current_key].config==NULL) {
						cfg->keys[current_key].n++;
						cfg->keys[current_key].config=(st_config_vals *)\
						malloc(sizeof(st_config_vals) * cfg->keys[current_key].n);
						if(cfg->keys[current_key].config==NULL) {
							return -4;
						}
					} else {

						for(e=0; e<cfg->keys[current_key].n; e++) {
							if(!strcmp((const char *)cfg->keys[current_key].config[e].name,\
							(const char *)left_buffer)) {
								found_key=e;
								break;
							}
						}

						if(found_key==-1) {
							cfg->keys[current_key].n++;
							cfg->keys[current_key].config=(st_config_vals *)\
							realloc((void *)cfg->keys[current_key].config,\
							sizeof(st_config_vals) * cfg->keys[current_key].n);
						}
					}
					if(cfg->keys[current_key].config==NULL) {
						return -4;
					}
					if(found_key==-1) {
						found_key=cfg->keys[current_key].n-1;
						strcpy((char *)cfg->keys[current_key].config[found_key].name,\
						(const char *)left_buffer);
						cfg->keys[current_key].config[found_key].value=NULL;
					}

					if(cfg->keys[current_key].config[found_key].value!=NULL) {
						free((void *)cfg->keys[current_key].config[found_key].value);
					}
					cfg->keys[current_key].config[found_key].value=(char *)\
					malloc(sizeof(char) * strlen((char *)right_buffer)+1);
					if(cfg->keys[current_key].config[found_key].value==NULL) {
						return -4;
					}
					strcpy((char *)cfg->keys[current_key].config[found_key].value,\
					(const char *)right_buffer);
				}
			}
		}
		store_key=0;

		if(parse_error) {
			fclose(f);
			return -2;
		}

	}

	fclose(f);

	if(warning) return -1;
	return 0;
}

U32 cnf_getU32(U32 defecto,char * what,char * where,st_config * cfg) {

	int i,e;

	if(cfg==NULL) return defecto;
	for(i=0; i<cfg->n; i++) {
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)where)) {
			for(e=0; e<cfg->keys[i].n; e++) {
				if(!strcmp((const char *)cfg->keys[i].config[e].name,(const char *)what)) {
					return(atoi(cfg->keys[i].config[e].value));
				}
			}
			break;
		}
	}
	//abort();
	return defecto;
}

U16 cnf_getU16(U16 defecto,char * what,char * where,st_config * cfg) {

	int i,e;

	if(cfg==NULL) {
		DBG(5,"cfg NULL?\n");
		return defecto;
	}
	for(i=0; i<cfg->n; i++) {
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)where)) {
			for(e=0; e<cfg->keys[i].n; e++) {
				if(!strcmp((const char *)cfg->keys[i].config[e].name,(const char *)what)) {
					return((U16)atoi(cfg->keys[i].config[e].value));
				}
			}
			break;
		}
	}
	//abort();
	return defecto;
}

Byte cnf_getByte(Byte defecto,char * what,char * where,st_config * cfg) {

	int i,e;

	DBG(5,"%s,%s\n",what,where);

	if(cfg==NULL) {
		DBG(5,"Is cfg null?");
		return defecto;
	}

	if(cfg==NULL) return defecto;
	DBG(6,"for(i=0; i<%i; i++)\n",cfg->n);
	for(i=0; i<cfg->n; i++) {
		DBG(5,"i:%i, key:%s\n",i,cfg->keys[i].key);
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)where)) {
			for(e=0; e<cfg->keys[i].n; e++) {
				DBG(6,"%i-%i\n",i,e);
				if(!strcmp((const char *)cfg->keys[i].config[e].name,(const char *)what)) {
					return((Byte)atoi(cfg->keys[i].config[e].value));
				}
			}
			break;
		}
	}
	return defecto;
}

Byte * cnf_getString(Byte * defecto,char * what,char * where,st_config * cfg) {

	int i,e;

	DBG(5,"%s,%s\n",what,defecto);

	if(cfg==NULL) {
		DBG(5,"Is cfg null?");
		return defecto;
	}
	for(i=0; i<cfg->n; i++) {
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)where)) {
			for(e=0; e<cfg->keys[i].n; e++) {
				if(!strcmp((const char *)cfg->keys[i].config[e].name,(const char *)what)) {
					return((Byte *)((cfg->keys[i].config[e].value)));
				}
			}
			break;
		}
	}
	return defecto;
}

#endif
