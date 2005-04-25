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

/* CVS tag - DON'T TOUCH*/
#define __U_PYTHONGLUE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "pythonh.h" //python zzzzzzzzzzzz

#include "data_types.h"
#include "stdebug.h"
#include "prot.h"
#include "protocol.h"

#include "conv_funs.h"
#include "useful.h"

#include "config_parser.h"
#include "uru.h"

//#include "ageparser.h"
//#include "sdlparser.h" //SDL byte code parser

#include "pythonsubsys.h"
#include "pythonglue.h"

#include "debug.h"

//class's (objects)
//Access to the python Logfile (and other logs)
extern PyTypeObject ptLog_Type;

typedef struct{
	PyObject_HEAD
	st_log * log;
} ptLog_Object;

//Access to the configuration
extern PyTypeObject ptConfig_Type;

typedef struct {
	PyObject_HEAD
	//st_config * cnf;
} ptConfig_Object;

//Access to the netcore
extern PyTypeObject ptNet_Type;

typedef struct {
	PyObject_HEAD
	//st_unet * net;
} ptNet_Object;

#if 0
//Access to the SDL vars - see proposedsdlstuff/pythonglue_partial.cpp
extern PyTypeObject ptSDL_Type;

typedef struct {
	PyObject_HEAD
	//TODO
} ptSDL_Object;
#endif
//end classes

//def's
static PyObject* alcugs_PtGetAgeName(PyObject *self, PyObject *args) {
	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("s", gnet->name);
}

static PyObject* alcugs_PtGetAgeGUID(PyObject *self, PyObject *args) {
	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("s", gnet->guid);
}

static PyObject* alcugs_PtGetNetTime(PyObject *self, PyObject *args) {
	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", gnet->timestamp);
}

static PyObject* alcugs_PtGetNetMicros(PyObject *self, PyObject *args) {
	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", gnet->microseconds);
}

static PyObject* alcugs_PtGetTime(PyObject *self, PyObject *args) {
	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", (U32)time(NULL));
}

static PyObject* alcugs_PtGetMicros(PyObject *self, PyObject *args) {
	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	return Py_BuildValue("i", get_microseconds());
}


static PyObject* alcugs_PtGetPythonLog(PyObject *self, PyObject *args) {
	ptLog_Object * ptLog;
	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	ptLog = PyObject_New(ptLog_Object, &ptLog_Type);
	((ptLog_Object *)ptLog)->log=f_python;

	return (PyObject *)ptLog;
}

#if 0
static PyObject* alcugs_PtGetAgeSDL(PyObject *self, PyObject *args) {
	ptSDL_Object * ptSDL;
	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	ptSDL = PyObject_New(ptSDL_Object, &ptSDL_Type);
	//ptSDL->bin=sdl;

	return (PyObject *)ptSDL;
}
#endif
//end def's

//class's (objects) def's/method's
//Python Log
static PyObject * ptLog_new(PyObject * self,PyObject * args) {
	ptLog_Object * ptLog;
	char * name=NULL;
	st_log * log=NULL;
	Byte level=2;
	Byte flags=DF_APPEND;

	if(!PyArg_ParseTuple(args,"s|bb:ptLog",&name,&level,&flags))
		return NULL;

	log=open_log(name,level,flags);
	if(log==NULL) return NULL;

	ptLog = PyObject_New(ptLog_Object, &ptLog_Type);
	((ptLog_Object *)ptLog)->log=log;

	return (PyObject *)ptLog;
}

static void ptLog_dealloc(PyObject * self) {
	if(((ptLog_Object *)self)->log!=f_python) {
		close_log(((ptLog_Object *)self)->log);
		((ptLog_Object *)self)->log=NULL;
	}
	PyObject_Del(self);
}

static PyObject* ptLog_write(PyObject *self, PyObject *args) {
	char * msg;
	if(!PyArg_ParseTuple(args, "s",&msg))
		return NULL;
	plog(((ptLog_Object *)self)->log,msg);
	if(msg[strlen(msg)-1]!='\n') {
		lognl(((ptLog_Object *)self)->log);
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* ptLog_flush(PyObject *self, PyObject *args) {
	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	logflush(((ptLog_Object *)self)->log);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject* ptLog_close(PyObject *self, PyObject *args) {
	if(!PyArg_ParseTuple(args, ""))
		return NULL;
	DBG(5,"ptLog_close\n");
	if(((ptLog_Object *)self)->log!=f_python) {
		DBG(5,"close_log\n");
		close_log(((ptLog_Object *)self)->log);
		((ptLog_Object *)self)->log=NULL;
	}
	DBG(5,"hmm\n");
	Py_INCREF(Py_None);
	return Py_None;
}

//Methods
static PyMethodDef ptLog_methods[] = {
	{"write", (PyCFunction)ptLog_write, METH_VARARGS,
		"Writes to the log file"},
	{"flush", (PyCFunction)ptLog_flush, METH_VARARGS,
		"Flushes the log file"},
	{"close", (PyCFunction)ptLog_close, METH_VARARGS,
		"Closes the log file"},
	{NULL, NULL, 0, NULL} /* sentinel */
};

static PyObject * ptLog_getattr(PyObject * obj, char * name) {
	return Py_FindMethod(ptLog_methods, (PyObject *)obj, name);
}

// Don't works
PyTypeObject ptLog_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"ptLog",
	sizeof(ptLog_Object),
	0,
	ptLog_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	ptLog_getattr, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_compare*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	0, /*tp_as_sequence*/
	0, /*tp_as_mapping*/
	0 /*tp_hash*/
};
//end ptLog class

//begin ptConfig class
static PyObject * ptConfig_new(PyObject * self,PyObject * args) {
	ptConfig_Object * ptConfig;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	ptConfig = PyObject_New(ptConfig_Object, &ptConfig_Type);
	//constructors

	return (PyObject *)ptConfig;
}

static void ptConfig_dealloc(PyObject * self) {
	//Destructors
	PyObject_Del(self);
}

static PyObject* ptConfig_getkey(PyObject *self, PyObject *args) {
	PyObject * pRet;
	char * key,* where;
	where="global";
	if(!PyArg_ParseTuple(args, "s|s",&key,&where))
		return NULL;

	pRet = PyString_FromString(cnf_getString("",key,(const char *)where,global_config));

	return pRet;
}

static PyObject* ptConfig_setkey(PyObject *self, PyObject *args) {
	char * val,* key,* where;
	int ret;
	where="global";
	if(!PyArg_ParseTuple(args, "ss|s",&val,&key,&where))
		return NULL;
	
	ret=cnf_add_key(val,key,where,&global_config);

	if(ret==0) {
		Py_INCREF(Py_None);
		return Py_None;
	} else {
		return NULL;
	}
}

static PyObject* ptConfig_reload(PyObject *self, PyObject *args) {
	return NULL;
}

//Methods
static PyMethodDef ptConfig_methods[] = {
	{"getkey", (PyCFunction)ptConfig_getkey, METH_VARARGS,
		"Returns the string for the selected key. Params: key, [section]"},
	{"setkey", (PyCFunction)ptConfig_setkey, METH_VARARGS,
		"Sets a new value for that key. Params: value, key, [section]"},
	{"reload", (PyCFunction)ptConfig_reload, METH_VARARGS,
		"Reloads the configuration"},
	{NULL, NULL, 0, NULL} /* sentinel */
};

static PyObject * ptConfig_getattr(PyObject * obj, char * name) {
	return Py_FindMethod(ptConfig_methods, (PyObject *)obj, name);
}

PyTypeObject ptConfig_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"ptConfig",
	sizeof(ptConfig_Object),
	0,
	ptConfig_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	ptConfig_getattr, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_compare*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	0, /*tp_as_sequence*/
	0, /*tp_as_mapping*/
	0 /*tp_hash*/
};
//end ptConfig class

//end classes


//method def's. WARNING, THIS LIST IS GOING TO BE VERY HUGE!!
static PyMethodDef AlcugsMethods[] = {
//GLUED METHODS
	{"PtGetAgeName", alcugs_PtGetAgeName, METH_VARARGS,
		"Return the current age name."},
	{"PtGetAgeGUID", alcugs_PtGetAgeGUID, METH_VARARGS,
		"Return the current age guid."},
	{"PtGetPythonLog", alcugs_PtGetPythonLog, METH_VARARGS,
		"Returns a ptLog object representing the python log file."},
	{"PtGetNetTime", alcugs_PtGetNetTime, METH_VARARGS,
		"Returns timestamp of the current netcore loop."},
	{"PtGetNetMicros", alcugs_PtGetNetMicros, METH_VARARGS,
		"Returns microseconds of the current netcore loop."},
	{"PtGetTime", alcugs_PtGetTime, METH_VARARGS,
		"Returns timestamp of the current time."},
	{"PtGetMicros", alcugs_PtGetMicros, METH_VARARGS,
		"Returns microseconds of the current time."},
#if 0
	{"PtGetAgeSDL", alcugs_PtGetAgeSDL, METH_VARARGS,
		"Returns a ptSDL object representing the SDL vars."}, 
#endif
//GLUED CLASSES
	{"ptLog", ptLog_new, METH_VARARGS,
		"Creates a new log file."},
	{"ptConfig", ptConfig_new, METH_VARARGS,
		"Returns the config manager abstract object"},
	{NULL, NULL, 0, NULL}
};

//Should be called only one time to install all modules
void initalcugs() {
	ptLog_Type.ob_type = &PyType_Type;

	Py_InitModule("alcugs", AlcugsMethods);
}

