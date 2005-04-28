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
#define __U_PYTHONGLUE_PT_SDL_OBJECT_ID "$Id$"


#include "../gamesubsys.h"


#if 0
//Access to the SDL vars
extern PyTypeObject ptSDL_Type;

typedef struct {
	PyObject_HEAD
	t_sdl_binary * bin;
} ptSDL_Object;
#endif



//begin ptSDL class
static PyObject * ptSDL_new(PyObject * self,PyObject * args) {
	ptSDL_Object * ptSDL;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	ptSDL = PyObject_New(ptSDL_Object, &ptSDL_Type);
	//constructors

	return (PyObject *)ptSDL;
}

static void ptSDL_dealloc(PyObject * self) {
	//Destructors
	PyObject_Del(self);
}


PyObject* Py_BuildValue_va(char *outputformatstr,int inputc,va_list inputs)
{
	char * inptr=(char *)inputs+(sizeof(void*)*inputc);

	void *startesp;

	_asm
	{
		mov startesp, esp;
	}

	for(int i=inputc; i>0;i--)
	{
		inptr-=sizeof(void *);
		
		_asm
		{
			push inptr;
		}
	}

	PyObject *return_val;

	_asm
	{
		push outputformatstr;
		call Py_BuildValue;
		mov esp, startesp;
		mov return_val,eax;
	}

	return return_val;
}




static PyObject* ptSDL_getitem(PyObject *self, PyObject *args)
{
	char * key;
	if(!PyArg_ParseTuple(args, "s",&key))
		return NULL;

	t_sdl_head *head = global_sdl_bin;

	int sdlindex=find_sdl_descriptor(head->name,head->version,global_sdl_def,global_sdl_def_n);

	t_sdl_def *sdldesc=&global_sdl_def[sdlindex];

	for(int i=0; i<head->bin.n_values; i++)
	{
		if(!strcmp((char *)&sdldesc->vars[head->bin.values[i].index].name,key))
		{
			//found the searched key!

			t_sdl_bin_tuple * binvar=&(head->bin.values[i]);
			t_sdl_var * descvar=&(sdldesc->vars[head->bin.values[i].index]);

			PyObject **outputs = (PyObject **)malloc(sizeof(PyObject *)*binvar->array_count);
			char *outputformatstr = (char *)malloc(binvar->array_count+3);

			outputformatstr[0]='(';

			for(int j=0; j<binvar->array_count; j++)
			{
				outputformatstr[j+1]='O';
				switch(descvar->type)
				{
					case 0: //INT (4 bytes)
						outputs[j]=Py_BuildValue("i", *(int *)head->bin.values[i].data);
						break;
					case 1: //FLOAT (4 bytes)
						outputs[j]=Py_BuildValue("f", *(float *)head->bin.values[i].data);
						break;
					case 3: //STRING32 (32 bytes)
						outputs[j]=PyString_FromString((char *)head->bin.values[i].data);
						break;
					case 5: //Sub SDL
						//plog(log,"   Error: There a struct in the value list.\n");
						return NULL;
						//outputs[j]=NULL;
						break;
					case 2: //BOOL (1 byte)
					case 9: //BYTE (1 byte)
						outputs[j]=Py_BuildValue("b", *(char *)head->bin.values[i].data);
						break;
					case 0xA: //SHORT (2 bytes)
						outputs[j]=Py_BuildValue("h", *(short *)head->bin.values[i].data);
						break;
					case 8: //TIME (4+4 bytes)
						return NULL;
						//outputs[j]=NULL;
						//Py_BuildValue("i", *(int *)head->bin.values[i].data);
						break;
					case 50: //VECTOR3 (3 floats)
					case 51: //POINT3 (3 floats)
						return NULL;
						break;
					case 54: //QUATERNION (4 floats)
						return NULL;
						break;
					case 55: //RGB8 (3 bytes)
						return NULL;
						break;
					case 4: //PLKEY (UruObject)
						return NULL;
						break;
				};
			}
			outputformatstr[binvar->array_count+1]=')';
			outputformatstr[binvar->array_count+2]=0;

			PyObject * ret=Py_BuildValue_va(outputformatstr,binvar->array_count,(char *)outputs);
			free(outputs);
			return ret;
		}
		//else: didn't find the searched key
	}

	return NULL;
}

//Methods
static PyMethodDef ptSDL_methods[] = {
	{"__getitem__", (PyCFunction)ptSDL_getitem, METH_VARARGS,
		"Standard array accessor []."},
	{NULL, NULL, 0, NULL} /* sentinel */
};

static PyObject * ptSDL_getattr(PyObject * obj, char * name) {
	return Py_FindMethod(ptSDL_methods, (PyObject *)obj, name);
}

PyTypeObject ptSDL_Type = {
	PyObject_HEAD_INIT(NULL)
	0,
	"ptSDL",
	sizeof(ptSDL_Object),
	0,
	ptSDL_dealloc, /*tp_dealloc*/
	0, /*tp_print*/
	ptSDL_getattr, /*tp_getattr*/
	0, /*tp_setattr*/
	0, /*tp_compare*/
	0, /*tp_repr*/
	0, /*tp_as_number*/
	0, /*tp_as_sequence*/
	0, /*tp_as_mapping*/
	0 /*tp_hash*/
};
//end ptSDL class