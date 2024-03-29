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

//#define _DBG_LEVEL_ 8
#include <alcdefs.h>
#include "sdl.h"

#include "sdl.h"
#include "gameserver.h"
#include <alcutil/alcos.h>
#include <alcutil/alcparser.h>
#include <urutypes/plmessage.h>
#include <urutypes/ageinfo.h>
#include <alcmain.h>
#include <netexception.h>

#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace alc {

	//// tAgeStateManager
	tAgeStateManager::tAgeStateManager(tUnetGameServer *net, tAgePages *pages) : net(net), agePages(pages), initialized(false)
	{}
	
	tAgeStateManager::~tAgeStateManager(void)
	{
		// save state to disk
		saveAgeState();
		// make sure the states are removed before the structs as they point to them
		sdlStates.clear();
		// remove clone messages
		tCloneList::iterator it = clones.begin();
		while (it != clones.end()) {
			delete *it;
			it = clones.erase(it);
		}
	}
	
	void tAgeStateManager::applyConfig(void)
	{
		tConfig *cfg = alcGetMain()->config();
		tString var = cfg->getVar("agestate.log");
		logDetailed = false;
		if (var.isEmpty() || var.asInt()) { // logging enabled per default
			log.open("agestate.log");
			var = cfg->getVar("agestate.log.detailed");
			if (!var.isEmpty() && var.asInt()) // detailed logging disabled per default
				logDetailed = true;
		}
		else
			log.close();
		
		if (!initialized) {
			bool found;
			tString var = cfg->getVar("sdl");
			if (var.size() < 2) throw txUnet(_WHERE("a sdl path must be set"));
			if (!var.endsWith("/")) var.writeStr("/");
			
			// load SDL files
			log.log("Reading SDL files from %s...\n", var.c_str());
			tDirectory sdlDir;
			tDirEntry file;
			sdlDir.open(var);
			while(!(file = sdlDir.getEntry()).isNull()) {
				if (!file.isFile() || alcGetExt(file.name).lower() != "sdl") continue;
				// load it
				loadSdlStructs(var + file.name);
			}
			// updates the version numbers of the embedded SDL structs (always uses the latest one)
			for (tSdlStructList::iterator it1 = structs.begin(); it1 != structs.end(); ++it1) {
				for (tSdlStruct::tVarList::iterator it2 = it1->vars.begin(); it2 != it1->vars.end(); ++it2) {
					if (it2->type == DStruct) it2->structVersion = findLatestStructVersion(it2->structName);
				}
			}
			
			// Check if we should load the state
			var = cfg->getVar("game.tmp.hacks.resetting_ages", &found);
			if (!found) var = "Kveer,Garden"; // see uru.conf.dist for explanation
			bool ageLoadsState = doesAgeLoadState(var, net->getName());
			// load agestate from file (do the migration even if we should not load)
			ageStateFile = cfg->getVar("game.agestates");
			if (ageStateFile.isEmpty())
				throw txUnet(_WHERE("You have to set the game.agestates to the directory for saving your agestate files"));
			alcMkdir(ageStateFile, 00750); // make sure the path exists
			ageStateFile.printf("/%s-%s.state", net->getName().c_str(), alcGetStrGuid(net->getGuid()).c_str());
			// check for old agestate location and migrate if necessary
			tString alternativeStateFile = log.getDir() + "agestate.raw";
			if (access(alternativeStateFile.c_str(), F_OK) == 0)
				rename(alternativeStateFile.c_str(), ageStateFile.c_str());
			// ok, now go and load it - maybe
			if (ageLoadsState) {
				loadAgeState();
			}
			else { // in case the server crashes, remove state to really reset it
				log.log("Not loading the age state - starting from scratch\n");
				unlink(ageStateFile.c_str());
				ageStateFile.clear(); // don't save age state
			}
			initialized = true;
		}
		
		log.log("AgeState backend started\n");
	}
	
	bool tAgeStateManager::doesAgeLoadState(const tString &resettingAges, const tString &age)
	{
		tString ages = ","+resettingAges+",";
		return ages.find(","+age+",") == npos;
	}
	
	void tAgeStateManager::loadAgeState()
	{
		log.log("Loading age state from %s\n", ageStateFile.c_str());
		// open file
		tFBuf file;
		try {
			file.open(ageStateFile.c_str());
		}
		catch (txNotFound &t) {
			log.log("File %s not found\n", ageStateFile.c_str());
			return;
		}
		
		// read it
		uint32_t nState = file.get32();
		for (uint32_t i = 0; i < nState; ++i) {
			tUruObject obj;
			file.get(obj);
			tStreamedObject sdlStream;
			file.get(sdlStream);
			if (obj.hasCloneId || !agePages->validPage(obj.pageId)) {
				log.log("Not loading state for object %s with clone ID or on a page not belonging to this age\n", obj.str().c_str());
				continue;
			}
			if (sdlStream.getType() != plNull)
				throw txProtocolError(_WHERE("Plasma object type of an SDL must be plNull"));
			
			tSdlState state(this, sdlStream, obj);
			tSdlList::iterator it = findSdlState(&state);
			if (it != sdlStates.end()) {
				log.log("Got state duplicate: %s and %s. Keeping the one with higher version number.\n", state.str().c_str(), it->str().c_str());
				if (it->content.getVersion() >= state.content.getVersion()) // keep existing version
					continue;
				else // remove existing version
					sdlStates.erase(it);
			}
			if (logDetailed) {
				log.log("Loaded ");
				state.print(&log);
			}
			// check if that struct can be updated
			uint16_t structVersion = findLatestStructVersion(state.content.getName());
			if (structVersion > state.content.getVersion()) {
				state.content.updateTo(findStruct(state.content.getName(), structVersion));
				if (logDetailed) {
					log.log("Updated it to ");
					state.print(&log);
				}
			}
			// save state
			sdlStates.push_back(state);
		}
		if (!file.eof())
			throw txProtocolError(_WHERE("Agestate file is too long"));
		file.close();
	}
	
	void tAgeStateManager::saveAgeState()
	{
		if (ageStateFile.isEmpty()) return;
		log.log("Saving age state to %s\n", ageStateFile.c_str());
		// open file
		tFBuf file;
		file.open(ageStateFile.c_str(), "wb");
		
		// write to it
		file.put32(sdlStates.size());
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			file.put(it->obj);
			
			tStreamedObject sdl;
			sdl.put(*it);
			sdl.compress();
			file.put(sdl);
		}
		file.close();
	}
	
	void tAgeStateManager::removePlayer(tNetSession *player)
	{
		// check for that player in the clone list
		tCloneList::iterator it = clones.begin();
		while (it != clones.end()) {
			if ((*it)->clonedObj.obj.clonePlayerId == player->ki) { // that clone is dead now, remove it and all of it's SDL states
				log.log("Removing Clone [%s] as it belongs to player %s who just left us\n", (*it)->clonedObj.str().c_str(), player->str().c_str());
				if (net->getLinkingOutIdle() && (*it)->getType() == plLoadAvatarMsg) { // make avatars idle
					// make sure that clone is in the idle state
					net->bcastMessage(net->makePlayerIdle(player, (*it)->clonedObj.obj));
					// make sure the clone does not run an animation
					net->bcastMessage(net->makePlayerIdle(player, (*it)->clonedObj.obj, 1), 0.2); // 200msecs later, let it walk forwards
					net->bcastMessage(net->makePlayerIdle(player, (*it)->clonedObj.obj, 0), 0.2+0.1); // again 100msecs later, it stops walking
				}
				// remove states from our list
				removeCloneStates((*it)->clonedObj.obj.clonePlayerId);
				// remove avatar from age (a delay of < 2700msecs is likely to cause crashes when the avatar just left the sitting state)
				(*it)->isLoad = false;
				tmLoadClone loadClone(player, *it, false/*isInitial*/);
				net->bcastMessage(loadClone, /*delay*/ net->getLinkingOutIdle() ? 3 : 0);
				delete *it;
				it = clones.erase(it);
			}
			else
				++it;
		}
	}
	
	void tAgeStateManager::saveSdlState(tMBuf &data, const tUruObject &obj)
	{
		tSdlState sdl(this, data, obj);
		if (logDetailed) {
			log.log("Got ");
			sdl.print(&log);
		}
		// check if this state is allowed in this age
		if (!sdl.obj.hasCloneId && !agePages->validPage(sdl.obj.pageId))
			throw txProtocolError(_WHERE("Object %s is not in this age", sdl.obj.str().c_str()));
		// check if state is already in list
		tSdlList::iterator it = findSdlState(&sdl);
		if (it == sdlStates.end()) {
			log.log("Adding %s\n", sdl.str().c_str());
			sdlStates.push_back(sdl);
		}
		else {
			if (it->content.getVersion() != sdl.content.getVersion()) {
				throw txProtocolError(_WHERE("SDL version mismatch: %s should be changed to %s", it->str().c_str(), sdl.str().c_str()));
			}
			// update existing state
			log.log("Updating %s\n", sdl.str().c_str());
			it->content.updateWith(&sdl.content);
		}
	}
	
	void tAgeStateManager::saveSdlVaultMessage(tMBuf &data, tNetSession *u)
	{
		// get the content
		data.rewind();
		uint16_t type = data.get16();
		if (type != plNull)
			throw txProtocolError(_WHERE("Plasma object type of an SDL must be plNull"));
		tSdlState sdl(this, data);
		if (sdl.content.getName() != agePages->getAgeName()) return; // ignore updates for other ages
		if (logDetailed) {
			log.log("Got SDL Vault Message ");
			sdl.print(&log);
		}
		// find our AgeSDLHook
		tSdlList::iterator sdlHook = findAgeSDLHook();
		if (sdlHook != sdlStates.end()) { // found it - update it
			if (sdlHook->content.getName() != sdl.content.getName() || sdlHook->content.getVersion() != sdl.content.getVersion())
				throw txProtocolError(_WHERE("SDL version mismatch: %s should be changed to %s", sdlHook->str().c_str(), sdl.str().c_str()));
			log.log("Updating %s\n", sdl.str().c_str());
			sdlHook->content.updateWith(&sdl.content);
			// send update to client (for some strange reason, it is enough to send the SDL update to the client who triggered it)
			tmSDLState sdlMsg(u, sdlHook->obj, &*sdlHook, /*initial*/false);
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
			if (pages->size()) { // it is necessary to check if this object should be sent
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
			else if (!it->obj.hasCloneId) { // objects with CloneID are ok
				if (agePages->isConditionallyLoaded(it->obj.pageId)) continue; // don't send this one, it's on an optional page
			}
			if (logDetailed) {
				log.log("Sending SDL State to %s:\n", u->str().c_str());
				it->print(&log);
			}
			tmSDLState sdlMsg(u, it->obj, &*it, /*initial*/true);
			net->send(sdlMsg);
			++n;
		}
		log.log("Sent %d SDLState messages to %s\n", n, u->str().c_str());
		return n;
	}
	
	void tAgeStateManager::removeCloneStates(uint32_t ki, uint32_t cloneId)
	{
		// remove all SDL states which belong the the object with the clonePlayerId which was passed
		tSdlList::iterator it = sdlStates.begin();
		while (it != sdlStates.end()) {
			if (it->obj.hasCloneId && it->obj.clonePlayerId == ki && (cloneId == 0 || it->obj.cloneId == cloneId)) {
				log.log("Removing %s because Clone was removed\n", it->str().c_str());
				it = sdlStates.erase(it);
			}
			else
				++it;
		}
	}
	
	void tAgeStateManager::saveClone(tpLoadCloneMsg *clone)
	{
		tCloneList::iterator it = findClone(clone->clonedObj.obj);
		if (logDetailed) {
			log.log("Got Message %s", clone->str().c_str());
		}
		if (clone->isLoad) {
			if (it != clones.end()) { // it is already in the list, remove old one
				log.log("Updating Clone ");
				delete *it;
				clones.erase(it);
			}
			else { // it's a new clone
				log.log("Adding Clone ");
			}
			log.print("[%s]\n", clone->clonedObj.str().c_str());
			it = clones.insert(clones.end(), clone); // save the message we got
		}
		else { // remove clone if it was in list
			log.log("Removing Clone [%s]\n", clone->clonedObj.str().c_str());
			// remove SDL states even if clone is not on list, just to be sure
			removeCloneStates(clone->clonedObj.obj.clonePlayerId, clone->clonedObj.obj.cloneId);
			if (it != clones.end()) {
				delete *it;
				clones.erase(it);
			} else
				log.log("WARN: Clone [%s] was not on our list!\n", clone->clonedObj.str().c_str());
			delete clone; // remove the message we got
		}
	}
	
	tAgeStateManager::tCloneList::iterator tAgeStateManager::findClone(const tUruObject &obj)
	{
		for (tCloneList::iterator it = clones.begin(); it != clones.end(); ++it) {
			if ((*it)->clonedObj.obj == obj) return it;
		}
		return clones.end();
	}
	
	int tAgeStateManager::sendClones(tNetSession *u)
	{
		int n = 0;
		for (tCloneList::iterator it = clones.begin(); it != clones.end(); ++it) {
			if (logDetailed) log.log("Sending to %s: clone [%s]\n", u->str().c_str(), (*it)->clonedObj.str().c_str());
			tmLoadClone loadClone(u, *it, true/*isInitial*/);
			net->send(loadClone);
			++n;
		}
		log.log("Sent %d LoadClone messages to %s\n", n, u->str().c_str());
		return n;
	}
	
	tAgeStateManager::tSdlList::iterator tAgeStateManager::findAgeSDLHook(void)
	{
		// look for the AgeSDLHook
		tSdlList::iterator sdlHook = sdlStates.end();
		for (tSdlList::iterator it = sdlStates.begin(); it != sdlStates.end(); ++it) {
			if (it->obj.objName == "AgeSDLHook") {
				if (sdlHook != sdlStates.end()) log.log("ERR: Two SDL Hooks found!\n");
				sdlHook = it;
			}
		}
		if (sdlHook != sdlStates.end()) return sdlHook;
		// ok, there is none, let's try to create it
		uint16_t ageSDLVersion = findLatestStructVersion(agePages->getAgeName(), false/*don't throw exception if no struct found*/);
		if (!ageSDLVersion) // no way, nothing to be done
			return sdlStates.end();
		// first make up the UruObject
		tUruObject obj;
		obj.pageId = alcPageNumberToId(agePages->getAgePrefix(), 254); // BultIn page ID
		obj.pageType = 0x0008; // BultIn
		obj.objType = 0x0001; // SceneObject
		obj.objName = "AgeSDLHook";
		// now create the SDL state
		sdlHook = sdlStates.insert(sdlStates.end(), tSdlState(this, obj, agePages->getAgeName(), ageSDLVersion, true/*init default*/));
		log.log("Set up default AgeSDLHook: %s\n", sdlHook->str().c_str());
		return sdlHook;
	}
	
	const tStreamable *tAgeStateManager::getAgeState(void)
	{
		tSdlList::iterator sdlHook = findAgeSDLHook();
		if (sdlHook != sdlStates.end()) { // found it - send it
			if (sdlHook->content.isIndexed())
				log.log("ERR: The Age SDL Hook is incomplete, strange behaviour of the client is guaranteed\n");
			log.log("Sending Age SDL Hook");
			if (logDetailed) {
				log.print(": ");
				sdlHook->print(&log);
			}
			else
				log.nl();
			return &*sdlHook;
		}
		else { // not found
			log.log("No Age SDL hook found, send empty SDL\n");
			return NULL;
		}
	}
	
	uint16_t tAgeStateManager::findLatestStructVersion(const tString &name, bool throwOnError)
	{
		DBG(6, "Looking for latest version of %s\n", name.c_str());
		uint16_t version = 0;
		for (tSdlStructList::iterator it = structs.begin(); it != structs.end(); ++it) {
			if (it->name == name && it->version > version) {
				version = it->version;
				DBG(6, "Found version %d\n", version);
			}
		}
		if (throwOnError && !version) throw txProtocolError(_WHERE("Could not find any SDL struct for %s", name.c_str()));
		return version;
	}
	
	tSdlStruct *tAgeStateManager::findStruct(tString name, uint16_t version, bool throwOnError)
	{
		for (tSdlStructList::iterator it = structs.begin(); it != structs.end(); ++it) {
			if (name == it->name && it->version == version) return &(*it);
		}
		if (throwOnError)
			throw txUnet(_WHERE("No SDL Struct found for %s version %d", name.c_str(), version));
		return NULL;
	}
	
	void tAgeStateManager::clearAllStates(void)
	{
		// remove clones
		tCloneList::iterator it = clones.begin();
		while (it != clones.end()) {
			delete *it;
			it = clones.erase(it);
		}
		// remove SDL states
		sdlStates.clear();
		// done
		log.log("Cleared the complete age state\n");
	}
	
	/** This is the SDL file parser */
	void tAgeStateManager::loadSdlStructs(tString filename)
	{
		log.log("  Reading %s\n", filename.c_str());
		tSdlStruct sdlStruct(this);
		tSdlStructVar sdlVar;
		
		// open and decrypt file
		tFBuf sdlFile;
		sdlFile.open(filename.c_str(), "r");
		tWDYSBuf sdlContent;
		sdlContent.put(sdlFile);
		sdlContent.decrypt(/*mustBeWDYS*/false);
		sdlFile.close();
		
		unsigned int state = 0;
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
		tString c;
		tStringTokenizer s = tString(sdlContent);
		while (!s.eof()) {
			c = s.getToken(); // getToken already skips comments for us, so we don't have to care about that
			DBG(9, "Token: %s, state: %d\n", c.c_str(), state);
			switch (state) {
				case 0: // out of a statedesc block, wait for one
					if (c == "STATEDESC")
						state = 1;
					else if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 1: // found a statedesc block, search for name
					// check if it is a valid name
					if (c.isNewline())
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					sdlStruct = tSdlStruct(this, c);
					state = 2;
					break;
				case 2: // search for the curly bracket
					if (c == "{")
						state = 3;
					else if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				// ------------------------------------------------------------------------------------------------------------------
				case 3: // in a statedesc block, search for VERSION
					if (c == "VERSION")
						state = 4;
					else if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 4: // in a statedesc block after VERSION, search version number
				{
					unsigned int version = c.asInt();
					if (!version)
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					sdlStruct.version = version;
					if (findStruct(sdlStruct.name, sdlStruct.version, /*throwOnError*/false))
						throw txUnet(_WHERE("%s: Duplicate definition of %s version %d\n", filename.c_str(), sdlStruct.name.c_str(), sdlStruct.version));
					state = 5;
					break;
				}
				case 5: // in a statedesc block after the version number, search for newline
					if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					state = 6;
					break;
				// ------------------------------------------------------------------------------------------------------------------
				case 6: // in a statedesc block, search for VAR or the curly bracket
					if (c == "}") {
						sdlStruct.count(); // count vars and structs
						DBG(4, "Found SDL struct %s, version %d\n", sdlStruct.name.c_str(), sdlStruct.version);
						structs.push_back(sdlStruct); // the STATEDESC is finished now
						state = 0; // go back to state 0 and wait for EOF or next statedesc
					} else if (c == "VAR")
						state = 7;
					else if (!c.isNewline())
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 7: // search for var type
					sdlVar = tSdlStructVar(c);
					state = 8;
					break;
				case 8: // search for var name
				{
					bool dynSize =  c.endsWith("[]");
					unsigned int size = alcParseKey(&c);
					if (!size && !dynSize)
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Invalid key %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
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
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				// ------------------------------------------------------------------------------------------------------------------
				case 10: // find equal sign after DEFAULT
					if (c == "=")
						state = 11;
					else
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 11: // find default data
				{
					if (c .startsWith("(") && !c.endsWith(")")) { // it's a part of a tuple, take the whole tuple (several tokes) as default value
						tString token;
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
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					break;
				case 13: // find defaultoption data
				{
					if (c == "VAULT")
						sdlVar.flags |= tSdlStructVar::DVault; // doesn't change anything (yet)
					else if (c == "hidden" && (sdlStruct.name == "EderDelin" || sdlStruct.name == "EderTsogal" || sdlStruct.name == "Minkata" || sdlStruct.name == "Negilahn"))  // work-around for "DEFAULTOPTION=hidden" in some sdl files
						sdlVar.flags |= tSdlStructVar::DHidden; // doesn't change anything (yet)
					else
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					state = 9; // find more information or the end of the VAR
					break;
				}
				// ------------------------------------------------------------------------------------------------------------------
				case 14: // find equal sign after DISPLAYOPTION
					if (c == "=")
						state = 15;
					else
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
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
						throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected token %s", filename.c_str(), s.getLineNum(), s.getColumnNum(), c.c_str()));
					state = 9; // find more information or the end of the VAR
					break;
				}
				default:
					throw txParseError(_WHERE("%s: Unknown state %d", filename.c_str(), state));
			}
		}
		
		if (state != 0)
			throw txParseError(_WHERE("Parse error at %s line %d, column %d: Unexpected end of file", filename.c_str(), s.getLineNum(), s.getColumnNum()));
	}
	
	/** The SDL Struct classes */
	//// tSdlStructVar
	tSdlStructVar::tSdlStructVar(tString type)
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
	tSdlStruct::tSdlStruct(tAgeStateManager *stateMgr, tString name) : name(name)
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
	
	tSdlStructVar *tSdlStruct::getElement(unsigned int nr, bool var)
	{
		unsigned int curNr = 0;
		for (tVarList::iterator it = vars.begin(); it != vars.end(); ++it) {
			if ((var && it->type != DStruct) || (!var && it->type == DStruct)) {
				if (nr == curNr) return &(*it);
				++curNr;
			}
		}
		throw txProtocolError(_WHERE("Could not find variable/struct number %d", nr));
	}

} //end namespace alc

