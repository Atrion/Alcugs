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

/* CVS tag - DON'T TOUCH*/
#define __U_TRACKINGBACKEND_ID "$Id$"

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <unet.h>
#include <protocol/trackingmsg.h>

////extra includes
#include "trackingbackend.h"

#include "alcdebug.h"

namespace alc {

	////IMPLEMENTATION
	void tPlayerList::findServer(tPlayer *player, const Byte *guid, const Byte *name, tNetSessionMgr *smgr)
	{
		
		DBG(5, "searching for age %s for client with KI %d\n", name, player->ki);
		tNetSession *server = NULL, *game = NULL;
		smgr->rewind();
		while ((server = smgr->getNext())) {
			if (server->data && memcmp(server->guid, guid, 8) == 0 && strcmp((char *)server->name, (char *)name) == 0) {
				game = server; // we found it
				break;
			}
		}
		if (!game) {
			DBG(5, "can't find age, spawning a new server\n");
			// search for the lobby with the least load
			tNetSession *lobby = NULL;
			int load = -1;
			smgr->rewind();
			while ((server = smgr->getNext())) {
				if (server->data && (load < 0 || ((tTrackingData*)server->data)->childs->getCount() < load)) {
					lobby = server;
					load = ((tTrackingData*)server->data)->childs->getCount();
				}
			}
			if (!lobby) {
				lerr->log("ERR: There's no lobby I could use to spawn the server\n");
				return;
			}
			// search for free ports
			tTrackingData *data = (tTrackingData*)lobby->data;
			int nPorts = data->port_end - data->port_start + 1;
			bool *freePorts = new bool[nPorts];
			for (int i = 0; i < nPorts; ++i) freePorts[i] = true;
			data->childs->rewind();
			while ((server = data->childs->getNext())) {
				freePorts[server->getPort() - data->port_start] = false;
			}
			int lowest;
			for (lowest = 0; lowest < nPorts; ++lowest) {
				if (freePorts[lowest]) break; // we found a free one
			}
			lowest += data->port_start;
			delete[] freePorts;
			if (lowest == nPorts) { // no free port on the lobby with the least childs
				lerr->log("ERR: No free port on lobby %s, can't spawn game server\n", lobby->str());
				return;
			}
			// ok, telling the lobby to fork
			tmCustomForkServer forkServer(lobby, player->ki, player->x, lowest, guid, name, false); // FIXME: atm it never loads the SDL state
			lobby->send(forkServer);
		}
		else {
			DBG(5, "found age (server: %s), telling the client about it\n", game->str());
			// TODO: send this one to the client
		}
	}
	
	tPlayerList::~tPlayerList(void)
	{
		if (players != NULL) {
			for (int i = 0; i < size; ++i) {
				if (players[i]) delete players[i];
			}
			free((void *)players);
		}
	}
	
	tPlayer *tPlayerList::getPlayer(U32 ki)
	{
		for (int i = 0; i < size; ++i) {
			if (players[i] && players[i]->ki == ki) return players[i];
		}
		return NULL;
	}
	
	void tPlayerList::updatePlayer(U32 ki, U32 x)
	{
		tPlayer *player = getPlayer(ki);
		if (!player) { // it doesn't exist, create it
			++size;
			if (!players) players = (tPlayer **)malloc(size*sizeof(tPlayer));
			else players = (tPlayer **)realloc((void *)players, size*sizeof(tPlayer));
			player = players[size-1] = new tPlayer(ki, x);
		}
		else
			player->x = x;
	}

} //end namespace alc

