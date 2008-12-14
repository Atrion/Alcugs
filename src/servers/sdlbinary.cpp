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
#include "sdl.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	/** SDL binary processing classes */
	
	//// tSdlStateVar
	void tSdlStateVar::store(tBBuf &t)
	{
		Byte type = t.getByte();
		if (type != 0x02) throw txProtocolError(_WHERE("sdlBinaryVar.type must be 0x02 to 0x%02X", type));
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
	}
	
	void tSdlStateVar::stream(tBBuf &t)
	{
		throw txProtocolError(_WHERE("streaming not supported")); // FIXME
	}
	
	//// tSdlStateBinary
	tSdlStateBinary::tSdlStateBinary(const tSdlStruct *sdlStruct)
	{ reset(sdlStruct); }
	
	void tSdlStateBinary::store(tBBuf &t)
	{
		if (sdlStruct == NULL)
			throw txProtocolError(_WHERE("You have to set a sdlStruct before parsing a sdlBinary"));
		U16 unk1 = t.getU16();
		if (unk1 != 0x0000)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.unk1 of 0x%04X (expected 0x0000)", unk1));
		Byte unk2 = t.getByte();
		if (unk2 != 0x06)
			throw txProtocolError(_WHERE("Unexpected sdlBinary.unk1 of 0x%02X (expected 0x06)", unk2));
		// parse the variables which are in here
		Byte nValues = t.getByte();
		DBG(7, "nValues: %d\n", nValues);
		if (nValues != sdlStruct->nVar)
			throw txProtocolError(_WHERE("size mismatch, expected %d values, got %d values", sdlStruct->nVar, nValues));
		for (int i = 0; i < nValues; ++i) {
			tSdlStateVar var;
			t.get(var); // parse this var
			vars.push_back(var);
		}
	}
	void tSdlStateBinary::stream(tBBuf &t)
	{
		throw txProtocolError(_WHERE("streaming not supported")); // FIXME
	}
	
	void tSdlStateBinary::reset(const tSdlStruct *sdlStruct)
	{
		this->sdlStruct = sdlStruct;
		vars.clear();
	}
	
	//// tSdlState
	tSdlState::tSdlState(const tUruObject &obj, const tSdlStructList *structs) : obj(obj)
	{
		version = 0;
		this->structs = structs;
	}
	
	tSdlState::tSdlState(void)
	{
		version = 0;
		this->structs = NULL;
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
		// use tSdlBinay to get the message body and decompress stuff
		tMBuf data = decompress(t);
		// parse "header data"
		data.rewind();
		data.get(name);
		version = data.getU16();
		// parse binary content
		DBG(5, "Parsing a %s version %d\n", name.c_str(), version);
		content.reset(findStruct());
		data.get(content);
		if (!data.eof()) throw txProtocolError(_WHERE("The SDL struct is too long (%d Bytes remaining after parsing)", data.remaining()));
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
	
	const tSdlStruct *tSdlState::findStruct(void)
	{
		for (tSdlStructList::const_iterator it = structs->begin(); it != structs->end(); ++it) {
			if (name == it->name && it->version == version) return &(*it);
		}
		throw txProtocolError(_WHERE("SDL version mismatch, no SDL Struct found for %s version %d", name.c_str(), version));
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

