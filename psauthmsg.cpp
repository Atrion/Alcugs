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

//server side message parser

#ifndef __U_AUTH_MSG_
#define __U_AUTH_MSG_
/* CVS tag - DON'T TOUCH*/
#define __U_AUTH_MSG_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"
#include "debug.h"

#include "config_parser.h"
#include "stdebug.h"
#include "conv_funs.h"
#include "protocol.h"
#include "urunet.h"

#include "vaultstrs.h"
#include "urumsg.h"

#include "auth_db.h"
#include "cgas_auth.h"

#include "psauthmsg.h"

/*-----------------------------------------------------------------
	processes the plNetMsg and sends the adient reply (preliminar)
	 -- returns the size of the packet --
------------------------------------------------------------------*/
int process_auth_plNetMsg(int sock,Byte * buf,int size,st_uru_client * u) {

	if(_DBG_LEVEL_>=3) {
		dump_packet(f_uru,buf,size,0,7);
		print2log(f_uru,"\n-------------\n");
	}

	int offset=0;
	int i,n;

	int rcode;

	//THE BIG VERY VERY BIG Switch
	switch (u->adv_msg.cmd) {
		case NetMsgCustomAuthAsk:
			//int rcode;
			U32 ip;
			Byte hash[33];
			Byte aux_hash[33];
			//Byte * md5buffer;
			print2log(f_uru,"<RCV> NetMsgCustomAuthAsk ");
			//login
			offset+=decode_urustring(u->login,buf+offset,100);
			offset+=2;
			//challenge
			memcpy(u->challenge,buf+offset,16);
			offset+=16;
			hex2ascii2(aux_hash,u->challenge,16);
			print2log(f_uru,"challenge: %s ",aux_hash);
			//hash (client answer)
			memcpy(hash,buf+offset,16);
			offset+=16;
			hex2ascii2(aux_hash,hash,16);
			print2log(f_uru,"hash: %s ",aux_hash);
			//build
			u->release=*(Byte *)(buf+offset);
			offset++;
			print2log(f_uru,"build: %i ",u->release);
			//ip
			ip=*(U32 *)(buf+offset);
			offset+=4;

			//WARNING!, THE MODIFICATION OF THIS CODE IN THE WHOLE OR PART WITH THE PURPOSE OF
			//BYPASSING THE CYAN'S GLOBAL AUTH SERVER GOES AGAINST THE URU CLIENT LICENSE

			//U32 s_att; //timestamp of last attempt
			U32 n_att; //Number of attempts
			U32 current_t; //Current time
			time((time_t *)&current_t); //set current stamp


			Byte str_challenge[40];
			Byte * str_ip;
			Byte str_hash[40];
			Byte guid[40];

			hex2ascii2(str_challenge,u->challenge,16);
			str_ip=(Byte *)get_ip(ip);

			int ret;
			int userexists;

			ret=0;

			rcode=plVaultQueryUserName(u->login);
			userexists=(rcode<0 ? 0:1);
			strcpy((char *)u->passwd,"");

			ret=CGAS_query((char *)u->login,(char *)str_challenge,(char *)str_ip,\
(char *)str_hash,(char *)u->passwd,(char *)guid);
			if(ret!=1) {
				rcode=AcNotRes;
			} else if(rcode<0 || rcode==AcNotRes) {
				//rcode=15; //Normal default auth level
				rcode=global_default_access_level;
			}

			if(rcode<0) { //Not found!
				print2log(f_uru," Acc. Not Found! ");
				rcode=0xFE; //invalid username/passwd
				u->access_level=AcNotActivated;
			} else {
				u->access_level=rcode;
				if(u->access_level<AcNotActivated) { rcode=0x01; } //a passwd must be in the buffer
				else if(u->access_level<AcNotRes) {
					print2log(f_uru," Account Disabled! ");
					rcode=0xFC;
				} else {
					print2log(f_uru," MySql server down? ");
					rcode=0xFF;
				}
			}

			if(rcode==0x01) {
				ascii2hex2(aux_hash,str_hash,16);
				for(i=0; i<0x10; i++) {
					if(aux_hash[i]!=hash[i]) {
						rcode=0xFD;
						break;
					}
				}
			}

			//Check tryes and timestamp (this should dissable login after 5 attempts, for 5 minutes
			/*
			if(n_att>5 && current_t-s_att<(5*60)) {
				rcode=0xFC; //Account disabled
				print2log(f_uru," Maxium number of attempts reached, disabling login for 5 minutes\n");
			}*/

			//check internal release
			#if 0
			if(rcode==0x01 && u->release==TIntRel && u->access_level>AcCCR) {
				print2log(f_uru," Unauthorized client! ");
				rcode=0xFC;
			}
			#endif

			//server version check
			if(u->adv_msg.max_version!=global_max_version || u->adv_msg.min_version!=global_min_version) {
				print2log(f_err," ERR: Protocol version server mismatch %i.%i vs %i.%i\n",u->adv_msg.max_version,u->adv_msg.min_version,global_max_version,global_min_version);
				rcode=0xFF;
			}

			//put the guid
			//printf("--->%s<--",guid);
			for(i=0; i<(int)strlen((char *)guid); i++) {
				guid[i]=toupper(guid[i]);
			}
			memcpy((char *)u->guid,(char *)str_guid_to_hex(guid),16);

			//check internal release
			if(rcode==0x01 && u->release==TIntRel && u->access_level>AcCCR) {
				print2log(f_uru," Unauthorized client! ");
				rcode=0xFC;
			}

			// do we allow non_existing users in our accounts table ?
			if (!userexists && !global_allow_unknown_accounts) {
				print2log(f_uru," Unauthorized client! ");
				rcode=0xFC;
			}

			if(rcode==0x01) {
				print2log(f_uru," Valid passwd auth\n");
				stamp2log(f_acc);
				print2log(f_acc,"Succesfull login for %s, guid:%s, ip:%s\n",u->login,guid,str_ip);

				// Store new user into accounts table 
				// if auto_register_account is not active AND user dosen't exist, don't store
				if ((global_auto_register_account || userexists) && !plVaultStoreUserInfo(u->login, userexists, (int)1))
				  print2log(f_uru,"Can't store user infos into accounts table\n");

				//plNetMsgAccountAuthenticated(sock,0x00,u);
				plNetMsgCustomAuthResponse(sock,u->login,0x00,u->passwd,u->access_level,u);
				//u->authenticated=1;
				//print2log(f_uru," I HAVE SETUP AUTHENTICATED WITH VALUE=2<---\n\n");
			} else {
				print2log(f_uru," Incorrect passwd auth\n");
				stamp2log(f_acc);
				print2log(f_acc,"Auth failed for %s, reason %i, ip:%s\n",u->login,rcode,str_ip);
				//plNetMsgAccountAuthenticated(sock,rcode,u);
				plNetMsgCustomAuthResponse(sock,u->login,rcode,u->passwd,u->access_level,u);
				n_att++;
				//plNetMsgTerminated(sock,RNotAuthenticated,u);
				//plNetMsgTerminated(go out);
			}
			//abort();
			logflush(f_uru);
			logflush(f_acc);
			n=0;
			break;
		default:
			//print2log(f_uru,"<RCV> Uknown Type of packet! %04X\n",u->msg.cmd1);
			//---store
			//char unk_mes[100];
			//static Byte unk_mes_rot=0;
			//sprintf(unk_mes,"dumps/unk%02i.raw",unk_mes_rot);
			//savefile(buf,n,unk_mes);
			//---end store
			//plNetMsgTerminated(sock,RKickedOff,u);
			//abort();
			n=-7;
			break;
	}

	logflush(f_uru);

	return n;

}
#endif

