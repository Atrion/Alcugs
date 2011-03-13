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

/** Alcugs logging system
*/

#define __U_ALCLOG_ID $Id$
//#define _DBG_LEVEL_ 10
#include "alcdefs.h"
#include "alclog.h"

#include "alcos.h"
#include "alcexception.h"
#include "alcmain.h"

#define __STDC_FORMAT_MACROS
#include <iostream>
#include <sys/stat.h>
#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <inttypes.h>


namespace alc {

tLogConfig::tLogConfig(void) : path("log/"), build("Alcugs logging system")
{
	verboseLevel=3;
	n_files2rotate=5;
	rotate_size=2*1024*1024;
	creation_mask=00750;
}



/** tLog class
 Opens a log file to write,
 flags are detailed in the alclog.h file
*/
void tLog::init(void)
{
	tvLogConfig = &(alcGetMain()->logCfg);
	dsc=NULL;
	flags = 0;
	count = 0;
}

void tLog::open(const tString &name, uint16_t newFlags) {
	if(dsc) close();
	count = 0;
	flags = newFlags;

	if(!name.isEmpty() && tvLogConfig->n_files2rotate > 0) {
		if(name.getAt(0)!='/') {
			fullpath = tvLogConfig->path + name; // a relative path, prepend the log directory
		} else {
			fullpath = name;
		}
		alcMkdir(fullpath.dirname(), tvLogConfig->creation_mask);
		DBG(5, "Full path is %s, flags are 0x%08X\n", fullpath.c_str(), flags);

		//rotation
		if((!(flags & DF_APPEND)) || (flags & DF_HTML)) {
			rotate(true);
		} else {
			rotate();
		}

		dsc=fopen(fullpath.c_str(),"a");
		if(dsc==NULL)
			throw txNotFound(_WHERE("Can not open %s", fullpath.c_str()));
		setCloseOnExec(fileno(dsc)); // close file when forking a game server
		if(flags & DF_HTML) {
			printHtmlHead(tvLogConfig->build);
		}
	}
	else {
		DBG(5, "No file opened, flags are 0x%08X\n", flags);
		fullpath = "";
		dsc = NULL;
	}
}

void tLog::rotate(bool force) {
	struct stat file_stats;
	
	if(fullpath.isEmpty() || tvLogConfig->n_files2rotate<=0) {
		return;
	}

	if(stat(fullpath.c_str(),&file_stats)!=0) {
		DBG(6,"file not found, cannot rotate!\n");
		return;
	}
	if(file_stats.st_size < tvLogConfig->rotate_size && !force) {
		return;
	}
	
	// ok, go ahead and rotate
	bool wasOpen = (dsc != NULL);
	if(dsc!=NULL) {
		DBG(5,"closing file %s to rotate...\n",fullpath.c_str());
		if(flags & DF_HTML) {
			fprintf(dsc,"</body></html>\n");
		}
		fclose(dsc);
		dsc = NULL;
	}

	//rotation
	tString prefix = alcStripExt(fullpath), suffix = alcGetExt(fullpath);
	if (suffix.size()) suffix = "."+suffix;
	for(unsigned int i=tvLogConfig->n_files2rotate; i>0; i--) {
		tString oldName = prefix;
		if(i-1 != 0) {
			oldName.printf(".%i", i-1);
		}
		oldName += suffix;
		
		tString newName = prefix;
		newName.printf(".%i", i);
		newName += suffix;
		
		if(stat(oldName.c_str(),&file_stats)==0) {
			if(i==tvLogConfig->n_files2rotate) {
				DBG(5,"deleting %s\n",oldName.c_str());
				unlink(oldName.c_str());
			} else {
				DBG(5,"rename from %s to %s\n",oldName.c_str(),newName.c_str());
				rename(oldName.c_str(), newName.c_str());
			}
		}
	}

	if(wasOpen) {
		DBG(5,"opening file %s...\n",fullpath.c_str());
		dsc=fopen(fullpath.c_str(),"w");
		if(dsc==NULL)
			throw txBase(_WHERE("fopen error"));
		setCloseOnExec(fileno(dsc)); // close file when forking a game server
		if(flags & DF_HTML) {
			printHtmlHead(tvLogConfig->build);
		}
	}
}

/**
	Close the log file
*/
void tLog::close(bool silent) {
	if(dsc!=NULL) {
		DBG(5,"closing file %s...\n",fullpath.c_str());
		if(flags & DF_HTML && !silent) { // a silent close is used during forking - we must not write this line then
			fprintf(dsc,"</body></html>\n");
		}
		fclose(dsc);
		dsc=NULL;
	}
	flags=0;
}


void tLog::printHtmlHead(const tString &generator) {
	fprintf(dsc,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n\
<html><head>\n<title>%s</title>\n\
<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n\
<meta HTTP-EQUIV=\"Generator\" CONTENT=\"%s\">\n\
</head>\n<body>" ,fullpath.c_str(), generator.c_str());
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
	if(tvLogConfig->n_files2rotate && dsc!=NULL) {
		fputs(str.c_str(),dsc);
	}

	//then print to the specific device
	if(!(flags & DF_HTML)) {
		if(tvLogConfig->verboseLevel>=1 && (flags & DF_STDERR)) {
			fputs(str.c_str(),stderr);     //stderr messages
		} else if(tvLogConfig->verboseLevel>=2 && (flags & DF_STDOUT)) {
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
	checkRotate(250);
	if (this->flags & DF_NOSTAMP) { return; }
	this->print("(%s)[%"PRIu64"] ",tTime::now().str().c_str(),static_cast<uint64_t>(alcGetSelfThreadId()));
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
}

/**
	flush all the streams we write to
*/
void tLog::flush() const {
	if(this->dsc!=NULL) {
		fflush(this->dsc);
	}
	if(!(flags & DF_HTML)) {
		if(this->flags & DF_STDERR && tvLogConfig->verboseLevel>=1) {  fflush(stderr); }
		if((this->flags & DF_STDOUT && tvLogConfig->verboseLevel>=2) || tvLogConfig->verboseLevel==3) {  fflush(stdout); }
	}
}


void tLog::dumpbuf(tBBuf & t, size_t n, size_t e, uint8_t how) const {
	if(n==0) n=t.size();
	size_t where=t.tell();
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
void tLog::dumpbuf(const void * bufv, size_t n, size_t e, uint8_t how) const {
	unsigned int i=0,j=0,k=0;

	if(n>2048) {
		this->print("MESSAGE of %i bytes TOO BIG TO BE DUMPED!! cutting it\n",n);
		n=2048;
	}
	
	const uint8_t *buf = reinterpret_cast<const uint8_t*>(bufv); // acess the buffer byte-wise

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

bool tLog::doesPrint(void) const
{
	if(this->dsc!=NULL) return true;
	if(!(flags & DF_HTML)) {
		DBG(7, "dsc is NULL, no HTML, checking stdout/stderr\n");
		if(this->flags & DF_STDERR && tvLogConfig->verboseLevel>=1) return true;
		if((this->flags & DF_STDOUT && tvLogConfig->verboseLevel>=2) || tvLogConfig->verboseLevel==3) return true;
	}
	else {
		DBG(7, "dsc is NULL, and HTML flag set - no printing\n");
	}
	return false;
}

tString tLog::getDir(void) const
{
	if (!dsc) return tvLogConfig->path;
	return tString(fullpath).dirname()+"/";
}


}

