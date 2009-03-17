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

#ifndef __U_PLBASE_H
#define __U_PLBASE_H
/* CVS tag - DON'T TOUCH*/
#define __U_PLBASE_H_ID "$Id$"

namespace alc {

#define plControlEventMsg          0x0210
#define plLoadCloneMsg             0x024E
#define plEnableMsg                0x024F
#define plWarpMsg                  0x0250
#define plServerReplyMsg           0x026A
#define plAvTaskMsg                0x0293
#define plNotifyMsg                0x02E8
#define plLinkEffectsTriggerMsg    0x02FB
#define plParticleTransferMsg      0x032E
#define plAvatarInputStateMsg      0x0342
#define pfKIMsg                    0x035F
#define plAvBrainGenericMsg        0x038A
#define plLoadAvatarMsg            0x03AC
#define plNull                     0x8000

	////DEFINITIONS
	class tpObject;
	
	const char *alcGetPlasmaType(U16 type);
	tpObject *alcCreatePlasmaObject(U16 type, bool mustBeComplete = true);
	
	class tpObject : public tBaseType {
	public:
		tpObject(U16 type) : tBaseType(), type(type) {}
		virtual void store(tBBuf &t) {}
		virtual void stream(tBBuf &t) {}
		
		inline U16 getType(void) { return type; }
		const char *str(void);
	protected:
		virtual void toString(void) {}
		tStrBuf strBuf;
	private:
		U16 type;
	};

} //End alc namespace

#endif
