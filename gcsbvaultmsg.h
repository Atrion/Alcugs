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

/**
	Basic message generator
*/

#ifndef __U_URUGCSBVAULTMSG_H_
#define __U_URUGCSBVAULTMSG_H_
/* CVS tag - DON'T TOUCH*/
#define __U_URUGCSBVAULTMSG_H_ID "$Id$"

int plNetMsgCustomVaultAskPlayerList(st_unet * net,int sid,int ssid,Byte proto);
int plNetMsgCustomVaultCreatePlayer(st_unet * net,Byte * data,int data_size,int sid,int ssid,Byte proto);
int plNetMsgCustomVaultDeletePlayer(st_unet * net,int sid,int ssid,Byte proto);
int plNetMsgCustomVaultCheckKi(st_unet * net,int sid,int ssid,Byte proto);
int plNetMsgCustomVaultPlayerStatus(st_unet * net,char * age,char * guid,Byte state,U32 online_time,int sid,Byte proto);

#endif


