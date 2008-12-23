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

/* CVS tag - DON'T TOUCH*/
#define __U_SDLBINARY_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/gamemsg.h>

////extra includes
#include "sdlbinary.h"
#include "sdl.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	/** SDL binary processing classes */
	
	//// tSdlStateVar
	tSdlStateVar::tSdlStateVar(tSdlStructVar *sdlVar, tAgeStateManager *stateMgr, Byte num)
	{
		this->num = num;
		this->sdlVar = sdlVar;
		this->stateMgr = stateMgr;
		str.setVersion(5); // inverted UruString
		flags = 0x18; // default non-struct var
	}
	
	tSdlStateVar::tSdlStateVar(const tSdlStateVar &var)
	{
		*this = var; // call operator= which does the real work
		str.setVersion(5); // inverted UruString
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
		Byte type = t.getByte();
		if (type != 0x02) throw txProtocolError(_WHERE("sdlBinaryVar.type must be 0x02 not 0x%02X", type));
		Byte unk = t.getByte();
		if (unk != 0x00)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.unk of 0x%02X (expected 0x00)", unk));
		t.get(str);
		// now come the flags
		flags = t.getByte();
		DBG(9, "Flags are 0x%02X\n", flags);
		//Supposicions meaning:
		// 0x04: Timestamp is present
		// 0x08: It has the default value
		// 0x10: non-struct var? (true for all the messages from the client, but agestate files from old client sometimes don't have it set on non-struct vars)
		Byte check = 0x04 | 0x08 | 0x10;
		if (flags & ~(check))
			throw txProtocolError(_WHERE("unknown flag 0x%02X for sdlBinaryVar", flags));
		if (flags & 0x10 && sdlVar->type == DStruct)
			throw txProtocolError(_WHERE("struct var must not have 0x10 flag set"));
		// parse according to flags
		if (flags & 0x04)
			throw txProtocolError(_WHERE("Parsing the timestamp is not yet supported"));
		if (flags & 0x08) {
			if (sdlVar->type == DStruct)
				throw txProtocolError(_WHERE("Default vars for complete structs are not yet supported"));
			// don't read anything, it's still the default
		} else {
			// it's not a default var, we have to do something
			U32 n = sdlVar->size;
			if (!n) // a var with dynamic size
				n = t.getU32();
			DBG(7, "Reading %d values for %s\n", n, sdlVar->name.c_str());
			if (sdlVar->type == DStruct) {
				// it seems for structs the number of structs to be parsed is saved again before the first struct
				Byte num = t.getByte();
				if (num != n) throw txProtocolError(_WHERE("Unexpected number of structs to be parsed - expected %d, got %d", n, num));
			}
			for (U32 i = 0; i < n; ++i) {
				tElementList::iterator it = elements.insert(elements.end(), tElement());
				// parse the variable and save it in the union
				switch (sdlVar->type) {
					case DInteger:
						it->intVal[0] = t.getU32();
						break;
					case DFloat:
						it->floatVal[0] = t.getFloat();
						break;
					case DBool:
					case DByte:
						it->byteVal[0] = t.getByte();
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
						it->intVal[0] = t.getU32(); // seconds
						it->intVal[1] = t.getU32(); // microseconds
						break;
					case DShort:
						it->shortVal = t.getU16();
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
						it->byteVal[0] = t.getByte();
						it->byteVal[1] = t.getByte();
						it->byteVal[2] = t.getByte();
						break;
					default:
						// FIXME: Yet to implement; DAgeTimeOfDay
						// DTimestamp is only used in vault messages (generic creatable value), not in SDL files
						// DCreatable is used in cloneMessage.sdl, but I could not yet find an example message
						throw txProtocolError(_WHERE("Unable to parse SDL var of type %s (0x%02X)", alcUnetGetVarType(sdlVar->type), sdlVar->type));
				}
			}
		}
	}
	
	void tSdlStateVar::stream(tBBuf &t)
	{
		t.putByte(0x02); // type
		t.putByte(0x00); // unk
		t.put(str);
		// check non-struct var flag (0x10)
		if (flags & 0x10 && sdlVar->type == DStruct)
			throw txProtocolError(_WHERE("struct var must not have 0x10 flag set"));
		// write flags and var
		t.putByte(flags);
		if (flags & 0x04)
			throw txProtocolError(_WHERE("Writing the timestamp is not yet supported"));
		if (flags & 0x08) { // default value
			if (sdlVar->type == DStruct)
				throw txProtocolError(_WHERE("Default vars for complete structs are not yet supported"));
			// don't write anything, it has the default value
		}
		else {
			// we have to write the value
			if (!sdlVar->size) t.putU32(elements.size());
			else if (elements.size() != sdlVar->size)
				throw txProtocolError(_WHERE("Element count mismatch, must be %d, is %d", sdlVar->size, elements.size()));
			if (sdlVar->type == DStruct) // for some reason, this needs the size again
				t.putByte(elements.size());
			for (tElementList::iterator it = elements.begin(); it != elements.end(); ++it) {
				switch (sdlVar->type) {
					case DInteger:
						t.putU32(it->intVal[0]);
						break;
					case DFloat:
						t.putFloat(it->floatVal[0]);
						break;
					case DBool:
					case DByte:
						t.putByte(it->byteVal[0]);
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
						t.putU32(it->intVal[0]); // seconds
						t.putU32(it->intVal[1]); // microseconds
						break;
					case DShort:
						t.putU16(it->shortVal);
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
						t.putByte(it->byteVal[0]);
						t.putByte(it->byteVal[1]);
						t.putByte(it->byteVal[2]);
						break;
					default:
						throw txProtocolError(_WHERE("Unable to write SDL var of type %s (0x%02X)", alcUnetGetVarType(sdlVar->type), sdlVar->type));
				}
			}
		}
	}
	
	void tSdlStateVar::print(tLog *log, Byte indentSize)
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
						log->print("%s", it->byteVal);
						break;
					case DPlKey:
						log->print("%s", it->obj->str());
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
	
	Byte tSdlStateVar::getType(void)
	{ return sdlVar->type; }
	
	//// tSdlStateBinary
	tSdlStateBinary::tSdlStateBinary(void)
	{
		unk1 = 0x01;
		stateMgr = NULL;
		sdlStruct = NULL;
		incompleteVars = incompleteStructs = false;
	}
	
	tSdlStateBinary::tSdlStateBinary(tAgeStateManager *stateMgr, tStrBuf name, U32 version, bool initDefault)
	{
		unk1 = 0x01;
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
		unk1 = t.getByte();
		if (unk1 != 0x00 && unk1 != 0x01)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.unk1 of 0x%02X (expected 0x00 or 0x01)", unk1));
		Byte unk2 = t.getByte();
		if (unk2 != 0x00)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.unk2 of 0x%02X (expected 0x00)", unk2));
		Byte unk3 = t.getByte();
		if (unk3 != 0x06)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.unk3 of 0x%02X (expected 0x06)", unk3));
		
		// get number of values
		Byte nValues = t.getByte();
		DBG(7, "nValues: %d, struct contains: %d\n", nValues, sdlStruct->nVar);
		if (nValues != sdlStruct->nVar) {
			incompleteVars = true;
			if (nValues > sdlStruct->nVar)
				throw txProtocolError(_WHERE("size mismatch, expected maximal %d values, got %d values", sdlStruct->nVar, nValues));
		}
		// parse those values
		for (int i = 0; i < nValues; ++i) {
			int nr = incompleteVars ? t.getByte() : i;
			tVarList::iterator it = vars.insert(vars.end(), tSdlStateVar(sdlStruct->getElement(nr, true/*search for a var*/), stateMgr, nr));
			t.get(*it); // parse this var
		}
		
		// get number of structs
		Byte nStructs = t.getByte();
		if (!nStructs) return;
		DBG(7, "nStructs: %d, struct contains: %d\n", nStructs, sdlStruct->nStruct);
		if (nStructs != sdlStruct->nStruct) {
			incompleteStructs = true;
			if (nStructs > sdlStruct->nStruct)
				throw txProtocolError(_WHERE("size mismatch, expected maximal %d structs, got %d structs", sdlStruct->nStruct, nStructs));
		}
		// parse those structs
		for (int i = 0; i < nStructs; ++i) {
			int nr = incompleteStructs ? t.getByte() : i;
			tVarList::iterator it = structs.insert(structs.end(), tSdlStateVar(sdlStruct->getElement(nr, false/*search for a struct*/),
					stateMgr, nr));
			t.get(*it); // parse this var
		}
	}
	
	void tSdlStateBinary::stream(tBBuf &t)
	{
		// write unknown header information
		t.putByte(unk1);
		t.putByte(0x00); // unk2
		t.putByte(0x06); // unk3
		
		// check number of values
		if (vars.size() != sdlStruct->nVar) {
			if (!incompleteVars || vars.size() > sdlStruct->nVar)
				throw txProtocolError(_WHERE("Size mismatch: %d vars to be stored, %d vars in struct, incomplete: %d", vars.size(), sdlStruct->nVar, incompleteVars));
		}
		else
			incompleteVars = false;
		// write values
		t.putByte(vars.size());
		for (tVarList::iterator it = vars.begin(); it != vars.end(); ++it) {
			if (it->getType() == DStruct) throw txProtocolError(_WHERE("There's a struct in the vars!"));
			if (incompleteVars) t.putByte(it->getNum());
			t.put(*it);
		}
		
		// check number of structs
		if (structs.size() != sdlStruct->nStruct) {
			if (!incompleteStructs || structs.size() > sdlStruct->nStruct)
				throw txProtocolError(_WHERE("Size mismatch: %d structs to be stored, %d structs in struct, incomplete: %d", structs.size(), sdlStruct->nStruct, incompleteStructs));
		}
		else
			incompleteStructs = false;
		// write structs
		t.putByte(structs.size());
		for (tVarList::iterator it = structs.begin(); it != structs.end(); ++it) {
			if (it->getType() != DStruct) throw txProtocolError(_WHERE("There's a var in the structs!"));
			if (incompleteStructs) t.putByte(it->getNum());
			t.put(*it);
		}
	}
	
	void tSdlStateBinary::print(tLog *log, Byte indentSize)
	{
		char indent[] = "                        ";
		if (indentSize < strlen(indent)) indent[indentSize] = 0; // let the string end there
		log->print("%s%s version %d (unk1: 0x%02X)", indent, sdlStruct->name.c_str(), sdlStruct->version, unk1);
		if (incompleteVars) log->print(", vars are indexed");
		if (incompleteStructs) log->print(", structs are indexed");
		log->nl();
		for (tVarList::iterator it = vars.begin(); it != vars.end(); ++it)
			it->print(log, indentSize+2);
		for (tVarList::iterator it = structs.begin(); it != structs.end(); ++it)
			it->print(log, indentSize+2);
	}
	
	tUStr tSdlStateBinary::getName(void) const {
		return sdlStruct ? tUStr(sdlStruct->name, 5) : tUStr(5); // 5 = inverted UruString
	}
	U16 tSdlStateBinary::getVersion(void) const {
		return sdlStruct ? sdlStruct->version : 0;
	}
	
	// this is not really something caring about the binary representation of a SDL state, but it just belongs to this class
	void tSdlStateBinary::updateWith(tSdlStateBinary *newState)
	{
		if (sdlStruct != newState->sdlStruct || !sdlStruct)
			throw txUnet(_WHERE("Merging different SDL states not possible!"));
		
		DBG(8, "Updating %s.%d\n", sdlStruct->name.c_str(), sdlStruct->version);
		// first the vars
		if (newState->incompleteVars) { // merge new data in
			DBG(8, "Merging %d current with %d new vars\n", vars.size(), newState->vars.size());
			mergeData(&vars, &newState->vars);
		} else { // just copy
			DBG(8, "Just copying %d vars\n", newState->vars.size());
			vars = newState->vars;
			incompleteVars = false;
		}
		
		// then the structs
		if (newState->incompleteStructs) { // merge new data in
			DBG(8, "Merging %d current with %d new structs\n", structs.size(), newState->structs.size());
			mergeData(&structs, &newState->structs); // FIXME: this will not correctly merge indexed sub-structs - but I have no clue how I would have to do that
		} else { // just copy
			DBG(8, "Just copying %d structs\n", newState->structs.size());
			structs = newState->structs;
			incompleteStructs = false;
		}
	}
	
	void tSdlStateBinary::mergeData(tVarList *curData, tVarList *newData)
	{
		tVarList::iterator curIt = curData->begin();
		for (tVarList::iterator newIt = newData->begin(); newIt != newData->end(); ++newIt) { // merge each new value
			// find the first current element with a number equal to or bigger than the one of the new element
			while (curIt->getNum() < newIt->getNum() && curIt != curData->end()) ++curIt;
			if (curIt == curData->end() || curIt->getNum() > newIt->getNum()) {
				// if we are at the end of the old list, or at an element with a bigger number, we have to isnert the new one
				curIt = curData->insert(curIt, *newIt)++; // save returned iterator as the old one got invalid, and go to next element since we are not interested in the just inserted one
			}
			else {
				// we found the new element in the old list, overwrite it
				*curIt = *newIt;
			}
		}
	}
	
	//// tSdlState
	tSdlState::tSdlState(tAgeStateManager *stateMgr, bool canBeLonger)
	{
		this->stateMgr = stateMgr;
		this->canBeLonger = canBeLonger;
		skipObj = false;
	}
	
	tSdlState::tSdlState(tAgeStateManager *stateMgr, const tUruObject &obj, tUStr name, U16 version, bool initDefault)
	 : obj(obj), content(stateMgr, name, version, initDefault)
	{
		this->stateMgr = stateMgr;
		canBeLonger = false;
		skipObj = false;
	}
	
	tSdlState::tSdlState(void)
	{
		this->stateMgr = NULL;
		skipObj = false;
	}
	
	tMBuf tSdlState::decompress(tBBuf &t, bool canBeLonger)
	{
		tMBuf data;
	
		U32 realSize = t.getU32();
		Byte compressed = t.getByte();
		U32 sentSize = t.getU32();
		
		if (sentSize > 0) {
			Byte objPresent = t.getByte();
			if (objPresent != 0x00) throw txProtocolError(_WHERE("objPresent must be 0x00 but is 0x%02X", objPresent));
			Byte sdlMagicNumber = t.getByte();
			if (sdlMagicNumber != 0x80) throw txProtocolError(_WHERE("sdlMagicNumber must be 0x80 but is 0x%02X", sdlMagicNumber));
			// remove these two Bytes from above counters
			realSize -= 2;
			sentSize -= 2;
			
			tBBuf *buf = &t;
			if (compressed == 0x02) {
				tZBuf *content = new tZBuf;
				content->write(t.read(sentSize), sentSize);
				content->uncompress(realSize);
				buf = content;
			}
			else if (compressed == 0x00) {
				realSize = sentSize;
			}
			else
				throw txProtocolError(_WHERE("unknown compression format 0x%02X", compressed));
			
			// get binary body
			data.write(buf->read(realSize), realSize);
			
			if (compressed == 0x02) { // it was compressed, so we have to clean up
				if (!buf->eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after what we parsed above
				delete buf;
			}
		}
		else {
			DBG(5, "Got empty SDL message\n");
		}
		
		if (!canBeLonger && !t.eof())
			throw txProtocolError(_WHERE("The SDL struct is too long (%d Bytes remaining after parsing)", t.remaining()));
		
		return data;
	}
	
	tMBuf tSdlState::compress(tMBuf &data)
	{
		tMBuf t;
		if (!data.size()) {
			// write an empty SDL
			t.putU32(0); // real size
			t.putByte(0x00); // compression flag
			t.putU32(0); // sent size
			return t;
		}
		
		// it's not yet empty, so we have to write something
		if (data.size() > 255) { // just a guess - we compress if it's bigger than 255 bytes
			t.putU32(data.size()+2); // uncompressed size (take object flag and SDL magic into account)
			t.putByte(0x02); // compression flag
			// compress it
			tZBuf content;
			content.put(data);
			content.compress();
			// put sent size and compressed data in buffer
			t.putU32(content.size()+2);
			t.putByte(0x00); // object present
			t.putByte(0x80); // SDL magic number
			t.put(content);
		}
		else {
			t.putU32(0); // the real size is 0 for uncompressed messages
			t.putByte(0x00); // compression flag
			t.putU32(data.size()+2); // sent size
			t.putByte(0x00); // object present
			t.putByte(0x80); // SDL magic number
			t.put(data);
		}
		return t;
	}
	
	void tSdlState::store(tBBuf &t)
	{
		if (stateMgr == NULL)
			throw txUnet(_WHERE("You have to set a stateMgr before parsing a sdlState"));
		// use tSdlBinay to get the message body and decompress stuff
		if (!skipObj) // the NetMsgJoinAck doesn't contain the UruObject
			t.get(obj);
		tMBuf data = decompress(t, canBeLonger);
		// parse "header data"
		data.rewind();
		tUStr name;
		data.get(name);
		U16 version = data.getU16();
		// parse binary content
		content = tSdlStateBinary(stateMgr, name, version);
		data.get(content);
		
		if (!data.eof()) {
			tMBuf buf;
			U32 size = data.remaining();
			buf.write(data.read(size), size);
			lstd->dumpbuf(buf);
			lstd->nl();
			throw txProtocolError(_WHERE("The SDL struct is too long (%d Bytes remaining after parsing)", size));
		}
	}
	
	void tSdlState::stream(tBBuf &t)
	{
		if (!skipObj) // the NetMsgJoinAck doesn't contain the UruObject
			t.put(obj);
		tMBuf data;
		if (content.getVersion() && content.getName().size()) {
			tUStr name(content.getName());
			data.put(name);
			data.putU16(content.getVersion());
			data.put(content);
		}
		// use tSdlBinary to get the SDL header around it
		tMBuf b = compress(data);
		t.put(b);
	}
	
	bool tSdlState::operator==(const tSdlState &state)
	{
		if (obj != state.obj) return false;
		if (content.getName() != state.content.getName()) return false;
		return true;
	}
	
	const Byte *tSdlState::str(void)
	{
		dbg.clear();
		dbg.printf("SDL State for [%s]: %s (version %d)", obj.str(), content.getName().c_str(), content.getVersion());
		return dbg.c_str();
	}
	
	void tSdlState::print(tLog *log)
	{
		if (!log->doesPrint()) return;
		log->print("SDL State for [%s]:\n", obj.str());
		content.print(log);
	}

} //end namespace alc

