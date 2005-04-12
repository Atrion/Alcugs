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

#ifndef __VAULT_ADVP_S
#define __VAULT_ADVP_S

#define __VAULT_ADVP_S_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#ifndef __MSVC__
#  include <sys/time.h>
#endif

#include "data_types.h"
#include "stdebug.h"
#include "debug.h"
#include "conv_funs.h"
#include "protocol.h"
#include "urunet.h"
#include "vaultstrs.h"

#include "vserversys.h"
#include "vaultsubsys.h"
#include "urustructs.h"
//#include "urumsg.h"

#include "gbasicmsg.h"
#include "gvaultmsg.h"

#include "prot.h"

#include "vaultstrs.h"
#include "vnodes.h"
#include "vault_obj.h"
#include "vault_db.h"
#include "guid_gen.h" //to generate unique id's
#include "htmldumper.h"

#include "uru.h"

#include "vault_advp.h"

#include "debug.h"

st_vault_mgrs * vmgrs=NULL;
int n_vmgrs=0;

void init_vmgr_data(st_vault_mgrs ** t) {
	*t=(st_vault_mgrs *)malloc(sizeof(st_vault_mgrs) * 1);
	memset(*t,0,sizeof(st_vault_mgrs));
}

int check_me_in(int id,int node,st_uru_client * u) {
	int i;
	for(i=0; i<n_vmgrs; i++) {
		DBG(5,"id:%i vs %i node:%i vs %i\n",vmgrs[i].id,id,vmgrs[i].node,node);
		if(vmgrs[i].id==id && vmgrs[i].node==node) {
			vmgrs[i].ip=u->ip;
			vmgrs[i].port=u->port;
			vmgrs[i].sid=u->sid;
			return 1;
		}
	}
	//abort();
	return 0;
}

int plVaultBcastNodeReferenceAdded(st_unet * net,int father,int son) {
	t_vault_mos robj; //---> remember to destroy it
	plMainVaultInit(&robj); //set default on generated response ojbect
	int i,e,t_n;
	int * table=NULL; //destroy it
	t_vault_cross_ref * wref;
	robj.cmd=0x03; //AddVaultNodeRef
	robj.result=0; //I need to play with this value
	robj.zlib=1; //uncompressed
	robj.ctx=0;
	robj.res=0;
	robj.mgr=0;
	robj.vn=0;

	robj.itm=vaultCreateItems(1);
	robj.n_itms=1;
	robj.itm[0].id=0x07;
	robj.itm[0].dtype=DVaultNodeRef;
	robj.itm[0].data=malloc(sizeof(t_vault_cross_ref) * 1);
	wref=(t_vault_cross_ref *)robj.itm[0].data;

	memset(wref,0,sizeof(t_vault_cross_ref));
	wref->id2=father;
	wref->id3=son;

	t_n=plVaultGetMGRS(wref->id2,&table,&db);

	if(t_n>0) {
		//And now broadcast the message to all clients
		for(i=0; i<n_vmgrs; i++) {
			DBG(5,"for(i=0; i<n_vmgrs; i++) i:%i,n_vmgrs:%i\n",i,n_vmgrs);
			if(vmgrs[i].node!=0) {
				for(e=0; e<t_n; e++) {
					DBG(5,"for(e=0; e<t_n; e++) e:%i,t_n:%i\n",e,t_n);
					if((int)vmgrs[i].sid<(int)net->n && vmgrs[i].node==table[e] && vmgrs[i].ip==net->s[vmgrs[i].sid].ip && vmgrs[i].port==net->s[vmgrs[i].sid].port) {
						//sent the broadcast message to that client
						DBG(5,"Vault Broadcast to %i..\n",vmgrs[i].id);
						net->s[vmgrs[i].sid].hmsg.ki=vmgrs[i].id;
						robj.mgr=vmgrs[i].node;
						htmlVaultParse(net,&robj,vmgrs[i].sid,0);
						plNetMsgVault(net,&robj,vmgrs[i].sid);
						break;
					}
				}
			}
		}
	}

	if(table!=NULL) {
		free((void *)table);
	}
	plMainVaultDestroy(&robj,0);

	DBG(6,"End Vault Broadcast...\n");
	return 0;
}


int plVaultBcastNodeReferenceRemoved(st_unet * net,int father,int son) {
	t_vault_mos robj; //---> remember to destroy it
	plMainVaultInit(&robj); //set default on generated response ojbect
	int i,e,t_n;
	int * table=NULL; //destroy it
	t_vault_cross_ref * wref;
	robj.cmd=0x04; //RemoveVaultNodeRef
	robj.result=0; //I need to play with this value
	robj.zlib=1; //uncompressed
	robj.ctx=0;
	robj.res=0;
	robj.mgr=0;
	robj.vn=0;

	robj.itm=vaultCreateItems(1);
	robj.n_itms=1;
	robj.itm[0].id=0x07;
	robj.itm[0].dtype=DVaultNodeRef;
	robj.itm[0].data=malloc(sizeof(t_vault_cross_ref) * 1);
	wref=(t_vault_cross_ref *)robj.itm[0].data;

	memset(wref,0,sizeof(t_vault_cross_ref));
	wref->id2=father;
	wref->id3=son;

	t_n=plVaultGetMGRS(wref->id2,&table,&db);

	if(t_n>0) {
		//And now broadcast the message to all clients
		for(i=0; i<n_vmgrs; i++) {
			DBG(5,"for(i=0; i<n_vmgrs; i++) i:%i,n_vmgrs:%i\n",i,n_vmgrs);
			if(vmgrs[i].node!=0) {
				for(e=0; e<t_n; e++) {
					DBG(5,"for(e=0; e<t_n; e++) e:%i,t_n:%i\n",e,t_n);
					if((int)vmgrs[i].sid<(int)net->n && vmgrs[i].node==table[e] && vmgrs[i].ip==net->s[vmgrs[i].sid].ip && vmgrs[i].port==net->s[vmgrs[i].sid].port) {
						//sent the broadcast message to that client
						DBG(5,"Vault Broadcast to %i..\n",vmgrs[i].id);
						net->s[vmgrs[i].sid].hmsg.ki=vmgrs[i].id;
						robj.mgr=vmgrs[i].node;
						htmlVaultParse(net,&robj,vmgrs[i].sid,0);
						plNetMsgVault(net,&robj,vmgrs[i].sid);
						break;
					}
				}
			}
		}
	}

	if(table!=NULL) {
		free((void *)table);
	}
	plMainVaultDestroy(&robj,0);

	DBG(6,"End Vault Broadcast...\n");
	return 0;
}


int plVaultBcastNodeSaved(st_unet * net,int index,double timestamp) {
	t_vault_mos robj; //---> remember to destroy it
	plMainVaultInit(&robj); //set default on generated response ojbect
	int i,e,t_n;
	int * table=NULL; //destroy it
	robj.cmd=0x06; //SaveNode
	robj.result=0; //I need to play with this value
	robj.zlib=1; //uncompressed
	robj.ctx=0;
	robj.res=0;
	robj.mgr=0;
	robj.vn=0;

	robj.itm=vaultCreateItems(2);
	robj.n_itms=2;
	robj.itm[0].id=0x09; //The saved (old index) node
	plVItmPutInteger(&robj.itm[0],index);

	//store timestamp
	plVItmPutTimestamp(&robj.itm[1],timestamp);
	robj.itm[1].id=0x18; //Timestamp

	t_n=plVaultGetMGRS(index,&table,&db); //Get the managers

	if(t_n>0) {
		//And now broadcast the message to all clients
		for(i=0; i<n_vmgrs; i++) {
			DBG(5,"for(i=0; i<n_vmgrs; i++) i:%i,n_vmgrs:%i\n",i,n_vmgrs);
			if(vmgrs[i].node!=0) {
				for(e=0; e<t_n; e++) {
					DBG(5,"for(e=0; e<t_n; e++) e:%i,t_n:%i\n",e,t_n);
					if((int)vmgrs[i].sid<(int)net->n && vmgrs[i].node==table[e] && vmgrs[i].ip==net->s[vmgrs[i].sid].ip && vmgrs[i].port==net->s[vmgrs[i].sid].port) {
						//sent the broadcast message to that client
						DBG(5,"Vault Broadcast to %i..\n",vmgrs[i].id);
						net->s[vmgrs[i].sid].hmsg.ki=vmgrs[i].id;
						robj.mgr=vmgrs[i].node;
						htmlVaultParse(net,&robj,vmgrs[i].sid,0);
						plNetMsgVault(net,&robj,vmgrs[i].sid);
						break;
					}
				}
			}
		}
	}

	if(table!=NULL) {
		free((void *)table);
	}
	plMainVaultDestroy(&robj,0);

	DBG(6,"End Vault Broadcast...\n");
	return 0;
}

int plVaultBcastOnlineState(st_unet * net,t_vault_node * n) {
	t_vault_mos robj; //---> remember to destroy it
	plMainVaultInit(&robj); //set default on generated response ojbect
	int i,e,t_n;
	int * table=NULL; //destroy it
	robj.cmd=0x0B; //OnlineState
	robj.result=0; //I need to play with this value
	robj.zlib=1; //uncompressed
	robj.ctx=0;
	robj.res=0;
	robj.mgr=0;
	robj.vn=0;

	if(n->torans==1) {
		robj.itm=vaultCreateItems(5);
		robj.n_itms=5;
	} else {
		robj.itm=vaultCreateItems(3);
		robj.n_itms=3;
	}

	robj.itm[0].id=0x09; //The saved (old index) node
	plVItmPutInteger(&robj.itm[0],n->index);

	//store timestamp
	plVItmPutTimestamp(&robj.itm[1],(double)n->timestamp);
	robj.itm[1].id=0x18; //Timestamp


	int nid=2;
	if(n->torans==1) {
		plVItmPutString(&robj.itm[nid],n->entry_name);
		robj.itm[nid].id=0x1B; //Age Name (Entry_Name)
		nid++;
		plVItmPutString(&robj.itm[nid],n->sub_entry_name);
		robj.itm[nid].id=0x1C; //Age GUID (sub_Entry_Name)
		nid++;
	}

	//The online state
	robj.itm[nid].id=0x1D; //The saved (old index) node
	plVItmPutInteger(&robj.itm[nid],n->torans);


	t_n=plVaultGetMGRS(n->index,&table,&db); //Get the managers

	if(t_n>0) {
		//And now broadcast the message to all clients
		for(i=0; i<n_vmgrs; i++) {
			DBG(5,"for(i=0; i<n_vmgrs; i++) i:%i,n_vmgrs:%i\n",i,n_vmgrs);
			if(vmgrs[i].node!=0) {
				for(e=0; e<t_n; e++) {
					DBG(5,"for(e=0; e<t_n; e++) e:%i,t_n:%i\n",e,t_n);
					if(vmgrs[i].sid<(int)net->n && vmgrs[i].node==table[e] && vmgrs[i].ip==net->s[vmgrs[i].sid].ip && vmgrs[i].port==net->s[vmgrs[i].sid].port) {
						//sent the broadcast message to that client
						DBG(5,"Vault Broadcast to %i..\n",vmgrs[i].id);
						net->s[vmgrs[i].sid].hmsg.ki=vmgrs[i].id;
						robj.mgr=vmgrs[i].node;
						htmlVaultParse(net,&robj,vmgrs[i].sid,0);
						plNetMsgVault(net,&robj,vmgrs[i].sid);
						break;
					}
				}
			}
		}
	}

	if(table!=NULL) {
		free((void *)table);
	}
	plMainVaultDestroy(&robj,0);

	DBG(6,"End Vault Broadcast...\n");
	return 0;
}


//we sent a struct (with the vault stuff)
int plAdvVaultParser(st_unet * net,t_vault_mos * obj,int sid) {
	st_uru_client * u=&net->s[sid];
	int i,e;
	int id,ret;
	int success=0;
	Byte * age_name=NULL;
	Byte age_guid[8];
	Byte * wbuf=NULL;
	Byte * buf=NULL;
	t_CreatableStream * wcreat=NULL;
	t_vault_node * wnode=NULL;
	t_vault_cross_ref * wref=NULL;
	int off;

	int t_n; //number of items
	int * table=NULL; //----<> (remember to destroy it)
	table=NULL;
	int node_parent=0;
	int node_son=0;
	int seen_flag=0; //for setseen
	int rcv_player=0; //For send node, the reciever id

	t_vault_node n; //--->remember to destroy it
	t_vault_manifest mfs;
	t_vault_mos robj; //---> remember to destroy it
	init_node(&n);
	plMainVaultInit(&robj); //set default on generated response ojbect

	//set the vmgr params
	robj.cmd=obj->cmd;
	robj.result=obj->result; //I need to play with this value
	robj.zlib=1; //uncompressed
	robj.ctx=obj->ctx;
	robj.res=obj->res;
	robj.mgr=obj->mgr;
	robj.vn=obj->vn;

#if 0
			//TODO
			case 0x1B: //inverted ustring16
			case 0x1C: //inverted ustring16
			case 0x1D: //another integer
			case 0x1F: //another integer?
#endif

	//all items that are suitable for a template will be stored in a template
	for(i=0; i<obj->n_itms; i++) {
		switch(obj->itm[i].id) {
			case 0: //undocumented (used in VaultManager)
				//check the reported value
				if(plVItmGetInteger(&obj->itm[i])!=(S32)0xC0AB3041) {
					plog(f_vmgr,"Got an invalid id:0 type, get %08X expected %08X!\n",plVItmGetInteger(&obj->itm[i]),0xC0AB3041);
				}
				break;
			case 1: //Node Type
				n.type=plVItmGetInteger(&obj->itm[i]);
				break;
			case 2: //Unique Id [Ki number]
				id=plVItmGetInteger(&obj->itm[i]);
				break;
			case 4:
				rcv_player=plVItmGetInteger(&obj->itm[i]);
				break;
			case 5: //A single vault node
				wnode=(t_vault_node *)(obj->itm[i].data);
				break;
			case 6: //A stream of Vault Nodes 0x06
				//Not for a server?
				break;
			case 7: //A single vault node ref
				wref=(t_vault_cross_ref *)(obj->itm[i].data);
				break;
			case 9: //FoundNode Index / Son of a NodeRef / Old Node Index (savenode)
				node_son=plVItmGetInteger(&obj->itm[i]);
				break;
			case 10: //List of integers 0x0A
				off=0;
				wcreat=(t_CreatableStream *)(obj->itm[i].data);
				wbuf=(Byte *)(wcreat->data);
				t_n=*(U16 *)(wbuf+off);
				off+=2;
				table=(int *)malloc(sizeof(int) * t_n);
				for(e=0; e<t_n; e++) {
					table[e]=*(U32 *)(wbuf+off);
					off+=4;
				}
				break;
			case 11: //New Node Index (saveNode) 0x0B
				//not for a server
				break;
			case 13: //Parent of a NodeRef 0x0D
				node_parent=plVItmGetInteger(&obj->itm[i]);
				break;
			case 14: //Manifest - 0x0E
			case 15: //Crossref - 0x0F
				//Not for a server?
				break;
			case 16: //An integer value seen in FindNode - 0x10
				if(plVItmGetInteger(&obj->itm[i])!=0 && plVItmGetInteger(&obj->itm[i])!=1) {
					plog(f_vmgr,"Got an invalid id:16 type, get %08X expected %08X or %08X!\n",plVItmGetInteger(&obj->itm[i]),0,1);
				}
				break;
			case 19: //Set Seen flag
				seen_flag=plVItmGetInteger(&obj->itm[i]);
				break;
			case 20: //A integer with a code 0x14
				//check it
				if(plVItmGetInteger(&obj->itm[i])!=-1) {
					plog(f_vmgr,"Got an invalid id:20 type, get %08X expected %08X!\n",plVItmGetInteger(&obj->itm[i]),-1);
				}
				break;
			case 21: //Age Name 0x15
				age_name=plVItmGetString(&obj->itm[i]);
				break;
			case 22: //Age Guid 0x16
				memcpy(age_guid,(Byte *)obj->itm[i].data,8);
				hex2ascii2(n.entry_name,age_guid,8);
				break;
			case 23: //Vault folder 0x17
			case 24: //Timestamp (double format) 0x18
			case 25: //number of vault nodes 0x19
				//Not for a Server?
				break;
		}
	}

	int connected=0;

	if(check_me_in(u->ki,obj->mgr,u)==1 || obj->cmd==VConnect) { //do the big switch

		switch (obj->cmd) {
			//****************************** CONNECT ******************************
			case VConnect: //Connect 0x01
				print2log(f_vmgr,"Vault Connect request for %i\n",u->ki);
				DBG(4,"n.type is %i\n",n.type);
				if(n.type==2) { //player node
					DBG(4,"player node\n");
					if(id!=u->ki) {
						DBG(4,"Invalid ki, disconnecting this client...");
						print2log(f_vmgr,"Requested Vault Connect for an invalid player id, got %i, expected %i.\n",id,u->ki);
						plNetMsgPlayerTerminated(net,RKickedOff,sid);
					} else {
						DBG(4,"Good ki, set answer\n");
						ret=id;
						DBG(4,"VaultCreateItems\n");
						robj.itm=vaultCreateItems(2);
						robj.n_itms=2;
						DBG(4,"PutInteger\n");
						plVItmPutInteger(&robj.itm[0],ret);
						robj.itm[0].id=2; //ki number (vmgr)
						DBG(4,"PutString\n");
						plVItmPutString(&robj.itm[1],(Byte *)global_vault_folder_name);
						robj.itm[1].id=23; //0x17 - vault folder
						DBG(4,"Calling Html parser...\n");
						htmlVaultParse(net,&robj,sid,0);
						DBG(4,"Sending the message...\n");
						plNetMsgVault(net,&robj,sid); //send the update
						connected=1;
					}
				} else if(n.type==3) { //age node
					ret=plVaultFindNode(&n,&mfs,1,&db);
					robj.itm=vaultCreateItems(3);
					robj.n_itms=3;
					plVItmPutInteger(&robj.itm[0],ret);
					robj.itm[0].id=2; //ki number (vmgr)
					plVItmPutString(&robj.itm[1],age_name);
					robj.itm[1].id=21; //0x17 - Age Name
					plVItmPutString(&robj.itm[2],(Byte *)global_vault_folder_name);
					robj.itm[2].id=23; //0x17 - vault folder
					htmlVaultParse(net,&robj,sid,0);
					plNetMsgVault(net,&robj,sid); //send the update
					connected=1;
				} else if(n.type==5) { //admin node
					ret=plVaultFindNode(&n,&mfs,1,&db);
					robj.itm=vaultCreateItems(3);
					robj.n_itms=3;
					plVItmPutInteger(&robj.itm[0],n.type);
					robj.itm[0].id=1; //node type
					plVItmPutInteger(&robj.itm[1],ret);
					robj.itm[1].id=2; //node VMGR ID
					plVItmPutString(&robj.itm[2],(Byte *)global_vault_folder_name);
					robj.itm[2].id=23; //0x17 - vault folder
					htmlVaultParse(net,&robj,sid,0);
					plNetMsgVault(net,&robj,sid); //send the update
					connected=1;
				} else {
					//ret=plVaultFindNode(&n,&mfs,0);
					plog(f_vmgr,"Unknown VConnect MGR type!\n");
				}
				DBG(4,"End VConnect\n");
				if(connected==1) {
					if(vmgrs==NULL && n_vmgrs==0) {
						n_vmgrs=1;
						init_vmgr_data(&vmgrs);
						vmgrs[0].id=u->ki;
						vmgrs[0].node=ret;
						vmgrs[0].ip=u->ip;
						vmgrs[0].port=u->port;
						vmgrs[0].sid=u->sid;
					} else if(vmgrs!=NULL) {
						int mt=-1;
						for(i=0; i<n_vmgrs; i++) {
							if(vmgrs[i].id==u->ki && vmgrs[i].node==ret) { mt=i; break; }
						}
						if(mt==-1) { //not found (find a free slot)
							for(i=0; i<n_vmgrs; i++) {
								if(vmgrs[i].node==0) {
									mt=i;
									break;
								}
							}
						}
						if(mt==-1) { //not found (create a new one)
							n_vmgrs++;
							vmgrs=(st_vault_mgrs *)realloc((void *)vmgrs,sizeof(st_vault_mgrs) * n_vmgrs);
							mt=n_vmgrs-1;
						}
						vmgrs[mt].id=u->ki;
						vmgrs[mt].node=ret;
						vmgrs[mt].ip=u->ip;
						vmgrs[mt].port=u->port;
						vmgrs[mt].sid=u->sid;
					} else {
						plog(f_err,"VMGRS ERROR!!: This should not be happening, something terribly has gone wrong, head for the cover!\n");
					}
				}
				break;
			//****************************** Disconnect ******************************
			case VDisconnect: //0x02 //disconnect
				print2log(f_vmgr,"Vault Disconnect for %i\n",u->ki);
				robj.n_itms=0;
				htmlVaultParse(net,&robj,sid,0);
				plNetMsgVault(net,&robj,sid);
				if(n_vmgrs<=0) break;
				for(i=0; i<n_vmgrs; i++) {
					if(vmgrs[i].id==u->ki && vmgrs[i].node==(int)robj.mgr) {
						vmgrs[i].id=0;
						vmgrs[i].node=0;
						vmgrs[i].ip=0;
						vmgrs[i].port=0;
						vmgrs[i].sid=0;
						if(i==n_vmgrs) {
							while(n_vmgrs>1 && vmgrs[n_vmgrs].node==0) {
								n_vmgrs--;
								vmgrs=(st_vault_mgrs *)realloc((void *)vmgrs,sizeof(st_vault_mgrs) * n_vmgrs);
								if(vmgrs==NULL) {
									plog(f_err,"WARNING: Not enough memory to vaultnify a player\n, also now there is an uncontrolled memory leak\n");
								}
							}
						}
					}
				}
				break;
			//****************************** AddNodeRef ******************************
			case 0x03: //add node ref
				print2log(f_vmgr,"Vault AddNodeRef for %i\n",u->ki);
				ret=robj.mgr; //store MGR
				if(table!=NULL) {
					free((void *)table);
				}
				t_n=plVaultGetMGRS(wref->id2,&table,&db);
				int valre;
				valre=0;
				valre=plVaultCreateRef(wref->id1,wref->id2,wref->id3,\
	wref->timestamp,wref->microseconds,wref->flag,&db);
				if(valre<0 || t_n<=0) { break; } //Cath up duplicate references
				DBG(5,"Storing items...\n");
				robj.itm=vaultCreateItems(1);
				robj.n_itms=1;
				robj.itm[0].id=0x07;
				robj.itm[0].dtype=DVaultNodeRef;
				robj.itm[0].data=malloc(sizeof(t_vault_cross_ref) * 1);
				//zero vals
				robj.result=0;
				robj.ctx=0;
				robj.res=0;
				robj.vn=0;
				DBG(5,"Copying big object...\n");
				memcpy((t_vault_cross_ref *)robj.itm[0].data,wref,sizeof(t_vault_cross_ref));
				DBG(5,"Big object copyed...\n");
				//And now broadcast the message to all clients
				for(i=0; i<n_vmgrs; i++) {
					DBG(5,"for(i=0; i<n_vmgrs; i++) i:%i,n_vmgrs:%i\n",i,n_vmgrs);
					if((vmgrs[i].node!=ret || vmgrs[i].id!=u->ki) && vmgrs[i].node!=0) {
						for(e=0; e<t_n; e++) {
							DBG(5,"for(e=0; e<t_n; e++) e:%i,t_n:%i\n",e,t_n);
							if(vmgrs[i].sid<(int)net->n && vmgrs[i].node==table[e]\
	&& vmgrs[i].ip==net->s[vmgrs[i].sid].ip && vmgrs[i].port==net->s[vmgrs[i].sid].port) {
								//sent the broadcast message to that client
								DBG(5,"Vault Broadcast to %i..\n",vmgrs[i].id);
								net->s[vmgrs[i].sid].hmsg.ki=vmgrs[i].id;
								robj.mgr=vmgrs[i].node;
								htmlVaultParse(net,&robj,vmgrs[i].sid,0);
								plNetMsgVault(net,&robj,vmgrs[i].sid);
								break;
							}
						}
					}
				}
				break;
			//****************************** RemoveNodeRef ******************************
			case 0x04: //remove node ref
				print2log(f_vmgr,"Vault RemoveNodeRef for %i\n",u->ki);
				ret=robj.mgr; //store MGR
				if(table!=NULL) {
					free((void *)table);
				}
				t_n=plVaultGetMGRS(node_parent,&table,&db);
				plVaultRemoveNodeRef(node_parent,node_son,0,&db);
				robj.itm=vaultCreateItems(2);
				robj.n_itms=1;
				robj.itm[0].id=0x07; //Node Reference
				robj.itm[0].dtype=DVaultNodeRef;
				robj.itm[0].data=malloc(sizeof(t_vault_cross_ref) * 1);
				wref=(t_vault_cross_ref *)robj.itm[0].data;
				memset(wref,0,sizeof(t_vault_cross_ref));
				wref->id2=node_parent;
				wref->id3=node_son;
				//plVItmPutInteger(&robj.itm[0],node_son);
				//robj.itm[1].id=0x0D;
				//plVItmPutInteger(&robj.itm[1],node_parent);
				//zero vals
				robj.result=0;
				robj.ctx=0;
				robj.res=0;
				robj.vn=0;

				//And now broadcast the message to all clients
				for(i=0; i<n_vmgrs; i++) {
					if((vmgrs[i].node!=ret || vmgrs[i].id!=u->ki) && vmgrs[i].node!=0) {
						for(e=0; e<t_n; e++) {
							if(vmgrs[i].sid<(int)net->n && vmgrs[i].node==table[e]\
	&& vmgrs[i].ip==net->s[vmgrs[i].sid].ip && vmgrs[i].port==net->s[vmgrs[i].sid].port) {
								//sent the broadcast message to that client
								net->s[vmgrs[i].sid].hmsg.ki=vmgrs[i].id;
								robj.mgr=vmgrs[i].node;
								htmlVaultParse(net,&robj,vmgrs[i].sid,0);
								plNetMsgVault(net,&robj,vmgrs[i].sid);
								break;
							}
						}
					}
				}
				break;
			//****************************** NegotiateManifest ******************************
			case VNegotiateManifest: //NegotiateManifest 0x05
				print2log(f_vmgr,"Vault NegotiateManifest for %i\n",u->ki);
				if(table!=NULL && t_n>=1) {
					//get the manifest (only the first int table[0]
					int num,num_ref; //n nodes
					t_vault_manifest * mfs2; //remember to destroy it
					t_vault_cross_ref * vref;
					mfs2=NULL;
					vref=NULL;
					DBG(4,"Before VaultGetManifest...\n");
					robj.zlib=0x03; //set compressed flag
					plVaultGetManifest(table[0],&mfs2,&num,&vref,&num_ref,&db);
					DBG(4,"After VaultGetManifest...\n");
					robj.itm=vaultCreateItems(2);
					robj.n_itms=2;
					robj.itm[0].id=14; //manifest 0x0E
					robj.itm[0].dtype=DCreatableStream;
					DBG(5,"Before a malloc...\n");
					robj.itm[0].data=malloc(sizeof(t_CreatableStream) * 1);
					DBG(5,"After a malloc...\n");
					wcreat=(t_CreatableStream *)(robj.itm[0].data);
					DBG(5,"Before a malloc...\n");
					wcreat->data=malloc(sizeof(Byte) * ((num*(4+8)+4)));
					DBG(5,"After a malloc...\n");
					off=0;
					buf=(Byte *)(wcreat->data);
					*(U32 *)(buf+off)=num;
					off+=4;
					for(i=0; i<num; i++) {
						DBG(5,"for(i=0; i<num; i++) i:%i,num:%i",i,num);
						*(U32 *)(buf+off)=mfs2[i].index;
						off+=4;
						*(double *)(buf+off)=mfs2[i].timestamp;
						off+=8;
					}
					wcreat->size=off;
					if(mfs2!=NULL) {
						free((void *)mfs2);
					}
					//now get all cross references
					//num=plVaultGetCrossRef(table[0],&vref,&db);
					num=num_ref; //quick hack
					robj.itm[1].id=15; //cross refs //0x0F
					robj.itm[1].dtype=DCreatableStream;
					robj.itm[1].data=malloc(sizeof(t_CreatableStream) * 1);
					wcreat=(t_CreatableStream *)(robj.itm[1].data);
					wcreat->data=malloc(sizeof(Byte) * ((num*(4+4+4+8+1)+4)));
					off=0;
					buf=(Byte *)(wcreat->data);
					*(U32 *)(buf+off)=num;
					off+=4;
					for(i=0; i<num; i++) {
						*(U32 *)(buf+off)=vref[i].id1;
						off+=4;
						*(U32 *)(buf+off)=vref[i].id2;
						off+=4;
						*(U32 *)(buf+off)=vref[i].id3;
						off+=4;
						*(U32 *)(buf+off)=vref[i].timestamp;
						off+=4;
						*(U32 *)(buf+off)=vref[i].microseconds;
						off+=4;
						*(Byte *)(buf+off)=vref[i].flag;
						off++;
					}
					wcreat->size=off;
					if(vref!=NULL) {
						free((void *)vref);
					}
					//end negotiation - now send it
					htmlVaultParse(net,&robj,sid,0);
					plNetMsgVault(net,&robj,sid);
				}
				break;
			//****************************** SaveNode ******************************
			case 0x06: //save node
				print2log(f_vmgr,"Vault SaveNode for %i\n",u->ki);
				ret=robj.mgr; //store MGR
				if(table!=NULL) {
					free((void *)table);
				}
				//time hack
				struct timeval tv;
				if(wnode->timestamp==0) {
					time((time_t *)&wnode->timestamp);
					gettimeofday(&tv,NULL);
					wnode->microseconds=tv.tv_usec;
				}
				//end time hack

				if(wnode->index<19000) { //It's a newly created node
					//allocating for two items
					robj.itm=vaultCreateItems(2);
					robj.n_itms=2; //number of items
					//query db & store new index in this object
					plVItmPutInteger(&robj.itm[0],wnode->index);
					robj.itm[0].id=0x09; //The Old index of the Vault Node
					plVItmPutInteger(&robj.itm[1],plVaultCreateNode(wnode,&db));
					robj.itm[1].id=0x0B; //New Index for that vault Node
					htmlVaultParse(net,&robj,sid,0);
					plNetMsgVault(net,&robj,sid);
				} else {
					robj.itm=vaultCreateItems(2);
					robj.n_itms=1; //number of items
					//query db & store new index in this object
					plVItmPutInteger(&robj.itm[0],wnode->index);
					robj.itm[0].id=0x09; //The Old index of the Vault Node
					//query db
					plVaultUpdateNode(wnode,&db);
					htmlVaultParse(net,&robj,sid,0);
					plNetMsgVault(net,&robj,sid);

					robj.n_itms=2; //number of items
					//store timestamp
					plVItmPutTimestamp(&robj.itm[1],((double)wnode->timestamp+\
	(((double)wnode->microseconds)/(double)1000000)));
					robj.itm[1].id=0x18; //Timestamp
					//zero vals
					robj.result=0;
					robj.ctx=0;
					robj.res=0;
					robj.vn=0;

					t_n=plVaultGetMGRS(wnode->index,&table,&db);
					//And now broadcast the message to all clients
					for(i=0; i<n_vmgrs; i++) {
						DBG(5,"for(i=0; i<n_vmgrs; i++) i:%i,n_vmgrs:%i\n",i,n_vmgrs);
						if((vmgrs[i].node!=ret || vmgrs[i].id!=u->ki) && vmgrs[i].node!=0) {
							for(e=0; e<t_n; e++) {
								DBG(5,"for(e=0; e<t_n; e++) e:%i,t_n:%i\n",e,t_n);
								if(vmgrs[i].sid<(int)net->n && vmgrs[i].node==table[e]\
		&& vmgrs[i].ip==net->s[vmgrs[i].sid].ip && vmgrs[i].port==net->s[vmgrs[i].sid].port) {
									//sent the broadcast message to that client
									DBG(5,"Vault Broadcast to %i..\n",vmgrs[i].id);
									net->s[vmgrs[i].sid].hmsg.ki=vmgrs[i].id;
									robj.mgr=vmgrs[i].node;
									htmlVaultParse(net,&robj,vmgrs[i].sid,0);
									plNetMsgVault(net,&robj,vmgrs[i].sid);
									break;
								}
							}
						}
					} //for
				} //else
				break;
			//****************************** FindNode ******************************
			case 0x07: //find node
				print2log(f_vmgr,"Vault FindNode for %i\n",u->ki);
				ret=plVaultFindNode(wnode, &mfs,1,&db);
				robj.itm=vaultCreateItems(2);
				robj.n_itms=2;
				plVItmPutInteger(&robj.itm[0],ret);
				robj.itm[0].id=9; //found node index
				plVItmPutTimestamp(&robj.itm[1],mfs.timestamp);
				robj.itm[1].id=0x18; //Node timestamp
				htmlVaultParse(net,&robj,sid,0);
				plNetMsgVault(net,&robj,sid); //send the update
				break;
			//****************************** FetchNodes ******************************
			case 0x08: //FetchNodes
				print2log(f_vmgr,"Vault FetchNodes for %i\n",u->ki);
				if(table!=NULL && t_n>=1) {
					//fetch the nodes, stored in the table
					int size; //size of stream
					t_vault_node * node3; //remember to destroy it
					node3=NULL;
					int n_fetched=0;
					size=plVaultFetchNodes(&node3,&n_fetched,(U32 *)table,t_n,&db);
					DBG(5,"End fetching nodes from db, avg size is %i...\n",size);
					robj.zlib=0x03; //set compressed flag
					DBG(5,"Create Items...\n");
					robj.itm=vaultCreateItems(3);
					robj.n_itms=3;
					robj.itm[0].id=6; //VaultNodes 0x06
					robj.itm[0].dtype=DCreatableStream;
					robj.itm[0].data=malloc(sizeof(t_CreatableStream) * 1);
					wcreat=(t_CreatableStream *)(robj.itm[0].data);
					wcreat->data=malloc(sizeof(Byte) * (size+100)); //aprox size
					buf=(Byte *)(wcreat->data);
					int processed=0;
					int stored=0;
					//set EOF --
					robj.itm[2].id=0x1F; //EOF
					plVItmPutInteger(&robj.itm[2],0);
					//end set EOF
					while(processed<n_fetched) {
						DBG(5,"Processing... processed:%i,n_fetched:%i,t_n:%i\n",processed,n_fetched,t_n);
						off=0;
						stored=0;
	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);
						while(off<128000 && processed<n_fetched) {
							//vault_parse_node_data(f_vhtml,&node3[processed]);
							DBG(5,"offset:%i,processed:%i,stored:%i\n",off,processed,stored);
							off+=node2stream(buf+off,&node3[processed]);
							processed++;
							stored++;
	DBG(5,"dmalloc_verify()\n");
	dmalloc_verify(NULL);
						}
						wcreat->size=off;
						//set number of nodes
						robj.itm[1].id=0x19;
						plVItmPutInteger(&robj.itm[1],stored);
						if(processed>=n_fetched-1) {
							robj.n_itms=3;
						} else {
							robj.n_itms=2;
						}
						DBG(5,"Calling to the html parser...\n");
						htmlVaultParse(net,&robj,sid,0);
						DBG(5,"Sending the message...\n");
						plNetMsgVault(net,&robj,sid);
						DBG(5,"Message sent...\n");
					}
					robj.n_itms=3; //por si acaso
					if(node3!=NULL) { //destroy this garbage
						for(i=0; i<n_fetched; i++) {
							destroy_node(&node3[i]);
						}
						free((void *)node3);
					}
				}
				break;
			//****************************** SendNode ******************************
			case 0x09:
				print2log(f_vmgr,"Vault SendNode for %i\n",u->ki);
				if(rcv_player>0 && wnode!=NULL && wnode->index>0) {
					//Find or create the PeopleIKnowAboutFolder
					n.type=30; //Info List node
					n.torans=4; //PeopleIKnowAboutFolder
					n.owner=rcv_player;
					id=plVaultFindNode(&n,&mfs,0,&db);
					if(id<=0) { //not found
						id=plVaultFindNode(&n,&mfs,1,&db); //Create the folder
						plVaultCreateRef(0,rcv_player,id,0,0,0,&db);
						plVaultBcastNodeReferenceAdded(net,rcv_player,id);
					}
					destroy_node(&n);
					init_node(&n);
					//Find The sender Info List Node
					n.type=23; //Player Info Node
					n.owner=u->ki; //Sender KI
					ret=plVaultFindNode(&n,&mfs,0,&db);
					if(id>0 && ret>0) {
						//Add the player to the Recent folder
						plVaultCreateRef(0,id,ret,0,0,0,&db);
						plVaultBcastNodeReferenceAdded(net,id,ret);

						destroy_node(&n);
						init_node(&n);
						//Find or create the recievers Inbox folder
						n.type=22; //Folder Node
						n.torans=1; //InboxFolder
						n.owner=rcv_player; //The destination
						id=plVaultFindNode(&n,&mfs,0,&db);
						if(id<=0) { //not found, create it
							id=plVaultFindNode(&n,&mfs,1,&db);
							plVaultCreateRef(0,rcv_player,id,0,0,0,&db);
							plVaultBcastNodeReferenceAdded(net,rcv_player,id);
						}
						if(id>0) { //And now add the node that we recieved from player
							plVaultCreateRef(u->ki,id,wnode->index,0,0,0,&db);
							plVaultBcastNodeReferenceAdded(net,id,wnode->index);
						}
					}
				}
	#if 0
				if(obj->n_itms!=3) {
					print2log(f_vmgr,"Unexpected number of items %i\n",obj->n_itms);
					success=-1;
					goto end_3;
				}
				//allocating for two items -- no response --
				//sobj.itm=create_objects(2);
				sobj.n_itms=0; //number of items
				//check item 1
				if(obj->itm[0].format_flag!=0x01 || obj->itm[0].cmd!=0x0387 || (obj->itm[0].integer!=2)) { // && obj->itm[0].integer!=3 && obj->itm[0].integer!=0x21)) {
					print2log(f_vmgr,"Unknown unexpected SendNode request format in item 1\n");
					success=-1;
					goto end_3;
				}
				//check item2
				if(obj->itm[1].format_flag!=0x04 || obj->itm[1].cmd!=0x0387) {
					print2log(f_vmgr,"Unknown unexpected SendNode request format in item 2\n");
					success=-1;
					goto end_3;
				}
				//check item3
				if(obj->itm[2].format_flag!=0x05 || (obj->itm[2].cmd!=0x0439 && obj->itm[2].cmd!=0x043A)) {
					print2log(f_vmgr,"Unknown unexpected SendNode request format in item 2\n");
					success=-1;
					goto end_3;
				}

				t_vault_node a_node;

				init_node(&a_node);

				//search for the inbox folder
				a_node.owner = obj->itm[1].integer;
				a_node.type = KFolderNode; //0x16 22
				a_node.torans = 0x01; //inbox folder 1

				t_vault_manifest a_mfs;

				print2log(f_vmgr,"Querying for inbox folder for that player\n");
				plVaultFindNode(&a_node,&a_mfs);

				time_t vlttime;
				struct timeval tv;
				gettimeofday(&tv,NULL);
				time(&vlttime);

				//create a reference to the recieved node
				plVaultCreateRef(a_node.owner,a_mfs.index,obj->itm[2].node[0].index,\
	(U32)vlttime,tv.tv_usec,0);

				destroy_node(&a_node);

				print2log(f_vmgr,"node sent\n",mfst.index,mfst.timestamp);

				//print2log(f_vmgr,"Unimplemented, and ignored SendNode petition\n");
				sobj.n_itms=0;
	#endif
				break;
			//****************************** SetSeen ******************************
			case 0x0A: //setseen
				print2log(f_vmgr,"Vault SetSeen for %i\n",u->ki);
				DBG(5,"Calling plVaultSeetRefSeen\n");
				plVaultSeetRefSeen(node_parent,node_son,seen_flag,&db);
				DBG(5,"After the plVaultSeetRefSeen call\n");
				break;
			default:
				print2log(f_vmgr,"Unkwon non-implemented vault request code %02X\n",obj->cmd);
				//return -1;
		} //end switch

	}//additional Else, from de additional check about if we are connected
	destroy_node(&n);
	plMainVaultDestroy(&robj,0);
	if(table!=NULL) {
		free((void *)table);
	}

	if(success<0) {
		print2log(f_vmgr,"Failed parsing vault stream!\n");
	}
	printf("RETURNING from vault_do_magic_stuff with value %i\n",success);
	//fflush(0);
	return success;
}

#if 0
//Sends the hosted age anounce message to the client
int sent_vault_age_anounce(int sock,st_uru_client * u,t_vault_node * node) {

	t_vault_mos sobj; //send object
	Byte * data_buf=NULL; //pointer to the vault data
	int size; //data size

	//copy the data structs default values
	sobj.ki=u->ki;
	sobj.code=0x0B; //anounce
	sobj.unk1=0; //set 0
	sobj.format=0x01; //default format
	sobj.fcode=0; //ctx (the context)
	sobj.unk2=0;
	sobj.unk_byte=0;
	sobj.fki=u->ki;
	sobj.tail=0x0000; //always??????????????

	sobj.itm=NULL; //security

	//allocating for 5 items
	sobj.itm=create_objects(5);
	sobj.n_itms=5; //number of items

	//main object
	///sobj.format=0x03; //enable compression
	//itm 1 - Node index
	sobj.itm[0].format_flag=0x09; //node index
	sobj.itm[0].unk1=0x00; //always 0
	sobj.itm[0].cmd=0x0387;
	sobj.itm[0].format=0x00; //integer
	sobj.itm[0].integer=node->index; //the player node
	//itm 2 - timestamp
	sobj.itm[1].format_flag=0x18; //node timestamp
	sobj.itm[1].unk1=0x00; //always 0
	sobj.itm[1].cmd=0x0387;
	sobj.itm[1].format=0x07; //an 8 bytes number //timestamp
	*(double *)sobj.itm[1].guid=(double)node->timestamp + ((double)node->timestamp/1000000); //the timestamp of the node
	//itm 3 - Age filename
	sobj.itm[2].format_flag=0x1B; //age filename
	sobj.itm[2].unk1=0x00; //always 0
	sobj.itm[2].cmd=0x0387;
	sobj.itm[2].format=0x03; //uru string (age filename)
	strcpy((char *)sobj.itm[2].ustring,(const char *)node->entry_name);
	//itm 4 - Age guid
	sobj.itm[3].format_flag=0x1C; //age filename
	sobj.itm[3].unk1=0x00; //always 0
	sobj.itm[3].cmd=0x0387;
	sobj.itm[3].format=0x03; //uru string (age guid)
	strcpy((char *)sobj.itm[3].ustring,(const char *)node->sub_entry_name);
	//itm 5 - an integer?
	sobj.itm[4].format_flag=0x1D; //end?
	sobj.itm[4].unk1=0x00; //always 0
	sobj.itm[4].cmd=0x0387;
	sobj.itm[4].format=0x00; //integer
	sobj.itm[4].integer=0x01; //always?? (number of players??!!)
	//0x01 - online
	//0x00 - offline

	print2log(f_vmgr,"vault anounce for %s, sent to %i\n",node->entry_name,node->index);

	print2log(f_vmgr,"packing the vault\n");
	logflush(f_vmgr);
	size=pack_vault(&data_buf,&sobj);
	//dump the packet vault
	store_packet(data_buf, size, "cow_dumps");
	//---- ecks
	print2log(f_vmgr,"finished packing the vault\n");
	logflush(f_vmgr);
	if(size>0) {
		//first pass throught the parser
		fprintf(f_vhtml,"<hr><font color=\"blue\"><b>SENT TO CLIENT</b></font>\n");
		dump_packet(f_vmgr,data_buf,size,0,7);
		print2log(f_vmgr,"\n");
		print2log(f_vmgr,"Calling parser...\n");
		logflush(f_vmgr);
		vault_parse_the_msg(data_buf,size,u);
		print2log(f_vmgr,"Succesfully parsed!, Sending %i bytes..,\n",size);
		logflush(f_vmgr);
		//then sent it
		plNetMsgVault(sock,u,data_buf,size);
		print2log(f_vmgr,"Succesfully sent!\n");
		logflush(f_vmgr);
	}
	//destroy the data buffer
	free((void *)data_buf);

	if(sobj.itm!=NULL) {
		vault_item_destroy(&sobj);
	}

	return 0;
}

//Sends the RefNode creation notification to the client
int plVaultCreateRefNotification(U32 id1,U32 id2,U32 id3,U32 stamp,U32 micros,Byte flag,int sock, st_uru_client * u) {

	t_vault_mos sobj; //send object
	Byte * data_buf=NULL; //pointer to the vault data
	int size; //data size

	//copy the data structs default values
	sobj.ki=u->ki;
	sobj.code=0x03; //AddNodeRef
	sobj.unk1=0; //set 0
	sobj.format=0x01; //default format
	sobj.fcode=0; //ctx (the context)
	sobj.unk2=0;
	sobj.unk_byte=0;
	sobj.fki=u->ki;
	sobj.tail=0x0000; //always??????????????

	sobj.itm=NULL; //security

	//allocating for 1 item
	sobj.itm=create_objects(1);
	sobj.n_itms=1; //number of items

	//main object
	///sobj.format=0x03; //enable compression
	//itm 1 - Node index
	sobj.itm[0].format_flag=0x07; //node index
	sobj.itm[0].unk1=0x00; //always 0
	if(u->tpots==1) { //tpots flag
		sobj.itm[0].cmd=0x0439;
	} else {
		sobj.itm[0].cmd=0x0438;
	}
	sobj.itm[0].format=0xFC; //cross_ref

	sobj.itm[0].num_items=1;
	//not sure, so let's go to calculate it for security
	sobj.itm[0].data_size=21; //sizeof(t_vault_cross_ref);

	sobj.itm[0].cross_ref=(t_vault_cross_ref *)(malloc(sizeof(t_vault_cross_ref)));
	sobj.itm[0].cross_ref->id1=id1;
	sobj.itm[0].cross_ref->id2=id2;
	sobj.itm[0].cross_ref->id3=id3;
	sobj.itm[0].cross_ref->timestamp=stamp;
	sobj.itm[0].cross_ref->microseconds=micros;
	sobj.itm[0].cross_ref->flag=flag;

	print2log(f_vmgr,"node ref added notification for %i...",u->ki);

	print2log(f_vmgr,"packing the vault\n");
	logflush(f_vmgr);
	size=pack_vault(&data_buf,&sobj);
	//dump the packet vault
	store_packet(data_buf, size, "cow_dumps");
	//---- ecks
	print2log(f_vmgr,"finished packing the vault\n");
	logflush(f_vmgr);
	if(size>0) {
		//first pass throught the parser
		fprintf(f_vhtml,"<hr><font color=\"blue\"><b>SENT TO CLIENT</b></font>\n");
		dump_packet(f_vmgr,data_buf,size,0,7);
		print2log(f_vmgr,"\n");
		print2log(f_vmgr,"Calling parser...\n");
		logflush(f_vmgr);
		vault_parse_the_msg(data_buf,size,u);
		print2log(f_vmgr,"Succesfully parsed!, Sending %i bytes..,\n",size);
		logflush(f_vmgr);
		//then sent it
		plNetMsgVault(sock,u,data_buf,size);
		print2log(f_vmgr,"Succesfully sent!\n");
		logflush(f_vmgr);
	}
	//destroy the data buffer
	free((void *)data_buf);

	if(sobj.itm!=NULL) {
		vault_item_destroy(&sobj);
	}

	return 0;
}

#endif

#endif
