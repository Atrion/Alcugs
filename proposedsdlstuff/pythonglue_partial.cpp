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
	t_sdl_head * bin_head;
} ptSDL_Object;
#endif



//begin ptSDL class

//this function will never be called from an external file
//as currently only the PtGetAgeSDL() function is allowed
//to create ptSDL objects
static PyObject * ptSDL_new(PyObject * self,PyObject * args) {
	ptSDL_Object * ptSDL;

	if(!PyArg_ParseTuple(args, ""))
		return NULL;

	ptSDL = PyObject_New(ptSDL_Object, &ptSDL_Type);
	//constructors

	ptSDL->bin_head=0;

	return (PyObject *)ptSDL;
}

static void ptSDL_dealloc(PyObject * self) {
	//Destructors
	PyObject_Del(self);
}

static int ptSDL_count(PyObject *self)
{
	if(((ptSDL_Object *)self)->bin_head==0)
		return -1;

	return  ((ptSDL_Object *)self)->bin_head->bin.n_values
	       +((ptSDL_Object *)self)->bin_head->bin.n_structs;
}

static PyObject* ptSDL_getitem(PyObject *self, PyObject *args)
{
	char * key;
	key=PyString_AsString(args);
	//if(!PyArg_ParseTuple(args,"s",&key))
	//	return NULL;

	t_sdl_head *head = ((ptSDL_Object *)self)->bin_head;

	if(head==0)
		return NULL;

	int sdlindex=find_sdl_descriptor(head->name,head->version,global_sdl_def,global_sdl_def_n);

	t_sdl_def *sdldesc=&global_sdl_def[sdlindex];

	for(int i=0; i<head->bin.n_values; i++)
	{
		if(!strcmp((char *)&sdldesc->vars[head->bin.values[i].index].name,key))
		{
			//found the searched key!

			t_sdl_bin_tuple * binvar=&(head->bin.values[i]);
			t_sdl_var * descvar=&(sdldesc->vars[head->bin.values[i].index]);

			int array_count;
			if(descvar->array_size)
				array_count=descvar->array_size;
			else
				array_count=binvar->array_count;

			PyObject **outputs = (PyObject **)malloc(sizeof(PyObject *)*array_count);

			for(int j=0; j<array_count; j++)
			{
				switch(descvar->type)
				{
					case 0: //INT (4 bytes)
						if(binvar->flags | 0x08)
							outputs[j]=Py_BuildValue("i", atoi((char *)&descvar->default_value)); //use default value
						else
							outputs[j]=Py_BuildValue("i", *(int *)head->bin.values[i].data);

						break;
					case 1: //FLOAT (4 bytes)
						if(binvar->flags | 0x08)
							outputs[j]=Py_BuildValue("f", (float)atof((char *)&descvar->default_value)); //use default value
						else
							outputs[j]=Py_BuildValue("f", *(float *)head->bin.values[i].data);
						break;
					case 3: //STRING32 (32 bytes)
						if(binvar->flags | 0x08)
							outputs[j]=PyString_FromString((char *)&descvar->default_value); //use default value
						else
							outputs[j]=PyString_FromString((char *)head->bin.values[i].data);
						break;
					case 5: //Sub SDL
						//plog(log,"   Error: There a struct in the value list.\n");
						free(outputs);
						return NULL;
						//outputs[j]=NULL;
						break;
					case 2: //BOOL (1 byte)
					case 9: //BYTE (1 byte)
						if(binvar->flags | 0x08)
							outputs[j]=Py_BuildValue("b", (char)atoi((char *)&descvar->default_value)); //use default value
						else
							outputs[j]=Py_BuildValue("b", *(char *)head->bin.values[i].data);
						break;
					case 0xA: //SHORT (2 bytes)
						if(binvar->flags | 0x08)
							outputs[j]=Py_BuildValue("h", (short)atoi((char *)&descvar->default_value)); //use default value
						else
							outputs[j]=Py_BuildValue("h", *(short *)head->bin.values[i].data);
						break;
					case 8: //TIME (4+4 bytes)
						free(outputs);
						return NULL;
						//outputs[j]=NULL;
						//Py_BuildValue("i", *(int *)head->bin.values[i].data);
						break;
					case 50: //VECTOR3 (3 floats)
					case 51: //POINT3 (3 floats)
						free(outputs);
						return NULL;
						break;
					case 54: //QUATERNION (4 floats)
						free(outputs);
						return NULL;
						break;
					case 55: //RGB8 (3 bytes)
						free(outputs);
						return NULL;
						break;
					case 4: //PLKEY (UruObject)
						free(outputs);
						return NULL;
						break;
				};
			}

			char *outputformatstr = (char *)malloc(array_count+3);

			outputformatstr[0]='(';
			memset(outputformatstr+1,'O',array_count);
			outputformatstr[array_count+1]=')';
			outputformatstr[array_count+2]=0;

			PyObject * ret=Py_VaBuildValue(outputformatstr,(char *)outputs);
			free(outputformatstr);
			free(outputs);
			return ret;
		}
		//else: didn't find the searched key
	}
	char errormsg[512];
	snprintf((char *)&errormsg,511,"didn't find the searched key (\"%s\")",key);
	PyErr_SetString(PyExc_KeyError,(char *)&errormsg);
	return NULL;
}

//Methods
static PyMappingMethods ptSDL_mapping = {
	(inquiry)		ptSDL_count, /*mp_length*/
	(binaryfunc)	ptSDL_getitem, /*mp_subscript*/
	(objobjargproc)	0, /*mp_ass_subscript*/
};

static PyMethodDef ptSDL_methods[] = {

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
	&ptSDL_mapping, /*tp_as_mapping*/
	0 /*tp_hash*/
};
//end ptSDL class