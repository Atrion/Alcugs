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

/* NOTE:
	This file is only for version numbers, windres is very sensible
	to the syntax.
*/

#ifndef __U_XVERSION_H_
#define __U_XVERSION_H_

//Alcugs version numbers
#define alcMAX_VER 1
#define alcMIN_VER 3
#define alcREL_VER 6
#define alcBET_VER 0
#define alcSTR_VER "1.3.6"

#define alcBUILD_TIME __DATE__ " " __TIME__

//Protocol version numbers
#define alcPROTO_MAX_VER 1
#define alcPROTO_MIN_VER 3
#define alcPROTO_VER 1003
//alcPROTO_MIN_VER+(1000*alcPROTO_MAX_VER)

/* NOTE:
	* <= 1.1  - Unet2 and older versions of the servers
	* 1.2     - Unet3 protocol messages
	* 1.3     - Unet3+ protocol
*/

#endif
