/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2006  The Alcugs Server Team                           *
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

/* CVS tag - DON'T TOUCH*/
#define __U_AUTHBACKEND_ID "$Id$"

#define _DBG_LEVEL_ 10

#include <alcugs.h>
#include <unet.h>

////extra includes
#include "authbackend.h"

#include <alcdebug.h>

namespace alc {

	tAuthBackend::tAuthBackend(void)
	{
		log = lnull;
	
		tConfig *cfg = alcGetConfig();
		tStrBuf var = cfg->getVar("auth.minalevel");
		if (var.isNull()) minAccess = 25;
		else minAccess = var.asU16();
		
		var = cfg->getVar("auth.att");
		if (var.isNull()) attempts = 6;
		else attempts = var.asU16();
		
		var = cfg->getVar("auth.distime");
		if (var.isNull()) disTime = 5*60;
		else disTime = var.asU16();
		
		var = cfg->getVar("auth.log");
		if (var.isNull() || var.asByte()) { // logging enabled per default
			log = new tLog("auth.log", 4, 0);
		}
		
		sql = tSQL::createFromConfig();
		
		log->log("Auth driver successfully started\n minimal access level: %d, max attempts: %d, disabled time: %d\n\n",
				minAccess, attempts, disTime);
	}
	
	int tAuthBackend::queryUser(Byte *login, Byte *passwd, Byte *guid)
	{
		passwd[0] = guid[0] = 0;
		if (strcmp((char *)login, "dakizo") == 0) { // only accept this username with this password... TODO: query database
			strcpy((char *)passwd, "76A2173BE6393254E72FFA4D6DF1030A"); // the md5sum of "passwd"
			strcpy((char *)guid, "7a9131b6-9dff-4103-b231-4887db6035b8");
			return 15;
		}
		else
			return -1;
	}

} //end namespace alc
