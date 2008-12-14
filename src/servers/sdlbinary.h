/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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

#ifndef __U_SDLBINARY_H
#define __U_SDLBINARY_H
/* CVS tag - DON'T TOUCH*/
#define __U_SDLBINARY_H_ID "$Id$"

#include <list>
#include <memory>

namespace alc {

	////DEFINITIONS
	class tSdlStruct;
	typedef std::vector<tSdlStruct> tSdlStructList;
	
	class tSdlStateVar : public tBaseType {
	public:
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
	private:
		Byte flags;
	};
	
	class tSdlStateBinary : public tBaseType {
	public:
		tSdlStateBinary(const tSdlStruct *sdlStruct = NULL);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		void reset(const tSdlStruct *sdlStruct); //!< reset the state, empty all lists etc.
	private:
		typedef std::vector<tSdlStateVar> tVarList;
		
		const tSdlStruct *sdlStruct;
		tVarList vars;
	};

	class tSdlState : public tBaseType {
	public:
		tSdlState(const tUruObject &obj, const tSdlStructList *structs);
		tSdlState(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		const Byte *str(void);
		bool operator==(const tSdlState &state);
		
		tUruObject obj;
		// format
		tUStr name;
		U16 version;
		tSdlStateBinary content;
	private:
		static tMBuf decompress(tBBuf &t);
		static tMBuf compress(tMBuf &data);
		const tSdlStruct *findStruct(void);
	
		tStrBuf dbg;
		
		const tSdlStructList *structs;
	};

} //End alc namespace

#endif
