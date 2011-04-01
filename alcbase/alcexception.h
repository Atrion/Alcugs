/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

#include "alctypes.h"

namespace alc {


/** \brief Exception, Base virtual class. */
class txBase {
public:
	/** \brief Constructor.
			\param msg A description message
			\param abort If true, this exception will always stop execution
			\param core If true, it will try to generate a coredump (requires the google Coredumper library), and of course, only works on a real operating system.
	*/
	txBase(const tString &msg) : msg(msg) { storeBacktrace(); }
	txBase(const tString &name, const tString &msg) : msg(name + ": " + msg) { storeBacktrace(); }
	virtual ~txBase() {}
	
	/** Returns the description message */
	const char *what() const { return msg.c_str(); }
	/** Returns a backtrace (Only on Linux) */
	const char *backtrace() const {
		if (btPrinted.isEmpty())
			btPrinted = printBacktrace(bt, size);
		return btPrinted.c_str();
	}
	/** Dumps exception information to a BackTrace file */
	void dump() const;
	
	static tString printBacktrace(void*const* bt, unsigned int size); //!< return a readable version of bt
private:
	enum { txExcLevels = 20 };
	
	tString msg; //!< exception message
	mutable tString btPrinted; //!< the printed backtrace (as a cache, will only be calculated once when needed)
	void *bt[txExcLevels];
	unsigned int size;
	
	void storeBacktrace();
};
//End Exception

/** Out Of Range */
class txOutOfRange : public txBase {
public:
	txOutOfRange(const tString &msg) :txBase("OutOfRange",msg) {}
	txOutOfRange(const txOutOfRange &t) :txBase(t) {}
};
/** unknown flags */
class txUnkFlags : public txBase {
public:
	txUnkFlags(const tString &msg) :txBase("UnkFlags",msg) {}
	txUnkFlags(const txUnkFlags &t) :txBase(t) {}
};
/** unexpected data */
class txUnexpectedData : public txBase {
public:
	txUnexpectedData(const tString &msg) :txBase("UnexpectedData",msg) {}
	txUnexpectedData(const txUnexpectedData &t) :txBase(t) {}
};
/** No Mem */
class txNoMem : public txBase {
public:
	txNoMem(const tString &msg) :txBase("NoMem",msg) {}
	txNoMem(const txNoMem &t) :txBase(t) {}
};
/** Ref Err */
class txRefErr : public txBase {
public:
	txRefErr(const tString &msg) :txBase("RefErr",msg) {}
	txRefErr(const txRefErr &t) :txBase(t) {}
};
/** Write Err */
class txWriteErr : public txBase {
public:
	txWriteErr(const tString &msg) :txBase("WriteErr",msg) {}
	txWriteErr(const txWriteErr &t) :txBase(t) {}
};
/** Read Err */
class txReadErr : public txBase {
public:
	txReadErr(const tString &msg) :txBase("ReadErr",msg) {}
	txReadErr(const txWriteErr &t) :txBase(t) {}
};
/** No File */
class txNoFile : public txBase {
public:
	txNoFile(const tString &msg) :txBase("NoFile",msg) {}
	txNoFile(const txNoFile &t) :txBase(t) {}
};
/** Unk Err */
class txUnkErr : public txBase {
public:
	txUnkErr(const tString &msg) :txBase("UnkErr",msg) {}
	txUnkErr(const txUnkErr &t) :txBase(t) {}
};
/** No File */
class txNotFound : public txBase {
public:
	txNotFound(const tString &msg) :txBase("NotFound",msg) {}
	txNotFound(const txNotFound &t) :txBase(t) {}
};
/** No File */
class txNoAccess : public txBase {
public:
	txNoAccess(const tString &msg) :txBase("NoAccess",msg) {}
	txNoAccess(const txNoAccess &t) :txBase(t) {}
};
/** Parse error */
class txParseError : public txBase {
public:
	txParseError(const tString &msg) :txBase("ParseError",msg) {}
	txParseError(const txParseError &t) :txBase(t) {}
};


} //End alc namespace


#endif
