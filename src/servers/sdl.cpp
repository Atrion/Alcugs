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

//#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <alcnet.h>
#include <protocol/gamemsg.h>

////extra includes
#include "sdl.h"
#include "gameserver.h"

#include <alcdebug.h>

namespace alc {

	//// tAgeStateManager
	tAgeStateManager::tAgeStateManager(tUnetGameServer *net, tAgeInfo *age)
	{
		this->net = net;
		this->age = age;
		log = lnull;
		load();
		
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("sdl");
		if (var.size() < 2) throw txBase(_WHERE("a sdl path must be set"));
		if (!var.endsWith("/")) var.writeStr("/");
		
		// load SDL files
		tDirectory sdlDir;
		tDirEntry *file;
		sdlDir.open(var.c_str());
		while( (file = sdlDir.getEntry()) != NULL) {
			if (file->type != 8 || strcasecmp(alcGetExt(file->name), "sdl") != 0) continue;
			// load it
			loadSdlStructs((var + file->name).c_str());
		}
		// updates the version numbers of the embedded SDL structs (always uses the latest one)
		for (tSdlStructList::iterator it1 = structs.begin(); it1 != structs.end(); ++it1) {
			for (tSdlStruct::tVarList::iterator it2 = it1->vars.begin(); it2 != it1->vars.end(); ++it2) {
				if (it2->type == DStruct) it2->structVersion = findLatestStructVersion(it2->structName.c_str());
			}
		}
		
		// load agestate from file
		tStrBuf fileName = alcLogGetLogPath() + "/agestate.raw";
		var = cfg->getVar("game.load_agestate");
		if (!var.isNull() && var.asByte()) { // disabled per default
			loadAgeState(fileName);
		}
		else // in case the server crashes, remove state to really reset it
			unlink(fileName.c_str());
		
		// if necessary, set up AgeSDLHook
		// sending just a default age SDL for the JoinAck is not enough, we really have to create a SDL state for it
		if (findAgeSDLHook() == sdlStates.end()) { // AgeSDLHook not found
			U32 ageSDLVersion = findLatestStructVersion(age->name, false/*don't throw exception if no struct found*/);
			if (!ageSDLVersion) return;
			// first make up the UruObject
			tUruObject obj;
			obj.pageId = 0x1f | (age->seqPrefix + 1 << 8);
			obj.pageType = 0x0008; // BultIn
			obj.objType = 0x0001; // SceneObject
			obj.objName = "AgeSDLHook";
			// now create the SDL state
			sdlStates.push_back(tSdlState(this, obj, age->name, ageSDLVersion, true/*init default*/));
			log->log("Set up default AgeSDLHook\n");
		}
		
		log->flush();
	}
	
	tAgeStateManager::~tAgeStateManager(void)
	{
		// save state to disk
		tStrBuf fileName = alcLogGetLogPath() + "/agestate.raw";
		saveAgeState(fileName);
		// close log
		unload();
		// make sure the states are removed before the structs as they point to them
		sdlStates.clear();
	}
	
	void tAgeStateManager::loadAgeState(tStrBuf &fileName)
	{
		log->log("Loading age state from %s\n", fileName.c_str());
		// open file
		tFBuf file;
		try {
			file.open(fileName.c_str());
		}
		catch (txNotFound &t) {
			log->log("File %s not found\n", fileName.c_str());
			return;
		}
		
		// read it
		U32 nState = file.getU32();
		for (U32 i = 0; i < nState; ++i) {
			tSdlState state(this, true/*there can be data atfer the end of the struct*/);
			file.get(state);
			if (state.obj.hasCloneId) {
				log->log("Not loading state %s with clone ID\n", state.str());
				continue;
			}
			tSdlList::iterator it = findSdlState(&state);
			if (it != sdlStates.end()) {
				log->log("Got state duplicate: %s and %s. Keeping the one with higher version number.\n", state.str(), it->str());
				if (it->content.getVersion() >= state.content.getVersion()) // keep existing version
					continue;
				else // remove existing version
					sdlStates.erase(it);
			}
			sdlStates.push_back(state);
		}
		if (!file.eof())
			throw txProtocolError(_WHERE("Agestate file is too long"));
		file.close();
	}
	
	void tAgeStateManager::saveAgeState(tStrBuf &fileName)
	{
		log->log("Saving age state to %s\n", fileName.c_str());
		// open file
		tFBuf file;
		file.open(fileName.c_str(), "wb");
		
		// write to it
		file.putU32(sdlStates.size());
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it)
			file.put(*it);
		file.close();
	}
	
	void tAgeStateManager::load(void)
	{
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("agestate.log");
		logDetailed = false;
		if (var.isNull() || var.asByte()) { // logging enabled per default
			log = new tLog("agestate.log", 4, 0);
			var = cfg->getVar("agestate.log.detailed");
			if (!var.isNull() && var.asByte()) // detailed logging disabled per default
				logDetailed = true;
		}
		
		log->log("AgeState backend started (%s)\n", __U_SDL_ID);
		log->flush();
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
	
	void tAgeStateManager::removePlayer(tNetSession *player)
	{
		// check for that player in the clone list
		tCloneList::iterator it = clones.begin();
		while (it != clones.end()) {
			if (it->clonedObj.clonePlayerId == player->ki) { // that clone is dead now, remove it and all of it's SDL states
				log->log("Removing Clone [%s] as it belongs to player %s who just left us\n", it->str(), player->str());
				// remove avatar from age
				it->isLoad = false;
				tmLoadClone loadClone (it->createNetMsg(player, false/*isInitial*/));
				net->bcastMessage(loadClone);
				// remove states from our list
				removeSDLStates(it->clonedObj.clonePlayerId);
				it = clones.erase(it);
			}
			else
				++it;
		}
		log->flush();
	}
	
	void tAgeStateManager::saveSdlState(tMBuf &data)
	{
		tSdlState sdl(this);
		data.rewind();
		data.get(sdl);
		if (logDetailed) {
			log->log("Got ");
			sdl.print(log);
		}
		// check if state is already in list
		tSdlList::iterator it = findSdlState(&sdl);
		if (it == sdlStates.end()) {
			log->log("Adding %s\n", sdl.str());
			sdlStates.push_back(sdl);
		}
		else {
			if (it->content.getVersion() != sdl.content.getVersion()) {
				throw txProtocolError(_WHERE("SDL version mismatch: %s should be changed to %s", it->str(), sdl.str()));
			}
			// update existing state
			log->log("Updating %s\n", sdl.str());
			it->content.updateWith(&sdl.content);
		}
		log->flush();
	}
	
	void tAgeStateManager::saveSdlVaultMessage(tMBuf &data, tNetSession *u)
	{
		// get the content
		tSdlState sdl(this);
		sdl.format = 0x02; // no compression header
		data.get(sdl);
		if (sdl.content.getName() != age->name) return; // ignore updates for other ages
		if (logDetailed) {
			log->log("Got SDL Vault Message ");
			sdl.print(log);
		}
		// find our AgeSDLHook
		tSdlList::iterator sdlHook = findAgeSDLHook();
		if (sdlHook != sdlStates.end()) { // found it - update it
			if (sdlHook->content.getName() != sdl.content.getName() || sdlHook->content.getVersion() != sdl.content.getVersion())
				throw txProtocolError(_WHERE("SDL version mismatch: %s should be changed to %s", sdlHook->str(), sdl.str()));
			log->log("Updating %s\n", sdl.str());
			sdlHook->content.updateWith(&sdl.content);
			// send update to client (for some strange reason, it is enough to send the SDL update to the client who triggered it)
			tmSDLState sdlMsg(u, /*initial*/false);
			sdlMsg.sdl.put(*sdlHook);
			net->send(sdlMsg);
		}
	}
	
	tAgeStateManager::tSdlList::iterator tAgeStateManager::findSdlState(tSdlState *state)
	{
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			if (*it == *state) return it;
		}
		return sdlStates.end();
	}
	
	int tAgeStateManager::sendSdlStates(tNetSession *u, tmGameStateRequest::tPageList *pages)
	{
		int n = 0;
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			if (pages->size()) { // check if this object should be sent
				if (it->obj.hasCloneId) continue; // objects with clone ID are never sent when a page is requested
				bool found = false;
				for (tmGameStateRequest::tPageList::iterator it2 = pages->begin(); it2 != pages->end(); ++it2) {
					if (it->obj.pageId == *it2) {
						found = true;
						break;
					}
				}
				if (!found) continue; // it's not on the list, don't send it
			}
			// FIXME: If no page is requested, we currently send all SDL states, including those which are on a page which is dynamically loaded
			if (logDetailed) {
				log->log("Sending SDL State to %s:\n", u->str());
				it->print(log);
			}
			tmSDLState sdlMsg(u, /*initial*/true);
			sdlMsg.sdl.put(*it);
			net->send(sdlMsg);
			++n;
		}
		log->log("Sent %d SDLState messages to %s\n", n, u->str());
		log->flush();
		return n;
	}
	
	void tAgeStateManager::removeSDLStates(U32 ki, U32 cloneId)
	{
		// remove all SDL states which belong the the object with the clonePlayerId which was passed
		tSdlList::iterator it = sdlStates.begin();
		while (it != sdlStates.end()) {
			if (it->obj.hasCloneId && it->obj.clonePlayerId == ki && (cloneId == 0 || it->obj.cloneId == cloneId)) {
				log->log("Removing %s because Clone was removed\n", it->str());
				it = sdlStates.erase(it);
			}
			else
				++it;
		}
		log->flush();
	}
	
	void tAgeStateManager::saveClone(tLoadCloneMsg &clone)
	{
		tCloneList::iterator it = findClone(clone.clonedObj);
		if (logDetailed) {
			log->log("Got ");
			clone.print(log);
		}
		if (clone.isLoad) {
			if (it != clones.end()) { // it is already in the list, remove old one
				log->log("Updating Clone ");
				clones.erase(it);
			}
			else { // it's a new clone
				log->log("Adding Clone ");
			}
			it = clones.insert(clones.end(), clone);
			log->log("[%s]\n", it->str());
		}
		else { // remove clone if it was in list
			log->log("Removing Clone [%s]\n", clone.str());
			removeSDLStates(clone.clonedObj.clonePlayerId, clone.clonedObj.cloneId); // remove SDL states even if cloenw as not on list
			if (it != clones.end())
				clones.erase(it);
			else
				log->log("WARN: That clone was not on our list!\n");
		}
		log->flush();
	}
	
	tAgeStateManager::tCloneList::iterator tAgeStateManager::findClone(const tUruObject &obj)
	{
		for (tCloneList::iterator it = clones.begin(); it != clones.end(); ++it) {
			if (it->clonedObj == obj) return it;
		}
		return clones.end();
	}
	
	int tAgeStateManager::sendClones(tNetSession *u)
	{
		int n = 0;
		for (tCloneList::iterator it = clones.begin(); it != clones.end(); ++it) {
			if (logDetailed) log->log("Sending to %s: clone [%s]\n", u->str(), it->str());
			tmLoadClone loadClone (it->createNetMsg(u, true/*isInitial*/));
			net->send(loadClone);
			++n;
		}
		log->log("Sent %d LoadClone messages to %s\n", n, u->str());
		return n;
	}
	
	tAgeStateManager::tSdlList::iterator tAgeStateManager::findAgeSDLHook(void)
	{
		// look for the AgeSDLHook
		tSdlList::iterator sdlHook = sdlStates.end();
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			if (it->obj.objName == "AgeSDLHook") {
				if (sdlHook != sdlStates.end()) log->log("ERR: Two SDL Hooks found!\n");
				sdlHook = it;
			}
		}
		return sdlHook;
	}
	
	void tAgeStateManager::writeAgeState(tMBuf *buf)
	{
		tSdlList::iterator sdlHook = findAgeSDLHook();
		if (sdlHook != sdlStates.end()) { // found it - send it
			if (sdlHook->content.isIndexed())
				log->log("ERR: The Age SDL Hook is incomplete, strange behaviour of the client is guaranteed\n");
			log->log("Sending Age SDL Hook");
			if (logDetailed) {
				log->print(": ");
				sdlHook->print(log);
			}
			else
				log->nl();
			sdlHook->format = 0x01; // write it without the object
			buf->put(*sdlHook);
			sdlHook->format = 0x00; // restore the correct format
		}
		else { // not found
			log->log("No Age SDL hook found, send empty SDL\n");
			tSdlState empty;
			empty.format = 0x01; // write it without the object
			buf->put(empty);
		}
		log->flush();
	}
	
	U32 tAgeStateManager::findLatestStructVersion(const char *name, bool throwOnError)
	{
		U32 version = 0;
		for (tSdlStructList::iterator it = structs.begin(); it != structs.end(); ++it) {
			if (it->name == name && it->version > version) version = it->version;
		}
		if (throwOnError && !version) throw txProtocolError(_WHERE("Could not find any SDL struct for %s", name));
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
	void tAgeStateManager::loadSdlStructs(const char *filename)
	{
		log->log("Reading %s\n", filename);
		tSdlStruct sdlStruct(this);
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
					sdlStruct = tSdlStruct(this, c);
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
				this->type = alcUnetGetVarTypeFromName(type);
		}
		else
			this->type = DInvalid;
		size = -1;
		structVersion = 0;
		flags = 0x00;
	}
	
	//// tSdlStruct
	tSdlStruct::tSdlStruct(tAgeStateManager *stateMgr, tStrBuf name) : name(name)
	{
		this->stateMgr = stateMgr;
		version = 0;
		nVar = nStruct = 0;
	}
	
	void tSdlStruct::count(void)
	{
		if (vars.size() > 255) throw txUnet(_WHERE("A SDL struct must never mave more than 255 entries!"));
		nVar = nStruct = 0;
		for (tVarList::iterator it = vars.begin(); it != vars.end(); ++it) {
			if (it->type == DStruct) ++nStruct;
			else ++nVar;
		}
		DBG(7, "%s version %d has %d vars and %d structs\n", name.c_str(), version, nVar, nStruct);
	}
	
	tSdlStructVar *tSdlStruct::getElement(int nr, bool var)
	{
		int curNr = 0;
		for (tVarList::iterator it = vars.begin(); it != vars.end(); ++it) {
			if ((var && it->type != DStruct) || (!var && it->type == DStruct)) {
				if (nr == curNr) return &(*it);
				++curNr;
			}
		}
		throw txProtocolError(_WHERE("Could not find variable/struct number %d", nr));
	}

} //end namespace alc

