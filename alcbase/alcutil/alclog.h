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

#ifndef __U_ALCLOG_H
#define __U_ALCLOG_H

#include "alctypes.h"
#include "alcutil/alcthread.h"

namespace alc {

#define DF_HTML      0x0002 // HTML files (disables console dumping, forces rotation on open, uses full instead of line buffering)
#define DF_STDOUT    0x0008 // mirror to stdout if verboseLevel is high enough
#define DF_STDERR    0x0010 // mirror to stderr if verboseLevel is high enough
#define DF_APPEND    0x0040 // append to existing file on open
/* 
	0x01 file is open, (internal flag)
	0x02 is an html file,
	0x04 mirror events to syslog (unimplemented)
	0x08 mirror to stdout
	0x10 mirror to stderr
	0x20 default to stdout
	0x40 don't rotate, append instead (only works on open_log_file();)
	0x80 log events into the local database (not implemented, and module
			needs to be compiled with db suppport enabled)
	0x100 send events to the uru_log daemon (not implemented, yet)
	0x200 send events to a remote udp listener. (not implemented)
	0x400 send events to a remote tcp client. (not implemented)
		(the last 3 ones, need to be compiled with unet support)
	0x800 enable timestamp logging
	0x1000 enable ip logging
	0x2000 anoys the user with message boxes
	0x4000 avoids dumping to the console
*/

class tLog;

//! Helper class to manage logging settings - instanciated by tAlcMain, there's no point in having another instance (it won't be used)
class tLogConfig {
public:
	tLogConfig(void); // Constructor - fills with default values
	
	//// settings
	//files
	unsigned short int verboseLevel; //!< set what to print to the console
	//silent
	// 3 - print all, ignoring if them have stdout or stderr flags, html are not print.
	// 2 - print only msgs with stderr or stdout flags.
	// 1 - print only msgs with stderr flags
	// 0 - don't print nothing

	unsigned int n_files2rotate; //!< set number the files to rotate
	/*  0 - logging disabled
		1 - one file (old behaviour)
			>=2 - rotate logs
	*/
	
	// general stuff
	tString path; //!<path to the log directory
	__off_t rotate_size; //!< maxium size of a file, if reached, file will be rotated (__off_t is "type of file sizes")
	mode_t creation_mask; //!< default permissions mask
	//build vars
	tString build;
private:
	FORBID_CLASS_COPY(tLogConfig)
};

class tLog {
public:
	tLog(const tString &name,uint16_t newFlags=0) {
		init();
		open(name,newFlags);
	}
	tLog(uint16_t newFlags=0) {
		init();
		open(newFlags);
	}
	~tLog() { close(); }
	void open(const tString &name,uint16_t newFlags=0);
	void open(uint16_t newFlags=0) { open(tString(), newFlags); } // useful if you only want to open stdout/stderr
	void rotate(bool force=false);
	void close(bool silent=false);
	void checkRotate(unsigned int maxCount) {
		countEx.lock();
		++count;
		if(count>maxCount) {
			count=0;
			countEx.unlock();
			rotate(false);
		}
		else countEx.unlock();
	}

	void print(const char * msg, ...) const GNUC_FORMAT_CHECK(printf, 2, 3);
	void print(const tString &str) const;
	void stamp();
	void log(const char * msg, ...) GNUC_FORMAT_CHECK(printf, 2, 3);

	void dumpbuf(const void * buf, size_t n, size_t e=0,uint8_t how=7) const;
	void dumpbuf(tBBuf &t, size_t n=0, size_t e=0,uint8_t how=7) const;

	void nl() const; //!< print newline

	bool doesPrint(void) const;
	tString getDir(void) const; //!< return directory of log file, or default directory if closed

private:
	void init(void);
	void printHtmlHead(const tString &generator);
	
	tLogConfig *tvLogConfig;
	tString fullpath;
	FILE * dsc;
	uint16_t flags; //see above (DF_*)
	unsigned int count; // counter used for rotate checking, protected by spinlock
	tSpinEx countEx;
	
	FORBID_CLASS_COPY(tLog)
};

}

#endif
