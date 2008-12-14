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
	class tSdlStructVar;
	class tSdlStateBinary;
	class tAgeStateManager;
	typedef std::vector<tSdlStruct> tSdlStructList;
	
	/** parses a signel SDL var */
	class tSdlStateVar : public tBaseType {
	public:
		tSdlStateVar(tSdlStructVar *sdlVar, tAgeStateManager *ageMgr);
		tSdlStateVar(const tSdlStateVar &var);
		const tSdlStateVar &operator=(const tSdlStateVar &var);
		~tSdlStateVar(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		void print(tLog *log, Byte indentSize);
	private:
		void clear(void);
	
		typedef union {
			Byte byteVal[3];
			U32 intVal[2];
			float floatVal;
			tSdlStateBinary *sdlState; // we have to use a pointer here - classes are not allowed in unions
			tUruObject *obj;
		} tElement;
		typedef std::vector<tElement> tElementList;
	
		Byte flags;
		tElementList elements;
		
		tSdlStructVar *sdlVar;
		tAgeStateManager *ageMgr;
	};
	
	/** parses a SDL state binary, which is a list of vars and can recursively contain other state binaries */
	class tSdlStateBinary : public tBaseType {
	public:
		tSdlStateBinary(void);
		tSdlStateBinary(tAgeStateManager *ageMgr, tStrBuf name, U32 version);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		void reset(tAgeStateManager *ageMgr, tStrBuf name, U32 version); //!< reset the state, empty all lists etc.
		void print(tLog *log, Byte indentSize = 1);
	private:
		typedef std::vector<tSdlStateVar> tVarList;
		
		Byte unk1;
		tVarList vars;
		tVarList structs;
		
		bool incompleteVars; //!< this state contains only a part of the full information and uses indices for the vars
		bool incompleteStructs; //!< this state contains only a part of the full information and uses indices for the structs
		
		tStrBuf dbg;
		tSdlStruct *sdlStruct;
		tAgeStateManager *ageMgr;
	};

	/** parses the SDL state */
	class tSdlState : public tBaseType {
	public:
		tSdlState(const tUruObject &obj, tAgeStateManager *stateMgr);
		tSdlState(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		const Byte *str(void);
		void print(tLog *log);
		bool operator==(const tSdlState &state);
		
		tUruObject obj;
		// format
		tUStr name;
		U16 version;
		tSdlStateBinary content;
	private:
		static tMBuf decompress(tBBuf &t); //!< gives us the decompressed content of the SDL stream, the bytes we really want to parse
		static tMBuf compress(tMBuf &data); //!< packs our SDL stream to be sent, adds length bytes and perhaps compresses
		tSdlStruct *findStruct(void); //!< finds the correct tSdlStruct which is necessary for this SDL State
	
		tStrBuf dbg;
		tAgeStateManager *stateMgr;
	};

} //End alc namespace

#endif
