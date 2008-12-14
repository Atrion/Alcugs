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

#define _DBG_LEVEL_ 10

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
	tSdlStateVar::tSdlStateVar(tSdlStructVar *sdlVar, tAgeStateManager *ageMgr)
	{
		this->sdlVar = sdlVar;
		this->ageMgr = ageMgr;
	}
	
	tSdlStateVar::tSdlStateVar(const tSdlStateVar &var)
	{
		*this = var;
	}
	
	const tSdlStateVar &tSdlStateVar::operator=(const tSdlStateVar &var)
	{
		// because there can be pointers in the elements list, we have to copy manually
		flags = var.flags;
		sdlVar = var.sdlVar;
		ageMgr = var.ageMgr;
		if (sdlVar->type == DStruct) {
			// cleanup
			for (tElementList::iterator it = elements.begin(); it != elements.end(); ++it)
				delete it->sdlState;
			// copy each element
			elements.clear();
			for (tElementList::const_iterator it = var.elements.begin(); it != var.elements.end(); ++it) {
				tElement element;
				element.sdlState = new tSdlStateBinary(*it->sdlState);
				elements.push_back(element);
			}
		}
		else {
			// copy whole list
			elements = var.elements;
		}
		return *this;
	}
	
	tSdlStateVar::~tSdlStateVar(void)
	{
		if (sdlVar->type == DStruct) {
			// cleanup
			for (tElementList::iterator it = elements.begin(); it != elements.end(); ++it)
				delete it->sdlState;
		}
	}
	
	void tSdlStateVar::store(tBBuf &t)
	{
		Byte type = t.getByte();
		if (type != 0x02) throw txProtocolError(_WHERE("sdlBinaryVar.type must be 0x02 not 0x%02X", type));
		Byte unk1 = t.getByte();
		if (unk1 != 0x00)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.unk1 of 0x%02X (expected 0x00)", unk1));
		tUStr unk2;
		t.get(unk2);
		if (unk2.size())
			throw txProtocolError(_WHERE("Unexpected non-empty sdlBinary.unk2 of %s", unk2.c_str()));
		// now come the flags
		flags = t.getByte();
		DBG(9, "Flags are 0x%02X\n", flags);
		//Supposicions meaning:
		// 0x04: Timestamp is present
		// 0x08: It has the default value
		// 0x10: unknown (non-struct var?)
		Byte check = 0x04 | 0x08 | 0x10;
		if (flags & ~(check))
			throw txProtocolError(_WHERE("unknown flag 0x%02X for sdlBinaryVar", flags));
		// parse according to flags
		if (flags & 0x04)
			throw txProtocolError(_WHERE("Parsing the timestamp is not yet supported"));
		if (flags & 0x08) {
			if (sdlVar->type == DStruct)
				throw txProtocolError(_WHERE("Default vars for complete structs are not yet supported"));
			// don't read anything, it's still the default
			DBG(9, "Using default for %s\n", sdlVar->name.c_str());
		} else {
			// it's not a default var, we have to do something
			U32 n = sdlVar->size;
			if (!n) // a var with dynamic size
				n = t.getU32();
			DBG(9, "Reading %d values for %s\n", n, sdlVar->name.c_str());
			for (U32 i = 0; i < n; ++i) {
				tElement element;
				switch (sdlVar->type) {
					case DInteger:
						element.intVal = t.getU32();
						DBG(9, "Int value of %d\n", element.intVal);
						break;
					case DFloat:
						element.floatVal = t.getFloat();
						DBG(9, "Float value of %f\n", element.floatVal);
						break;
					case DBool:
					case DByte:
						element.byteVal = t.getByte();
						DBG(9, "Byte/Bool value of 0x%02X\n", element.byteVal);
						break;
					case DStruct:
						element.sdlState = new tSdlStateBinary(ageMgr, sdlVar->structName, sdlVar->structVersion);
						t.getByte(); // FIXME: somewhere we need to read one byte more for recursive structs
						t.get(*element.sdlState);
						break;
					case DTime:
						element.time[0] = t.getU32(); // seconds
						element.time[1] = t.getU32(); // microseconds
						DBG(9, "Time value of %d.%d\n", element.time[0], element.time[1]);
						break;
					default:
						throw txProtocolError(_WHERE("Unable to parse SDL var of type 0x%02X", sdlVar->type));
				}
				elements.push_back(element);
			}
		}
	}
	
	void tSdlStateVar::stream(tBBuf &t)
	{
		throw txUnet(_WHERE("streaming not supported")); // FIXME
	}
	
	//// tSdlStateBinary
	tSdlStateBinary::tSdlStateBinary(void)
	{
		ageMgr = NULL;
		sdlStruct = NULL;
	}
	
	tSdlStateBinary::tSdlStateBinary(tAgeStateManager *ageMgr, tStrBuf name, U32 version)
	{
		this->ageMgr = ageMgr;
		this->sdlStruct = ageMgr->findStruct(name, version);
	}
	
	void tSdlStateBinary::reset(tAgeStateManager *ageMgr, tStrBuf name, U32 version)
	{
		this->ageMgr = ageMgr;
		this->sdlStruct = ageMgr->findStruct(name, version);
		vars.clear();
	}
	
	void tSdlStateBinary::store(tBBuf &t)
	{
		if (sdlStruct == NULL)
			throw txUnet(_WHERE("You have to set a sdlStruct before parsing a sdlBinary"));
		DBG(5, "Parsing a %s version %d\n", sdlStruct->name.c_str(), sdlStruct->version);
		
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
		DBG(7, "nValues: %d\n", nValues);
		if (nValues != sdlStruct->nVar)
			throw txProtocolError(_WHERE("size mismatch, expected %d values, got %d values", sdlStruct->nVar, nValues));
		// parse those values
		tSdlStruct::tVarList::iterator it = sdlStruct->vars.begin(); // we have to know which SDL var we speak about
		for (int i = 0; i < nValues; ++i, ++it) {
			while (it->type == DStruct) ++it; // skip structs
			tSdlStateVar var(&(*it), ageMgr);
			t.get(var); // parse this var
			vars.push_back(var);
		}
		
		// get number of structs
		Byte nStructs = t.getByte();
		DBG(7, "nStructs: %d\n", nStructs);
		if (nStructs != sdlStruct->nStruct)
			throw txProtocolError(_WHERE("size mismatch, expected %d structs, got %d structs", sdlStruct->nStruct, nStructs));
		// parse those structs
		it = sdlStruct->vars.begin(); // we have to know which SDL var we speak about
		for (int i = 0; i < nStructs; ++i, ++it) {
			while (it->type != DStruct) ++it; // skip non-structs
			tSdlStateVar var(&(*it), ageMgr);
			t.get(var); // parse this var
			vars.push_back(var);
		}
	}
	
	void tSdlStateBinary::stream(tBBuf &t)
	{
		throw txUnet(_WHERE("streaming not supported")); // FIXME
	}
	
	//// tSdlState
	tSdlState::tSdlState(const tUruObject &obj, tAgeStateManager *stateMgr) : obj(obj)
	{
		version = 0;
		this->stateMgr = stateMgr;
	}
	
	tSdlState::tSdlState(void)
	{
		version = 0;
		this->stateMgr = NULL;
	}
	
	tMBuf tSdlState::decompress(tBBuf &t)
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
		
		if (!t.eof()) throw txProtocolError(_WHERE("The SDL struct is too long (%d Bytes remaining after parsing)", t.remaining()));
		
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
			t.putU32(0); // the real size 0 for uncompressed messages
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
		tMBuf data = decompress(t);
		// parse "header data"
		data.rewind();
		data.get(name);
		version = data.getU16();
		// parse binary content
		content.reset(stateMgr, name, version);
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
		tMBuf data;
		if (version && name.size() && obj.objName.size()) {
			data.put(name);
			data.putU16(version);
			data.put(content);
		}
		// use tSdlBinary to get the SDL header around it
		tMBuf b = compress(data);
		t.put(b);
	}
	
	bool tSdlState::operator==(const tSdlState &state)
	{
		if (obj != state.obj) return false;
		if (name != state.name) return false;
		return true;
	}
	
	const Byte *tSdlState::str(void)
	{
		dbg.clear();
		dbg.printf("SDL State for [%s]: %s (version %d)", obj.str(), name.c_str(), version);
		return dbg.c_str();
	}

} //end namespace alc

