//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// $Id$

#include "MJapi.h"

#include <sys/un.h>
#include <sys/socket.h>
#include <fcntl.h>
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
#include "MPrinter.h"
#include "MSound.h"
#include "MePubDocument.h"
#include "MePubWindow.h"
#include "MShell.h"

#include "MTestWindow.h"

#include <iostream>

using namespace std;
namespace ba = boost::algorithm;

static bool gQuit = false;

int VERBOSE;

const char
	kAppName[] = "Japi",
	kVersionString[] = "0.9.7-b4";

MJapieApp* gApp;
fs::path gExecutablePath, gPrefixPath;

const char
	kSocketName[] = "/tmp/japi.%d.socket";

namespace {
	
const uint32
	cmd_ImportOEB =			'ImpB';
}

// --------------------------------------------------------------------

MJapieApp::MJapieApp()
	: MHandler(nil)
	, eUpdateSpecialMenu(this, &MJapieApp::UpdateSpecialMenu)
	, mSocketFD(-1)
	, mQuit(false)
	, mQuitPending(false)
	, mInitialized(false)
{
	// set the global pointing to us
	gApp = this;
}

MJapieApp::~MJapieApp()
{
	if (mSocketFD >= 0)
		close(mSocketFD);
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
				"copyright", "Copyright © 2007-2009 Maarten L. Hekkelman",
				"comments", _("A simple development environment"),
				"website", "http://www.hekkelman.com/",
				nil);
			break;
		}
		
		case cmd_PageSetup:
			MPrinter::DoPageSetup();
			break;
		
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
		
		case cmd_NewEPub:
			DoNewEPub();
			break;
		
		case cmd_ImportOEB:
			ImportOEB();
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
		
		case cmd_OpenTemplate:
			if (inModifiers & GDK_CONTROL_MASK)
				OpenOneDocument(MFile(gTemplatesDir / inMenu->GetItemLabel(inItemIndex)));
			else
				DoOpenTemplate(inMenu->GetItemLabel(inItemIndex));
			break;
		
		case cmd_ApplyScript:
		{
			MFile url(gScriptsDir / inMenu->GetItemLabel(inItemIndex));
			OpenOneDocument(url);
			break;
		}
		
		case cmd_Find:
			MFindDialog::Instance().Select();
			break;
		
		case cmd_FindNext:
			if (not MFindDialog::Instance().FindNext())
				PlaySound("warning");
			break;
	
		case cmd_ReplaceFindNext:
			if (not MFindDialog::Instance().FindNext())
				PlaySound("warning");
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
			MWindow* w = new MTestWindow();
			w->Show();
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
		case cmd_OpenTemplate:
		case cmd_CloseAll:
		case cmd_SaveAll:
		case cmd_PageSetup:
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

void MJapieApp::UpdateSpecialMenu(
	const std::string&	inName,
	MMenu*				inMenu)
{
	if (inName == "window")
		UpdateWindowMenu(inMenu);
	else if (inName == "template")
		UpdateTemplateMenu(inMenu);
	else if (inName == "scripts")
		UpdateScriptsMenu(inMenu);
	else if (inName == "epub")
		UpdateEPubMenu(inMenu);
	else
		PRINT(("Unknown special menu %s", inName.c_str()));
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
				const MFile& url = doc->GetFile();
				
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
	const uint32 kStandardNewMenuItems = 4;
	
	inMenu->RemoveItems(
		kStandardNewMenuItems, inMenu->CountItems() - kStandardNewMenuItems);

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

void MJapieApp::UpdateEPubMenu(
	MMenu*				inMenu)
{
	inMenu->RemoveItems(0, inMenu->CountItems());

	MePubDocument* epub = MePubDocument::GetFirstEPubDocument();
	while (epub != nil)
	{
		inMenu->AppendItem(epub->GetFile().GetFileName(), cmd_SaveInEPub);
		epub = epub->GetNextEPubDocument();
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

void MJapieApp::RunEventLoop()
{
	try
	{
		if (Preferences::GetInteger("open worksheet", 0))
			ShowWorksheet();
		
		if (Preferences::GetInteger("reopen project", 0))
		{
			MFile pp(fs::path(Preferences::GetString("last project", "")));
			if (pp.IsLocal() and pp.Exists())
				OpenProject(pp);
		}
	}
	catch (...) {}

	uint32 snooper = gtk_key_snooper_install(&MJapieApp::Snooper, nil);
	
	g_timeout_add(50, &MJapieApp::Timeout, nil);

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
				cerr << _("Weird, document without controller: ") << doc->GetFile() << endl;
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
	MDocument* doc = MDocument::Create<MTextDocument>(MFile());
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
		
		MFile url(uri, true);
		g_free(uri);
		
		if (not url.IsLocal())
			THROW(("Projects can only be created on local file systems, sorry"));

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
		mrsrc::rsrc rsrc("Templates/Projects/hello-cmdline.prj");

		if (not rsrc)
			THROW(("Failed to load project resource"));
		
		file.write(rsrc.data(), rsrc.size());
		file.close();
		
		// and write out a sample source file
		fs::ofstream srcfile(p / "src" / "HelloWorld.cpp");
		if (not srcfile.is_open())
			THROW(("Failed to create project file"));
		
		rsrc = mrsrc::rsrc("Templates/Projects/hello.cpp");
		if (not rsrc)
			THROW(("Failed to load project sourcefile resource"));
		
		srcfile.write(rsrc.data(), rsrc.size());
		srcfile.close();
		
		OpenProject(MFile(projectFile));
	}
	
	gtk_widget_destroy(dialog);
}

void MJapieApp::DoNewEPub()
{
	auto_ptr<MePubDocument> epub(new MePubDocument());
	epub->InitializeNew();
	
	auto_ptr<MePubWindow> w(new MePubWindow());
	w->Initialize(epub.get());
	epub.release();
	w->Show();
	w.release();
}

void MJapieApp::ImportOEB()
{
	MFile oeb;
	if (ChooseOneFile(oeb))
	{
		auto_ptr<MePubDocument> epub(new MePubDocument());
		epub->ImportOEB(oeb);
		
		auto_ptr<MePubWindow> w(new MePubWindow());
		w->Initialize(epub.get());
		epub.release();
		w->Show();
		w.release();
	}
}

void MJapieApp::SetCurrentFolder(
	const char*	inFolder)
{
	if (inFolder != nil)
		mCurrentFolder = inFolder;
}

void MJapieApp::DoOpen()
{
	vector<MFile> urls;
	
	MDocument* doc = nil;
	
	if (ChooseFiles(false, urls))
	{
		for (vector<MFile>::iterator url = urls.begin(); url != urls.end(); ++url)
			doc = OpenOneDocument(*url);
	}
	
	if (doc != nil)
		DisplayDocument(doc); 
}

// ---------------------------------------------------------------------------
//	OpenOneDocument

MDocument* MJapieApp::OpenOneDocument(
	const MFile&			inFileRef)
{
	if (inFileRef.IsLocal() and fs::is_directory(inFileRef.GetPath()))
		THROW(("Cannot open a directory"));
	
	MDocument* doc = MDocument::GetDocumentForFile(inFileRef);
	
	if (doc == nil)
	{
		if (FileNameMatches("*.prj", inFileRef))
		{
			if (not inFileRef.IsLocal())
				THROW(("Can only open local project files for now, sorry"));
			OpenProject(inFileRef);
		}
		else if (FileNameMatches("*.epub", inFileRef))
			OpenEPub(inFileRef);
		else
			doc = MDocument::Create<MTextDocument>(inFileRef);
	}
	
	if (doc != nil)
	{
		DisplayDocument(doc);
		MMenu::AddToRecentMenu(inFileRef);
	}
	
	return doc;
}

// ---------------------------------------------------------------------------
//	OpenProject

void MJapieApp::OpenProject(
	const MFile&		inPath)
{
	auto_ptr<MDocument> project(MDocument::Create<MProject>(inPath));
	auto_ptr<MProjectWindow> w(new MProjectWindow());
	w->Initialize(project.get());
	project.release();
	w->Show();
	w.release();

	MMenu::AddToRecentMenu(inPath);
}

// ---------------------------------------------------------------------------
//	OpenEPub

void MJapieApp::OpenEPub(
	const MFile&		inPath)
{
	try
	{
		auto_ptr<MDocument> epub(MDocument::Create<MePubDocument>(inPath));
		auto_ptr<MePubWindow> w(new MePubWindow());
		
		w->Initialize(epub.get());
		epub.release();

		w->Show();
		w.release();
	
		MMenu::AddToRecentMenu(inPath);
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
}

void MJapieApp::DoOpenTemplate(
	const string&		inTemplate)
{
	fs::ifstream file(gTemplatesDir / inTemplate, ios::binary);
	MTextBuffer buffer;
	buffer.ReadFromFile(file);

	string text;
	buffer.GetText(0, buffer.GetSize(), text);
	
	ba::replace_all(text, "$date$", GetDateTime());
	ba::replace_all(text, "$name$", GetUserName(false));
	ba::replace_all(text, "$shortname$", GetUserName(true));
	
	MTextDocument* doc = MDocument::Create<MTextDocument>(MFile());
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
		
	MDocument* doc = OpenOneDocument(MFile(worksheet));
	if (doc != nil and dynamic_cast<MTextDocument*>(doc) != nil)
		MTextDocument::SetWorksheet(static_cast<MTextDocument*>(doc));
}

gboolean MJapieApp::Timeout(
	gpointer		inData)
{
	gdk_threads_enter();
	try
	{
		gApp->Pulse();
	}
	catch (exception& e)
	{
		DisplayError(e);
	}
	gdk_threads_leave();
	return true;
}

void MJapieApp::Pulse()
{
	MWindow::RecycleWindows();
	
	if (mSocketFD >= 0)
		ProcessSocketMessages();

	if (gQuit or
		(mInitialized and
		 MWindow::GetFirstWindow() == nil and
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
						doc = gApp->OpenOneDocument(MFile(buffer + sizeof(lineNr)));
						break;
					
					case 'new ':
						doc = MDocument::Create<MTextDocument>(MFile());
						break;
					
					case 'data':
						readStdin = true;
						doc = MDocument::Create<MTextDocument>(MFile());
						break;
				}
				
				if (doc != nil)
				{
					mInitialized = true;
					
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

namespace {

int OpenSocket(
	struct sockaddr_un&		addr)
{
	int sockfd = -1;
	
	if (fs::exists(fs::path(addr.sun_path)))
	{
		sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
		if (sockfd < 0)
			cerr << "creating socket failed: " << strerror(errno) << endl;
		else
		{
			int err = connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
			if (err < 0)
			{
				close(sockfd);
				sockfd = -1;

				unlink(addr.sun_path);	// allowed to fail
			}
		}
	}
	
	return sockfd;
}
	
}

bool MJapieApp::IsServer()
{
	bool isServer = false;

	struct sockaddr_un addr = {};
	addr.sun_family = AF_LOCAL;
	snprintf(addr.sun_path, sizeof(addr.sun_path), kSocketName, getuid());
	
	mSocketFD = OpenSocket(addr);

	if (mSocketFD == -1)
	{
		isServer = true;

		// So we're supposed to become a server
		// We will fork here and open a new socket
		// We use a pipe to let the client wait for the server

		int fd[2];
		(void)pipe(fd);
		
		int pid = fork();
		
		if (pid == -1)
		{
			close(fd[0]);
			close(fd[1]);
			
			cerr << _("Fork failed: ") << strerror(errno) << endl;
			
			return false;
		}
		
		if (pid == 0)	// forked process (child which becomes the server)
		{
			// detach from the process group, create new
			// to avoid being killed by a CNTRL-C in the shell
			setpgid(0, 0);

			// now setup the socket
			mSocketFD = socket(AF_LOCAL, SOCK_STREAM, 0);
			int err = ::bind(mSocketFD, (const sockaddr*)&addr, SUN_LEN(&addr));
		
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
			
			write(fd[1], " ", 1);
			close(fd[0]);
			close(fd[1]);
		}
		else
		{
			// client (parent process). Wait until the server has finished setting up the socket.
			isServer = false;
			
			char c;
			(void)read(fd[0], &c, 1);

			close(fd[1]);
			close(fd[0]);
			
			// the socket should now really exist
			mSocketFD = OpenSocket(addr);
		}
	}
	
	return isServer;
}

bool MJapieApp::IsClient()
{
	return mSocketFD >= 0;
}

void MJapieApp::ProcessArgv(
	bool				inReadStdin,
	MFilesToOpenList&	inDocs)
{
	MSockMsg msg = { };

	if (inReadStdin)
	{
		msg.msg = 'data';
		(void)write(mSocketFD, &msg, sizeof(msg));
	}

	if (inDocs.size() > 0)
	{
		msg.msg = 'open';
		for (MJapieApp::MFilesToOpenList::const_iterator d = inDocs.begin(); d != inDocs.end(); ++d)
		{
			int32 lineNr = d->first;
			string url = d->second.GetURI();
			
			msg.length = url.length() + sizeof(lineNr);
			(void)write(mSocketFD, &msg, sizeof(msg));
			(void)write(mSocketFD, &lineNr, sizeof(lineNr));
			(void)write(mSocketFD, url.c_str(), url.length());
		}
	}
	
	if (not inReadStdin and inDocs.size() == 0)
	{
		msg.msg = 'new ';
		(void)write(mSocketFD, &msg, sizeof(msg));
	}
	
	msg.msg = 'done';
	msg.length = 0;
	(void)write(mSocketFD, &msg, sizeof(msg));

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
				r = write(mSocketFD, buffer, r);
		}
	}

	// now block until all windows are closed or server dies
	char c;
	read(mSocketFD, &c, 1);
}

void usage()
{
	cout << "usage: japi [options] [ [+line] files ]" << endl
		 << "    available options: " << endl
		 << endl
		 << "    -i         Install japi at prefix where prefix path" << endl
		 << "    -p prefix  Prefix path where to install japi, default is /usr/local" << endl
		 << "               resulting in /usr/local/bin/japi" << endl
		 << "    -h         This help message" << endl
		 << "    -f         Don't fork into client/server mode" << endl
		 << endl
		 << "  One or more files may be specified, use - for reading from stdin" << endl
		 << endl;
	
	exit(1);
}

void InstallJapi(
	std::string		inPrefix)
{
	if (getuid() != 0)
		error("You must be root to be able to install japi");
	
	// copy the executable to the appropriate destination
	if (inPrefix.empty())
		inPrefix = "/usr/local";
	
	fs::path prefix(inPrefix);
	
	if (not fs::exists(prefix / "bin"))
	{
		cout << "Creating directory " << (prefix / "bin") << endl;
		fs::create_directories(prefix / "bin");
	}

	if (not fs::exists(gExecutablePath))
		error("I don't seem to exist...[%s]?", gExecutablePath.string().c_str());

	fs::path dest = prefix / "bin" / "japi";
	cout << "copying " << gExecutablePath.string() << " to " << dest.string() << endl;
	
	if (fs::exists(dest))
		fs::remove(dest);
		
	fs::copy_file(gExecutablePath, dest);
	
	// create desktop file
		
	mrsrc::rsrc rsrc("japi.desktop");
	if (not rsrc)
		error("japi.desktop file could not be created, missing data");

	string desktop(rsrc.data(), rsrc.size());
	ba::replace_first(desktop, "__EXE__", dest.string());
	
	// locate applications directory
	// don't use glib here, 
	
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
		cout << "Creating directory " << applicationsDir << endl;
		fs::create_directories(applicationsDir);
	}

	desktopFile = applicationsDir / "japi.desktop";
	cout << "writing desktop file " << desktopFile << endl;

	fs::ofstream df(desktopFile, ios::trunc);
	df << desktop;
	df.close();
	
	// write out all locale files
	
	mrsrc::rsrc_list loc_rsrc = mrsrc::rsrc("Locale").children();
	for (mrsrc::rsrc_list::iterator l = loc_rsrc.begin(); l != loc_rsrc.end(); ++l)
	{
		mrsrc::rsrc_list loc_files = l->children();
		if (loc_files.empty())
			continue;
		
		rsrc = loc_files.front();
		if (not rsrc or rsrc.name() != "japi.po")
			continue;
		
		fs::path localeDir =
			applicationsDir.branch_path() / "japi" / "locale" / l->name() / "LC_MESSAGES";
		
		if (not fs::exists(localeDir))
		{
			cout << "Creating directory " << localeDir << endl;
			fs::create_directories(localeDir);
		}
		
		stringstream cmd;
		cmd << "msgfmt -o " << (localeDir / "japi.mo") << " -";
		cout << "Installing locale file: `" << cmd.str() << '`' << endl;
		
		FILE* f = popen(cmd.str().c_str(), "w");
		if (f != nil)
		{
			fwrite(rsrc.data(), rsrc.size(), 1, f);
			pclose(f);
		}
		else
			cout << "failed: " << strerror(errno) << endl;
	}
	
	exit(0);
}

int main(int argc, char* argv[])
{
	try
	{
		fs::path::default_name_check(fs::no_check);

		// First find out who we are. Uses proc filesystem to find out.
		char exePath[PATH_MAX + 1];
		
		int r = readlink("/proc/self/exe", exePath, PATH_MAX);
		if (r > 0)
		{
			exePath[r] = 0;
			gExecutablePath = fs::system_complete(exePath);
			gPrefixPath = gExecutablePath.branch_path();
		}
		
		if (not fs::exists(gExecutablePath))
			gExecutablePath = fs::system_complete(argv[0]);
	
		// Collect the options
		int c;
		bool fork = true, readStdin = false, install = false;
		string target, prefix;

		while ((c = getopt(argc, const_cast<char**>(argv), "h?fip:m:vt")) != -1)
		{
			switch (c)
			{
				case 'f':
					fork = false;
					break;
				
				case 'i':
					install = true;
					break;
				
				case 'p':
					prefix = optarg;
					break;
				
				case 'm':
					target = optarg;
					break;
#if DEBUG
				case 'v':
					++VERBOSE;
					break;
#endif
				default:
					usage();
					break;
			}
		}
		
		if (install)
		{
			gtk_init(&argc, &argv);
			InstallJapi(prefix);
		}
		
		// if the option was to build a target, try it and exit.
		if (not target.empty())
		{
			if (optind >= argc)
				THROW(("You should specify a project file to use for building"));
			
			MFile file(fs::system_complete(argv[optind]));
			
			auto_ptr<MProject> project(MDocument::Create<MProject>(file));
			project->SelectTarget(target);
			if (project->Make(false))
				cout << "Build successful, " << target << " is up-to-date" << endl;
			else
				cout << "Building " << target << " Failed" << endl;
			exit(0);
		}

		// setup locale, if we can find it.
		fs::path localeDir = gPrefixPath / "share" / "japi" / "locale";
		if (not fs::exists(localeDir))
			localeDir = fs::path("/usr/local/share/japi/locale");
		if (not fs::exists(localeDir))
			localeDir = fs::path("/usr/share/japi/locale");
		if (fs::exists(localeDir))
		{
			setlocale(LC_CTYPE, "");
			setlocale(LC_MESSAGES, "");
			bindtextdomain("japi", localeDir.string().c_str());
			textdomain("japi");
		}
		
		// see if we need to open any files from the commandline
		int32 lineNr = -1;
		MJapieApp::MFilesToOpenList filesToOpen;
		
		for (int32 i = optind; i < argc; ++i)
		{
			string a(argv[i]);
			
			if (a == "-")
				readStdin = true;
			else if (a.substr(0, 1) == "+")
				lineNr = atoi(a.c_str() + 1);
			else
			{
				filesToOpen.push_back(make_pair(lineNr, MFile(a)));
				lineNr = -1;
			}
		}
		
		MJapieApp app;
		
		if (fork == false or app.IsServer())
		{
			g_thread_init(nil);
			gdk_threads_init();
			gtk_init(&argc, &argv);
	
			InitGlobals();
			
			// now start up the normal executable		
			gtk_window_set_default_icon_name ("accessories-text-editor");
	
			struct sigaction act, oact;
			act.sa_handler = my_signal_handler;
			sigemptyset(&act.sa_mask);
			act.sa_flags = 0;
			::sigaction(SIGTERM, &act, &oact);
			::sigaction(SIGUSR1, &act, &oact);
			::sigaction(SIGPIPE, &act, &oact);
			::sigaction(SIGINT, &act, &oact);
	
			gdk_notify_startup_complete();

			app.RunEventLoop();
		
			// we're done, clean up
			MFindDialog::Instance().Close();
			
			SaveGlobals();
	
			if (fork)
			{
				char path[1024] = {};
				snprintf(path, sizeof(path), kSocketName, getuid());
				unlink(path);
			}
		}
		else if (app.IsClient())
			app.ProcessArgv(readStdin, filesToOpen);
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

