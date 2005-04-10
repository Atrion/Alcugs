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

#define _URUPRP_ID_ "$Id$"
#define _MAYOR_ 1
#define _MINOR_ 2

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "files.h"
#include "license.h"
#include "data_types.h"
#include "conv_funs.h"
#include "useful.h"
#include "adv_gamemsgparser.h"

#include "prpstuff.h"

//#define _DBG_PRP_

#define NPatches 39
#define NFiles 18

const Byte refpatch[NPatches][2]={ \
{0x14,0x33},{0x08,0x34},{0x0C,0x35},{0x1E,0x36},{0x39,0x37},{0x3E,0x38},{0x11,0x39},\
{0x10,0x3A},{0x21,0x3B},{0x16,0x3C},{0x13,0x3D},{0x0E,0x3E},{0x28,0x3F},{0x06,0x40},\
{0x2B,0x41},{0x1B,0x42},{0x1A,0x43},{0x05,0x44},{0x35,0x45},{0x24,0x46},{0x25,0x47},\
{0x04,0x48},{0x02,0x49},{0x18,0x4A},{0x19,0x4B},{0x1F,0x4C},{0x38,0x4D},{0x27,0x4E},\
{0x29,0x4F},{0x20,0x50},{0x07,0x51},{0x17,0x52},{0x0A,0x53},{0x15,0x54},{0x0D,0x55},\
{0x12,0x56},{0x32,0x57},{0x37,0x60},{0x3B,0x61}};
//the above table if for GUI_District only. 0x02, also note that:
//  0x2C is KiHelpMenu
//  0x58 is bkEditableJournal


const char * patchfiles[NFiles]={ \
"GUI_District_KIAgeOwnerSettings.prp",\
"GUI_District_KIMarkerTimeMenu.prp",\
"GUI_District_KINewItemAlert.prp",\
"GUI_District_KIBlackBar.prp",\
"GUI_District_KIMarkerTypeMenu.prp",\
"GUI_District_KIPictureExpanded.prp",\
"GUI_District_KIJournalExpanded.prp",\
"GUI_District_KIMicroBlackBar.prp",\
"GUI_District_KIPlayerExpanded.prp",\
"GUI_District_KIListMode.prp",\
"GUI_District_KIMicro.prp",\
"GUI_District_KIQuestionNote.prp",\
"GUI_District_KIMain.prp",\
"GUI_District_KIMini.prp",\
"GUI_District_KISettings.prp",\
"GUI_District_KIMarkerFolder.prp",\
"GUI_District_KINanoBlackBar.prp",\
"GUI_District_KIYesNo.prp"\
};


void init_PrpIObject(st_PrpIObject * obj) {
	bzero(obj,sizeof(st_PrpIObject));
	obj->buf=NULL;
}

void destroy_PrpIObject(st_PrpIObject * obj) {
	if(obj->buf!=NULL) {
		free((void *)obj->buf);
		obj->buf=NULL;
	}
}

void init_PrpIndex(st_PrpIndex * index) {
	bzero(index,sizeof(st_PrpIndex));
	index->objects=NULL;
}

void destroy_PrpIndex(st_PrpIndex * index) {
	int i;
	if(index->objects!=NULL) {
		for(i=0; i<index->count; i++) {
			destroy_PrpIObject(&index->objects[i]);
		}
		free((void *)index->objects);
		index->objects=NULL;
	}
}


void init_PrpHeader(st_PrpHeader * header) {
	bzero(header,sizeof(st_PrpHeader));
	header->Index=NULL;
}

void destroy_PrpHeader(st_PrpHeader * header) {
	int i;
	if(header->Index!=NULL) {
		for(i=0; i<header->IndexCount; i++) {
			destroy_PrpIndex(&header->Index[i]);
		}
		free((void *)header->Index);
		header->Index=NULL;
	}
}


//end stuff that will be moved


void usage() {
	printf("\nUsage:\nUpdate Only Version numbers: uruprp fix filename.prp \n\
List contents only: uruprp ls filename.prp\n\
Apply all patches: uruprp patch filename.prp\n\
Fix banned references: uruprp bans filename.prp\n\
Compare 2 prp files: uruprp diff one.prp two.prp [rled]\n\
Really apply all patches: uruprp patchall uu_dat tpots_dat out_dat\n\
Show version: uruprp -v\n\
Show help: uruprp -h\n\
Diff params:\n\
l: left only\n\
r: right only\n\
e: equal only\n\
d: different only\n\
You can use different combinations\n\n\
TODO:\n\
Delete a object: uruprp rm filename.prp game_object\n\
Copy a object: uruprp cp source.prp destination.prp game_object\n\
\n");
	fflush(0);
}

void version() {
	printf("UruPRP %i.%i build: %s\r\n",_MAYOR_,_MINOR_,_URUPRP_ID_);
	fflush(0);
}

//DEBUG flags are
/*
0x00 == Nothing
0x01 - Basic
0x02 - Errors
0x04 - Header
0x08 - Index Dump
0x10 - Errors to stderr / else, go to stdout
*/


int readprp(st_PrpHeader * prp,char * input,Byte flags) {

	FILE * ferr;

	Byte * buf; //the buffer
	int size;
	int off=0;

	int ret=0;

	int i,e;

	buf=NULL; //avoid a SEGFAULT

	if(flags & 0x10) { ferr=stderr; } else { ferr=stdout; }

	if(flags & 0x01) {
		printf("Reading file... please wait...\n");
		fflush(0);
	}
	size=readfile((char **)&buf, input);

	init_PrpHeader(prp);

	prp->Version=*(U32 *)(buf+off);
	off+=4;

	if(prp->Version!=5) {
		if(flags & 0x02) {
			fprintf(ferr,"Error: Prp Version Mismatch! - Unknown format\n");
		}
		return -1;
	}

	prp->PageId=*(U32 *)(buf+off);
	off+=4;
	prp->PageType=*(U16 *)(buf+off);
	off+=2;
	off+=decode_urustring(prp->AgeName,buf+off,STR_SIZE);
	off+=2;
	off+=decode_urustring(prp->District,buf+off,STR_SIZE);
	off+=2;

	if(strcmp((char *)prp->District,"District")) {
		if(flags & 0x02) {
			fprintf(ferr,"Error: Not a District!\n");
		}
		return -1;
	}

	off+=decode_urustring(prp->PageName,buf+off,STR_SIZE);
	off+=2;
	prp->MajorVersion=*(U16 *)(buf+off);
	off+=2;
	prp->MinorVersion=*(U16 *)(buf+off);
	off+=2;

	//well, let's go
	prp->Unknown3=*(U32 *)(buf+off); //0x00
	off+=4;
	prp->Unknown4=*(U32 *)(buf+off); //0x08
	off+=4;
	prp->FileDataSize=*(U32 *)(buf+off);
	off+=4;
	prp->FirstOffset=*(U32 *)(buf+off);
	off+=4;
	prp->IndexOffset=*(U32 *)(buf+off);
	off+=4;

	if(flags & 0x04) {
		printf("%s:\n",input);
		printf("Version:%i, PageId=%08X (%i), PageType=%04X\n",prp->Version,\
		prp->PageId,prp->PageId,prp->PageType);
		printf("%s %s %s - %i.%i\n",prp->AgeName,prp->District,prp->PageName,\
		prp->MajorVersion,prp->MinorVersion);
		printf("Size %i, off1: %08X, off2: %08X\n",prp->FileDataSize,prp->FirstOffset,\
		prp->IndexOffset);
		fflush(0);
	}

	if(prp->Unknown3!=0x00 || prp->Unknown4!=0x08) {
		if(flags & 0x02) {
			fprintf(ferr,"Unexpected Unknown3 or Unknown4 fields!\n");
		}
		return -1;
	}

	//set the offset to the index
	off=prp->IndexOffset;

	prp->IndexCount=*(U32 *)(buf+off);
	off+=4;

	if(flags & 0x04) {
		printf("Found %i objects\n",prp->IndexCount);
	}
	if(prp->IndexCount==0) {
		printf("This package is void!\n");
		return -1;
	}

	if(flags & 0x08) {
		printf("Index DUMP:\n------------------\n");
	}

	prp->Index=(st_PrpIndex *)malloc(sizeof(st_PrpIndex) * prp->IndexCount);

	for(i=0; i<prp->IndexCount; i++) {
		init_PrpIndex(&prp->Index[i]);

		prp->Index[i].type=*(U16 *)(buf+off);
		off+=2;
		prp->Index[i].count=*(U32 *)(buf+off);
		off+=4;

		if(flags & 0x08) {
			printf("%04X - %i\n",prp->Index[i].type,prp->Index[i].count);
		}

		if(prp->Index[i].count==0) {
			if(flags & 0x02) {
				fprintf(ferr,"0 objects?, not possible!!\n");
			}
			return -1;
		}

		prp->Index[i].objects=(st_PrpIObject *)malloc(sizeof(st_PrpIObject) *\
		 prp->Index[i].count);

		for(e=0; e<prp->Index[i].count; e++) {
			init_PrpIObject(&prp->Index[i].objects[e]);

			ret=storeUruObjectDesc(buf+off,&prp->Index[i].objects[e].desc,0);
			if(flags & 0x08) {
				dumpUruObjectDesc(stdout,&prp->Index[i].objects[e].desc);
				printf("\n");
			}

			if(ret<=0) {
				if(flags & 0x02) {
					fprintf(ferr,"Fatal error parsing an UruObject descriptor\n");
				}
				return -1;
			}

			off+=ret;

			prp->Index[i].objects[e].offset=*(U32 *)(buf+off);
			off+=4;
			prp->Index[i].objects[e].size=*(U32 *)(buf+off);
			off+=4;

			prp->Index[i].objects[e].buf=(Byte *)malloc(sizeof(Byte) *\
			 prp->Index[i].objects[e].size);

			st_UruObjectDesc aux;

			ret=storeUruObjectDesc(buf+prp->Index[i].objects[e].offset+2,&aux,0);
			if(ret<=0) {
				if(flags & 0x02) {
					fprintf(ferr,"Fatal error parsing an UruObject descriptor, from an object header\n");
				}
				return -1;
			}


			ret=compareUruObjectDesc(stdout,&aux,&prp->Index[i].objects[e].desc);
			if(ret<0) {
				if(flags & 0x02) {
					dumpUruObjectDesc(ferr,&aux);
					fprintf(ferr,"\nFatal Header/Index mismatch!!\n");
				}
				return -1;
			}

			memcpy(prp->Index[i].objects[e].buf,buf+prp->Index[i].objects[e].offset,\
			prp->Index[i].objects[e].size);

		}

	}

	if(buf!=NULL) {
		free((void *)buf);
	}

	return 0;
}

int saveprp(st_PrpHeader * prp,char * input,Byte flags) {

	int maxosize;
	int i,e;
	FILE * ferr;

	Byte * buf;
	buf=NULL;

	int off,size;

	maxosize=300*4; //header size

	if(flags & 0x10) { ferr=stderr; } else { ferr=stdout; }

	//now do all the hard stuff
	for(i=0; i<prp->IndexCount; i++) {
		maxosize+=500;
		for(e=0; e<prp->Index[i].count; e++) {
			maxosize+=prp->Index[i].objects[e].size;
			maxosize+=500;
		}
	}

	//store in memory
	buf=(Byte *)malloc(sizeof(Byte) * maxosize);
	if(buf==NULL) {
		if(flags & 0x02) {
			fprintf(ferr,"Fatal, Not enough memory to operate\n");
		}
		return -1;
	}

	off=0;

	if(flags & 0x08) {
		printf("\nStoring header... Buffer size is %i bytes\n",maxosize);
	}

	//header
	*(U32 *)(buf+off)=prp->Version;
	off+=4;
	*(U32 *)(buf+off)=prp->PageId;
	off+=4;
	*(U16 *)(buf+off)=prp->PageType;
	off+=2;
	off+=encode_urustring(buf+off,prp->AgeName,strlen((char *)prp->AgeName),1);
	off+=encode_urustring(buf+off,prp->District,strlen((char *)prp->District),1);
	off+=encode_urustring(buf+off,prp->PageName,strlen((char *)prp->PageName),1);
	*(U16 *)(buf+off)=prp->MajorVersion;
	off+=2;
	*(U16 *)(buf+off)=prp->MinorVersion;
	off+=2;

	//well, let's go
	*(U32 *)(buf+off)=prp->Unknown3; //0x00
	off+=4;
	*(U32 *)(buf+off)=prp->Unknown4; //0x08
	off+=4;

	int header_start;
	header_start=off;

	//prp->FileDataSize=*(U32 *)(buf+off);
	*(U32 *)(buf+off)=0;
	off+=4;
	prp->FirstOffset=off+8;
	*(U32 *)(buf+off)=off+8;
	off+=4;
	//prp->IndexOffset=*(U32 *)(buf+off);
	*(U32 *)(buf+off)=0;
	off+=4;

	//Now dump all the objects, and update the offsets of the index
	size=off;

	for(i=0; i<prp->IndexCount; i++) {
		for(e=0; e<prp->Index[i].count; e++) {

			if(flags & 0x08) {
				printf("Storing object %i,%i at offset %08X(%i)...\n",i,e,off,off);
			}

			if(prp->Index[i].objects[e].buf==NULL) {
				if(flags & 0x02) {
					fprintf(ferr,"Fatal error, obj buf is NULL!!\n");
				}
				return -1;
			}

			//update the offset, where the object is placed
			prp->Index[i].objects[e].offset=off;

			//copy the object to the correct place
			memcpy(buf+off,prp->Index[i].objects[e].buf,prp->Index[i].objects[e].size);

			off+=prp->Index[i].objects[e].size;

		}
	}

	prp->IndexOffset=off; //set the offset where the index starts

	if(flags & 0x08) {
		printf("Generating the index at %08X(%i)...\n",off,off);
	}

	//now generate the index
	*(U32 *)(buf+off)=prp->IndexCount;
	off+=4;

	for(i=0; i<prp->IndexCount; i++) {

		*(U16 *)(buf+off)=prp->Index[i].type;
		off+=2;
		*(U32 *)(buf+off)=prp->Index[i].count;
		off+=4;

		for(e=0; e<prp->Index[i].count; e++) {

			if(flags & 0x08) {
				printf("Storing entry %i,%i at %08X(%i)...\n",i,e,off,off);
			}

			off+=streamUruObjectDesc(buf+off,&prp->Index[i].objects[e].desc);

			*(U32 *)(buf+off)=prp->Index[i].objects[e].offset;
			off+=4;
			*(U32 *)(buf+off)=prp->Index[i].objects[e].size;
			off+=4;

		}

	}

	prp->FileDataSize=off-size; //set contents size
	size=off; //set file size

	*(U32 *)(buf+header_start)=prp->FileDataSize;
	*(U32 *)(buf+header_start+8)=prp->IndexOffset;

	if(flags & 0x01) {
		printf("Saving... please wait...\n");
	}
	savefile((char *)buf,size,input); //imput

	return 0;
}

void listprp(st_PrpHeader * prp) {

	printf("Version:%i, PageId=%08X (%i), PageType=%04X\n",prp->Version,\
	prp->PageId,prp->PageId,prp->PageType);
	printf("%s %s %s - %i.%i\n",prp->AgeName,prp->District,prp->PageName,\
	prp->MajorVersion,prp->MinorVersion);
	printf("Size %i, off1: %08X, off2: %08X\n",prp->FileDataSize,prp->FirstOffset,\
	prp->IndexOffset);
	fflush(0);

	printf("Found %i objects\n",prp->IndexCount);

	printf("Index DUMP:\n------------------\n");

	int i,e;

	for(i=0; i<prp->IndexCount; i++) {
		printf("%04X - %i\n",prp->Index[i].type,prp->Index[i].count);

		for(e=0; e<prp->Index[i].count; e++) {
				dumpUruObjectDesc(stdout,&prp->Index[i].objects[e].desc);
				printf("\n");
		}
	}
}


void printU32diff(U32 a,U32 b,char * what) {
	if(a!=b) { printf("[diff] %s: %08X (%i) - %08X (%i)\n",what,a,a,b,b); }
}

void printUSTRdiff(Byte * a,Byte * b,char * what) {
	if(strcmp((char *)a,(char *)b)) { printf("[diff] %s: %s - %s\n",what,a,b); }
}


/*
	0x01 - prp1 only (left only)
	0x02 - prp2 only (right only)
	0x04 - differences (differs)
	0x08 - matches (equal)
	0x10 - set stderr
	0x20 - set debug

*/
int compareprps(st_PrpHeader * prp1,st_PrpHeader * prp2,Byte flags) {

	int i,e,j,k;
	int found;

	FILE * ferr;

	if(flags & 0x10) { ferr=stderr; } else { ferr=stdout; }

	printU32diff(prp1->Version,prp2->Version,"Version");
	printU32diff(prp1->PageId,prp2->PageId,"PageId");
	printU32diff(prp1->PageType,prp2->PageType,"PageType");
	printUSTRdiff(prp1->AgeName,prp2->AgeName,"AgeName");
	printUSTRdiff(prp1->District,prp2->District,"District");
	printUSTRdiff(prp1->PageName,prp2->PageName,"PageName");
	printU32diff(prp1->MajorVersion,prp2->MajorVersion,"MajorVersion");
	printU32diff(prp1->MinorVersion,prp2->MinorVersion,"MinorVersion");
	printU32diff(prp1->Unknown3,prp2->Unknown3,"Unknown3");
	printU32diff(prp1->Unknown4,prp2->Unknown4,"Unknown4");
	printU32diff(prp1->FileDataSize,prp2->FileDataSize,"FileDataSize");
	printU32diff(prp1->FirstOffset,prp2->FirstOffset,"FirstOffset");
	printU32diff(prp1->IndexOffset,prp2->IndexOffset,"IndexOffset");
	printU32diff(prp1->IndexCount,prp2->IndexCount,"IndexCount");

	for(i=0; i<prp1->IndexCount; i++) {

		found=-1;

		for(j=0; j<prp2->IndexCount; j++) {
			if(prp1->Index[i].type==prp2->Index[j].type) {
				found=j;
				break;
			}
		}

		if(found==-1) {
			if(flags & 0x20) {
				printf("Prp2 don't has Index entry for type %02X\n",prp1->Index[i].type);
			}

			for(e=0; e<prp1->Index[i].count; e++) {
				if(flags & 0x01) {
					printf("[Only on Prp1]: ");
					dumpUruObjectDesc(stdout,&prp1->Index[i].objects[e].desc);
					printf("\n");
				}
			}

		} else {
			j=found;

			for(e=0; e<prp1->Index[i].count; e++) {

				found=-1;

				for(k=0; k<prp2->Index[j].count; k++) {
					if(!strcmp((char *)prp1->Index[i].objects[e].desc.name,(char *)prp2->Index[j].objects[k].desc.name)) {
						found=k;
						break;
					}
				}

				if(found==-1) {
					if(flags & 0x01) {
						printf("[Only on Prp1]: ");
						dumpUruObjectDesc(stdout,&prp1->Index[i].objects[e].desc);
						printf("\n");
					}
				} else {
					if(compareUruObjectDesc(stdout,&prp1->Index[i].objects[e].desc,\
					&prp2->Index[j].objects[k].desc)!=0) {
						if(flags & 0x04) {
							printf("[diff] ");
							printf("Prp1: ");
							dumpUruObjectDesc(stdout,&prp1->Index[i].objects[e].desc);
							printf("\nPrp2: ");
							dumpUruObjectDesc(stdout,&prp2->Index[j].objects[k].desc);
							printf("\n");
						}
					}
					if(flags & 0x20) {
						printU32diff(prp1->Index[i].objects[e].offset,\
						prp2->Index[j].objects[k].offset,"Offset");
						printU32diff(prp1->Index[i].objects[e].size,\
						prp2->Index[j].objects[k].size,"Size");
					}
					if(prp1->Index[i].objects[e].size==prp2->Index[j].objects[k].size) {
						if(memcmp(prp1->Index[i].objects[e].buf,\
						prp2->Index[j].objects[k].buf,prp1->Index[i].objects[e].size)) {
							if(flags & 0x04) {
								printf("[diff] ");
								dumpUruObjectDesc(stdout,&prp1->Index[i].objects[e].desc);
								printf(" differs\n");
							}
						} else {
							if(flags & 0x08) {
								printf("[equal] ");
								dumpUruObjectDesc(stdout,&prp1->Index[i].objects[e].desc);
								printf(" equal\n");
							}
						}
					} else {
							if(flags & 0x04) {
								printf("[diff] ");
								dumpUruObjectDesc(stdout,&prp1->Index[i].objects[e].desc);
								printf(" differs on size\n");
							}
					}

				}

			}

			for(k=0; k<prp2->Index[j].count; k++) {

				found=-1;

				for(e=0; e<prp1->Index[i].count; e++) {
					if(!strcmp((char *)prp1->Index[i].objects[e].desc.name,(char *)prp2->Index[j].objects[k].desc.name)) {
						found=k;
						break;
					}
				}

				if(found==-1) {
					if(flags & 0x02) {
						printf("[Only on Prp2]: ");
						dumpUruObjectDesc(stdout,&prp2->Index[j].objects[k].desc);
						printf("\n");
					}
				}

			}

		}

	}

	for(j=0; j<prp2->IndexCount; j++) {

		found=-1;

		for(i=0; i<prp1->IndexCount; i++) {
			if(prp1->Index[i].type==prp2->Index[j].type) {
				found=j;
				break;
			}
		}

		if(found==-1) {
			if(flags & 0x20) {
				printf("Prp1 don't has Index entry for type %02X\n",prp2->Index[j].type);
			}

			for(k=0; k<prp2->Index[j].count; k++) {
				if(flags & 0x02) {
					printf("[Only on Prp2]: ");
					dumpUruObjectDesc(stdout,&prp2->Index[j].objects[k].desc);
					printf("\n");
				}
			}

		}
	}

	return 0;

}

//All PRP patches go here (ve sure that is a 11 version, and not a 12 version)

int prp_patch_GUIDynDisplayCtrl(st_PrpHeader * prp) {
	int i,e;
	Byte * abuf;
	abuf=NULL;
	int asize=0;

	int found=-1;

	int save=0;
	int off=0;

	printf("Attempting to upgrade the 0xBD object type...\n");

	//search the affected objects
	for(i=0; i<prp->IndexCount; i++) {
		if(prp->Index[i].type==0xBD) {
			found=i;
			break;
		}
	}

	if(found!=-1) {

		i=found;

		for(e=0; e<prp->Index[i].count; e++) {
			printf("Fixing offending object %i,%i\n",i,e);
			asize=prp->Index[i].objects[e].size+500;
			abuf=(Byte *)malloc(sizeof(Byte) * asize);
			if(abuf==NULL) {
				printf("Fatal: Not enough memory to continue!!!\n");
				return -1;
			}

			memcpy(abuf,prp->Index[i].objects[e].buf,prp->Index[i].objects[e].size);
			off=prp->Index[i].objects[e].size;

			Byte new_name[400];

			//check object name
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine02")) {
				strcpy((char *)new_name,"PlasmaDynMapLine03");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine03")) {
				strcpy((char *)new_name,"PlasmaDynMapLine03");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine04")) {
				strcpy((char *)new_name,"PlasmaDynMapLine04");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine05")) {
				strcpy((char *)new_name,"PlasmaDynMapLine05");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine06")) {
				strcpy((char *)new_name,"PlasmaDynMapLine06");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine07")) {
				strcpy((char *)new_name,"PlasmaDynMapLine07");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine08")) {
				strcpy((char *)new_name,"PlasmaDynMapLine08");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine09")) {
				strcpy((char *)new_name,"PlasmaDynMapLine09");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayLMIconLine10")) {
				strcpy((char *)new_name,"PlasmaDynMapLine10");
			} else
			if(!strcmp((char *)prp->Index[i].objects[e].desc.name,\
			"BKDynDisplayPICImage")) {
				strcpy((char *)new_name,"PictureImageDynaMap");
			} else {
				printf("Error: I don't know how to fix %s!\n",\
				prp->Index[i].objects[e].desc.name);
				return -1;
			}

			//The fix
			*(U32 *)(abuf+off)=0x01;
			off+=4;
			*(U16 *)(abuf+off)=0x01;
			off+=2;
			*(U32 *)(abuf+off)=prp->Index[i].objects[e].desc.page_id;
			off+=4;
			*(U16 *)(abuf+off)=prp->Index[i].objects[e].desc.page_type;
			off+=2;
			*(U16 *)(abuf+off)=0x07; //object type
			off+=2;
			off+=encode_urustring(abuf+off,new_name,strlen((char *)new_name),1);

			//update things
			free((void *)prp->Index[i].objects[e].buf);
			prp->Index[i].objects[e].buf=abuf;
			prp->Index[i].objects[e].size=off;

			save=1;
		}
	}

	return save;
}

int prp_patch_refs(st_PrpHeader * prp) {

	int i,e;

	int f,total,count,r;

	total=0;
	count=0;

	//good key
	for(r=0; r<NPatches; r++) {
		if((prp->PageId & 0x000000FF)==refpatch[r][0]) {
			total++;
			count++;
			prp->PageId=refpatch[r][1] | 0xFFFF0200;
			break;
		}
	}


	for(i=0; i<prp->IndexCount; i++) {
		for(e=0; e<prp->Index[i].count; e++) {
			printf("Scanning object %i,%i... ",i,e);
			count=0;

			for(r=0; r<NPatches; r++) {
				if((prp->Index[i].objects[e].desc.page_id & 0x000000FF)==refpatch[r][0]) {
					total++;
					count++;
					prp->Index[i].objects[e].desc.page_id=refpatch[r][1] | 0xFFFF0200;
					break;
				}
			}

			for(f=0; f<(int)(prp->Index[i].objects[e].size-4); f++) {
				if(*(Byte *)(prp->Index[i].objects[e].buf+f)==0x02) {
					if(*(U32 *)(prp->Index[i].objects[e].buf+f)==0x04FFFF02) {

						//good key
						for(r=0; r<NPatches; r++) {
							if((*(Byte *)(prp->Index[i].objects[e].buf+(f-1)))==refpatch[r][0]) {
								total++;
								count++;
								*(Byte *)(prp->Index[i].objects[e].buf+(f-1))=refpatch[r][1];
								break;
							}

						}

					}
				}

			}
			printf(" %i keys where replaced\n",count);

		}
	}
	printf(" %i total keys where replaced\n",total);
	//if(total!=0) { return=1; }
	//return 0;
	return total;

}

//The patchet itself

int do_all_patches(char * uu,char * tpots,char * out) {

	//Do here all the SDL patches

	/*
		read source sdl file to patch from tpots/sdl/file.sdl & out/srcbak/sdl/file.sdl
		compile
		md5 it
		check the obtained md5 with the expected one
		if check fails, abort else continue
		store a backup of the file, if there isn't any previous one in out/sbak/ext/file
	*/

	//Do here all the Prp patches

	//Do here all *.age patches

	//Do here all *.fni patches

	return 0;
}


//main
int main(int argc, char * argv[]) {
	Byte * buf; //the buffer
	int fix=0; //update version headers?
	int repair=0; //repair the file? (migration from 11 to 12)
	int binsearch=0; //do binary search for banned references
	int save=0; //save input file to disk?
	int compare=0; //compare two files?
	int list=0; //show file contents?
	int do_all=0; //do all patches
	//int size;
	//int off=0;

	int force=0; //force the update? (0 recomended, 1 force)

	//destination
	//int maxosize;
	char * params=NULL;

#ifdef _DBG_PRP_
	Byte n_flags=0x1F;
	Byte diff_flags=0x1F;
#else
	Byte n_flags=0x13;
	Byte diff_flags=0x0F;
#endif

	int ret=0;

	int i;//,e;

	char * input;
	input=NULL;
	char * fcomp;
	fcomp=NULL;

	buf=NULL; //avoid a SEGFAULT

	printf("This is the Uru \"prp\" patcher\n\n");
	printf("Warning your game can be easily damaged by using this nice software!!\n");
	version();
	if(license_check(stdout,argc,argv)==1) {
		return -1;
	}
	//printf("%s",__uru_disclaimer_short);
	fflush(0);

	if(argc<=1) {
		version();
		usage();
		return 0;
	//} else if(argc==2) {
	//	input=argv[1];
	} else {
		if(!strcmp(argv[1],"fix") && argc==3) {
			fix=1;
			input=argv[2];
		} else if(!strcmp(argv[1],"ls") && argc==3) {
			input=argv[2];
			list=1;
		} else if(!strcmp(argv[1],"diff") && (argc==4 || argc==5)) {
			input=argv[2];
			fcomp=argv[3];
			compare=1;
			if(argc==5) {
				diff_flags=0;
				for(i=0; i<(int)strlen(argv[4]); i++) {
					//printf("%i...\n",i); fflush(0);
					params=argv[4];
					if(tolower(params[i])=='d') { diff_flags|=0x04; }
					else if(tolower(params[i])=='e') { diff_flags|=0x08; }
					else if(tolower(params[i])=='r') { diff_flags|=0x02; }
					else if(tolower(params[i])=='l') { diff_flags|=0x01; }
				}
			}
		} else if(!strcmp(argv[1],"bans") && argc==3) {
			input=argv[2];
			fix=1;
			binsearch=1;
			if(argc==4) { params=argv[3]; }
		} else if(!strcmp(argv[1],"patch") && argc==3) {
			fix=1;
			repair=1;
			binsearch=1;
			input=argv[2];
			if(argc==4) { params=argv[3]; }
		} else if(!strcmp(argv[1],"patchall") && argc==5) {
			do_all=1;
		} else if(!strcmp(argv[1],"-v")) {
			version();
			return 0;
		} else if(!strcmp(argv[1],"-h")) {
			version();
			usage();
			return 0;
		} else {
			version();
			usage();
			return 0;
		}
	}

	if(do_all==1) {
		//do all patches here
		ret=do_all_patches(argv[2],argv[3],argv[4]);
		if(ret<0) {
			fprintf(stderr,"Fatal: Error occured patching your game!, please be sure that both, the Until Uru installation and the Tpots installation are clean. Uninstall any patches like fly mode, or reinstall your game(s).\n");
			//ask();
			return -1;
		} else {
			printf("Congratulations, your game has been succesfully patched!, now you can play in any H'uru server, with your new Ki\n");
			return 1;
		}


	} else {

		st_PrpHeader prp;

		ret=readprp(&prp,input,n_flags);

		if(ret<0) {
			printf("Fatal error reading input file %s\n",input);
			return -1;
		}

	//list

		if(list==1) {
			listprp(&prp);
		}

	//compare

		if(compare==1) {

			st_PrpHeader prp2;

			ret=readprp(&prp2,fcomp,n_flags);

			if(ret<0) {
				printf("Fatal error reading input file %s\n",fcomp);
				return -1;
			}

			ret=compareprps(&prp,&prp2,diff_flags);

		}

	/////Single Patches

		if(fix==1) { //update version header
			if(prp.MinorVersion!=11 || prp.MajorVersion!=63) {
				printf("Cannot update, I was expecting a prp version 63.11\n");
				if(force==0) {
					return -1;
				}
			} else {
				//*(U16 *)(buf+(off-2))=12; //tpots version
				prp.MinorVersion=12; //tpots version
				save=1;
			}
		}

		if(repair==1) { //repair the 0xBD object type
			ret=prp_patch_GUIDynDisplayCtrl(&prp);
			if(ret<0) {
				fprintf(stderr,"Failed patcher, attempting to patch a GUIDynDisplayCtrl object on %s\n",input);
				return -1;
			} else {
				if(ret==1) { save=1; }
			}
		}

		if(binsearch==1) { //Ok, perform a binary search and fix all the banned references

			ret=prp_patch_refs(&prp);

			if(ret!=0) { save=1; }

		}


	//end patches--->

	//--->

		//save=1; //force a save change

		//save changes?
		if(save==1) {
			char * c;
			printf("File changed, Save changes? (Warning, old file will be overwritten):\n");
			fflush(0);
			c=ask();
			if(!strcmp(c,"y")) {
				printf("Saving... please wait...\n");
				//savefile((char *)buf,size,input); //imput
				saveprp(&prp,input,n_flags);
			} else {
				printf("Not saving changes.\n");
			}
		}

	}

	return 0;

}

