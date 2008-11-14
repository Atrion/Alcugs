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

	class tmLoadClone;

	////DEFINITIONS
	typedef std::list<tmLoadClone *> tCloneList;

	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/
	
	class tSdlState : public tBaseType {
	public:
		tSdlState(tUruObject &obj);
		tSdlState(const tSdlState &);
		virtual void store(tBBuf &t);
		virtual void stream(tBBuf &t);
	private:
		tUruObject &obj;
		// format
		bool compress;
		tUStr name;
		U16 version;
		tMBuf data;
	};
	
	class tAgeStateManager {
	public:
		tAgeStateManager(tUnet *net);
		~tAgeStateManager(void);
		void saveSdlState(tUruObject &obj, tMBuf &data);
		void saveClone(tmLoadClone &clone);
		int sendClones(tNetSession *u);
		void writeAgeState(tMBuf *buf);
	private:
		tCloneList::iterator findClone(tUruObject &obj);
	
		tCloneList clones;
		tUnet *net;
	};

} //End alc namespace

#endif
