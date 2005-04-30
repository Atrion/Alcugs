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

/* UNET 3 debbuging system */

#ifndef __U_DEBUG_H_
#define __U_DEBUG_H_
#define __U_DEBUG_H_ID $Id$

#include <stdio.h>
#include <sys/types.h>

#ifdef __MSVC__
#  define mode_t unsigned short
#endif


#include "data_types.h"

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

typedef struct {
	char * name;
	FILE * dsc;
	U16 flags; /* 0x01 file is open, (internal flag)
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
	char level;    /* 0 dissabled  (logging level)
										1 lowest
										2 low
										3 normal
										4 med
										5 high
										6 max
									*/
	int facility; //this params are passed to syslog
	int priority; //this params are passed to syslog
} st_log;

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
	st_log ** logs; //Pointer to each one
} st_log_config;

extern st_log_config * stdebug_config;

//basic log subsystems
extern st_log * f_err;
extern st_log * f_uru;

//-->
void log_init();
void log_shutdown();

//For forked childs
void log_shutdown_silent();

st_log * open_log(const char * name, char level, U16 flags);
int rotate_log(st_log * file,char force);
void close_log(st_log * log);
void close_log_silent(st_log * log); //for forked stuff (internal, don't use, use log_shutdown_silent instead)

void print2log(st_log * log, char * msg, ...);
void stamp2log(st_log * log);

void plog(st_log * log, char * msg, ...);
void logl(st_log * log, char level,char * msg, ...);

void logflush(st_log * log);

void dumpbuf(st_log * dsc,unsigned char* buf, unsigned int n);
void dump_packet(st_log * dsc,unsigned char* buf, unsigned int n, unsigned int e, int how);

void lognl(st_log * log);

void logerr(st_log * log,char *msg);

void log_openstdlogs();


#if 0

//Deleted or moved to other files
U32 get_microseconds();

void ip2log_old(FILE * file, U32 ip, U16 port);
void ip2log(FILE * file, st_uru_client * s);


char * get_string_time(int stamp,int micros);
#endif

#endif
