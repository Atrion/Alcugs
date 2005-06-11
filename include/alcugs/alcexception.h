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

/** Dumps core to coredumps directory. TODO: implement a Mutex here */
void alcWriteCoreDump();
//0x00 - dissabled, 0x01 - enabled, 0x02 -always
void alcSetCoreDumpFlags(char f);
void alcSetAbort(bool c);

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
};
//End Exception

} //End alc namespace


#endif
