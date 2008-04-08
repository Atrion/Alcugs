/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Project Server Team                   *
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

#ifndef __U_TRACKINGBACKEND_H
#define __U_TRACKINGBACKEND_H
/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGBACKEND_H_ID "$Id$"

namespace alc {

	////DEFINITIONS
	/**
		If we want to do it well and nice, we should add pre and post conditions here.
	*/
	
		
	class tTrackingData {
	public:
		tTrackingData(bool lobby) { this->lobby = lobby; port_start = 5001; port_end = 6000; childs = new tNetSessionList; }
		~tTrackingData(void) { delete childs; }
		bool lobby;
		tNetSessionList *childs;
		U16 port_start, port_end;
	};
	
	class tPlayer {
	public:
		tPlayer(U32 ki, U32 x) { this->ki = ki; this->x = x; }
		U32 ki; // player's ki number
		U32 x; // player's X value
#if 0
		Byte uid[17];
	int x; //client X
	//link with _home_ server peer
	int sid; //the unique session identifier
	U32 ip; //the server session, where this player is
	U16 port; //the server session, where this player is
	//end link
	Byte flag; // 0-> delete, 1-> set invisible, 2-> set visible, 3-> set only buddies
	Byte status; //RStopResponding 0x00, 0x01 ok, RInroute 0x16, RArriving 0x17, RJoining 0x18, RLeaving 0x19, RQuitting 0x1A
	Byte avie[200];
	Byte login[200];
	Byte guid[31]; //Age guid where the player is
	Byte age_name[101]; //Age name where the player is
	Byte waiting; //Waiting var (1 if is waiting to the FindAgeReply, elsewhere nothing...)
	U32 client_ip; //clients ip address
	U16 client_port; //clients port address
#endif
	};
	
	class tPlayerList {
	public:
		tPlayerList(void) { size = 0; players = NULL; }
		~tPlayerList(void);
		void updatePlayer(U32 ki, U32 x);
		tPlayer *getPlayer(U32 ki);
		void findServer(tPlayer *player, const Byte *guid, const Byte *name, tNetSessionMgr *smgr);
	private:
		int size;
		tPlayer **players;
	};

} //End alc namespace

#endif
