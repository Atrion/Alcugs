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

//! A tuple of configuration values (only bidimensional tuples are allowed)
typedef struct {
	char name[200];
	char ** value;
	int x; //< Number of columns
	int y; //< Number of rows
} st_config_vals;

//! Configuration struct
typedef struct {
	char key[200];
	int n;
	st_config_vals * config;
} st_config_keys;

//! A group of configuration keys
typedef struct {
	int n;
	st_config_keys * keys;
} st_config;

#define CNF_FATALERR -4 //terrible error, execution should stop
#define CNF_OPENFAIL -3 //if it fails to open the file
#define CNF_PARSEERR -2 //if a parser error happens
#define CNF_WARNING  -1 //if a warning ocurred
#define CNF_SUCCESS   0 //if success

int read_config(FILE * dsc, char * conf,st_config ** cfg);
U32 cnf_getU32(U32 defecto,char * what,char * where,st_config * cfg);
U16 cnf_getU16(U16 defecto,char * what,char * where,st_config * cfg);
Byte cnf_getByte(Byte defecto,char * what,char * where,st_config * cfg);
char * cnf_getString(const char * defecto,const char * what,const char * where,st_config * cfg);
char cnf_exists(char * what,char * where,st_config * cfg);

void cnf_setU32(U32 val,char * what,char * where,st_config ** cfg2);
void cnf_setU16(U16 val,char * what,char * where,st_config ** cfg2);
void cnf_setByte(Byte val,char * what,char * where,st_config ** cfg2);
int cnf_add_key(char * value,char * what,char * where,st_config ** cfg2);
int cnf_add_key_xy(char * gvalue,char * what,char * where,int x,int y,st_config ** cfg2);

void cnf_copy(const char * to, const char * from,st_config ** cfg2);
void cnf_copy_key(const char * to_name,const char * from_name,const char * to, const char * from,st_config ** cfg2);

void cnf_destroy(st_config ** cfg2);

#endif
