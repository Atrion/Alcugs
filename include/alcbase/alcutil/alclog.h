/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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
#define DF_SYSLOG    0x0004
#define DF_STDOUT    0x0008
#define DF_STDERR    0x0010
#define DF_DEFSTDOUT 0x0020
#define DF_APPEND    0x0040
#define DF_DB        0x0080
#define DF_LOGD      0x0100
#define DF_UDP       0x0200
#define DF_TCP       0x0400
#define DF_STAMP     0x0800
#define DF_IP        0x1000
#define DF_ANOY      0x2000
#define DF_NODUMP    0x4000
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

class tLog {
public:
	tLog(const char * name=NULL,char level=2,U16 flags=0);
	~tLog();
	void open(const char * name=NULL, char level=2, U16 flags=0);
	int rotate(bool force=false);
	void close(bool silent=false);

	void print(const char * msg, ...);
	void stamp();
	void log(const char * msg, ...);
	void logl(char level,const char * msg,...);
	void flush();

	void dumpbuf(Byte * buf, U32 n, U32 e=0,Byte how=7);
	void dumpbuf(tBBuf &t, U32 n=0, U32 e=0,Byte how=7);

	/** New Line */
	void nl();
	/** logs an std error */
	void logerr(const char *msg);

private:
	char * name;
	FILE * dsc;
	tBBuf * bdsc;
	U16 flags; //see above (DF_*)
	Byte level;    /* 0 disabled  (logging level)
										1 lowest
										2 low
										3 normal
										4 med
										5 high
										6 max
									*/
	int facility; //this params are passed to syslog
	int priority; //this params are passed to syslog
};

//basic log subsystems
extern tLog * lerr;
extern tLog * lstd;
extern tLog * lnull;

void alcLogInit();
void alcLogShutdown(bool silent=false);

void alcLogSetDefaults();
void alcLogSetLogPath(tStrBuf & path);

void alcLogOpenStdLogs(bool shutup=false);

void alcLogSetLogLevel(Byte level);

char * alcHtmlGenerateHead(char * title,char * powered);

}

#endif
