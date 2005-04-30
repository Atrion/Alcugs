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
const char * _auth_driver_ver="1.1.1f-pb";

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

#include "auth_db.h"

#include "auth.h"

#include "debug.h"


int auth_initialitzed=0;
int auth_default_access_level=AcPlayer;

Byte min_alevel=AcNotActivated;
U32 a_att=6; //allowed attempts
U32 dis_time=5*60;

Byte cgas_log=1;
Byte auth_reg=1;

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

	//cgas logging
	cgas_log=cnf_getByte(cgas_log,"auth.cgas.log","global",global_config);

	//auth logging
	auth_reg=cnf_getByte(auth_reg,"auth.reg","global",global_config);

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

int authenticate_player(char * login,char * challenge,char * aux_hash,char release,\
char * ip,char * passwd,char * guid,char * access_level) {
	//WARNING!, THE MODIFICATION OF THIS CODE IN THE WHOLE OR PART WITH THE PURPOSE OF
	//BYPASSING THE CYAN'S GLOBAL AUTH SERVER GOES AGAINST THE URU CLIENT LICENSE

	U32 s_att; //timestamp of last attempt
	U32 n_att=0; //Number of attempts
	U32 current_t; //Current time
	time((time_t *)&current_t); //set current stamp
	s_att=current_t;

	char hash[33];

	int rcode=-1;
	int ret;
	ret=0;

	*access_level=AcNotRes;
	strcpy(passwd,"");
	strcpy(guid,"00000000-0000-0000-0000-000000000000");
	memset(hash,'0',32);
	hash[32]='\0';

	if(auth_initialitzed==0)
		if(init_auth_driver()!=0)
			return 0xFF;

	rcode=plVaultQueryUserName((Byte *)login,&s_att,&n_att,&db);
	if(current_t-s_att>(dis_time)) {
		n_att=0; //autoexpire number of attempts
	}
	DBG(5,"rcode is %i\n",rcode);

	if((auth_default_access_level<min_alevel) || (rcode>=0 && rcode<min_alevel)) {
		if(n_att>a_att && current_t-s_att<(dis_time)) {
			plog(f_acc,"Not quering CGAS because number of attempts is %i of %i. Login will be enabled again in %i minutes.\n",n_att,a_att,dis_time/60);
			rcode=AcNotRes;
		} else {
			ret=CGAS_query((char *)login,(char *)challenge,(char *)ip,\
					(char *)hash,(char *)passwd,(char *)guid,cgas_log);
			if(ret!=1) {
				rcode=AcNotRes; //hmmm... CGAS not responding (or user doesn't exist)
				plog(f_acc,"Seems that the CGAS is down, I'm sorry, but I can't validate the account :(\n");
			} else {
				if(rcode<0) {
					if(auth_reg==1) { //store the user if enabled
						plVaultAddUser((Byte *)login,(Byte *)guid,\
						(Byte *)ip,auth_default_access_level,&db);
					}
					rcode=auth_default_access_level; //Normal default auth level
				}
			}
		}
	} else {
		rcode=auth_default_access_level;
	}

	plog(f_acc,"Authenticating %s - %s %i/%i out, rcode:%i",login,guid,n_att,a_att,rcode);

	*access_level=rcode;
	if(*access_level<min_alevel) { rcode=0x01; } //a passwd must be in the buffer
	else if(*access_level<AcNotRes) {
		print2log(f_acc," Account Disabled! ");
		rcode=0xFC;
	} else {
		print2log(f_acc," MySql server down? ");
		rcode=0xFF;
	}

	//Check tryes and timestamp (this should dissable login after 5 attempts, for 5 minutes
	if(n_att>a_att && current_t-s_att<(dis_time)) {
		rcode=0xFC; //Account disabled
		print2log(f_acc," Maxium number of attempts reached, disabling login for %i minutes\n",dis_time/60);
	}

	if(rcode==0x01) { //If there is a passwd, then check it..
		if(strcmp(aux_hash,hash)) {
			rcode=0xFD;
		}
	}

	//check internal release
	if(rcode==0x01 && release==TIntRel && *access_level>AcCCR) {
		print2log(f_acc," Unauthorized client! ");
		rcode=0xFC;
	}

	if(rcode==0x01) {
		print2log(f_acc," Valid auth\n");
		plog(f_acc,"Succesfull login for %s, level:%i, guid:%s, ip:%s\n",login,*access_level,guid,ip);
		rcode=0x00;
		if(auth_reg==1) {
			plVaultUpdateUser((Byte *)login,(Byte *)guid,(Byte *)ip,0,&db);
		}
	} else {
		print2log(f_acc," Failed auth\n");
		plog(f_acc,"Auth failed for %s, reason %i,%s, ip:%s\n",login,rcode,unet_get_auth_code(rcode),ip);
		n_att++;
		if(auth_reg==1) {
			plVaultUpdateUser((Byte *)login,(Byte *)guid,(Byte *)ip,n_att,&db);
		}
	}
	logflush(f_acc);

	DBG(5,"access level is %i, rcode:%i\n",*access_level,rcode);

	return rcode;
}

