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

/**********************************************
  For parsing the configuration URU files
*************************************************/

#ifndef __U_CONFIG_PARSER_H_
#define __U_CONFIG_PARSER_H_
#define __U_CONFIG_PARSER_H_ID "$Id$"

#include "data_types.h" //for Byte, and others

#include <stdio.h>

typedef struct {
	char name[200];
	char * value;
} st_config_vals;

//!configuration struct
typedef struct {
	char key[200];
	int n;
	st_config_vals * config;
} st_config_keys;

typedef struct {
	int n;
	st_config_keys * keys;
} st_config;


int read_config(FILE * dsc, char * conf,st_config ** cfg, char ** allowed, int nall);
U32 cnf_getU32(U32 defecto,char * what,char * where,st_config * cfg);
U16 cnf_getU16(U16 defecto,char * what,char * where,st_config * cfg);
Byte cnf_getByte(Byte defecto,char * what,char * where,st_config * cfg);
Byte * cnf_getString(Byte * defecto,char * what,char * where,st_config * cfg);

#endif
