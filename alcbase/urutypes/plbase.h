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

#ifndef __U_PLBASE_H
#define __U_PLBASE_H

#include "alctypes.h"

namespace alc {

	class tpObject;
	class tStreamedObject;
	
	const char *alcGetPlasmaType(uint16_t type);
	
	class tpObject : public tStreamable {
	public:
		virtual void store(tBBuf &/*t*/) {}
		virtual void stream(tBBuf &/*t*/) const {}

		static tpObject *createFromStream(tStreamedObject *stream, bool UUFormat, bool mustBeComplete = true);
		
		void setUUFormat(bool UUFormat) { this->UUFormat = UUFormat; }
		uint16_t getType(void) const { return type; }
		bool isIncomplete(void) const { return incomplete; }
		virtual tString str(void) const;
	protected:
		tpObject(uint16_t type, bool incomplete = false) : incomplete(incomplete), type(type) {}
		
		static tpObject *createByType(uint16_t type, bool mustBeComplete = true);
		bool UUFormat;
	private:
		bool incomplete;
		uint16_t type;
	};

} //End alc namespace

#endif
