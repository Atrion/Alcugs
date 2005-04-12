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

//This is the vault subsystem required to parse those vault messages

/* CVS tag - DON'T TOUCH*/
#define __U_PYTHONSUBSYS_ID "$Id: psauthmsg_internal.cpp,v 1.1 2004/10/24 11:20:27 almlys Exp $"
const char * _python_driver_ver="1.1.1b";

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef __WIN32__
#  include "windoze.h"
#endif

#ifdef __MSVC__
#  include <direct.h>
#endif

#include "pythonh.h" //python zzzzzzzzzzzz

#include "data_types.h"
#include "stdebug.h"
#include "prot.h"
#include "protocol.h"

#include "conv_funs.h"

#include "config_parser.h"
#include "uru.h"

//#include "ageparser.h"
//#include "sdlparser.h" //SDL byte code parser

#include "pythonglue.h"

#include "pythonsubsys.h"

#include "debug.h"

int python_initialitzed=0;

st_log * f_python=NULL;

st_unet * gnet=NULL;


int init_python_subsys(st_unet * net) {
	int ret=0;
	if(python_initialitzed) return 0;
	python_initialitzed=1;

	gnet=net;

	DBG(5,"Initialiting Python subsystem...\n");

	if(cnf_getByte(1,"python.log","global",global_config)==1) {
		f_python=open_log("python.log",2,DF_STDOUT);
	}

	char * aux;
	aux = (char *)cnf_getString("./python/","python","global",global_config);

	//Start up Python
	Py_Initialize();

	//glue
	initalcugs();

	//set scripts paths
	char pydir[500];
	getcwd(pydir,499);
	char buf[1024];
	sprintf(buf,"\
import sys\n\
sys.path.append(\"%s\")\n\
sys.path.append(\"%s/%s\")\n\
#print sys.path\n",pydir,pydir,aux); //print sys.path\n",aux);
	PyRun_SimpleString(buf);

	PyRun_SimpleString("\
import alcugs\n\
___p=alcugs.PtGetPythonLog()\n\
sys.stdout=___p\n\
sys.stderr=___p\n\
print sys.path\n");

	plog(f_python,"INF: Python subsystem v %s started\n",_python_driver_ver);
	plog(f_python,"Python Version: %s\n",Py_GetVersion());

	sprintf(buf,"\
import traceback\n\
try:\n\
    import xSystem\n\
    #xSystem.onStartup()\n\
except:\n\
    traceback.print_exc()\n\
\n\
try:\n\
    import %s\n\
    #%s.onStartup()\n\
except:\n\
    traceback.print_exc()\n\
\n\
",net->name,net->name);
	PyRun_SimpleString(buf);

	PyRun_SimpleString("xSystem.onStartup()");
	sprintf(buf,"%s.onStartup()",net->name);
	PyRun_SimpleString(buf);

	logflush(f_python);

	return ret;
}

void stop_python_subsys() {
	if(python_initialitzed!=1) return;

	char buf[1024];
/*
	sprintf(buf,"\
import traceback\n\
try:\n\
    import xSystem\n\
    xSystem.onShutdown()\n\
except:\n\
    traceback.print_exc()\n\
\n\
try:\n\
    import %s\n\
    %s.onShutdown()\n\
except:\n\
    traceback.print_exc()\n\
\n\
",gnet->name,gnet->name);
	PyRun_SimpleString(buf);
*/
	PyRun_SimpleString("xSystem.onShutdown()");
	sprintf(buf,"%s.onShutdown()",gnet->name);
	PyRun_SimpleString(buf);


	plog(f_python,"INF: python subsystem stopped\n");

	//End Python
	Py_Finalize();

	close_log(f_python);
	f_python=NULL;
	python_initialitzed=0;
}

//Needs to be done on SIGHUP to be able to apply any changes performed in the config files
void reload_python_subsys(st_unet * net) {
	plog(f_python,"INF: Reloading python subsystem...\n");
	stop_python_subsys();
	init_python_subsys(net);
}

void update_python_subsys() {
	//
}

void python_subsys_idle_operation() {
	char buf[1024];

	PyRun_SimpleString("xSystem.onIdle()");
	sprintf(buf,"%s.onIdle()",gnet->name);
	PyRun_SimpleString(buf);
}
