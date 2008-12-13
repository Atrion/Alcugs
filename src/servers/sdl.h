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
#include <memory>

namespace alc {

	////DEFINITIONS
	class tmLoadClone;
	class tSdlStruct;
	typedef std::vector<tSdlStruct> tSdlStructList;
	
	class tSdlStateVar : public tBaseType {
	public:
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
	private:
		Byte flags;
	};
	
	class tSdlStateBinary : public tBaseType {
	public:
		tSdlStateBinary(tSdlStruct *sdlStruct = NULL);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		void reset(tSdlStruct *sdlStruct); //!< reset the state, empty all lists etc.
	private:
		typedef std::vector<tSdlStateVar> tVarList;
		
		tSdlStruct *sdlStruct;
		tVarList vars;
	};

	class tSdlState : public tBaseType {
	public:
		tSdlState(const tUruObject &obj, tSdlStructList *structs);
		tSdlState(void);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
		const Byte *str(void);
		bool operator==(const tSdlState &state);
		
		tUruObject obj;
		// format
		tUStr name;
		U16 version;
		tSdlStateBinary content;
	private:
		static tMBuf decompress(tBBuf &t);
		static tMBuf compress(tMBuf &data);
		tSdlStruct *findStruct(void);
	
		tStrBuf dbg;
		
		tSdlStructList *structs;
	};
	
	class tAgeStateManager {
	public:
		tAgeStateManager(tUnet *net);
		~tAgeStateManager(void);
		void reload(void);
		
		void saveSdlState(const tUruObject &obj, tMBuf &data);
		void saveClone(const tmLoadClone &clone);
		int sendClones(tNetSession *u);
		int sendSdlStates(tNetSession *u);
		void writeAgeState(tMBuf *buf);
		void removePlayer(U32 ki);
	private:
		void load(void);
		void unload(void);
		
		typedef std::list<tmLoadClone *> tCloneList;
		typedef std::list<tSdlState *> tSdlList;
		
		void loadSdlStructs(const Byte *filename);
		
		tCloneList::iterator findClone(const tUruObject &obj);
		tSdlList::iterator findSdlState(tSdlState *state);
		void removeSDLStates(U32 ki, U32 cloneId = 0);
	
		tCloneList clones;
		tSdlList sdlStates;
		tSdlStructList structs;
		tUnet *net;
		tLog *log;
	};
	
	/** The SDL Struct classes */
	class tSdlStructVar {
	public:
		tSdlStructVar(tStrBuf type = tStrBuf());
		
		typedef enum { DVault = 0x01, DHidden = 0x02, DRed = 0x04 } tSdlStructVarFlags;
		
		// these are public, I would have to add write functions for them anyway or make many classes "friend"
		Byte type;
		U32 size; // "-1" means variable size
		tStrBuf name;
		tStrBuf structName;
		tStrBuf defaultVal;
		Byte flags; // see tSdlStructVarFlags
	private:
		Byte getVarTypeFromName(tStrBuf type);
	};
	
	class tSdlStruct {
	public:
		tSdlStruct(tStrBuf name = tStrBuf());
		void count(void);
	
		typedef std::vector<tSdlStructVar> tVarList;
		// these are public, I would have to add write functions for them anyway or make many classes "friend"
		tStrBuf name;
		U32 version;
		tVarList vars;
		int nVar, nStruct;
	};

} //End alc namespace

#endif
