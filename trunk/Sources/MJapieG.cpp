/*
	Copyright (c) 2007, Maarten L. Hekkelman
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the Maarten L. Hekkelman nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
	STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
	ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
	OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "MJapieG.h"

#include "MDocument.h"
#include "MEditWindow.h"
#include "MPreferences.h"
#include "MGlobals.h"
#include "MUtils.h"
#include "MAcceleratorTable.h"

#include <iostream>

using namespace std;

MJapieApp* gApp;

MJapieApp::MJapieApp(
	int				argc,
	char*			argv[])
	: MHandler(nil)
	, mRecentMgr(gtk_recent_manager_get_default())
	, mQuit(false)
	, mQuitPending(false)
{
}

MJapieApp::~MJapieApp()
{
}

bool MJapieApp::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = true;

//	MProject* project = MProject::Instance();
//	if (project != nil)
//	{
//		result = project->ProcessCommand(inEvent, inCommand);
//	
//		if (result != eventNotHandledErr)
//			return result;
//	}

	switch (inCommand)
	{
		case cmd_Preferences:
//			MPrefsDialog::Create();
			break;
		
		case cmd_Quit:
			if (not MSaverMixin::IsNavDialogVisible())
				DoQuit();
			break;
		
		case cmd_New:
			DoNew();
			break;
		
		case cmd_Open:
			DoOpen();
			break;
		
		case cmd_CloseAll:
			DoCloseAll(kSaveChangesClosingDocument);
			break;
		
		case cmd_SaveAll:
			DoSaveAll();
			break;
		
//		case cmd_ClearRecent:
//			DoClearRecent();
//			break;
//		
//		case cmd_OpenRecent:
//			DoOpenRecent(inCommand);
//			break;
		
		case cmd_OpenTemplate:
			DoOpenTemplate(inCommand);
			break;
		
//		case cmd_Find:
//			MFindDialog::Instance().Select();
//			break;
		
//		case cmd_FindInNextFile:
//			MFindDialog::Instance().FindNext();
//			break;
		
//		case cmd_OpenIncludeFile:
//		{
//			std::auto_ptr<MFindAndOpenDialog> dlog(new MFindAndOpenDialog);
//			dlog->Initialize(nil, nil);
//			dlog.release();
//			break;
//		}
		
		case cmd_Worksheet:
			ShowWorksheet();
			break;

//		case cmd_ShowDiffWindow:
//		{
//			auto_ptr<MDiffWindow> w(new MDiffWindow);
//			w->Initialize();
//			w->Show();
//			w.release();
//			break;
//		}

//		case 'DgTs':
//		{
//			auto_ptr<MDebuggerWindow> w(new MDebuggerWindow);
//			w->Initialize();
//			w->Show();
//			w.release();
//			break;
//		}

		case cmd_SelectWindowFromMenu:
			DoSelectWindowFromWindowMenu(inItemIndex - 2);
			break;

		default:
			result = false;
			break;
	}
	
	return result;
}

bool MJapieApp::UpdateCommandStatus(
	uint32			inCommand,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;

//	MProject* project = MProject::Instance();
//	if (project != nil)
//	{
//		result = project->UpdateCommandStatus(inEvent, inCommand);
//	
//		if (result != eventNotHandledErr)
//			return result;
//	}

	switch (inCommand)
	{
		case cmd_New:
		case cmd_Open:
		case cmd_OpenRecent:
		case cmd_OpenTemplate:
		case cmd_ClearRecent:
		case cmd_CloseAll:
		case cmd_SaveAll:
		case cmd_Quit:
		case cmd_Preferences:
		case cmd_Find:
		case cmd_FindInNextFile:
		case cmd_OpenIncludeFile:
		case cmd_Worksheet:
			outEnabled = true;
			break;
		
		default:
			result = false;
			break;
	}
	
	return result;
}

void MJapieApp::UpdateWindowMenu(
	MMenu*				inMenu)
{
	inMenu->RemoveItems(2, inMenu->CountItems() - 2);
	
	MDocument* doc = MDocument::GetFirstDocument();
	while (doc != nil)
	{
		string label;
		
		if (doc->IsSpecified())
			label = doc->GetURL().leaf();
		else
		{
			MDocWindow* w = MDocWindow::FindWindowForDocument(doc);
			if (w != nil)
				label = w->GetTitle();
			else
				label = "weird";
		}
		
		inMenu->AppendItem(label, cmd_SelectWindowFromMenu);
		doc = doc->GetNextDocument();
	}	
}

void MJapieApp::DoSelectWindowFromWindowMenu(
	uint32				inIndex)
{
	MDocument* doc = MDocument::GetFirstDocument();

	while (doc != nil and inIndex-- > 0)
		doc = doc->GetNextDocument();
	
	if (doc != nil)
		MDocWindow::DisplayDocument(doc);	
}	

void MJapieApp::RecycleWindow(
	MWindow*		inWindow)
{
	mTrashCan.push_back(inWindow);
}

void MJapieApp::RunEventLoop()
{
	uint32 snooper = gtk_key_snooper_install(
		&MJapieApp::Snooper, nil);
	
	/*uint32 timer = */g_timeout_add(250, &MJapieApp::Timeout, nil);
	
	gtk_main();
	
	gtk_key_snooper_remove(snooper);
}

gint MJapieApp::Snooper(
	GtkWidget*		inGrabWidget,
	GdkEventKey*	inEvent,
	gpointer		inFuncData)
{
	bool result = false;
	uint32 cmd;

	if (inEvent->type == GDK_KEY_PRESS and 
		MAcceleratorTable::Instance().IsAcceleratorKey(inEvent, cmd))
	{
		result = true;
		
		MHandler* handler = MHandler::GetFocus();
		if (handler == nil)
			handler = gApp;
		
		bool enabled, checked;
		if (handler->UpdateCommandStatus(cmd, enabled, checked) and enabled)
			handler->ProcessCommand(cmd, nil, 0);
	}

	return false;
}

void MJapieApp::DoSaveAll()
{
	MDocument* doc = MDocument::GetFirstDocument();
	
	while (doc != nil)
	{
		if (doc->IsSpecified() and doc->IsModified())
			doc->DoSave();
		doc = doc->GetNextDocument();
	}
	
	doc = MDocument::GetFirstDocument();

	while (doc != nil)
	{
		if (not doc->IsSpecified() and doc->IsModified())
		{
			MController* controller = doc->GetFirstController();

			assert(controller != nil);
			
			controller->SaveDocumentAs();
		}
		
		doc = doc->GetNextDocument();
	}
}

void MJapieApp::DoCloseAll(
	MCloseReason		inAction)
{
	MDocument* doc = MDocument::GetFirstDocument();
	
	// first close all that can be closed

	while (doc != nil)
	{
		MDocument* next = doc->GetNextDocument();
		
		if (not doc->IsModified() and not doc->IsWorksheet())
		{
			MController* controller = doc->GetFirstController();
			
			assert(controller != nil);
			
			(void)controller->TryCloseDocument(inAction);
		}
		
		doc = next;
	}
	
	// then close what remains

	doc = MDocument::GetFirstDocument();

	while (doc != nil)
	{
		MDocument* next = doc->GetNextDocument();

		if (not doc->IsWorksheet() or inAction == kSaveChangesQuittingApplication)
		{
			MController* controller = doc->GetFirstController();
			
			assert(controller != nil);
			
			if (not controller->TryCloseDocument(inAction))
				break;
		}
		
		doc = next;
	}
}

void MJapieApp::DoQuit()
{
	DoCloseAll(kSaveChangesQuittingApplication);
//	MDialog::CloseAllDialogs();
	
//	if (MProject::Instance() != nil)
//	{
//		string p = MProject::Instance()->GetPath().string();
//		Preferences::SetString("last project", p);
//	}
//	
//	MProject::CloseAllProjects(kNavSaveChangesQuittingApplication);
	
	if (MDocument::GetFirstDocument() == nil /* and MProject::Instance() == nil*/)
		gtk_main_quit();
}

void MJapieApp::DoNew()
{
	MDocument*	doc = new MDocument(nil);
	MDocWindow::DisplayDocument(doc);
}

void MJapieApp::DoOpen()
{
	GtkWidget* dialog = 
		gtk_file_chooser_dialog_new("Open", nil,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);

	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), true);
	
	MDocument* doc = nil;
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GSList* files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));	
		
		GSList* file = files;	
		
		while (file != nil)
		{
			MPath url(reinterpret_cast<char*>(file->data));
			doc = OpenOneDocument(url);
			
			g_free(file->data);
			file->data = nil;
			file = file->next;
		}
		
		g_slist_free(files);
	}
	
	gtk_widget_destroy(dialog);
	
	if (doc != nil)
		MDocWindow::DisplayDocument(doc); 
}

// ---------------------------------------------------------------------------
//	OpenOneDocument

MDocument* MJapieApp::OpenOneDocument(
	const MPath&			inFileRef)
{
	AddToRecentMenu(inFileRef);
	
	MDocument* doc = MDocument::GetDocumentForURL(inFileRef, false);

	if (doc == nil)
		doc = new MDocument(&inFileRef);

	MDocWindow::DisplayDocument(doc);
	
	return doc;
}

// ---------------------------------------------------------------------------
//	OpenProject

void MJapieApp::OpenProject(
	const MPath&		inPath)
{
	AddToRecentMenu(inPath);
	
//	auto_ptr<MProject> project(new MProject(inPath));
//	project->Initialize();
//	project->Show();
//	project.release();
}

void MJapieApp::AddToRecentMenu(const MPath& inFileRef)
{
	string path = "file://";
	path += fs::system_complete(inFileRef).string();

	if (gtk_recent_manager_has_item(mRecentMgr, path.c_str()))
		gtk_recent_manager_remove_item(mRecentMgr, path.c_str(), nil);
	
	gtk_recent_manager_add_item(mRecentMgr, path.c_str());
}

void MJapieApp::DoOpenTemplate(
	uint32			inCommand)
{//
//	CFStringRef s;
//	if (::CopyMenuItemTextAsCFString(inCommand.menu.menuRef,
//		inCommand.menu.menuItemIndex, &s) == noErr)
//	{
//		MCFString sr(s, false);
//		
//		string n;
//		sr.GetString(n);
//		
//		MFile file(gTemplatesDir / n);
//		file.Open(O_RDONLY);
//		
//		uint64 len = file.GetSize();
//		auto_array<char> buf(new char[len]);
//		
//		file.Read(buf.get(), len);
//		
//		string text(buf.get(), len);
//		string::size_type offset = 0;
//		
//		while ((offset = text.find("$date$", offset)) != string::npos)
//			text.replace(offset, 6, GetDateTime());
//
//		while ((offset = text.find("$name$", offset)) != string::npos)
//			text.replace(offset, 6, GetUserName(false));
//		
//		while ((offset = text.find("$shortname$", offset)) != string::npos)
//			text.replace(offset, 11, GetUserName(true));
//		
//		ustring utext;
//		Convert(text, utext);
//		
//		MDocument* doc = new MDocument(utext, n);
//		MDocWindow::DisplayDocument(doc);
//	}
}

void MJapieApp::ShowWorksheet()
{
	MPath worksheet = gPrefsDir / "Worksheet";
	
	if (not fs::exists(worksheet))
	{
		MFile file(worksheet);
		file.Open(O_CREAT | O_RDWR);
	}
		
	MDocument* doc = OpenOneDocument(worksheet);
	if (doc != nil)
		doc->SetWorksheet(true);
}

gboolean MJapieApp::Timeout(
	gpointer		inData)
{
	gApp->Pulse();
	return true;
}

void MJapieApp::Pulse()
{
	for (MWindowList::iterator w = mTrashCan.begin(); w != mTrashCan.end(); ++w)
		delete *w;
	
	mTrashCan.clear();
	
	if (MDocWindow::GetFirstDocWindow() == nil)
		DoQuit();
	else
		eIdle(GetLocalTime());
}

int main(int argc, char* argv[])
{
	try
	{
		fs::path::default_name_check(fs::no_check);
		
		gtk_init(&argc, &argv);

		vector<MPath> docs;
		
		int c;
		while ((c = getopt(argc, const_cast<char**>(argv), "h?p:")) != -1)
		{
			switch (c)
			{
				case 'h':
				case '?':
					cout << "usage: " << argv[0] << " [files to open]" << endl;
					exit(0);
					break;
				
				case 'p':
					break;
				
				default:
					cerr << "unknown option: " << char(c) << endl;
					exit(1);
					break;
			}
		}

		char b[PATH_MAX];
		getcwd(b, PATH_MAX);
		MPath cwd(b);

		for (int32 i = optind; i < argc; ++i)
			docs.push_back(cwd / argv[i]);

		InitGlobals();

		gApp = new MJapieApp(argc, argv);

		if (docs.size() > 0)
		{
			for (vector<MPath>::iterator d = docs.begin(); d != docs.end(); ++d)
			{
				try
				{
					gApp->OpenOneDocument(*d);
				}
				catch (std::exception& inErr)
				{
					MError::DisplayError(inErr);
				}
			}
		}
		else
			gApp->ProcessCommand(cmd_New, nil, 0);

		gApp->RunEventLoop();

		// we're done, clean up
		SaveGlobals();
		
		delete gApp;
	}
	catch (exception& e)
	{
		cerr << e.what() << endl;
	}
	catch (...)
	{
		cerr << "Exception caught" << endl;
	}
	
	return 0;
}

