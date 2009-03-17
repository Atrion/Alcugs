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

/** VERY IMPORTANT!!!!!!!!!!!

	You can do anything that you want with this code, under the terms of the GPL.
	But, we ask some things.
	
	1) Please, DO NOT release any patcher as an official patcher to be used on Public Alcugs shards.
	2) Several Alcugs shard admins, agree to use the same game data files.
	3) Using different datafiles, and different patchers, can cause:
		A) Confussion. General public confussion. A caos on client versions.
		B) Headache Debbuging the servers, you never will know if the server crashed by some specific
		python code introduced by player XX, or if it crashed because someone else did a small fix
		on a specific age.
	
	Also, you will win that some shards, may or not may ban you.
	
	
	The good way to do this.
	1) Do anything that you want on your own shard. (You are the owner)
	2) Ask permission to a shard owner. There is a testing shard for testing the patcher, and new
	game content. Testing this on public shards is not allowed.
	3) Send the patches (PRP, python, et all) to the Alcugs development team (until we have another
	team working only on releasing/fixing game content). 
	Note: We are going to accept almost everything except:
		A) Content that violates Cyan copyrights, EULA, and other agreements.
		B) Content not suitable for people under 18 years. All must be suitable for all ages.
		C) Non-IC content. Things that will destroy the In Character atmosphere.
	4) If you are still going to ignore anything stated here, and you are going to release a patcher,
	then you must:
		A) WARN the user that is using a patcher from another group. (You must rename the patcher, and
		version number)
		B) WARN the user that he cannot use this other client on any other public shard. (The user
		must have a separate installation for that client version).
		C) WARN in the login page that you must use your own custom patcher and not the Alcugs ones.
		D) You must use another dataset unique identificator, to avoid collision with other content.
		

When the server side game data validation system is finished then this notice will be removed.


	for any comments:
		almol AT users DOT sourceforge DOT net
		http://alcugs.almlys.dyns.net/forum
		http://alcugs.almlys.dyns.net/forum/profile.php?mode=viewprofile&u=2

*/


/* Build vars - Don't touch - NEVER */
#include "xversion.h"
const char * ID = "$Id$";
const char * BUILD =  __DATE__ " " __TIME__;
const char * SNAME = "Alcugs patcher";
const char * VERSION = alcSTR_VER; //"1.0.1d";
//Now all app's will have same version as the entire distro

//#define _DBG_LEVEL_ 10

#include "config.h"

#include <wx/wx.h>
#include <wx/wizard.h>
#include <wx/notebook.h>
#include <wx/image.h>
//#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/html/htmlwin.h>
#include <wx/html/htmlproc.h>
#include <wx/fs_inet.h>
#include <wx/filedlg.h>
#include <wx/sysopt.h>
#include <wx/settings.h>
#include <wx/window.h>
#include <wx/thread.h>

    #include <sys/stat.h>
       #include <sys/types.h>
       #include <fcntl.h>
       #include <unistd.h>




#ifdef __WIN32__
#include "windoze.h"
#endif

#include "license.h"

#include "conv_funs.h"
#include "useful.h"
#include "md5.h"

#include "urustructs.h"
#include "prpstuff.h"
#include "uobjectdesc.h"
#include "prpreader.h"
#include "kipatch.h"

#include "glicense.h"

#include "alctypes.h"
#include "alcpackage.h"

#include "alcpatch.h"

#include "debug.h"

using namespace alc;

IMPLEMENT_APP(gMainApp)

/** My suggestion, is to read this slowly and with calm. */

//ID's
enum {
	ID_BUTTON_UU = wxID_HIGHEST+1,
	ID_BUTTON_TPOTS,
}; //2

/** I warned you. - This code is pure shit in pure state */

class gPatcherPage;

class PatcherThread: public wxThread {
public:
	PatcherThread(gPatcherPage * p) :wxThread() {
		parent=p;
		well=true;
	}
	virtual void *Entry() {
		try {
			DoPatch();
			Ok();
			PatchOk();
		}
		catch(NotFound & t) {
			Log(wxString("Cannot find required file: ") + wxString(t.what()));
			PatchNotOk();
		}
		catch(...) {
			PatchNotOk();
		}
		return NULL;
	}
	void Update();
	virtual void OnExit() {}
	void Log(const wxString& text);
	void ClearLog();
	void UpdatePrgTop(int t);
	void UpdatePgr(int t);
	void PatchOk();
	void PatchNotOk();
	void DoPatch() {
		pkg=&wxGetApp().pkg;
		//1st Check Md5 sum of files to patch
		ClearLog();
		Log("Init ...\n");
		bak_path=wxGetApp().tpots_path + wxT("\\alcBackup");

		pgr_val=0;
		pgr_top=1144;
		UpdatePrgTop(pgr_top);
		Md5SumCheck();
		#if 0
		this->UpdatePrgTop(10000);
		for (int i=0; i<10000; i++) {
			usleep(1000);
			//wxGetApp()->Yield(true);
			UpdatePgr(i);
			Update();
			/*
			myparent->UpdateWindowUI();
			myparent->Update();
			myparent->Refresh();
			myparent->Layout();
			UpdateWindowUI();
			Update();
			Refresh();
			Layout();
			*/
			printf("%i\n",i);
		}
		#endif
		SomeFiles();
		
		//2nd PRP patches
		// A) Copy old prp to bakcup dir
		// B) Patch file
		PrpPatch();
		
		//3rd Python
		// A) Extract xKi.py from UU
		// B) Extract xKi.py from Tpots
		// C) UU_xKi.py + delta = deltaC
		// D) TPOTS_xKi.py + deltaC = Final xKi.py file
		PythonPatch();
		
		//4th Update Md5Sums
		UpdateSums();
		
		Log("Finished\n");
		
	}
	void Md5SumCheck() {
		Log("Checking files...\n");
		//mbuf * cbuf;
		strbuf * a;
		pkgfile * pfile;
		Log("Openning live.md5...\n");
		pfile=pkg->find("live.md5");
		if(pfile==NULL) Fatal("live.md5 not found!");
		Log("Parsing live.md5...\n");
		a=new strbuf(pfile->pkg);
		
		wxString curfile,fname,strmd5,path;
		
		fbuf f1;
		md5buf d5;
		
		IncPgr(1);
		
		while(!a->eof()) {
			//Log("Got:");
			//Log(a->getLine());
			curfile=wxString(a->getLine());
			strmd5=curfile.Mid(0,0x20);
			fname=curfile.Mid(0x21).Trim();
			if(fname=="") break;
			Log(wxT("Checking ") + fname + wxT(" ") + strmd5);
			
			path=wxGetApp().uu_path + wxT("\\") + fname;
			
			f1.open(path.c_str());
			d5.rewind();
			d5.clear();
			d5.put(f1);
			d5.compute();
			f1.close();
			//Log("\n");
			//Log(wxString(d5.HexToAscii()));
			//Log("\n");
			//Log(wxString(strmd5));
			
			if(wxString(d5.HexToAscii())==strmd5) Log(" Ok\n");
			else { 
				Log("\n"); 
				wxMessageBox(wxT("Mismatch ") + wxString(path) + wxString("\nExpected: ") + wxString(strmd5) + wxString("\n     Got: ") + wxString(d5.HexToAscii()), "Unexpected file signature", wxOK | wxICON_ERROR);
				Fatal(wxString("Checksum Mismatch: ") + wxString(d5.HexToAscii())); 
			}
			
			IncPgr(1);
		}
		
		IncPgr(1);
		
		delete a;

		Log("Openning tpots.md5...\n");
		pfile=pkg->find("tpots.md5");
		if(pfile==NULL) Fatal("tpots.md5 not found!");
		Log("Parsing tpots.md5...\n");
		a=new strbuf(pfile->pkg);
		
		mkdir(bak_path.c_str(),755);
		
		wxString zzz=bak_path + "\\dat";
		mkdir(zzz.c_str(),755);
		
		path=bak_path + wxT("\\A_README.TXT");
		Log(wxString("Openning ") + path + (" ...\n"));
		f1.open(path.c_str(),"wb");
		strbuf bb;
		bb.writeStr("The files in this folder, are a backup of the patched ones, with the purpose of compatibility with future versions of this patcher. You can delete this folder to save Space, but if you do, you will need to reinstall the game each time that a new version of the patcher is released. Deltas are always generated from the original files.");
		f1.put(bb);
		f1.close();
		
		bb.rewind();
		bb.clear();
		bb.writeStr("version=1");
		
		path=bak_path + wxT("\\version.dat");
		Log(wxString("Openning ") + path + (" ...\n"));
		f1.open(path.c_str(),"wb");
		f1.put(bb);
		f1.close();

		while(!a->eof()) {
			curfile=wxString(a->getLine());
			strmd5=curfile.Mid(0,0x20);
			fname=curfile.Mid(0x21).Trim();
			if(fname=="") break;
			Log(wxT("Checking ") + fname + wxT(" ") + strmd5);
			
			path=wxGetApp().tpots_path + wxT("\\") + fname;
			
			f1.open(path.c_str());
			d5.rewind();
			d5.clear();
			d5.put(f1);
			d5.compute();
			f1.close();
			//Log("\n");
			//Log(wxString(d5.HexToAscii()));
			//Log("\n");
			//Log(wxString(strmd5));
			
			if(wxString(d5.HexToAscii())==strmd5) Log(" Ok\n");
			else {
				Log(wxString(" Checksum Mismatch: ") + wxString(d5.HexToAscii()));
				//then check if backup file exists, and restore it
				wxString bakedfile=bak_path + wxT("\\") + fname;
				Log(wxString("\nChecking baked file ") + bakedfile);
				
				try {
					f1.open(bakedfile.c_str());
					d5.rewind();
					d5.clear();
					d5.put(f1);
					d5.compute();
				
					if(wxString(d5.HexToAscii())==strmd5) { 
						Log(" Ok\n");
						//copy file
						mbuf quab;
						f1.rewind();
						quab.put(f1);
						f1.close();
						f1.open(path.c_str(),"wb");
						f1.put(quab);
						f1.close();
						Log(wxString("Restored ") + path + wxT("\n"));
					} else {
						f1.close();
						Log("\n");
						wxMessageBox(wxT("Mismatch ") + wxString(bakedfile) + wxString("\nExpected: ") + wxString(strmd5) + wxString("\n     Got: ") + wxString(d5.HexToAscii()), "Unexpected file signature", wxOK | wxICON_ERROR);
						Fatal(wxString("Checksum Mismatch: ") + wxString(d5.HexToAscii())); 
					}
					
				}
				catch (NotFound) {
					Log("\n");
					wxMessageBox(wxT("Mismatch ") + wxString(path) + wxString("\nExpected: ") + wxString(strmd5) + wxString("\n     Got: ") + wxString(d5.HexToAscii()), "Unexpected file signature", wxOK | wxICON_ERROR);
					Fatal(wxString("Checksum Mismatch: ") + wxString(d5.HexToAscii())); 
				}
			}
			
			IncPgr(1);
		}
		
		delete a;
		
		IncPgr(1);
		
	}
	void SomeFiles() {
		wxString path,path2,path69;
		
		fbuf f1,f2;
		
		wdysbuf w1;
		//mbuf * a;
		pkgfile * pfile;
		Log("Openning plClientSetup.cfg...\n");
		pfile=pkg->find("plClientSetup.cfg");
		if(pfile==NULL) Fatal("plClientSetup.cfg not found!");
		Log("Storing plClientSetup.cfg...\n");
		//a=new strbuf(pfile->pkg);
		w1.put(pfile->pkg);
		w1.encrypt();

		path=wxGetApp().tpots_path + wxT("\\plClientSetup.cfg");
		
		IncPgr(1);
		
		f1.open(path.c_str(),"wb");
		//f1.put(*a);
		f1.put(w1);
		f1.close();
		
		Log("Storing plClientSetup.cfg.alcdist...\n");
		path=wxGetApp().tpots_path + wxT("\\plClientSetup.cfg.alcdist");
		IncPgr(1);
		
		f1.open(path.c_str(),"wb");
		f1.put(w1);
		f1.close();
		
		Log("Openning MultiPlayer.bat...\n");
		pfile=pkg->find("MultiPlayer.bat");
		if(pfile==NULL) Fatal("MultiPlayer.bat not found!");
		Log("Storing MultiPlayer.bat...\n");
		path=wxGetApp().tpots_path + wxT("\\MultiPlayer.bat");
		IncPgr(1);
		f1.open(path.c_str(),"wb");
		f1.put(pfile->pkg);
		f1.close();

		Log("Openning SinglePlayer.bat...\n");
		pfile=pkg->find("SinglePlayer.bat");
		if(pfile==NULL) Fatal("SinglePlayer.bat not found!");
		Log("Storing SinglePlayer.bat...\n");
		path=wxGetApp().tpots_path + wxT("\\SinglePlayer.bat");
		IncPgr(1);
		f1.open(path.c_str(),"wb");
		f1.put(pfile->pkg);
		f1.close();

		//delete done.dat to ensure that new installed sounds are present
		path=wxGetApp().tpots_path + wxT("\\sfx\\streamingCache\\done.dat");
		Log(wxString("Removing ") + path + wxString(" ...\n"));
		IncPgr(1);
		remove(path);

		//delete a;
		//Add serverconfig.ini
		path69=wxGetApp().uu_path + wxT("\\serverconfig.ini");
		path2=wxGetApp().tpots_path + wxT("\\serverconfig.ini");
		
		Log("Reading " + path69 + " ...\n");
		Log("Writting " + path2 + " ...\n");
		
		f1.open(path69.c_str(),"rb");
		f2.open(path2.c_str(),"wb");
		f2.put(f1);
		f2.close();
		f1.close();

		path=wxGetApp().tpots_path + wxT("\\UruSetup.exe");
		path2=wxGetApp().tpots_path + wxT("\\UruSetupSinglePlayer.exe");
		path69=wxGetApp().uu_path + wxT("\\UruSetup.exe");
		
		Log("Replacing UruSetup.exe...\n");
		IncPgr(1);
		try {
			f1.open(path2.c_str(),"rb");
			f1.close();
			return; //Don't ask the user, just avoid replacing it.
			if(wxMessageBox(_T("Do you want to replace UruSetup.exe.bak?"), _T("Confirm"), wxICON_QUESTION | wxYES_NO) != wxYES) {
				return;
			}
		} catch(NotFound) {}
		rename(path.c_str(),path2.c_str());
		f1.open(path69.c_str(),"rb");
		f2.open(path.c_str(),"wb");
		f2.put(f1);
		f2.close();
		f1.close();

	}
	void PrpPatch() {
		//copy files from UU
		Log("Starting file copy...\n");
		IncPgr(1);
		
		strbuf a;
		
		fbuf f1,f2;
		
		wxString pathsrc,pathdest,pathbak,path,fname,path2,path3;
		
		pathsrc=wxGetApp().uu_path + wxT("\\");
		pathdest=wxGetApp().tpots_path + wxT("\\");
		pathbak=bak_path + wxT("\\");
		
		pkgfile * pfile;
		Log("Openning filecopy.dat...\n");
		pkg->rewind();
		pfile=pkg->find("filecopy.dat");
		if(pfile==NULL) Fatal("filecopy.dat not found!");
		Log("Parsing filecopy.dat...\n");
		a.put(pfile->pkg);
		a.rewind();

		while(!a.eof()) {
			fname=wxString(a.getLine()).Trim();
			if(fname=="") break;
			Log(wxT("Copying ") + fname + wxT(" ...\n"));
			IncPgr(1);
			
			path = pathsrc + fname;
			path2 = pathdest + fname;
			path3 = pathbak + fname;
			
			try {
				Log(wxT("Trying to open ") + path2 + wxT(" ...\n"));
				f1.open(path2,"rb");
				Log(wxT("Trying to open ") + path3 + wxT(" ...\n"));
				f2.open(path3,"wb");
				f2.put(f1);
				f1.close();
				f2.close();
				Log(wxT("Baked file ") + fname + wxT(" ...\n"));
			} catch(NotFound) {}
			
			f1.open(path,"rb");
			f2.open(path2,"wb");
			f2.put(f1);
			f1.close();
			f2.close();
			
		}
		//Ki patch
		Log("Patching GUI District files...\n");
		IncPgr(1);
		//Note, we have already done a backup don't need to do it again
		Log("Openning gui.dat...\n");
		pkg->rewind();
		pfile=pkg->find("gui.dat");
		if(pfile==NULL) Fatal("gui.dat not found!");
		Log("Parsing gui.dat...\n");
		a.rewind();
		a.clear();
		a.put(pfile->pkg);
		a.rewind();

		st_PrpHeader prp;
		int ret;
		Byte n_flags=0x13; //0x1F;
		
		while(!a.eof()) {
			fname=wxString(a.getLine()).Trim();
			if(fname=="") break;
			Log(wxT("Patching ") + fname + wxT(" ...\n"));
			IncPgr(1);
			
			path = pathdest + fname;
			
			//input prp
			Log("   Reading...\n");
			ret=readprp(&prp,path.c_str(),n_flags);
			if(ret<0) Fatal("Error reading input file");
			if(prp.MinorVersion!=11 || prp.MajorVersion!=63) {
				Fatal("Cannot update, I was expecting a prp version 63.11");
			}
			prp.MinorVersion=12;
			//version update
			Log("   Fixing GUIDynDisplayCtrl...\n");
			IncPgr(1);
			ret=prp_patch_GUIDynDisplayCtrl(&prp);
			if(ret<0) Fatal("Failed patching the GUIDynDisplayCtrl object!!");
			Log("   Repaging Banned references...\n");
			IncPgr(1);
			ret=prp_patch_refs(&prp);
			if(ret<0) Fatal("Failed patching Banned references!");
			Log("   Saving...\n");
			saveprp(&prp,path.c_str(),n_flags);
			
			//cleanup
			destroy_PrpHeader(&prp);

		}
		
	
	}
	void PythonPatch() {
		Log("Generating alcugs.pak...\n");
		IncPgr(1);
		
		wxString pathuu,pathtpots,pathbak,path;
		
		pathuu=wxGetApp().uu_path + wxT("\\Python\\");
		pathtpots=wxGetApp().tpots_path + wxT("\\Python\\");
		pathbak=bak_path + wxT("\\");
		
		
		pypackage uupak,tpotspak,alcpak;
		fbuf f1,f2;
		wdysbuf w;
		Log("Openning uu python.pak ...\n");
		IncPgr(1);
		path=pathuu + wxString("python.pak");
		f1.open(path);
		f1.get(w);
		f1.close();
		w.decrypt();
		w.get(uupak);
		w.rewind();
		w.clear();
		
		Log("Openning tpots python.pak ...\n");
		IncPgr(1);
		path=pathtpots + wxString("python.pak");
		f1.open(path);
		f1.get(w);
		f1.close();
		w.decrypt();
		w.get(tpotspak);
		w.rewind();
		w.clear();
		
		mbuf a;
		
		pypkgfile * pfile;
		uupak.rewind();
		pfile=uupak.find("xKI.pyc");
		if(pfile==NULL) Fatal("xKi.pyc not found!");
		a.put(pfile->pkg);
		a.rewind();
		
		//-a-
		

		alcpak.add("xKI.pyc",a);
		
		Log("Saving alcugs.pak");
		path=pathtpots + wxString("alcugs.pak");
		f2.open(path,"wb");
		w.put(alcpak);
		w.encrypt();
		f2.put(w);
		f2.close();
		
		//Open UU python.pak and extract xKi.py  xKi.py1
		//Open Tpots python.pak and extract xKi.py  xKi.py2
		
		// delta1 + xKi.py1 = delta2
		// delta2 + xKi.py2 = xKi.py
		
		//Insert xKi.py into alcugs.pak
		
		//Delete temporany files
	
	}
	void UpdateSums() {
		Log("Updating Checksums...\n");
		IncPgr(1);
		
		wxString path;
		
		path=wxGetApp().tpots_path + wxT("\\dat");
		
		directory d;
		direntry * e;
		d.open(path.c_str());
		
		wxString sumname;
		
		while((e=d.getEntry())!=NULL) {
			sumname=wxString(e->name);
			if(sumname.Right(4)==wxString(".sum")) {
				IncPgr(1);
				Log(wxString("Updating ") + wxString(sumname) + wxString("...\n"));
				UpdateSum(path + wxString("\\") + sumname);
			}
		}
		
		d.close();
	}
	void UpdateSum(wxString t) {
		fbuf f1,f2;
		wdysbuf w1;
		mbuf work;
		md5buf sum;
		f1.open(t.c_str(),"rb");
		w1.put(f1);
		w1.decrypt();
		f1.close();
		
		U32 n=w1.getU32();
		U32 magic=w1.getU32();
		
		if(magic!=0) Fatal("Uknown header reading sum file");
		
		work.putU32(n);
		work.putU32(magic);
		//U32 count=0;
		
		ustr fname;
		wxString path;
		
		for(U32 i=0; i<n; i++) {
			w1.get(fname);
			w1.seek(0x10); //MD5
			w1.seek(8); //timestamp
			Log(wxString("   Updating ") + wxString(fname.str()) + wxString("...\n"));
			IncPgr(1);

			path=wxGetApp().tpots_path + wxString("\\") + wxT(fname.str());
				
			try {
				f2.open(path,"rb");
			} catch (NotFound) {
				if(wxString(fname.str()).Left(3)!="dat") {
					wxString tmpstr=wxString("dat\\") + wxString(fname.str());
					fname.assing((Byte *)tmpstr.c_str());
					path=wxGetApp().tpots_path + wxString("\\") + wxT(fname.str());
					f2.open(path,"rb");
				} else {
					throw NotFound(path);
				}
			}
			
			sum.rewind();
			sum.clear();
			sum.put(f2);
			sum.compute();
			f2.close();
			
			fname.stream(work,0x01);
			work.put(sum);
			
			struct stat fstat;
			stat(path,&fstat);
			
			work.putU32((U32)fstat.st_mtime);
			work.putU32(0);
		}
		
		w1.rewind();
		w1.clear();
		w1.put(work);
		w1.encrypt();
		
		f1.open(t.c_str(),"wb");
		f1.put(w1);
		f1.close();
		IncPgr(1);
	}
	void Fatal(const wxString &what) {
		Log(wxString("\n\nFATAL ERROR: Pacth FAILED!, Reason:") + wxString(what) + wxString("\n"));
		well=false;
		throw "Panic!";
	}
	void Ok() {
		Log(wxT("\n\nSuccesfully Patched!!. Click NEXT to continue.\n"));
		printf("PgrVal: %i\n",pgr_val);
		UpdatePgr(pgr_top);
	}
	void IncPgr(int inc) {
		pgr_val+=inc;
		if(pgr_val>pgr_top) {
			pgr_top+=10;
			UpdatePrgTop(pgr_top);
		}
		
		UpdatePgr(pgr_val);
	}
private:
	gPatcherPage * parent;
	package * pkg;
	bool well;
	int pgr_val;
	int pgr_top;

	wxString bak_path;
	
};


class gLicensePage : public wxWizardPageSimple {
public:
	gLicensePage(wxWizard *parent): wxWizardPageSimple(parent) {

		m_lic = new gLicenseText(this,-1,wxPoint(0,0),wxSize(410,400));
		m_check = new wxCheckBox(this,-1,_T("I Agree"),wxPoint(50,410));
		m_check->SetValue(0);

	}
	void OnPageChange(wxWizardEvent &ev) {
		if(m_check->GetValue()!=true && ev.GetDirection()) {
			ev.Veto();
		}
	}
private:
	wxCheckBox * m_check;
	gLicenseText * m_lic;
	
	DECLARE_EVENT_TABLE()
};

class gSelectFilesPage : public wxWizardPageSimple {
public:
	gSelectFilesPage(wxWizard *parent): wxWizardPageSimple(parent) {
		
		int w,h,p=10;
		
		m_intro = new wxTextCtrl(this,-1,_T("\
Now, you need to select the complete\
 path to the directory/folder where \
 your Unt�l Uru game client is \
 installed. (For example: \"C:\\Program Files\\Cyan Wolrds\\Until Uru\\\"). \
This path will be used only as a \
source of files, so it will be not\
 touched by the patcher."), wxPoint(10,p), wxSize(390,70), wxTE_MULTILINE | wxTE_READONLY | wxNO_BORDER );
		m_intro->SetBackgroundColour(wxSystemSettings::GetSystemColour(wxSYS_COLOUR_BTNFACE));
		
		h=m_intro->GetSize().GetHeight();
		p+=h+10;
		
		m_luu = new wxStaticText(this,-1,_T("Full path to the Unt�l Uru Base installation directory:"), wxPoint(10,p) , wxDefaultSize);
		
		h=m_luu->GetSize().GetHeight();
		p+=h+10;
		
		m_uu = new wxTextCtrl(this,-1,_T("F:\\uru\\Uru - Prime"),wxPoint(10,p),wxSize(300,25));
		w=m_uu->GetSize().GetWidth()+20;
		m_buu = new wxButton(this,ID_BUTTON_UU,_T("..."),wxPoint(w,p));
		
		h=m_uu->GetSize().GetHeight();
		p+=h+30;
		
		//second row
		m_intro2 = new wxTextCtrl(this,-1,_T("\
Now, you need to select the complete\
 path to the directory/folder where \
 \"Path of The Shell\" or \"Complete Chronicles\" game client is \
 installed. (For example: \"C:\\Program Files\\Cyan Wolrds\\Path of The Shell\\\"). \
The Patcher will patch several game files of that installation, so you must have a backup of that client, so if something fails, then you can overwrite the failed install with your backup."), wxPoint(10,p), wxSize(390,90), wxTE_MULTILINE | wxTE_READONLY | wxNO_BORDER );
		m_intro2->SetBackgroundColour(wxSystemSettings::GetSystemColour(wxSYS_COLOUR_BTNFACE));

		h=m_intro2->GetSize().GetHeight();
		p+=h+10;
		
		m_ltpots = new wxStaticText(this,-1,_T("Full path to the Path of The Shell/Complete Chronicles base directory:"), wxPoint(10,p) , wxDefaultSize);
		
		h=m_ltpots->GetSize().GetHeight();
		p+=h+10;
		
		m_tpots = new wxTextCtrl(this,-1,_T("N:\\clients\\original\\exp"),wxPoint(10,p),wxSize(300,25));
		w=m_tpots->GetSize().GetWidth()+20;
		m_btpots = new wxButton(this,ID_BUTTON_TPOTS,_T("..."),wxPoint(w,p));
		
		h=m_tpots->GetSize().GetHeight();
		p+=h+10;

		
	}
	void OnPageChange(wxWizardEvent &ev) {
		if(ev.GetDirection()) {
			FILE * f;
			
			wxString k=wxString(GetUUPath()) + wxString("/UruExplorer.exe");
			if(!(f=fopen(k,"r"))) {
				ev.Veto();
				wxMessageBox(_T("The Selected Path \"") + wxString(GetUUPath()) + _T("\" doesn't contain the required Unt�l Uru game files."),_T("Incorrect Path!"), wxOK | wxICON_ERROR);
			} else {
				fclose(f);
			}

			k=wxString(GetTPOTSPath()) + wxString("/UruExplorer.exe");
			if(!(f=fopen(k,"r"))) {
				ev.Veto();
				wxMessageBox(_T("The Selected Path \"") + wxString(GetTPOTSPath()) + _T("\" doesn't contain the required Tpots game files."),_T("Incorrect Path!"), wxOK | wxICON_ERROR);
			} else {
				fclose(f);
			}
			
			wxGetApp().tpots_path=wxString(GetTPOTSPath());
			wxGetApp().uu_path=wxString(GetUUPath());

		}
	}
	void OnButton(wxWizardEvent &ev) {
		wxDirDialog * me;
		switch(ev.GetId()) {
			case ID_BUTTON_UU:
				me = new wxDirDialog(this, "Select Until Uru Root/Base Folder", "");
				me->ShowModal();
				if(!me->GetPath().IsEmpty() || me->GetPath()==_T("")) {
					m_uu->SetValue(me->GetPath());
				}
				break;
			case ID_BUTTON_TPOTS:
				me = new wxDirDialog(this, "Select TPOTS/CC Root/Base Destination Folder", "");
				me->ShowModal();
				if(!me->GetPath().IsEmpty() || me->GetPath()==_T("")) {
					m_tpots->SetValue(me->GetPath());
				}
				break;
		}
	}
	char * GetUUPath() {
		return m_uu->GetValue().c_str();
	}
	char * GetTPOTSPath() {
		return m_tpots->GetValue().c_str();
	}
private:
	wxTextCtrl * m_intro;
	wxTextCtrl * m_intro2;
	wxStaticText * m_luu;
	wxTextCtrl * m_uu;
	wxButton * m_buu;
	wxStaticText * m_ltpots;
	wxTextCtrl * m_tpots;
	wxButton * m_btpots;
	
	DECLARE_EVENT_TABLE()
};

class gPatcherPage : public wxWizardPageSimple {
public:
	gPatcherPage(wxWizard *parent): wxWizardPageSimple(parent) {
		myparent=parent;
		done=false;
		failed=false;
		
		int h,p=10;
		
		m_t = new wxStaticText(this,-1,_T("Patching, Please Wait..."),wxPoint(10,p));

		h=m_t->GetSize().GetHeight();
		p+=h+10;
		
		pgr = new wxGauge(this,-1,100, wxPoint(10,p) , wxSize(390,20),wxGA_HORIZONTAL | wxGA_SMOOTH);
		
		h=pgr->GetSize().GetHeight();
		p+=h+10;
		
		tlog = new wxTextCtrl(this,-1,_T(""),wxPoint(10,p),wxSize(390,300),wxTE_MULTILINE | wxTE_RICH);
		
		//DoPatch();
		//thepatch = new PatcherThread(this);
		//thepatch->Create();
	}
	void DoUpdate() {
		myparent->Update();
		//myparent->Refresh();
	}
	void OnPageChange(wxWizardEvent &ev) {
		if(ev.GetDirection()) {
			if(! done) {
				ev.Veto();
			} else done=false;
		} else {
			if(! failed) {
				ev.Veto();
			} else failed=false;
		}
	}
	void OnPage(wxWizardEvent &ev) {
		printf("now!?");
		//DoPatch();
		thepatch = new PatcherThread(this); //yes, I know, leakage........
		thepatch->Create();
		thepatch->Run();
	}
	void OnCancel(wxWizardEvent& ev) {
		if(!failed) {
			if(wxMessageBox(_T("Warning: Closing the patcher when it has not finished, may corrupt your game. Are you sure that you want to cancel and corrupt your client?"), _T("Are you sure?"), wxICON_QUESTION | wxYES_NO, this) != wxYES) {
				ev.Veto();
			}
		}
	}
	void UpdatePgrTop(int val) {
		pgr->SetRange(val);
	}
	void UpdatePgr(int val) {
		pgr->SetValue(val);
	}
	void ClearLog() {
		tlog->Clear();
	}
	void Log(const wxString &txt) {
		tlog->AppendText(txt);
	}
	void PatchOk() {
		done=true;
	}
	void PatchNotOk() {
		failed=true;
	}
private:
	bool done;
	bool failed;
	wxStaticText * m_t;
	wxGauge * pgr;
	wxTextCtrl * tlog;
	
	wxWizard * myparent;
	
	PatcherThread * thepatch;
	
	DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(gLicensePage,wxWizardPageSimple)
	EVT_WIZARD_PAGE_CHANGING(-1, gLicensePage::OnPageChange)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(gSelectFilesPage,wxWizardPageSimple)
	EVT_WIZARD_PAGE_CHANGING(-1, gSelectFilesPage::OnPageChange)
	EVT_BUTTON (ID_BUTTON_UU, gSelectFilesPage::OnButton)
	EVT_BUTTON (ID_BUTTON_TPOTS, gSelectFilesPage::OnButton)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(gPatcherPage,wxWizardPageSimple)
	EVT_WIZARD_PAGE_CHANGING(-1, gPatcherPage::OnPageChange)
	EVT_WIZARD_CANCEL(-1,gPatcherPage::OnCancel)
	EVT_WIZARD_PAGE_CHANGED(-1,gPatcherPage::OnPage)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(gWiz,wxWizard)
	EVT_WIZARD_CANCEL(-1,gWiz::OnCancel)
END_EVENT_TABLE()

/*
BEGIN_EVENT_TABLE(gMainWindow,wxFrame)
EVT_BUTTON (ID_BUTTON_LOGIN, gMainWindow::onLogin)
EVT_BUTTON (ID_BUTTON_LOGOUT, gMainWindow::onLogin)
EVT_BUTTON (ID_BUTTON_OK, gMainWindow::onLogin)
EVT_BUTTON (ID_BUTTON_CANCEL, gMainWindow::onLogin)
EVT_BUTTON (ID_BUTTON_PING, gMainWindow::onPing)
EVT_LIST_ITEM_SELECTED(ID_LIST_SHARD, gMainWindow::onSelectShard)
EVT_TEXT(ID_TEXT_ADDRESS, gMainWindow::onEditAddress)
EVT_TEXT(ID_TEXT_PORT, gMainWindow::onEditAddress)
END_EVENT_TABLE()
*/

void PatcherThread::Log(const wxString& text) {
	parent->Log(text);
}
void PatcherThread::ClearLog() {
	parent->ClearLog();
}
void PatcherThread::UpdatePrgTop(int t) {
	parent->UpdatePgrTop(t);
}
void PatcherThread::UpdatePgr(int t) {
	parent->UpdatePgr(t);
}
void PatcherThread::Update() {
	parent->DoUpdate();
	/*parent->Update();
	parent->UpdateWindowUI();
	parent->Refresh();
	parent->Layout();*/
}
void PatcherThread::PatchOk() {
	parent->PatchOk();
}
void PatcherThread::PatchNotOk() {
	Log("\n\nSomething Terrible happened! :( - Please copy the output, and report it to the forums\n");
	parent->PatchNotOk();
}

/** APP 
*/

/** Main method, on startup */
bool gMainApp::OnInit() {
	char name[200];
	sprintf(name,"%s %s - Build:%s",SNAME,VERSION,BUILD);
	
	//wxFileSystem::AddHandler(new wxInternetFSHandler);
	
	SetVendorName(_T("Alcugs"));
	SetAppName(_T(name));
	
	//printf("argc:%i,argv:",gMainApp::argc);
	int i;
	for(i=0; i<gMainApp::argc; i++) {
		//printf("%s,",gMainApp::argv[i]);
	}
	//printf("\n");
	//fflush(stdout);
	
	//check if everything is ok
	bool ok=false;
	
	fbuf f1;
	mbuf * buf1 = new mbuf();
	f1.open(gMainApp::argv[0],"rb");
	buf1->put(f1);
	f1.close();
	
	md5buf * md5buf1;
	md5buf1 = new md5buf(*buf1,0,buf1->size()-0x10);
	
	mbuf * buf2 = new mbuf(*buf1,buf1->size()-0x10,0x10);
	if(buf2->compare(*md5buf1)) ok=true;
	
	delete buf2;
	delete md5buf1;
	
	if(ok) printf("ok");
	else {
		wxMessageBox(_T("Checksum verification failed! \nYour download may be corrupted, or the file on the server is corrupted. \n\nPlease re-download a new version."),_T("CRC Error!"), wxOK | wxICON_ERROR);
		exit(-1);
	}
	
	//Open the resources
	printf("Opening resources...\n");
	buf1->seek(0x14,SEEK_END);
	U32 lsize=buf1->getU32();
	printf("lsize: %u\n",lsize);
	buf1->seek(lsize+0x14,SEEK_END);
	
	zbuf * zbuf1=new zbuf(*buf1,(U32)buf1->tell(),(U32)lsize);
	printf("uncompressing...\n");
	zbuf1->uncompress();
	
	printf("get package...\n");
	zbuf1->get(this->pkg);
	delete buf1;
	delete zbuf1;
	printf("everything gone well.. continuing...\n");

	//wxInitAllImageHandlers();
	
	//Parse configuration files
	//cfg=NULL;
	//read_config(stdout,"gsetup.conf",&cfg);
	wxSize mSize = wxSize(410,430);
	wxSize imSize = wxSize(400,420);
	
	w_root = new gWiz(_T(name),440,530);
	//w_root->Show(TRUE);
	wxWizardPageSimple * page1 = new wxWizardPageSimple(w_root);
	gHtml * intro = new gHtml(page1,-1,wxPoint(0,0),imSize);
	
	wxString sintro=wxString("<html><head></head><body>\
	<h3>AlcugsKI v 1.0</h3>\
	<h4>Check List</h4>\
	<ul>\
	<li><b>Unt&igrave;l Uru</b>: You need an unmoded (not manipulated or hacked) version of the Unt&igrave;l Uru client. UserKi/AdminKi mods are ok, only if them are in a separate *.pak file. If you have installed one of the GoA versions of the userki, the patch should work without problems. (Your UU install will be not touched by the patcher)</li>\
	<li><b>Path of The Shell/Complete Chronicles</b>: You need an unmoded (not manipulated or hacked) version of \"Myst Uru: The Path of The Shell\", or \"Myst Uru: Complete Chronicles\". Versions that has been already patched to work with an Alcugs server should work. Fly mode patch, and any other modification will cause this patcher to fail.</li>\
	<li><b>A Backup</b>: You need a backup of your \"Path of The Shell\"/\"Complete Chronicles\" client. So if something bad happens, you can restore it.</li>\
	<li><b>Last version</b>: Check <a href=\"http://alcugs.sourceforge.net\" target=_blank>alcugs.sourceforge.net</a> for the latest version of this patcher.</li>\
	</ul>\
	</body></html>");
	
	intro->SetPage(sintro);
	w_root->SetPageSize(mSize);
	
	wxWizardPageSimple * page2 = new wxWizardPageSimple(w_root);
	gHtml * intro2 = new gHtml(page2,-1,wxPoint(0,0),imSize);
	
	wxString sintro2=wxString("<html><head></head><body>\
	<h3>AlcugsKI v 1.0</h3>\
	<h4>Understand What are you doing</h4>\
	This patcher is going to patch your Tpots installation, with the purpose of adding all required files for a working KI.<br><br>\
	This is a beta version, so some things may not work as expected.<br>\
	On the current version, some features like the journal don't work, mainly because this patch only replaces the KI with the one from UU. In order to have a working journal we will need to merge both KIs, and more research on this area is still required.<br>\
	There are several bugs in this version, mainly due to lack of time to fix them, so I'm open for patches to the patcher.<br><br>\
	The source code of this patcher, is available on the Alcugs server subversion repository, as some information of how to manually extract the delta files from this package.\
	</body></html>");
	
	intro2->SetPage(sintro2);
	page1->SetNext(page2);
	page2->SetPrev(page1);
	
	//License page
	
	gLicensePage * page3 = new gLicensePage(w_root);
	page2->SetNext(page3);
	page3->SetPrev(page2);
	
	gSelectFilesPage * page4 = new gSelectFilesPage(w_root);
	page3->SetNext(page4);
	page4->SetPrev(page3);
	
	gPatcherPage * page5 = new gPatcherPage(w_root);
	page4->SetNext(page5);
	page5->SetPrev(page4);


	//FINAL page

	wxWizardPageSimple * page6 = new wxWizardPageSimple(w_root);
	gHtml * intro3 = new gHtml(page6,-1,wxPoint(0,0),imSize);
	
	wxString sintro3=wxString("<html><head></head><body>\
	<h3>AlcugsKI v 1.0</h3>\
	<h4>Installation succesfull</h4>\
	Now, you should be able to play with your new patched client on any Alcugs powered shard<br>\n\
	<br>Don't forget to report bugs, or any other issues on the <a href=\"http://alcugs.almlys.dyns.net/forum\" target=_blank>\
	Alcugs project forum</a>\
	<br><br>\
	You can Play ONLINE, by double clicking over \"UruSetup.exe\"<br>\n\
	This version does not support Offline play, you can still play the game offline, but that\
	will enable the dataserver again, and you will need to disable it again.\
	<br>\
	<b>Remember that this patch is NOT for PLASMA servers. Only Alcugs.</b>\
	</body></html>");
	
	intro3->SetPage(sintro3);

	page5->SetNext(page6);
	//page6->SetPrev(page5);

	//**
	
	//SetTopWindow(w_root);
	int ret=w_root->RunWizard(page1);
	printf("%i\n",ret);
	
	if(ret) {
		wxShell(_T("start \"\" \"") + wxString(page4->GetTPOTSPath()) + _T("\""));
	}
	
	exit(ret);
	return false; //true=correct init, false=stops app
}


/** Root Window
*/
gWiz::gWiz(const wxChar *title, int width, int height) 
	: wxWizard((wxWindow *) NULL,-1,wxString(title)) 
{
	

}

gWiz::~gWiz() {}


void gWiz::OnCancel(wxWizardEvent& ev) {
	if(wxMessageBox(_T("Are you sure that you want to cancel?"), _T("Are you sure?"), wxICON_QUESTION | wxYES_NO, this) != wxYES) {
		ev.Veto();
	}
}


