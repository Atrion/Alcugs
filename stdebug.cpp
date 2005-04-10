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

#ifndef __U_DEBUG_
#define __U_DEBUG_
#define __U_DEBUG_ID $Id$

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "stdebug.h"

//log files
//Byte global_log_files_path[512] = "log/";

//global log files file descriptors
FILE * f_err;
const char * l_err = "error.log"; //stores stderr
U32 m_err=0x01;
FILE * f_uru;
const char * l_uru = "uru.log"; //stores the normal stdout activity
U32 m_uru=0x02;

FILE * f_une;
const char * l_une = "unexpected.log"; //stores unknown expected packets
U32 m_une=0x04;

FILE * f_vlt;
const char * l_vlt = "vault.log"; //stores all vault operations
U32 m_vlt=0x20;

FILE * f_vmgr;
const char * l_vmgr = "vmgr.log"; //stores all vault transmissions
U32 m_vmgr=0x40;

FILE * f_vhtml;
const char * l_vhtml = "vmgr.html"; //stores all vault transmission in html format
U32 m_vhtml=0x80;

FILE * f_vtask;
const char * l_vtask = "vtask.log"; //stores vault tasks
U32 m_vtask=0x100;

FILE * f_acc;
const char * l_acc = "access.log"; //stores all accesses, and logins from the remote clients
U32 m_acc=0x200;

FILE * f_ack;
const char * l_ack = "ack.html"; //stores the ack sequence
U32 m_ack=0x400;

FILE * f_bcast;
const char * l_bcast = "bcast.log"; //broadcasts
U32 m_bcast=0x800;

FILE * f_sdl;
const char * l_sdl = "sdl.log"; //SDL status
U32 m_sdl=0x1000;


int global_logs_enabled = 0;
/*
FILE * f_chk;
const char * l_chk = "checksum.log"; //stores checksums, with associated packet info
m_chk=0x08
FILE * f_chkal;
const char * l_chkal = "checksum_alg.log"; //stores results from the checksum algorithms
m_chkal=0x10
*/

//moved to config_parser
/*
int silent=0; //be silent yes/no (print or no uru_log messages to the stdout)
				// 0-> print all (I say, ALL!!)
				// 1-> print only uru.log messages and error.log messages
				// 2-> silence only the stdout
				// 3-> total silence, don't say nothing, be totally quiet
*/

void dump_logs_to_std() {
#ifdef _DEBUG_LOGS
	f_err=stderr;
	f_uru=stdout;
	f_une=stderr;

	f_acc=stdout;
	f_vlt=stdout;

	f_vmgr=stdout;
	f_vhtml=stdout;

	f_ack=stdout;

#if 0
	fprintf(f_ack,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	fprintf(f_vhtml,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	fprintf(f_ack,"<html><head>\n<title>Ack drawing</title>\n\
<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n\
<meta HTTP-EQUIV=\"Generator\" CONTENT=\"%s. Build %s - Version %s - Id: %s\">\n\
</head>\n<body>",SNAME,BUILD,VERSION,ID);
	fprintf(f_vhtml,"<html><head>\n<title>The VAULT</title>\n\
<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n\
<meta HTTP-EQUIV=\"Generator\" CONTENT=\"%s. Build %s - Version %s - Id: %s\">\n\
</head>\n<body>",SNAME,BUILD,VERSION,ID);
#endif
	f_vtask=stdout;
	f_bcast=stdout;
	f_sdl=stdout;

	//f_chk=open_log_file(l_chk);
	//f_chkal=open_log_file(l_chkal);
#endif
}

/*--------------------------------------------
   Controls open/close the log files
---------------------------------------------*/
void open_log_files() {
#ifdef _DEBUG_LOGS

	char croak[200];
	int i,e,size;
	size=strlen((char *)global_log_files_path);
	e=0;
	for(i=0; i<size; i++) {
		croak[e]=global_log_files_path[i];
		e++;
		if(croak[e-1]=='/') {
			croak[e]='\0';
			mkdir(croak,00750);
			//e=0;
		}
	}

	//mkdir((char *)global_log_files_path,00750);

	f_err=open_log_file(l_err);
	f_uru=open_log_file(l_uru);
	f_une=open_log_file(l_une);

	f_acc=open_log_file(l_acc);
	f_vlt=open_log_file(l_vlt);

	f_vmgr=open_log_file(l_vmgr);
	f_vhtml=open_log_file(l_vhtml);

	f_ack=open_log_file(l_ack);

	fprintf(f_ack,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	fprintf(f_vhtml,"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n");
	fprintf(f_ack,"<html><head>\n<title>Ack drawing</title>\n\
<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n\
<meta HTTP-EQUIV=\"Generator\" CONTENT=\"%s. Build %s - Version %s - Id: %s\">\n\
</head>\n<body>",SNAME,BUILD,VERSION,ID);
	fprintf(f_vhtml,"<html><head>\n<title>The VAULT</title>\n\
<meta HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=iso-8859-1\">\n\
<meta HTTP-EQUIV=\"Generator\" CONTENT=\"%s. Build %s - Version %s - Id: %s\">\n\
</head>\n<body>",SNAME,BUILD,VERSION,ID);

	f_vtask=open_log_file(l_vtask);
	f_bcast=open_log_file(l_bcast);
	f_sdl=open_log_file(l_sdl);

	//f_chk=open_log_file(l_chk);
	//f_chkal=open_log_file(l_chkal);
#endif
}

void close_log_files() {
#ifdef _DEBUG_LOGS
	fclose(f_err);
	fclose(f_uru);
	fclose(f_une);

	fclose(f_acc);
	fclose(f_vlt);

	fclose(f_vmgr);

	fprintf(f_vhtml,"\n</body>\n</html>\n");
	fclose(f_vhtml);

	fprintf(f_ack,"\n</body>\n</html>\n");
	fclose(f_ack);

	fclose(f_vtask);
	fclose(f_bcast);
	fclose(f_sdl);

	//fclose(f_chk);
	//fclose(f_chkal);
#endif
}


/*-------------------------------------
 Opens a log file to write,
 returns the file descriptor
 associated to the log file
--------------------------------------*/
FILE * open_log_file(const char * name) {
	FILE * f;
	char * path;

	path = (char *)malloc((strlen(name) + strlen((const char *)global_log_files_path)+1)*sizeof(char));
	strcpy(path,(const char *)global_log_files_path);
	strcat(path,name);
	f=fopen(path,"wb");
	if(f==NULL) {
		fprintf(stderr,"ERR: Fatal - Cannot Open %s to write ",path);
	perror("returned error");
	f=stdout; //default standard output
	printf("DBG: Sending debug messages to the stdout instead of %s\n",path);
	}
	free(path);
	return f;
}

/*---------------------------
  Closes the log file
----------------------------*/
void close_log_file(FILE * file) {
	if(file!=stdout && file!=stderr) {
		fclose(file);
	}
}

/*-----------------------------------------------
  Prints a timestamp into the log file
-----------------------------------------------*/
void stamp2log(FILE * file) {
#ifdef _DEBUG_LOGS
	char c_time_aux[25];
	struct tm * tptr;
	time_t timestamp;

	struct timeval tv;

	time(&timestamp);
	gettimeofday(&tv,NULL);

	//micros=(((U32)clock()*1000/CLOCKS_PER_SEC)%1000);
	//micros=clock();

	if(global_log_timestamp==0) { return; }

	tptr=gmtime((const time_t *)&timestamp);

	strftime(c_time_aux,25,"(%Y:%m:%d:%H:%M:%S",tptr);
	//free(tptr);

	print2log(file,"%s.%06d) ",c_time_aux,tv.tv_usec);
#endif
}

/*-----------------------------------------------
  Prints the ip into the log file
-----------------------------------------------*/
void ip2log_old(FILE * file, U32 ip, U16 port) {
#ifdef _DEBUG_LOGS
	in_addr cip;
	char mip[16];
	cip.s_addr=(unsigned long)ip;
	strcpy(mip, inet_ntoa(cip));
	print2log(file,"[%s:%i] ",mip,ntohs(port));
#endif
}

void ip2log(FILE * file, st_uru_client * s) {
#ifdef _DEBUG_LOGS
	in_addr cip;
	char mip[16];
	cip.s_addr=(unsigned long)s->client_ip;
	strcpy(mip, inet_ntoa(cip));
	print2log(file,"[%s:%i] ",mip,ntohs(s->client_port));
#endif
}

char * get_ip(U32 ip) {
	in_addr cip;
	static char mip[16];
	cip.s_addr=(unsigned long)ip;
	strcpy(mip, inet_ntoa(cip));
	//print2log(f_uru,"DBGDBGDBG:<<<----->>>>%s:%08X\n",mip,ip);
	return mip;
}


/*----------------------------------------------
  Gets the microseconds value
-----------------------------------------------*/
U32 get_microseconds() {
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return tv.tv_usec;
}


/*----------------------------------------------
  Adds and an entry to the specified log file
----------------------------------------------*/
void plog(FILE * file, char * msg, ...) {
	va_list ap,ap2;
	va_start(ap,msg);
	va_start(ap2,msg);

	stamp2log(file);

	if(global_logs_enabled==1) {
		if(file==NULL) {
			if(silent<=2) {
	#ifdef _DEBUG_LOGS
				fprintf(stderr,"ERR: There was a problem trying to write to the log files!\n");
	#endif
			}
		} else {
				vfprintf(file,msg,ap); //first print via normal way
		}
	}

	if((file!=stdout && file!=stderr && file!=f_vhtml && file!=f_ack) || file==NULL) {
		if(file==f_err && silent<=2) {
			vfprintf(stderr,msg,ap2);     //stderr messages
		} else if(file==f_uru && silent<=1) {
				vfprintf(stdout,msg,ap2);      //stdout messages
		} else if(silent==0) {
				vfprintf(stdout,msg,ap2);      //print all (muhahahaha)
		}
	}
	va_end(ap2);
	va_end(ap);
}

/*----------------------------------------------
  Adds and an entry to the specified log file
----------------------------------------------*/
void print2log(FILE * file, char * msg, ...) {
	va_list ap,ap2;
	va_start(ap,msg);
	va_start(ap2,msg);

	if(global_logs_enabled==1) {
		if(file==NULL) {
			if(silent<=2) {
	#ifdef _DEBUG_LOGS
				fprintf(stderr,"ERR: There was a problem trying to write to the log files!\n");
	#endif
			}
		} else {
			vfprintf(file,msg,ap); //first print via normal way
		}
	}

	if((file!=stdout && file!=stderr && file!=f_vhtml && file!=f_ack) || file==NULL) {
		if(file==f_err && silent<=2) {
			vfprintf(stderr,msg,ap2);     //stderr messages
		} else if(file==f_uru && silent<=1) {
				vfprintf(stdout,msg,ap2);      //stdout messages
		} else if(silent==0) {
				vfprintf(stdout,msg,ap2);      //print all (muhahahaha)
		}
	}
	va_end(ap2);
	va_end(ap);
}

/*-----------------------------------------
  fflush to all logging outputs..
-----------------------------------------*/
void logflush(FILE * file) {
#ifdef _DEBUG_LOGS
	fflush(file);
	if(file==f_err && silent<=2) {  fflush(stderr); }
	if(file==f_uru && silent<=1 || silent==0) {  fflush(stdout); }
#endif
}

/*-------------------------------
control fatal error messages
-----------------------------*/
void error(char *msg) {
	perror(msg);
	exit(1); //this was a fatal error, kill the server
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
void dump_packet(FILE * dsc,unsigned char* buf, unsigned int n, unsigned int e, int how) {
#ifdef _DEBUG_DUMP_PACKETS
	unsigned int i,j,k;

	if(n>1027) {
		print2log(dsc,"MESSAGE TOO BIG TO BE DUMPED!! cutting it\n");
		n=1027;
	}

	switch(how) {
		case 0:
			for(i=e;i<n;i++) {
				if(i%4==0) { print2log(dsc," "); }
				print2log(dsc,"%02X",buf[i]);
			}
			break;
		case 1:
		case 3:
			for(i=e;i<n;i++) {
				if(i%4==0) { print2log(dsc," "); }
				if((how==1 && isprint((int)buf[i])) || (how!=1 && isprint((Byte)(~buf[i])))) {
					if(how==1) { print2log(dsc,"%c",buf[i]); }
					else { print2log(dsc,"%c",(Byte)(~buf[i])); }
				}
				else print2log(dsc,".");
			}
			break;
		case 2:
		case 4:
			for(i=e;i<n;i++) {
				if(i%4==0) { print2log(dsc," "); }
				if((how==2 && isprint((int)buf[i])) || (how!=2 && isprint((Byte)(~buf[i])))) {
					if(how==2) { print2log(dsc," %c",buf[i]); }
					else { print2log(dsc," %c",(Byte)(~buf[i])); }
				}
				else print2log(dsc," .");
			}
			break;
		case 5:
		case 6:
		case 7:
			print2log(dsc,"Offs");
			for(i=0; i<0x10; i++) {
				//0 1 2 3 ... A B ... F
				print2log(dsc," %2X",i);
			}
			for(i=e;i<n;i++) {
				if(!((i-e)%0x10)) { //end of line
					if((i-e)!=0) {  //not after first line
						print2log(dsc," ");
						for(j=i-0x10; j<i; j++) {
							if((how!=6 && isprint((int)buf[j])) || (how==6 && isprint((Byte)(~buf[j])))) {
								if(how!=6) { print2log(dsc,"%c",buf[j]); }
								else { print2log(dsc,"%c",(Byte)(~buf[j])); }
							}
							else print2log(dsc,".");
						}
						if(how==7) {
							print2log(dsc," ");
							for(j=i-0x10; j<i; j++) {
								if(isprint((Byte)(~buf[j]))) {
									print2log(dsc,"%c",(Byte)(~buf[j]));
								}
								else print2log(dsc,".");
							}
						}
					}
					print2log(dsc,"\n%04X",i-e); //offset
				}
				print2log(dsc," %02X",buf[i]);
			}
			if((n-j)<0x10) {
				for(i=(i-e)%0x10;i<0x10;i++) { print2log(dsc,"   ");	}
			}
			print2log(dsc," ");
			for(k=j; k<n; k++) {
				if((how!=6 && isprint((int)buf[k])) || (how==6 && isprint((Byte)(~buf[k])))) {
						if(how!=6) { print2log(dsc,"%c",buf[k]); }
						else { print2log(dsc,"%c",(Byte)(~buf[k])); }
				}
				else print2log(dsc,".");
			}
			if(how==7) {
				if((n-j)<0x10) {
					for(k=k; k<=(((n/0x10)+1)*0x10); k++) {
						print2log(dsc," ",k,n,((n/0x10)));
					}
				} else {
					print2log(dsc," ");
				}
				for(k=j; k<n; k++) {
					if((isprint(((Byte)(~buf[k]))))) {
						print2log(dsc,"%c",(Byte)(~buf[k]));
					}
					else print2log(dsc,".");
				}
			}
	}
	logflush(dsc);
#endif
}

#endif
