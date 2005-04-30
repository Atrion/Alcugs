/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs H'uru Server Team                     *
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

#ifndef __U_URUGBASICMSG_H_
#define __U_URUGBASICMSG_H_
/* CVS tag - DON'T TOUCH*/
#define __U_URUGBASICMSG_H_ID "$Id$"

int plNetMsgPing(st_unet * net,U16 flags,double mtime,Byte destination,int sid,int ssid);
int plNetMsgLeave(st_unet * net,Byte reason,int sid);
int plNetMsgTerminated(st_unet * net,Byte code,int sid);
int plNetMsgPlayerTerminated(st_unet * net,Byte code,int sid);
int plNetMsgAlive(st_unet * net,int sid);

#endif


