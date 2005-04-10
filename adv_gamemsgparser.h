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

/*
	This file is subject to dissapear in the future.

	Used to parse UruGame objects, and some very old outdated and non-functional code

*/

#ifndef __ADVGAMEMSG_SPARSER_S_H_
#define __ADVGAMEMSG_SPARSER_S_H_

#define __ADVGAMEMSG_SPARSER_S_H_ID "$Id$"

#include "data_types.h"

int storeUruObjectDesc(Byte * buf,st_UruObjectDesc * o,int size);
int streamUruObjectDesc(Byte * buf,st_UruObjectDesc * o);
void dumpUruObjectDesc(FILE * f,st_UruObjectDesc * o);
int compareUruObjectDesc(FILE * f,st_UruObjectDesc * a,st_UruObjectDesc * b);

#endif
