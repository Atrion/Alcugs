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

#ifndef __U_SDL_H
#define __U_SDL_H

#include "sdlbinary.h"

#include <protocol/gamemsg.h>
#include <alcutil/alclog.h>

#include <list>
#include <vector>

namespace alc {

	class tmLoadClone;
	class tUnetGameServer;
	class tAgeInfo;
	
	
	typedef std::vector<tSdlStruct> tSdlStructList;
	
	class tAgeStateManager {
	public:
		tAgeStateManager(tUnetGameServer *net, tAgeInfo *age);
		~tAgeStateManager(void);
		void applyConfig(void);
		
		void saveSdlState(tMBuf &data, const tUruObject &obj);
		void saveSdlVaultMessage(tMBuf &data, tNetSession *u);
		void saveClone(tpLoadCloneMsg *clone);
		int sendClones(tNetSession *u);
		int sendSdlStates(tNetSession *u, tmGameStateRequest::tPageList *pages);
		const tStreamable *getAgeState(void); //!< returns the AgeSDLHook or NULL
		void removePlayer(tNetSession *player);
		void clearAllStates(void);//!< remove all SDL states and clones (only call if you are sure noone is in the age!)
		
		tSdlStruct *findStruct(tString name, uint16_t version, bool throwOnError = true);
	private:
		
		typedef std::vector<tpLoadCloneMsg *> tCloneList;
		typedef std::list<tSdlState> tSdlList;
		
		bool doesAgeLoadState(const tString &resettingAges, const tString &age);
		void loadAgeState();
		void saveAgeState();
		
		void loadSdlStructs(tString filename);
		uint16_t findLatestStructVersion(const tString &name, bool throwOnError = true); //!< returns the highest version number available for this struct
		tSdlList::iterator findAgeSDLHook(void);
		
		tCloneList::iterator findClone(const tUruObject &obj);
		tSdlList::iterator findSdlState(tSdlState *state);
		void removeCloneStates(uint32_t ki, uint32_t cloneId = 0);
	
		tCloneList clones;
		tSdlList sdlStates;
		tSdlStructList structs;
		tUnetGameServer *net;
		tAgeInfo *age;
		tLog log;
		bool logDetailed, initialized;
		tString ageStateFile;
	};
	
	/** The SDL Struct classes */
	class tSdlStructVar {
	public:
		tSdlStructVar(tString type = tString());
		
		typedef enum { DVault = 0x01, DHidden = 0x02, DRed = 0x04 } tSdlStructVarFlags;
		
		// these are public, I would have to add write functions for them anyway or make many classes "friend"
		uint8_t type;
		size_t size; // "0" means dynamic size
		tString name;
		tString structName;
		uint32_t structVersion;
		tString defaultVal;
		uint8_t flags; // see tSdlStructVarFlags
	};
	
	class tSdlStruct {
	public:
		tSdlStruct(tAgeStateManager *stateMgr, tString name = tString());
		void count(void);
		
		tSdlStructVar *getElement(unsigned int nr, bool var); //!< returns the nth variable (if var == true) or the nth struct (if var == false)
	
		typedef std::vector<tSdlStructVar> tVarList;
		// these are public, I would have to add write functions for them anyway or make many classes "friend"
		tString name;
		uint16_t version;
		tVarList vars;
		uint32_t nVar, nStruct;
	private:
		tAgeStateManager *stateMgr;
	};

} //End alc namespace

#endif
