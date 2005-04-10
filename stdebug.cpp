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

/** UNET3 debugging system

*/

#define __U_DEBUG_ID $Id$

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>

#ifdef __WIN32__
#include "windoze.h"
#include <windows.h>
#else
#include <syslog.h>
#endif

#include "stdebug.h"

#include "debug.h"

//dump packets?
#define _DEBUG_DUMP_PACKETS

st_log_config * stdebug_config=NULL;

st_log * f_err=NULL;
st_log * f_uru=NULL;


void log_set_defaults() {
	if(stdebug_config==NULL) {
		stdebug_config=(st_log_config *)malloc(sizeof(st_log_config) * 1);
	}
	if(stdebug_config!=NULL) {
	memset(stdebug_config,0,sizeof(stdebug_config));
	stdebug_config->silent=0;
	stdebug_config->n_files2rotate=5;
	stdebug_config->path="log/";
	stdebug_config->rotate_size=2*1024*1024;
	stdebug_config->creation_mask=00750;
	stdebug_config->level=6;
	stdebug_config->log_flags= DF_DEFSTDOUT | DF_STAMP | DF_IP; // | DF_ANOY;
	stdebug_config->build="Alcugs H'uru server logging system";
	//syslog
	stdebug_config->syslogname="alcugs";
	stdebug_config->syslog_enabled=0x00;
	//db
	stdebug_config->dbhost=NULL;
	stdebug_config->dbport=0;
	stdebug_config->dbname="uru_events";
	stdebug_config->dbuser="uru";
	stdebug_config->dbpasswd="";
	stdebug_config->dbeventtable="events";
	stdebug_config->db_enabled=0x00;
	//unet
	stdebug_config->host="localhost";
	stdebug_config->port=9000;
	stdebug_config->protocol=0x00;
	//track logs
	stdebug_config->n_logs=0;
	stdebug_config->logs=NULL;
	}

	/*
	if(f_err==NULL) {
		f_err=open_log("error.log",2,DF_STDERR);
	}
	if(f_uru==NULL) {
		f_uru=open_log("uru.log",2,DF_STDOUT);
	}
	*/
	//TODO for db and socket debugging based systems
	//do socket init if required
	//do db init if required
	//spawn a logger worker thread if required
}

void log_shutdown() {
	//close_log(f_err);
	//close_log(f_uru);
	int i;
	for(i=0; i<stdebug_config->n_logs; i++) {
		close_log(stdebug_config->logs[i]);
	}
	if(stdebug_config->logs!=NULL) {
		free((void *)stdebug_config->logs);
		stdebug_config->logs=NULL;
	}
	if(stdebug_config!=NULL) {
		free((void *)stdebug_config);
		stdebug_config=NULL;
	}
}

// For forked childs
void log_shutdown_silent() {
	//close_log(f_err);
	//close_log(f_uru);
	int i;
	for(i=0; i<stdebug_config->n_logs; i++) {
		close_log_silent(stdebug_config->logs[i]);
	}
	if(stdebug_config->logs!=NULL) {
		free((void *)stdebug_config->logs);
		stdebug_config->logs=NULL;
	}
	if(stdebug_config!=NULL) {
		free((void *)stdebug_config);
		stdebug_config=NULL;
	}
}


void log_init() {
	DBG(5,"init..");
	if(stdebug_config==NULL) {
		log_set_defaults();
	}
}

void log_openstdlogs() {
	if(f_err==NULL) {
		f_err=open_log("error.log",2,DF_STDERR);
	}
	if(f_uru==NULL) {
		f_uru=open_log("uru.log",2,DF_STDOUT);
	}
}

char * html_generate_head(char * title,char * powered) {
	static char head[512];
	//%s. Build %s - Version %s - Id: %s

	snprintf(head,511,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n\
<html><head>\n<title>%s</title>\n\
<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n\
<meta HTTP-EQUIV=\"Generator\" CONTENT=\"%s\">\n\
</head>\n<body>"\
,title,powered);
	return head;
}

char * get_ext(const char * addr) {
	static char ext[11];
	int i;
	for(i=strlen(addr); i>((int)(strlen(addr)-10)); i--) {
		if(addr[i]=='/' || addr[i]=='\\' || addr[i]==':' || addr[i]=='.') {
			break;
		}
	}
	memset(ext,0,sizeof(ext));
	//printf("prext:%s\n",ext);
	if(addr[i]=='.') {
		strncpy(ext,addr+(i+1),strlen(addr)-(i+1));
	} else {
		strcpy(ext,"");
	}
	//printf("ext:%s,%s,%i\n",addr,ext,sizeof(ext));
	return ext;
}

void strip_ext(char * addr) {
	int i=0;
	for(i=strlen(addr); i>(int)(strlen(addr)-10); i--) {
		if(addr[i]=='/' || addr[i]=='\\' || addr[i]==':') {
			break;
		} else if(addr[i]=='.') {
			addr[i]='\0';
			break;
		}
	}
}


/**
 Opens a log file to write,
 /return the log descriptor
 associated to the log file

 flags are detailed in the stdebug.h file

 returns NULL if failed
*/
st_log * open_log(const char * name, char level, U16 flags) {
	char * path=NULL;
	char * croak=NULL;
	int i,e,size;

	int f,found=-1;

	for(f=0; f<stdebug_config->n_logs; f++) {
		if(stdebug_config->logs[f]==NULL) {
			found=f;
			break;
		}
	}

	st_log * log=NULL;
	log=(st_log *)malloc(sizeof(st_log) * 1);

	DBG(6,"init\n");
	log_init();
	if(log==NULL) return NULL;

	memset(log,0,sizeof(log));
	log->name=NULL;
	log->dsc=NULL;
	log->level=level;
	log->flags = stdebug_config->log_flags | flags;
	log->facility = LOG_USER;
	log->priority = LOG_DEBUG;

	if(name!=NULL) {
		log->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
		if(log->name==NULL) { free((void *)log); return NULL; }
		strcpy(log->name,name);

		DBG(5,"cont...\n");

		FILE * def=NULL;
		if(log->flags & DF_DEFSTDOUT) def=stdout;

		if(stdebug_config->n_files2rotate<=0 || level==0) {
			log->dsc=def;
			//return log;
		} else {

			size=strlen(name) + strlen((const char *)stdebug_config->path);
			DBG(6,"size is:%i\n",size);

			croak=(char *)malloc(sizeof(char) * (size+1+5));
			if(croak==NULL) return log;
			DBG(6,"im here\n");
			path=(char *)malloc(sizeof(char) * (size+1));
			if(path==NULL) { free((void *)croak); return log; }
			DBG(7,"here too\n");

			if(name[0]!='/') {
				strcpy(path,stdebug_config->path);
			} else {
				strcpy(path,"");
				size=strlen(name);
			}
			strcat(path,name);

			DBG(5,"path is:%s\n",path);

			e=0;
			for(i=0; i<size; i++) {
				croak[e]=path[i];
				e++;
				if(croak[e-1]=='/' || croak[e-1]=='\\') { // || croak[e-1]==':') {
					croak[e]='\0';
					DBG(6,"mkdir %s\n",croak);
					mkdir(croak,stdebug_config->creation_mask);
					//e=0;
				}
			}

			if(croak!=NULL) { free((void *)croak); }

			//rotation
			if(!(log->flags & DF_APPEND) || (log->flags & DF_HTML)) {
				rotate_log(log,1);
			} else {
				rotate_log(log,0);
			}

			log->dsc=fopen(path,"a");
			if(log->dsc==NULL) {
				DBG(5,"ERR: Fatal - Cannot Open %s to write ",path);
				ERR(6,"returned error");
				log->dsc=def; //default standard output
				if(def==stdout) {
					DBG(6,"DBG: Sending debug messages to the stdout instead of %s\n",path);
				}
			} else {
				log->flags |= DF_OPEN;
				if(log->flags & DF_HTML) {
					fprintf(log->dsc,"%s",html_generate_head(log->name,stdebug_config->build));
				}

			}
			if(path!=NULL) { free((void *)path); }

		}
	}

	if(found==-1) {
		stdebug_config->n_logs++;
		st_log ** aux=NULL;
		aux=(st_log **)realloc((void *)stdebug_config->logs,sizeof(st_log *) * stdebug_config->n_logs);
		if(aux==NULL) {
			fprintf(stderr,"Failed allocating memory!\n");
		} else {
			stdebug_config->logs=aux;
			stdebug_config->logs[stdebug_config->n_logs-1]=log;
		}
	} else {
		stdebug_config->logs[found]=log;
	}

	return log;
}

int rotate_log(st_log * file,char force) {

	char * path=NULL;
	char * croak=NULL;
	char * croak2=NULL;
	char * gustavo=NULL;
	int i,size;
	int ret;

	struct stat file_stats;

	DBG(5,"init\n");
	log_init();

	if(file==NULL) return 0;

	if(stdebug_config->n_files2rotate<=0 || file->level==0) { //file->dsc==NULL ||
		//printf("%i,%i\n",stdebug_config->n_files2rotate,file->level);
		//abort();
		return 0;
	}

	size=strlen(file->name) + strlen((const char *)stdebug_config->path);

	croak=(char *)malloc(sizeof(char) * (size+1+5));
	if(croak==NULL) return -1;
	path=(char *)malloc(sizeof(char) * (size+1));
	if(path==NULL) { free((void *)croak); return -1; }
	croak2=(char *)malloc(sizeof(char) * (size+1+5));
	if(croak2==NULL) { free((void *)path); free((void *)croak); return -1; }
	gustavo=(char *)malloc(sizeof(char) * (size+1+5));
	if(gustavo==NULL) { free((void *)path); free((void *)croak);
		free((void *)croak2); return -1; }


	if(file->name[0]!='/') {
		strcpy(path,stdebug_config->path);
	} else {
		strcpy(path,"");
	}
	strcat(path,file->name);

	DBG(5,"path is:%s\n",path);

	if(stat(path,&file_stats)!=0) {
		DBG(6,"file not found, cannot rotate!\n");
		ret=-1;
	} else {

		if((int)file_stats.st_size<stdebug_config->rotate_size && force==0) {
			ret=0;
		} else {

			if(file->flags & DF_OPEN && file->dsc!=NULL && file->dsc!=stdout && file->dsc!=stderr && file->dsc!=stdin) {
				DBG(5,"clossing file %s...\n",path);
				if(file->flags & DF_HTML) {
					fprintf(file->dsc,"</body></html>\n");
				}
				fclose(file->dsc);
			}

			//rotation
			for(i=stdebug_config->n_files2rotate; i>0; i--) {
				sprintf(gustavo,"%s",path);
				strip_ext(gustavo);
				if(i-1==0) {
					sprintf(croak,"%s",path);
				} else {
					if(strlen(get_ext(path))!=0) {
						sprintf(croak,"%s.%i.%s",gustavo,i-1,get_ext(path));
					} else {
						sprintf(croak,"%s.%i",gustavo,i-1);
					}
				}
				if(strlen(get_ext(path))!=0) {
					sprintf(croak2,"%s.%i.%s",gustavo,i,get_ext(path));
				} else {
					sprintf(croak2,"%s.%i",gustavo,i);
				}
				if(stat(croak,&file_stats)==0) {
					if(i==stdebug_config->n_files2rotate) {
						DBG(5,"deleting %s\n",croak);
						unlink((const char *)croak);
					} else {
						DBG(5,"rename from %s to %s\n",croak,croak2);
						rename((const char *)croak,(const char *)croak2);
					}
				}
			}

			if(file->flags & DF_OPEN) {
				DBG(5,"opening file %s...\n",path);
				file->dsc=fopen(path,"w");
				if(file->dsc==NULL) {
					file->flags=file->flags & ~DF_OPEN;
					ret=-1;
				} else {
					if(file->flags & DF_HTML) {
						fprintf(file->dsc,"%s",html_generate_head(file->name,stdebug_config->build));
					}
				}
			}
		}
	}

	if(croak!=NULL) free((void *)croak);
	if(path!=NULL) free((void *)path);
	if(croak2!=NULL) free((void *)croak2);
	if(gustavo!=NULL) free((void *)gustavo);

	return ret;
}

/**
	Close the log file
*/
void close_log(st_log * log) {
	int f;
	if(log==NULL) return;
	for(f=0; f<stdebug_config->n_logs; f++) {
		if(stdebug_config->logs[f]==log) {
			stdebug_config->logs[f]=NULL;
			break;
		}
	}
	DBG(6,"clossing log %s...\n",log->name);
	if(log->dsc!=NULL && log->dsc!=stdout && log->dsc!=stderr && log->dsc!=stdin) {
		if(log->flags & DF_HTML) {
			fprintf(log->dsc,"</body></html>\n");
		}
		fclose(log->dsc);
		log->dsc=NULL;
	}
	if(log->name!=NULL) free((void *)log->name);
	free((void *)log);
	log=NULL; //this doens't do nothing, you must set it to NULL after calling close_log or BOUMB!
}

//for forked childs
void close_log_silent(st_log * log) {
	int f;
	if(log==NULL) return;
	for(f=0; f<stdebug_config->n_logs; f++) {
		if(stdebug_config->logs[f]==log) {
			stdebug_config->logs[f]=NULL;
			break;
		}
	}
	DBG(6,"clossing log %s...\n",log->name);
	if(log->dsc!=NULL && log->dsc!=stdout && log->dsc!=stderr && log->dsc!=stdin) {
		//if(log->flags & DF_HTML) {
			//fprintf(log->dsc,"</body></html>\n");
		//}
		fclose(log->dsc);
		log->dsc=NULL;
	}
	if(log->name!=NULL) free((void *)log->name);
	free((void *)log);
	log=NULL;
}


/**
	Allows to print big raw messages to the file and stdout descriptors.
	This don't sends messages to syslog, database, or socket
	- there is no rotate check code -
*/
void print2log(st_log * log, char * msg, ...) {
	va_list ap,ap2;

	if(log==NULL) return;

	va_start(ap,msg);
	va_start(ap2,msg);

	if(stdebug_config->level>=log->level && log->level!=0) {

		//first print to the file
		if(stdebug_config->n_files2rotate && log->dsc!=NULL && log->dsc!=stdout && log->dsc!=stderr && log->dsc!=stdin) {
			vfprintf(log->dsc,msg,ap);
		}

		//then print to the specific device
		if(!(log->flags & DF_HTML) && !(log->flags & DF_NODUMP)) {
			if(stdebug_config->silent<=2 && (log->flags & DF_STDERR)) {
				vfprintf(stderr,msg,ap2);     //stderr messages
			} else if(stdebug_config->silent<=1 && (log->flags & DF_STDOUT)) {
				vfprintf(stdout,msg,ap2);      //stdout messages
			} else if(stdebug_config->silent==0) {
				vfprintf(stdout,msg,ap2);      //print all (muhahahaha)
			}
		}

	}

	va_end(ap2);
	va_end(ap);
}


/**
  Prints a timestamp into the log file
	rotates the log if it is required
	(the rotation check is done, every 250 calls to this function)
*/
void stamp2log(st_log * log) {
	static Byte count=0;

	if(log==NULL) return;

	count++;
	if(count>250) { count=0; rotate_log(log,0); }

	if(!(log->flags & DF_STAMP)) { return; }

	char c_time_aux[26];
	struct tm * tptr;
	time_t timestamp;

	struct timeval tv;

	time(&timestamp);
	gettimeofday(&tv,NULL);

	tptr=gmtime((const time_t *)&timestamp);

	strftime(c_time_aux,25,"(%Y:%m:%d:%H:%M:%S",tptr);

	print2log(log,"%s.%06d) ",c_time_aux,tv.tv_usec);
}

#if 0
char * get_string_time(int stamp,int micros) {

	char c_time_aux[26];
	static char c_time_static[61];
	struct tm * tptr;

	tptr=gmtime((const time_t *)&stamp);
	strftime(c_time_aux,25,"(%Y:%m:%d:%H:%M:%S",tptr);

	snprintf(c_time_static,60,"%s.%06d) ",c_time_aux,micros);
	return c_time_static;
}
#endif

/**
  Adds and an entry to the specified log
	db and syslog logging is enabled, if specified,
	timestamp is also enabled
*/
void plog(st_log * log, char * msg, ...) {
	va_list ap;
	static char buf[2*1024];

	if(log==NULL) return;

	if(stdebug_config->level>=log->level && log->level!=0) {

		va_start(ap,msg);
		vsnprintf(buf,sizeof(buf)-1,msg,ap);
		va_end(ap);

		stamp2log(log);
		print2log(log,"%s",buf);

#ifdef __WIN32__
	//TODO, implement here, windoze based syslog logging, if it has something similar.
		if(log->flags & DF_ANOY) {
			MessageBox(NULL,buf,stdebug_config->syslogname,0);
		}
#else
		if(log->flags & DF_SYSLOG && stdebug_config->syslog_enabled==0x01) {
			//log to the syslog
			openlog(stdebug_config->syslogname, LOG_NDELAY | LOG_PID, LOG_AUTHPRIV); //LOG_USER
			syslog(LOG_DEBUG,"%s",buf);
			closelog();
		}
#endif

#ifdef _DBLOGGING_
		if(log->flags & DF_DB && stdebug_config->db_enabled==0x01) {
			dblog(log,"generic","system","system","%s",buf);
		}
#endif

	}
}

//log with another loging level
void logl(st_log * log, char level,char * msg, ...) {
	va_list ap;
	static char buf[1024];
	char tmp;

	if(log==NULL) return;

	if(stdebug_config->level>=level && log->level!=0) {

		tmp=log->level;
		log->level=level;

		va_start(ap,msg);
		vsnprintf(buf,sizeof(buf)-1,msg,ap);
		va_end(ap);

		plog(log,"%s",buf);

		log->level=tmp;
	}
}

/**
	Database based logging system

	type -> event type (column type)
	user -> who has generated the event? - object or user that generated the event
	location -> user ip address, or object location
	msg -> the message to register

*/
void dblog(st_log * log, char * type, char * user, char * location, char * msg, ...) {
#ifdef _DBLOGGING_
		//TODO, implement here database based log system, that is optional.
		//will work only, if the user compiles this module with the _DBLOGGING_ option
#endif
}

/**
	TODO socket based logging, send all debugging messages to an udp listener, or spawn
	a new thread to listen to incoming tcp connections.
*/


/**
	Fflush all the streams
*/
void logflush(st_log * log) {
	if(log==NULL) return;
	if(log->dsc!=NULL) {
		fflush(log->dsc);
	}
	if(log->flags & DF_STDERR && stdebug_config->silent<=2) {  fflush(stderr); }
	if((log->flags & DF_STDOUT && stdebug_config->silent<=1) || stdebug_config->silent==0) {  fflush(stdout); }
}


void dumpbuf(st_log * dsc,unsigned char* buf, unsigned int n) {
	if(dsc==NULL) return;
	dump_packet(dsc,buf,n,0,7);
}

/*-------------------------------------------------------------
 Dumps the packet into the desired FileStream
 how
  0 -- Hex
  1 -- Human readable
  2 -- Spaced human readable
  3 -- Inverted bits Human readable
  4 -- Spaced Inverted bits Human readable
  5 -- Hex table
  6 -- Hex table with inverted bits
  7 -- Hex table with and without inverted bits
e = offset
n = buffer size
buf = The Buffer
dsc = File descriptor where the packet will be dumped
 --------------------------------------------------------------*/
void dump_packet(st_log * dsc,unsigned char* buf, unsigned int n, unsigned int e, int how) {
#ifdef _DEBUG_DUMP_PACKETS
	unsigned int i=0,j=0,k=0;
	if(dsc==NULL) return;

	if(n>1327) {
		print2log(dsc,"MESSAGE of %i bytes TOO BIG TO BE DUMPED!! cutting it\n",n);
		n=1327;
	}

	switch(how) {
		case 0:
			for(i=e;i<n;i++) {
				if(i%4==0) { print2log(dsc," "); }
				print2log(dsc,"%02X",buf[i]);
			}
			break;
		case 1:
		case 3:
			for(i=e;i<n;i++) {
				if(i%4==0) { print2log(dsc," "); }
				if((how==1 && isprint((int)buf[i])) || (how!=1 && isprint((Byte)(~buf[i])))) {
					if(how==1) { print2log(dsc,"%c",buf[i]); }
					else { print2log(dsc,"%c",(Byte)(~buf[i])); }
				}
				else print2log(dsc,".");
			}
			break;
		case 2:
		case 4:
			for(i=e;i<n;i++) {
				if(i%4==0) { print2log(dsc," "); }
				if((how==2 && isprint((int)buf[i])) || (how!=2 && isprint((Byte)(~buf[i])))) {
					if(how==2) { print2log(dsc," %c",buf[i]); }
					else { print2log(dsc," %c",(Byte)(~buf[i])); }
				}
				else print2log(dsc," .");
			}
			break;
		case 5:
		case 6:
		case 7:
			print2log(dsc,"Offs");
			for(i=0; i<0x10; i++) {
				//0 1 2 3 ... A B ... F
				print2log(dsc," %2X",i);
			}
			for(i=e;i<n;i++) {
				if(!((i-e)%0x10)) { //end of line
					if((i-e)!=0) {  //not after first line
						print2log(dsc," ");
						for(j=i-0x10; j<i; j++) {
							if((how!=6 && isprint((int)buf[j])) || (how==6 && isprint((Byte)(~buf[j])))) {
								if(how!=6) { print2log(dsc,"%c",buf[j]); }
								else { print2log(dsc,"%c",(Byte)(~buf[j])); }
							}
							else print2log(dsc,".");
						}
						if(how==7) {
							print2log(dsc," ");
							for(j=i-0x10; j<i; j++) {
								if(isprint((Byte)(~buf[j]))) {
									print2log(dsc,"%c",(Byte)(~buf[j]));
								}
								else print2log(dsc,".");
							}
						}
					}
					print2log(dsc,"\n%04X",i-e); //offset
				}
				print2log(dsc," %02X",buf[i]);
			}
			if((n-j)<0x10) {
				for(i=(i-e)%0x10;i<0x10;i++) { print2log(dsc,"   ");	}
			}
			print2log(dsc," ");
			for(k=j; k<n; k++) {
				if((how!=6 && isprint((int)buf[k])) || (how==6 && isprint((Byte)(~buf[k])))) {
						if(how!=6) { print2log(dsc,"%c",buf[k]); }
						else { print2log(dsc,"%c",(Byte)(~buf[k])); }
				}
				else print2log(dsc,".");
			}
			if(how==7) {
				if((n-j)<0x10) {
					for(k=k; k<=(((n/0x10)+1)*0x10); k++) {
						print2log(dsc," ",k,n,((n/0x10)));
					}
				} else {
					print2log(dsc," ");
				}
				for(k=j; k<n; k++) {
					if((isprint(((Byte)(~buf[k]))))) {
						print2log(dsc,"%c",(Byte)(~buf[k]));
					}
					else print2log(dsc,".");
				}
			}
	}
	logflush(dsc);
#endif
}

void lognl(st_log * log) {
	print2log(log,"\n");
}

/**
error logging
*/
void logerr(st_log * log,char *msg) {
	plog(log,"%s\n",msg);
	plog(log," errno %i: %s\n",errno,strerror(errno));
}


#if 0

//Moved to useful.cpp

/**
	Quick way to get microseconds
*/
U32 get_microseconds() {
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return tv.tv_usec;
}

#endif

#ifdef _TESTDBG_
// g++ -Wall -g -D_TESTDBG_ stdebug.cpp -o test

int main() {

	st_log * log1;
	st_log * log2;
	st_log * system;

	DBG(5,"attempting to open the log file\n");

	log1=open_log("test",5,0);
	system=open_log("sys",5,DF_SYSLOG);

	stdebug_config->syslog_enabled=0x01;

	plog(system,"Hello world\n");
	logl(system,10,"This line will be not printed\n");
	logl(system,1,"This one yes\n");

	DBG(5,"attempting to print into the log file\n");

	stamp2log(log1);
	print2log(log1,"This is a test\n");

	rotate_log(log1,1);

	print2log(log1,"The test continues\n");

	rotate_log(log1,0);

	log2=open_log("maika/sordida",4,DF_HTML);

	print2log(log1,"I'm going to continue writting here\n");

	rotate_log(log1,0);
	rotate_log(log2,0);
	rotate_log(log2,0);
	rotate_log(log2,0);
	rotate_log(log2,0);
	rotate_log(log2,0);
	rotate_log(log2,0);
	rotate_log(log2,0);

	print2log(log2,"Now, I'm generating an <b>html</b> file\n");

	print2log(log1,"ABDCADFADSFASDFASFDDASFDASFASDFASFSAASDFASDFASDFASDFASDFASDFADFAS\n");
	fflush(log1->dsc);

	rotate_log(log1,0);

	char * kk="afjsakflñjasfdñalsdkfjskdlfñjasdfjj3\
	8p94r37u9jujujoiasdfujasdofuasdfuñjlkasfdhasjfkl\
	8p94r37u9jujujoiasdfujasdofuasdfuñjlkasfdhasjfkl\
	8p94r37u9jujujoiasdfujasdofuasdfuñjlkasfdhasjfkl\
	ñasdfhasfpiuoeñawhfe43w89piyehyhfwe";

	char * kk3="hola whola ";

	dumpbuf(log1,(Byte *)kk,strlen(kk));
	lognl(log1);

	dumpbuf(log1,(Byte *)kk3,strlen(kk3));
	lognl(log1);

	print2log(log1,"The test ends _here_\n");

	close_log(log1);
	close_log(log2);
	close_log(system);

	return 0;
}


#endif

#if 0

/*-----------------------------------------------
  Prints the ip into the log file
-----------------------------------------------*/
void ip2log_old(FILE * file, U32 ip, U16 port) {
#ifdef _DEBUG_LOGS
	in_addr cip;
	char mip[16];
	cip.s_addr=(unsigned long)ip;
	strcpy(mip, inet_ntoa(cip));
	print2log(file,"[%s:%i] ",mip,ntohs(port));
#endif
}

void ip2log(FILE * file, st_uru_client * s) {
#ifdef _DEBUG_LOGS
	in_addr cip;
	char mip[16];
	cip.s_addr=(unsigned long)s->client_ip;
	strcpy(mip, inet_ntoa(cip));
	print2log(file,"[%s:%i] ",mip,ntohs(s->client_port));
#endif
}

#endif
