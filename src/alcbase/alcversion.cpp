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

/* CVS tag - DON'T TOUCH*/
#define __U_ALCVERSION_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "alcugs.h"

#ifndef __WIN32__
#include <sys/utsname.h>
#endif

#include "alcdebug.h"

namespace alc {

char tvalcVerTextShort[500];
char tvalcVerText[1024];
char tvalcSystemInfo[500];

static bool tvalcVerInit=false;

void _alcVersionInitVars() {
	if(tvalcVerInit) return;
	char p[200];
	char * p1=tvalcVerTextShort;
	char * p2=tvalcVerText;
	char * p3=tvalcSystemInfo;
	//short
	sprintf(p1,"%s\nId: %s\n",alcXSNAME,alcXID);
	sprintf(p,"Alcugs %s %s - Version %s\n",alcXBUILDINFO,_PLATFORM_,alcSTR_VER);
	strcat(p1,p);
	sprintf(p,"Unet 3+ Protocol %i.%i\n",alcProtoMAX_VER,alcProtoMIN_VER);
	strcat(p1,p);
	//long
	sprintf(p2,"%s",alcLicenseTextShort());
	sprintf(p,"%s","Alcugs Project - ");
	strcat(p2,p);
	strcat(p2,p1);
	#ifndef __WIN32__
	struct utsname buf;
	uname(&buf);
	char *domainname;
	#ifdef _GNU_SOURCE // this from the linux uname(2) man page
	domainname = buf.domainname;
	#else
	domainname = "";
	#endif
	sprintf(p3,"%s %s %s %s %s %s",buf.sysname,buf.nodename,buf.release,buf.version,buf.machine,domainname);
	#else
	sprintf(p3,"Unable to determine system info");
	#endif
	tvalcVerInit = true;
}

const char * alcSystemInfo() {
	_alcVersionInitVars();
	return tvalcSystemInfo;
}

const char * alcVersionTextShort() {
	_alcVersionInitVars();
	return tvalcVerTextShort;
}

const char * alcVersionText() {
	_alcVersionInitVars();
	return tvalcVerText;
}

int alcGetMaxVersion() {
	return alcMAX_VER;
}
int alcGetMinVersion() {
	return alcMIN_VER;
}
int alcGetRelVersion() {
	return alcREL_VER;
}
int alcGetBetVersion() {
	return alcBET_VER;
}
int alcGetProtocolMaxVersion() {
	return alcProtoMAX_VER;
}
int alcGetProtocolMinVersion() {
	return alcProtoMIN_VER;
}
int alcGetProtocolVersion() {
	return alcProtoVer;
}

const char * alcGetPlatform() {
	return _PLATFORM_;
}
const char * alcGetOS() {
	return _OS_;
}

} //end namespace

