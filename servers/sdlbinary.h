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

#ifndef __U_SDLBINARY_H
#define __U_SDLBINARY_H
/* CVS tag - DON'T TOUCH*/
#define __U_SDLBINARY_H_ID "$Id$"

#include <urutypes/urubasetypes.h>

#include <list>
#include <memory>
#include <vector>

namespace alc {
	class tSdlStruct;
	class tSdlStructVar;
	class tSdlStateBinary;
	class tAgeStateManager;
	class tLog;
	class tElementList;
	
	/** parses a signel SDL var */
	class tSdlStateVar : public tBaseType {
	public:
		tSdlStateVar(tSdlStructVar *sdlVar, tAgeStateManager *stateMgr, uint8_t num);
		tSdlStateVar(tSdlStructVar *sdlVar, const tSdlStateVar &var, uint8_t num);
		tSdlStateVar(const tSdlStateVar &var);
		const tSdlStateVar &operator=(const tSdlStateVar &var);
		~tSdlStateVar(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		void print(alc::tLog* log, unsigned int indentSize);
		uint32_t getNum(void) const { return num; }
		bool hasFlags(uint8_t f) const {
			return (flags | f) == flags; // there can be several flags enabled in f, so a simple & is not enough
		}
		uint8_t getType(void) const;
		tString getName(void) const;
		size_t getSize(void) const;
	private:
		void clear(void);
	
		typedef union {
			uint8_t byteVal[32]; // also used to store the STRING32
			uint16_t shortVal;
			uint32_t intVal[2];
			float floatVal[4];
			tSdlStateBinary *sdlState; // we have to use a pointer here - classes are not allowed in unions
			tUruObject *obj;
		} tElement;
		typedef std::vector<tElement> tElementList;
	
		uint32_t num; //!< this is the nth var/struct of the SDL, starting with 0
		
		tUruString str; //!< details about how the state was changed
		uint8_t flags;
		tElementList elements;
		
		tSdlStructVar *sdlVar;
		tAgeStateManager *stateMgr;
	};
	
	/** parses a SDL state binary, which is a list of vars and can recursively contain other state binaries */
	class tSdlStateBinary : public tBaseType {
	public:
		tSdlStateBinary(void);
		tSdlStateBinary(tAgeStateManager *stateMgr, tString name, uint16_t version, bool initDefault = false);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		void print(tLog *log, unsigned int indentSize = 1);
		
		tUruString getName(void) const;
		uint16_t getVersion(void) const;
		bool isIndexed(void) const { return incompleteVars || incompleteStructs; }
		
		void updateWith(tSdlStateBinary *newState); //!< updates the current state with the additional information from the new one
		void updateTo(tSdlStruct *newSdlStruct); //!< updates the struct the data is saved in
		
		typedef std::list<tSdlStateVar> tVarList;
	private:
		void mergeData(tVarList *curData, tVarList *newData);
		
		bool volatileState;
		tVarList vars;
		tVarList structs;
		
		bool incompleteVars; //!< this state contains only a part of the full information and uses indices for the vars
		bool incompleteStructs; //!< this state contains only a part of the full information and uses indices for the structs
		
		tSdlStruct *sdlStruct;
		tAgeStateManager *stateMgr;
	};

	/** parses the SDL state */
	class tSdlState : public tBaseType {
	public:
		tSdlState(tAgeStateManager *stateMgr, const tUruObject &obj, tUruString name, uint16_t version, bool initDefault = false);
		tSdlState(tAgeStateManager *stateMgr, tMBuf &t, const tUruObject &obj);
		tSdlState(tAgeStateManager *stateMgr, tMBuf &t);
		tSdlState(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t) const;
		tString str(void) const;
		void print(tLog *log);
		bool operator==(const tSdlState &state) const;
		
		tUruObject obj;
		// format
		tSdlStateBinary content;
	private:
		tAgeStateManager *stateMgr;
	};

} //End alc namespace

#endif
