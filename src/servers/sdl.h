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

#ifndef __U_SDL_H
#define __U_SDL_H
/* CVS tag - DON'T TOUCH*/
#define __U_SDL_H_ID "$Id$"

#include <list>
#include <protocol/gamemsg.h>
#include "sdlbinary.h"

namespace alc {

	////DEFINITIONS
	class tmLoadClone;
	
	typedef std::vector<tSdlStruct> tSdlStructList;
	
	class tAgeStateManager {
	public:
		tAgeStateManager(tUnet *net, const tAgeInfo *age);
		~tAgeStateManager(void);
		void reload(void);
		
		void saveSdlState(tMBuf &data);
		void saveClone(const tmLoadClone &clone);
		int sendClones(tNetSession *u);
		int sendSdlStates(tNetSession *u, tmGameStateRequest::tPageList *pages);
		void writeAgeState(tMBuf *buf);
		void removePlayer(U32 ki);
		
		tSdlStruct *findStruct(tStrBuf name, U32 version);
	private:
		void load(void);
		void unload(void);
		
		typedef std::list<tmLoadClone> tCloneList;
		typedef std::list<tSdlState> tSdlList;
		
		void loadAgeState(tStrBuf &fileName);
		void saveAgeState(tStrBuf &fileName);
		
		void loadSdlStructs(const Byte *filename);
		U32 findLatestStructVersion(tStrBuf name, bool throwOnError = true); //!< returns the highest version number available for this struct
		tSdlList::iterator findAgeSDLHook(void);
		
		tCloneList::iterator findClone(const tUruObject &obj);
		tSdlList::iterator findSdlState(tSdlState *state);
		void removeSDLStates(U32 ki, U32 cloneId = 0);
	
		tCloneList clones;
		tSdlList sdlStates;
		tSdlStructList structs;
		tUnet *net;
		tLog *log;
		bool logDetailed;
	};
	
	/** The SDL Struct classes */
	class tSdlStructVar {
	public:
		tSdlStructVar(tStrBuf type = tStrBuf());
		
		typedef enum { DVault = 0x01, DHidden = 0x02, DRed = 0x04 } tSdlStructVarFlags;
		
		// these are public, I would have to add write functions for them anyway or make many classes "friend"
		Byte type;
		U32 size; // "0" means dynamic size
		tStrBuf name;
		tStrBuf structName;
		U32 structVersion;
		tStrBuf defaultVal;
		Byte flags; // see tSdlStructVarFlags
	};
	
	class tSdlStruct {
	public:
		tSdlStruct(tAgeStateManager *stateMgr, tStrBuf name = tStrBuf());
		void count(void);
		
		tSdlStructVar *getElement(int nr, bool var); //!< returns the nth variable (if var == true) or the nth struct (if var == false)
	
		typedef std::vector<tSdlStructVar> tVarList;
		// these are public, I would have to add write functions for them anyway or make many classes "friend"
		tStrBuf name;
		U32 version;
		tVarList vars;
		U32 nVar, nStruct;
	private:
		tAgeStateManager *stateMgr;
	};

} //End alc namespace

#endif