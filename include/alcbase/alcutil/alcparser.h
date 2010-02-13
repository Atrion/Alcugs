/*******************************************************************************
*    Alcugs H'uru server                                                       *
*                                                                              *
*    Copyright (C) 2004-2010  The Alcugs H'uru Server Team                     *
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

class tStringTokenizer {
public:
	tStringTokenizer(const tString &text) : str(text), line(0), col(0), sep('=') { str.rewind(); }

	inline U16 getLineNum() { return line; }
	inline U16 getColumnNum() { return col; }
	inline void setSeparator(char w) { sep=w; }
	inline bool eof(void) { return str.eof(); }
	
	/** \brief returns a line
			\param nl If true, it will also append the \\n if it's present
			\param slash If false, a \\n followed by an slash will be ignored
			\return A tString object
	*/
	tString getLine(bool nl=false,bool slash=false);
	/** \brief returns a token (newline, key, value, separator - but not a space)
			\return A tStBuf object
	*/
	tString getToken();
private:
	tString str;
	U16 line, col;
	char sep;
};

/**
	Simple parser
	key value
	key = value
	# comment
	; comment
*/
class tSimpleParser : public tBaseType {
public:
	tSimpleParser();
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual void store(const tString &str);
	virtual void stream(tString &str) const;
	/**
		\brief Computes the size, runs the same code as stream
	*/
	U32 size() const;
	void setConfig(tConfig * c);
	tConfig * getConfig() const { return cfg; }
protected:
	tConfig * cfg;
};

/**
	Alcugs parser
	[key]
	key = value
	key = "value"
	key = "value1",value2
	key[1] = "var1","var2"
*/
class tXParser : public tSimpleParser {
public:
	tXParser(bool override = true);
	virtual ~tXParser() {}
	virtual void store(tBBuf &t);
	virtual void stream(tBBuf &t) const;
	virtual void store(const tString &str);
	virtual void stream(tString &str) const;
	void setBasePath(const tString & base);
private:
	bool override;
	tString path;
};

} //End alc namespace

#endif
