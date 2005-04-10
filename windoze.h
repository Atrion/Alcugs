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

/**
	Windoze porting
*/

#ifndef __U_WINDOZE_H_
#define __U_WINDOZE_H_
#define __U_WINDOZE_H_ID $Id: stdebug.h,v 1.2 2004/12/02 23:09:36 almlys Exp $

#ifdef __WIN32__

#include <windows.h>
#include <sys/time.h>
#include <unistd.h>

#define mkdir(a,b) mkdir(a)

#define random rand
#define srandom srand

//signals
#define SIGHUP  NSIG+1
#define SIGALRM NSIG+2
#define SIGUSR1 NSIG+3
#define SIGUSR2 NSIG+4
#define SIGCHLD NSIG+5

//syslog (taken from syslog.h)
#define LOG_EMERG       0       /* system is unusable */
#define LOG_ALERT       1       /* action must be taken immediately */
#define LOG_CRIT        2       /* critical conditions */
#define LOG_ERR         3       /* error conditions */
#define LOG_WARNING     4       /* warning conditions */
#define LOG_NOTICE      5       /* normal but significant condition */
#define LOG_INFO        6       /* informational */
#define LOG_DEBUG       7       /* debug-level messages */

/* facility codes */
#define LOG_KERN        (0<<3)  /* kernel messages */
#define LOG_USER        (1<<3)  /* random user-level messages */
#define LOG_MAIL        (2<<3)  /* mail system */
#define LOG_DAEMON      (3<<3)  /* system daemons */
#define LOG_AUTH        (4<<3)  /* security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* line printer subsystem */
#define LOG_NEWS        (7<<3)  /* network news subsystem */
#define LOG_UUCP        (8<<3)  /* UUCP subsystem */
#define LOG_CRON        (9<<3)  /* clock daemon */
#define LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define LOG_FTP         (11<<3) /* ftp daemon */

//missing gettimeofday function
int gettimeofday(struct timeval *tv, struct timezone *tz);

//another very important missing function
void usleep(unsigned long usec);
int getuid(void);
int isblank(int c);
int daemon(int a,int b);
unsigned int alarm(unsigned int sec);
char *strsep(char **pcadena, const char *delim);

#endif

#endif
