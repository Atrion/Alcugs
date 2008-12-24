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

/**
	URUNET 3+
*/

#ifndef __U_NETLOG_H
#define __U_NETLOG_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETLOG_H_ID "$Id$"

namespace alc {

char * alcGetStrIp(U32 ip);
//const char * alcGetStrUnetErr(S16 code);

class txUnet : public txBase {
public:
	txUnet(const char * name="unet",const char * msg="Unknown Unet Error",bool abort=false,bool core=false) :txBase(name,msg,abort,core) {}
};

class txUnetIniErr : public txUnet {
public:
	txUnetIniErr(const char * msg="",bool abort=false,bool core=false) :txUnet("UnetIniError",msg,abort,core) {}
};

class txToMCons : public txUnet {
public:
	txToMCons(const char * msg="",bool abort=false,bool core=false) :txUnet("txToMCons",msg,abort,core) {}
};

class txTooBig : public txUnet {
public:
	txTooBig(const char * msg="",bool abort=false,bool core=false) :txUnet("txTooBig",msg,abort,core) {}
};

class txProtocolError : public txUnet {
public:
	txProtocolError(const char * msg="",bool abort=false,bool core=false) :txUnet("txProtocolError",msg,abort,core) {}
};

class txDatabaseError : public txUnet {
public:
	txDatabaseError(const char * msg="",bool abort=false,bool core=false) :txUnet("txDatabaseError",msg,abort,core) {}
};


}

#endif
