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

// $Id$

#include "MJapi.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <cerrno>
#include <signal.h>
#include <libintl.h>

#include <gdk/gdkx.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/replace.hpp>

#include "MJapiApp.h"
#include "MTextDocument.h"
#include "MEditWindow.h"
#include "MPreferences.h"
#include "MGlobals.h"
#include "MUtils.h"
#include "MAcceleratorTable.h"
#include "MDocClosedNotifier.h"
#include "MFindAndOpenDialog.h"
#include "MFindDialog.h"
#include "MProject.h"
#include "MPrefsDialog.h"
#include "MStrings.h"
#include "MAlerts.h"
#include "MDiffWindow.h"
#include "MResources.h"
#include "MProjectWindow.h"

#include <iostream>

using namespace std;

static bool gQuit = false;

int VERBOSE;

const char
	kAppName[] = "Japi",
	kVersionString[] = "0.9.6";

MJapieApp* gApp;

const char
	kSocketName[] = "/tmp/japi.%d.socket";

// --------------------------------------------------------------------

MJapieApp::MJapieApp(
	bool	inForked)
	: MHandler(nil)
	, mRecentMgr(gtk_recent_manager_get_default())
	, mSocketFD(-1)
	, mReceivedFirstMsg(not inForked)
	, mQuit(false)
	, mQuitPending(false)
{
	if (inForked)
	{
		mSocketFD = socket(AF_LOCAL, SOCK_STREAM, 0);
	
		struct sockaddr_un addr = {};
		addr.sun_family = AF_LOCAL;
		snprintf(addr.sun_path, sizeof(addr.sun_path), kSocketName, getuid());
		
		unlink(addr.sun_path);	// allowed to fail
	
		int err = bind(mSocketFD, (const sockaddr*)&addr, SUN_LEN(&addr));
	
		if (err < 0)
			cerr << _("bind failed: ") << strerror(errno) << endl;
		else
		{
			err = listen(mSocketFD, 5);
			if (err < 0)
				cerr << _("Failed to listen to socket: ") << strerror(errno) << endl;
			else
			{
				int flags = fcntl(mSocketFD, F_GETFL, 0);
				if (fcntl(mSocketFD, F_SETFL, flags | O_NONBLOCK))
					cerr << _("Failed to set mSocketFD non blocking: ") << strerror(errno) << endl;
			}
		}
	
		if (err != 0)
		{
			close(mSocketFD);
			mSocketFD = -1;
		}
	}
}

MJapieApp::~MJapieApp()
{
	if (mSocketFD >= 0)
	{
		close(mSocketFD);
	
		char path[1024] = {};
		snprintf(path, sizeof(path), kSocketName, getuid());
		unlink(path);
	}
}

bool MJapieApp::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;

//	MProject* project = MProject::Instance();
//	if (project != nil and project->ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers))
//		return true;

	switch (inCommand)
	{
		case cmd_About:
		{
			MWindow* w = MWindow::GetFirstWindow();
			GtkWidget* ww = nil;
			if (w != nil)
				ww = w->GetGtkWidget();
			
			gtk_show_about_dialog(GTK_WINDOW(ww),
				"program_name", kAppName,
				"version", kVersionString,
				"copyright", "Copyright © 2007 Maarten L. Hekkelman",
				"comments", _("A simple development environment"),
				"website", "http://www.hekkelman.com/",
				nil);
			break;
		}
		
		case cmd_Preferences:
			MPrefsDialog::Create();
			break;
		
		case cmd_Quit:
			if (not MSaverMixin::IsNavDialogVisible())
				DoQuit();
			break;
		
		case cmd_New:
			DoNew();
			break;
		
		case cmd_NewProject:
			DoNewProject();
			break;
		
		case cmd_Open:
			DoOpen();
			break;
		
		case cmd_CloseAll:
			DoCloseAll(kSaveChangesClosingAllDocuments);
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
			if (inModifiers & GDK_CONTROL_MASK)
				OpenOneDocument(MUrl(gTemplatesDir / inMenu->GetItemLabel(inItemIndex)));
			else
				DoOpenTemplate(inMenu->GetItemLabel(inItemIndex));
			break;
		
		case cmd_ApplyScript:
		{
			MUrl url(gScriptsDir / inMenu->GetItemLabel(inItemIndex));
			OpenOneDocument(url);
			break;
		}
		
		case cmd_Find:
			MFindDialog::Instance().Select();
			break;
	
		case cmd_FindInNextFile:
			MFindDialog::Instance().FindNext();
			break;
		
		case cmd_OpenIncludeFile:
			new MFindAndOpenDialog(MProject::Instance(), MWindow::GetFirstWindow());
			break;
		
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
		
		case 'test':
		{
			MProjectWindow* w = new MProjectWindow();
			w->Initialize(new MTextDocument(nil));
			w->Select();
			break;
		}
		
		case cmd_ShowDiffWindow:
			new MDiffWindow;
			break;
		
		default:
			result = false;
			break;
	}
	
	return result;
}

bool MJapieApp::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;

//	MProject* project = MProject::Instance();
//	if (project != nil and project->UpdateCommandStatus(inCommand, outEnabled, outChecked))
//		return true;

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
		case cmd_About:
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
		if (doc != MTextDocument::GetWorksheet())
		{
			string label;
			
			if (doc->IsSpecified())
			{
				const MUrl& url = doc->GetURL();
				
				if (not url.IsLocal())
					label += url.GetScheme() + ':';
				
				label += url.GetFileName();
			}
			else
			{
				MDocWindow* w = MDocWindow::FindWindowForDocument(doc);
				if (w != nil)
					label = w->GetTitle();
				else
					label = _("weird");
			}
			
			inMenu->AppendItem(label, cmd_SelectWindowFromMenu);
		}
		
		doc = doc->GetNextDocument();
	}	
}

void MJapieApp::UpdateTemplateMenu(
	MMenu*				inMenu)
{
	inMenu->RemoveItems(2, inMenu->CountItems() - 2);

	fs::path templatesDir = gTemplatesDir;
	if (fs::exists(templatesDir) and fs::is_directory(templatesDir))
	{
		MFileIterator iter(templatesDir, 0);
		
		fs::path file;
		while (iter.Next(file))
			inMenu->AppendItem(file.leaf(), cmd_OpenTemplate);
	}
}

void MJapieApp::UpdateScriptsMenu(
	MMenu*				inMenu)
{
	inMenu->RemoveItems(0, inMenu->CountItems());

	if (fs::exists(gScriptsDir) and fs::is_directory(gScriptsDir))
	{
		MFileIterator iter(gScriptsDir, 0);
		
		fs::path file;
		while (iter.Next(file))
			inMenu->AppendItem(file.leaf(), cmd_ApplyScript);
	}
}

void MJapieApp::DoSelectWindowFromWindowMenu(
	uint32				inIndex)
{
	MDocument* doc = MDocument::GetFirstDocument();

	while (doc != nil and inIndex-- > 0)
		doc = doc->GetNextDocument();
	
	if (doc != nil)
		DisplayDocument(doc);	
}	

void MJapieApp::RecycleWindow(
	MWindow*		inWindow)
{
	mTrashCan.push_back(inWindow);
}

void MJapieApp::RunEventLoop()
{
	if (Preferences::GetInteger("open worksheet", 0))
		ShowWorksheet();
	
	if (Preferences::GetInteger("reopen project", 1))
	{
		fs::path pp = Preferences::GetString("last project", "");
		if (fs::exists(pp))
			OpenProject(pp);
	}
	
//	gdk_display_add_client_message_filter(
//		gdk_display_get_default(),
//		gdk_atom_intern("WM_PROTOCOLS", false),
//		&MJapieApp::ClientMessageFilter, this);
//	gdk_window_add_filter(nil, &MJapieApp::ClientMessageFilter, this);

	uint32 snooper = gtk_key_snooper_install(
		&MJapieApp::Snooper, nil);
	
	/*uint32 timer = */
		//g_timeout_add(250, &MJapieApp::Timeout, nil);
	g_timeout_add(50, &MJapieApp::Timeout, nil);
	
//	gdk_event_handler_set(&MJapieApp::EventHandler, nil, nil);
	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();
	
	gtk_key_snooper_remove(snooper);
}

GdkFilterReturn MJapieApp::ClientMessageFilter(
	GdkXEvent*			inXEvent,
	GdkEvent*			inEvent,
	gpointer			data)
{
	XEvent* xevent = (XEvent*)inXEvent;
	GdkFilterReturn result = GDK_FILTER_CONTINUE;

	if (xevent->type == ClientMessage)
	{
		PRINT(("Received ClientMessage: "));
		
		if ((Atom)xevent->xclient.data.l[0] == gdk_x11_get_xatom_by_name("WM_TAKE_FOCUS"))
		{
			PRINT(("Take Focus event"));
		}
	}
	
	return result;
}

gint MJapieApp::Snooper(
	GtkWidget*		inGrabWidget,
	GdkEventKey*	inEvent,
	gpointer		inFuncData)
{
	bool result = false;

	try
	{
		uint32 cmd;
	
		if (inEvent->type == GDK_KEY_PRESS and 
			MAcceleratorTable::Instance().IsAcceleratorKey(inEvent, cmd))
		{
			MHandler* handler = MHandler::GetFocus();
			if (handler == nil)
				handler = gApp;
			
			bool enabled = true, checked = false;
			if (handler->UpdateCommandStatus(cmd, nil, 0, enabled, checked) and enabled)
				result = handler->ProcessCommand(cmd, nil, 0, 0);
		}
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	catch (...) {}

	return result;
}

void MJapieApp::EventHandler(
	GdkEvent*			inEvent,
	gpointer			inData)
{
//	if (inEvent->type != GDK_KEY_PRESS or
//		Snooper(nil, (GdkEventKey*)inEvent, nil) == false)
//	{
		gtk_main_do_event(inEvent);
//	}
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
	// first close all that can be closed

	MDocument* doc = MDocument::GetFirstDocument();
	
	while (doc != nil)
	{
		MDocument* next = doc->GetNextDocument();
		
		if (dynamic_cast<MTextDocument*>(doc) != nil and
			doc != MTextDocument::GetWorksheet() and
			not doc->IsModified())
		{
			MController* controller = doc->GetFirstController();
			
			assert(controller != nil);
			
			if (controller != nil)
				(void)controller->TryCloseDocument(inAction);
			else
				cerr << _("Weird, document without controller: ") << doc->GetURL() << endl;
		}
		
		doc = next;
	}
	
	// then close what remains

	doc = MDocument::GetFirstDocument();

	while (doc != nil)
	{
		MDocument* next = doc->GetNextDocument();

		MController* controller = doc->GetFirstController();
		assert(controller != nil);

		if (doc == MTextDocument::GetWorksheet())
		{
			if (inAction == kSaveChangesQuittingApplication)
			{
				doc->DoSave();
				
				if (not controller->TryCloseDocument(inAction))
					break;
			}
		}
		else if (dynamic_cast<MTextDocument*>(doc) != nil or
			inAction == kSaveChangesQuittingApplication)
		{
			if (not controller->TryCloseDocument(inAction))
				break;
		}
		
		doc = next;
	}
}

void MJapieApp::DoQuit()
{
	if (MProject::Instance() != nil)
	{
		string p = MProject::Instance()->GetPath().string();
		Preferences::SetString("last project", p);
	}

	DoCloseAll(kSaveChangesQuittingApplication);
	
	if (MDocument::GetFirstDocument() == nil and MProject::Instance() == nil)
	{
		MFindDialog::Instance().Close();
		gtk_main_quit();
	}
}

MDocWindow* MJapieApp::DisplayDocument(
	MDocument*		inDocument)
{
	MDocWindow* result = MDocWindow::FindWindowForDocument(inDocument);
	
	if (result == nil)
	{
		if (dynamic_cast<MTextDocument*>(inDocument) != nil)
		{
			MEditWindow* e = new MEditWindow;
			e->Initialize(inDocument);
			e->Show();

			result = e;
		}
	}
	
	result->Select();
	
	return result;
}

void MJapieApp::DoNew()
{
	MDocument*	doc = new MTextDocument(nil);
	DisplayDocument(doc);
}

void MJapieApp::DoNewProject()
{
	GtkWidget *dialog;
	
	dialog = gtk_file_chooser_dialog_new(_("Save File"),
					      nil,
					      GTK_FILE_CHOOSER_ACTION_SAVE,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					      NULL);

	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	
	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), _("NewProject"));
	gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), true);

	if (mCurrentFolder.length() > 0)
	{
		gtk_file_chooser_set_current_folder_uri(
			GTK_FILE_CHOOSER(dialog), mCurrentFolder.c_str());
	}
	
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
		
		THROW_IF_NIL((uri));
		
		MUrl url(uri);
		g_free(uri);

		mCurrentFolder = 
			gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
		
		// get the path
		fs::path p = url.GetPath();

		// create a directory to store our project and the source files in it.		
		fs::create_directories(p);
		fs::create_directories(p / "src");
		
		// create the project path
		fs::path projectFile = p / (p.leaf() + ".prj");
		
		// and file
		fs::ofstream file(projectFile);
		if (not file.is_open())
			THROW(("Failed to create project file"));
		
		// write out a default project from our resources
		const char* txt;
		uint32 length;

		if (not LoadResource(string("Templates/Projects/hello-cmdline.prj"), txt, length))
			THROW(("Failed to load project resource"));
		
		file.write(txt, length);
		file.close();
		
		// and write out a sample source file
		fs::ofstream srcfile(p / "src" / "HelloWorld.cpp");
		if (not srcfile.is_open())
			THROW(("Failed to create project file"));
		
		if (not LoadResource(string("Templates/Projects/hello.cpp"), txt, length))
			THROW(("Failed to load project sourcefile resource"));
		
		srcfile.write(txt, length);
		srcfile.close();
		
		OpenProject(projectFile);
	}
	
	gtk_widget_destroy(dialog);
}

void MJapieApp::SetCurrentFolder(
	const char*	inFolder)
{
	if (inFolder != nil)
		mCurrentFolder = inFolder;
}

void MJapieApp::DoOpen()
{
	vector<MUrl> urls;
	
	MDocument* doc = nil;
	
	if (ChooseFiles(false, urls))
	{
		for (vector<MUrl>::iterator url = urls.begin(); url != urls.end(); ++url)
			doc = OpenOneDocument(*url);
	}
	
	if (doc != nil)
		DisplayDocument(doc); 
}

// ---------------------------------------------------------------------------
//	OpenOneDocument

MDocument* MJapieApp::OpenOneDocument(
	const MUrl&			inFileRef)
{
	if (inFileRef.IsLocal() and fs::is_directory(inFileRef.GetPath()))
		THROW(("Cannot open a directory"));
	
	MDocument* doc = MDocument::GetDocumentForURL(inFileRef);
	
	if (doc == nil)
	{
		if (inFileRef.IsLocal() and FileNameMatches("*.prj", inFileRef.GetPath()))
			OpenProject(inFileRef.GetPath());
		else
			doc = new MTextDocument(&inFileRef);
	}
	
	if (doc != nil)
	{
		DisplayDocument(doc);
		AddToRecentMenu(inFileRef);
	}
	
	return doc;
}

// ---------------------------------------------------------------------------
//	OpenProject

void MJapieApp::OpenProject(
	const fs::path&		inPath)
{
	auto_ptr<MProject> project(new MProject(inPath));
	auto_ptr<MProjectWindow> w(new MProjectWindow());
	w->Initialize(project.get());
	project.release();
	w->Show();
	w.release();

	AddToRecentMenu(MUrl(inPath));
}

void MJapieApp::AddToRecentMenu(const MUrl& inFileRef)
{
	if (gtk_recent_manager_has_item(mRecentMgr, inFileRef.str().c_str()))
		gtk_recent_manager_remove_item(mRecentMgr, inFileRef.str().c_str(), nil);
	
	gtk_recent_manager_add_item(mRecentMgr, inFileRef.str().c_str());
}

void MJapieApp::DoOpenTemplate(
	const string&		inTemplate)
{
	fs::ifstream file(gTemplatesDir / inTemplate, ios::binary);
	MTextBuffer buffer;
	buffer.ReadFromFile(file);

	string text;
	buffer.GetText(0, buffer.GetSize(), text);
	
	boost::algorithm::replace_all(text, "$date$", GetDateTime());
	boost::algorithm::replace_all(text, "$name$", GetUserName(false));
	boost::algorithm::replace_all(text, "$shortname$", GetUserName(true));
	
	MTextDocument* doc = new MTextDocument(nil);
	doc->SetText(text.c_str(), text.length());
	doc->SetFileNameHint(inTemplate);
	DisplayDocument(doc);
}

void MJapieApp::ShowWorksheet()
{
	fs::path worksheet = gPrefsDir / "Worksheet";
	
	if (not fs::exists(worksheet))
	{
		fs::ofstream file(worksheet);
		string default_text = _(
			"This is a worksheet, you can type shell commands here\n"
			"and execute them by pressing CNTRL-Return or Enter on\n"
			"the numeric keypad.\n"
			"This worksheet will be saved automatically when closed.\n");
		
		file.write(default_text.c_str(), default_text.length());
	}
		
	MDocument* doc = OpenOneDocument(MUrl(worksheet));
	if (doc != nil and dynamic_cast<MTextDocument*>(doc) != nil)
		MTextDocument::SetWorksheet(static_cast<MTextDocument*>(doc));
}

gboolean MJapieApp::Timeout(
	gpointer		inData)
{
	gdk_threads_enter();
	gApp->Pulse();
	gdk_threads_leave();
	return true;
}

void MJapieApp::Pulse()
{
	for (MWindowList::iterator w = mTrashCan.begin(); w != mTrashCan.end(); ++w)
		delete *w;
	
	mTrashCan.clear();
	
	if (mSocketFD >= 0)
		ProcessSocketMessages();
	
	if (gQuit or
		(MWindow::GetFirstWindow() == nil and
		 mReceivedFirstMsg and
		 MProject::Instance() == nil))
	{
		DoQuit();
		gQuit = false;	// in case user cancelled the quit
	}
	else
		eIdle(GetLocalTime());
}

// ----------------------------------------------------------------------------
//	Main routines, forking a client/server e.g.

void my_signal_handler(int inSignal)
{
	switch (inSignal)
	{
		case SIGPIPE:
			break;
		
		case SIGUSR1:
			break;
		
		case SIGINT:
			gQuit = true;
			break;
		
		case SIGTERM:
			gQuit = true;
			break;
	}
}

void error(const char* msg, ...)
{
	fprintf(stderr, "%s stopped with an error:\n", g_get_application_name());
	va_list vl;
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);
	if (errno != 0)
		fprintf(stderr, "\n%s\n", strerror(errno));
	exit(1);
}

struct MSockMsg
{
	uint32		msg;
	int32		length;
};

void MJapieApp::ProcessSocketMessages()
{
	int fd = accept(mSocketFD, nil, nil);
	
	if (fd >= 0)
	{
		mReceivedFirstMsg = true;	
		MDocClosedNotifier notify(fd);		// takes care of closing fd
		
		bool readStdin = false;
		
		for (;;)
		{
			MSockMsg msg = {};
			int r = read(fd, &msg, sizeof(msg));
			
			if (r == 0 or msg.msg == 'done' or msg.length > PATH_MAX)		// done
				break;
			
			char buffer[PATH_MAX + 1];
			if (msg.length > 0)
			{
				r = read(fd, buffer, msg.length);
				if (r != msg.length)
					break;
				buffer[r] = 0;
			}
			
			try
			{
				MDocument* doc = nil;
				int32 lineNr = -1;

				switch (msg.msg)
				{
					case 'open':
						memcpy(&lineNr, buffer, sizeof(lineNr));
						doc = gApp->OpenOneDocument(MUrl(buffer + sizeof(lineNr)));
						break;
					
					case 'new ':
						doc = new MTextDocument(nil);
						break;
					
					case 'data':
						readStdin = true;
						doc = new MTextDocument(nil);
						break;
				}
				
				if (doc != nil)
				{
					DisplayDocument(doc);
					doc->AddNotifier(notify, readStdin);
					
					if (lineNr > 0 and dynamic_cast<MTextDocument*>(doc) != nil)
						static_cast<MTextDocument*>(doc)->GoToLine(lineNr - 1);
				}
			}
			catch (exception& e)
			{
				DisplayError(e);
				readStdin = false;
			}
		}
	}
}

int OpenSocketToServer()
{
	int result = -1;

	struct sockaddr_un addr = {};
	addr.sun_family = AF_LOCAL;
	snprintf(addr.sun_path, sizeof(addr.sun_path), kSocketName, getuid());

	if (fs::exists(fs::path(addr.sun_path)))
	{
		result = socket(AF_LOCAL, SOCK_STREAM, 0);
		if (result < 0)
			cerr << "sockfd failed: " << strerror(errno) << endl;
		else
		{
			int err = connect(result, (const sockaddr*)&addr, sizeof(addr));
			if (err < 0)
			{
				close(result);
				result = -1;
			}
		}
	}
	
	return result;
}

bool ForkServer(
	const vector<pair<int32,MUrl> >&
						inDocs,
	bool				inReadStdin)
{
	int sockfd = OpenSocketToServer();
	
	if (sockfd < 0)
	{
		// no server available, apparently. Create one
		if (fork() == 0)
		{
			// detach from the process group, create new
			// to avoid being killed by a CNTRL-C in the shell
			setpgid(0, 0);

			return true;
		}
		
		sleep(1);
		sockfd = OpenSocketToServer();
	}
	
	if (sockfd < 0)
		error("Failed to open connection to server: %s", strerror(errno));
	
	MSockMsg msg = { };

	if (inReadStdin)
	{
		msg.msg = 'data';
		(void)write(sockfd, &msg, sizeof(msg));
	}
	
	if (inDocs.size() > 0)
	{
		msg.msg = 'open';
		for (vector<pair<int32,MUrl> >::const_iterator d = inDocs.begin(); d != inDocs.end(); ++d)
		{
			int32 lineNr = d->first;
			string url = d->second.str();
			
			msg.length = url.length() + sizeof(lineNr);
			(void)write(sockfd, &msg, sizeof(msg));
			(void)write(sockfd, &lineNr, sizeof(lineNr));
			(void)write(sockfd, url.c_str(), url.length());
		}
	}
	
	if (not inReadStdin and inDocs.size() == 0)
	{
		msg.msg = 'new ';
		(void)write(sockfd, &msg, sizeof(msg));
	}
	
	msg.msg = 'done';
	msg.length = 0;
	(void)write(sockfd, &msg, sizeof(msg));
	
	if (inReadStdin)
	{
		int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
		if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK))
			cerr << _("Failed to set fd non blocking: ") << strerror(errno) << endl;

		for (;;)
		{
			char buffer[10240];
			int r = read(STDIN_FILENO, buffer, sizeof(buffer));
			
			if (r == 0 or (r < 0 and errno != EAGAIN))
				break;
			
			if (r > 0)
				r = write(sockfd, buffer, r);
		}
	}

	// now block until all windows are closed or server dies
	char c;
	read(sockfd, &c, 1);
	
	return false;
}

void usage()
{
	cout << "usage: japi [options] [ [+line] files ]" << endl
		 << "    available options: " << endl
		 << endl
		 << "    -i path    Install japi at location 'path', e.g. /usr/local/bin/japi" << endl
		 << "    -l path    Install locale files at location 'path' " << endl
		 << "                    e.g. /usr/local/share/japi" << endl
		 << "    -h         This help message" << endl
		 << "    -f         Don't fork into client/server" << endl
		 << endl
		 << "  One or more files may be specified, use - for reading from stdin" << endl
		 << endl;
	
	exit(1);
}

void install(
	const char*		inPath)
{
	if (getuid() != 0)
		error("You must be root to be able to install japi");
	
	// copy the executable to the appropriate destination
	
	if (inPath == nil or strlen(inPath) == 0)
		inPath = "/usr/bin/japi";
	
	fs::path dest(inPath);
	
	if (not fs::exists(dest.branch_path()))
		error("Destination directory %s does not seem to exist", 
			dest.branch_path().string().c_str());
	
	fs::path exe(g_get_prgname());
	
	if (not fs::exists(exe))
		error("I don't seem to exist...[%s]?", exe.string().c_str());
	
	cout << "copying " << exe.string() << " to " << dest.string() << endl;
	
	if (fs::exists(dest))
		fs::remove(dest);
		
	fs::copy_file(exe, dest);
	
	// create desktop file
	
	const char* desktop_text;
	uint32 length;
	if (not LoadResource("japi.desktop", desktop_text, length))
		error("japi.desktop file could not be created, missing data");

	string desktop(desktop_text, desktop_text + length);
	string::size_type p;
	if ((p = desktop.find("__EXE__")) == string::npos)
		error("japi.desktop file could not be created, invalid data");
	
	desktop.replace(p, 7, dest.string());
	
	// locate applications directory
	
	fs::path desktopFile, applicationsDir;
	
	const char* const* config_dirs = g_get_system_data_dirs();
	for (const char* const* dir = config_dirs; *dir != nil; ++dir)
	{
		applicationsDir = fs::path(*dir) / "applications";
		if (fs::exists(applicationsDir) and fs::is_directory(applicationsDir))
			break;
	}

	if (not fs::exists(applicationsDir))
	{
		cout << "Could not locate the directory to store the .desktop file." << endl
			 << "Using current directory instead" << endl;
		
		desktopFile = "japi.desktop";
	}
	else
		desktopFile = applicationsDir / "japi.desktop";

	cout << "writing desktop file " << desktopFile.string() << endl;

	fs::ofstream df(desktopFile, ios::trunc);
	df << desktop;
	df.close();
	
	exit(0);
}

void install_locale(
	const char*		inPath)
{
	// write localized strings (Dutch)
	
	fs::ofstream po("/tmp/japi.po", ios::trunc);
	
	const char* po_text;
	uint32 length;
	if (not LoadResource("Dutch/japi.po", po_text, length))
		error("Dutch localisation file could not be created, missing data");
	
	po.write(po_text, length);
	po.close();
	
	fs::path japiLocale(inPath);
	
	fs::create_directories(japiLocale / "locale" / "nl" / "LC_MESSAGES");
	
	string cmd = "msgfmt -o ";
	cmd += (japiLocale / "locale" / "nl" / "LC_MESSAGES" / "japi.mo").string();
	cmd += " /tmp/japi.po";

	cout << "Executing: " << cmd << endl;
	
	(void)system(cmd.c_str());
	
	exit(0);
}

int main(int argc, char* argv[])
{
	struct sigaction act, oact;
	act.sa_handler = my_signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	::sigaction(SIGTERM, &act, &oact);
	::sigaction(SIGUSR1, &act, &oact);
	::sigaction(SIGPIPE, &act, &oact);
	::sigaction(SIGINT, &act, &oact);

	try
	{
		bool fork = true, readStdin = false;
		string target;

		fs::path::default_name_check(fs::no_check);

		/* init threads */	
		g_thread_init(nil);
		gdk_threads_init();

		gtk_init(&argc, &argv);

		gtk_window_set_default_icon_name ("accessories-text-editor");

//		gdk_set_show_events(true);

		vector<pair<int32,MUrl> > docs;
		
		int c;
		while ((c = getopt(argc, const_cast<char**>(argv), "h?fi:l:m:v")) != -1)
		{
			switch (c)
			{
				case 'f':
					fork = false;
					break;
				
				case 'i':
					install(optarg);
					break;
				
				case 'l':
					install_locale(optarg);
					break;
				
				case 'm':
					target = optarg;
					break;
				
				case 'v':
					++VERBOSE;
					break;
				
				default:
					usage();
					break;
			}
		}
		
		if (not target.empty() and optind < argc)
		{
			MProject project(fs::system_complete(argv[optind]));
			project.SelectTarget(target);
			if (project.Make(false))
				cout << "Build successful, " << target << " is up-to-date" << endl;
			else
				cout << "Building " << target << " Failed" << endl;
			exit(0);
		}
		
		if (fs::exists("/usr/local/share/japi/locale"))
			bindtextdomain("japi", "/usr/local/share/japi/locale");
		
		int32 lineNr = -1;
		
		for (int32 i = optind; i < argc; ++i)
		{
			string a(argv[i]);
			
			if (a == "-")
				readStdin = true;
			else if (a.substr(0, 1) == "+")
				lineNr = atoi(a.c_str() + 1);
			else  if (a.substr(0, 7) == "file://" or
				a.substr(0, 7) == "sftp://" or
				a.substr(0, 6) == "ssh://")
			{
				MUrl url(a);
				
				if (url.GetScheme() == "ssh")
					url.SetScheme("sftp");
				
				docs.push_back(make_pair(lineNr, url));
				lineNr = -1;
			}
			else
			{
				docs.push_back(make_pair(lineNr, MUrl(fs::system_complete(a))));
				lineNr = -1;
			}
		}
		
		if (fork == false or ForkServer(docs, readStdin))
		{
			InitGlobals();
	
			gApp = new MJapieApp(fork);

			gApp->RunEventLoop();
	
			// we're done, clean up
			MFindDialog::Instance().Close();
			
			SaveGlobals();
			
			delete gApp;
		}
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

