//          Copyright Maarten L. Hekkelman 2006-2008
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "MJapi.h"

//#if defined(_MSC_VER)
//#pragma comment ( lib, "libzeep" )
//#endif

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
#include "MProject.h"
#include "MPrefsDialog.h"
#include "MStrings.h"
#include "MAlerts.h"
//#include "MDiffWindow.h"
#include "MResources.h"
#include "MProjectWindow.h"
#include "MPrinter.h"
#include "MSound.h"
//#include "MePubDocument.h"
//#include "MePubWindow.h"
//#include "MShell.h"
#include "CTestWindow.h"
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
	kCurrentLineColor("#ffffdd"),
	//kCurrentLineColor("#eeeeee"),
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

fs::path		gTemplatesDir, gScriptsDir;

// --------------------------------------------------------------------

MJapiApp::MJapiApp(
	MApplicationImpl*	inImpl)
	: MApplication(inImpl)
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
}

bool MJapiApp::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;

	MProject* project = MProject::Instance();
	if (project != nil and project->ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers))
		return true;

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
		
		case cmd_OpenIncludeFile:
			new MFindAndOpenDialog(MProject::Instance(), MWindow::GetFirstWindow());
			break;
		
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
			MWindow* w = new CTestWindow();
			w->Select();
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

	MProject* project = MProject::Instance();
	if (project != nil and project->UpdateCommandStatus(inCommand, inMenu, inItemIndex, outEnabled, outChecked))
		return true;

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

bool MJapiApp::CloseAll(
	MCloseReason		inAction)
{
	bool result = true;
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
			if (controller != nil)
			{
				MWindow* w = controller->GetWindow();
				if (controller->TryCloseDocument(inAction))
					w->Close();
			}
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
				{
					result = false;
					break;
				}
			}
		}
		else if (dynamic_cast<MTextDocument*>(doc) != nil or
			inAction == kSaveChangesQuittingApplication)
		{
			if (not controller->TryCloseDocument(inAction))
			{
				result = false;
				break;
			}
		}
		
		doc = next;
	}

	return result;
}

void MJapiApp::DoQuit()
{
	if (MProject::Instance() != nil)
	{
		string p = MProject::Instance()->GetPath().string();
		Preferences::SetString("last project", p);
	}

	MApplication::DoQuit();
	
	if (MDocument::GetFirstDocument() == nil and MProject::Instance() == nil)
	{
		MFindDialog::Instance().Quit();
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

MDocument* MJapiApp::CreateNewDocument()
{
	return MDocument::Create<MTextDocument>(this, MFile());
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
		if (FileNameMatches("*.prj", inFileRef))
		{
			if (not inFileRef.IsLocal())
				THROW(("Can only open local project files for now, sorry"));
			OpenProject(inFileRef);
		}
		//else if (FileNameMatches("*.epub", inFileRef))
		//	OpenEPub(inFileRef);
		else
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
	unique_ptr<MDocument> project(MDocument::Create<MProject>(this, inPath));
	unique_ptr<MProjectWindow> w(new MProjectWindow());
	w->SetDocument(project.get());
	project->SetModified(false);
	project.release();
	w->Show();
	w->Select();
	w.release();
	
	AddToRecentMenu(inPath);
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
	
	gPlaySounds = Preferences::GetBoolean("play sounds", gPlaySounds);
	gAutoIndent = Preferences::GetBoolean("auto indent", gAutoIndent);
	gSmartIndent = Preferences::GetBoolean("smart indent", gSmartIndent);
	gKiss = Preferences::GetBoolean("kiss", gKiss);
	gCharsPerTab = Preferences::GetInteger("chars per tab", gCharsPerTab);
	gSpacesPerTab = Preferences::GetInteger("spaces per tab", gSpacesPerTab);
//	gSmoothFonts = Preferences::GetBoolean("smooth fonts", gSmoothFonts);
	gShowInvisibles = Preferences::GetBoolean("show invisibles", gShowInvisibles);
	gTabEntersSpaces = Preferences::GetBoolean("tab enters spaces", gTabEntersSpaces);
	
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
	
	if (Preferences::GetBoolean("reopen project", true))
	{
		MFile pp(fs::path(Preferences::GetString("last project", "")));
		if (pp.IsLocal() and pp.Exists())
			OpenProject(pp);
	}
}

void MJapiApp::SaveGlobals()
{
	Preferences::SetBoolean("play sounds", gPlaySounds);
	Preferences::SetBoolean("auto indent", gAutoIndent);
	Preferences::SetBoolean("smart indent", gSmartIndent);
	Preferences::SetBoolean("kiss", gKiss);
	Preferences::SetInteger("chars per tab", gCharsPerTab);
	Preferences::SetInteger("spaces per tab", gSpacesPerTab);
//	Preferences::SetInteger("smooth fonts", gSmoothFonts);
	Preferences::SetBoolean("show invisibles", gShowInvisibles);
	//Preferences::SetInteger("fontsize", gFontSize);
	//Preferences::SetString("fontname", gFontName);
	Preferences::SetBoolean("tab enters spaces", gTabEntersSpaces);

	//for (uint32 ix = 0; ix < kLStyleCount; ++ix)
	//{
	//	gLanguageColors[ix] = Preferences::GetColor("color_" + char('0' + ix), kLanguageColors[ix]);
	//}

	Preferences::SetInteger("concurrent-jobs", gConcurrentJobs);

	MApplication::SaveGlobals();
}

MApplication* MApplication::Create(
	MApplicationImpl*		inImpl)
{
	return new MJapiApp(inImpl);
}
