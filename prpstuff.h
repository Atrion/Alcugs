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

#define _URUPRPSTUFF_ID_ "$Id$"

#ifdef STR_SIZE
#error WARNING STR_SIZE already defined... STOP
#endif

#define STR_SIZE 255

//Thanks to everybody at wiki.cobbs.ca

typedef struct {
	st_UruObjectDesc desc; // The object descriptor.
	U32 offset; // File offset to the start of the object
	U32 size; // File size of the object (with header)
	Byte * buf; //Buffer with the object (with header)
} st_PrpIObject;

typedef struct {
	U16 type; // The type of the object. See the PrpObjectReference
	S32 count; // The number of objects of this type
	st_PrpIObject * objects; //an array of objects
} st_PrpIndex;

typedef struct {
	U32 Version; // Always 5
	U32 PageId; // The ID of the page. It is signed. It is assigned logically, explanation to follow later.
	U16 PageType; // 0 = Page Data, 4 = Global, 8 = Builtin/Texture, 16=??
	Byte AgeName[STR_SIZE]; // The name of the age (as seen in the filename)
	Byte District[STR_SIZE]; // The (uru) string "District"
	Byte PageName[STR_SIZE]; // The name of the page (as seen in the filename)
	U16 MajorVersion; // Always 63
	U16 MinorVersion; // 11 in Prime, Live, and To D'ni; 12 in PotS
	U32 Unknown3; // Always 0
	U32 Unknown4; // Always 8
	U32 FileDataSize; // The number of bytes following this header
	U32 FirstOffset; // Offset to the first object
	U32 IndexOffset; // Offset to the object index
	//from here to the end, is the FileDataSize (at offset IndexOffset)
	S32 IndexCount; // The number of index items
	st_PrpIndex * Index; //the Prp Index
} st_PrpHeader;

