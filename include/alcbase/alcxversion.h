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

/* NOTE:
	This file is only for version numbers, windres is very sensible
	to the syntax.
*/

/* CVS tag - DON'T TOUCH*/
#ifndef __U_XVERSION_H_
#define __U_XVERSION_H_
#define __U_XVERSION_H_ID "$Id$"

//Alcugs version numbers
#define alcMAX_VER 1
#define alcMIN_VER 3
#define alcREL_VER 3
#define alcBET_VER 7
#define alcSTR_VER "1.3.3g"
#define alcREVISION "$Revision$"

#define alcDATE __DATE__
#define alcTIME __TIME__
#define alcBUILD_TIME alcDATE " " alcTIME

#define alcXBUILDINFO "Rev: " alcREVISION " - Built:" alcBUILD_TIME

//Protocol version numbers
#define alcProtoMAX_VER 1
#define alcProtoMIN_VER 3
#define alcProtoVer 1003
//alcProtoMIN_VER+(1000*alcProtoMAX_VER)

/* NOTE:
	* <= 1.1  - Unet2 and older versions of the servers
	* 1.2     - Unet3 protocol messages
	* 1.3     - Unet3+ protocol
*/

#endif
