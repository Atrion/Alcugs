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

#define txExcLevels 20

/** \brief Exception, Base virtual class. */
class txBase {
public:
	/** \brief Constructor.
			\param msg A description message
			\param abort If true, this exception will always stop execution
			\param core If true, it will try to generate a coredump (requires the google Coredumper library), and of course, only works on a real operating system.
	*/
	txBase(const tString &msg,bool abort=false);
	txBase(const tString &name,const tString &msg,bool abort=false);
	txBase(const txBase &t);
	void copy(const txBase &t);
	const txBase & operator=(txBase &t) { this->copy(t); return *this; }
	/** \brief Returns the description message */
	inline const char *what() const { return msg.c_str(); }
	/** \brief Returns a backtrace (Only on Linux) */
	inline const char *backtrace() const { return bt.c_str(); }
	void dump(bool toStderr = true);
	inline virtual ~txBase() {}
private:
	void _preparebacktrace();
	tString msg, bt;
	bool abort;
};
//End Exception

/** Out Of Range */
class txOutOfRange : public txBase {
public:
	txOutOfRange(const tString &msg,bool abort=false) :txBase("OutOfRange",msg,abort) {}
	txOutOfRange(const txOutOfRange &t) :txBase(t) {}
};
/** unknown flags */
class txUnkFlags : public txBase {
public:
	txUnkFlags(const tString &msg,bool abort=false) :txBase("UnkFlags",msg,abort) {}
	txUnkFlags(const txUnkFlags &t) :txBase(t) {}
};
/** unexpected data */
class txUnexpectedData : public txBase {
public:
	txUnexpectedData(const tString &msg,bool abort=false) :txBase("UnexpectedData",msg,abort) {}
	txUnexpectedData(const txUnexpectedData &t) :txBase(t) {}
};
/** No Mem */
class txNoMem : public txBase {
public:
	txNoMem(const tString &msg,bool abort=false) :txBase("NoMem",msg,abort) {}
	txNoMem(const txNoMem &t) :txBase(t) {}
};
/** Ref Err */
class txRefErr : public txBase {
public:
	txRefErr(const tString &msg,bool abort=false) :txBase("RefErr",msg,abort) {}
	txRefErr(const txRefErr &t) :txBase(t) {}
};
/** Write Err */
class txWriteErr : public txBase {
public:
	txWriteErr(const tString &msg,bool abort=false) :txBase("WriteErr",msg,abort) {}
	txWriteErr(const txWriteErr &t) :txBase(t) {}
};
/** Read Err */
class txReadErr : public txBase {
public:
	txReadErr(const tString &msg,bool abort=false) :txBase("ReadErr",msg,abort) {}
	txReadErr(const txWriteErr &t) :txBase(t) {}
};
/** No File */
class txNoFile : public txBase {
public:
	txNoFile(const tString &msg,bool abort=false) :txBase("NoFile",msg,abort) {}
	txNoFile(const txNoFile &t) :txBase(t) {}
};
/** Unk Err */
class txUnkErr : public txBase {
public:
	txUnkErr(const tString &msg,bool abort=false) :txBase("UnkErr",msg,abort) {}
	txUnkErr(const txUnkErr &t) :txBase(t) {}
};
/** No File */
class txNotFound : public txBase {
public:
	txNotFound(const tString &msg,bool abort=false) :txBase("NotFound",msg,abort) {}
	txNotFound(const txNotFound &t) :txBase(t) {}
};
/** No File */
class txNoAccess : public txBase {
public:
	txNoAccess(const tString &msg,bool abort=false) :txBase("NoAccess",msg,abort) {}
	txNoAccess(const txNoAccess &t) :txBase(t) {}
};
/** Parse error */
class txParseError : public txBase {
public:
	txParseError(const tString &msg,bool abort=false) :txBase("ParseError",msg,abort) {}
	txParseError(const txParseError &t) :txBase(t) {}
};


} //End alc namespace


#endif
