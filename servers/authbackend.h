/*******************************************************************************
*    Alcugs Server                                                             *
*                                                                              *
*    Copyright (C) 2004-2011  The Alcugs Server Team                           *
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

#ifndef __U_AUTHBACKEND_H
#define __U_AUTHBACKEND_H
/* CVS tag - DON'T TOUCH*/
#define __U_AUTHBACKEND_H_ID "$Id$"

#include <alcutil/alclog.h>

namespace alc {
	
	class tSQL;
	class tNetSession;
	
	class tAuthBackend {
	public:
		tAuthBackend(void);
		~tAuthBackend(void);
		
		int authenticatePlayer(tNetSession *u, const tString &login, const tString &challenge, const tString &hash, uint8_t release, const tString &ip, tString *passwd, uint8_t *hexUid, uint8_t *accessLevel); //!< authenticates the player
	private:
		uint16_t minAccess, disTime, maxAttempts;
		tString cgasServer, cgasPath;
		uint16_t cgasPort;
		uint8_t cgasDefaultAccess;
		time_t cgasMaxCacheTime;
		tSQL *sql;
		tLog log;
		
		typedef enum { kSuccess, kError, kNotFound, kCacheTooOld } tQueryResult;
		
		tString calculateHash(const tString &login, const tString &passwd, const tString &challenge); //!< calculate the hash needed to check the password
		tQueryResult queryPlayer(const tString &login, tString *passwd, tString *guid, uint32_t *attempts, uint32_t *lastAttempt, uint8_t *accessLevel);
		void updatePlayer(const tString &guid, const tString &ip, uint32_t attempts, uint8_t updateStamps); // updateStamps can be: 0 = don't update any stamp, 1 = update only last attempt, 2 = update last attempt and last login
		tQueryResult queryCgas(const tString &login, const tString &challenge, const tString &hash, bool hasCache, tString *passwd, tString *guid, uint8_t *accessLevel);
		
		// CGAS internal functions
		tString sendCgasRequest(const tString &login, const tString &challenge, const tString &hash); //!< send a CGAS request and \return the reply or an empty String in case of an error
		tQueryResult parseCgasResponse(const tString &response, tString *passwd, tString *guid); //!< parse the CGAS reply and \return true if the response was valid (it could still say that the login was wrong though!), false if not
	};
	
} //End alc namespace

#endif
