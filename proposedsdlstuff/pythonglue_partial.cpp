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

#define OOM_CHECK(var,todo)	if(!var) \
							{ \
								snprintf((char *)&errormsg,511,"out of memory"); \
								PyErr_SetString(PyExc_MemoryError,(char *)&errormsg); \
								todo \
								return NULL; \
							}

static PyObject* ptSDL_getitem(PyObject *self, PyObject *args)
{
	char errormsg[512];
	char * key;
	key=PyString_AsString(args);
	if(key==NULL)
		return NULL;
	//if(!PyArg_ParseTuple(args,"s",&key))
	//	return NULL;

	t_sdl_head *head = ((ptSDL_Object *)self)->bin_head;
	if(head==0)
	{
		snprintf((char *)&errormsg,511,"a really strange error has occured (ptSDL->bin_head was 0)");
		PyErr_SetString(PyExc_Exception,(char *)&errormsg);
		return NULL;
	}

	int sdlindex=find_sdl_descriptor(head->name,head->version,global_sdl_def,global_sdl_def_n);

	if(sdlindex<0)
	{
		snprintf((char *)&errormsg,511,"a really strange error has occured (did not find the searched SDL descriptor)");
		PyErr_SetString(PyExc_Exception,(char *)&errormsg);
		return NULL;
	}

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
			OOM_CHECK(outputs)

			for(int j=0; j<array_count; j++)
			{
				switch(descvar->type)
				{
					case 0: //INT (4 bytes)
						if(binvar->flags & 0x08)
							outputs[j]=Py_BuildValue("i", atoi((char *)&descvar->default_value)); //use default value
						else
							outputs[j]=Py_BuildValue("i", ((int *)head->bin.values[i].data)[j]);

						break;
					case 1: //FLOAT (4 bytes)
						if(binvar->flags & 0x08)
							outputs[j]=Py_BuildValue("f", (float)atof((char *)&descvar->default_value)); //use default value
						else
							outputs[j]=Py_BuildValue("f", ((float *)head->bin.values[i].data)[j]);
						break;
					case 3: //STRING32 (32 bytes)
						if(binvar->flags & 0x08)
							outputs[j]=PyString_FromString((char *)&descvar->default_value); //use default value
						else
							outputs[j]=PyString_FromString((char *)head->bin.values[i].data+32*j);
						break;
#if 0
					case 5: //Sub SDL
						//plog(log,"   Error: There a struct in the value list.\n");
						free(outputs);
						return NULL;
						//outputs[j]=NULL;
						break;
#endif
					case 2: //BOOL (1 byte)
					case 9: //BYTE (1 byte)
						if(binvar->flags & 0x08)
							outputs[j]=Py_BuildValue("b", (char)atoi((char *)&descvar->default_value)); //use default value
						else
							outputs[j]=Py_BuildValue("b", ((char *)head->bin.values[i].data)[j]);
						break;
					case 0xA: //SHORT (2 bytes)
						if(binvar->flags & 0x08)
							outputs[j]=Py_BuildValue("h", (short)atoi((char *)&descvar->default_value)); //use default value
						else
							outputs[j]=Py_BuildValue("h", ((short *)head->bin.values[i].data)[j]);
						break;
#if 0
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
#endif
					default:
						free(outputs);
						snprintf((char *)&errormsg,511,"the requested type %s can not be copied to a Python value yet",sdl_get_var_type_nspc(descvar->type));
						PyErr_SetString(PyExc_NotImplementedError,(char *)&errormsg);
						return NULL;
				};
			}

			char *outputformatstr = (char *)malloc(array_count+3);
			OOM_CHECK(outputformatstr,free(outputs);)

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
	snprintf((char *)&errormsg,511,"didn't find the searched key (\"%s\")",key);
	PyErr_SetString(PyExc_KeyError,(char *)&errormsg);
	return NULL;
}
#undef OOM_CHECK


#define OOM_CHECK(var,todo)	if(!var) \
							{ \
								snprintf((char *)&errormsg,511,"out of memory"); \
								PyErr_SetString(PyExc_MemoryError,(char *)&errormsg); \
								todo \
								return -1; \
							}

static int ptSDL_setitem(PyObject *self, PyObject *args, PyObject *values)
{
	char errormsg[512];
	char * key;
	key=PyString_AsString(args);
	if(key==NULL)
		return -1;

	t_sdl_head *head = ((ptSDL_Object *)self)->bin_head;

	if(head==0)
	{
		snprintf((char *)&errormsg,511,"a really strange error has occured (ptSDL->bin_head was 0)");
		PyErr_SetString(PyExc_Exception,(char *)&errormsg);
		return -1;
	}

	int sdlindex=find_sdl_descriptor(head->name,head->version,global_sdl_def,global_sdl_def_n);

	if(sdlindex<0)
	{
		snprintf((char *)&errormsg,511,"a really strange error has occured (did not find the searched SDL descriptor)");
		PyErr_SetString(PyExc_Exception,(char *)&errormsg);
		return -1;
	}

	t_sdl_def *sdldesc=&global_sdl_def[sdlindex];

	for(int i=0; i<head->bin.n_values; i++)
	{
		if(!strcmp((char *)&sdldesc->vars[head->bin.values[i].index].name,key))
		{
			//found the searched key!

			if(!PyTuple_Check(values))
			{
				snprintf((char *)&errormsg,511,"the value is not a tuple");
				PyErr_SetString(PyExc_TypeError,(char *)&errormsg);
				return -1;
			}

			t_sdl_bin_tuple * binvar=&(head->bin.values[i]);
			t_sdl_var * descvar=&(sdldesc->vars[head->bin.values[i].index]);

			int array_count;

			if(descvar->array_size)
			{
				int tuplecount=PyTuple_Size(values);
				if(descvar->array_size!=tuplecount)
				{
					snprintf((char *)&errormsg,511,"tuple item count does not match the expected number %i",tuplecount);
					PyErr_SetString(PyExc_ValueError,(char *)&errormsg);
					return -1;
				}
				array_count=descvar->array_size;
			}
			else
				array_count=PyTuple_Size(values);

			char parsechar;
			size_t tupleitemsize;
			switch(descvar->type)
			{
				case 0: //INT (4 bytes)
					parsechar='i';
					tupleitemsize=sizeof(int);
					break;
				case 1: //FLOAT (4 bytes)
					parsechar='f';
					tupleitemsize=sizeof(float);
					break;
				case 3: //STRING32 (32 bytes)
					parsechar='s';
					tupleitemsize=sizeof(PyObject *);
					break;
#if 0
				case 5: //Sub SDL
					return -1;
					break;
#endif
				case 2: //BOOL (1 byte)
				case 9: //BYTE (1 byte)
					parsechar='b';
					tupleitemsize=sizeof(char);
					break;
				case 0xA: //SHORT (2 bytes)
					parsechar='h';
					tupleitemsize=sizeof(short);
					break;
#if 0
				case 8: //TIME (4+4 bytes)
					return -1;
					break;
				case 50: //VECTOR3 (3 floats)
				case 51: //POINT3 (3 floats)
					return -1;
					break;
				case 54: //QUATERNION (4 floats)
					return -1;
					break;
				case 55: //RGB8 (3 bytes)
					return -1;
					break;
				case 4: //PLKEY (UruObject)
					return -1;
					break;
#endif
				default:
					snprintf((char *)&errormsg,511,"the requested type %s can not be changed by Python yet",sdl_get_var_type_nspc(descvar->type));
					PyErr_SetString(PyExc_NotImplementedError,(char *)&errormsg);
					return -1;
			}

			char *parsestr=(char *)malloc(array_count+1);
			OOM_CHECK(parsestr)

			memset(parsestr,parsechar,array_count);
			parsestr[array_count]=0;

			void *tuple=malloc(tupleitemsize*array_count);
			OOM_CHECK(parsestr,free(parsestr);)

			//this looks like crap but works like charm! ;-)
			void *tupleptrs=malloc(sizeof(size_t)*array_count);
			OOM_CHECK(parsestr,free(parsestr); free(tuple);)

			size_t *tupleptr=(size_t *)tupleptrs;
			for(int j=0; j<array_count; j++)
			{
				tupleptr[j]=(size_t)((size_t)tuple+j*tupleitemsize);
			}

			if(!PyArg_VaParse(values,parsestr,(char *)tupleptrs))
			{
				//!TODO add frees
				free(tupleptrs);
				free(parsestr);
				free(tuple);
				return -1;
			}

			free(tupleptrs);

			free(parsestr);

			for(j=0; j<array_count; j++)
			{
				switch(descvar->type)
				{
					case 0: //INT (4 bytes)
						if(((int *)tuple)[j]!=atoi((char *)&descvar->default_value))
							goto copystuff;
						break;
					case 1: //FLOAT (4 bytes)
						if(((float *)tuple)[j]!=(float)atof((char *)&descvar->default_value))
							goto copystuff;
						break;
					case 3: //STRING32 (32 bytes)
						if(strcmp(PyString_AsString(&((PyObject *)tuple)[j]),(char *)&descvar->default_value))
							goto copystuff;
						break;
#if 0
					case 5: //Sub SDL
						return 1;
						break;
#endif
					case 2: //BOOL (1 byte)
					case 9: //BYTE (1 byte)
						if(((char *)tuple)[j]!=(char)atoi((char *)&descvar->default_value))
							goto copystuff;
						break;
					case 0xA: //SHORT (2 bytes)
						if(((short *)tuple)[j]!=(short)atoi((char *)&descvar->default_value))
							goto copystuff;
						break;
#if 0
					case 8: //TIME (4+4 bytes)
						return 1;
						break;
					case 50: //VECTOR3 (3 floats)
					case 51: //POINT3 (3 floats)
						return 1;
						break;
					case 54: //QUATERNION (4 floats)
						return 1;
						break;
					case 55: //RGB8 (3 bytes)
						return 1;
						break;
					case 4: //PLKEY (UruObject)
						return 1;
						break;
#endif
				}
			}

			//the values are equal to the defaults

			free(tuple);

			if(binvar->data_size!=0)
			{
				free(binvar->data);
				binvar->data=0;
				binvar->data_size=0;
			}

			binvar->array_count=0;

			binvar->flags|=0x08;

			return 0;

copystuff:
			if(binvar->data_size==0)
			{
				binvar->data=malloc(tupleitemsize*array_count);
				binvar->data_size=tupleitemsize*array_count;
			}
			memcpy(binvar->data,tuple,tupleitemsize*array_count);

			free(tuple);

			binvar->array_count=array_count;

			binvar->flags&=~0x08;

			return 0;
		}
		//else: didn't find the searched key
	}
	snprintf((char *)&errormsg,511,"didn't find the searched key (\"%s\")",key);
	PyErr_SetString(PyExc_KeyError,(char *)&errormsg);

	return -1;
}
#undef OOM_CHECK


//Methods
static PyMappingMethods ptSDL_mapping = {
	(inquiry)		ptSDL_count, /*mp_length*/
	(binaryfunc)	ptSDL_getitem, /*mp_subscript*/
	(objobjargproc)	ptSDL_setitem, /*mp_ass_subscript*/
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