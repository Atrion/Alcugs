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
	URUNET 3+
*/

#ifndef __U_NETMSGQ_H
#define __U_NETMSGQ_H
/* CVS tag - DON'T TOUCH*/
#define __U_NETMSGQ_H_ID "$Id$"

namespace alc {

#if 0
class tUnetOutMsgQ {
public:
	tUnetOutMsgQ() {
		first=last=current=prev=NULL;
		n=0;
	}
	~tUnetOutMsgQ() {
		clear();
	}
	void clear() {
		current=first;
		while(current) {
			first=first->next;
			delete current;
			current=first;
		}
		first=last=current=prev=NULL;
		n=0;
	}
	tUnetUruMsg * getNext() {
		if(current==NULL) prev=current=first;
		else {
			prev=current;
			current=current->next;
		}
		return current;
	}
	void deleteCurrent() {
		if(current) {
			if(current==first) {
				prev=first=first->next;
				delete current;
				current=prev;
			} else {
				prev->next=current->next;
				if(current==last) last=prev;
				delete current;
				current=prev->next;
			}
			n--;
		}
	}
	void add(tUnetUruMsg * msg) {
		if(first==NULL) { 
			first=msg;
		} else {
			last->next=msg;
		}
		last=msg;
		n++;
	}
	void rewind() {
		prev=current=NULL;
	}
	U32 len() { return n; }
private:
	tUnetUruMsg * first;
	tUnetUruMsg * last;
	tUnetUruMsg * current;
	tUnetUruMsg * prev;
	U32 n;
};
#endif

template <class T> class tUnetMsgQ {
public:
	tUnetMsgQ() {
		first=last=current=prev=NULL;
		n=0;
	}
	~tUnetMsgQ() {
		clear();
	}
	void clear() {
		current=first;
		while(current) {
			first=first->next;
			delete current;
			current=first;
		}
		first=last=current=prev=NULL;
		n=0;
	}
	T* getNext() {
		if(current==NULL) prev=current=first;
		else {
			prev=current;
			current=current->next;
		}
		return current;
	}
	T* getCurrent() {
		return current;
	}
	T* unstackNext() {
		T* ret=NULL;
		if(current==NULL) prev=current=first;
		else {
			prev=current;
			current=current->next;
		}
		if(current) {
			if(current==first) {
				prev=first=first->next;
				ret=current;
				current=prev;
			} else {
				prev->next=current->next;
				if(current==last) last=prev;
				ret=current;
				current=prev->next;
			}
			n--;
		}
		return ret;
	}
	void deleteCurrent() {
		if(current) {
			if(current==first) {
				prev=first=first->next;
				delete current;
				current=prev;
			} else {
				prev->next=current->next;
				if(current==last) last=prev;
				delete current;
				current=prev->next;
			}
			n--;
		}
	}
	void add(T * msg) {
		if(first==NULL) { 
			first=msg;
		} else {
			last->next=msg;
		}
		last=msg;
		n++;
	}
	void insert(T * msg) {
		if(first==NULL) {
			prev=current=first=msg;
		} else {
			prev=current;
			msg->next=prev->next;
			prev->next=msg;
			current=msg;
		}
		if(msg->next==NULL) last=msg;
		n++;
	}
	void insertBefore(T * msg) {
		if(first==NULL) {
			prev=first=last=msg;
			current=NULL;
		} else if(current==NULL || current==first) {
			msg->next=first;
			prev=first=msg;
		} else {
			msg->next=current;
			prev->next=msg;
			prev=msg;
		}
		n++;
	}
	void rewind() {
		prev=current=NULL;
	}
	bool isEmpty() {
		return(first==NULL);
	}
	U32 len() { return n; }
private:
	T* first;
	T* last;
	T* current;
	T* prev;
	U32 n;
};


}

#endif