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

#ifndef __U_LOBBYSERVER_H
#define __U_LOBBYSERVER_H

#include <unetlobbyserverbase.h>

namespace alc {

	class tUnetLobbyServer : public tUnetLobbyServerBase {
	public:
		tUnetLobbyServer(void);
	protected:
		virtual int onMsgRecieved(alc::tUnetMsg *msg, alc::tNetSession *u);
		virtual void onApplyConfig(void);
	private:
		tString website, gameLogPath, gameConfig, gameBin;
		bool loadOnDemand;
	};
	
} //End alc namespace

#endif
