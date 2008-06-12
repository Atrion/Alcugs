/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2008  The Alcugs Server Team                           *
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

/**
	Description:
		This does this, and that.
	ChangeLog:
		Initial
	Bugs:
		Several
*/

#ifndef __U_VAULTROUTER_H
#define __U_VAULTROUTER_H
/* CVS tag - DON'T TOUCH*/
#define __U_VAULTROUTER_H_ID "$Id$"

namespace alc {

//data types
#define DCreatableGenericValue 0x0387
#define DCreatableStream       0x0389

	////DEFINITIONS
	class tvBase : public tBaseType {
	public:
		tvBase(void) : tBaseType() {}
	};
	
	class tvCreatableGenericValue : public tvBase {
	public:
		tvCreatableGenericValue(void) : tvBase() { str.setVersion(5); /* inverted */ }
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
	private:
		Byte format;
		S32 integer;
		tUStr str;
	};
	
	class tvCreatableStream : public tvBase {
	public:
		tvCreatableStream(void) : tvBase() {  }
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
	private:
		tMBuf data;
	};
	
	class tvItem : public tvBase {
	public:
		tvItem(void) : tvBase() { data = NULL; }
		virtual ~tvItem(void) { if (data) delete data; }
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
	private:
		Byte id;
		U16 type;
		tvBase *data;
	};
	
	class tvMessage : public tvBase {
	public:
		tvMessage(bool isTask) : tvBase() { task = isTask; items = NULL; }
		virtual ~tvMessage(void);
		virtual void store(tBBuf &t); //!< unpacks the message
		virtual int stream(tBBuf &t);
	private:
		bool task;
		Byte cmd; //!< the vault command
		Byte compressed; //!< 1 when uncompressed, 3 when compressed
		U32 realSize; //!< the real size of the message
		U16 numItems; //!< number of items
		tvItem **items;
		U16 context;
		U32 vmgr;
		U16 vn; //!< what is that?
		
	};
	
	
} //End alc namespace

#endif
