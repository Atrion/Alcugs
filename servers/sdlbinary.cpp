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

/* CVS tag - DON'T TOUCH*/
#define __U_SDLBINARY_ID "$Id$"
//#define _DBG_LEVEL_ 10
#include <alcdefs.h>
#include "sdlbinary.h"

#include "sdl.h"
#include <netexception.h>

#include <cstring>

namespace alc {

	/** SDL binary processing classes */
	
	//// tSdlStateVar (Plasma: plStateVariable and subclasses)
	tSdlStateVar::tSdlStateVar(tSdlStructVar *sdlVar, tAgeStateManager *stateMgr, uint8_t num)
	{
		this->num = num;
		this->sdlVar = sdlVar;
		this->stateMgr = stateMgr;
		flags = 0x18; // default non-struct var
	}
	
	tSdlStateVar::tSdlStateVar(alc::tSdlStructVar* sdlVar, const alc::tSdlStateVar& var, uint8_t num)
	{
		operator=(var); // first, copy the old one
		// now we need to change some properties as this is updated to a new version
		this->num = num;
		this->sdlVar = sdlVar;
	}
	
	tSdlStateVar::tSdlStateVar(const tSdlStateVar &var)
	{
		operator=(var); // this one does the real work
	}
	
	const tSdlStateVar &tSdlStateVar::operator=(const tSdlStateVar &var)
	{
		flags = var.flags;
		sdlVar = var.sdlVar;
		stateMgr = var.stateMgr;
		str = var.str;
		num = var.num;
		// because there can be pointers in the elements list, we have to copy manually
		if (sdlVar->type == DStruct || sdlVar->type == DPlKey) {
			// cleanup
			clear();
			// copy each element
			for (tElementList::const_iterator it = var.elements.begin(); it != var.elements.end(); ++it) {
				tElementList::iterator newIt = elements.insert(elements.end(), tElement());
				// the pointers to the new elements are immediatley stored in the list, so they are correctly cleaned up
				if (sdlVar->type == DStruct) newIt->sdlState = new tSdlStateBinary(*it->sdlState);
				else                         newIt->obj = new tUruObject(*it->obj);
			}
		}
		else {
			// copy whole list
			elements = var.elements;
		}
		return *this;
	}
	
	void tSdlStateVar::clear(void)
	{
		if (sdlVar->type == DStruct || sdlVar->type == DPlKey) {
			// cleanup
			for (tElementList::iterator it = elements.begin(); it != elements.end(); ++it) {
				if (sdlVar->type == DStruct) delete it->sdlState;
				else                         delete it->obj;
			}
		}
		elements.clear();
	}
	
	tSdlStateVar::~tSdlStateVar(void)
	{ clear(); }
	
	void tSdlStateVar::store(tBBuf &t)
	{
		uint8_t type = t.get8(); // type 2 = has notification info
		if (type != 0x02) throw txProtocolError(_WHERE("sdlBinaryVar.type must be 0x02 not 0x%02X", type));
		// read notification info
		uint8_t null = t.get8();
		if (null != 0x00) // this is just ignored by Plasma...
			throw txProtocolError(_WHERE("Unexpected sdlBinary.null of 0x%02X (expected 0x00)", null));
		t.get(str);
		// now comes the "real" variable, starting with the flags
		flags = t.get8();
		DBG(9, "Flags are 0x%02X\n", flags);
		//Supposicions meaning:
		// 0x04: Timestamp is present (a plUnifiedTime)
		// 0x08: Default value
		// 0x10: The value overwrites the current age status (if unset, it is ignored while merging) - seen only on vars, not on structs
		// 0x20: "want timestamp": Asks the server to set the current timestamp (which we currently don't do)
		uint8_t check = 0x04 | 0x08 | 0x10 | 0x20;
		if (flags & ~(check))
			throw txProtocolError(_WHERE("unknown flag 0x%02X for sdlBinaryVar", flags));
		if (flags & 0x10 && sdlVar->type == DStruct)
			throw txProtocolError(_WHERE("Structs must not have the 0x10 flag set"));
		// parse according to flags
		if (flags & 0x04)
			throw txProtocolError(_WHERE("Parsing the timestamp is not yet supported"));
		if (flags & 0x08) {
			if (sdlVar->type == DStruct)
				throw txProtocolError(_WHERE("Default vars for complete structs are not yet supported"));
			// don't read anything, it's still the default
		} else {
			// it's not a default var, we have to do something
			size_t n = sdlVar->size;
			if (!n) // a var with dynamic size
				n = t.get32();
			DBG(7, "Reading %Zd values for %s\n", n, sdlVar->name.c_str());
			if (sdlVar->type == DStruct) {
				// it seems for structs the number of structs to be parsed is saved again before the first struct
				uint8_t num = t.get8();
				if (num != n) throw txProtocolError(_WHERE("Unexpected number of structs to be parsed - expected %d, got %d", n, num));
			}
			for (size_t i = 0; i < n; ++i) {
				tElementList::iterator it = elements.insert(elements.end(), tElement());
				// parse the variable and save it in the union
				switch (sdlVar->type) {
					case DInteger:
						it->intVal[0] = t.get32();
						break;
					case DFloat:
						it->floatVal[0] = t.getFloat();
						break;
					case DBool:
					case DByte:
						it->byteVal[0] = t.get8();
						if (sdlVar->type == DBool && it->byteVal[0] != 0 && it->byteVal[0] != 1)
							throw txProtocolError(_WHERE("Unexpected BOOL var, must be 0 or 1 but is %d\n", it->byteVal));
						break;
					case DUruString:
						memcpy(it->byteVal, t.read(32), 32);
						break;
					case DPlKey:
						// the pointer to the new element are immediatley stored in the list, so they are correctly cleaned up
						it->obj = new tUruObject;
						t.get(*it->obj);
						break;
					case DStruct:
					{
						// the pointer to the new element are immediatley stored in the list, so they are correctly cleaned up
						it->sdlState = new tSdlStateBinary(stateMgr, sdlVar->structName, sdlVar->structVersion);
						t.get(*it->sdlState);
						break;
					}
					case DTime:
						it->intVal[0] = t.get32(); // seconds
						it->intVal[1] = t.get32(); // microseconds
						break;
					case DShort:
						it->shortVal = t.get16();
						break;
					case DVector3:
					case DPoint3:
						it->floatVal[0] = t.getFloat();
						it->floatVal[1] = t.getFloat();
						it->floatVal[2] = t.getFloat();
						break;
					case DQuaternion:
						it->floatVal[0] = t.getFloat();
						it->floatVal[1] = t.getFloat();
						it->floatVal[2] = t.getFloat();
						it->floatVal[3] = t.getFloat();
						break;
					case DRGB8:
						it->byteVal[0] = t.get8();
						it->byteVal[1] = t.get8();
						it->byteVal[2] = t.get8();
						break;
					default:
						// DTimestamp is only used in vault messages (generic creatable value), not in SDL files
						// DCreatable is used in cloneMessage.sdl, but I could not yet find an example message
						// DAgeTimeOfDay obviously is never sent, even when it is used in an age
						throw txProtocolError(_WHERE("Unable to parse SDL var of type %s (0x%02X)", alcUnetGetVarType(sdlVar->type), sdlVar->type));
				}
			}
		}
	}
	
	void tSdlStateVar::stream(tBBuf &t) const
	{
		t.put8(0x02); // type (2 = has notification info)
		// notification info
		t.put8(0x00); // ignored by Plasma
		t.put(str);
		// SDL state
		if (flags & 0x10 && sdlVar->type == DStruct)
			throw txProtocolError(_WHERE("Structs must not have the 0x10 flag set"));
		// write flags and var
		t.put8(flags);
		if (flags & 0x04)
			throw txProtocolError(_WHERE("Writing the timestamp is not yet supported"));
		if (flags & 0x08) { // default value
			if (sdlVar->type == DStruct)
				throw txProtocolError(_WHERE("Default vars for complete structs are not yet supported"));
			// don't write anything, it has the default value
		}
		else {
			// we have to write the value
			if (!sdlVar->size) t.put32(elements.size());
			else if (elements.size() != sdlVar->size)
				throw txProtocolError(_WHERE("Element count mismatch, must be %d, is %d", sdlVar->size, elements.size()));
			if (sdlVar->type == DStruct) // for some reason, this needs the size again
				t.put8(elements.size());
			for (tElementList::const_iterator it = elements.begin(); it != elements.end(); ++it) {
				switch (sdlVar->type) {
					case DInteger:
						t.put32(it->intVal[0]);
						break;
					case DFloat:
						t.putFloat(it->floatVal[0]);
						break;
					case DBool:
					case DByte:
						t.put8(it->byteVal[0]);
						break;
					case DUruString:
						t.write(it->byteVal, 32);
						break;
					case DPlKey:
						t.put(*it->obj);
						break;
					case DStruct:
						t.put(*it->sdlState);
						break;
					case DTime:
						t.put32(it->intVal[0]); // seconds
						t.put32(it->intVal[1]); // microseconds
						break;
					case DShort:
						t.put16(it->shortVal);
						break;
					case DVector3:
					case DPoint3:
						t.putFloat(it->floatVal[0]);
						t.putFloat(it->floatVal[1]);
						t.putFloat(it->floatVal[2]);
						break;
					case DQuaternion:
						t.putFloat(it->floatVal[0]);
						t.putFloat(it->floatVal[1]);
						t.putFloat(it->floatVal[2]);
						t.putFloat(it->floatVal[3]);
						break;
					case DRGB8:
						t.put8(it->byteVal[0]);
						t.put8(it->byteVal[1]);
						t.put8(it->byteVal[2]);
						break;
					default:
						throw txProtocolError(_WHERE("Unable to write SDL var of type %s (0x%02X)", alcUnetGetVarType(sdlVar->type), sdlVar->type));
				}
			}
		}
	}
	
	void tSdlStateVar::print(alc::tLog* log, unsigned int indentSize)
	{
		char indent[] = "                        ";
		if (indentSize < strlen(indent)) indent[indentSize] = 0; // let the string end there
		log->print("%s%s[%d] (", indent, sdlVar->name.c_str(), elements.size());
		if (str.size()) log->print("str: %s, ", str.c_str());
		log->print("flags: 0x%02X) = ", flags);
		if (flags & 0x08) {
			log->print("default value %s\n", sdlVar->defaultVal.c_str());
		}
		else {
			log->print("%s: ", alcUnetGetVarType(sdlVar->type));
			for (tElementList::iterator it = elements.begin(); it != elements.end(); ++it) {
				if (it != elements.begin() && sdlVar->type != DStruct) log->print(", ");
				// print value
				switch (sdlVar->type) {
					case DInteger:
						log->print("%d", it->intVal[0]);
						break;
					case DFloat:
						log->print("%f", it->floatVal[0]);
						break;
					case DBool:
					case DByte:
						log->print("%d", it->byteVal[0]);
						break;
					case DUruString:
					{
						tString str;
						str.write(it->byteVal, 32);
						log->print(str);
						break;
					}
					case DPlKey:
						log->print(it->obj->str());
						break;
					case DStruct:
						if (it == elements.begin()) log->nl();
						it->sdlState->print(log, indentSize+2);
						break;
					case DTime:
						log->print("%d.%d", it->intVal[0], it->intVal[1]);
						break;
					case DShort:
						log->print("%d", it->shortVal);
						break;
					case DVector3:
					case DPoint3:
						log->print("(%f,%f,%f)", it->floatVal[0], it->floatVal[1], it->floatVal[2]);
						break;
					case DQuaternion:
						log->print("(%f,%f,%f,%f)", it->floatVal[0], it->floatVal[1], it->floatVal[2], it->floatVal[3]);
						break;
					case DRGB8:
						log->print("(%d-%d-%d)", it->byteVal[0], it->byteVal[1], it->byteVal[2]);
						break;
					default:
						throw txProtocolError(_WHERE("Unable to print SDL var of type %s (0x%02X)", alcUnetGetVarType(sdlVar->type), sdlVar->type));
				}
			}
			if (sdlVar->type != DStruct || !elements.size()) log->nl();
		}
	}
	
	uint8_t tSdlStateVar::getType(void) const
	{ return sdlVar->type; }
	
	tString tSdlStateVar::getName(void) const
	{ return sdlVar->name; }
	
	size_t tSdlStateVar::getSize(void) const
	{ return sdlVar->size; }
	
	//// tSdlStateBinary (Plasma: plStateDataRecord)
	tSdlStateBinary::tSdlStateBinary(void) : volatileState(true)
	{
		stateMgr = NULL;
		sdlStruct = NULL;
		incompleteVars = incompleteStructs = false;
	}
	
	tSdlStateBinary::tSdlStateBinary(tAgeStateManager *stateMgr, tString name, uint16_t version, bool initDefault) : volatileState(true)
	{
		this->stateMgr = stateMgr;
		sdlStruct = stateMgr->findStruct(name, version);
		incompleteVars = incompleteStructs = false;
		
		if (initDefault) {
			// add defaults for everything (just sending an emtpy indexed state does not work)
			for (tSdlStruct::tVarList::iterator it = sdlStruct->vars.begin(); it != sdlStruct->vars.end(); ++it) {
				if (it->type == DStruct)
					throw txUnet(_WHERE("Can't create default for struct")); // this should never be necessary
				else
					vars.push_back(tSdlStateVar(&(*it), stateMgr, vars.size()));
			}
		}
	}
	
	void tSdlStateBinary::store(tBBuf &t)
	{
		if (sdlStruct == NULL)
			throw txUnet(_WHERE("You have to set a sdlStruct before parsing a sdlBinary"));
		DBG(5, "Parsing a %s version %d\n", sdlStruct->name.c_str(), sdlStruct->version);
		
		// parse the unknown header information
		uint16_t flags = t.get16();
		if (flags != 0x0000 && flags != 0x0001)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.flags of 0x%04X (expected 0x0000)", flags));
		volatileState = flags; // this is the only known flag value: 0x0001 = volatile
		uint8_t version = t.get8();
		if (version != 0x06)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.version of 0x%02X (expected 0x06)", version));
		
		// get number of values
		uint8_t nValues = t.get8();
		DBG(7, "nValues: %d, struct contains: %d\n", nValues, sdlStruct->nVar);
		if (nValues != sdlStruct->nVar) {
			incompleteVars = true;
			if (nValues > sdlStruct->nVar)
				throw txProtocolError(_WHERE("size mismatch, expected maximal %d values, got %d values", sdlStruct->nVar, nValues));
		}
		// parse those values
		for (uint8_t i = 0; i < nValues; ++i) {
			uint8_t nr = incompleteVars ? t.get8() : i;
			tVarList::iterator it = vars.insert(vars.end(), tSdlStateVar(sdlStruct->getElement(nr, true/*search for a var*/), stateMgr, nr));
			t.get(*it); // parse this var
		}
		
		// get number of structs
		uint8_t nStructs = t.get8();
		if (!nStructs) return;
		DBG(7, "nStructs: %d, struct contains: %d\n", nStructs, sdlStruct->nStruct);
		if (nStructs != sdlStruct->nStruct) {
			incompleteStructs = true;
			if (nStructs > sdlStruct->nStruct)
				throw txProtocolError(_WHERE("size mismatch, expected maximal %d structs, got %d structs", sdlStruct->nStruct, nStructs));
		}
		// parse those structs
		for (uint8_t i = 0; i < nStructs; ++i) {
			uint8_t nr = incompleteStructs ? t.get8() : i;
			tVarList::iterator it = structs.insert(structs.end(), tSdlStateVar(sdlStruct->getElement(nr, false/*search for a struct*/),
					stateMgr, nr));
			t.get(*it); // parse this struct
		}
	}
	
	void tSdlStateBinary::stream(tBBuf &t) const
	{
		if (sdlStruct == NULL)
			throw txUnet(_WHERE("You have to set a sdlStruct before streaming a sdlBinary"));
		// write unknown header information
		t.put16(volatileState);
		t.put8(0x06); // version
		
		// check number of values
		bool writeIndex = incompleteVars;
		if (vars.size() != sdlStruct->nVar) {
			if (!incompleteVars || vars.size() > sdlStruct->nVar)
				throw txProtocolError(_WHERE("Size mismatch: %d vars to be stored, %d vars in struct, incomplete: %d", vars.size(), sdlStruct->nVar, incompleteVars));
		}
		else
			writeIndex = false;
		// write values
		t.put8(vars.size());
		for (tVarList::const_iterator it = vars.begin(); it != vars.end(); ++it) {
			if (it->getType() == DStruct) throw txProtocolError(_WHERE("There's a struct in the vars!"));
			if (writeIndex) t.put8(it->getNum());
			t.put(*it);
		}
		
		// check number of structs
		writeIndex = incompleteStructs;
		if (structs.size() != sdlStruct->nStruct) {
			if (!incompleteStructs || structs.size() > sdlStruct->nStruct)
				throw txProtocolError(_WHERE("Size mismatch: %d structs to be stored, %d structs in struct, incomplete: %d", structs.size(), sdlStruct->nStruct, incompleteStructs));
		}
		else
			writeIndex = false;
		// write structs
		t.put8(structs.size());
		for (tVarList::const_iterator it = structs.begin(); it != structs.end(); ++it) {
			if (it->getType() != DStruct) throw txProtocolError(_WHERE("There's a var in the structs!"));
			if (writeIndex) t.put8(it->getNum());
			t.put(*it);
		}
	}
	
	void tSdlStateBinary::print(tLog *log, unsigned int indentSize)
	{
		char indent[] = "                        ";
		if (indentSize < strlen(indent)) indent[indentSize] = 0; // let the string end there
		log->print("%s%s version %d, volatile: ", indent, sdlStruct->name.c_str(), sdlStruct->version);
		if (volatileState) log->print("yes");
		else log->print("no");
		if (incompleteVars) log->print(", vars are indexed");
		if (incompleteStructs) log->print(", structs are indexed");
		log->nl();
		for (tVarList::iterator it = vars.begin(); it != vars.end(); ++it)
			it->print(log, indentSize+2);
		for (tVarList::iterator it = structs.begin(); it != structs.end(); ++it)
			it->print(log, indentSize+2);
	}
	
	tUruString tSdlStateBinary::getName(void) const {
		return sdlStruct ? tUruString(sdlStruct->name) : tUruString();
	}
	uint16_t tSdlStateBinary::getVersion(void) const {
		return sdlStruct ? sdlStruct->version : 0;
	}
	
	// what is below here is not really something caring about the binary representation of a SDL state, but it just belongs to this class
	void tSdlStateBinary::updateWith(tSdlStateBinary *newState)
	{
		if (sdlStruct != newState->sdlStruct || !sdlStruct)
			throw txUnet(_WHERE("Merging different SDL states not possible!"));
		
		DBG(8, "Updating %s.%d\n", sdlStruct->name.c_str(), sdlStruct->version);
		// first merge the vars
		DBG(8, "Merging %Zi current with %Zi new vars\n", vars.size(), newState->vars.size());
		mergeData(&vars, &newState->vars);
		
		// then the structs (if we have indexed sub-structs here, they are not recursively merged but overwritten as I see no way to decide which structs to merge if their number is dynamic and old and new number don't match)
		DBG(8, "Merging %Zi current with %Zi new structs\n", structs.size(), newState->structs.size());
		mergeData(&structs, &newState->structs);
	}
	
	void tSdlStateBinary::mergeData(tVarList *curData, tVarList *newData)
	{
		tVarList::iterator curIt = curData->begin();
		for (tVarList::iterator newIt = newData->begin(); newIt != newData->end(); ++newIt) { // merge each new value
			if (newIt->getType() != DStruct && !newIt->hasFlags(0x10)) continue; // vars without that flag set are skipped
			// find the first current element with a number equal to or bigger than the one of the new element
			while (curIt->getNum() < newIt->getNum() && curIt != curData->end()) ++curIt;
			if (curIt == curData->end() || curIt->getNum() > newIt->getNum()) {
				// if we are at the end of the old list, or at an element with a bigger number, we have to isnert the new one
				curIt = curData->insert(curIt, *newIt); // save returned iterator as the old one got invalid
				++curIt; // go to next element since we are not interested in the just inserted one
			}
			else {
				// we found the new element in the old list, overwrite it
				*curIt = *newIt;
			}
		}
	}
	
	void tSdlStateBinary::updateTo(tSdlStruct *newSdlStruct)
	{
		if (!sdlStruct || newSdlStruct->name != sdlStruct->name)
			throw txUnet(_WHERE("Can only update structs of same type"));
		if (newSdlStruct->version <= sdlStruct->version)
			throw txUnet(_WHERE("I will not downgrade anything"));
		// ok, do the same as when initializing a default struct, but fill it with the old data
		tVarList newVars;
		for (tSdlStruct::tVarList::iterator it = newSdlStruct->vars.begin(); it != newSdlStruct->vars.end(); ++it) {
			if (it->type == DStruct)
				throw txUnet(_WHERE("Can't update nested struct")); // this should never be necessary
			else {
				// check if we have that var in our old list
				bool found = false;
				for (tVarList::iterator jt = vars.begin(); jt != vars.end(); ++jt) {
					if (jt->getName() != it->name) continue; // this is another var
					if (jt->getSize() == it->size && jt->getType() == it->type) {
						// ok, we found an old var of the same type - let's copy it
						newVars.push_back(tSdlStateVar(&(*it), *jt, newVars.size()));
						found = true;
					}
					break;
				}
				if (!found) // add default state
					newVars.push_back(tSdlStateVar(&(*it), stateMgr, newVars.size()));
			}
		}
		// save the new stuff
		sdlStruct = newSdlStruct;
		incompleteVars = incompleteStructs = false;
		vars = newVars;
		structs.clear();
	}
	
	//// tSdlState
	tSdlState::tSdlState(alc::tAgeStateManager* stateMgr, const alc::tUruObject& obj, alc::tUruString name, uint16_t version, bool initDefault)
	 : obj(obj), content(stateMgr, name, version, initDefault)
	{
		this->stateMgr = stateMgr;
	}
	
	tSdlState::tSdlState(tAgeStateManager *stateMgr, tMBuf &t, const tUruObject &obj) : obj(obj)
	{
		this->stateMgr = stateMgr;
		store(t);
	}
	
	tSdlState::tSdlState(tAgeStateManager *stateMgr, tMBuf &t)
	{
		this->stateMgr = stateMgr;
		store(t);
	}
	
	tSdlState::tSdlState(void)
	{
		this->stateMgr = NULL;
	}
	
	void tSdlState::store(tBBuf &t)
	{
		if (stateMgr == NULL)
			throw txUnet(_WHERE("You have to set a stateMgr before parsing an sdlState"));
		// parse "header data"
		tUruString name;
		t.get(name);
		uint16_t version = t.get16();
		// parse binary content
		content = tSdlStateBinary(stateMgr, name, version);
		t.get(content);
		
		if (!t.eof()) {
			throw txProtocolError(_WHERE("The SDL struct is too long (%d Bytes remaining after parsing)", t.remaining()));
		}
	}
	
	void tSdlState::stream(tBBuf &t) const
	{
		if (content.getVersion() && content.getName().size()) {
			tUruString name(content.getName());
			t.put(name);
			t.put16(content.getVersion());
			t.put(content);
		}
	}
	
	bool tSdlState::operator==(const tSdlState &state) const
	{
		if (obj != state.obj) return false;
		if (content.getName() != state.content.getName()) return false;
		return true;
	}
	
	tString tSdlState::str(void) const
	{
		tString dbg;
		dbg.printf("SDL State for [%s]: %s (version %d)", obj.str().c_str(), content.getName().c_str(), content.getVersion());
		return dbg;
	}
	
	void tSdlState::print(tLog *log)
	{
		if (!log->doesPrint()) return;
		if (obj.objName.size())
			log->print("SDL State for [%s]:\n", obj.str().c_str());
		else
			log->print("SDL State:\n"); // other formats than 0x00 don't contain the object
		content.print(log);
	}

} //end namespace alc

