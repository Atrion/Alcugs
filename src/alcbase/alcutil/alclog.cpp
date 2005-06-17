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

/** UNET3 debugging system

*/

#define __U_ALCLOG_ID $Id$

//#define _DBG_LEVEL_ 10

#define _DEBUG_DUMP_PACKETS

#include "alcugs.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>

#include "alcdebug.h"

namespace alc {

tLog * lstd=NULL;
tLog * lerr=NULL;

typedef struct {
	//files
	Byte silent; //!< set what to print to the console
	//silent
	// 0 - print all, ignoring if them have stdout or stderr flags, html are not print.
	// 1 - print only msgs with stderr or stdout flags.
	// 2 - print only msgs with stderr flags
	// 3 - don't print nothing

	//global verbose level vs silent
	//  0 - 3
	//  1 - 2
	//  2 - 1
	// >3 - 0

	int n_files2rotate; //!< set number the files to rotate
												/* 0 - logging disabled
												   1 - one file (old behaviour)
													 >2 - rotate logs
												*/
	char * path; //!<path to the log directory
	int rotate_size; //!< maxium size of a file, if reached, file will be rotated
	mode_t creation_mask; //!< default permissions mask
	char level; //!< current logging level (0 - disabled, 1 minimal, 6 huge)
	U16 log_flags; //!< default flags assigned to a log file on creation
	//build vars
	char * build; //!< build
	//syslog
	char * syslogname; //<! the syslog name
	char syslog_enabled; //<! enable syslog logging? (0x01 yes, 0x0 no)
	//db
	char * dbhost; //<!database params
	U16 dbport;
	char * dbname;
	char * dbuser;
	char * dbpasswd;
	char * dbeventtable;
	char db_enabled; //<! 0x01 enabled, 0x00 disabled
	//unet
	char * host; //<! udp/tcp listener
	U16 port;
	char protocol; //UDP, TCP <! 0x00 disabled, 0x01 udp, 0x02 tcp
	int n_logs; //How many log files are open
	tLog ** logs; //Pointer to each one
} tLogConfig;

tLogConfig * tvLogConfig=NULL;

void alcLogSetDefaults() {
	if(tvLogConfig==NULL) {
		tvLogConfig=(tLogConfig *)malloc(sizeof(tLogConfig) * 1);
	}
	if(tvLogConfig!=NULL) {
	memset(tvLogConfig,0,sizeof(tvLogConfig));
	tvLogConfig->silent=0;
	tvLogConfig->n_files2rotate=5;
	tvLogConfig->path="log/";
	tvLogConfig->rotate_size=2*1024*1024;
	tvLogConfig->creation_mask=00750;
	tvLogConfig->level=6;
	tvLogConfig->log_flags= DF_DEFSTDOUT | DF_STAMP | DF_IP; // | DF_ANOY;
	tvLogConfig->build="Alcugs H'uru server logging system";
	//syslog
	tvLogConfig->syslogname="alcugs";
	tvLogConfig->syslog_enabled=0x00;
	//db
	tvLogConfig->dbhost=NULL;
	tvLogConfig->dbport=0;
	tvLogConfig->dbname="uru_events";
	tvLogConfig->dbuser="uru";
	tvLogConfig->dbpasswd="";
	tvLogConfig->dbeventtable="events";
	tvLogConfig->db_enabled=0x00;
	//unet
	tvLogConfig->host="localhost";
	tvLogConfig->port=9000;
	tvLogConfig->protocol=0x00;
	//track logs
	tvLogConfig->n_logs=0;
	tvLogConfig->logs=NULL;
	}
}

void alcLogShutdown(bool silent) {
	int i;
	if(lerr!=NULL) {
		lerr->close(silent);
		delete lerr;
		lerr=NULL;
	}
	if(lstd!=NULL) {
		lstd->close(silent);
		delete lstd;
		lstd=NULL;
	}
	for(i=0; i<tvLogConfig->n_logs; i++) {
		if(tvLogConfig->logs[i]!=NULL) {
			tvLogConfig->logs[i]->close(silent);
			delete tvLogConfig->logs[i];
		}
	}
	if(tvLogConfig->logs!=NULL) {
		free((void *)tvLogConfig->logs);
		tvLogConfig->logs=NULL;
	}
	if(tvLogConfig!=NULL) {
		free((void *)tvLogConfig);
		tvLogConfig=NULL;
	}
}

void alcLogInit() {
	DBG(5,"alcLogInit()..\n");
	if(tvLogConfig==NULL) {
		alcLogSetDefaults();
	}
}

void alcLogOpenStdLogs() {
	if(lerr==NULL) {
		lerr=new tLog();
		lerr->open("error.log",2,DF_STDERR);
	}
	if(lstd==NULL) {
		lstd=new tLog();
		lstd->open("uru.log",2,DF_STDOUT);
	}
}

char * alcHtmlGenerateHead(char * title,char * powered) {
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


/**
 Opens a log file to write,
 \return the log descriptor associated to the log file
 flags are detailed in the stdebug.h file
 \returns NULL if failed
*/
tLog::tLog(const char * name, char level, U16 flags) {
	dmalloc_verify(NULL);
	this->name=NULL;
	this->dsc=NULL;
	this->bdsc=NULL;
	this->level=0;
	this->flags = 0;
	this->facility = LOG_USER;
	this->priority = LOG_DEBUG;
	tLog::open(name,level,flags);
}

tLog::~tLog() {
	DBG(5,"~tLog()\n");
	dmalloc_verify(NULL);
	this->close();
	dmalloc_verify(NULL);
}

void tLog::open(const char * name, char level, U16 flags) {
	char * path=NULL;
	char * croak=NULL;
	int i,e,size;

	int f,found=-1;
	
	if(this->flags & DF_OPEN) return;

	for(f=0; f<tvLogConfig->n_logs; f++) {
		if(tvLogConfig->logs[f]==NULL) {
			found=f;
			break;
		}
	}
	
	this->flags = tvLogConfig->log_flags | flags;
	this->level = level;

	if(name!=NULL) {
		assert(this->name==NULL);
		this->name = (char *)malloc(sizeof(char) * (strlen(name) + 1));
		if(this->name==NULL) { throw txNoMem(_WHERE("")); }
		strcpy(this->name,name);
		DBG(5,"cont...\n");

		FILE * def=NULL;
		if(this->flags & DF_DEFSTDOUT) def=stdout;

		if(tvLogConfig->n_files2rotate<=0 || level==0) {
			this->dsc=def;
		} else {
			size=strlen(name) + strlen((const char *)tvLogConfig->path);
			DBG(6,"size is:%i\n",size);

			croak=(char *)malloc(sizeof(char) * (size+1+5));
			if(croak==NULL) throw txNoMem(_WHERE(""));
			DBG(6,"im here\n");
			path=(char *)malloc(sizeof(char) * (size+1));
			if(path==NULL) { free((void *)croak); throw txNoMem(_WHERE("")); }
			DBG(7,"here too\n");

			if(name[0]!='/') {
				strcpy(path,tvLogConfig->path);
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
					mkdir(croak,tvLogConfig->creation_mask);
					//e=0;
				}
			}

			if(croak!=NULL) { free((void *)croak); }

			//rotation
			if(!(this->flags & DF_APPEND) || (this->flags & DF_HTML)) {
				this->rotate(true);
			} else {
				this->rotate();
			}

			this->dsc=fopen(path,"a");
			if(this->dsc==NULL) {
				DBG(5,"ERR: Fatal - Cannot Open %s to write ",path);
				ERR(6,"returned error");
				this->dsc=def; //default standard output
				if(def==stdout) {
					DBG(6,"DBG: Sending debug messages to the stdout instead of %s\n",path);
				}
			} else {
				this->flags |= DF_OPEN;
				if(this->flags & DF_HTML) {
					fprintf(this->dsc,"%s",alcHtmlGenerateHead(this->name,tvLogConfig->build));
				}
			}
			if(path!=NULL) { free((void *)path); }
		}
	}

	if(found==-1) {
		tvLogConfig->n_logs++;
		tLog ** aux=NULL;
		aux=(tLog **)realloc((void *)tvLogConfig->logs,sizeof(tLog *) * tvLogConfig->n_logs);
		if(aux==NULL) {
			fprintf(stderr,"Failed allocating memory!\n");
			throw txNoMem(_WHERE(""));
		} else {
			tvLogConfig->logs=aux;
			tvLogConfig->logs[tvLogConfig->n_logs-1]=this;
		}
	} else {
		tvLogConfig->logs[found]=this;
	}
}

int tLog::rotate(bool force) {

	char * path=NULL;
	char * croak=NULL;
	char * croak2=NULL;
	char * gustavo=NULL;
	int i,size;
	int ret;

	struct stat file_stats;
	
	DBG(5,"init..\n");

	//if(!(this->flags & DF_OPEN)) return 0;

	DBG(5,"2..\n");
	
	if(tvLogConfig->n_files2rotate<=0 || this->level==0) {
		return 0;
	}
	
	DBG(5,"3..\n");

	size=strlen(this->name) + strlen((const char *)tvLogConfig->path);

	croak=(char *)malloc(sizeof(char) * (size+1+5));
	if(croak==NULL) return -1;
	path=(char *)malloc(sizeof(char) * (size+1));
	if(path==NULL) { free((void *)croak); return -1; }
	croak2=(char *)malloc(sizeof(char) * (size+1+5));
	if(croak2==NULL) { free((void *)path); free((void *)croak); return -1; }
	gustavo=(char *)malloc(sizeof(char) * (size+1+5));
	if(gustavo==NULL) { free((void *)path); free((void *)croak);
		free((void *)croak2); return -1; }

	DBG(5,"4..\n");

	if(this->name[0]!='/') {
		strcpy(path,tvLogConfig->path);
	} else {
		strcpy(path,"");
	}
	strcat(path,this->name);

	DBG(5,"path is:%s\n",path);

	if(stat(path,&file_stats)!=0) {
		DBG(6,"file not found, cannot rotate!\n");
		ret=-1;
	} else {

		if((int)file_stats.st_size<tvLogConfig->rotate_size && !force) {
			ret=0;
		} else {

			if(this->flags & DF_OPEN && this->dsc!=NULL && this->dsc!=stdout && this->dsc!=stderr) {
				DBG(5,"clossing file %s...\n",path);
				if(this->flags & DF_HTML) {
					fprintf(this->dsc,"</body></html>\n");
				}
				fclose(this->dsc);
			}

			//rotation
			for(i=tvLogConfig->n_files2rotate; i>0; i--) {
				sprintf(gustavo,"%s",path);
				alcStripExt(gustavo);
				if(i-1==0) {
					sprintf(croak,"%s",path);
				} else {
					if(strlen(alcGetExt(path))!=0) {
						sprintf(croak,"%s.%i.%s",gustavo,i-1,alcGetExt(path));
					} else {
						sprintf(croak,"%s.%i",gustavo,i-1);
					}
				}
				if(strlen(alcGetExt(path))!=0) {
					sprintf(croak2,"%s.%i.%s",gustavo,i,alcGetExt(path));
				} else {
					sprintf(croak2,"%s.%i",gustavo,i);
				}
				if(stat(croak,&file_stats)==0) {
					if(i==tvLogConfig->n_files2rotate) {
						DBG(5,"deleting %s\n",croak);
						unlink((const char *)croak);
					} else {
						DBG(5,"rename from %s to %s\n",croak,croak2);
						rename((const char *)croak,(const char *)croak2);
					}
				}
			}

			if(this->flags & DF_OPEN) {
				DBG(5,"opening file %s...\n",path);
				this->dsc=fopen(path,"w");
				if(this->dsc==NULL) {
					this->flags=this->flags & ~DF_OPEN;
					ret=-1;
					throw txBase(_WHERE("fopen error"));
				} else {
					if(this->flags & DF_HTML) {
						fprintf(this->dsc,"%s",alcHtmlGenerateHead(this->name,tvLogConfig->build));
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
void tLog::close(bool silent) {
	dmalloc_verify(NULL);
	int f;
	for(f=0; f<tvLogConfig->n_logs; f++) {
		if(tvLogConfig->logs[f]==this) {
			tvLogConfig->logs[f]=NULL;
			break;
		}
	}
	if(this->name!=NULL) DBG(6,"clossing log %s...\n",this->name);
	if(this->dsc!=NULL && this->dsc!=stdout && this->dsc!=stderr) {
		if(this->flags & DF_HTML && !silent) {
			fprintf(this->dsc,"</body></html>\n");
		}
		fclose(this->dsc);
		this->dsc=NULL;
	}
	if(this->name!=NULL) free((void *)this->name);
	this->name=NULL;
	this->flags=0;
}

/**
	Allows to print big raw messages to the file and stdout descriptors.
	This don't sends messages to syslog, database, or socket
	- there is no rotate check code -
*/
void tLog::print(const char * msg, ...) {
	va_list ap,ap2;

	va_start(ap,msg);
	va_start(ap2,msg);

	if(tvLogConfig->level>=this->level && this->level!=0) {

		//first print to the file
		if(tvLogConfig->n_files2rotate && this->dsc!=NULL && this->dsc!=stdout && this->dsc!=stderr) {
			vfprintf(this->dsc,msg,ap);
		}

		//then print to the specific device
		if(!(this->flags & DF_HTML) && !(this->flags & DF_NODUMP)) {
			if(tvLogConfig->silent<=2 && (this->flags & DF_STDERR)) {
				vfprintf(stderr,msg,ap2);     //stderr messages
			} else if(tvLogConfig->silent<=1 && (this->flags & DF_STDOUT)) {
				vfprintf(stdout,msg,ap2);      //stdout messages
			} else if(tvLogConfig->silent==0) {
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
void tLog::stamp() {
	static Byte count=0;

	count++;
	if(count>250) { count=0; this->rotate(false); }

	if(!(this->flags & DF_STAMP)) { return; }

	char c_time_aux[26];
	struct tm * tptr;
	time_t timestamp;

	struct timeval tv;

	time(&timestamp);
	gettimeofday(&tv,NULL);

	tptr=gmtime((const time_t *)&timestamp);

	strftime(c_time_aux,25,"(%Y:%m:%d:%H:%M:%S",tptr);

	this->print("%s.%06d) ",c_time_aux,tv.tv_usec);
}

/**
  Adds and an entry to the specified log
	db and syslog logging is enabled, if specified,
	timestamp is also enabled (size limitation of 2KBytes)
*/
void tLog::log(const char * msg, ...) {
	va_list ap;
	static char buf[2*1024];

	if(tvLogConfig->level>=this->level && this->level!=0) {

		va_start(ap,msg);
		vsnprintf(buf,sizeof(buf)-1,msg,ap);
		va_end(ap);

		this->stamp();
		this->print("%s",buf);

#ifdef __WIN32__
	//TODO, implement here, windoze based syslog logging, if it has something similar.
		if(this->flags & DF_ANOY) {
			MessageBox(NULL,buf,tvLogConfig->syslogname,0);
		}
#elif 0
		if(this->flags & DF_SYSLOG && tvLogConfig->syslog_enabled==0x01) {
			//log to the syslog
			openlog(tvLogConfig->syslogname, LOG_NDELAY | LOG_PID, LOG_AUTHPRIV); //LOG_USER
			syslog(LOG_DEBUG,"%s",buf);
			closelog();
		}
#endif

#ifdef _DBLOGGING_
		if(this->flags & DF_DB && tvLogConfig->db_enabled==0x01) {
			dblog(log,"generic","system","system","%s",buf);
		}
#endif

	}
}

//log with another loging level
void tLog::logl(char level,const char * msg, ...) {
	va_list ap;
	static char buf[1024];
	char tmp;

	if(tvLogConfig->level>=level && this->level!=0) {

		tmp=this->level;
		this->level=level;

		va_start(ap,msg);
		vsnprintf(buf,sizeof(buf)-1,msg,ap);
		va_end(ap);

		this->log("%s",buf);

		this->level=tmp;
	}
}

#ifdef WHEN_THE_COWS_FLY_AND_THE_FROGS_HAVE_HAIR
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
#endif

/**
	Fflush all the streams
*/
void tLog::flush() {
	if(this->dsc!=NULL) {
		fflush(this->dsc);
	}
	if(this->flags & DF_STDERR && tvLogConfig->silent<=2) {  fflush(stderr); }
	if((this->flags & DF_STDOUT && tvLogConfig->silent<=1) || tvLogConfig->silent==0) {  fflush(stdout); }
}


void tLog::dumpbuf(tBBuf & t, U32 n, U32 e, Byte how) {
	if(n==0) n=t.size();
	t.rewind();
	this->dumpbuf(t.read(n),n,e,how);
	t.rewind();
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
void tLog::dumpbuf(Byte * buf, U32 n, U32 e, Byte how) {
#ifdef _DEBUG_DUMP_PACKETS
	unsigned int i=0,j=0,k=0;

	if(n>2048) {
		this->print("MESSAGE of %i bytes TOO BIG TO BE DUMPED!! cutting it\n",n);
		n=2048;
	}

	switch(how) {
		case 0:
			for(i=e;i<n;i++) {
				if(i%4==0) { this->print(" "); }
				this->print("%02X",buf[i]);
			}
			break;
		case 1:
		case 3:
			for(i=e;i<n;i++) {
				if(i%4==0) { this->print(" "); }
				if((how==1 && isprint((int)buf[i])) || (how!=1 && isprint((Byte)(~buf[i])))) {
					if(how==1) { this->print("%c",buf[i]); }
					else { this->print("%c",(Byte)(~buf[i])); }
				}
				else this->print(".");
			}
			break;
		case 2:
		case 4:
			for(i=e;i<n;i++) {
				if(i%4==0) { this->print(" "); }
				if((how==2 && isprint((int)buf[i])) || (how!=2 && isprint((Byte)(~buf[i])))) {
					if(how==2) { this->print(" %c",buf[i]); }
					else { this->print(" %c",(Byte)(~buf[i])); }
				}
				else this->print(" .");
			}
			break;
		case 5:
		case 6:
		case 7:
			this->print("Offs");
			for(i=0; i<0x10; i++) {
				//0 1 2 3 ... A B ... F
				this->print(" %2X",i);
			}
			for(i=e;i<n;i++) {
				if(!((i-e)%0x10)) { //end of line
					if((i-e)!=0) {  //not after first line
						this->print(" ");
						for(j=i-0x10; j<i; j++) {
							if((how!=6 && isprint((int)buf[j])) || (how==6 && isprint((Byte)(~buf[j])))) {
								if(how!=6) { this->print("%c",buf[j]); }
								else { this->print("%c",(Byte)(~buf[j])); }
							}
							else this->print(".");
						}
						if(how==7) {
							this->print(" ");
							for(j=i-0x10; j<i; j++) {
								if(isprint((Byte)(~buf[j]))) {
									this->print("%c",(Byte)(~buf[j]));
								}
								else this->print(".");
							}
						}
					}
					this->print("\n%04X",i-e); //offset
				}
				this->print(" %02X",buf[i]);
			}
			if((n-j)<0x10) {
				for(i=(i-e)%0x10;i<0x10;i++) { this->print("   ");	}
			}
			this->print(" ");
			for(k=j; k<n; k++) {
				if((how!=6 && isprint((int)buf[k])) || (how==6 && isprint((Byte)(~buf[k])))) {
						if(how!=6) { this->print("%c",buf[k]); }
						else { this->print("%c",(Byte)(~buf[k])); }
				}
				else this->print(".");
			}
			if(how==7) {
				if((n-j)<0x10) {
					for(k=k; k<=(((n/0x10)+1)*0x10); k++) {
						this->print(" ",k,n,((n/0x10)));
					}
				} else {
					this->print(" ");
				}
				for(k=j; k<n; k++) {
					if((isprint(((Byte)(~buf[k]))))) {
						this->print("%c",(Byte)(~buf[k]));
					}
					else this->print(".");
				}
			}
	}
	this->flush();
#endif
}

void tLog::nl() {
	this->print("\n");
}

/**
error logging
*/
void tLog::logerr(char *msg) {
	this->log("%s\n",msg);
	this->log(" errno %i: %s\n",errno,strerror(errno));
}


}

