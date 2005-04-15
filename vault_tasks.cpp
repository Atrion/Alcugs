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

#ifndef __U_VAULT_TASKS_
#define __U_VAULT_TASKS_
/* CVS tag - DON'T TOUCH*/
#define __U_VAULT_TASKS_ID "$Id$"

/*
 Parses and generates Vault tasks
*/

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifndef __MSVC__
#  include <sys/time.h>
#  include <unistd.h>
#endif

#include "data_types.h"
#include "stdebug.h"
#include "conv_funs.h"
#include "protocol.h"
#include "urunet.h"
#include "vaultstrs.h"
#include "prot.h"

//#include "urumsg.h"
#include "gvaultmsg.h"

#include "vaultsubsys.h"
#include "vserversys.h"
#include "urustructs.h"
#include "vaultstrs.h"
#include "vnodes.h"
#include "vault_obj.h"
#include "vault_db.h"

#include "vault_advp.h" //to be able to do broadcasts
#include "guid_gen.h" //to generate unique id's
#include "htmldumper.h"
#include "uru.h"

#include "config_parser.h"

#include "vault_tasks.h"

#include "debug.h"

int plVaultInitVault() {
	int ret=1;

	t_vault_node * n=NULL;
	t_vault_manifest mfs;
	int n_nodes,i;
	int system,global,id;

	n_nodes=6;
	DBG(4,"plVaultGetFolder()\n");

	if(plVaultGetFolder(&db)<0) {
		print2log(f_err,"ERR: FATAL - Cannot negotatiate the folder, IS the MySQL server down?, or is there a misconfiguration?!\n");
		return -1;
	}

	n=(t_vault_node *)malloc(sizeof(t_vault_node) * n_nodes);

	DBG(5,"n_nodes=%i\n",n_nodes);
	for(i=0; i<n_nodes; i++) {
		init_node(&n[i]);
	}

  // Create AllPlayers folder
	n[0].type=KFolderNode;
	n[0].torans=KAllPlayersFolder;	// 12
	plVaultFindNode(&n[0],&mfs,1,&db); //create the node if not exists

  // Create AllAgeGlobalSDLNodesFolder folder
	n[1].type=KFolderNode;
	n[1].torans=KAllAgeGlobalSDLNodesFolder;	// 20
	plVaultFindNode(&n[1],&mfs,1,&db); //create the node if not exists

  // Create PublicAges folder
	n[2].type=KFolderNode;
	n[2].torans=KPublicAgesFolder; // 22
	plVaultFindNode(&n[2],&mfs,1,&db); //create the node if not exists

  // Create System node
	n[3].type=KSystem; // 24
	system=plVaultFindNode(&n[3],&mfs,1,&db); //create the node if not exists

  // Create GlobalInbox folder
	n[4].type=KFolderNode;
	n[4].torans=KGlobalInboxFolder; // 30
	global=plVaultFindNode(&n[4],&mfs,1,&db); //create the node if not exists

  //  Create System->GlobalInboxFolder link
	plVaultCreateRef(KVaultID,system,global,0,0,0,&db); //link it

	DBG(5,"Creating the default Welcome Message\n");
	n[5].type=KTextNoteNode;	// 26
	strncpy((char *)n[5].entry_name,(char *)gvinit_title,STR_MAX_SIZE-1);
	n[5].data=(Byte *)malloc(sizeof(Byte) * (1024));
	strncpy((char *)n[5].data,(char *)gvinit_desc,1023); //here was a nice buffer
	n[5].data_size=strlen((char *)n[5].data)+1;
	DBG(6,"plVaultCreateNode()\n");
	id=plVaultCreateNode(&n[5],&db);
	DBG(6,"plVaultCreateRef()\n");
	// Create GlobalInbox->TextNote link
	plVaultCreateRef(0,global,id,0,0,0,&db);  //KVaultID (must be 0 to be DRC owned)

	DBG(5,"n_nodes=%i\n",n_nodes);
	for(i=0; i<n_nodes; i++) {
		destroy_node(&n[i]);
	}
	free((void *)n);
	DBG(4,"Returning from plVaultInit(), %i\n",ret);
	return ret;
}

/** Creates an Age, awesome eh!
*/
int plVaultCreateAge(t_AgeInfoStruct * ainfo) {
	t_vault_node n;
	t_vault_manifest mfs;
	//t_vault_node * tnodes;
	//tnodes=NULL;
	//int n_tnodes=0;
	int age_mgr,age_info;

	//Get the Age MGR node
	init_node(&n);
	n.type=KVNodeMgrAgeNode;	// 3
	hex2ascii2(n.entry_name,ainfo->guid,8); //Set the age guid
	age_mgr=plVaultFindNode(&n,&mfs,1,&db); //Find or create the Age MGR node
	destroy_node(&n);

	//Now get the Age Info Node
	init_node(&n);
	n.type=KAgeInfoNode; // 33
	n.id2=age_mgr; //The parent of the info node
	strcpy((char *)n.entry_name,(char *)ainfo->filename); //set the filename
	//--> hmm, bad, very bad
	strcpy((char *)n.sub_entry_name,(char *)ainfo->instance_name);
	strcpy((char *)n.owner_name,(char *)ainfo->user_name);
	hex2ascii2(n.guid,ainfo->guid,8); //set the age guid
	strcpy((char *)n.entry_value,(char *)ainfo->display_name);
	//-->
	age_info=plVaultFindNode(&n,&mfs,1,&db); //Find or create the Age Info Node

	//n_tnodes=1;
	//plVaultFetchNodes(&tnodes,1,&age_info); //Get that node

	// Create Age->AgeInfo link
	plVaultCreateRef(0,age_mgr,age_info,0,0,0,&db);
	destroy_node(&n);
	return age_info; //return the age_info id
}

int plVaultAddLinkingPoint(st_unet * net,int ki,int age_id,t_SpawnPoint * spoint) {

	t_vault_node n;
	t_vault_manifest mfs;
	t_vault_node * tnodes;
	tnodes=NULL;
	int n_tnodes=0;
	t_vault_cross_ref * wref;
	int n_refs=0;
	wref=NULL;
	init_node(&n);

	int oages_id,found,i;

	n.type=KFolderNode; //0x16
	n.torans=KAgesIOwnFolder; // 23
	n.owner=ki; //Owner, the KI
	oages_id=plVaultFindNode(&n,&mfs,0,&db); //Now we have the Folder ID
	if(oages_id<=0) { //Not found, or db error (then create it, with the reference)
		oages_id=plVaultFindNode(&n,&mfs,1,&db);
		plVaultCreateRef(ki,ki,oages_id,0,0,0,&db);
		if(net!=NULL) {
			plVaultBcastNodeReferenceAdded(net,ki,oages_id);
		}
	}
	if(oages_id>0) {
		//Now get all the tree of references
		wref=NULL;
		n_refs=plVaultGetCrossRef(oages_id,&wref,&db);
		if(n_refs>=0) {
			//Now search in the manifest the NODE
			found=0;
			for(i=0; i<n_refs; i++) {
				if(wref[i].id3==(U32)age_id) {
					//Found the node
					found=wref[i].id2; //The link node
				}
			}
			//Todo (look the code of the old vtask.cpp), you need to:
			// 1-Create the node if it doesn't exists, and broadcast the added references
			// 2-Fetch the old node, and update it, and then broadcast the SavedNode
			if(found!=0) { //we found the node
				//fetch the node
				n_tnodes=1;
				int tmp1=0;
				plVaultFetchNodes(&tnodes,&tmp1,(U32 *)&found,1,&db);
			} else { //node not found, create it
				tnodes=(t_vault_node *)malloc(sizeof(t_vault_node) * 1);
				init_node(tnodes);
				tnodes[0].type=KAgeLinkNode; //0x1C 28
				tnodes[0].torans=1; //Locked status (0-unlocked,1-locked,elsewhere unknown)
				tnodes[0].distance=0; //Volatile status (0-non_volatile,1-volatile)
				tnodes[0].owner=ki; //Owner, the KI client id
			}

			//Init the node data
			if(tnodes->data==NULL) {
				tnodes->data=(Byte *)malloc(sizeof(Byte) * (strlen((char *)spoint->title)\
+ strlen((char *)spoint->name) + strlen((char *)spoint->camera)+5));
				strcpy((char *)tnodes->data,""); //arfgh!
			} else {
				tnodes->data=(Byte *)realloc((void *)tnodes->data,sizeof(Byte) * \
(strlen((char *)spoint->title) + strlen((char *)spoint->name)\
+ strlen((char *)spoint->camera)+5+strlen((char *)tnodes->data)));
			}

			//set linking point info
			strcat((char *)tnodes->data,(char *)spoint->title); //Link Title
			strcat((char *)tnodes->data,":");
			strcat((char *)tnodes->data,(char *)spoint->name); //Link Name
			strcat((char *)tnodes->data,":");
			strcat((char *)tnodes->data,(char *)spoint->camera); //Camera stack
			strcat((char *)tnodes->data,";");
			tnodes->data_size=strlen((char *)tnodes->data); //Update size of data

			if(found!=0) { //we found the node
				plVaultUpdateNode(tnodes,&db); //update the node
				double stamp;
				time((time_t *)&stamp);
				if(net!=NULL) {
					plVaultBcastNodeSaved(net,tnodes->index,stamp);
				}
			} else { //node not found
				//time((time_t *)&tnodes->timestamp); //unusefull
				found=plVaultCreateNode(tnodes,&db);
				//Link the link node with the Age Info Node
				plVaultCreateRef(ki,found,age_id,0,0,0,&db);
				//Link the AgesIOwnFolder with the Link Node
				plVaultCreateRef(ki,oages_id,found,0,0,0,&db);
				//Send a Vault Notification
				if(net!=NULL) {
					plVaultBcastNodeReferenceAdded(net,oages_id,found);
				}
			}
		}
	}
	destroy_node(&n);
	if(tnodes!=NULL) {
		for(i=0; i<n_tnodes; i++) {
			destroy_node(&tnodes[i]);
		}
		free((void *)tnodes);
	}
	return found;
}

//Age id, is the Age Info node eh!!
int plVaultAddOwnerToAge(st_unet * net,int age_id,int ki) {
	t_vault_node n;
	t_vault_manifest mfs;
	init_node(&n);
	int age_owners,p_info;

	//Find the age owners folder
	n.type=KPlayerInfoListNode; // 30
	n.torans=KAgeOwnersFolder; // 19
	n.owner=age_id;
	age_owners=plVaultFindNode(&n,&mfs,0,&db);
	if(age_owners<=0) {
		//Not found, then create it
		age_owners=plVaultFindNode(&n,&mfs,1,&db);
		plVaultCreateRef(0,age_id,age_owners,0,0,0,&db);
		if(net!=NULL) {
			plVaultBcastNodeReferenceAdded(net,age_id,age_owners);
		}
	}
	if(age_owners>0) {
		destroy_node(&n);
		init_node(&n);
		n.type=KPlayerInfoNode; // 23
		n.torans=0;
		n.owner=ki;
		p_info=plVaultFindNode(&n,&mfs,0,&db);
		if(p_info>0) { //found the player Info node (create the reference)
			plVaultCreateRef(0,age_owners,p_info,0,0,0,&db);
			if(net!=NULL) {
				plVaultBcastNodeReferenceAdded(net,age_owners,p_info);
			}
		}
	}
	destroy_node(&n);
	return age_owners;
}

int plVaultUpdatePlayerStatus(st_unet * net,U32 id,Byte * age,Byte * guid,Byte state,U32 online_time,int sid) {

	st_uru_client * u=&net->s[sid];

	t_vault_node n;
	t_vault_manifest mfs;
	t_vault_node * tnodes=NULL;
	int n_tnodes=1;
	int i,info_node;

	//Update mgr table
	if(state==1) {
		for(i=0; i<n_vmgrs; i++) {
			DBG(5,"id:%i vs %i\n",vmgrs[i].id,id);
			if(vmgrs[i].id==(int)id) {
				vmgrs[i].ip=u->ip;
				vmgrs[i].port=u->port;
				vmgrs[i].sid=u->sid;
			}
		}
	}

	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);

	//Fetch the player id
	int tmp2=0;
	plVaultFetchNodes(&tnodes,&tmp2,&id,1,&db);
	//DBG(5,"Fetch tnodes, address is:%i\n",&tnodes);

	//Update the online time
	tnodes->unk7+=online_time;
	time((time_t *)&tnodes->timestamp);
	plVaultUpdateNode(tnodes,&db);
	plVaultBcastNodeSaved(net,tnodes->index,(double)tnodes->timestamp);
	//Now update the Player Info Node
	init_node(&n);
	n.type=KPlayerInfoNode; // 23
	n.owner=id; //Player Owner
	info_node=plVaultFindNode(&n,&mfs,0,&db);
	if(info_node>0) {
		destroy_node(tnodes);
		free((void *)tnodes);
		tnodes=NULL;
		int tmp3=0;
		plVaultFetchNodes(&tnodes,&tmp3,(U32 *)&info_node,1,&db);
		//DBG(5,"Fetch tnodes, address is:%i\n",&tnodes);
		strcpy((char *)tnodes->entry_name,(char *)age);
		strcpy((char *)tnodes->sub_entry_name,(char *)guid);
		tnodes->torans=state;
		time((time_t *)&tnodes->timestamp);
		plVaultUpdateNode(tnodes,&db);
		plVaultBcastOnlineState(net,tnodes);
		//plVaultBcastNodeSaved(sock,tnodes->index,(double)tnodes->timestamp);
	}

	DBG(5,"Destroying node..\n");
	destroy_node(&n);
	if(tnodes!=NULL) {
		DBG(5,"Destroying tnodes...\n");
		for(i=0; i<n_tnodes; i++) {
			//DBG(6,"Destroying node %i located at %i\n",i,&tnodes[i]);
			destroy_node(&tnodes[i]);
		}
		DBG(5,"Calling free...\n");
		free((void *)tnodes);
	}
	DBG(5,"Ending Update Player status\n");
	return 0;
}

/*--------------------------------------------------------------------
  Creates a new player, and returns the KI
	returns -1 on db error
	returns 0 if player already exists
	returns KI if player has been succesfully created
--------------------------------------------------------------------*/
int plVaultCreatePlayer(st_unet * net,Byte * login,Byte * guid, Byte * avie,Byte * gender,Byte access_level) {
	int ret; //for store result codes
	int ki=-1;
	int ref,admin; //for store references

	int i;
	int n_nodes; //number of nodes
	t_vault_node * n=NULL;
	t_vault_manifest mfs; //a manifest

	n=(t_vault_node *)malloc(sizeof(t_vault_node) * 1);
	init_node(n);

	n->type=2; //player MGR node
	strcpy((char *)n->avie,(char *)avie);

	ki=plVaultFindNode(n,&mfs,0,&db); //returns id if founds a player

	destroy_node(n);

	if(ki<0) {
		plVaultInitVault();
		ki=0;
	}

	//ok do the really hard stuff here .. ->
	if(ki==0) {

		//ok a normal vault contains about 4 nodes
		n_nodes=4;
		n=(t_vault_node *)realloc((void *)n,sizeof(t_vault_node) * n_nodes);
		if(n==NULL) {
			print2log(f_err,"Memory error, now there is a leak!!! - in plVaultCreatePlayer\n");
			return -1;
		}
		for(i=0; i<n_nodes; i++) {
			init_node(&n[i]);
		}

		struct timeval tv;
		time_t tstamp;

		//--->find the allplayers node
		n[2].type=KFolderNode; //Folder 22
		n[2].torans=KAllPlayersFolder; //all players 12
		admin=plVaultFindNode(&n[2],&mfs,0,&db);
		if(admin==0) {
			plVaultInitVault(); //if not exists it will be created
			admin=plVaultFindNode(&n[2],&mfs,1,&db);
		}
		//<---

		time(&tstamp);
		gettimeofday(&tv,NULL);
		//first the player node
		n[0].type=KVNodeMgrPlayerNode; // 2
		n[0].permissions=0x17; //-r-rwr
		n[0].timestamp=tstamp;
		n[0].microseconds=tv.tv_usec;
		strcpy((char *)n[0].entry_name,(const char *)gender);
		strcpy((char *)n[0].avie,(const char *)avie);
		strcpy((char *)n[0].uid,(const char *)create_str_guid(guid));
		strcpy((char *)n[0].entry_value,(const char *)login);
		n[0].unk8=access_level;

		ki=plVaultCreateNode(&n[0],&db);

		if(ki>0) {
			gettimeofday(&tv,NULL);

			// Create PlayerInfo node
			n[1].type=KPlayerInfoNode; // 23
			n[1].permissions=0x17; //-r-rwr
			n[1].owner=ki;
			n[1].timestamp=tstamp;
			n[1].microseconds=tv.tv_usec;
			n[1].id2=ki;
			strcpy((char *)n[1].avie,(const char *)avie);
			ref=plVaultCreateNode(&n[1],&db);

			//plVaultCreateRef(0,ki,ref,tstamp,tv.tv_usec,0);
			// Create Player->PlayerInfo link
			plVaultCreateRef(ki,ki,ref,tstamp,tv.tv_usec,0,&db);

			//link with the admin node
			plVaultCreateRef(ki,admin,ref,tstamp,tv.tv_usec,0,&db);

			//do vault broadcast
			plVaultBcastNodeReferenceAdded(net,admin,ref);

			//---now all folders---
			//create reference to the unknown generic public inbox folder
			//plVaultCreateRef(ki,ki,20005,tstamp,tv.tv_usec,0);
			/*
			gettimeofday(&tv,NULL);

			//KFolderNode (ages i own folder)
			n[2].type=KFolderNode;	// 22
			n[2].torans=KAgesIOwnFolder; // 23
			n[2].permissions=0x17; //-r-rwr
			n[2].owner=ki;
			n[2].timestamp=tstamp;
			n[2].microseconds=tv.tv_usec;
			ref=plVaultCreateNode(&n[2]);

			//plVaultCreateRef(0,ki,ref,tstamp,tv.tv_usec,0);
			plVaultCreateRef(ki,ki,ref,tstamp,tv.tv_usec,0);

			gettimeofday(&tv,NULL);

			// Create BuddyList folder
			n[3].type=KPlayerInfoListNode;	// 30
			n[3].torans=KBuddyListFolder;		// 2
			n[3].permissions=0x17; //-r-rwr
			n[3].owner=ki;
			n[3].timestamp=tstamp;
			n[3].microseconds=tv.tv_usec;
			ref=plVaultCreateNode(&n[3]);

			plVaultCreateRef(0,ki,ref,tstamp,tv.tv_usec,0);
			*/

			//Search for the AgesIOwnFolder

			//Now link that player with Ae'gura and with the Hood
			n[3].type=KAgeInfoNode; // 33
			strcpy((char *)n[3].entry_name,"city");
			int city_id,hood_id;
			t_AgeInfoStruct ainfo;
			t_SpawnPoint spoint;
			memset(&ainfo,0,sizeof(t_AgeInfoStruct));
			memset(&spoint,0,sizeof(t_SpawnPoint));
			Byte a_guid[17];

			strcpy((char *)spoint.title,"FerryTerminal");
			strcpy((char *)spoint.name,"LinkInPointFerry");

			city_id=plVaultFindNode(&n[3],&mfs,0,&db);
			if(city_id<=0) {
				//then create it
				strcpy((char *)ainfo.filename,"city");
				strcpy((char *)ainfo.instance_name,"Ae'gura");
				strcpy((char *)ainfo.user_name,"Ae'gura");
				strcpy((char *)ainfo.display_name,"Ae'gura");
				generate_newguid(a_guid,(Byte *)"city",0);
				ascii2hex2(ainfo.guid,a_guid,8);
				city_id=plVaultCreateAge(&ainfo);
			}
			if(city_id>0) {
				//Add the linking point
				plVaultAddLinkingPoint(net,ki,city_id,&spoint);
			}

			strcpy((char *)n[3].entry_name,"Neighborhood");
			memset(&ainfo,0,sizeof(t_AgeInfoStruct));

			strcpy((char *)spoint.title,"Default");
			strcpy((char *)spoint.name,"LinkInPointDefault");

			hood_id=plVaultFindNode(&n[3],&mfs,0,&db);
			if(hood_id<=0) {
				//then create it
				strcpy((char *)ainfo.filename,"Neighborhood");
				strcpy((char *)ainfo.instance_name,"Neighborhood");
				strcpy((char *)ainfo.user_name,(char *)ghood_name);
				strcpy((char *)ainfo.display_name,(char *)ghood_desc);
				generate_newguid(a_guid,(Byte *)"Neighborhood",0);
				ascii2hex2(ainfo.guid,a_guid,8);
				hood_id=plVaultCreateAge(&ainfo);
			}
			if(hood_id>0) {
				//Add the linking point
				plVaultAddLinkingPoint(net,ki,hood_id,&spoint);
				plVaultAddOwnerToAge(net,hood_id,ki);
			}

			/******* EXPERIMENTAL BLOCK OF CODE */
			int finale_id;
			
			strcpy((char *)n[3].entry_name,"DniCityX2Finale");
			memset(&ainfo,0,sizeof(t_AgeInfoStruct));

			strcpy((char *)spoint.title,"Default");
			strcpy((char *)spoint.name,"LinkInPointDefault");

			finale_id=plVaultFindNode(&n[3],&mfs,0,&db);
			if(finale_id<=0) {
				//then create it
				strcpy((char *)ainfo.filename,"DniCityX2Finale");
				strcpy((char *)ainfo.instance_name,"DniCityX2Finale");
				strcpy((char *)ainfo.user_name,(char *)"");
				strcpy((char *)ainfo.display_name,(char *)"");
				generate_newguid(a_guid,(Byte *)"DniCityX2Finale",0);
				ascii2hex2(ainfo.guid,a_guid,8);
				finale_id=plVaultCreateAge(&ainfo);
			}
			if(finale_id>0) {
				//Add the linking point
				plVaultAddLinkingPoint(net,ki,finale_id,&spoint);
				//plVaultAddOwnerToAge(net,finale_id,ki);
			}
			/**** EXPERIMENTAL BLOCK OF CODE ENDS */
		}

		ret=ki; //o well let's go to test this initial stuff .. hummm!
		for(i=0; i<n_nodes; i++) {
			destroy_node(&n[i]);
		}
	} else {
		ret=0;
	}
	if(n!=NULL) {
		free((void *)n);
	}

	return ret;
}

/*--------------------------------------------------------------------
  Deletes a player
	returns -1 on db error
	returns 0 or >0 on success
--------------------------------------------------------------------*/
int plVaultDeletePlayer(st_unet * net,Byte * guid, U32 ki,Byte access_level) {
	int ret; //for store result codes

	int i;
	int n_nodes; //number of nodes
	t_vault_node * n=NULL;
	t_vault_node n2;
	t_vault_manifest mfs; //a manifest

	//n=(t_vault_node *)malloc(sizeof(t_vault_node) * 1);
	//init_node(n);
	int info;

	n_nodes=1;
	i=1;
	int tmp4=0;
	plVaultFetchNodes(&n,&tmp4,&ki,1,&db); //only one node to fetch
	//n->index;

	if((access_level<=AcAdmin || !strcmp((char *)create_str_guid(guid),(char *)n->uid)) && n->index==ki) {
		//now delete it
		//ret=plVaultRemoveNodeRef(0,ki,2); //force player deletion, deleting the infonodes
		//BEFORE - Find the Info Node
		init_node(&n2);
		n2.type=23; //Player Info Node
		n2.owner=ki; //The Owner of the Player info Node
		info=plVaultFindNode(&n2,&mfs,0,&db);
		destroy_node(&n2);

		int * table=NULL;
		int n_table=0;

		n_table=plVaultGetParentNodes(ki,&table,&db);
		for(i=0; i<n_table; i++) {
			plVaultBcastNodeReferenceRemoved(net,table[i],ki);
		}

		ret=plVaultRemoveNodeRef2(0,ki,1,1,&db); //force player deletion

		if(table!=NULL) { free((void *)table); }

		n_table=plVaultGetParentNodes(info,&table,&db);
		for(i=0; i<n_table; i++) {
			plVaultBcastNodeReferenceRemoved(net,table[i],info);
		}

		ret=plVaultRemoveNodeRef2(0,info,1,1,&db); //force all Player Info Node(s) deletion

		if(table!=NULL) { free((void *)table); }

	} else {
		ret=0;
	}

	for(i=0; i<n_nodes; i++) {
		destroy_node(&n[i]);
	}
	if(n!=NULL) {
		free((void *)n);
	}
	return ret;
}

//Delete age TODO
/*int plVaultDeleteAge() {

}*/


//we sent a struct (with the vault task stuff)
int plAdvVaultTaskParser(st_unet * net,t_vault_mos * obj,int sid) {
	st_uru_client * u=&net->s[sid];
	int i; //,e;
	int id,ret;
	int success=0;
	//Byte * age_name;
	//Byte age_guid[8];
	//Byte * wbuf;
	//Byte * buf;
	//t_CreatableStream * wcreat=NULL;
	//t_vault_node * wnode=NULL;
	int n_refs; //number of items
	t_vault_cross_ref * wref=NULL;
	t_AgeLinkStruct * wals=NULL;
	//int off;

	//int t_n; //number of items
	int * table; //----<> (remember to destroy it)
	table=NULL;
	//int node_parent=0;
	//int node_son=0;

	t_vault_node n; //--->remember to destroy it
	t_vault_manifest mfs;
	t_vault_mos robj; //---> remember to destroy it
	init_node(&n);
	plMainVaultInit(&robj); //set default on generated response ojbect

	t_vault_node * tnodes=NULL; //---> remember to destroy it
	int n_tnodes; //number of nodes

	//set the vmgr params
	robj.cmd=obj->cmd;
	robj.result=obj->result; //I need to play with this value
	robj.zlib=1; //uncompressed
	robj.ctx=obj->ctx; //[sub]
	//robj.res=obj->res; //Unused
	robj.mgr=obj->mgr; //[client id]
	//robj.vn=obj->vn; //Unused

	//all items that are suitable for a template will be stored in a template
	for(i=0; i<obj->n_itms; i++) {
		switch(obj->itm[i].id) {
			case 0x0B: //AgeLinkStruct
				wals=(t_AgeLinkStruct *)(obj->itm[i].data);
				break;
		}
	}

	int found;

	switch (obj->cmd) {
		case 0x0B: //RegisterVisitAge
		case 0x09: //RegisterOwnedAge
			if(wals==NULL) { break; }
			//1st Get the AgesIOwnFolder of the client
			n.type=KFolderNode; //0x16
			n.torans=0x17; //AgesIOwnFolder
			n.owner=robj.mgr; //Owner, the KI
			ret=plVaultFindNode(&n,&mfs,0,&db); //Now we have the Folder ID
			if(ret<=0) { //Not found, or db error
				//ret=plVaultFindNode(&n,&mfs,1);
				//then link it
				break;
			}
			//Now get all the tree of references
			wref=NULL;
			n_refs=plVaultGetCrossRef(ret,&wref,&db);
			if(n_refs>=0) {
				//2nd Search for the AgeNode
				destroy_node(&n);
				init_node(&n);
				n.type=KAgeInfoNode; //33
				n.torans=0;
				n.owner=0;
				hex2ascii2(n.guid,wals->ainfo.guid,8); //string4
				if(!strcmp((char *)n.guid,"0000000000000000") || strlen((char *)n.guid)!=16) {
					//create the guid here
					generate_newguid(n.guid,wals->ainfo.filename,0); //it should be players ki
					if(!strcmp((char *)n.guid,"0000000000000000") || strlen((char *)n.guid)!=16) {
						break; //do nothing
					}
					ascii2hex2(wals->ainfo.guid,n.guid,8);
				}
				id=plVaultFindNode(&n,&mfs,0,&db); //Now we have the age ID
				if(id<=0) { //Age Info node not found, then create it..
					id=plVaultCreateAge(&wals->ainfo);
					//search again
					//id=plVaultFindNode(&n,&mfs,0); //Now we have the age ID
					if(id<=0) {
						break; //Not found ????
					}
				}
				//Now search in the manifest the NODE
				found=0;
				for(i=0; i<n_refs; i++) {
					if(wref[i].id3==(U32)id) {
						//Found the node
						found=wref[i].id2; //The link node
					}
				}
				//Todo (look the code of the old vtask.cpp), you need to:
				// 1-Create the node if it doesn't exists, and broadcast the added references
				// 2-Fetch the old node, and update it, and then broadcast the SavedNode
				if(found!=0) { //we found the node
					//fetch the node
					n_tnodes=1;
					int tmp5=0;
					plVaultFetchNodes(&tnodes,&tmp5,(U32 *)&found,1,&db);
				} else { //node not found, create it
					n_tnodes=1;
					tnodes=(t_vault_node *)malloc(sizeof(t_vault_node) * 1);
					init_node(tnodes);
					tnodes[0].type=KAgeLinkNode; //0x1C 28
					tnodes[0].torans=1; //Locked status (0-unlocked,1-locked,elsewhere unknown)
					tnodes[0].distance=0; //Volatile status (0-non_volatile,1-volatile)
					tnodes[0].owner=robj.mgr; //Owner, the KI client id
				}

				//Init the node data
				if(tnodes->data==NULL) {
					tnodes->data=(Byte *)malloc(sizeof(Byte) * (strlen((char *)wals->spoint.title)\
+ strlen((char *)wals->spoint.name) + strlen((char *)wals->spoint.camera)+5));
					strcpy((char *)tnodes->data,""); //arfgh!
				} else {
					tnodes->data=(Byte *)realloc((void *)tnodes->data,sizeof(Byte) * \
(strlen((char *)wals->spoint.title) + strlen((char *)wals->spoint.name)\
 + strlen((char *)wals->spoint.camera)+5+strlen((char *)tnodes->data)));
				}

				//set linking point info
				strcat((char *)tnodes->data,(char *)wals->spoint.title); //Link Title
				strcat((char *)tnodes->data,":");
				strcat((char *)tnodes->data,(char *)wals->spoint.name); //Link Name
				strcat((char *)tnodes->data,":");
				strcat((char *)tnodes->data,(char *)wals->spoint.camera); //Camera stack
				strcat((char *)tnodes->data,";");
				tnodes->data_size=strlen((char *)tnodes->data); //Update size of data

				if(found!=0) { //we found the node
					plVaultUpdateNode(tnodes,&db); //update the node
					double stamp;
					time((time_t *)&stamp);
					plVaultBcastNodeSaved(net,tnodes->index,stamp);
				} else { //node not found
					//time((time_t *)&tnodes->timestamp); //unusefull
					found=plVaultCreateNode(tnodes,&db);
					//Link the link node with the Age Info Node
					plVaultCreateRef(robj.mgr,found,id,0,0,0,&db);
					//Link the AgesIOwnFolder with the Link Node
					plVaultCreateRef(robj.mgr,ret,found,0,0,0,&db);
					//Send a Vault Notification
					plVaultBcastNodeReferenceAdded(net,ret,found);
				}
				//Find the player info node & and add it to the age
				plVaultAddOwnerToAge(net,id,robj.mgr);

				//Now generate the vault task response
				robj.itm=vaultCreateItems(1);
				robj.n_itms=1;
				robj.itm[0].id=0x01; //The Id of the Link Node
				plVItmPutInteger(&robj.itm[0],found);
				u->hmsg.cmd=NetMsgVaultTask;
				htmlVaultParse(net,&robj,sid,0);
				DBG(5,"Sending vault task\n");
				plNetMsgVaultTask(net,&robj,sid);
				DBG(5,"Vault task send\n");
			}
			if(wref!=NULL) { free((void *)wref); }
			break;
		default:
			print2log(f_vmgr,"Unkwon non-implemented vaulttask request code %02X\n",obj->cmd);
			//return -1;
	} //end switch

	destroy_node(&n);
	if(tnodes!=NULL) {
		for(i=0; i<n_tnodes; i++) {
			DBG(5,"Destroying node %i of %i\n",i,n_tnodes);
			destroy_node(&tnodes[i]);
		}
		free((void *)tnodes);
	}
	plMainVaultDestroy(&robj,0);
	if(table!=NULL) {
		free((void *)table);
	}

	if(success<0) {
		print2log(f_vmgr,"Failed parsing vault stream!\n");
	}
	DBG(5,"RETURNING from vault_task parser with value %i\n",success);
	return success;
}



#if 0
int parse_vault_task(Byte * buf,int n,int sock,st_uru_client * u) {

	t_vault_mos vobj; //the vault object
	t_vault_object item; //a vault item
	int off=0;

	vobj.ki=u->msg.ki;
	vobj.code=*(Byte *)(buf+off);
	off++;
	vobj.unk1=*(U16 *)(buf+off);
	off+=2;
	vobj.format=*(Byte *)(buf+off);
	off++;
	vobj.real_size=*(U32 *)(buf+off);
	off+=4;

	if(vobj.ki!=(U32)u->ki) {
		plog(f_vtask,"UNE: Ki mismatch %i vs %i\n",u->ki,vobj.ki);
		return -1;
	}
	if(vobj.code!=0x09) {
		plog(f_vtask,"UNE: Unknown vault task request %i\n",vobj.code);
		return -1;
	}
	if(vobj.unk1!=0) {
		plog(f_vtask,"UNE: Unexpected non-zero data at unk1 %i\n",vobj.unk1);
		return -1;
	}
	if(vobj.format!=1) {
		plog(f_vtask,"UNE: Unexpected Vault Task data format %i\n",vobj.format);
		return -1;
	}

	vobj.n_itms=*(U16 *)(buf+off);
	off+=2;

	if(vobj.n_itms!=1) {
		plog(f_vtask,"UNE: Unexpected Number of items %i\n",vobj.n_itms);
		return -1;
	}

	//the item
	vobj.itm=&item;

	//now parse the item
	item.format_flag=*(Byte *)(buf+off);
	off++;

	if(item.format_flag!=0x0B) {
		plog(f_vtask,"UNE: unk Format flag %i!=0x0B\n",item.format_flag);
		return -1;
	}

	item.unk1=*(Byte *)(buf+off);
	off++;

	if(item.unk1!=0) {
		plog(f_vtask,"UNE: unexpected non-zero item.unk1 %i\n",item.unk1);
		return -1;
	}

	item.cmd=*(U16 *)(buf+off);
	off+=2;

	if(item.cmd!=0x02BF) { //plAgeLinkStruct
		plog(f_vtask,"UNE: Unimplemented vault cmd %04X\n",item.cmd);
		return -1;
	}

	//this code should search for the Vault Node that contains the linking points
	//folder, search for the specific node for that age, and add the linking point
	//to the list of known linking points
	//then the server should create, and notify the client for the different
	//node references that will be added

	//parse the plAgeLinkStruct stuff here
	plAgeLinkStruct link;

	link.unk1=*(U16 *)(buf+off);
	off+=2;
	if(link.unk1!=0x0023) {
		plog(f_vtask,"UNE: invalid link.unk1 value %04X\n",link.unk1);
		return -1;
	}

	link.mask=*(Byte *)(buf+off);
	off++;
	if(link.mask!=0x2F && link.mask!=0x0F) {
		plog(f_vtask,"UNE: Invalid mask! %02X\n",link.mask);
		return -1;
	}

	off+=decode_urustring(link.age_file_name,buf+off,STR_MAX_SIZE);
	off+=2;
	off+=decode_urustring(link.age_name,buf+off,STR_MAX_SIZE);
	off+=2;

	memcpy(link.age_guid,buf+off,8);
	off+=8;

	off+=decode_urustring(link.owner_name,buf+off,STR_MAX_SIZE);
	off+=2;

	if(link.mask==0x2F) {
		off+=decode_urustring(link.full_name,buf+off,STR_MAX_SIZE);
		off+=2;
	} else {
		strcpy((char *)link.full_name,"");
	}

	link.unk2=*(Byte *)(buf+off);
	off+=1;
	link.unk3=*(U32 *)(buf+off);
	off+=4;
	link.unk4=*(U32 *)(buf+off);
	off+=4;

	off+=decode_urustring(link.link_name,buf+off,STR_MAX_SIZE);
	off+=2;
	off+=decode_urustring(link.link_point,buf+off,STR_MAX_SIZE);
	off+=2;

	link.unk4=*(U16 *)(buf+off);
	off+=2;

	//2n object
	vobj.fcode=*(Byte *)(buf+off);
	off++;
	vobj.fki=*(U32 *)(buf+off);
	off+=4;

	if(n!=off) {
		plog(f_vtask,"UNE: Size mismatch %i vs %i\n",off,n);
	}

	if(vobj.fcode!=0) {
		plog(f_vtask,"UNE: non-zero object2 code %i\n",vobj.fcode);
		return -1;
	}
	if(vobj.fki!=vobj.ki) {
		plog(f_vtask,"UNE: Ki mismatch %08Xvx%08X\n",vobj.fki,vobj.ki);
		return -1;
	}
	//all went ok
	int ret_code,i,end;
	t_vault_node node,age_node;
	t_vault_manifest mfs,age_mfs;

	t_vault_cross_ref * cross_ref;
	t_vault_node * tnodes;

	cross_ref=NULL;
	tnodes=NULL;

	//node id
	U32 node_id_ret=0; //store the node id that we are going to send to the client


	init_node(&node);
	init_node(&age_node);

	//query the vault for the AgesIOwnFolder 0x17 node index
	node.type=KFolderNode; //0x16
	node.torans=0x17; //AgesIOwnFolder
	node.owner=(U32)u->ki; //the KVNodeMgrPlayerNode index, the ki

	ret_code=plVaultFindNode(&node,&mfs);
	if(ret_code<0) {
		plog(f_vtask,"DB error\n");
		return -1;
	}

	plog(f_vtask,"plVaultFindNode returned %i for AgesIOwnFolder node\n",mfs.index);

	//query the vault for the manifest (cross ref only) of that node
	end=plVaultGetCrossRef(mfs.index,&cross_ref);

	if(end<0) {
		plog(f_vtask,"DB error\n");
		return -1;
	}

	//query the vault for the AgeInfoNode of the current age
	age_node.type=KAgeInfoNode; //0x21 33
#ifdef I_AM_A_GAME_SERVER
	strcpy((char *)age_node.guid,(char *)global_age.guid);
#else
	strcpy((char *)age_node.guid,"0000000000000000");
	//bzero(age_node.guid,8);
#endif

	ret_code=plVaultFindNode(&age_node,&age_mfs);
	if(ret_code<0) {
		plog(f_vtask,"DB error\n");
		ret_code=-1;
	} else {

		plog(f_vtask,"plVaultFindNode returned %i for AgeInfoNode node\n",age_mfs.index);

		//check if the manifest contains that node
		plog(f_vtask,"Checking if the manifest contains node %i\n",age_mfs.index);

		ret_code=0;

		for(i=0; i<end; i++) {
			if(cross_ref[i].id3==age_mfs.index) {
				ret_code=1; //found
				plog(f_vtask,"Node %i succesfully found in the manifest!\n",age_mfs.index);
				break;
			}
		}

		struct timeval tv;
		time_t tstamp;

		time(&tstamp);
		gettimeofday(&tv,NULL); //tv.tv_usec


		//if true add the link to the linkNode that points to it
		if(ret_code==1) { //found
			//fetch the node
			U32 node_table;
			node_table=cross_ref[i].id2; //the parent
			node_id_ret=node_table;
			plVaultFetchNodes(&tnodes,1,&node_table);

			if(tnodes->data==NULL) {
				tnodes->data=(Byte *)malloc((sizeof(Byte) * (strlen((const char *)link.link_name)+strlen((const char *)link.link_point)+4)));
				strcpy((char *)tnodes->data,"");
			}
			strcat((char *)tnodes->data,(char *)link.link_name);
			strcat((char *)tnodes->data,":");
			strcat((char *)tnodes->data,(char *)link.link_point);
			strcat((char *)tnodes->data,":;");
			tnodes->data_size=strlen((char *)tnodes->data);

			tnodes->timestamp3=tnodes->timestamp2;
			tnodes->microseconds3=tnodes->microseconds2;
			tnodes->timestamp2=tnodes->timestamp;
			tnodes->microseconds2=tnodes->microseconds;
			tnodes->timestamp=tstamp;
			tnodes->microseconds=tv.tv_usec;


			//now update the node
			plVaultUpdateNode(tnodes);

			destroy_node(tnodes);


		} else { //not found
			//if not create a linkNode, and link the AgeInfo node to that link, and add the link
			//create a linkNode
			t_vault_node tlinkNode;
			t_vault_manifest tlinkmfs;

			init_node(&tlinkNode);

			tlinkNode.type=KAgeLinkNode; //0x1C 28
			tlinkNode.torans=1; //?
			tlinkNode.owner=(U32)u->ki;
			tlinkNode.data=(Byte *)malloc((sizeof(Byte) * (strlen((char *)link.link_name)+strlen((char *)link.link_point)+4)));
			strcpy((char *)tlinkNode.data,(char *)link.link_name);
			strcat((char *)tlinkNode.data,":");
			strcat((char *)tlinkNode.data,(char *)link.link_point);
			strcat((char *)tlinkNode.data,":;");
			tlinkNode.data_size=strlen((char *)tlinkNode.data);

			tlinkmfs.index=plVaultCreateNode(&tlinkNode);

			node_id_ret=tlinkmfs.index; //the node index that we are going to return

			destroy_node(&tlinkNode);

			//we need to link the AgesIOwnFolder to the LinkNode
			plVaultCreateRef(0,mfs.index,node_id_ret,tstamp,tv.tv_usec,0);
			//we need to send a notification to the client from the server side!
			plVaultCreateRefNotification(0,mfs.index,node_id_ret,0,0,0,sock,u);

			//then we need to link the LinkNode with the KAgeInfoNode
			plVaultCreateRef(0,node_id_ret,age_mfs.index,tstamp,tv.tv_usec,0);

		}
		ret_code=0;

	}
	//destroy nodes
	destroy_node(&age_node);
	destroy_node(&node);

	if(cross_ref!=NULL) {
		free((void *)cross_ref);
	}
	if(tnodes!=NULL) {
		free((void *)tnodes);
	}

	if(ret_code<0) { return -1; }
	//now send all the stuff here

	Byte bufeta[OUT_BUFFER_SIZE]; //the data buffer
	int dsize=0; //the data size

	*(Byte *)(bufeta+dsize)=0x09; //KRegisterOwnedAge
	dsize++;
	*(U16 *)(bufeta+dsize)=0x00; //always
	dsize+=2;
	*(Byte *)(bufeta+dsize)=0x01; //uncompressed
	dsize++;
	*(U32 *)(bufeta+dsize)=0x0B; //11 bytes
	dsize+=4;
	*(Byte *)(bufeta+dsize)=0x01; //one object
	dsize++;
	*(Byte *)(bufeta+dsize)=0x00; //unk always
	dsize++;
	*(Byte *)(bufeta+dsize)=0x01; //integer - main type code
	dsize++;
	*(Byte *)(bufeta+dsize)=0x00; //unk always
	dsize++;
	*(U16 *)(bufeta+dsize)=0x0387; //vault cmd
	dsize+=2;
	*(Byte *)(bufeta+dsize)=0x00; //integer
	dsize++;
	*(U32 *)(bufeta+dsize)=node_id_ret; //the node index
	dsize+=4;
	*(Byte *)(bufeta+dsize)=0x00; //ret cmd (ctx?)
	dsize++;
	*(U32 *)(bufeta+dsize)=u->ki; //the ki
	dsize+=4;

	ret_code=plNetMsgVaultTask(sock,u,bufeta,dsize);

	return ret_code;
}
#endif

#endif
