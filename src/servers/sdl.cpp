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
#define __U_GAMESERVER_ID "$Id$"

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/gamemsg.h>

////extra includes
#include "sdl.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	//// tSdlState
	tSdlState::tSdlState(tUruObject &obj) : obj(obj)
	{
		compress = false;
		version = 0;
	}
	
	tSdlState::tSdlState(const tSdlState &state) : obj(state.obj), data(state.data)
	{
		name = state.name;
		compress = state.compress;
		version = state.version;
	}
	
	void tSdlState::store(tBBuf &t)
	{
		data.clear();
	
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
				compress = true;
				tZBuf *content = new tZBuf;
				content->write(t.read(sentSize), sentSize);
				content->uncompress(realSize);
				buf = content;
			}
			else if (compressed == 0x00) {
				realSize = sentSize;
				compress = false;
			}
			else
				throw txProtocolError(_WHERE("unknown compression format 0x%02X", compressed));
			U32 dataEnd = buf->tell() + realSize;
			
			// get version information
			buf->get(name);
			version = buf->getU16();
			DBG(5, "Got SDL message of type %s, version %d\n", name.c_str(), version);
			
			// get SDL binary. FIXME: parse it
			U32 dataSize = dataEnd - buf->tell();
			data.write(buf->read(dataSize), dataSize);
			
			if (compressed == 0x02) { // it was compressed, so we have to clean up
				if (!buf->eof()) throw txProtocolError(_WHERE("Message is too long")); // there must not be any byte after what we parsed above
				delete buf;
			}
		}
		else {
			DBG(5, "Got empty SDL message\n");
		}
		
		if (!t.eof()) throw txProtocolError(_WHERE("The SDL struct is too long (%d Bytes remaining after parsing)", t.remaining()));
	}
	
	void tSdlState::stream(tBBuf &t)
	{
		if (!data.size()) {
			// write an empty SDL
			t.putU32(0); // real size
			t.putByte(0); // compression flag
			t.putU32(0); // sent size
			return;
		}
		throw txProtocolError(_WHERE("Writing a non-empty SDL is not supported"));
	}
	
	//// tAgeStateManager
	tAgeStateManager::tAgeStateManager(tUnet *net)
	{
		this->net = net;
	}
	
	void tAgeStateManager::saveSdlState(tUruObject &obj, tMBuf &data)
	{
		tSdlState sdl(obj);
		data.rewind();
		data.get(sdl);
		// FIXME: do something
	}
	
	tAgeStateManager::~tAgeStateManager(void)
	{
		for (tCloneList::iterator it = clones.begin(); it != clones.end(); ++it) {
			delete *it;
		}
	}
	
	void tAgeStateManager::saveClone(tmLoadClone &clone)
	{
		tCloneList::iterator it = findClone(clone.obj);
		if (clone.isLoad) {
			tmLoadClone *savedClone = new tmLoadClone(clone);
			savedClone->isInitial = true; // it is sent as initial clone later
			if (it != clones.end()) { // it is already in the list, don't save a duplicate
				delete *it;
				*it = savedClone;
			}
			else
				clones.push_back(savedClone);
		}
		else if (it != clones.end()) { // remove clone if it was in list
			delete *it;
			clones.erase(it);
		}
		
		// FIXME: make somehow sure that the clone for a player is also removed when the player leaves
	}
	
	tCloneList::iterator tAgeStateManager::findClone(tUruObject &obj)
	{
		for (tCloneList::iterator it = clones.begin(); it != clones.end(); ++it) {
			if ((*it)->obj == obj) return it;
		}
		return clones.end();
	}
	
	int tAgeStateManager::sendClones(tNetSession *u)
	{
		int n = 0;
		for (tCloneList::iterator it = clones.begin(); it != clones.end(); ++it) {
			tmLoadClone cloneMsg(u, **it);
			net->send(cloneMsg);
			++n;
		}
		return n;
	}
	
	void tAgeStateManager::writeAgeState(tMBuf *buf)
	{
		tUruObject obj;
		tSdlState state(obj);
		buf->put(state);
		// FIXME: actually write a useful SDL state
	}

} //end namespace alc

