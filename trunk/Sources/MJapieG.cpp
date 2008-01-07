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

#include <sys/un.h>
#include <sys/socket.h>
#include <cerrno>
#include <signal.h>

#include <boost/filesystem/fstream.hpp>

#include "MDocument.h"
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

#include <iostream>

using namespace std;

static bool gQuit = false;

const char
	kAppName[] = "Japi",
	kVersionString[] = "0.9";

MJapieApp* gApp;

const char
	kSocketName[] = "/tmp/japi.%d.socket";

// --------------------------------------------------------------------

MJapieApp::MJapieApp()
	: MHandler(nil)
	, mRecentMgr(gtk_recent_manager_get_default())
	, mSocketFD(-1)
	, mReceivedFirstMsg(false)
	, mQuit(false)
	, mQuitPending(false)
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

MJapieApp::~MJapieApp()
{
	if (mSocketFD >= 0)
		close(mSocketFD);
	
	char path[1024] = {};
	snprintf(path, sizeof(path), kSocketName, getuid());
	unlink(path);
}

bool MJapieApp::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex)
{
	bool result = true;

//	MProject* project = MProject::Instance();
//	if (project != nil and project->ProcessCommand(inCommand, inMenu, inItemIndex))
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
			DoOpenTemplate(inMenu->GetItemLabel(inItemIndex));
			break;
		
		case cmd_Find:
			MFindDialog::Instance().Select();
			break;
	
		case cmd_FindInNextFile:
			MFindDialog::Instance().FindNext();
			break;
		
		case cmd_OpenIncludeFile:
			new MFindAndOpenDialog(nil, nil);
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
			break;
		
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
		if (not doc->IsWorksheet())
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

	MPath templatesDir = gTemplatesDir;
	if (fs::exists(templatesDir) and fs::is_directory(templatesDir))
	{
		MFileIterator iter(templatesDir, 0);
		
		MPath file;
		while (iter.Next(file))
			inMenu->AppendItem(file.leaf(), cmd_OpenTemplate);
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
	if (Preferences::GetInteger("open worksheet", 0))
		ShowWorksheet();
	
	if (Preferences::GetInteger("reopen project", 1))
	{
		MPath pp = Preferences::GetString("last project", "");
		if (fs::exists(pp))
			OpenProject(pp);
	}

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
				result = handler->ProcessCommand(cmd, nil, 0);
		}
	}
	catch (exception& e)
	{
		MError::DisplayError(e);
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
		
		if (not doc->IsModified() and not doc->IsWorksheet())
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

	if (MDocument::GetFirstDocument() == nil)
	{
		if (MProject::Instance() != nil)
		{
			string p = MProject::Instance()->GetPath().string();
			Preferences::SetString("last project", p);
		}
		
		MProject::CloseAllProjects(kSaveChangesQuittingApplication);
	}
	
	if (MDocument::GetFirstDocument() == nil and MProject::Instance() == nil)
		gtk_main_quit();
}

void MJapieApp::DoNew()
{
	MDocument*	doc = new MDocument(nil);
	MDocWindow::DisplayDocument(doc);
}

void MJapieApp::SetCurrentFolder(
	const char*	inFolder)
{
	if (inFolder != nil)
		mCurrentFolder = inFolder;
}

void MJapieApp::DoOpen()
{
	GtkWidget* dialog = nil;
	MDocument* doc = nil;
	
	try
	{
		dialog = 
			gtk_file_chooser_dialog_new(_("Open"), nil,
				GTK_FILE_CHOOSER_ACTION_OPEN,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				NULL);
		
		THROW_IF_NIL(dialog);
	
		gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), true);
		gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), false);
		
		if (mCurrentFolder.length() > 0)
		{
			gtk_file_chooser_set_current_folder(
				GTK_FILE_CHOOSER(dialog), mCurrentFolder.c_str());
		}
		
		vector<MUrl> urls;
		
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			GSList* uris = gtk_file_chooser_get_uris(GTK_FILE_CHOOSER(dialog));	
			
			GSList* file = uris;	
			
			while (file != nil)
			{
				MUrl url(reinterpret_cast<char*>(file->data));

				g_free(file->data);
				file->data = nil;

				urls.push_back(url);

				file = file->next;
			}
			
			g_slist_free(uris);
		}
		
		for (vector<MUrl>::iterator url = urls.begin(); url != urls.end(); ++url)
			doc = OpenOneDocument(*url);
		
		char* cwd = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		if (cwd != nil)
		{
			mCurrentFolder = cwd;
			g_free(cwd);
		}
	}
	catch (exception& e)
	{
		if (dialog)
			gtk_widget_destroy(dialog);
		
		throw;
	}
	
	gtk_widget_destroy(dialog);
	
	if (doc != nil)
		MDocWindow::DisplayDocument(doc); 
}

// ---------------------------------------------------------------------------
//	OpenOneDocument

MDocument* MJapieApp::OpenOneDocument(
	const MUrl&			inFileRef)
{
	AddToRecentMenu(inFileRef);

	MDocument* doc = nil;
	
	if (inFileRef.IsLocal() and FileNameMatches("*.prj", inFileRef.GetPath()))
		OpenProject(inFileRef.GetPath());
	else
	{
		doc = MDocument::GetDocumentForURL(inFileRef, false);
	
		if (doc == nil)
			doc = new MDocument(&inFileRef);
	
		MDocWindow::DisplayDocument(doc);
	}
	
	return doc;
}

// ---------------------------------------------------------------------------
//	OpenProject

void MJapieApp::OpenProject(
	const MPath&		inPath)
{
	AddToRecentMenu(MUrl(inPath));
	
	auto_ptr<MProject> project(new MProject(inPath));
	project->Initialize();
	project->Show();
	project.release();
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
	
	string::size_type offset = 0;
	
	while ((offset = text.find("$date$", offset)) != string::npos)
		text.replace(offset, 6, GetDateTime());

	offset = 0;
	while ((offset = text.find("$name$", offset)) != string::npos)
		text.replace(offset, 6, GetUserName(false));
	
	offset = 0;
	while ((offset = text.find("$shortname$", offset)) != string::npos)
		text.replace(offset, 11, GetUserName(true));
	
	MDocument* doc = new MDocument(nil);
	doc->SetText(text.c_str(), text.length());
	doc->SetFileNameHint(inTemplate);
	MDocWindow::DisplayDocument(doc);
}

void MJapieApp::ShowWorksheet()
{
	MPath worksheet = gPrefsDir / "Worksheet";
	
	if (not fs::exists(worksheet))
	{
		fs::ofstream file(worksheet);
		string default_text = _(
			"This is a worksheet, you can type shell commands here\n"
			"and execute them by pressing CNTRL-Return or Enter on\n"
			"the numeric keypad.\n"
			"This worksheet will be saved automatically when you close it.");
		
		file.write(default_text.c_str(), default_text.length());
	}
		
	MDocument* doc = OpenOneDocument(MUrl(worksheet));
	if (doc != nil)
		doc->SetWorksheet(true);
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
		MWindow::GetFirstWindow() == nil and
		mReceivedFirstMsg and
		MProject::Instance() == nil)
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
	fprintf(stderr, "Error launching %s\n", g_get_application_name());
	va_list vl;
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);
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

				switch (msg.msg)
				{
					case 'open':
						doc = gApp->OpenOneDocument(MUrl(buffer));
						break;
					
					case 'new ':
						doc = new MDocument(nil);
						break;
					
					case 'data':
						readStdin = true;
						doc = new MDocument(nil);
						break;
				}
				
				if (doc != nil)
				{
					MDocWindow::DisplayDocument(doc);
					doc->AddNotifier(notify, readStdin);
				}
			}
			catch (exception& e)
			{
				MError::DisplayError(e);
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

	if (fs::exists(MPath(addr.sun_path)))
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
	const vector<MUrl>&	inDocs,
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
		write(sockfd, &msg, sizeof(msg));
	}
	
	if (inDocs.size() > 0)
	{
		msg.msg = 'open';
		for (vector<MUrl>::const_iterator d = inDocs.begin(); d != inDocs.end(); ++d)
		{
			string url = d->str();
			
			msg.length = url.length();
			write(sockfd, &msg, sizeof(msg));
			write(sockfd, url.c_str(), url.length());
		}
	}
	
	if (not inReadStdin and inDocs.size() == 0)
	{
		msg.msg = 'new ';
		write(sockfd, &msg, sizeof(msg));
	}
	
	msg.msg = 'done';
	msg.length = 0;
	write(sockfd, &msg, sizeof(msg));
	
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
	cout << "usage: japi [options] [ files ]" << endl
		 << "    available options: " << endl
		 << "    -h      This help message" << endl
		 << "    -f      Don't fork into client/server" << endl
		 << endl
		 << "  One or more files may be specified, use - for reading from stdin" << endl
		 << endl;
	
	exit(1);
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

		fs::path::default_name_check(fs::no_check);

		/* init threads */	
		g_thread_init(nil);
		gdk_threads_init();

		gtk_init(&argc, &argv);

		gtk_window_set_default_icon_name ("accessories-text-editor");

//		gdk_set_show_events(true);

		vector<MUrl> docs;
		
		int c;
		while ((c = getopt(argc, const_cast<char**>(argv), "h?f")) != -1)
		{
			switch (c)
			{
				case 'f':
					fork = false;
					break;
				
				default:
					usage();
					break;
			}
		}
		
		for (int32 i = optind; i < argc; ++i)
		{
			string a(argv[i]);
			
			if (a == "-")
				readStdin = true;
			else  if (a.substr(0, 7) == "file://" or
				a.substr(0, 7) == "sftp://" or
				a.substr(0, 6) == "ssh://")
			{
				MUrl url(a);
				
				if (url.GetScheme() == "ssh")
					url.SetScheme("sftp");
				
				docs.push_back(url);
			}
			else
				docs.push_back(MUrl(fs::system_complete(a)));
		}
		
		if (fork == false or ForkServer(docs, readStdin))
		{
			InitGlobals();
	
			gApp = new MJapieApp();

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

