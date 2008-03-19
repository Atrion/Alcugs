/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2005  The Alcugs H'uru Server Team                     *
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

#ifndef __U_ALCPARSER_H
#define __U_ALCPARSER_H
/* CVS tag - DON'T TOUCH*/
#define __U_ALCPARSER_H_ID "$Id$"

namespace alc {

class tParserBase :public tBaseType {
public:
	virtual void store(tStrBuf &t)=0;
	virtual int stream(tStrBuf &t)=0;
};

/**
	Simple parser
	key value
	key = value
	# comment
	; comment
*/
class tSimpleParser :public tParserBase {
public:
	tSimpleParser();
	virtual void store(tBBuf &t);
	virtual int stream(tBBuf &t);
	virtual void store(tStrBuf &t);
	virtual int stream(tStrBuf &t);
	/**
		\brief Computes the size, runs the same code as stream
	*/
	virtual U32 size();
	void setConfig(tConfig * c);
	tConfig * getConfig();
protected:
	tConfig * cfg;
	char sep;
};

/**
	Alcugs parser
	[key]
	key = value
	key = "value"
	key = "value1",value2
	key[1] = "var1","var2"
*/
class tXParser :public tSimpleParser {
public:
	tXParser();
	virtual ~tXParser() {}
	virtual void store(tStrBuf &t);
	virtual int stream(tStrBuf &t);
	void setBasePath(tStrBuf & base);
private:
	U16 parseKey(tStrBuf &t);
	tStrBuf path;
};

} //End alc namespace

#endif