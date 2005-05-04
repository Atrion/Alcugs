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

/* Build vars - Don't touch - NEVER */
#define __U_GLICENSE_ID "$Id$"

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <wx/html/htmlwin.h>

#ifdef __WIN32__
#include "windoze.h"
#endif

#include "license.h"

#include "glicense.h"

#include "debug.h"

BEGIN_EVENT_TABLE(gHtml,wxHtmlWindow)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(gLicenseText,wxTextCtrl)
END_EVENT_TABLE()

/** HTML widget
*/
gHtml::gHtml(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
	:wxHtmlWindow(parent,id,pos,size,style) {
	if(brownser.IsEmpty()) {
#ifdef __WIN32__
		//brownser="start \"\""; //this should work
		/* Note: "start "" "format c:"" is a nice link, so I have temporany disabled this thingie */
		brownser="";
		/* I need to search more, I will add a verification to ensure "http" on the links.
		 But I'm still not sure. Nobody knows how many other exploits may be hidden under http://address2exploit
		 Of course, this does not happen under Linux. Or at least I don't know any way to exploit a link that is directly passed to firefox. 
		 Well, the brownser command will be configurable from the settings page, so the user will be able to put the full address to his favourite brownser*/
#else
		brownser="firefox";
#endif
	}
}

void gHtml::OnLinkClicked(const wxHtmlLinkInfo& link) {
	printf("link:%s, target:%s clicked!\n",link.GetHref().c_str(),link.GetTarget().c_str());
	if(!strcmp(link.GetTarget().c_str(),"_blank")) {
		//TODO open link on Default Brownser
		//char buf[500];
		//sprintf(buf,"mozilla %s",link.GetTarget().c_str());
		//wxShell(buf);
		if(brownser.IsEmpty()) { return; }
		if(link.GetHref().Find("\"")!=-1) return;
		url=brownser + " \"" + link.GetHref() + "\"";
		wxShell(url);
	} else {
		//LoadPage(link.GetHref());
		wxHtmlWindow::OnLinkClicked(link);
	}
}

void gHtml::SetBrownser(wxString what) { brownser=what; }


/** License
*/
gLicenseText::gLicenseText(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
	:wxTextCtrl(parent,id,"",pos,size,wxTE_MULTILINE | wxTE_READONLY) {
	char * message1="\n\n\
--------------------------------------------------------------------------------\n\
DISCLAIMERS:\n\
All Myst, Riven and D'ni images and text © Cyan, Inc. All URU images © Cyan Worlds, Inc.\n\
Myst, Riven and D'ni are trademarks or registered trademarks of Cyan, Inc.\n\
URU is a trademark of Cyan Worlds, Inc. All rights reserved.\n\n\
Disclaimer: Alcugs is an unofficial fan/community project.\n\
\n";
	char * message2="\n\
This client has been created with the purpose of adding a new validation layer to the game, and give back to the shard admins full control of which clients, and which client mods are allowed in their shards.\n\n\
";
	AppendText(message2);
	AppendText(__uru_disclaimer_long);
	AppendText(message1);
}


