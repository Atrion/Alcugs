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


// The vault

/*
	this will contain all related with using the vault, mysql access
*/

#ifndef __U_VAULT_DB_H_
#define __U_VAULT_DB_H_
/* CVS tag - DON'T TOUCH*/
#define __U_VAULT_DB_H_ID "$Id$"

#include "sql.h"
#include "vaultstrs.h"

extern const int vault_version;

int plVaultQueryPlayerList(Byte * guid,Byte ** data,int * size,st_sql * db);
int plVaultGetNumberOfPlayers(Byte * guid,st_sql * db);
int plVaultCheckKi(Byte * guid,U32 ki,Byte * avatar_name,st_sql * db);
/**
  Creates a new node, and returns the id
	returns -1 on db error
	returns 0 if node already exists
	returns node_id if node has been succesfully created
*/
int plVaultCreateNode(t_vault_node * node,st_sql * db);
/**
  Updates a node, and returns the id
	returns -1 on db error
	returns node_id if node has been succesfully created
	(important mask must be set!, only masked items will be updated!)
*/
int plVaultUpdateNode(t_vault_node * node,st_sql * db);
/** search for a node,
   returns the id of the node with the modify timestamp
   if it doesn't exist
	 flag=0 -> returns '0' does not exist
	 flag=1 -> creates the node, and returns the id of that node
	 -1 on db error
*/
int plVaultFindNode(t_vault_node * node, t_vault_manifest * mfs,Byte flag,st_sql * db);
/**
	Creates a vault reference
	returns -1 on db error
*/
int plVaultCreateRef(U32 id1,U32 id2,U32 id3,U32 tstamp,U32 micros,Byte flag,st_sql * db);
/**
  Removes a node (put father=0, son=id to remove a root node)
	with flag=1 to force deletion!
	with flag=2 search for the info nodes and delete them (only if they are at level 1)
	if flag=1 and the node is a MGR it will be deleted!
	\return -1 on db error, 0 if node doens't exist and >0 if success
	(if tree_force==1, deletes the tree, if not, it is not deleted)
*/
int plVaultRemoveNodeRef2(U32 father, U32 son, Byte flag,Byte tree_force,st_sql * db);
int plVaultRemoveNodeRef(U32 father, U32 son, Byte flag,st_sql * db);
/** Set the reference as a seen reference
*/
int plVaultSeetRefSeen(U32 father, U32 son,Byte flag,st_sql * db);
/** Query the database for nodes
   (num is the number of nodes, table is a list of nodes)
	 remember to destroy the node buffer!
	 (it tryes to return the exact size of the node stream data)
*/
int plVaultFetchNodes(t_vault_node ** node,int * num,U32 * table,int n_table,st_sql * db);
/* Get the cross reference from an specific node
   (it has anti-loop protection) <-not implemented
   returns the number of references if success
	 returns 0 if fails
*/
int plVaultGetCrossRef(U32 id, t_vault_cross_ref ** ref,st_sql * db);
/** query the manifest
     all immediatly child nodes (it must get all, but well.. just testing..)
     returns -1 on db error, elsewhere the number of nodes
*/
//int plVaultGetManifest(U32 id, t_vault_manifest ** mfs,st_sql * db);
int plVaultGetManifest(U32 id, t_vault_manifest ** mfs,int * n_mfs,t_vault_cross_ref ** ref,int * n_ref,st_sql * db);
/** query the manifest
     all immediatly child nodes (it must get all, but well.. just testing..)
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetMGRS(U32 id, int ** table,st_sql * db);
/** query for parent nodes
     returns -1 on db error, elsewhere the number of nodes
*/
int plVaultGetParentNodes(U32 id, int ** table,st_sql * db);

/** Initialitzes the database
*/
int plVaultInitializeDB(st_sql * db);

/** Gets the folder
 returns -1 on error 0 or >0 if query was succesfull
*/
char * plVaultGetFolder(st_sql * db);

int plVaultGetVersion(st_sql * db);

int plVaultMigrate(int ver,st_sql * db);

#endif

