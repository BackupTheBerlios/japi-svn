//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

#include <fcntl.h>
#include <cerrno>
#include <signal.h>

#include <boost/filesystem/fstream.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

#include "MFile.h"
#include "MJapiApp.h"
#include "MTextDocument.h"
#include "MEditWindow.h"
#include "MPreferences.h"
#include "MUtils.h"
#include "MAcceleratorTable.h"
#include "MDocClosedNotifier.h"
#include "MFindAndOpenDialog.h"
#include "MFindDialog.h"
//#include "MProject.h"
#include "MPrefsDialog.h"
#include "MStrings.h"
#include "MAlerts.h"
//#include "MDiffWindow.h"
#include "MResources.h"
//#include "MProjectWindow.h"
#include "MPrinter.h"
#include "MSound.h"
//#include "MePubDocument.h"
//#include "MePubWindow.h"
//#include "MShell.h"
//#include "MTestWindow.h"
#include "MLanguage.h"
#include "MDevice.h"

#include "MControls.h"

#include <iostream>

using namespace std;
namespace ba = boost::algorithm;

const char
	kAppName[] = "Japi",
	kVersionString[] = "0.9.8";

const char
	kSocketName[] = "/tmp/japi.%d.socket";

namespace {
	
const uint32
	cmd_ImportOEB =			'ImpB';
}

// --------------------------------------------------------------------

const char kHexChars[] = "0123456789abcdef";

const MColor
	kTextColor,
	kKeyWordColor("#3d4c9e"),
	kPreProcessorColor("#005454"),
	kCharConstColor("#ad6739"),
	kCommentColor("#9b2e35"),
	kStringColor("#666666"),
	kTagColor("#008484"),
	kAttribColor("#1e843b"),
	kInvisiblesColor("#aaaaaa"),
	kHiliteColor("#ffd281"),
	//kCurrentLineColor("#ffffcc"),
	kCurrentLineColor("#eeeeee"),
	kMarkedLineColor("#efff7f"),
	kPCLineColor = MColor("#cce5ff"),
	kBreakpointColor = MColor("#5ea50c"),
	kWhiteSpaceColor = MColor("#cf4c42");

const MColor
	kInactiveHighlightColor("#e5e5e5"),
	kOddRowBackColor("#eff7ff");

bool			gAutoIndent = true;
bool			gSmartIndent = true;
bool			gKiss = true;
bool			gSmoothFonts = false;
bool			gShowInvisibles = true;
bool			gTabEntersSpaces = false;
bool			gPlaySounds = true;
uint32			gCharsPerTab = 4;
uint32			gSpacesPerTab = 4;
MColor			gLanguageColors[kLStyleCount];
MColor			gHiliteColor, gInactiveHiliteColor;
MColor			gCurrentLineColor, gMarkedLineColor;
MColor			gPCLineColor, gBreakpointColor;
MColor			gWhiteSpaceColor;

uint32			gConcurrentJobs = 2;

fs::path		gTemplatesDir, gScriptsDir, gPrefsDir;

// --------------------------------------------------------------------

MJapiApp::MJapiApp()
	: mSocketFD(-1)
{
	MAcceleratorTable& at = MAcceleratorTable::Instance();

	at.RegisterAcceleratorKey(cmd_New, 'N', kControlKey);
	at.RegisterAcceleratorKey(cmd_Open, 'O', kControlKey);
	at.RegisterAcceleratorKey(cmd_OpenIncludeFile, 'D', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_SwitchHeaderSource, '1', kControlKey);
	at.RegisterAcceleratorKey(cmd_Close, 'W', kControlKey);
	at.RegisterAcceleratorKey(cmd_CloseAll, 'W', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_Save, 'S', kControlKey);
	at.RegisterAcceleratorKey(cmd_SaveAll, 'S', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_Quit, 'Q', kControlKey);
	at.RegisterAcceleratorKey(cmd_Undo, 'Z', kControlKey);
	at.RegisterAcceleratorKey(cmd_Redo, 'Z', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_Cut, 'X', kControlKey);
	at.RegisterAcceleratorKey(cmd_CutAppend, 'X', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_Copy, 'C', kControlKey);
	at.RegisterAcceleratorKey(cmd_CopyAppend, 'C', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_Paste, 'V', kControlKey);
	at.RegisterAcceleratorKey(cmd_PasteNext, 'V', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_SelectAll, 'A', kControlKey);
	at.RegisterAcceleratorKey(cmd_Balance, 'B', kControlKey);
	at.RegisterAcceleratorKey(cmd_ShiftLeft, '[', kControlKey);
	at.RegisterAcceleratorKey(cmd_ShiftRight, ']', kControlKey);
	at.RegisterAcceleratorKey(cmd_Comment, '\'', kControlKey);
	at.RegisterAcceleratorKey(cmd_Uncomment, '\'', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_FastFind, 'I', kControlKey);
	at.RegisterAcceleratorKey(cmd_FastFindBW, 'I', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_Find, 'F', kControlKey);
	at.RegisterAcceleratorKey(cmd_FindNext, 'G', kControlKey);
	at.RegisterAcceleratorKey(cmd_FindPrev, 'G', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_FindInNextFile, 'J', kControlKey);
	at.RegisterAcceleratorKey(cmd_EnterSearchString, 'E', kControlKey);
	at.RegisterAcceleratorKey(cmd_EnterReplaceString, 'E', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_Replace, '=', kControlKey);
	at.RegisterAcceleratorKey(cmd_ReplaceAll, '=', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_ReplaceFindNext, 'T', kControlKey);
	at.RegisterAcceleratorKey(cmd_ReplaceFindPrev, 'T', kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_CompleteLookingBack, kTabKeyCode, kControlKey);
	at.RegisterAcceleratorKey(cmd_CompleteLookingFwd, kTabKeyCode, kControlKey | kShiftKey);
	at.RegisterAcceleratorKey(cmd_GoToLine, ',', kControlKey);
	at.RegisterAcceleratorKey(cmd_JumpToNextMark, kF2KeyCode, 0);
	at.RegisterAcceleratorKey(cmd_JumpToPrevMark, kF2KeyCode, kShiftKey);
	at.RegisterAcceleratorKey(cmd_MarkLine, kF1KeyCode, 0);
	
	at.RegisterAcceleratorKey(cmd_BringUpToDate, 'U', kControlKey);
	at.RegisterAcceleratorKey(cmd_Compile, 'K', kControlKey);
	at.RegisterAcceleratorKey(cmd_CheckSyntax, ';', kControlKey);
	at.RegisterAcceleratorKey(cmd_Make, 'M', kControlKey | kShiftKey);
	
	at.RegisterAcceleratorKey(cmd_Worksheet, '0', kControlKey);

	at.RegisterAcceleratorKey(cmd_Stop, '.', kControlKey);
			//
//	at.RegisterAcceleratorKey(cmd_Menu, GDK_Menu, 0);
//	at.RegisterAcceleratorKey(cmd_Menu, GDK_F10, kShiftKey);
}

MJapiApp::~MJapiApp()
{
	//if (mSocketFD >= 0)
	//	close(mSocketFD);
}

bool MJapiApp::ProcessCommand(
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
		case cmd_NewProject:
			DoNewProject();
			break;
		
		case cmd_NewEPub:
			DoNewEPub();
			break;
		
		case cmd_ImportOEB:
			ImportOEB();
			break;
		
		case cmd_OpenTemplate:
			if (inModifiers & kControlKey)
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
		
		//case cmd_OpenIncludeFile:
		//	new MFindAndOpenDialog(MProject::Instance(), MWindow::GetFirstWindow());
		//	break;
		//
		//case cmd_Worksheet:
		//	ShowWorksheet();
		//	break;

//		case cmd_ShowDiffWindow:
//		{
//			unique_ptr<MDiffWindow> w(new MDiffWindow);
//			w->Initialize();
//			w->Show();
//			w.release();
//			break;
//		}

//		case 'DgTs':
//		{
//			unique_ptr<MDebuggerWindow> w(new MDebuggerWindow);
//			w->Initialize();
//			w->Show();
//			w.release();
//			break;
//		}

		//case cmd_SelectWindowFromMenu:
		//	DoSelectWindowFromWindowMenu(inItemIndex - 2);
		//	break;
		
		case 'test':
		{
//			MWindow* w = new MTestWindow();
//			w->Show();
			break;
		}
		
		//case cmd_ShowDiffWindow:
		//	new MDiffWindow;
		//	break;
		
		default:
			result = MApplication::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

bool MJapiApp::UpdateCommandStatus(
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
		case cmd_OpenTemplate:
		case cmd_Find:
		case cmd_FindInNextFile:
		case cmd_OpenIncludeFile:
		case cmd_Worksheet:
			outEnabled = true;
			break;
		
		default:
			result = MApplication::UpdateCommandStatus(inCommand, inMenu, inItemIndex, outEnabled, outChecked);
			break;
	}
	
	return result;
}

void MJapiApp::UpdateSpecialMenu(
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
	else if (inName == "recent")
		UpdateRecentMenu(inMenu);
	else
		MApplication::UpdateSpecialMenu(inName, inMenu);
}

void MJapiApp::UpdateWindowMenu(
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

void MJapiApp::UpdateTemplateMenu(
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
			inMenu->AppendItem(file.filename(), cmd_OpenTemplate);
	}
}

void MJapiApp::UpdateScriptsMenu(
	MMenu*				inMenu)
{
	inMenu->RemoveItems(0, inMenu->CountItems());

	if (fs::exists(gScriptsDir) and fs::is_directory(gScriptsDir))
	{
		MFileIterator iter(gScriptsDir, 0);
		
		fs::path file;
		while (iter.Next(file))
			inMenu->AppendItem(file.filename(), cmd_ApplyScript);
	}
}

void MJapiApp::UpdateEPubMenu(
	MMenu*				inMenu)
{
	//inMenu->RemoveItems(0, inMenu->CountItems());

	//MePubDocument* epub = MePubDocument::GetFirstEPubDocument();
	//while (epub != nil)
	//{
	//	inMenu->AppendItem(epub->GetFile().GetFileName(), cmd_SaveInEPub);
	//	epub = epub->GetNextEPubDocument();
	//}
}

void MJapiApp::DoSaveAll()
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

void MJapiApp::DoCloseAll(
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
			
//			assert(controller != nil);
			
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

void MJapiApp::DoQuit()
{
	//if (MProject::Instance() != nil)
	//{
	//	string p = MProject::Instance()->GetPath().string();
	//	Preferences::SetString("last project", p);
	//}

	MApplication::DoQuit();
	
	if (MDocument::GetFirstDocument() == nil/* and MProject::Instance() == nil*/)
	{
		MFindDialog::Instance().Close();
//		gtk_main_quit();
	}
}

MWindow* MJapiApp::DisplayDocument(
	MDocument*		inDocument)
{
	MWindow* result = MDocWindow::FindWindowForDocument(inDocument);
	
	if (result == nil)
	{
		if (dynamic_cast<MTextDocument*>(inDocument) != nil)
		{
			MEditWindow* e = new MEditWindow;
			e->SetDocument(inDocument);
			e->Show();

			result = e;
		}
	}
	
	if (result != nil)
		result->Select();
	
	return result;
}

void MJapiApp::DoNew()
{
	MDocument* doc = MDocument::Create<MTextDocument>(this, MFile());
	DisplayDocument(doc);
}

void MJapiApp::DoNewProject()
{
	//GtkWidget *dialog;
	//
	//dialog = gtk_file_chooser_dialog_new(_("Save File"),
	//				      nil,
	//				      GTK_FILE_CHOOSER_ACTION_SAVE,
	//				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	//				      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
	//				      NULL);

	//gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), true);
	//
	//gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), _("NewProject"));
	//gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dialog), true);

	//if (mCurrentFolder.length() > 0)
	//{
	//	gtk_file_chooser_set_current_folder_uri(
	//		GTK_FILE_CHOOSER(dialog), mCurrentFolder.c_str());
	//}
	//
	//if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	//{
	//	char* uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
	//	
	//	THROW_IF_NIL((uri));
	//	
	//	MFile url(uri, true);
	//	g_free(uri);
	//	
	//	if (not url.IsLocal())
	//		THROW(("Projects can only be created on local file systems, sorry"));

	//	mCurrentFolder = 
	//		gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(dialog));
	//	
	//	// get the path
	//	fs::path p = url.GetPath();

	//	// create a directory to store our project and the source files in it.		
	//	fs::create_directories(p);
	//	fs::create_directories(p / "src");
	//	
	//	// create the project path
	//	fs::path projectFile = p / (p.filename() + ".prj");
	//	
	//	// and file
	//	fs::ofstream file(projectFile);
	//	if (not file.is_open())
	//		THROW(("Failed to create project file"));
	//	
	//	// write out a default project from our resources
	//	mrsrc::rsrc rsrc("Templates/Projects/hello-cmdline.prj");

	//	if (not rsrc)
	//		THROW(("Failed to load project resource"));
	//	
	//	file.write(rsrc.data(), rsrc.size());
	//	file.close();
	//	
	//	// and write out a sample source file
	//	fs::ofstream srcfile(p / "src" / "HelloWorld.cpp");
	//	if (not srcfile.is_open())
	//		THROW(("Failed to create project file"));
	//	
	//	rsrc = mrsrc::rsrc("Templates/Projects/hello.cpp");
	//	if (not rsrc)
	//		THROW(("Failed to load project sourcefile resource"));
	//	
	//	srcfile.write(rsrc.data(), rsrc.size());
	//	srcfile.close();
	//	
	//	OpenProject(MFile(projectFile));
	//}
	//
	//gtk_widget_destroy(dialog);
}

void MJapiApp::DoNewEPub()
{
	//unique_ptr<MePubDocument> epub(new MePubDocument());
	//epub->InitializeNew();
	//
	//unique_ptr<MePubWindow> w(new MePubWindow());
	//w->Initialize(epub.get());
	//epub->SetModified(false);
	//epub.release();
	//w->Show();
	//w.release();
}

void MJapiApp::ImportOEB()
{
	//MFile oeb;
	//if (ChooseOneFile(oeb))
	//{
	//	unique_ptr<MePubDocument> epub(new MePubDocument());
	//	epub->ImportOEB(oeb);
	//	
	//	unique_ptr<MePubWindow> w(new MePubWindow());
	//	w->Initialize(epub.get());
	//	epub->SetModified(false);
	//	epub.release();
	//	w->Show();
	//	w.release();
	//}
}

void MJapiApp::SetCurrentFolder(
	const char*	inFolder)
{
	if (inFolder != nil)
		mCurrentFolder = inFolder;
}

// ---------------------------------------------------------------------------
//	OpenOneDocument

MDocument* MJapiApp::OpenOneDocument(
	const MFile&			inFileRef)
{
	if (inFileRef.IsLocal() and fs::is_directory(inFileRef.GetPath()))
		THROW(("Cannot open a directory"));
	
	MDocument* doc = MDocument::GetDocumentForFile(inFileRef);
	
	if (doc == nil)
	{
		//if (FileNameMatches("*.prj", inFileRef))
		//{
		//	if (not inFileRef.IsLocal())
		//		THROW(("Can only open local project files for now, sorry"));
		//	OpenProject(inFileRef);
		//}
		//else if (FileNameMatches("*.epub", inFileRef))
		//	OpenEPub(inFileRef);
		//else
			doc = MDocument::Create<MTextDocument>(this, inFileRef);
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

void MJapiApp::OpenProject(
	const MFile&		inPath)
{
	//unique_ptr<MDocument> project(MDocument::Create<MProject>(inPath));
	//unique_ptr<MProjectWindow> w(new MProjectWindow());
	//w->Initialize(project.get());
	//project->SetModified(false);
	//project.release();
	//w->Show();
	//w.release();

	//AddToRecentMenu(inPath);
}

// ---------------------------------------------------------------------------
//	OpenEPub

void MJapiApp::OpenEPub(
	const MFile&		inPath)
{
	//try
	//{
	//	unique_ptr<MDocument> epub(MDocument::Create<MePubDocument>(inPath));
	//	unique_ptr<MePubWindow> w(new MePubWindow());
	//	
	//	w->Initialize(epub.get());
	//	epub->SetModified(false);
	//	epub.release();

	//	w->Show();
	//	w.release();
	//
	//	AddToRecentMenu(inPath);
	//}
	//catch (exception& e)
	//{
	//	DisplayError(e);
	//}
}

void MJapiApp::DoOpenTemplate(
	const string&		inTemplate)
{
	//fs::ifstream file(gTemplatesDir / inTemplate, ios::binary);
	//MTextBuffer buffer;
	//buffer.ReadFromFile(file);

	//string text;
	//buffer.GetText(0, buffer.GetSize(), text);
	//
	//ba::replace_all(text, "$date$", GetDateTime());
	//ba::replace_all(text, "$name$", GetUserName(false));
	//ba::replace_all(text, "$shortname$", GetUserName(true));
	//
	//MTextDocument* doc = MDocument::Create<MTextDocument>(MFile());
	//doc->SetText(text.c_str(), text.length());
	//doc->SetFileNameHint(inTemplate);
	//DisplayDocument(doc);
}

void MJapiApp::ShowWorksheet()
{
	//fs::path worksheet = gPrefsDir / "Worksheet";
	//
	//if (not fs::exists(worksheet))
	//{
	//	fs::ofstream file(worksheet);
	//	string default_text = _(
	//		"This is a worksheet, you can type shell commands here\n"
	//		"and execute them by pressing CNTRL-Return or Enter on\n"
	//		"the numeric keypad.\n"
	//		"This worksheet will be saved automatically when closed.\n");
	//	
	//	file.write(default_text.c_str(), default_text.length());
	//}
	//	
	//MDocument* doc = OpenOneDocument(MFile(worksheet));
	//if (doc != nil and dynamic_cast<MTextDocument*>(doc) != nil)
	//	MTextDocument::SetWorksheet(static_cast<MTextDocument*>(doc));
}

//// ----------------------------------------------------------------------------
////	Main routines, forking a client/server e.g.
//
//void my_signal_handler(int inSignal)
//{
//	switch (inSignal)
//	{
//		case SIGPIPE:
//			break;
//		
//		case SIGUSR1:
//			break;
//		
//		case SIGINT:
//			gQuit = true;
//			break;
//		
//		case SIGTERM:
//			gQuit = true;
//			break;
//	}
//}
//
//void error(const char* msg, ...)
//{
//	fprintf(stderr, "%s stopped with an error:\n", g_get_application_name());
//	va_list vl;
//	va_start(vl, msg);
//	vfprintf(stderr, msg, vl);
//	va_end(vl);
//	if (errno != 0)
//		fprintf(stderr, "\n%s\n", strerror(errno));
//	exit(1);
//}
//
//struct MSockMsg
//{
//	uint32		msg;
//	int32		length;
//};
//
//void MJapiApp::ProcessSocketMessages()
//{
//	int fd = accept(mSocketFD, nil, nil);
//	
//	if (fd >= 0)
//	{
//		MDocClosedNotifier notify(fd);		// takes care of closing fd
//		
//		bool readStdin = false;
//		
//		for (;;)
//		{
//			MSockMsg msg = {};
//			int r = read(fd, &msg, sizeof(msg));
//			
//			if (r == 0 or msg.msg == 'done' or msg.length > PATH_MAX)		// done
//				break;
//			
//			char buffer[PATH_MAX + 1];
//			if (msg.length > 0)
//			{
//				r = read(fd, buffer, msg.length);
//				if (r != msg.length)
//					break;
//				buffer[r] = 0;
//			}
//			
//			try
//			{
//				MDocument* doc = nil;
//				int32 lineNr = -1;
//
//				switch (msg.msg)
//				{
//					case 'open':
//						memcpy(&lineNr, buffer, sizeof(lineNr));
//						doc = gApp->OpenOneDocument(MFile(buffer + sizeof(lineNr)));
//						break;
//					
//					case 'new ':
//						doc = MDocument::Create<MTextDocument>(MFile());
//						break;
//					
//					case 'data':
//						readStdin = true;
//						doc = MDocument::Create<MTextDocument>(MFile());
//						break;
//				}
//				
//				if (doc != nil)
//				{
//					mInitialized = true;
//					
//					DisplayDocument(doc);
//					doc->AddNotifier(notify, readStdin);
//					
//					if (lineNr > 0 and dynamic_cast<MTextDocument*>(doc) != nil)
//						static_cast<MTextDocument*>(doc)->GoToLine(lineNr - 1);
//				}
//			}
//			catch (exception& e)
//			{
//				DisplayError(e);
//				readStdin = false;
//			}
//		}
//	}
//}
//
//namespace {
//
//int OpenSocket(
//	struct sockaddr_un&		addr)
//{
//	int sockfd = -1;
//	
//	if (fs::exists(fs::path(addr.sun_path)))
//	{
//		sockfd = socket(AF_LOCAL, SOCK_STREAM, 0);
//		if (sockfd < 0)
//			cerr << "creating socket failed: " << strerror(errno) << endl;
//		else
//		{
//			int err = connect(sockfd, (const sockaddr*)&addr, sizeof(addr));
//			if (err < 0)
//			{
//				close(sockfd);
//				sockfd = -1;
//
//				unlink(addr.sun_path);	// allowed to fail
//			}
//		}
//	}
//	
//	return sockfd;
//}
//	
//}

//bool MJapiApp::IsServer()
//{
//	bool isServer = false;
//
//	struct sockaddr_un addr = {};
//	addr.sun_family = AF_LOCAL;
//	snprintf(addr.sun_path, sizeof(addr.sun_path), kSocketName, getuid());
//	
//	mSocketFD = OpenSocket(addr);
//
//	if (mSocketFD == -1)
//	{
//		isServer = true;
//
//		// So we're supposed to become a server
//		// We will fork here and open a new socket
//		// We use a pipe to let the client wait for the server
//
//		int fd[2];
//		(void)pipe(fd);
//		
//		int pid = fork();
//		
//		if (pid == -1)
//		{
//			close(fd[0]);
//			close(fd[1]);
//			
//			cerr << _("Fork failed: ") << strerror(errno) << endl;
//			
//			return false;
//		}
//		
//		if (pid == 0)	// forked process (child which becomes the server)
//		{
//			// detach from the process group, create new
//			// to avoid being killed by a CNTRL-C in the shell
//			setpgid(0, 0);
//
//			// now setup the socket
//			mSocketFD = socket(AF_LOCAL, SOCK_STREAM, 0);
//			int err = ::bind(mSocketFD, (const sockaddr*)&addr, SUN_LEN(&addr));
//		
//			if (err < 0)
//				cerr << _("bind failed: ") << strerror(errno) << endl;
//			else
//			{
//				err = listen(mSocketFD, 5);
//				if (err < 0)
//					cerr << _("Failed to listen to socket: ") << strerror(errno) << endl;
//				else
//				{
//					int flags = fcntl(mSocketFD, F_GETFL, 0);
//					if (fcntl(mSocketFD, F_SETFL, flags | O_NONBLOCK))
//						cerr << _("Failed to set mSocketFD non blocking: ") << strerror(errno) << endl;
//				}
//			}
//			
//			write(fd[1], " ", 1);
//			close(fd[0]);
//			close(fd[1]);
//		}
//		else
//		{
//			// client (parent process). Wait until the server has finished setting up the socket.
//			isServer = false;
//			
//			char c;
//			(void)read(fd[0], &c, 1);
//
//			close(fd[1]);
//			close(fd[0]);
//			
//			// the socket should now really exist
//			mSocketFD = OpenSocket(addr);
//		}
//	}
//	
//	return isServer;
//}
//
//bool MJapiApp::IsClient()
//{
//	return mSocketFD >= 0;
//}
//
//void MJapiApp::ProcessArgv(
//	bool				inReadStdin,
//	mRecentFilesToOpenList&	inDocs)
//{
//	MSockMsg msg = { };
//
//	if (inReadStdin)
//	{
//		msg.msg = 'data';
//		(void)write(mSocketFD, &msg, sizeof(msg));
//	}
//
//	if (inDocs.size() > 0)
//	{
//		msg.msg = 'open';
//		for (MJapiApp::mRecentFilesToOpenList::const_iterator d = inDocs.begin(); d != inDocs.end(); ++d)
//		{
//			int32 lineNr = d->first;
//			string url = d->second.GetURI();
//			
//			msg.length = url.length() + sizeof(lineNr);
//			(void)write(mSocketFD, &msg, sizeof(msg));
//			(void)write(mSocketFD, &lineNr, sizeof(lineNr));
//			(void)write(mSocketFD, url.c_str(), url.length());
//		}
//	}
//	
//	if (not inReadStdin and inDocs.size() == 0)
//	{
//		msg.msg = 'new ';
//		(void)write(mSocketFD, &msg, sizeof(msg));
//	}
//	
//	msg.msg = 'done';
//	msg.length = 0;
//	(void)write(mSocketFD, &msg, sizeof(msg));
//
//	if (inReadStdin)
//	{
//		int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
//		if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK))
//			cerr << _("Failed to set fd non blocking: ") << strerror(errno) << endl;
//
//		for (;;)
//		{
//			char buffer[10240];
//			int r = read(STDIN_FILENO, buffer, sizeof(buffer));
//			
//			if (r == 0 or (r < 0 and errno != EAGAIN))
//				break;
//			
//			if (r > 0)
//				r = write(mSocketFD, buffer, r);
//		}
//	}
//
//	// now block until all windows are closed or server dies
//	char c;
//	read(mSocketFD, &c, 1);
//}
//
//void usage()
//{
//	cout << "usage: japi [options] [ [+line] files ]" << endl
//		 << "    available options: " << endl
//		 << endl
//		 << "    -i         Install japi at prefix where prefix path" << endl
//		 << "    -p prefix  Prefix path where to install japi, default is /usr/local" << endl
//		 << "               resulting in /usr/local/bin/japi" << endl
//		 << "    -h         This help message" << endl
//		 << "    -f         Don't fork into client/server mode" << endl
//		 << endl
//		 << "  One or more files may be specified, use - for reading from stdin" << endl
//		 << endl;
//	
//	exit(1);
//}
//
//void InstallJapi(
//	std::string		inPrefix)
//{
//	if (getuid() != 0)
//		error("You must be root to be able to install japi");
//	
//	// copy the executable to the appropriate destination
//	if (inPrefix.empty())
//		inPrefix = "/usr/local";
//	
//	fs::path prefix(inPrefix);
//	
//	if (not fs::exists(prefix / "bin"))
//	{
//		cout << "Creating directory " << (prefix / "bin") << endl;
//		fs::create_directories(prefix / "bin");
//	}
//
//	if (not fs::exists(gExecutablePath))
//		error("I don't seem to exist...[%s]?", gExecutablePath.string().c_str());
//
//	fs::path dest = prefix / "bin" / "japi";
//	cout << "copying " << gExecutablePath.string() << " to " << dest.string() << endl;
//	
//	if (fs::exists(dest))
//		fs::remove(dest);
//		
//	fs::copy_file(gExecutablePath, dest);
//	
//	// create desktop file
//		
//	mrsrc::rsrc rsrc("japi.desktop");
//	if (not rsrc)
//		error("japi.desktop file could not be created, missing data");
//
//	string desktop(rsrc.data(), rsrc.size());
//	ba::replace_first(desktop, "__EXE__", dest.string());
//	
//	// locate applications directory
//	// don't use glib here, 
//	
//	fs::path desktopFile, applicationsDir;
//	
//	const char* const* config_dirs = g_get_system_data_dirs();
//	for (const char* const* dir = config_dirs; *dir != nil; ++dir)
//	{
//		applicationsDir = fs::path(*dir) / "applications";
//		if (fs::exists(applicationsDir) and fs::is_directory(applicationsDir))
//			break;
//	}
//
//	if (not fs::exists(applicationsDir))
//	{
//		cout << "Creating directory " << applicationsDir << endl;
//		fs::create_directories(applicationsDir);
//	}
//
//	desktopFile = applicationsDir / "japi.desktop";
//	cout << "writing desktop file " << desktopFile << endl;
//
//	fs::ofstream df(desktopFile, ios::trunc);
//	df << desktop;
//	df.close();
//	
//	// write out all locale files
//	
//	mrsrc::rsrc_list loc_rsrc = mrsrc::rsrc("Locale").children();
//	for (mrsrc::rsrc_list::iterator l = loc_rsrc.begin(); l != loc_rsrc.end(); ++l)
//	{
//		mrsrc::rsrc_list loc_files = l->children();
//		if (loc_files.empty())
//			continue;
//		
//		rsrc = loc_files.front();
//		if (not rsrc or rsrc.name() != "japi.po")
//			continue;
//		
//		fs::path localeDir =
//			applicationsDir.parent_path() / "japi" / "locale" / l->name() / "LC_MESSAGES";
//		
//		if (not fs::exists(localeDir))
//		{
//			cout << "Creating directory " << localeDir << endl;
//			fs::create_directories(localeDir);
//		}
//		
//		stringstream cmd;
//		cmd << "msgfmt -o " << (localeDir / "japi.mo") << " -";
//		cout << "Installing locale file: `" << cmd.str() << '`' << endl;
//		
//		FILE* f = popen(cmd.str().c_str(), "w");
//		if (f != nil)
//		{
//			fwrite(rsrc.data(), rsrc.size(), 1, f);
//			pclose(f);
//		}
//		else
//			cout << "failed: " << strerror(errno) << endl;
//	}
//	
//	exit(0);
//}
//
//int main(int argc, char* argv[])
//{
//	try
//	{
//		fs::path::default_name_check(fs::no_check);
//
//		// First find out who we are. Uses proc filesystem to find out.
//		char exePath[PATH_MAX + 1];
//		
//		int r = readlink("/proc/self/exe", exePath, PATH_MAX);
//		if (r > 0)
//		{
//			exePath[r] = 0;
//			gExecutablePath = fs::system_complete(exePath);
//			gPrefixPath = gExecutablePath.parent_path();
//		}
//		
//		if (not fs::exists(gExecutablePath))
//			gExecutablePath = fs::system_complete(argv[0]);
//	
//		// Collect the options
//		int c;
//		bool fork = true, readStdin = false, install = false;
//		string target, prefix;
//
//		while ((c = getopt(argc, const_cast<char**>(argv), "h?fip:m:vt")) != -1)
//		{
//			switch (c)
//			{
//				case 'f':
//					fork = false;
//					break;
//				
//				case 'i':
//					install = true;
//					break;
//				
//				case 'p':
//					prefix = optarg;
//					break;
//				
//				case 'm':
//					target = optarg;
//					break;
//#if DEBUG
//				case 'v':
//					++VERBOSE;
//					break;
//#endif
//				default:
//					usage();
//					break;
//			}
//		}
//		
//		if (install)
//		{
//			gtk_init(&argc, &argv);
//			InstallJapi(prefix);
//		}
//		
//		// if the option was to build a target, try it and exit.
//		if (not target.empty())
//		{
//			if (optind >= argc)
//				THROW(("You should specify a project file to use for building"));
//			
//			MFile file(fs::system_complete(argv[optind]));
//			
//			unique_ptr<MProject> project(MDocument::Create<MProject>(file));
//			project->SelectTarget(target);
//			if (project->Make(false))
//				cout << "Build successful, " << target << " is up-to-date" << endl;
//			else
//				cout << "Building " << target << " Failed" << endl;
//			exit(0);
//		}
//
//		// setup locale, if we can find it.
//		fs::path localeDir = gPrefixPath / "share" / "japi" / "locale";
//		if (not fs::exists(localeDir))
//			localeDir = fs::path("/usr/local/share/japi/locale");
//		if (not fs::exists(localeDir))
//			localeDir = fs::path("/usr/share/japi/locale");
//		if (fs::exists(localeDir))
//		{
//			setlocale(LC_CTYPE, "");
//			setlocale(LC_MESSAGES, "");
//			bindtextdomain("japi", localeDir.string().c_str());
//			textdomain("japi");
//		}
//		
//		// see if we need to open any files from the commandline
//		int32 lineNr = -1;
//		MJapiApp::mRecentFilesToOpenList filesToOpen;
//		
//		for (int32 i = optind; i < argc; ++i)
//		{
//			string a(argv[i]);
//			
//			if (a == "-")
//				readStdin = true;
//			else if (a.substr(0, 1) == "+")
//				lineNr = atoi(a.c_str() + 1);
//			else
//			{
//				filesToOpen.push_back(make_pair(lineNr, MFile(a)));
//				lineNr = -1;
//			}
//		}
//		
//		MJapiApp app;
//		
//		if (fork == false or app.IsServer())
//		{
//			g_thread_init(nil);
//			gdk_threads_init();
//			gtk_init(&argc, &argv);
//	
//			InitGlobals();
//			
//			// now start up the normal executable		
//			gtk_window_set_default_icon_name ("accessories-text-editor");
//	
//			struct sigaction act, oact;
//			act.sa_handler = my_signal_handler;
//			sigemptyset(&act.sa_mask);
//			act.sa_flags = 0;
//			::sigaction(SIGTERM, &act, &oact);
//			::sigaction(SIGUSR1, &act, &oact);
//			::sigaction(SIGPIPE, &act, &oact);
//			::sigaction(SIGINT, &act, &oact);
//	
//			gdk_notify_startup_complete();
//
//			app.RunEventLoop();
//		
//			// we're done, clean up
//			MFindDialog::Instance().Close();
//			
//			SaveGlobals();
//	
//			if (fork)
//			{
//				char path[1024] = {};
//				snprintf(path, sizeof(path), kSocketName, getuid());
//				unlink(path);
//			}
//		}
//		else if (app.IsClient())
//			app.ProcessArgv(readStdin, filesToOpen);
//	}
//	catch (exception& e)
//	{
//		cerr << e.what() << endl;
//	}
//	catch (...)
//	{
//		cerr << "Exception caught" << endl;
//	}
//	
//	return 0;
//}
//

// --------------------------------------------------------------------

void MJapiApp::InitGlobals()
{
	MApplication::InitGlobals();

	//gPrefsDir = g_get_user_config_dir();
	//gPrefsDir /= "japi";
	
	//const char* templatesDir = g_get_user_special_dir(G_USER_DIRECTORY_TEMPLATES);
	//if (templatesDir != nil)
	//	gTemplatesDir = fs::system_complete(templatesDir) / "japi";
	//else
	//	gTemplatesDir = gPrefsDir / "Templates";

	gScriptsDir = gPrefsDir / "Scripts";
		
	gPlaySounds = Preferences::GetInteger("play sounds", gPlaySounds);
	gAutoIndent = Preferences::GetInteger("auto indent", gAutoIndent);
	gSmartIndent = Preferences::GetInteger("smart indent", gSmartIndent);
	gKiss = Preferences::GetInteger("kiss", gKiss);
	gCharsPerTab = Preferences::GetInteger("chars per tab", gCharsPerTab);
	gSpacesPerTab = Preferences::GetInteger("spaces per tab", gSpacesPerTab);
//	gSmoothFonts = Preferences::GetInteger("smooth fonts", gSmoothFonts);
	gShowInvisibles = Preferences::GetInteger("show invisibles", gShowInvisibles);
	gTabEntersSpaces = Preferences::GetInteger("tab enters spaces", gTabEntersSpaces);
	
	gLanguageColors[kLTextColor] =			Preferences::GetColor("text color", kTextColor);
	gLanguageColors[kLKeyWordColor] =		Preferences::GetColor("keyword color", kKeyWordColor);
	gLanguageColors[kLPreProcessorColor] =	Preferences::GetColor("preprocessor color", kPreProcessorColor);
	gLanguageColors[kLCharConstColor] =		Preferences::GetColor("char const color", kCharConstColor);
	gLanguageColors[kLCommentColor] =		Preferences::GetColor("comment color", kCommentColor);
	gLanguageColors[kLStringColor] =		Preferences::GetColor("string color", kStringColor);
	gLanguageColors[kLTagColor] =			Preferences::GetColor("tag color", kTagColor);
	gLanguageColors[kLAttribColor] =		Preferences::GetColor("attribute color", kAttribColor);
	gLanguageColors[kLInvisiblesColor] =	Preferences::GetColor("invisibles color", kInvisiblesColor);

	gHiliteColor = kHiliteColor;
	MDevice::GetSysSelectionColor(gHiliteColor);
	Preferences::GetColor("hilite color", kHiliteColor);
	gInactiveHiliteColor = Preferences::GetColor("inactive hilite color", kInactiveHighlightColor);
//	gOddRowColor = kOddRowBackColor;
	gCurrentLineColor = Preferences::GetColor("current line color", kCurrentLineColor);
	gMarkedLineColor = Preferences::GetColor("marked line color", kMarkedLineColor);
	gPCLineColor = Preferences::GetColor("pc line color", kPCLineColor);
	gBreakpointColor = Preferences::GetColor("breakpoint color", kBreakpointColor);
	gWhiteSpaceColor = Preferences::GetColor("whitespace color", kWhiteSpaceColor);

	//if (not fs::exists(gTemplatesDir))
	//	fs::create_directory(gTemplatesDir);

	//mrsrc::rsrc_list templates = mrsrc::rsrc("Templates").children();
	//for (mrsrc::rsrc_list::iterator t = templates.begin(); t != templates.end(); ++t)
	//{
	//	if (t->size() == 0 or fs::exists(gTemplatesDir / t->name()))
	//		continue;
	//	
	//	fs::ofstream f(gTemplatesDir / t->name());
	//	f.write(t->data(), t->size());
	//}

	//if (not fs::exists(gScriptsDir))
	//	fs::create_directory(gScriptsDir);

	//mrsrc::rsrc_list scripts = mrsrc::rsrc("Scripts").children();
	//for (mrsrc::rsrc_list::iterator t = scripts.begin(); t != scripts.end(); ++t)
	//{
	//	if (t->size() == 0 or fs::exists(gScriptsDir / t->name()))
	//		continue;
	//	
	//	fs::path file(gScriptsDir / t->name());
	//	
	//	fs::ofstream f(file);
	//	f.write(t->data(), t->size());
	//	
	//	//chmod(file.string().c_str(), S_IRUSR | S_IWUSR | S_IXUSR);
	//}
	
	gConcurrentJobs = Preferences::GetInteger("concurrent-jobs", gConcurrentJobs);
}

void MJapiApp::SaveGlobals()
{
	Preferences::SetInteger("play sounds", gPlaySounds);
	Preferences::SetInteger("auto indent", gAutoIndent);
	Preferences::SetInteger("smart indent", gSmartIndent);
	Preferences::SetInteger("kiss", gKiss);
	Preferences::SetInteger("chars per tab", gCharsPerTab);
	Preferences::SetInteger("spaces per tab", gSpacesPerTab);
//	Preferences::SetInteger("smooth fonts", gSmoothFonts);
	Preferences::SetInteger("show invisibles", gShowInvisibles);
	//Preferences::SetInteger("fontsize", gFontSize);
	//Preferences::SetString("fontname", gFontName);
	Preferences::SetInteger("tab enters spaces", gTabEntersSpaces);

	//for (uint32 ix = 0; ix < kLStyleCount; ++ix)
	//{
	//	gLanguageColors[ix] = Preferences::GetColor("color_" + char('0' + ix), kLanguageColors[ix]);
	//}

	Preferences::SetInteger("concurrent-jobs", gConcurrentJobs);

	MApplication::SaveGlobals();
}

// --------------------------------------------------------------------

int MApplication::Main(
	const vector<string>&	inArgs)
{
	//po::options_description desc("Known options");
	//desc.add_options()
	//    ("help", "Produce help message")
	//    ("verbose", "Produce verbose output")
	//	("file", po::value<vector<string> >(), "The file to edit")
	//;
	//
	//po::positional_options_description p;
	//p.add("file", -1);

	//po::variables_map vm;
	//po::store(po::command_line_parser(inArgs).options(desc).positional(p).run(), vm);
	//po::notify(vm);

	//int result = 0;
	//
	//if (vm.count("help"))
	//{
	//	cout << desc;
	//	result = 1;
	//}
	
	MJapiApp app;
	return app.RunEventLoop();
}

