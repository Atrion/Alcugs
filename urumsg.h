/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004  The Alcugs H'uru Server Team                          *
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

// UruNet class implementation will be here (temporany, it will be a list of all, C functions)

/*
	this will contain all related with using the socket
*/

#ifndef __U_URUMSG_H_
#define __U_URUMSG_H_
/* CVS tag - DON'T TOUCH*/
#define __U_URUMSG_H_ID "$Id$"

#include "vaultstrs.h"

/*-----------------------------------------------------------
	Sends a ping, to the host specified in the session struct (client/server)
------------------------------------------------------------*/
int plNetMsgPing(int sock,U16 flags,double mtime,Byte destination,st_uru_client * u);
int plNetMsgCustomMetaPing(int sock,U16 flags,double mtime,Byte destination,int population,st_uru_client * u);
/*---------------------------------------------------------------
	Sends a MsgLeave, to the host specified in the session struct
	 ((seems useless for a server, but useful for a client)) (client)
----------------------------------------------------------------*/
int plNetMsgLeave(int sock,Byte reason,st_uru_client * u);
/*---------------------------------------------------------------
	Sends the AuthenticateHello (client)
----------------------------------------------------------------*/
int plNetAuthenticateHello(int sock,Byte * account,st_uru_client * u);
/*---------------------------------------------------------------
	Sends the Challenge (it automatically calcules a challenge)
	for authresult, see authresponse for a complete list (server)
----------------------------------------------------------------*/
int plNetMsgAuthenticateChallenge(int sock, int authresult, st_uru_client * u);
/*---------------------------------------------------------------
	Sends the Response AuthenticateResponse (client)
----------------------------------------------------------------*/
int plNetMsgAuthenticateResponse(int sock, st_uru_client * u);
/* Asks the auth server to verify an user
*/
int plNetMsgCustomAuthAsk(int sock,Byte * login,Byte * challenge,Byte * hash,Byte release,U32 ip,st_uru_client * u);
/* Asks the auth server to verify an user
*/
int plNetMsgCustomAuthResponse(int sock,Byte * login,Byte result,Byte * passwd,Byte access_level,st_uru_client * u);
/*---------------------------------------------------------------
	Sends the Auth response (server)
----------------------------------------------------------------*/
int plNetMsgAccountAuthenticated(int sock,Byte authresult,st_uru_client * u);
/*---------------------------------------------------------------
	Send the alive message (client)
----------------------------------------------------------------*/
int plNetMsgAlive(int sock,st_uru_client * u);
/*---------------------------------------------------------------
	Sends the vault list request message (client)
----------------------------------------------------------------*/
int plNetMsgRequestMyVaultPlayerList(int sock,st_uru_client * u);
/*---------------------------------------------------------------
	Sends the vault list request message (server->vault)
----------------------------------------------------------------*/
int plNetMsgCustomVaultAskPlayerList(int sock,Byte * guid,st_uru_client * u);
/*---------------------------------------------------------------
	Sends the vault list response message (vault->server)
----------------------------------------------------------------*/
int plNetMsgCustomVaultPlayerList(int sock,st_uru_client * u);
/*---------------------------------------------------------------
	Sends the Players list (server)
----------------------------------------------------------------*/
int plNetMsgVaultPlayerList(int sock,st_uru_client * u);
/*---------------------------------------------------------------
	Create player petition (client)
	//data contains the vault block
----------------------------------------------------------------*/
int plNetMsgCreatePlayer(int sock,Byte * avie, Byte * gender, Byte * friendn, Byte * key, Byte * data,int data_size,st_uru_client * u);
/*---------------------------------------------------------------
	Create player petition (server->vault)
	//data contains the vault block
----------------------------------------------------------------*/
int plNetMsgCustomVaultCreatePlayer(int sock,Byte * login,Byte * guid,Byte access_level,Byte * data,int data_size,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends the KI of the new created player, and status of the creation
--------------------------------------------------------------------*/
int plNetMsgCustomVaultPlayerCreated(int sock,Byte status,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends the KI of the new created player, and status of the creation
--------------------------------------------------------------------*/
int plNetMsgPlayerCreated(int sock,Byte status,st_uru_client * u);
/*-------------------------------------------------------------------
 Player deletion message (client)
--------------------------------------------------------------------*/
int plNetMsgDeletePlayer(int sock,st_uru_client * u);
/*-------------------------------------------------------------------
 Asks vault for player deletion (server->vault)
--------------------------------------------------------------------*/
int plNetMsgCustomVaultDeletePlayer(int sock,Byte * guid,Byte access_level,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends set my active player request (client)
--------------------------------------------------------------------*/
int plNetMsgSetMyActivePlayer(int sock,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends a response to the setmyactive player (server)
--------------------------------------------------------------------*/
int plNetMsgActivePlayerSet(int sock,st_uru_client * u);
/** Sends player status to the tracking server (server->tracking)
	Flag
	0-> delete
	1-> set invisible
	2-> set visible
	3-> set only buddies
	<---->
	Status
	RStopResponding 0x00
	RInroute 0x16
	RArriving 0x17
	RJoining 0x18
	RLeaving 0x19
	RQuitting 0x1A
	<----->
*/
int plNetMsgCustomPlayerStatus(int sock,Byte * uid,Byte * name,Byte * avie,Byte flag,Byte status,st_uru_client * u);
/** Asks the vault for the player ownership (game->vault)
*/
int plNetMsgCustomVaultCheckKi(int sock,Byte * uid,st_uru_client * u);
/** Asks the vault for the player ownership (server->vault)
		//status
		0x01 OK
		elsewhere failed!
*/
int plNetMsgCustomVaultKiChecked(int sock,Byte * uid,Byte status,Byte * avie,st_uru_client * u);
//--- Set the server guid (game-->tracking)
int plNetMsgCustomSetGuid(int sock,Byte * guid,Byte * age_fname,Byte * public_address,Byte * private_mask,st_uru_client * u);
int plNetMsgCustomMetaRegister(int sock,int cmd,int pop,Byte * address,Byte * name,Byte * site,Byte * desc,Byte * passwd,Byte * contact,int dataset,Byte * dset,st_uru_client * u);
//---- Find server (game-->tracking)
int plNetMsgCustomFindServer(int sock,Byte * guid,Byte * age_fname,U32 client_ip,st_uru_client * u);
//--- Server Found (tracking-->game)
int plNetMsgCustomServerFound(int sock,Byte * address,int port,Byte * guid,Byte * age_fname,st_uru_client * u);
//--- Server Found (tracking-->game)
int plNetMsgCustomForkServer(int sock,int port,Byte * guid,Byte * age_fname,st_uru_client * u);
//--- Vault Messages -- (client<-->game<-->vault)
int plNetMsgVault(int sock,t_vault_mos * v,st_uru_client * u,U32 flags);
/*-------------------------------------------------------------------
	Sends a request to the age (client)
--------------------------------------------------------------------*/
int plNetMsgFindAge(int sock,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends a response to the plNetMsgFindAgeReply (server)
--------------------------------------------------------------------*/
int plNetMsgFindAgeReply(int sock,Byte * age_name,Byte * address,int port,Byte * guid,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends a request to Join (client)
--------------------------------------------------------------------*/
int plNetMsgJoinReq(int sock,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends a response to the plNetMsgJoinReq (server)
--------------------------------------------------------------------*/
int plNetMsgJoinAck(int sock,st_uru_client * u); //TODO,t_sdl_binary * agehook);
//--- Vault Task Messages -- (client<-->game<-->vault)
int plNetMsgVaultTask(int sock,t_vault_mos * v,st_uru_client * u,U32 flags);
//--- Vault Update Player status (game-->vault)
int plNetMsgCustomVaultPlayerStatus(int sock,Byte * age,Byte * guid,Byte state,U32 online_time,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends the GroupOwner response
--------------------------------------------------------------------*/
int plNetMsgGroupOwner(int sock,U32 page_id,U16 page_type, Byte page_flag,st_uru_client * u);


/*-------------------------------------------------------------------
	Sends the LoadClone response
--------------------------------------------------------------------*/
int plNetMsgLoadClone(int sock,st_uru_client * u,Byte * in_buf,int size);
/*-------------------------------------------------------------------
	Sends the InitialAgeStateSent response
--------------------------------------------------------------------*/
int plNetMsgInitialAgeStateSent(int sock,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends the plNetMsgMembersList response
--------------------------------------------------------------------*/
int plNetMsgMembersList(int sock,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends the plNetMsgMembersList response
--------------------------------------------------------------------*/
int plNetMsgMemberUpdate(int sock,st_uru_client * u,st_uru_client * copy,Byte flag);
/*-------------------------------------------------------------------
	Sends the NetMsgSDLStateBCast response
--------------------------------------------------------------------*/
int plNetMsgSDLStateBCast(int sock,Byte * in_buf,int size,st_uru_client * u,st_uru_client * copy);
/*-------------------------------------------------------------------
	Sends the plNetMsgGameMessage response
--------------------------------------------------------------------*/
int plNetMsgGameMessage(int sock,Byte * in_buf,int size,st_uru_client * u,st_uru_client * copy);
/*-------------------------------------------------------------------
	Sends the plNetMsgGameMessage response
--------------------------------------------------------------------*/
int plNetMsgGameMessage69(int sock,Byte * in_buf,int size,st_uru_client * u);
/*-------------------------------------------------------------------
	Sends the plNetMsgVoiceChat!!!
--------------------------------------------------------------------*/
int plNetMsgVoice(int sock,Byte * in_buf,int size,st_uru_client * u,U32 snd_ki);

int plNetMsgSDLStateEspecial(int sock,st_uru_client * u);

#endif


