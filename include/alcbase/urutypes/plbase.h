/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs Server Team                           *
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
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

#ifndef __U_PLBASE_H
#define __U_PLBASE_H
/* CVS tag - DON'T TOUCH*/
#define __U_PLBASE_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	class tpObject;
	
	const char *alcGetPlasmaType(U16 type);
	tpObject *alcCreatePlasmaObject(U16 type, bool mustBeComplete = true);
	
	class tpObject : public tBaseType {
	public:
		tpObject(U16 type, bool incomplete = false) : tBaseType(), incomplete(incomplete), type(type) {}
		virtual void store(tBBuf &/*t*/) {}
		virtual void stream(tBBuf &/*t*/) const {}
		
		inline U16 getType(void) const { return type; }
		inline bool isIncomplete(void) const { return incomplete; }
		virtual tString str(void) const;
	private:
		bool incomplete;
		U16 type;
	};

} //End alc namespace

#endif
