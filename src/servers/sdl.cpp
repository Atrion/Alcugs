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

#define _DBG_LEVEL_ 5

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/gamemsg.h>

////extra includes
#include "sdl.h"

#include <alcdebug.h>

namespace alc {

	//// tAgeStateManager
	tAgeStateManager::tAgeStateManager(tUnet *net)
	{
		this->net = net;
		log = lnull;
		load();
		
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("sdl");
		if (var.size() < 2) throw txBase(_WHERE("a sdl path must be set"));
		if (!var.endsWith("/")) var.writeStr("/");
		
		// load SDL files
		tDirectory sdlDir;
		tDirEntry *file;
		sdlDir.open((char *)var.c_str());
		while( (file = sdlDir.getEntry()) != NULL) {
			if (file->type != 8 || strcasecmp(alcGetExt(file->name), "sdl") != 0) continue;
			// load it
			loadSdlStructs((var + file->name).c_str());
		}
		// updates the version numbers of the embedded SDL structs (always uses the latest one)
		for (tSdlStructList::iterator it1 = structs.begin(); it1 != structs.end(); ++it1) {
			for (tSdlStruct::tVarList::iterator it2 = it1->vars.begin(); it2 != it1->vars.end(); ++it2) {
				if (it2->type == DStruct) it2->structVersion = findLatestStructVersion(it2->structName);
			}
		}
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
		tStrBuf var = cfg->getVar("agestate.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			log = new tLog("agestate.log", 4, 0);
		}
		
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
				// FIXME: Somehow tell the rest of the clients that this avatar left. Just sending the load message again with isLaod set to 0 doesn't seem to work
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
		tSdlState *sdl = new tSdlState(obj, this);
		data.rewind();
		data.get(*sdl);
		log->log("Got "); // FIXME: Add option to disable detailed logging
		sdl->print(log);
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
	
	tAgeStateManager::tSdlList::iterator tAgeStateManager::findSdlState(tSdlState *state)
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
	
	tAgeStateManager::tCloneList::iterator tAgeStateManager::findClone(const tUruObject &obj)
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
			tSdlState empty;
			buf->put(empty);
		}
	}
	
	U32 tAgeStateManager::findLatestStructVersion(tStrBuf name)
	{
		U32 version = 0;
		for (tSdlStructList::iterator it = structs.begin(); it != structs.end(); ++it) {
			if (it->name == name && it->version > version) version = it->version;
		}
		if (!version) throw txProtocolError(_WHERE("Could not find any SDL struct for %s", name.c_str()));
		return version;
	}
	
	tSdlStruct *tAgeStateManager::findStruct(tStrBuf name, U32 version)
	{
		for (tSdlStructList::iterator it = structs.begin(); it != structs.end(); ++it) {
			if (name == it->name && it->version == version) return &(*it);
		}
		throw txUnet(_WHERE("SDL version mismatch, no SDL Struct found for %s version %d", name.c_str(), version));
	}
	
	/** This is the SDL file parser */
	void tAgeStateManager::loadSdlStructs(const Byte *filename)
	{
		log->log("Reading %s\n", filename);
		tSdlStruct sdlStruct;
		tSdlStructVar sdlVar;
		
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
	1: found a statedesc block, search for name
	2: search for the curly bracket
	-------------------------------------------------
	3: in a statedesc block, search for VERSION
	4: in a statedesc block after VERSION, search version number
	5: in a statedesc block after the version number, search for newline
	-------------------------------------------------
	6: in a statedesc block, search for VAR or the curly bracket
	7: search for var type
	8: search for var name
	9: find additional information (DEFAULT, DEFAULTOPTION or DISPLAYOPTION) or newline
	-------------------------------------------------
	10: find equal sign after DEFAULT
	11: find default data
	-------------------------------------------------
	12: find equal sign after DEFAULTOPTION
	13: find defaultoption data
	-------------------------------------------------
	14: find equal sign after DISPLAYOPTION
	15: find displayoption data
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
				case 1: // found a statedesc block, search for name
					// check if it is a valid name
					if (c.isNewline())
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					sdlStruct = tSdlStruct(c);
					state = 2;
					break;
				case 2: // search for the curly bracket
					if (c == "{")
						state = 3;
					else if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				// ------------------------------------------------------------------------------------------------------------------
				case 3: // in a statedesc block, search for VERSION
					if (c == "VERSION")
						state = 4;
					else if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 4: // in a statedesc block after VERSION, search version number
				{
					U32 version = c.asU32();
					if (!version)
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					sdlStruct.version = version;
					state = 5;
					break;
				}
				case 5: // in a statedesc block after the version number, search for newline
					if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					state = 6;
					break;
				// ------------------------------------------------------------------------------------------------------------------
				case 6: // in a statedesc block, search for VAR or the curly bracket
					if (c == "}") {
						sdlStruct.count(); // count vars and structs
						structs.push_back(sdlStruct); // the STATEDESC is finished now
						state = 0; // go back to state 0 and wait for EOF or next statedesc
					} else if (c == "VAR")
						state = 7;
					else if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 7: // search for var type
					sdlVar = tSdlStructVar(c);
					state = 8;
					break;
				case 8: // search for var name
				{
					bool dynSize =  c.endsWith("[]");
					U32 size = alcParseKey(c);
					if (!size && !dynSize)
						throw txParseError(_WHERE("Parse error at line %d, column %d: Invalid key %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					sdlVar.name = c;
					sdlVar.size = dynSize ? 0 : size; // "0" means dynamic size
					state = 9;
					break;
				}
				case 9: // find additional information (DEFAULT, DEFAULTOPTION, DISPLAYOPTION) or newline
					if (c == "DEFAULT")
						state = 10;
					else if (c == "DEFAULTOPTION")
						state = 12;
					else if (c == "DISPLAYOPTION")
						state = 14;
					else if (c.isNewline()) {
						sdlStruct.vars.push_back(sdlVar); // this VAR is finished, save it
						state = 6; // go back to state 6 and search for next var or end of statedesc
					} else
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				// ------------------------------------------------------------------------------------------------------------------
				case 10: // find equal sign after DEFAULT
					if (c == "=")
						state = 11;
					else
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 11: // find default data
				{
					if (c .startsWith("(") && !c.endsWith(")")) { // it's a part of a tuple, take the whole tuple (several tokes) as default value
						tStrBuf token;
						do {
							token = s.getToken();
							c = c+token;
						} while (!token.endsWith(")"));
					}
					sdlVar.defaultVal = c;
					state = 9; // find more information or the end of the VAR
					break;
				}
				// ------------------------------------------------------------------------------------------------------------------
				case 12: // find equal sign after DEFAULTOPTION
					if (c == "=")
						state = 13;
					else
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 13: // find defaultoption data
				{
					if (c == "VAULT")
						sdlVar.flags |= tSdlStructVar::DVault; // doesn't change anything (yet)
					else if (c == "hidden" && (sdlStruct.name == "EderDelin" || sdlStruct.name == "EderTsogal" || sdlStruct.name == "Minkata" || sdlStruct.name == "Negilahn"))  // work-around for "DEFAULTOPTION=hidden" in some sdl files
						sdlVar.flags |= tSdlStructVar::DHidden; // doesn't change anything (yet)
					else
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					state = 9; // find more information or the end of the VAR
					break;
				}
				// ------------------------------------------------------------------------------------------------------------------
				case 14: // find equal sign after DISPLAYOPTION
					if (c == "=")
						state = 15;
					else
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 15: // find displayoption data
				{
					if (c == "hidden")
						sdlVar.flags |= tSdlStructVar::DHidden; // doesn't change anything (yet)
					else if (c == "red")
						sdlVar.flags |= tSdlStructVar::DRed; // doesn't change anything (yet)
					else if (c == "VAULT" && (sdlStruct.name == "BahroCave" || sdlStruct.name == "Jalak")) // work-around for "DISPLAYOPTION=VAULT" in some sdl files
						sdlVar.flags |= tSdlStructVar::DVault; // doesn't change anything (yet)
					else
						throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected token %s", s.getLineNum(), s.getColumnNum(), c.c_str()));
					state = 9; // find more information or the end of the VAR
					break;
				}
				default:
					throw txParseError(_WHERE("Unknown state %d", state));
			}
		}
		
		if (state != 0)
			throw txParseError(_WHERE("Parse error at line %d, column %d: Unexpected end of file", s.getLineNum(), s.getColumnNum()));
	}
	
	/** The SDL Struct classes */
	//// tSdlStructVar
	tSdlStructVar::tSdlStructVar(tStrBuf type)
	{
		if (type.size()) {
			if (type.startsWith("$")) {
				structName = type.substring(1, type.size()-1);
				this->type = DStruct;
			} else
				this->type = getVarTypeFromName(type);
		}
		else
			this->type = DInvalid;
		size = -1;
		structVersion = 0;
		flags = 0x00;
	}
	
	Byte tSdlStructVar::getVarTypeFromName(tStrBuf type)
	{
		if (type == "INT") return DInteger;
		else if (type == "FLOAT") return DFloat;
		else if (type == "BOOL") return DBool;
		else if (type == "STRING32") return DUruString;
		else if (type == "PLKEY") return DPlKey;
		else if (type == "CREATABLE") return DCreatable;
		else if (type == "TIME") return DTime;
		else if (type == "BYTE" || type == "Byte") return DByte; // don't fail on Prad SDL version 9 which uses "Byte" instead of "BYTE"
		else if (type == "SHORT") return DShort;
		else if (type == "AGETIMEOFDAY") return DAgeTimeOfDay;
		else if (type == "VECTOR3") return DVector3;
		else if (type == "POINT3") return DPoint3;
		else if (type == "QUATERNION") return DQuaternion;
		else if (type == "RGB8") return DRGB8;
		else throw txParseError(_WHERE("Unknown SDL VAR type %s", type.c_str()));
	}
	
	//// tSdlStruct
	tSdlStruct::tSdlStruct(tStrBuf name) : name(name)
	{
		version = 0;
		nVar = nStruct = 0;
	}
	
	void tSdlStruct::count(void)
	{
		nVar = nStruct = 0;
		for (tVarList::iterator it = vars.begin(); it != vars.end(); ++it) {
			if (it->type == DStruct) ++nStruct;
			else ++nVar;
		}
		DBG(7, "%s version %d has %d vars and %d structs\n", name.c_str(), version, nVar, nStruct);
	}

} //end namespace alc

