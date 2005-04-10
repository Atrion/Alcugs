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

/* Old debug system ---
This is the debug system that is used in version 1.0.3 of the server.
Version 1.0.5 is a more object oriented server */

#ifndef __U_DEBUG_H_
#define __U_DEBUG_H_
#define __U_DEBUG_H_ID $Id$

#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

//print debug info?
#define _DEBUG_LOGS
#define _DEBUG_DUMP_PACKETS

extern int global_logs_enabled; //print logs?

extern const char * BUILD;
extern const char * SNAME;
extern const char * ID;
extern const char * VERSION;

#include "data_types.h"
//perhaps we need the config_parser, for some variables
#include "config_parser.h" //for parsing configuration files (all globals are here)
#include "tmp_config.h"

//global log files file descriptors
extern FILE * f_err;
extern const char * l_err; //stores stderr
extern FILE * f_uru;
extern const char * l_uru;

extern FILE * f_une;
extern const char * l_une;

extern FILE * f_vlt;
extern const char * l_vlt;

extern FILE * f_vmgr;
extern const char * l_vmgr;

extern FILE * f_vhtml;
extern const char * l_vhtml;

extern FILE * f_vtask;
extern const char * l_vtask;

extern FILE * f_acc;
extern const char * l_acc;

extern FILE * f_ack;
extern const char * l_ack;

extern FILE * f_bcast;
extern const char * l_bcast;

extern FILE * f_sdl;
extern const char * l_sdl;


void dump_logs_to_std();

FILE * open_log_file(const char * name);
void close_log_file(FILE * file);
void print2log(FILE * file, char * msg, ...);

/*--------------------------------------------
   Controls open/close the log files
---------------------------------------------*/
void open_log_files();
void close_log_files();

/*-------------------------------------
 Opens a log file to write,
 returns the file descriptor
 associated to the log file
--------------------------------------*/
FILE * open_log_file(const char * name);

/*---------------------------
  Closes the log file
----------------------------*/
void close_log_file(FILE * file);

/*-----------------------------------------------
  Prints a timestamp into the log file
-----------------------------------------------*/
void stamp2log(FILE * file);

/*-----------------------------------------------
  Prints the ip into the log file
-----------------------------------------------*/
void ip2log_old(FILE * file, U32 ip, U16 port);
void ip2log(FILE * file, st_uru_client * s);
char * get_ip(U32 ip);
/*----------------------------------------------
  Gets the microseconds value
-----------------------------------------------*/
U32 get_microseconds();

/*----------------------------------------------
  Adds and an entry to the specified log file
----------------------------------------------*/
void plog(FILE * file, char * msg, ...);

/*----------------------------------------------
  Adds and an entry to the specified log file
----------------------------------------------*/
void print2log(FILE * file, char * msg, ...);

/*-----------------------------------------
  fflush to all logging outputs..
-----------------------------------------*/
void logflush(FILE * file);

/*-------------------------------
control fatal error messages
-----------------------------*/
void error(char *msg);

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
void dump_packet(FILE * dsc,unsigned char* buf, unsigned int n, unsigned int e, int how);

#endif
