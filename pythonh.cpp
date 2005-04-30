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

//Python Subsystem

/* CVS tag - DON'T TOUCH*/
#define __U_PYTHONH_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pythonh.h"

#include "debug.h"

/** Imports a new Module
*/
st_pymodule * pyt_import(char * name) {
	st_pymodule * ret=NULL;
	PyObject * pName;
	pName = PyString_FromString(name);
	//TODO error checking
	ret=(st_pymodule *)malloc(sizeof(st_pymodule) * 1);
	if (ret!=NULL) {
		memset(ret,0,sizeof(st_pymodule));
		ret->obj = PyImport_Import(pName);
		if(ret->obj!=NULL) {
			ret->flags |= PYN_EXISTS;
		} else {
			free((void *)ret);
			ret=NULL;
		}
	}
	Py_DECREF(pName); //destroy pName
	return ret;
}

/** Runs a method. Returns a new reference (returned value), that you must destroy it */
PyObject * pyt_call(st_pymodule * where, char * what, PyObject * pArgs) {
	if(where==NULL || !(where->flags & PYN_EXISTS)) return NULL;

	PyObject * pDict, * pFunc, * pValue;
	
	pDict = PyModule_GetDict(where->obj);
	/* pDict, borrowed reference */
	pFunc = PyDict_GetItemString(pDict,what);
	/* pFunc, borrowed */
	//Check if can be called
	if(pFunc && PyCallable_Check(pFunc)) {
		//call it
		pValue = PyObject_CallObject(pFunc, pArgs);
	} else { 
		return NULL; 
	}
	return pValue;
}

/** Returns true (1) if the selected method exists, else it returns false (0) */
int pyt_exists(st_pymodule * where, char * what) {
	if(where==NULL || !(where->flags & PYN_EXISTS)) return 0;

	PyObject * pDict, * pFunc;
	
	pDict = PyModule_GetDict(where->obj);
	/* pDict, borrowed reference */
	pFunc = PyDict_GetItemString(pDict,what);
	/* pFunc, borrowed */
	//Check if can be called
	if(pFunc && PyCallable_Check(pFunc)) { return 1; }
	return 0;
}

/** If a python error ocurred, returns 1 and prints it to the python log, else returns 0 */
int pyt_error() {
	if(PyErr_Occurred()) {
		PyErr_Print();
		return 1;
	}
	return 0;
}

