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
	Alcugs exception Handler.
*/

#ifndef __U_ALCEXCEPTION_H
#define __U_ALCEXCEPTION_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCEXCEPTION_H_ID "$Id$"

namespace alc {

/** 
	\brief Dumps core to coredumps directory.
	\todo implement a Mutex here
	\param name Mr. core name */
void alcWriteCoreDump(char * name="");
/** 
	\brief Set coredump flags
	\param flags 0x00 - dissabled, 0x01 - enabled, 0x02 -always
*/
void alcSetCoreDumpFlags(char f);
/** Set if you want to abort always when an exception is raised */
void alcSetAbort(bool c);
/** Sets the coredumper path */
void alcSetCoreDumpPath(char * p);
/** Get the coredumper path */
char * alcGetCoreDumpPath();

#define txExcLevels 20

/** Exception, Base virtual class */
class txBase {
public:
	/** Constructor
			\param msg A description message
			\param abort If true, this exception will always stop execution
			\param core If true, it will try to generate a coredump (requires the google Coredumper library), and of course, only works on a real operating system.
	*/
	txBase(char * msg="",bool abort=false,bool core=false);
	txBase(char * name,char * msg,bool abort,bool core);
	/**	Returns the description message */
	virtual char * what();
	/** Returns a backtrace (Only on Linux) */
	char * backtrace();
	virtual ~txBase();
private:
	void _preparebacktrace();
	char * msg;
	void * btArray[txExcLevels];
	unsigned int size;
	char * bt;
	bool abort;
	bool core;
	char * imsg;
	char * name;
};
//End Exception

/** Out Of Range */
class txOutOfRange : public txBase {
public:
	txOutOfRange(char * msg="",bool abort=false,bool core=false) :txBase("OutOfRange",msg,abort,core) {}
};
/** unknown flags */
class txUnkFlags : public txBase {
public:
	txUnkFlags(char * msg="",bool abort=false,bool core=false) :txBase("UnkFlags",msg,abort,core) {}
};
/** unknown flags */
class txUnexpectedData : public txBase {
public:
	txUnexpectedData(char * msg="",bool abort=false,bool core=false) :txBase("UnexpectedData",msg,abort,core) {}
};
/** No Mem */
class txNoMem : public txBase {
public:
	txNoMem(char * msg="",bool abort=false,bool core=false) :txBase("NoMem",msg,abort,core) {}
};
/** Ref Err */
class txRefErr : public txBase {
public:
	txRefErr(char * msg="",bool abort=false,bool core=false) :txBase("RefErr",msg,abort,core) {}
};
/** Write Err */
class txWriteErr : public txBase {
public:
	txWriteErr(char * msg="",bool abort=false,bool core=false) :txBase("WriteErr",msg,abort,core) {}
};
/** No File */
class txNoFile : public txBase {
public:
	txNoFile(char * msg="",bool abort=false,bool core=false) :txBase("NoFile",msg,abort,core) {}
};
/** Unk Err */
class txUnkErr : public txBase {
public:
	txUnkErr(char * msg="",bool abort=false,bool core=false) :txBase("UnkErr",msg,abort,core) {}
};
/** No File */
class txNotFound : public txBase {
public:
	txNotFound(char * msg="",bool abort=false,bool core=false) :txBase("NotFound",msg,abort,core) {}
};
/** No File */
class txNoAccess : public txBase {
public:
	txNoAccess(char * msg="",bool abort=false,bool core=false) :txBase("NoAccess",msg,abort,core) {}
};
/** Parse error */
class txParseError : public txBase {
public:
	txParseError(char * msg="",bool abort=false,bool core=false) :txBase("ParseError",msg,abort,core) {}
};


} //End alc namespace


#endif
