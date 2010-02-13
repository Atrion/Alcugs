/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs Server Team                           *
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

/** Alcugs debugging system
*/

#define __U_ALCLOG_ID $Id$

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#include <sys/stat.h>
#include <cerrno>
#include <cstdarg>

#ifndef __WIN32__
#include <syslog.h>
#endif

#include "alcdebug.h"

namespace alc {

tLogConfig::tLogConfig(void) : path("log/"), build("Alcugs logging system")
{
	//memset(tvLogConfig,0,sizeof(tvLogConfig));
	verboseLevel=3;
	n_files2rotate=5;
	rotate_size=2*1024*1024;
	creation_mask=00750;
	//syslog
	/*alcStrncpy(syslogname, "alcugs", 99);
	syslog_enabled=0x00;
	//db
	alcStrncpy(dbhost, "", 99);
	dbport=0;
	alcStrncpy(dbname, "uru_events", 99);
	alcStrncpy(dbuser, "uru", 99);
	alcStrncpy(dbpasswd, "", 99);
	alcStrncpy(dbeventtable, "events", 99);
	db_enabled=0x00;
	//unet
	alcStrncpy(host, "localhost", 99);
	port=9000;
	protocol=0x00;*/
}

void tLogConfig::addLog(tLog *log)
{
	logs.push_back(log);
}

void tLogConfig::removeLog(tLog *log)
{
	for (tLogList::iterator it = logs.begin(); it != logs.end(); ++it) {
		if (*it == log) {
			logs.erase(it);
			return;
		}
	}
}

void tLogConfig::forceCloseAllLogs(void) {
	for (tLogList::iterator it = logs.begin(); it != logs.end(); ++it)
		(*it)->close();
}



/** tLog class
 Opens a log file to write,
 flags are detailed in the alclog.h file
*/
tLog::tLog(const char * name, U16 flags) {
	tvLogConfig = &(alcGetMain()->logCfg);
	tvLogConfig->addLog(this);
	this->name=NULL;
	this->fullpath=NULL;
	this->dsc=NULL;
	this->bdsc=NULL;
	this->flags = 0;
	this->facility = LOG_USER;
	this->priority = LOG_DEBUG;
	count = 0;
	tLog::open(name,flags);
}

tLog::~tLog() {
	this->close();
	tvLogConfig->removeLog(this);
}

void tLog::open(const char * name, U16 flags) {
	char * path=NULL;
	char * croak=NULL;
	int i,e,size;

	if(this->flags & DF_OPEN) close();
	count = 0;
	this->flags = flags;

	if(name!=NULL && tvLogConfig->n_files2rotate > 0) {
		assert(this->name==NULL);
		this->name = static_cast<char *>(malloc(sizeof(char) * (strlen(name) + 1)));
		if(this->name==NULL) { throw txNoMem(_WHERE("")); }
		strcpy(this->name,name);
		DBG(5,"cont...\n");

		size=strlen(name) + tvLogConfig->path.size();
		DBG(6,"size is:%i\n",size);

		croak=static_cast<char *>(malloc(sizeof(char) * (size+1+5)));
		if(croak==NULL) throw txNoMem(_WHERE(""));
		DBG(6,"im here\n");
		path=static_cast<char *>(malloc(sizeof(char) * (size+1)));
		if(path==NULL) { free(croak); throw txNoMem(_WHERE("")); }
		DBG(7,"here too\n");

		if(name[0]!='/') {
			strcpy(path,tvLogConfig->path.c_str());
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

		free(croak);

		//rotation
		if(!(this->flags & DF_APPEND) || (this->flags & DF_HTML)) {
			this->rotate(true);
		} else {
			this->rotate();
		}
		
		// preserve the path
		free(fullpath);
		fullpath=static_cast<char *>(malloc(sizeof(char) * (strlen(path)+1)));
		if (!fullpath) throw txNoMem(_WHERE(""));
		strcpy(fullpath, path);
		free(path);

		this->dsc=fopen(fullpath,"a");
		if(this->dsc==NULL)
			throw txNotFound(_WHERE("Can not open %s", fullpath));
		this->flags |= DF_OPEN;
		if(this->flags & DF_HTML) {
			printHtmlHead(tvLogConfig->build);
		}
	}
	else
		this->dsc = NULL;
}

void tLog::rotate(bool force) {

	char * path=NULL;
	char * croak=NULL;
	char * croak2=NULL;
	char * gustavo=NULL;
	U32 i,size;

	struct stat file_stats;
	
	DBG(5,"init..\n");

	//if(!(this->flags & DF_OPEN)) return 0;

	DBG(5,"2..\n");
	
	if(this->name==NULL || tvLogConfig->n_files2rotate<=0) {
		return;
	}
	
	DBG(5,"3..\n");

	size=strlen(this->name) + tvLogConfig->path.size();

	croak=static_cast<char *>(malloc(sizeof(char) * (size+1+5)));
	if(croak==NULL) return;
	path=static_cast<char *>(malloc(sizeof(char) * (size+1)));
	if(path==NULL) { free(croak); return; }
	croak2=static_cast<char *>(malloc(sizeof(char) * (size+1+5)));
	if(croak2==NULL) { free(path); free(croak); return; }
	gustavo=static_cast<char *>(malloc(sizeof(char) * (size+1+5)));
	if(gustavo==NULL) { free(path); free(croak);
		free(croak2); return; }

	DBG(5,"4..\n");

	if(this->name[0]!='/') {
		strcpy(path,tvLogConfig->path.c_str());
	} else {
		strcpy(path,"");
	}
	strcat(path,this->name);

	DBG(5,"path is:%s\n",path);

	if(stat(path,&file_stats)!=0) {
		DBG(6,"file not found, cannot rotate!\n");
	} else {

		if(file_stats.st_size<tvLogConfig->rotate_size && !force) {
			;
		} else {

			if(this->flags & DF_OPEN && this->dsc!=NULL && this->dsc!=stdout && this->dsc!=stderr) {
				DBG(5,"closing file %s...\n",path);
				if(this->flags & DF_HTML) {
					fprintf(this->dsc,"</body></html>\n");
				}
				fclose(this->dsc);
			}

			//rotation
			for(i=tvLogConfig->n_files2rotate; i>0; i--) {
				snprintf(gustavo,size+1+5,"%s",path);
				alcStripExt(gustavo);
				if(i-1==0) {
					snprintf(croak,size+1+5,"%s",path);
				} else {
					if(alcGetExt(path).size()!=0) {
						snprintf(croak,size+1+5,"%s.%i.%s",gustavo,i-1,alcGetExt(path).c_str());
					} else {
						snprintf(croak,size+1+5,"%s.%i",gustavo,i-1);
					}
				}
				if(alcGetExt(path).size()!=0) {
					snprintf(croak2,size+1+5,"%s.%i.%s",gustavo,i,alcGetExt(path).c_str());
				} else {
					snprintf(croak2,size+1+5,"%s.%i",gustavo,i);
				}
				if(stat(croak,&file_stats)==0) {
					if(i==tvLogConfig->n_files2rotate) {
						DBG(5,"deleting %s\n",croak);
						unlink(croak);
					} else {
						DBG(5,"rename from %s to %s\n",croak,croak2);
						rename(croak, croak2);
					}
				}
			}

			if(this->flags & DF_OPEN) {
				DBG(5,"opening file %s...\n",path);
				this->dsc=fopen(path,"w");
				if(this->dsc==NULL) {
					this->flags=this->flags & ~DF_OPEN;
					throw txBase(_WHERE("fopen error"));
				} else {
					if(this->flags & DF_HTML) {
						printHtmlHead(tvLogConfig->build);
					}
				}
			}
		}
	}

	free(croak);
	free(path);
	free(croak2);
	free(gustavo);

	//return ret;
}

/**
	Close the log file
*/
void tLog::close(bool silent) {
	if(this->name!=NULL) { DBG(1,"closing log %s...\n",this->name); }
	if(this->dsc!=NULL && this->dsc!=stdout && this->dsc!=stderr) {
		if(this->flags & DF_HTML && !silent) { // a silent close is used during forking - we must not write this line then
			fprintf(this->dsc,"</body></html>\n");
		}
		fclose(this->dsc);
		this->dsc=NULL;
	}
	free(this->name);
	free(this->fullpath);
	this->name=NULL;
	this->fullpath=NULL;
	this->flags=0;
}


void tLog::printHtmlHead(const tString &generator) {
	fprintf(dsc,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n\
<html><head>\n<title>%s</title>\n\
<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n\
<meta HTTP-EQUIV=\"Generator\" CONTENT=\"%s\">\n\
</head>\n<body>" ,name, generator.c_str());
}

/**
	Allows to print big raw messages to the file and stdout descriptors.
	This don't sends messages to syslog, database, or socket
	- there is no rotate check code -
*/
void tLog::print(const char * msg, ...) const {
	va_list ap;
	tString buf;
	va_start(ap,msg);
	buf.vprintf(msg,ap);
	va_end(ap);
	print(buf);
}

void tLog::print(const tString &str) const
{
	//first print to the file
	if(tvLogConfig->n_files2rotate && this->dsc!=NULL && this->dsc!=stdout && this->dsc!=stderr) {
		fputs(str.c_str(),this->dsc);
	}

	//then print to the specific device
	if(!(this->flags & DF_HTML)) {
		if(tvLogConfig->verboseLevel>=1 && (this->flags & DF_STDERR)) {
			fputs(str.c_str(),stderr);     //stderr messages
		} else if(tvLogConfig->verboseLevel>=2 && (this->flags & DF_STDOUT)) {
			fputs(str.c_str(),stdout);      //stdout messages
		} else if(tvLogConfig->verboseLevel==3) {
			fputs(str.c_str(),stdout);      //print all (muhahahaha)
		}
	}
}

/**
  Prints a timestamp into the log file
	rotates the log if it is required
	(the rotation check is done, every 250 calls to this function)
*/
void tLog::stamp() {
	++count;
	if(count>250) { count=0; this->rotate(false); }

	if (this->flags & DF_NOSTAMP) { return; }

	this->print("(%s)[%d] ",alcGetStrTime().c_str(),alcGetSelfThreadId());
}

/**
  Adds and an entry to the specified log
	db and syslog logging is enabled, if specified,
	timestamp is also enabled (size limitation of 2KBytes)
*/
void tLog::log(const char * msg, ...) {
	va_list ap;
	tString buf;
	va_start(ap,msg);
	buf.vprintf(msg,ap);
	va_end(ap);

	this->stamp();
	this->print(buf);

#ifdef __WIN32__
// implement here, windoze based syslog logging, if it has something similar.
	if(this->flags & DF_ANOY) {
		MessageBox(NULL,buf.c_str(),tvLogConfig->syslogname,0);
	}
#elif 0
	if(this->flags & DF_SYSLOG && tvLogConfig->syslog_enabled==0x01) {
		//log to the syslog
		openlog(tvLogConfig->syslogname, LOG_NDELAY | LOG_PID, LOG_AUTHPRIV); //LOG_USER
		syslog(LOG_DEBUG,"%s",buf.c_str());
		closelog();
	}
#endif

#ifdef _DBLOGGING_
	if(this->flags & DF_DB && tvLogConfig->db_enabled==0x01) {
		dblog(log,"generic","system","system","%s",buf.c_str());
	}
#endif
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
		//database based log system
		//will work only, if the user compiles this module with the _DBLOGGING_ option
#endif
}
/**
	socket based logging, send all debugging messages to an udp listener, or spawn
	a new thread to listen to incoming tcp connections.
*/
#endif

/**
	flush all the streams we write to
*/
void tLog::flush() const {
	if(this->dsc!=NULL) {
		fflush(this->dsc);
	}
	if(this->flags & DF_STDERR && tvLogConfig->verboseLevel>=1) {  fflush(stderr); }
	if((this->flags & DF_STDOUT && tvLogConfig->verboseLevel>=2) || tvLogConfig->verboseLevel==3) {  fflush(stdout); }
}


void tLog::dumpbuf(tBBuf & t, U32 n, U32 e, Byte how) const {
	if(n==0) n=t.size();
	U32 where=t.tell();
	t.rewind();
	this->dumpbuf(t.read(n),n,e,how);
	t.set(where);
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
void tLog::dumpbuf(const Byte * buf, U32 n, U32 e, Byte how) const {
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
				if((how==1 && isprint(buf[i])) || (how!=1 && isprint((~buf[i])))) {
					if(how==1) { this->print("%c",buf[i]); }
					else { this->print("%c",(~buf[i])); }
				}
				else this->print(".");
			}
			break;
		case 2:
		case 4:
			for(i=e;i<n;i++) {
				if(i%4==0) { this->print(" "); }
				if((how==2 && isprint(buf[i])) || (how!=2 && isprint((~buf[i])))) {
					if(how==2) { this->print(" %c",buf[i]); }
					else { this->print(" %c",(~buf[i])); }
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
							if((how!=6 && isprint(buf[j])) || (how==6 && isprint((~buf[j])))) {
								if(how!=6) { this->print("%c",buf[j]); }
								else { this->print("%c",(~buf[j])); }
							}
							else this->print(".");
						}
						if(how==7) {
							this->print(" ");
							for(j=i-0x10; j<i; j++) {
								if(isprint((~buf[j]))) {
									this->print("%c",(~buf[j]));
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
				if((how!=6 && isprint(buf[k])) || (how==6 && isprint(~buf[k]))) {
						if(how!=6) { this->print("%c",buf[k]); }
						else { this->print("%c",(~buf[k])); }
				}
				else this->print(".");
			}
			if(how==7) {
				if((n-j)<0x10) {
					for(k=k; k<=(((n/0x10)+1)*0x10); k++) {
						this->print(" ",k,n,(n/0x10));
					}
				} else {
					this->print(" ");
				}
				for(k=j; k<n; k++) {
					if((isprint((~buf[k])))) {
						this->print("%c",(~buf[k]));
					}
					else this->print(".");
				}
			}
	}
	this->flush();
}

void tLog::nl() const {
	this->print("\n");
}

void tLog::logErr(const char *msg) {
	this->log("%s\n",msg);
	this->log(" errno %i: %s\n",errno,strerror(errno));
}

bool tLog::doesPrint(void) const
{
	if(this->dsc!=NULL) return true;
	if(this->flags & DF_STDERR && tvLogConfig->verboseLevel>=1) return true;
	if((this->flags & DF_STDOUT && tvLogConfig->verboseLevel>=2) || tvLogConfig->verboseLevel==3) return true;
	return false;
}

tString tLog::getDir(void) const
{
	if (!fullpath) return tvLogConfig->path;
	return tString(fullpath).dirname()+"/";
}


}

