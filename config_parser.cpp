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

/**********************************************
  For parsing the configuration URU files
*************************************************/

#define __U_CONFIG_PARSER_ID "$Id$"

#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include "config_parser.h"

#include "debug.h"

// We don't want sockets, network, protocol, and nothing else here, it's only a parser
// and it should be a parser

//configuration variables moved to tmp_config.cpp and tmp_config.h, since the new
// netcore is not using them.

//TODO: read_config only works on: A) Absolute paths, B) relative paths to the current working directory. Needs to be fixed, to read relative paths from the specific config file.

//ChangeLog: Allowed array has been removed, any directive is now allowed.
//           Allow tuples in the config files.
//            For example: shards[1]="GoE","almlys.dyns.net","5000","tpots","spain","1.1","0","0","1"
//                         shards[2]="Foo's shard","here.dot.slash.kk","65534","hacking","milky way","109.2"


/** Handle nicely foreign languages. setlocale() is not sufficient on systems which didn't installed all locales,
 so we better use our own function. Feel free to add other accents according to your language.
*/
long my_isalpha(int c) {
#ifdef __WIN32__
	return(IsCharAlpha(c));
#else
  return((long)index("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ·‡‚‰\
ÈËÍÎÛÚÙˆ˙˘˚¸Á«Ò—", c)); //<- Not sure how this looks like under UTF, but I'm sure that it looks horrible.
#endif
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
int read_config(FILE * dsc, char * conf,st_config ** cfg2) { //, char ** allowed, int nall) {
	FILE * f;

	st_config * cfg=*cfg2;

	int c; //current_char
	char left_buffer[1024]; //Max size of a variable
	char right_buffer[1024]; //Max size of a value
	int sizeA=0;
	int sizeB=0;

	int i; //,e; //an iterator
	int l; //line counter

	Byte comment=0; //commentary
	Byte mode=0; //parse mode 0 none, 1 left, 2 mid, 3 right, 4 end
	Byte quote=0; //quote mode
	Byte slash=0; //slash mode
	Byte win=0; //windoze mode
//	Byte check=0; //check if valid
	Byte key=0; //are we in key mode?
	Byte tuple=0; //tuple mode
	
	int tuple_y=0,tuple_x=0; //rows x columns

	Byte store_key=0; //store the key?
	//Byte first_key_done=0; //we have a current key?

	//int found_key=0;
	//int current_key=0; //the current key (default global)
	char cur_key[200]="global";

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
		if((c=='#' || c==';') && quote==0) {
			if(mode==1 || mode==2 || mode==3) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"%c\"",conf,l,c);
				parse_error=1;
			}
			comment=1;  //activate comment flag
			win=0;
		}
		else if(c=='[' && quote==0 && comment==0) {
			if(mode==2 || mode==3 || key==1) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"[\"",conf,l);
				parse_error=1;
			}
			if(mode==1) {
				tuple=1; //set tuple flag
				sizeA=i;
				mode=2;
			} else {
				key=1; //set key flag
			}
			win=0;
			i=0;
		}
		else if(c==']' && quote==0 && comment==0) {
			if(mode==1 || (mode==2 && tuple==0) || mode==3 || (key==0 && tuple==0)) {
				DBG(5,"mode:%i,key:%i,tuple:%i\n",mode,key,tuple);
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"]\"\n",conf,l);
				parse_error=1;
			}
			if(tuple==1) {
				tuple=0;
				right_buffer[i]='\0';
				tuple_y=atoi(right_buffer); //store the row
			} else {
				key=0; //unset key flag
				left_buffer[i]='\0';
				sizeA=i;
				store_key=1;
				mode=0;
			}
			win=0;
			i=0;
		}
		else if(key==1) {
			left_buffer[i]=tolower(c);
			i++;
		}
		else if(tuple==1) {
			if(!isdigit(c)) {
				fprintf(dsc,"ERR: Parse error reading %s at line %i, unexpected \"%c\"\n",conf,l,c);
			}
			right_buffer[i]=c;
			i++;
		}
		else if(c==',' && mode==4 && quote==0 && comment==0) {
			mode=2;
			sizeB=i;
			i=0;
			left_buffer[sizeA]='\0';
			right_buffer[sizeB]='\0';
			DBG(5,"tuple_x:%i,tuple_y:%i\n",tuple_x,tuple_y);
			cnf_add_key_xy(right_buffer,left_buffer,cur_key,tuple_x,tuple_y,cfg2);
			cfg=*cfg2;
			tuple_x++;
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
			if(key==1 || tuple==1) {
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
			else {
				//right_buffer[i++]='\\';
				//NOTE: If you want something like this "\k" to be "\k", please use "\\k", that will be "\k"
				//We want always to strip out any single slash. There are several other reserved chars that will
				// be added in the future. ("$man bash" shows a small list, but it depends on the OS)
				right_buffer[i]=c;
			}
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

		if(store_key) {
			store_key=2;
			sizeA=0;
			strcpy(cur_key,left_buffer);
		}
		
		#if 0
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
		#endif

		if(store_key==0 && mode==0 && key==0 && sizeA!=0) { //store inside correct key
			left_buffer[sizeA]='\0';
			right_buffer[sizeB]='\0';

			DBG(8,"sk:%i,mode:%i,key:%i,sizeA:%i,sizeB:%i\n",store_key,mode,key,sizeA,sizeB);
			DBG(3,"read %s:%s [%s]\n",left_buffer,right_buffer,cur_key); //cfg->keys[current_key].key);

			if(!strcmp((const char *)left_buffer,"read_config")) {
				ret=read_config(dsc,(char *)right_buffer,&cfg); //,allowed,nall);
				if(ret<=-4) {
					fprintf(dsc,"ERR: Fatal error reading include file %s at line %i of %s\n",right_buffer,l-1,conf);
					return -4;
				} else if(ret>-4 && ret<=-1) {
					fprintf(dsc,"WAR: Error reading include file %s at line %i of %s\n",right_buffer,l-1,conf);
					warning=1;
				}
			} else {
				DBG(5,"tuple_x:%i,tuple_y:%i\n",tuple_x,tuple_y);
				cnf_add_key_xy(right_buffer,left_buffer,cur_key,tuple_x,tuple_y,cfg2);
				cfg=*cfg2;

				#if 0
/*				check=0;
				for(e=0; e<nall; e++) {
					if(!strcmp((const char *)allowed[e],left_buffer)) {
						check=1;
						break;
					}
				} */
/*				if(check!=1) {
					fprintf(dsc,"WAR: Unknown configuration directive %s reading %s at line %i\n",left_buffer,conf,l);
					warning=1;
				} else { */
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
				//}
				#endif
			}
			sizeA=0;
			sizeB=0;
			tuple_x=0;
			tuple_y=0;
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

int cnf_add_key(char * value,char * what,char * where,st_config ** cfg2) {
	return(cnf_add_key_xy(value,what,where,0,0,cfg2));
}

/** adds a new key
*/
int cnf_add_key_xy(char * gvalue,char * what,char * where,int x,int y,st_config ** cfg2) {
	DBG(4,"adding key at (%i,%i) [%s] %s:%s\n",x,y,where,what,gvalue);
	st_config * cfg;
	int e,found_key,current_key;
	if(*cfg2==NULL) {
		*cfg2=(st_config *)malloc(sizeof(st_config));
		cfg=*cfg2;
		cfg->n=0;
		cfg->keys=NULL;
		if(cfg==NULL) {
			return -4;
		}
	}
	cfg=*cfg2;

	found_key=-1;
	if(cfg->keys==NULL) {
		cfg->n++;
		cfg->keys=(st_config_keys *)malloc(sizeof(st_config_keys) * cfg->n);
	} else {
		//search a previous key
		for(e=0; e<cfg->n; e++) {
			if(!strcmp((const char *)cfg->keys[e].key,(const char *)where)) {
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
		strcpy((char *)cfg->keys[current_key].key,(char *)where);
		cfg->keys[current_key].n=0;
		cfg->keys[current_key].config=NULL;
	}

	//now the value
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
			(const char *)what)) {
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
		(const char *)what);
		cfg->keys[current_key].config[found_key].value=NULL;
		cfg->keys[current_key].config[found_key].x=0;
		cfg->keys[current_key].config[found_key].y=0;
	}
	
	int j,k;
	char ** value=NULL;
	int ox,oy,my;
	
	ox=cfg->keys[current_key].config[found_key].x;
	oy=cfg->keys[current_key].config[found_key].y;
	
	my=( oy > (y+1) ? oy : (y+1));

		DBG(8,"dmalloc_verify()\n");
	dmalloc_verify();

		
	if(ox<(x+1)) {
		//then we must resize & copy
		DBG(5,"malloc %i bytes\n",((my)*(x+1)));
		value=(char **)malloc(sizeof(char *)*((my)*(x+1)));
		memset(value,0,(sizeof(char *)*((my)*(x+1))));
		for(j=0; j<oy; j++) {
			for(k=0; k<ox; k++) {
				*(value+(((x+1)*j)+k))=*((cfg->keys[current_key].config[found_key].value)+((ox*j)+k));
			}
			/*
			for(;k<x;k++) {
				*(value+((x*j)+k))=NULL;
			}*/
		}
		cfg->keys[current_key].config[found_key].x=x+1;
		cfg->keys[current_key].config[found_key].y=my;
		ox=x+1;
		oy=my;
		free((void *)cfg->keys[current_key].config[found_key].value);
		cfg->keys[current_key].config[found_key].value=value;
		value=NULL;
	}
	
		DBG(8,"dmalloc_verify()\n");
	dmalloc_verify();

	
	if(oy<(y+1)) {
		value=(char **)realloc((void *)(cfg->keys[current_key].config[found_key].value),sizeof(char *)*((y+1)*(ox)));
		DBG(5,"realloc %i bytes\n",((y+1)*(ox)));
		if(value==NULL) { _DIE("FATAL: value is NULL!"); }
		DBG(7,"memset off:%i size:%i\n",((ox)*(oy)),((((y+1)*(ox)))-((ox)*(oy))));
		memset((value+((ox)*(oy))),0,(sizeof(char *)*((((y+1)*(ox)))-((ox)*(oy)))));
		cfg->keys[current_key].config[found_key].value=value;
		value=NULL;
	}

	DBG(8,"dmalloc_verify()\n");
	dmalloc_verify();
	
	char ** pval=NULL;
	
		DBG(8,"dmalloc_verify()\n");
	dmalloc_verify();

	DBG(8,"%i\n",(ox*y)+x);
	DBG(8,"ckey:%i,fkey:%i\n",current_key,found_key);
	pval=(cfg->keys[current_key].config[found_key].value+((ox*y)+x));
	DBG(8,"key:%s\n",cfg->keys[current_key].key);
	
		DBG(8,"dmalloc_verify()\n");
	dmalloc_verify();
	
	DBG(8,"var:%s\n",cfg->keys[current_key].config[found_key].name);

			DBG(8,"dmalloc_verify()\n");
	dmalloc_verify();
		
	//pval=cfg->keys[current_key].config[found_key].value;

	DBG(8,"dmalloc_verify()\n");
	dmalloc_verify();
	
	if(((char *)(*pval))!=NULL) {
		free((void *)(*pval));
		*pval=NULL;
	}
	*pval=(char *)malloc(sizeof(char) * (strlen((char *)gvalue)+1));
	if(*pval==NULL) {
		return -4;
	}
	strcpy((char *)*pval,(const char *)gvalue);
	return 0;
}


/** Checks if an specific var is available
		\returns 1 if true, elsewhere if not
*/
char cnf_exists(char * what,char * where,st_config * cfg) {
	DBG(4,"cnf_exists? [%s] %s\n",where,what);
	int i,e;
	if(cfg==NULL) return 0;
	for(i=0; i<cfg->n; i++) {
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)where)) {
			for(e=0; e<cfg->keys[i].n; e++) {
				if(!strcmp((const char *)cfg->keys[i].config[e].name,(const char *)what)) {
					DBG(4,"true\n");
					return 1;
				}
			}
			break;
		}
	}
	DBG(4,"false\n");
	return 0;
}

//gets a U32 value
U32 cnf_getU32(U32 defecto,char * what,char * where,st_config * cfg) {
	DBG(4,"getU32 [%s] %s:%i\n",where,what,defecto);
	int i,e;

	if(cfg==NULL) return defecto;
	for(i=0; i<cfg->n; i++) {
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)where)) {
			for(e=0; e<cfg->keys[i].n; e++) {
				if(!strcmp((const char *)cfg->keys[i].config[e].name,(const char *)what)) {
					if(cfg->keys[i].config[e].value==NULL) return defecto;
					DBG(4,"return %i\n",atoi(*(cfg->keys[i].config[e].value)));
					return(atoi(*(cfg->keys[i].config[e].value)));
				}
			}
			break;
		}
	}
	DBG(4,"return %i\n",defecto);
	//abort();
	return defecto;
}

//sets a U32 value
void cnf_setU32(U32 val,char * what,char * where,st_config ** cfg2) {
	DBG(4,"setU32 [%s] %s:%i\n",where,what,val);
	char * value=NULL;
	value=(char *)malloc(101*sizeof(char));
	snprintf(value,100,"%i",val);

	cnf_add_key(value,what,where,cfg2);

	if(value!=NULL) { free((void *)value); }
}

//sets a U32 value
void cnf_setU16(U16 val,char * what,char * where,st_config ** cfg2) {
	DBG(4,"setU16 [%s] %s:%i\n",where,what,val);
	char * value=NULL;
	value=(char *)malloc(101*sizeof(char));
	snprintf(value,50,"%i",val);

	cnf_add_key(value,what,where,cfg2);

	if(value!=NULL) { free((void *)value); }
}

void cnf_setByte(Byte val,char * what,char * where,st_config ** cfg2) {
	DBG(4,"setByte [%s] %s:%i\n",where,what,val);
	char * value=NULL;
	value=(char *)malloc(101*sizeof(char));
	snprintf(value,5,"%i",val);

	cnf_add_key(value,what,where,cfg2);

	if(value!=NULL) { free((void *)value); }
}


U16 cnf_getU16(U16 defecto,char * what,char * where,st_config * cfg) {
	DBG(4,"getU16 [%s] %s:%i\n",where,what,defecto);
	int i,e;

	if(cfg==NULL) {
		DBG(5,"cfg NULL?\n");
		return defecto;
	}
	for(i=0; i<cfg->n; i++) {
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)where)) {
			for(e=0; e<cfg->keys[i].n; e++) {
				if(!strcmp((const char *)cfg->keys[i].config[e].name,(const char *)what)) {
					if(cfg->keys[i].config[e].value==NULL) return defecto;
					DBG(4,"return %i\n",atoi(*(cfg->keys[i].config[e].value)));
					return((U16)atoi(*(cfg->keys[i].config[e].value)));
				}
			}
			break;
		}
	}
	DBG(4,"return %i\n",defecto);
	//abort();
	return defecto;
}

Byte cnf_getByte(Byte defecto,char * what,char * where,st_config * cfg) {
	DBG(4,"getByte [%s] %s:%i\n",where,what,defecto);
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
					if(cfg->keys[i].config[e].value==NULL) return defecto;
					DBG(4,"return %i\n",atoi(*(cfg->keys[i].config[e].value)));
					return((Byte)atoi(*(cfg->keys[i].config[e].value)));
				}
			}
			break;
		}
	}
	DBG(4,"return %i\n",defecto);
	return defecto;
}

char * cnf_getString(const char * defecto,const char * what,const char * where,st_config * cfg) {
	DBG(4,"getString [%s] %s:%s\n",where,what,defecto);
	int i,e;

	DBG(5,"%s,%s\n",what,defecto);

	if(cfg==NULL) {
		DBG(5,"Is cfg null?");
		return (char *)defecto;
	}
	for(i=0; i<cfg->n; i++) {
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)where)) {
			for(e=0; e<cfg->keys[i].n; e++) {
				if(!strcmp((const char *)cfg->keys[i].config[e].name,(const char *)what)) {
					if(cfg->keys[i].config[e].value==NULL) return (char *)defecto;
					DBG(4,"return %s\n",(*(cfg->keys[i].config[e].value)));
					return((char *)(*(cfg->keys[i].config[e].value)));
				}
			}
			break;
		}
	}
	DBG(4,"return %s\n",defecto);
	return (char *)defecto;
}

/** Copy all vars from a keygroup to another one */
void cnf_copy(const char * to, const char * from,st_config ** cfg2) {
	DBG(4,"copy from %s to %s\n",from,to);
	int i,e;
	st_config * cfg;
	cfg=*cfg2;

	if(cfg==NULL) return;
	if(!strcmp(from,to)) return;
	for(i=0; i<cfg->n; i++) {
		if(!strcmp((const char *)cfg->keys[i].key,(const char *)from)) {

			for(e=0; e<cfg->keys[i].n; e++) {
				cnf_add_key((char *)((*(cfg->keys[i].config[e].value))),\
				(char *)cfg->keys[i].config[e].name,\
				(char *)to,cfg2);
			}

			break;
		}
	}
}


/** Copy a var from one key group to another one, under other name */
void cnf_copy_key(const char * to_name,const char * from_name,const char * to, const char * from,st_config ** cfg2) {
	DBG(4,"copy from %s:%s to %s:%s\n",from,from_name,to,to_name);
	if(cnf_exists((char *)from_name,(char *)from,*cfg2)==1) {
		cnf_add_key((char *)cnf_getString("",(char *)from_name,(char *)from,*cfg2),(char *)to_name,(char *)to,cfg2);
	}
}

void cnf_destroy(st_config ** cfg2) {

	int i,e;
	st_config * cfg;
	cfg=*cfg2;

	if(*cfg2==NULL) return;

	for(i=0; i<cfg->n; i++) {
		for(e=0; e<cfg->keys[i].n; e++) {
			if(cfg->keys[i].config[e].value!=NULL) {
				free((void *)cfg->keys[i].config[e].value);
			}
		}
		if(cfg->keys[i].config!=NULL) {
			free((void *)cfg->keys[i].config);
		}
	}
	free((void *)cfg->keys);
	free((void *)*cfg2);
	cfg2=NULL;
}


#ifdef _TEST_

// g++ -Wall -g config_parser.cpp -o cfgtest -D_TEST_
// g++ -Wall -g config_parser.cpp -o cfgtest -D_TEST_ debug.o -ldmalloccxx -DDMALLOC_FUNC_CHECK

int main() {

	printf("Testing config parser suite\n");
	
	st_config * cfg=NULL;
	
	read_config(stdout,"uru.conf",&cfg);
	
	U32 a;
	U16 b;
	Byte c;
	char * d=NULL;
	Byte g;
	
	a=cnf_getU32(23,"maika","pegasus",cfg);
	printf("Test:23,%i\n",a);
	b=cnf_getU16(13,"qwerty","jota",cfg);
	printf("Test:13,%i\n",b);
	c=cnf_getByte(88,"byte","maika",cfg);
	d=cnf_getString("noway","jo","ja",cfg);
	printf("Test:noway,%s\n",d);

	g=cnf_exists("kk","kk",cfg);
	printf("exists kk?: %i\n",g);

	cnf_setU32(123,"u32","tests",&cfg);
	cnf_setU16(456,"u16","tests",&cfg);
	cnf_setByte(111,"byte","tests",&cfg);
	cnf_add_key("key","string","tests",&cfg);

	a=cnf_getU32(23,"u32","tests",cfg);
	printf("Test:23,%i\n",a);
	b=cnf_getU16(13,"u16","tests",cfg);
	printf("Test:13,%i\n",b);
	c=cnf_getByte(88,"byte","tests",cfg);
	d=cnf_getString("noway","string","tests",cfg);
	printf("Test:noway,%s\n",d);
	
	cnf_add_key("I'm Maria","maria","tests",&cfg);
	cnf_add_key("I'm Laura","laura","tests",&cfg);

	cnf_copy("koka","tests",&cfg);
	cnf_copy_key("laura","maria","koka","tests",&cfg);
	cnf_copy_key("maria","laura","koka","tests",&cfg);

	//dump all
	int i,e,j,k,nx,ny;

	for(i=0; i<cfg->n; i++) {
		printf("[%s]\n",cfg->keys[i].key);
		for(e=0; e<cfg->keys[i].n; e++) {
			if(cfg->keys[i].config[e].value!=NULL) {
				nx=cfg->keys[i].config[e].x;
				ny=cfg->keys[i].config[e].y;
				for(j=0;j<ny;j++) {
					if(ny==1) {
						printf("%s = ",cfg->keys[i].config[e].name);
					} else {
						printf("%s[%i] = ",cfg->keys[i].config[e].name,j);
					}
					for(k=0;k<nx;k++) {
						if(k!=0) { printf(","); }
						printf("\"%s\"",*((cfg->keys[i].config[e].value)+((j*nx)+k)));
					}
					printf("\n");
				}
			}
		}
	}
	
	cnf_destroy(&cfg);
	
	printf("Success!\n");
	
	return 0;
}

//This is work in progress!


#endif

