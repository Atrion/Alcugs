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
#define __U_SDL_ID "$Id$"

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/gamemsg.h>

////extra includes
#include "sdl.h"

#include <alcdebug.h>

namespace alc {

	////IMPLEMENTATION
	//// tSdlBinary
	tSdlBinary::tSdlBinary(void)
	{
		compress = false;
	}
	
	tSdlBinary::tSdlBinary(const tMBuf &b) : data(b)
	{
		compress = (data.size() > 255); // just a guess
	}
	
	void tSdlBinary::store(tBBuf &t)
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
	}
	
	void tSdlBinary::stream(tBBuf &t)
	{
		if (!data.size()) {
			// write an empty SDL
			t.putU32(0); // real size
			t.putByte(0x00); // compression flag
			t.putU32(0); // sent size
			return;
		}
		
		// it's not yet empty, so we have to write something
		if (compress) {
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
	}
	
	//// tSdlState
	tSdlState::tSdlState(const tUruObject &obj) : obj(obj)
	{
		version = 0;
	}
	
	void tSdlState::store(tBBuf &t)
	{
		unparsed.clear();
		// use tSdlBinay to get the message body
		tSdlBinary bin;
		t.get(bin);
		// parse it
		tBBuf *b = &bin.data;
		b->rewind();
		b->get(name);
		version = b->getU16();
		U32 size = b->remaining();
		unparsed.write(b->read(size), size); // FIXME: parse more
	}
	
	void tSdlState::stream(tBBuf &t)
	{
		tMBuf b;
		b.put(name);
		b.putU16(version);
		b.put(unparsed);
		// use tSdlBinary to get the SDL header around it
		tSdlBinary bin(b);
		t.put(bin);
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
	
	//// tAgeStateManager
	tAgeStateManager::tAgeStateManager(tUnet *net)
	{
		this->net = net;
		log = lnull;
		load();
	}
	
	tAgeStateManager::~tAgeStateManager(void)
	{
		unload();
		for (tCloneList::iterator it = clones.begin(); it != clones.end(); ++it) {
			delete *it;
		}
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			delete *it;
		}
		
		// FIXME: Set up default AgeSDLHook
	}
	
	void tAgeStateManager::load(void)
	{
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("game.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			log = new tLog("agestate.log", 4, 0);
		}
		
		var = cfg->getVar("sdl");
		if (var.size() < 2) throw txBase(_WHERE("a sdl path must be set"));
		strncpy(sdlDir, var.c_str(), 255);
		
		// FIXME: Load all SDL files in that directory
		loadSdlStructs((var + "/Dustin.sdl").c_str());
		
		log->log("AgeState backend started (%s)\n", __U_SDL_ID);
	}
	
	void tAgeStateManager::unload(void)
	{
		if (log != lnull) {
			delete log;
			log = lnull;
		}
	}
	
	void tAgeStateManager::reload(void)
	{
		unload();
		load();
	}
	
	void tAgeStateManager::removePlayer(U32 ki)
	{
		// check for that player in the clone list
		tCloneList::iterator it = clones.begin();
		while (it != clones.end()) {
			if ((*it)->obj.clonePlayerId == ki) { // that clone is dead now, remov it and all of it's SDL states
				log->log("Removing Clone [%s] as it belongs to player with KI %d who just left us\n", (*it)->obj.str(), ki);
				// FIXME: Somehow tell the rest of the clients that this one left. Just sending the load message again with isLaod set to 0 doesn't seem to work
				// remove states from our list
				removeSDLStates((*it)->obj.clonePlayerId);
				delete *it;
				clones.erase(it++);
			}
			else
				++it;
		}
	}
	
	void tAgeStateManager::saveSdlState(const tUruObject &obj, tMBuf &data)
	{
		tSdlState *sdl = new tSdlState(obj);
		data.rewind();
		data.get(*sdl);
		// check if state is already in list
		tSdlList::iterator it = findSdlState(sdl);
		if (it == sdlStates.end()) {
			log->log("Adding %s\n", sdl->str());
			sdlStates.push_back(sdl);
		}
		else {
			if ((*it)->version > sdl->version) {
				// don't override the SDL state with a state described in an older version
				throw txProtocolError(_WHERE("SDL version mismatch: %s should be downgraded to %s", (*it)->str(), sdl->str()));
			}
			else { // it's the same or a newer version, use it
				log->log("Updating %s\n", sdl->str());
				delete *it;
				*it = sdl;
			}
		}
	}
	
	tSdlList::iterator tAgeStateManager::findSdlState(tSdlState *state)
	{
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			if (**it == *state) return it;
		}
		return sdlStates.end();
	}
	
	int tAgeStateManager::sendSdlStates(tNetSession *u)
	{
		int n = 0;
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			tmSDLState sdlMsg(u, (*it)->obj, /*initial*/true);
			sdlMsg.sdl.put(**it);
			net->send(sdlMsg);
			++n;
		}
		log->log("Sent %d SDLState messages to %s\n", n, u->str());
		return n;
	}
	
	void tAgeStateManager::removeSDLStates(U32 ki, U32 cloneId)
	{
		// remove all SDL states which belong the the object with the clonePlayerId which was passed
		tSdlList::iterator it = sdlStates.begin();
		while (it != sdlStates.end()) {
			if ((*it)->obj.hasCloneId && (*it)->obj.clonePlayerId == ki && (cloneId == 0 || (*it)->obj.cloneId == cloneId)) {
				log->log("Removing %s because Clone was removed\n", (*it)->str());
				delete *it;
				sdlStates.erase(it++);
			}
			else
				++it;
		}
	}
	
	void tAgeStateManager::saveClone(const tmLoadClone &clone)
	{
		tCloneList::iterator it = findClone(clone.obj);
		if (clone.isLoad) {
			tmLoadClone *savedClone = new tmLoadClone(clone);
			savedClone->isInitial = true; // it is sent as initial clone later
			if (it != clones.end()) { // it is already in the list, don't save a duplicate but replace the existing one
				log->log("Updating Clone [%s]\n", savedClone->obj.str());
				delete *it;
				*it = savedClone;
			}
			else { // add new clone
				log->log("Adding Clone [%s]\n", savedClone->obj.str());
				clones.push_back(savedClone);
			}
		}
		else if (it != clones.end()) { // remove clone if it was in list
			log->log("Removing Clone [%s]\n", (*it)->obj.str());
			removeSDLStates((*it)->obj.clonePlayerId, (*it)->obj.cloneId);
			delete *it;
			clones.erase(it);
		}
	}
	
	tCloneList::iterator tAgeStateManager::findClone(const tUruObject &obj)
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
		log->log("Sent %d LoadClone messages to %s\n", n, u->str());
		return n;
	}
	
	void tAgeStateManager::writeAgeState(tMBuf *buf)
	{
		// look for the AgeSDLHook
		tSdlState *sdlHook = NULL;
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			if ((*it)->obj.objName == "AgeSDLHook") {
				if (sdlHook) log->log("ERR: Two SDL Hooks found!\n");
				sdlHook = *it;
			}
		}
		if (sdlHook) { // found it - send it
			buf->put(*sdlHook);
		}
		else {
			log->log("No SDL hook found, send empty SDL\n");
			tSdlBinary empty;
			buf->put(empty);
		}
	}
	
	/** This is the SDL file parser */
	void tAgeStateManager::loadSdlStructs(const Byte *filename)
	{
		DBG(9, "Loading %s\n", filename);
		
		// open and decrypt file
		tFBuf sdlFile;
		sdlFile.open(filename, "r");
		tWDYSBuf sdlContent;
		sdlFile.get(sdlContent);
		sdlContent.decrypt();
		
		Byte state = 0;
/*
	states:
	0: out of a statedesc block, wait for one
*/
		tStrBuf s(sdlContent), c;
		while (!s.eof()) {
			c = s.getToken(); // getToken already skips comments for us, so we don't have to care about that
			DBG(9, "Token: %s, state: %d\n", c.c_str(), state);
			switch (state) {
				case 0: // out of a statedesc block, wait for one
					if (c == "STATEDESC")
						state = 1;
					else if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				default:
					throw txParseError(_WHERE("Unknown state %d", state));
			}
		}
		
		// TODO: Finish it
	}

} //end namespace alc

