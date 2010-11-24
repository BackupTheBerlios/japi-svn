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
#include <boost/bind.hpp>

#include "MApplication.h"
#include "MApplicationImpl.h"
#include "MCommands.h"
#include "MDocument.h"
#include "MPreferences.h"
#include "MDocWindow.h"
#include "MUtils.h"
#include "MMenu.h"
#include "MStrings.h"

#include "MControls.h"

using namespace std;
namespace ba = boost::algorithm;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

#if DEBUG
int VERBOSE, TRACE;
#endif

MApplication* gApp;
fs::path gExecutablePath, gPrefixPath;

// --------------------------------------------------------------------

MApplication::MApplication(
	MApplicationImpl*		inImpl)
	: MHandler(nil)
	, mImpl(inImpl)
	, mQuit(false)
	, mQuitPending(false)
{
	// set the global pointing to us
	gApp = this;
}

MApplication::~MApplication()
{
	delete mImpl;
}

void MApplication::InitGlobals()
{
	mRecentFiles.clear();

	vector<string> recent;
	Preferences::GetArray("recent", recent);
	reverse(recent.begin(), recent.end());

	foreach (string path, recent)
	{
		fs::path file(path);
		if (fs::exists(file))
			AddToRecentMenu(file);
	}
}

void MApplication::SaveGlobals()
{
	vector<string> recent;
	transform(mRecentFiles.begin(), mRecentFiles.end(), back_inserter(recent), boost::bind(&fs::path::native_file_string, _1));
	Preferences::SetArray("recent", recent);
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
			DoQuit();
			break;
		
		case cmd_New:
			DoNew();
			break;
		
		case cmd_Open:
			DoOpen();
			break;
		
		case cmd_CloseAll:
			CloseAll(kSaveChangesClosingAllDocuments);
			break;
		
		case cmd_SaveAll:
			DoSaveAll();
			break;
		
		case cmd_SelectWindowFromMenu:
			DoSelectWindowFromWindowMenu(inItemIndex - 2);
			break;
		
		case cmd_OpenRecent:
			if (inMenu != nil and inItemIndex < mRecentFiles.size())
				OpenOneDocument(MFile(mRecentFiles[inItemIndex]));
			break;
		
		case 'test':
		{
//			MWindow* w = new MTestWindow();
//			w->Show();
			break;
		}
		
		default:
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
		case cmd_SelectWindowFromMenu:
		case cmd_OpenRecent:
			outEnabled = true;
			break;
		
		default:
			result = false;
			break;
	}
	
	return result;
}

void MApplication::UpdateSpecialMenu(
	const string&		inName,
	MMenu*				inMenu)
{
	if (inName == "window")
		UpdateWindowMenu(inMenu);
	else if (inName == "recent")
		UpdateRecentMenu(inMenu);
	else
		PRINT(("Unknown special menu %s", inName.c_str()));
}

void MApplication::UpdateWindowMenu(
	MMenu*				inMenu)
{
	inMenu->RemoveItems(2, inMenu->CountItems() - 2);
	
	MDocument* doc = MDocument::GetFirstDocument();
	while (doc != nil)
	{
		string label;

		MDocWindow* w = MDocWindow::FindWindowForDocument(doc);
		if (w != nil)
			label = w->GetTitle();
		else
			label = _("weird");
			
		inMenu->AppendItem(label, cmd_SelectWindowFromMenu);
		
		doc = doc->GetNextDocument();
	}	
}

void MApplication::UpdateRecentMenu(
	MMenu*				inMenu)
{
	inMenu->RemoveItems(0, inMenu->CountItems());

	foreach (fs::path file, mRecentFiles)
	{
		if (fs::exists(file))
			inMenu->AppendItem(file.native_file_string(), cmd_OpenRecent);
	}
}

void MApplication::AddToRecentMenu(
	const fs::path&		inFile)
{
	mRecentFiles.erase(remove(mRecentFiles.begin(), mRecentFiles.end(), inFile), mRecentFiles.end());
	mRecentFiles.push_front(inFile);
	if (mRecentFiles.size() > 10)
		mRecentFiles.pop_back();
}

void MApplication::DoSelectWindowFromWindowMenu(
	uint32				inIndex)
{
	MDocument* doc = MDocument::GetFirstDocument();

	while (doc != nil and inIndex-- > 0)
		doc = doc->GetNextDocument();
	
	if (doc != nil)
		DisplayDocument(doc);	
}	

int MApplication::RunEventLoop()
{
	mImpl->Initialise();

	InitGlobals();

	int result = mImpl->RunEventLoop();

	SaveGlobals();

	return result;
}

void MApplication::DoSaveAll()
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

bool MApplication::CloseAll(
	MCloseReason		inAction)
{
	bool result = true;
	// first close all that can be closed

	MDocument* doc = MDocument::GetFirstDocument();
	
	while (doc != nil)
	{
		MDocument* next = doc->GetNextDocument();
		
		if (not doc->IsModified() and IsCloseAllCandidate(doc))
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

		if (IsCloseAllCandidate(doc) or
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

void MApplication::DoQuit()
{
	if (CloseAll(kSaveChangesQuittingApplication))
		mImpl->Quit();
}

MWindow* MApplication::DisplayDocument(
	MDocument*		inDocument)
{
	return nil;
}

MDocument* MApplication::CreateNewDocument()
{
	assert(false);
	return nil;
}

void MApplication::DoNew()
{
	DisplayDocument(CreateNewDocument());
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
	
	if (MFileDialogs::ChooseFiles(nil, false, urls))
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
		AddToRecentMenu(inFileRef.GetPath());
	}
	
	return doc;
}

void MApplication::Pulse()
{
	// if there are no visible windows left, we quit
	MWindow* front = MWindow::GetFirstWindow();
	while (front != nil and not front->IsVisible())
		front = front->GetNextWindow();
	
	if (front == nil)
		DoQuit();
	else
		eIdle(GetLocalTime());
}

