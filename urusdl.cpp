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

const char * ID = "$Id$";
const char * BUILD =  __DATE__ " " __TIME__;
const char * SNAME = "urusdl tool";
const char * VERSION = "1.3";

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <stdio.h>


#define DEBUG

#include "license.h"
#include "data_types.h" //for U32,Byte and others
#include "files.h" //to open close files into a buffer (nice eh!)
#include "conv_funs.h"
#include "adv_gamemsgparser.h"
#include "sdlparser.h"

#include "stdebug.h"

#include "config_parser.h"
#include "tmp_config.h"

int whoami;

//Utility to uncompress/decompress uru files (works!)

int main(int argc, char * argv[]) {

	int what;  //action to do
	license_check(stdout,argc,argv);
	printf("This is the Uru SDL parser\n\nWARNING: Use it at your own risk\nThe authors of this software are not responsible of any strange effects that this software may cause at your computer or game.\n\n");
	fflush(0);
	int totalsize;
	int off; //offset
	//compression stuff
	int realsize; //real size
	int compsize; //compressed size
	Byte * buf=NULL; //the source Buffer
	Byte * dbuf=NULL; //the destination buffer
	int erro; //save error codes here

	t_sdl_def * sdl=NULL; //The sdl struct
	int n_sdl; //number of sdl descriptors

  if(argc<3 || argc>4) {
		printf("Usage:\nFor decompress: \n  urusdl -d source.sav destination.sdl\n");
		printf("For compress: \n urusdl -c source.sdl destination.sav\n");
		printf("To parse the uncompressed sdl:\n urusdl -p source.sdl\n");
		printf("To parse a compressed sdl:\n urusdl -dp source.sav\n");
		printf("To compile a sdl:\n urusdl -m source.sdl\n");
		printf("To compile and compress a sdl:\n urusdl -cm source.sdl\n");
		printf("To compile only an sdl header:\n urusdl -chsdl source.sdl out.raw\n");
		printf("To parse only an sdl compiled header:\n urusdl -phsdl source.raw out.sdl\n");
		printf("To decompress a plNetMsgJoinAck server stream:\n urusdl -dnja source.raw destination.raw\n");
		printf("To decompress a plNetMsgJoinAck server stream (without header):\n urusdl -dnja2 source.raw destination.raw\n");
		printf("To decompress a plNetMsgSDLBCast server stream:\n urusdl -dsbc source.raw destination.raw\n");
		printf("To decompress a plNetMsgSDLBCast server stream (without header):\n urusdl -dsbc2 source.raw destination.raw\n");
		printf("To decompress a plNetMsgSDLState server stream:\n urusdl -dss source.raw destination.raw\n");
		return -1;
  }

	if(!strcmp(argv[1],"-d") && argc==4) {
		what=1; //decompress & store
	} else if(!strcmp(argv[1],"-c") && argc==4) {
		what=2; //compress & store
	} else if(!strcmp(argv[1],"-p") && argc==3) {
		what=3; //only parse
	} else if(!strcmp(argv[1],"-dp") && argc==3) {
		what=4; //decompress and parse
	} else if(!strcmp(argv[1],"-m") && argc==3) {
		what=5; //compile
	} else if(!strcmp(argv[1],"-cm") && argc==3) {
		what=6; //compile and compress
	} else if(!strcmp(argv[1],"-chsdl") && argc==4) {
		what=7; //compile only a sdl header
	} else if(!strcmp(argv[1],"-phsdl") && argc==3) {
		what=8; //parse only a sdl header
	} else if(!strcmp(argv[1],"-dnja") && argc==4) {
		what=15; //uncompress a plNetMsgJoinAck
	} else if(!strcmp(argv[1],"-dsbc2") && argc==4) {
		what=18; //uncompress a plNetMsgSDLBCast without header
	} else if(!strcmp(argv[1],"-dss") && argc==4) {
		what=19; //uncompress a plNetMsgSDLState with header
	} else {
		printf("Type urusdl for help\n");
		return -1;
  }
  fflush(0);

	//open the source, and store it into buf
	totalsize=readfile((char **)&buf,argv[2]);
	off=0;

	//raw file info
	U32 magic1; //0x01
	U16 magic2; //0x01 // 0x07
	U16 magic3; //0x01 // 0x02
	Byte magic4; //0x00 // 0x1F

	Byte age_name[100];
	Byte flag; //0x03
	Byte ip[100];
	U16 port;
	Byte guid[8];
	Byte guid2[20];
	//end raw file info

	int size_off; //where the size offsets are

	//auto code for dummies goes here //TODO

	//compile code should go here //TODO
	if(what==7) {
		sdl_statedesc_reader(stderr,buf+off,totalsize-off,&sdl,&n_sdl);
		totalsize=sdl_statedesc_streamer(stderr,&dbuf,sdl,n_sdl);
		realsize=totalsize;
	}

	if(what==1 || what==4) { //decompress

		magic1=*(U32 *)(buf+off); //data //0x00 no data, 0x01 compressed data
		off+=4;
		magic2=*(U16 *)(buf+off); //major version 0x07
		off+=2;
		magic3=*(U16 *)(buf+off); //minor version 0x02
		off+=2;
		magic4=*(Byte *)(buf+off); //mask
		off++;

		if(magic1==0) {
			printf(" Nothing to Do, this file is void\n");
			free((void *)buf);
			return -1;
		} else if(magic1!=0x01 || (magic2!=0x01 && magic2!=0x07 ) || (magic3!=0x01 && magic3!=0x02) || (magic4!=0x00 && magic4!=0x1F)) {
			printf(" Unknown format!\n");
			free((void *)buf);
			return -1;
		} else {
			printf(" Recognized compressed check");
		}

		if(magic4==0x1F) {
			off+=decode_urustring(age_name,buf+off,99);
			off+=2;
			flag=*(Byte *)(buf+off);
			off++;
			off+=decode_urustring(ip,buf+off,99);
			off+=2;
			port=*(U16 *)(buf+off);
			off+=2;
			memcpy(guid,buf+off,8);
			off+=8;
			hex2ascii2(guid2,guid,8);

			if(flag!=0x03) {
				printf("Unexpected flag found! %02X\n",flag);
				free((void *)buf);
				return -1;
			}

			printf("OK - Valid Plasma SDL format found: Age: %s,address:%s:%i,guid:%s\n",\
			age_name,ip,port,guid2);
		}

		realsize=*(U32 *)(buf+off);
		off+=4;
		compsize=*(U32 *)(buf+off);
		off+=4;

		if(compsize==0) {
			printf(" 0 compressed Bytes?\n");
			free((void *)buf);
			return 0;
		}

		if(compsize!=(totalsize-off)) {
			printf(" File is inconsistent, it claims to have %i bytes, but only has %i bytes!\n",compsize,totalsize-off);
			free((void *)buf);
			return -1;
		}
		printf(" %i bytes, will be uncompressed into %i bytes\n",compsize,realsize);

		//allocation dbuffer
		dbuf=(Byte *)malloc(sizeof(Byte) * realsize);

		printf("Uncompressing file...\n");
    erro = uncompress(dbuf,(uLongf*)&realsize,buf+off,compsize);
		if(erro!=0) {
			printf("Seems that an error ocurred uncompressing the file!\n");
			free((void *)buf);
			free((void *)dbuf);
			return -1;
		}

		//now we can free the buf
		free((void *)buf);

	} else if(what==2) { //compress & store the file

		realsize=totalsize; //fix the uncompressed size
		compsize=totalsize;
		if(realsize==0) {
			printf("Cannot compress a void file!\n");
			free((void *)buf);
			return -1;
		}

		//allocating
		dbuf=(Byte *)malloc(sizeof(Byte) * compsize);

		*(U32 *)(dbuf+off)=0x01;
		off+=4;
		*(U16 *)(dbuf+off)=0x01;
		off+=2;
		*(U16 *)(dbuf+off)=0x01;
		off+=2;
		*(Byte *)(dbuf+off)=0x00;
		off++;

		//set realsize
		*(U32 *)(dbuf+off)=realsize;
		off+=4;

		size_off=off;
		//here goes the compressed size
		off+=4;

		printf("Compressing file...\n");

		erro = compress(dbuf+off,(uLongf*)&compsize,buf,realsize);
		if(erro!=0) {
			printf("Seems that an error ocurred compressing the file!\n");
			free((void *)buf);
			free((void *)dbuf);
			return -1;
		}

		//free buffer
		free((void *)buf);

		//store compressed size
		*(U32 *)(dbuf+size_off)=compsize;

		off+=compsize;
		realsize=off; //;)
	} else if(what==15 || what==18 || what==19) { //server side SDL file

		st_UruObjectDesc o;
		int ret;

		if(what==19) {
			off+=2+4;
			if(*(U32 *)(buf+off-4) & 0x00001000) {
				off+=4;
			}
			what=18; //only changes the offset
			totalsize++; //add an extra byte (because later it will be dropped)
		}

		//stuff starts at offset 2+4+4+4+2
		if(what==15) {
			off+=2+4+4+4+2;
		} else if(what==18) {
			ret=storeUruObjectDesc(buf+off,&o,totalsize-off);
			if(ret<0) {
				printf(" An error ocurred parsing an UruObject!\n");
				exit(-1);
			}
			off+=ret;
			dumpUruObjectDesc(stdout,&o);
			printf("\n");
			totalsize-=2;
		}

		realsize=*(U32 *)(buf+off);
		off+=4;
		off++; //there is a flag, that should be always 0x02 (or at least we saw that)
		compsize=*(U32 *)(buf+off);
		off+=4;
		//strip out 2 bytes of the compressed size
		compsize-=2;
		//now bypass the 0x00 and 0x80 bytes
		off+=2;

		if(compsize==0) {
			printf(" 0 compressed Bytes?\n");
			free((void *)buf);
			return 0;
		}

		if(compsize!=(totalsize-off)) {
			printf(" File is inconsistent, it claims to have %i bytes, but only has %i bytes!\n",compsize,totalsize-off);
			free((void *)buf);
			return -1;
		}
		printf(" %i bytes, will be uncompressed into %i bytes\n",compsize,realsize);

		printf("Uncompressing file...\n");
		if(realsize!=0) {
			//allocation dbuffer
			dbuf=(Byte *)malloc(sizeof(Byte) * realsize);
			erro = uncompress(dbuf,(uLongf*)&realsize,buf+off,compsize);
		} else {
			//allocation dbuffer
			dbuf=(Byte *)malloc(sizeof(Byte) * compsize);
			memcpy(dbuf,buf+off,compsize);
		}
		if(erro!=0) {
			printf("Seems that an error ocurred uncompressing the file!\n");
			free((void *)buf);
			free((void *)dbuf);
			return -1;
		}

		//now we can free the buf
		free((void *)buf);


	}

	//now check if we need to store
	if(what==1 || what==2 || what==15 || what==18 || what==7) {
		//and store the uncompressed file, if it was requested
		savefile((char *)dbuf,realsize,argv[3]);
	}

	if(what==4) {
		totalsize=realsize;
		buf=dbuf; //set buf with the data
	}

	if(what==8) {
		magic4=0x00;
	}

	if(what==4 || what==3 || what==8) { //do sdl parser here
		off=0;
		FILE * f_report;
		char where[230];
		sprintf(where,"%s.txt",argv[2]);
		//f_report=fopen("sdl.txt","w");
		f_report=fopen(where,"w");

		U32 timestamp;
		time((time_t *)&timestamp);

		fprintf(f_report,"\r\n\r\n# Age state description language file has been generated from %s\r\n# Generated: %s\r\n",argv[2],get_stime(timestamp,0));
		if(what!=8) {
			fprintf(f_report,"\r\n\r\nHEADER { \r\n");
			if(magic4==0x1F) {
				fprintf(f_report,"  TYPE = server\r\n");
				fprintf(f_report,"  AGE  = %s\r\n",age_name);
				fprintf(f_report,"  IP   = %s\r\n",ip);
				fprintf(f_report,"  PORT = %i\r\n",port);
				fprintf(f_report,"  GUID = %s\r\n",guid2);
			} else {
				fprintf(f_report,"  TYPE = offline\r\n");
			}
			fprintf(f_report,"}\r\n\r\n#SDL definitions, please don't edit them (or do it, at your own risk!)\r\n\r\nSDLDEF {\r\n");
		}

		off+=sdl_parse_contents(f_report,buf,totalsize,&sdl,&n_sdl);

		if(what!=8) {
			fprintf(f_report,"}\r\n\r\n\r\n# Game Objects SDL\r\n\
	#  Feel free to edit/add/delete game objects from this list,\r\n\
	#  this will affect to the status of different game objects.\r\n\
	#  The sintaxys is very strict, so be sure that each Object\r\n\
	#  entry Matches exactly the corresponding entry in the above\r\n\
	#  SDLDEF section\r\n\r\nSDL {\r\n");

			printf("The SDL header ends at offset %i,%08X\n",off,off);

			fprintf(f_report,"\r\n\r\n ######### Binary Object data extraction here ######## \r\n\r\n");

			off+=sdl_parse_binary_data(f_report,buf+off,totalsize-off,sdl,n_sdl);

			fprintf(f_report,"}\r\n");
		}

		fclose(f_report);

		U32 end;

		if(what!=8) {
			end=*(U32 *)(buf+off);
			off+=4;

			if(end!=0) {
				printf("Unexpected non-zero value at end of stream!\n");
			}
		}

		if(off!=totalsize) {
			printf("FATAL, parsed %i bytes, stream has %i bytes, diff %i bytes\n",off,totalsize,totalsize-off);
		}

		destroy_sdl_def(sdl,n_sdl);
		if(sdl!=NULL) {
			free((void *)sdl);
			sdl=NULL;
		}
	}

	return 0;
}
