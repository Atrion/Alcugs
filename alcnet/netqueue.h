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

/**
	URUNET 3+
*/

#ifndef __U_NETMSGQ_H
#define __U_NETMSGQ_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETMSGQ_H_ID "$Id$"

#include <list>

namespace alc {

template <typename T> class tNetQeue : public std::list<T *>
{
public:
	~tNetQeue() { clear(); }
	void clear(void) {
		for (typename std::list<T *>::iterator it = std::list<T *>::begin(); it != std::list<T *>::end(); ++it) {
			delete *it;
		}
		std::list<T *>::clear();
	}
	T *pop_front(void) {
		T *t = std::list<T *>::front();
		std::list<T *>::pop_front();
		return t;
	}
	typename std::list<T *>::iterator eraseAndDelete(typename std::list<T *>::iterator it) {
		delete *it;
		return std::list<T *>::erase(it);
	}
};

}

#endif
