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

//vault operations
#define VConnect 0x01
#define VDisconnect 0x02
#define VAddNodeRef 0x03
#define VRemoveNodeRef 0x04
#define VNegotiateManifest 0x05
#define VSaveNode 0x06
#define VFindNode 0x07
#define VFetchNode 0x08
#define VSendNode 0x09
#define VSetSeen 0x0A
#define VOnlineState 0x0B

//data types
#define DCreatableGenericValue 0x0387
#define DCreatableStream       0x0389

//sub data types
#define DInteger 0x00 // (4 bytes) integer
#define DUruString 0x03 // string
#define DTimestamp 0x07 // (8 bytes) timestamp as double

	const char *alcVaultGetCmd(Byte cmd);
	const char *alcVaultGetTaskCmd(Byte cmd);
	const char *alcVaultGetDataType(U16 type);

	////DEFINITIONS
	class tvBase : public tBaseType {
	public:
		tvBase(void) : tBaseType() {}
		virtual void asHtml(tLog *log) = 0;
	};
	
	class tvCreatableGenericValue : public tvBase {
	public:
		tvCreatableGenericValue(void) : tvBase() { str.setVersion(5); /* inverted */ }
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
		virtual void asHtml(tLog *log);
	private:
		Byte format;
		S32 integer;
		tUStr str;
		double time;
	};
	
	class tvCreatableStream : public tvBase {
	public:
		tvCreatableStream(Byte id) : tvBase() { this->id = id; data = NULL; }
		virtual ~tvCreatableStream(void) { if (data) free(data); }
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
		virtual void asHtml(tLog *log);
	private:
		U32 size; // only defined when data != NULL
		Byte id;
		Byte *data;
	};
	
	class tvItem : public tvBase {
	public:
		tvItem(void) : tvBase() { data = NULL; }
		virtual ~tvItem(void) { if (data) delete data; }
		virtual void store(tBBuf &t);
		virtual int stream(tBBuf &t);
		virtual void asHtml(tLog *log);
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
		virtual void asHtml(tLog *log);
		void print(tLog *log, bool clientToServer, tNetSession *client);
	private:
		bool task;
		Byte cmd; //!< the vault command
		Byte compressed; //!< 1 when uncompressed, 3 when compressed
		U32 realSize; //!< the real size of the message
		U16 numItems; //!< number of items
		tvItem **items;
		U16 context; // sub in VaultTask
		U32 vmgr; // client in VaultTask
		U16 vn; // what is that?
		
	};
	
	
} //End alc namespace

#endif
