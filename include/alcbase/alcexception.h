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
	\param name Mr. core name
	\todo implement a Mutex here
*/
void alcWriteCoreDump(const char * name="");
/** 
	\brief Set coredump flags.
	\param flags 0x00 - disabled, 0x01 - enabled, 0x02 -always
*/
void alcSetCoreDumpFlags(char f);
/** 
	\brief Set if you want to abort always when an exception is raised. 
*/
void alcSetAbort(bool c);
/** 
	\brief Sets the coredumper path. 
*/
void alcSetCoreDumpPath(char * p);
/** 
	\brief Gets the coredumper path. 
*/
char * alcGetCoreDumpPath();

#define txExcLevels 20

/** \brief Exception, Base virtual class. */
class txBase {
public:
	/** \brief Constructor.
			\param msg A description message
			\param abort If true, this exception will always stop execution
			\param core If true, it will try to generate a coredump (requires the google Coredumper library), and of course, only works on a real operating system.
	*/
	txBase(const void * msg="",bool abort=false,bool core=false);
	txBase(const void * name,const void * msg,bool abort,bool core);
	txBase(const txBase &t);
	void copy(txBase &t);
	const txBase & operator=(txBase &t) { this->copy(t); return *this; }
	/** \brief Returns the description message */
	virtual const char * what();
	/** \brief Returns a backtrace (Only on Linux) */
	const char * backtrace();
	void dump(bool toStderr = true);
	virtual ~txBase();
private:
	void _preparebacktrace();
	const char * msg;
	void * btArray[txExcLevels];
	unsigned int size;
	char * bt;
	bool abort;
	bool core;
	char * imsg;
	const char * name;
};
//End Exception

/** Out Of Range */
class txOutOfRange : public txBase {
public:
	txOutOfRange(const void * msg="",bool abort=false,bool core=false) :txBase("OutOfRange",msg,abort,core) {}
	txOutOfRange(const txOutOfRange &t) :txBase((txOutOfRange &)t) {}
};
/** unknown flags */
class txUnkFlags : public txBase {
public:
	txUnkFlags(const void * msg="",bool abort=false,bool core=false) :txBase("UnkFlags",msg,abort,core) {}
	txUnkFlags(const txUnkFlags &t) :txBase((txUnkFlags &)t) {}
};
/** unknown flags */
class txUnexpectedData : public txBase {
public:
	txUnexpectedData(const void * msg="",bool abort=false,bool core=false) :txBase("UnexpectedData",msg,abort,core) {}
	txUnexpectedData(const txUnexpectedData &t) :txBase((txUnexpectedData &)t) {}
};
/** No Mem */
class txNoMem : public txBase {
public:
	txNoMem(const void * msg="",bool abort=false,bool core=false) :txBase("NoMem",msg,abort,core) {}
	txNoMem(const txNoMem &t) :txBase((txNoMem &)t) {}
};
/** Ref Err */
class txRefErr : public txBase {
public:
	txRefErr(const void * msg="",bool abort=false,bool core=false) :txBase("RefErr",msg,abort,core) {}
	txRefErr(const txRefErr &t) :txBase((txRefErr &)t) {}
};
/** Write Err */
class txWriteErr : public txBase {
public:
	txWriteErr(const void * msg="",bool abort=false,bool core=false) :txBase("WriteErr",msg,abort,core) {}
	txWriteErr(const txWriteErr &t) :txBase((txWriteErr &)t) {}
};
/** No File */
class txNoFile : public txBase {
public:
	txNoFile(const void * msg="",bool abort=false,bool core=false) :txBase("NoFile",msg,abort,core) {}
	txNoFile(const txNoFile &t) :txBase((txNoFile &)t) {}
};
/** Unk Err */
class txUnkErr : public txBase {
public:
	txUnkErr(const void * msg="",bool abort=false,bool core=false) :txBase("UnkErr",msg,abort,core) {}
	txUnkErr(const txUnkErr &t) :txBase((txUnkErr &)t) {}
};
/** No File */
class txNotFound : public txBase {
public:
	txNotFound(const void * msg="",bool abort=false,bool core=false) :txBase("NotFound",msg,abort,core) {}
	txNotFound(const txNotFound &t) :txBase((txNotFound &)t) {}
};
/** No File */
class txNoAccess : public txBase {
public:
	txNoAccess(const void * msg="",bool abort=false,bool core=false) :txBase("NoAccess",msg,abort,core) {}
	txNoAccess(const txNoAccess &t) :txBase((txNoAccess &)t) {}
};
/** Parse error */
class txParseError : public txBase {
public:
	txParseError(const void * msg="",bool abort=false,bool core=false) :txBase("ParseError",msg,abort,core) {}
	txParseError(const txParseError &t) :txBase((txParseError &)t) {}
};


} //End alc namespace


#endif
