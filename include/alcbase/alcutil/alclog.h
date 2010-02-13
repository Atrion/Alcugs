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

/* UNET 3 debbuging system */

#ifndef __U_ALCLOG_H
#define __U_ALCLOG_H
#define __U_ALCLOG_H_ID $Id$

namespace alc {

#define DF_OPEN      0x0001
#define DF_HTML      0x0002
#define DF_STDOUT    0x0008
#define DF_STDERR    0x0010
#define DF_APPEND    0x0040
#define DF_NOSTAMP   0x0800
//#define DF_SYSLOG    0x0004
//#define DF_DB        0x0080
//#define DF_LOGD      0x0100
//#define DF_UDP       0x0200
//#define DF_TCP       0x0400
//#define DF_IP        0x1000
//#define DF_ANOY      0x2000
//#define DF_NODUMP    0x4000
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
	void forceCloseAllLogs(void);
	void addLog(tLog *log);
	void removeLog(tLog *log);
	
	//// settings
	//files
	Byte verboseLevel; //!< set what to print to the console
	//silent
	// 3 - print all, ignoring if them have stdout or stderr flags, html are not print.
	// 2 - print only msgs with stderr or stdout flags.
	// 1 - print only msgs with stderr flags
	// 0 - don't print nothing

	U32 n_files2rotate; //!< set number the files to rotate
	/*  0 - logging disabled
		1 - one file (old behaviour)
			>=2 - rotate logs
	*/
	
	// general stuff
	tString path; //!<path to the log directory
	int rotate_size; //!< maxium size of a file, if reached, file will be rotated
	mode_t creation_mask; //!< default permissions mask
	//build vars
	tString build;
	//syslog
	/*char syslogname[100]; //!< the syslog name
	char syslog_enabled; //!< enable syslog logging? (0x01 yes, 0x0 no)
	//db
	char dbhost[100]; //!<database params
	U16 dbport;
	char dbname[100];
	char dbuser[100];
	char dbpasswd[100];
	char dbeventtable[100];
	char db_enabled; //!< 0x01 enabled, 0x00 disabled
	//unet
	char host[100]; //!< udp/tcp listener
	U16 port;
	char protocol; //UDP, TCP <! 0x00 disabled, 0x01 udp, 0x02 tcp*/
private:
	//track logs (to close them all when forking)
	typedef std::vector<tLog *> tLogList;
	tLogList logs; //! save a pointer to each log (to able to shut them all down when forking)
	
	// prevent copying
	tLogConfig(const tLogConfig &);
	tLogConfig &operator=(const tLogConfig &);
};

class tLog {
public:
	tLog(const char * name=NULL,U16 flags=0);
	~tLog();
	void open(const char * name=NULL,U16 flags=0);
	void rotate(bool force=false);
	void close(bool silent=false);

	void print(const char * msg, ...) const;
	void stamp();
	void log(const char * msg, ...);
	void flush() const;

	void dumpbuf(const Byte * buf, U32 n, U32 e=0,Byte how=7) const;
	void dumpbuf(tBBuf &t, U32 n=0, U32 e=0,Byte how=7) const;

	/** New Line */
	void nl() const;
	/** logs a std error */
	void logerr(const char *msg);
	bool doesPrint(void) const;
	const char *getDir(void) const;

private:
	void printHtmlHead(const tString &generator);
	
	tLogConfig *tvLogConfig;
	char * name;
	char *fullpath;
	FILE * dsc;
	tBBuf * bdsc;
	U16 flags; //see above (DF_*)
	int facility; //this params are passed to syslog
	int priority; //this params are passed to syslog
	
	// prevent copying
	tLog(const tLog &);
	tLog &operator=(const tLog &);
};

}

#endif
