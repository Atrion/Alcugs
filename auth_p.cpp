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

//server side message parser

/* CVS tag - DON'T TOUCH*/
#define __U_AUTH_ID "$Id$"
const char * _auth_driver_ver="1.1.1e-private";

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "data_types.h"
#include "stdebug.h"
#include "prot.h"
#include "protocol.h"

#include "conv_funs.h"

#include "md5.h"

#include "config_parser.h"
#include "uru.h"

#include "sql.h"
#include "cgas_auth.h"

#include "auth_db_p.h"

#include "auth_p.h"

#include "debug.h"


int auth_initialitzed=0;
int auth_default_access_level=AcPlayer;

Byte local=1; //Allow local logins
Byte cgas=0; //enable cgas auth
Byte cgas_cache=1; //enable cgas auth cache
U32 cgas_cache_expires=60*60*24*2; // two days
Byte min_alevel=AcNotActivated;
U32 a_att=6; //allowed attempts
U32 dis_time=5*60;
U32 cache_miss=3*60; //cache miss

Byte cgas_log=1;

st_log * f_acc=NULL;

st_sql db;

int init_auth_driver() {
	int ret=0;
	if(auth_initialitzed) return 0;
	auth_initialitzed=1;

	DBG(5,"Initialiting auth driver...\n");

	//set up settings
	auth_default_access_level=cnf_getByte(AcPlayer,"default_access_level",\
	"global",global_config);
	min_alevel=cnf_getByte(AcNotActivated,"auth.minalevel",\
	"global",global_config);

	if(min_alevel>AcNotActivated) min_alevel=AcNotActivated;

	//auth subsystem
	local=cnf_getByte(local,"auth.local","global",global_config);
	cgas=cnf_getByte(cgas,"auth.cgas","global",global_config);
	cgas_cache=cnf_getByte(cgas_cache,"auth.cgas.cache","global",global_config);
	cgas_cache_expires=cnf_getU32(cgas_cache_expires,"auth.cgas.expire","global",\
	global_config);
	cache_miss=cnf_getU32(cache_miss,"auth.cgas.miss","global",global_config);

	//cgas logging
	cgas_log=cnf_getByte(cgas_log,"auth.cgas.log","global",global_config);

	//params
	a_att=cnf_getU32(a_att,"auth.att","global",global_config);
	dis_time=cnf_getU32(dis_time,"auth.distime","global",global_config);

	if(cnf_getByte(1,"auth.log","global",global_config)==1) {
		f_acc=open_log("auth.log",2,DF_STDOUT | DF_APPEND);
	}

	sql_init(&db);
	//set here database params
	char * tmp;
	tmp = (char *)cnf_getString("","db.host","global",global_config);
	db.host=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(db.host,tmp);
	tmp = cnf_getString("","db.username","global",global_config);
	db.username=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(db.username,tmp);
	tmp = cnf_getString("","db.passwd","global",global_config);
	db.passwd=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(db.passwd,tmp);
	tmp = cnf_getString("","db.name","global",global_config);
	db.name=(char *)malloc((strlen(tmp)+1) * sizeof(char));
	strcpy(db.name,tmp);
	db.port= cnf_getU16(0,"db.port","global",global_config);

	//set more params
	if(cnf_getByte(1,"db.log","global",global_config)==0) {
		db.flags &= ~SQL_LOG; //dissable logging
	}
	if(cnf_getByte(1,"db.sql.log","global",global_config)==0) {
		db.flags &= ~SQL_LOGQ; //dissable logging sql statements
	}
	if(cnf_getByte(1,"db.persistent","global",global_config)==0) {
		db.flags &= ~SQL_STAYCONN; //dissable Always stay connected
	}
	db.timeout=cnf_getU32(db.timeout,"db.timeout","global",global_config);

	//start up the sql driver
	sql_start(&db);

	//now check fo the databases
	if(plVaultInitializeAuthDB(&db,0)<0) {
		ret=-1;
		plog(f_acc,"INF: Fatal could not start authentication driver!\n");
		plog(f_err,"Auth initialization failed!\n");
		stop_auth_driver();
	} else {
		plog(f_acc,"INF: Authentication driver succesfully started v%s\n",_auth_driver_ver);
		ret=0;
	}

	print2log(f_acc,"default_alevel:%i,min_alevel:%i,att:%i,distime:%i\n",\
	auth_default_access_level,min_alevel,a_att,dis_time);
	logflush(f_acc);

	return ret;
}

void stop_auth_driver() {
	if(auth_initialitzed!=1) return;
	//This will work, only if you always call init_auth_driver after this ...
	if(db.host!=NULL) free((void *)db.host);
	if(db.username!=NULL) free((void *)db.username);
	if(db.passwd!=NULL) free((void *)db.passwd);
	if(db.name!=NULL) free((void *)db.name);
	db.host=NULL;
	db.username=NULL;
	db.passwd=NULL;
	db.name=NULL;
	plog(f_acc,"INF: Authentication driver stopped\n");
	auth_initialitzed=0;
	sql_shutdown(&db);
	close_log(f_acc);
	f_acc=NULL;
}

//Needs to be done on SIGHUP to be able to apply any changes performed in the config files
void reload_auth_driver() {
	plog(f_acc,"INF: Reloading auth driver...\n");
	stop_auth_driver();
	init_auth_driver();
}

//This must be called some times each x seconds for the proper operation of the driver
// It will be called on each netcore idle loop
void auth_idle_operation() {
	sql_idle(&db);
}

void calculate_champhash(char * login,char * passwd,char * challenge,char * str_hash) {
	Byte * md5buffer;
	Byte hash[17];
	//calculate the server_side hash
	md5buffer=(Byte *)malloc((strlen((const char *)login)+32+32+1)*sizeof(Byte));
	//hex2ascii2(md5buffer,(Byte *)challenge,16);
	strcpy((char *)md5buffer,(char *)challenge);
	//DBG(3,"login is %s\n",login);
	strcat((char *)md5buffer,(char *)login);
	//hex2ascii(u.passwd,md5buffer+32+strlen((const char *)u.login),16);
	//DBG(3,"passwd is %s\n",passwd);
	strcat((char *)md5buffer,(char *)passwd);
	//print2log(f_uru,"\nmd5buffer Hash: %s\n",md5buffer);
	MD5(md5buffer,32+32+strlen((const char *)login),hash);
	hex2ascii2((Byte *)str_hash,(Byte *)hash,16);
	free((void *)md5buffer);
}


int authenticate_player(char * login,char * challenge,char * aux_hash,char release,\
char * ip,char * passwd,char * guid,char * access_level) {
	Byte flag;

	U32 s_att; //timestamp of last attempt
	U32 cached; //timestamp of the last cgas query update
	U32 n_att=0; //Number of attempts
	U32 current_t; //Current time
	time((time_t *)&current_t); //set current stamp
	cached=current_t;
	s_att=current_t;

	char hash[33];

	int rcode=-1;
	int ret;
	ret=0;

	//check for local accounts
	char * test_p;
	test_p="local@";
	flag=1;

	*access_level=AcNotRes;
	strcpy(passwd,"");
	strcpy(guid,"00000000-0000-0000-0000-000000000000");
	memset(hash,'0',32);
	hash[32]='\0';

	if(auth_initialitzed==0)
		if(init_auth_driver()!=0)
			return 0xFF;

	if(cgas==1) { //CGAS auth
		flag=memcmp(login,test_p,strlen(test_p));
		if(flag==0 && local==1) {
			rcode=plVaultQueryUserName((Byte *)login+strlen(test_p),(Byte *)passwd,(Byte *)guid,\
			0,&s_att,&cached,&n_att,&db);
			calculate_champhash((char *)login,(char *)passwd,(char *)challenge,(char *)hash);
			if(current_t-s_att>(dis_time)) {
				n_att=0; //autoexpire number of attempts
			}
		} else { //get cached account
			rcode=plVaultQueryUserName((Byte *)login,(Byte *)passwd,(Byte *)guid,\
			1,&s_att,&cached,&n_att,&db);
			if(current_t-s_att>(dis_time)) {
				n_att=0; //autoexpire number of attempts
			}
			if(cgas_cache!=1) { //cgas cache disabled
				strcpy((char *)passwd,"");
			}
			DBG(5,"rcode is %i\n",rcode);

			//CGAS query, only if some things are meet
			//(the CGAS cache is dissabled or the account was not found in the database)
			if(rcode<0 || cgas_cache!=1 || (current_t-cgas_cache_expires)>cached) {
				if((auth_default_access_level<min_alevel) || (rcode>=0 && rcode<min_alevel)) {
					if(n_att>a_att && current_t-s_att<(dis_time)) {
						plog(f_acc,"Not quering CGAS because number of attempts is %i of %i. Login will be enabled again in %i minutes.\n",n_att,a_att,dis_time/60);
						rcode=AcNotRes;
					} else {
						ret=CGAS_query((char *)login,(char *)challenge,(char *)ip,\
							(char *)hash,(char *)passwd,(char *)guid,cgas_log);
						cached=current_t; //set the last cache update time
						if(ret!=1) {
							if(rcode<0 || cgas_cache!=1) {
								rcode=AcNotRes; //hmmm... CGAS not responding (or user doesn't exist)
							//	needs to get again the passwd, guid, and other stuff
							} else {//<- allow access to already cached users
								rcode=plVaultQueryUserName((Byte *)login,(Byte *)passwd,(Byte *)guid,\
								1,&s_att,&cached,&n_att,&db); //query again
								calculate_champhash((char *)login,(char *)passwd,(char *)challenge,\
								(char *)hash);
							}
							plog(f_acc,"Seems that the CGAS is down, I'm sorry, but I can't validate the account :(\n");
						} else {
							if(rcode<0) {
								if(cgas_cache==1) { //store the user in the cache if enabled
									plVaultAddUser((Byte *)login,(Byte *)passwd,(Byte *)guid,\
									1,(Byte *)ip,auth_default_access_level,&db);
								}
								rcode=auth_default_access_level; //Normal default auth level
							} else {
								if(cgas_cache==1) {
									plVaultUpdateUserPasswd((Byte *)login,(Byte *)passwd,(Byte *)guid,1,\
									(Byte *)ip,&db);
								}
							}
						}
					}
				} else {
					rcode=auth_default_access_level;
				}
			} else {
				calculate_champhash((char *)login,(char *)passwd,(char *)challenge,(char *)hash);
			}
		}

	} else if(local==1) { //Local auth
		rcode=plVaultQueryUserName((Byte *)login,(Byte *)passwd,(Byte *)guid,\
		0,&s_att,&cached,&n_att,&db);
		calculate_champhash((char *)login,(char *)passwd,(char *)challenge,(char *)hash);
	} else { //Auth globally dissabled
		rcode=AcNotRes;
	}

	plog(f_acc,"Authenticating %s - %s %i/%i out, rcode:%i",login,guid,n_att,a_att,rcode);

	if(rcode<0) { //Not found!
		print2log(f_acc," Acc. Not Found! ");
		rcode=0xFE; //invalid username/passwd
		*access_level=AcNotActivated;
	} else {
		*access_level=rcode;
		if(*access_level<min_alevel) { rcode=0x01; } //a passwd must be in the buffer
		else if(*access_level<AcNotRes) {
			print2log(f_acc," Account Disabled! ");
			rcode=0xFC;
		} else {
			print2log(f_acc," MySql server down? ");
			rcode=0xFF;
		}
	}

	//Check tryes and timestamp (this should dissable login after 5 attempts, for 5 minutes
	if(n_att>a_att && current_t-s_att<(dis_time)) {
		rcode=0xFC; //Account disabled
		print2log(f_acc," Maxium number of attempts reached, disabling login for %i minutes\n",dis_time/60);
	}

	if(rcode==0x01) { //If there is a passwd, then check it..
		DBG(5,"aux_hash %s vs %s\n",aux_hash,hash);
		//abort();
		if(strcmp(aux_hash,hash)) {
			rcode=0xFD;
		}
	}

	//Check cache miss
	if(cgas_cache==1 && rcode==0xFD && cgas==1 && (current_t-cached)>(cache_miss) && !(flag==0 && local==1)) {
		ret=CGAS_query((char *)login,(char *)challenge,(char *)ip,\
		(char *)hash,(char *)passwd,(char *)guid,cgas_log);
		if(ret!=1) {
			rcode=0xFF;
		} else {
			rcode=0x01;
			if(strcmp(aux_hash,hash)) {
				rcode=0xFD;
			}
			plVaultUpdateUserPasswd((Byte *)login,(Byte *)passwd,(Byte *)guid,1,(Byte *)ip,&db);
		}
	}

	//check internal release
	if(rcode==0x01 && release==TIntRel && *access_level>AcCCR) {
		print2log(f_acc," Unauthorized client! ");
		rcode=0xFC;
	}

	Byte cgas_flag_acc;
	Byte log_offset;
	log_offset=0;
	if((flag!=0 || local==0) && cgas==1) {
		cgas_flag_acc=1;
	} else {
		cgas_flag_acc=0;
		if(local==1 && cgas==1) {
			log_offset=strlen(test_p);
		}
	}

	if(rcode==0x01) {
		print2log(f_acc," Valid auth\n");
		plog(f_acc,"Succesfull login for %s, level:%i, guid:%s, ip:%s\n",login,*access_level,guid,ip);
		rcode=0x00;
		if((local==1 && (flag==0 || cgas==0)) || (cgas_cache==1 && cgas==1)) {
			plVaultUpdateUser((Byte *)login+log_offset,(Byte *)guid,cgas_flag_acc,(Byte *)ip,0,&db);
		}
	} else {
		print2log(f_acc," Failed auth\n");
		plog(f_acc,"Auth failed for %s, reason %i,%s, ip:%s\n",login,rcode,unet_get_auth_code(rcode),ip);
		n_att++;
		if((local==1 && (flag==0 || cgas==0)) || (cgas_cache==1 && cgas==1)) {
			plVaultUpdateUser((Byte *)login+log_offset,(Byte *)guid,\
			cgas_flag_acc,(Byte *)ip,n_att,&db);
		}
	}
	logflush(f_acc);

	DBG(5,"access level is %i, rcode:%i\n",*access_level,rcode);

	return rcode;
}


#if 0
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
#endif

