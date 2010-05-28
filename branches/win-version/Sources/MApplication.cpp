//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// $Id$

#include "MLib.h"

#include <iostream>

#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "MApplication.h"
#include "MApplicationImpl.h"
#include "MCommands.h"
#include "MDocument.h"
#include "MPreferences.h"
#include "MWindow.h"
#include "MUtils.h"

#include "CTestView.h"
#include "MControls.h"

using namespace std;
namespace ba = boost::algorithm;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

static bool gQuit = false;

#if DEBUG
int VERBOSE, TRACE;
#endif

MApplication* gApp;
fs::path gExecutablePath, gPrefixPath;

// --------------------------------------------------------------------

MApplicationImpl::MApplicationImpl(
	MApplication*		inApp)
	: mApp(inApp)
{
}

MApplicationImpl::~MApplicationImpl()
{
}

void MApplicationImpl::Pulse()
{
	mApp->Pulse();
}

// --------------------------------------------------------------------

MApplication::MApplication()
	: MHandler(nil)
	, mImpl(MApplicationImpl::Create(this))
	, mQuit(false)
	, mQuitPending(false)
	, mInitialized(false)
{
	// set the global pointing to us
	gApp = this;
}

MApplication::~MApplication()
{
	delete mImpl;
}

bool MApplication::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;

	switch (inCommand)
	{
		//case cmd_About:
		//{
		//	MWindow* w = MWindow::GetFirstWindow();
		//	GtkWidget* ww = nil;
		//	if (w != nil)
		//		ww = w->GetGtkWidget();
		//	
		//	gtk_show_about_dialog(GTK_WINDOW(ww),
		//		"program_name", kAppName,
		//		"version", kVersionString,
		//		"copyright", "Copyright © 2007-2009 Maarten L. Hekkelman",
		//		"comments", _("A simple development environment"),
		//		"website", "http://www.hekkelman.com/",
		//		nil);
		//	break;
		//}
		
		//case cmd_PageSetup:
		//	MPrinter::DoPageSetup();
		//	break;
		
		//case cmd_Preferences:
		//	MPrefsDialog::Create();
		//	break;
		
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
		
		case cmd_SelectWindowFromMenu:
			DoSelectWindowFromWindowMenu(inItemIndex - 2);
			break;
		
		case 'test':
		{
//			MWindow* w = new MTestWindow();
//			w->Show();
			break;
		}
		
		default:
			// MApplication is the last in the hierarchy of handlers
			result = false;
			break;
	}
	
	return result;
}

bool MApplication::UpdateCommandStatus(
	uint32			inCommand,
	MMenu*			inMenu,
	uint32			inItemIndex,
	bool&			outEnabled,
	bool&			outChecked)
{
	bool result = true;

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

void MApplication::UpdateWindowMenu(
	MMenu*				inMenu)
{
	//inMenu->RemoveItems(2, inMenu->CountItems() - 2);
	//
	//MDocument* doc = MDocument::GetFirstDocument();
	//while (doc != nil)
	//{
	//	if (doc != MTextDocument::GetWorksheet())
	//	{
	//		string label;
	//		
	//		if (doc->IsSpecified())
	//		{
	//			const MFile& url = doc->GetFile();
	//			
	//			if (not url.IsLocal())
	//				label += url.GetScheme() + ':';
	//			
	//			label += url.GetFileName();
	//		}
	//		else
	//		{
	//			MDocWindow* w = MDocWindow::FindWindowForDocument(doc);
	//			if (w != nil)
	//				label = w->GetTitle();
	//			else
	//				label = _("weird");
	//		}
	//		
	//		inMenu->AppendItem(label, cmd_SelectWindowFromMenu);
	//	}
	//	
	//	doc = doc->GetNextDocument();
	//}	
}

void MApplication::DoSelectWindowFromWindowMenu(
	uint32				inIndex)
{
	//MDocument* doc = MDocument::GetFirstDocument();

	//while (doc != nil and inIndex-- > 0)
	//	doc = doc->GetNextDocument();
	//
	//if (doc != nil)
	//	DisplayDocument(doc);	
}	

int MApplication::RunEventLoop()
{
	return mImpl->RunEventLoop();
}

void MApplication::DoSaveAll()
{
	//MDocument* doc = MDocument::GetFirstDocument();
	//
	//while (doc != nil)
	//{
	//	if (doc->IsSpecified() and doc->IsModified())
	//		doc->DoSave();
	//	doc = doc->GetNextDocument();
	//}
	//
	//doc = MDocument::GetFirstDocument();

	//while (doc != nil)
	//{
	//	if (not doc->IsSpecified() and doc->IsModified())
	//	{
	//		MController* controller = doc->GetFirstController();

	//		assert(controller != nil);
	//		
	//		controller->SaveDocumentAs();
	//	}
	//	
	//	doc = doc->GetNextDocument();
	//}
}

void MApplication::DoCloseAll(
	MCloseReason		inAction)
{
//	// first close all that can be closed
//
//	MDocument* doc = MDocument::GetFirstDocument();
//	
//	while (doc != nil)
//	{
//		MDocument* next = doc->GetNextDocument();
//		
//		if (dynamic_cast<MTextDocument*>(doc) != nil and
//			doc != MTextDocument::GetWorksheet() and
//			not doc->IsModified())
//		{
//			MController* controller = doc->GetFirstController();
//			
////			assert(controller != nil);
//			
//			if (controller != nil)
//				(void)controller->TryCloseDocument(inAction);
//			else
//				cerr << _("Weird, document without controller: ") << doc->GetFile() << endl;
//		}
//		
//		doc = next;
//	}
//	
//	// then close what remains
//
//	doc = MDocument::GetFirstDocument();
//
//	while (doc != nil)
//	{
//		MDocument* next = doc->GetNextDocument();
//
//		MController* controller = doc->GetFirstController();
//		assert(controller != nil);
//
//		if (doc == MTextDocument::GetWorksheet())
//		{
//			if (inAction == kSaveChangesQuittingApplication)
//			{
//				doc->DoSave();
//				
//				if (not controller->TryCloseDocument(inAction))
//					break;
//			}
//		}
//		else if (dynamic_cast<MTextDocument*>(doc) != nil or
//			inAction == kSaveChangesQuittingApplication)
//		{
//			if (not controller->TryCloseDocument(inAction))
//				break;
//		}
//		
//		doc = next;
//	}
}

void MApplication::DoQuit()
{
	//if (MProject::Instance() != nil)
	//{
	//	string p = MProject::Instance()->GetPath().string();
	//	Preferences::SetString("last project", p);
	//}

	DoCloseAll(kSaveChangesQuittingApplication);

	//if (MDocument::GetFirstDocument() == nil and MProject::Instance() == nil)
	//{
	//	MFindDialog::Instance().Close();
		mImpl->Quit();
	//}
}

MDocWindow* MApplication::DisplayDocument(
	MDocument*		inDocument)
{
	//MDocWindow* result = MDocWindow::FindWindowForDocument(inDocument);
	//
	//if (result == nil)
	//{
	//	if (dynamic_cast<MTextDocument*>(inDocument) != nil)
	//	{
	//		MEditWindow* e = new MEditWindow;
	//		e->Initialize(inDocument);
	//		e->Show();

	//		result = e;
	//	}
	//}
	//
	//if (result != nil)
	//	result->Select();
	//
	//return result;
	return nil;
}

void MApplication::DoNew()
{
	MWindow* w = new MWindow("Aap noot mies", MRect(10, 10, 210, 210), kMPostionDefault, "edit-window-menu");

	MRect bounds(10, 10, 75, 23);

	MButton* button = new MButton('okok', bounds, "OK");

	button->Show();
	button->Enable();

	w->AddChild(button);

	w->GetBounds(bounds);
	bounds.y += 43;
	bounds.height -= 43 + kScrollbarWidth;

	MView* v = new MViewScroller('scrl', new CTestView(bounds), false, true);
	v->SetBindings(true, true, true, true);
	w->AddChild(v);

	////bounds.width = 16;
	////bounds.height -= kScrollbarWidth;
	////MScrollbar* scrollbar = new MScrollbar('vscl', bounds);
	////scrollbar->SetBindings(false, true, true, true);
	////w->AddChild(scrollbar);

	//w->AddChild(new CTestView(MRect(10, 40, 180, 180)));


	w->Select();

	//MDocument* doc = MDocument::Create<MTextDocument>(MFile());
	//DisplayDocument(doc);
}

void MApplication::SetCurrentFolder(
	const char*	inFolder)
{
	if (inFolder != nil)
		mCurrentFolder = inFolder;
}

void MApplication::DoOpen()
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

MDocument* MApplication::OpenOneDocument(
	const MFile&			inFileRef)
{
	if (inFileRef.IsLocal() and fs::is_directory(inFileRef.GetPath()))
		THROW(("Cannot open a directory"));
	
	MDocument* doc = MDocument::GetDocumentForFile(inFileRef);
	
	if (doc == nil)
	{
	//	if (FileNameMatches("*.prj", inFileRef))
	//	{
	//		if (not inFileRef.IsLocal())
	//			THROW(("Can only open local project files for now, sorry"));
	//		OpenProject(inFileRef);
	//	}
	//	else if (FileNameMatches("*.epub", inFileRef))
	//		OpenEPub(inFileRef);
	//	else
			//doc = MDocument::Create<MTextDocument>(inFileRef);
	}
	
	if (doc != nil)
	{
		DisplayDocument(doc);
		//MMenu::AddToRecentMenu(inFileRef);
	}
	
	return doc;
}

void MApplication::Pulse()
{
	if (not mInitialized)
	{
		DoNew();
		mInitialized = true;
	}
	else
	{
		MWindow::RecycleWindows();
		//
		//if (mSocketFD >= 0)
		//	ProcessSocketMessages();

		if (gQuit or
			(mInitialized and
			 MWindow::GetFirstWindow() == nil))
		{
			DoQuit();
			gQuit = false;	// in case user cancelled the quit
		}
		else
			eIdle(GetLocalTime());
	}
}

int MApplication::Main(
	const vector<string>&	inArgs)
{
	po::options_description desc("Known options");
	desc.add_options()
	    ("help", "Produce help message")
	    ("verbose", "Produce verbose output")
		("file", po::value<vector<string> >(), "The file to edit")
	;
	
	po::positional_options_description p;
	p.add("file", -1);

	po::variables_map vm;
	po::store(po::command_line_parser(inArgs).options(desc).positional(p).run(), vm);
	po::notify(vm);

	int result = 0;
	
	if (vm.count("help"))
	{
		cout << desc;
		result = 1;
	}
	
	gApp = new MApplication();

	result = gApp->RunEventLoop();

	//foreach (const string& file, vm["file"].as<vector<string> >())
	//{
	//	cout << file << endl;
	//}

	return result;
}

/*
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
			gPrefixPath = gExecutablePath.parent_path();
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
			
			unique_ptr<MProject> project(MDocument::Create<MProject>(file));
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
		MApplication::MFilesToOpenList filesToOpen;
		
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
		
		MApplication app;
		
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
*/
