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

#include <alcexception.h>

namespace alc {

class txUnet : public txBase {
public:
	txUnet(const tString &name,const tString &msg,bool abort=false) :txBase(name,msg,abort) {}
	txUnet(const tString &msg,bool abort=false) :txBase("unet",msg,abort) {}
};

class txUnetIniErr : public txUnet {
public:
	txUnetIniErr(const tString &msg,bool abort=false) :txUnet("UnetIniError",msg,abort) {}
};

class txToMCons : public txUnet {
public:
	txToMCons(const tString &msg,bool abort=false) :txUnet("txToMCons",msg,abort) {}
};

class txTooBig : public txUnet {
public:
	txTooBig(const tString &msg,bool abort=false) :txUnet("txTooBig",msg,abort) {}
};

class txProtocolError : public txUnet {
public:
	txProtocolError(const tString &msg,bool abort=false) :txUnet("txProtocolError",msg,abort) {}
};

class txDatabaseError : public txUnet {
public:
	txDatabaseError(const tString &msg,bool abort=false) :txUnet("txDatabaseError",msg,abort) {}
};


}

#endif
