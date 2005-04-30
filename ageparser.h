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

#ifndef __U_AGE_PARSER_H_
#define __U_AGE_PARSER_H_
#define __U_AGE_PARSER_H_ID "$Id$"

//parse out age structures

typedef struct {
	Byte name[255]; //page name
	U16 id; //used to calculate the correct page id
	Byte sdl; //0 or 1 for conditional load (unused)
} t_page_def;

typedef struct {
	Byte name[255]; //the age name
	U32 StartDateTime; //unused
	float DayLength; //4 bytes //unused
	U16 MaxCapacity; //unused
	U16 LingerTime; //unused
	U16 SequencePrefix; //used to calculate the correct page id
	U16 ReleaseVersion; //unused
	S32 n_pages; //number of pages
	t_page_def * page; //struct with pages
} t_age_def;


void init_age_def(t_age_def * age);
void destroy_age_def(t_age_def * age);
int age_add_page(char * right_buffer,t_page_def ** page,S32 * n_pages);
int parse_age_descriptor(st_log * dsc,char * buf,int size,t_age_def * age);
int read_age_descriptor(st_log * f,char * address,t_age_def ** p_age);
void dump_age_descriptor(st_log * f,t_age_def age);
int read_all_age_descriptors(st_log * f,char * address,t_age_def ** p_age);

#endif
