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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <syslog.h>

#include "log.h"


#ifdef LOG_TEST

/* Compile this included test proggy with:
 *	g++ -DLOG_TEST -o log log.cpp
 */

char *syslogname;
int global_logs_enabled;
int silent;

main(int argc, char **argv)
{
  syslogname = "testlog";
  global_logs_enabled = 1;
  silent = 3;

  Log(L_CO, NULL, "this will print on stderr\n");
  Log(L_CO, stdout, "this will print on stdout\n");
  Log(L_COTS, stdout, "this will print on stdout with timestamp\n");
  Log(L_FCOTS, stdout, "this will print on stdout with timestamp with flush\n");
  Log(L_SY, NULL, "this will print to syslog\n");
  Log(L_ALL, stdout, "this will print to syslog and to stdout\n");
  global_logs_enabled = 0;
  Log(L_SY, NULL, "this will also print to syslog\n");
  Log(L_CO, stdout, "this will not print\n");
  Log(L_ALL, stdout, "this will print to syslog but not to stdout\n");
  silent = 0; 
  Log(L_CO, stdout, "this will print because silent=0\n");
}
#else
extern char *syslogname;
extern int global_logs_enabled;
extern int silent;
#endif /* LOG_TEST */

/*
 * Log()
 *	long loglevel		L_CO	log into file
 *				  L_TS	log timestamp if L_CO
 *				  L_FL	flush logs if L_CO
 *				L_SY	log into syslog
 *				L_VB	log both in file and syslog
 *	FILE *fout			File descriptor, NULL will print to stderrA
 *	char * format			Message to be displayed
 *
 *	This functions output formatted msg onto syslog() if asked explicitly.
 *	It will print msg onto given file (or stderr if NULL) if global_logs_enabled is true and if asked for.
 *	It will nevertheless print msg if silent global parameter is 0.
 */
void
Log (long loglevel, FILE *fout, char *format, ...)
{
	va_list args;
	char c_time_aux[25];
	time_t timestamp;
	struct tm *tptr;
	struct timeval tv;
	static char msg[1024];

	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);

	if (!syslogname) syslogname = "HURU";

	if (loglevel & L_SY) {
		openlog(syslogname, LOG_NDELAY, LOG_USER);
		syslog(LOG_DEBUG, "%s", msg);
		closelog();
	}
	// Log to file or if !silent print all (muhahahaha)
	if ((global_logs_enabled && (loglevel & L_CO)) || (silent == 0)) {
		if (loglevel & L_TS) {
			time(&timestamp);
	        	gettimeofday(&tv,NULL);
			tptr=gmtime((const time_t *)&timestamp);
	        	strftime(c_time_aux,25,"(%Y:%m:%d:%H:%M:%S",tptr);
			fprintf((fout ? fout : stderr), "%s.%06d [%s] %s\n", c_time_aux, tv.tv_usec, syslogname, msg);
		} else
			fprintf((fout ? fout : stderr), "[%s] %s\n", syslogname, msg);
		if (loglevel & L_FL)
			fflush(fout);
	}
}

