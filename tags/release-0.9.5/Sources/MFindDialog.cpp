/*
	Copyright (c) 2006, Maarten L. Hekkelman
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

/*	$Id: MFindDialog.cpp 170 2007-06-12 20:55:53Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday August 15 2004 20:47:19
*/

#include "MJapi.h"

#include <boost/bind.hpp>

#include "MFindDialog.h"
#include "MTextDocument.h"
#include "MEditWindow.h"
#include "MPreferences.h"
#include "MUtils.h"
#include "MMessageWindow.h"
#include "MCommands.h"
#include "MGlobals.h"
#include "MStrings.h"
#include "MProject.h"
#include "MSound.h"
#include "MAlerts.h"
#include "MError.h"
#include "MJapiApp.h"

using namespace std;

namespace {
	
const uint32
	kMaxComboListSize =			10;

enum {
	kFindButtonID =				'btnf',
	kReplaceAndFindButtonID =	'btrf',
	kReplaceButtonID =			'btnr',
	kReplaceAllButtonID =		'btra',
	kFindComboboxID =			'find',
	kReplaceComboboxID =	 	'repl',
	kInSelectionCheckboxID = 	'insl',
	kWrapCheckboxID =			'wrap',
	kIgnoreCaseCheckboxID =		'ignc',
	kRegexCheckboxID = 			'regx',
	kEntireWordCheckboxID =		'word',
	kBatchCheckboxID =			'btch',
	
	// --- multi
	
	kMultiFileExpanderID	 =	'exp1',
	kMethodPopupID = 			'meth',
	kRecursiveCheckboxID =		'recu',
	kStartDirComboboxID =		'sdir',
	kTextFilesOnlyCheckboxID =	'txto',
	kEnableFilterCheckboxID =	'ffnm',
	kNameFilterEditboxID =		'filt',
	kBrowseStartDirButtonID =	'chdr',
	
	// --- status
	
	kChasingArrowsID =			301,
	kStatusPanelID =			'curf'
};

enum {
	kMethodDirectory = 			1,
	kMethodOpenWindows = 		2,
	kMethodIncludeFiles =		3
};

enum {
	kReplaceAll_Save =			1,
	kReplaceAll_LeaveOpen =		2
};

const int16
	kFindDialogCollapsedHeight	= 179,
	kFindDialogExpandedHeight	= 281;

}

MFindDialog& MFindDialog::Instance()
{
	static std::auto_ptr<MFindDialog> sInstance;

	if (sInstance.get() == nil)
		sInstance.reset(new MFindDialog);

	return *sInstance;
}

MFindDialog::MFindDialog()
	: MDialog("find-dialog")
	, eIdle(this, &MFindDialog::Idle)
	, mFindAllThread(nil)
	, mFindAllResult(nil)
{
	RestorePosition("find dialog position");
	
	SetChecked(kInSelectionCheckboxID, Preferences::GetInteger("find in selection", 0));
	SetChecked(kWrapCheckboxID, Preferences::GetInteger("find wrap around", 0));
	SetChecked(kIgnoreCaseCheckboxID, Preferences::GetInteger("find ignore case", 0));
	SetChecked(kRegexCheckboxID, Preferences::GetInteger("find regular expression", 0));
	SetChecked(kEntireWordCheckboxID, Preferences::GetInteger("find entire word", 0));
	SetChecked(kRecursiveCheckboxID, Preferences::GetInteger("find recursive", 0));
	SetChecked(kTextFilesOnlyCheckboxID, Preferences::GetInteger("find only TEXT", 1));
	SetChecked(kRecursiveCheckboxID, Preferences::GetInteger("find recursive", 1));

	SetChecked(kEnableFilterCheckboxID, Preferences::GetInteger("find name filter enabled", 1));
	SetText(kNameFilterEditboxID, Preferences::GetString("find name filter", "*.h;*.cpp;*.inl"));

	SetValue(kMethodPopupID, Preferences::GetInteger("find multi method", kMethodDirectory));

	// never set multi-mode automatically
	mMultiMode = false;
	SetExpanded(kMultiFileExpanderID, mMultiMode);

	Preferences::GetArray("find find strings", mFindStrings);
	SetValues(kFindComboboxID, mFindStrings);
	
	Preferences::GetArray("find replace strings", mReplaceStrings);
	SetValues(kReplaceComboboxID, mReplaceStrings);

	Preferences::GetArray("find directories", mStartDirectories);
	SetValues(kStartDirComboboxID, mStartDirectories);
	
	AddRoute(eIdle, gApp->eIdle);
	
//	SetVisible(kChasingArrowsID, false);
	SetText(kStatusPanelID, "");
}

bool MFindDialog::ProcessCommand(
	uint32			inCommand,
	const MMenu*	inMenu,
	uint32			inItemIndex,
	uint32			inModifiers)
{
	bool result = true;
	
	switch (inCommand)
	{
		case cmd_Stop:
			result = Stop();
			break;
		
		default:
			result = MDialog::ProcessCommand(inCommand, inMenu, inItemIndex, inModifiers);
			break;
	}
	
	return result;
}

bool MFindDialog::DoClose()
{
	Preferences::SetInteger("find in selection", IsChecked(kInSelectionCheckboxID));
	Preferences::SetInteger("find wrap around", IsChecked(kWrapCheckboxID));
	Preferences::SetInteger("find ignore case", IsChecked(kIgnoreCaseCheckboxID));
	Preferences::SetInteger("find regular expression", IsChecked(kRegexCheckboxID));
	
	SavePosition("find dialog position");
	
	Preferences::SetArray("find find strings", mFindStrings);
	Preferences::SetArray("find replace strings", mReplaceStrings);
	Preferences::SetArray("find directories", mStartDirectories);

//	Preferences::SetInteger("find multi", GetValue(kMultiFileExpanderID));
	Preferences::SetInteger("find multi method", GetValue(kMethodPopupID));
	Preferences::SetInteger("find only TEXT", IsChecked(kTextFilesOnlyCheckboxID));
	Preferences::SetInteger("find recursive", IsChecked(kRecursiveCheckboxID));

	Preferences::SetInteger("find name filter enabled", IsChecked(kEnableFilterCheckboxID));
	
	string s;
	GetText(kNameFilterEditboxID, s);
	Preferences::SetString("find name filter", s);

	Hide();
	
	return false;
}

void MFindDialog::Select()
{
	RestorePosition("find dialog position");
	Show(nil);
	MWindow::Select();
	SetFocus(kFindComboboxID);
}

bool MFindDialog::OKClicked()
{
	DoFindCommand(cmd_FindNext);
	return false;
}

void MFindDialog::DoFindCommand(
	uint32		inCommand)
{
	// check regular expression first?
	
	if (mMultiMode and IsChecked(kBatchCheckboxID) and mFindAllThread != nil)
	{
		PlaySound("warning");
		return;
	}
	
	mInSelection = IsChecked(kInSelectionCheckboxID);

	string what;
	GetText(kFindComboboxID, what);
	StoreComboText(kFindComboboxID, what, mFindStrings);

	string with;
	GetText(kReplaceComboboxID, with);
	StoreComboText(kReplaceComboboxID, with, mReplaceStrings);

	string where;
	GetText(kStartDirComboboxID, where);
	StoreComboText(kStartDirComboboxID, where, mStartDirectories);

	MTextDocument* doc = MTextDocument::GetFirstTextDocument();

	if (IsExpanded(kMultiFileExpanderID))
	{
		fs::path dir;
		bool recursive, textFilesOnly;
		string filter;
		MMultiMethod method = eMMDirectory;

		recursive = IsChecked(kRecursiveCheckboxID);
		textFilesOnly = IsChecked(kTextFilesOnlyCheckboxID);
		
		if (IsChecked(kEnableFilterCheckboxID))
			GetText(kNameFilterEditboxID, filter);
		
		switch (GetValue(kMethodPopupID))
		{
			case kMethodDirectory:
			{
				dir = where;

				if (not exists(dir) or not is_directory(dir))
					THROW(("Start directory does not exist or is not a directory"));
				
				method = eMMDirectory;
				break;
			}
			
			case kMethodIncludeFiles:
				method = eMMIncludes;
				break;
			
			case kMethodOpenWindows:
				method = eMMOpenWindows;
				break;
		}

		if (IsChecked(kBatchCheckboxID))
		{
			mFindAllThread = new boost::thread(
				boost::bind(&MFindDialog::FindAll, this, what,
					IsChecked(kIgnoreCaseCheckboxID),
					IsChecked(kRegexCheckboxID),
					method, dir, recursive, textFilesOnly, filter));
		}
		else
		{
			mMultiFiles.clear();
			
			FileSet files;
			
			GetFilesForFindAll(method, dir,
				recursive, textFilesOnly, filter, files);
				
			copy(files.begin(), files.end(), back_inserter(mMultiFiles));

			switch (inCommand)
			{
				case cmd_FindNext:
					FindNext();
					break;
				
				case cmd_ReplaceAll:
					switch (DisplayAlert("replace-all-alert"))
					{
						case kReplaceAll_Save:
							ReplaceAll(true);
							break;
						
						case kReplaceAll_LeaveOpen:
							ReplaceAll(false);
							break;
					}
					break;
			}
		}
	}
	else if (IsChecked(kBatchCheckboxID))
	{
		if (doc != nil)
		{
			MMessageList list;
			
			doc->FindAll(what, IsChecked(kIgnoreCaseCheckboxID),
				IsChecked(kRegexCheckboxID), IsChecked(kInSelectionCheckboxID), list);

			if (list.GetCount())
			{
				MMessageWindow* w = new MMessageWindow("");
				w->SetMessages(
					FormatString("Found ^0 hits for ^1",
						list.GetCount(), mFindStrings.front()),
					list);
			}
		}
	}
	else
	{
		if (doc != nil)
			doc->HandleFindDialogCommand(inCommand);
	}
}

void
MFindDialog::SetFindString(
	const string&	inString,
	bool			inReplaceFirst)
{
	string s = Escape(inString);
	
	if (s != GetFindString())
	{
		if (inReplaceFirst and mFindStrings.size() > 0)
			mFindStrings.erase(mFindStrings.begin());
		
		StoreComboText(kFindComboboxID, s, mFindStrings);

		SetChecked(kRegexCheckboxID, false);
		SetChecked(kBatchCheckboxID, false);

		mMultiMode = false;
		SetExpanded(kMultiFileExpanderID, mMultiMode);
	}
}

string MFindDialog::GetFindString()
{
	string result;
	
	if (not mFindStrings.empty())
		result = mFindStrings.front();
	
	if (not GetRegex())
		result = Unescape(result);
	
	return result;
}

void MFindDialog::SetReplaceString(
	const string&	inString)
{
	string s = Escape(inString);

	if (s != GetReplaceString())
	{
		if (mReplaceStrings.size() > 10)
			mReplaceStrings.pop_back();

		StoreComboText(kReplaceComboboxID, s, mReplaceStrings);
	}
}

string MFindDialog::GetReplaceString()
{
	string result;
	
	if (not mReplaceStrings.empty())
		result = mReplaceStrings.front();
	
	if (not GetRegex())
		result = Unescape(result);

	return result;
}

void MFindDialog::ValueChanged(
	uint32		inButonID)
{
	switch (inButonID)
	{
		case kFindButtonID:
			DoFindCommand(cmd_FindNext);
			break;
		
		case kReplaceButtonID:
			DoFindCommand(cmd_Replace);
			break;
		
		case kReplaceAndFindButtonID:
			DoFindCommand(cmd_ReplaceFindNext);
			break;
		
		case kReplaceAllButtonID:
			DoFindCommand(cmd_ReplaceAll);
			break;
		
		case kBrowseStartDirButtonID:
			SelectSearchDir();
			break;
		
		case kMultiFileExpanderID:
			SetVisible(kStatusPanelID, false);
			break;
		
		case kBatchCheckboxID:
		{
			bool batch = IsChecked(kBatchCheckboxID);
		
			SetEnabled(kReplaceAllButtonID, not batch);
			SetEnabled(kReplaceButtonID, not batch);
			SetEnabled(kReplaceAndFindButtonID, not batch);
			break;
		}
		
		case kMethodPopupID:
		{
			switch (GetValue(kMethodPopupID))
			{
				case kMethodDirectory:
					SetEnabled(kStartDirComboboxID, true);
					SetEnabled(kBrowseStartDirButtonID, true);
					SetEnabled(kRecursiveCheckboxID, true);
					SetEnabled(kEnableFilterCheckboxID, true);
					SetEnabled(kNameFilterEditboxID, IsChecked(kEnableFilterCheckboxID));
					SetEnabled(kTextFilesOnlyCheckboxID, true);
					break;
				
				case kMethodIncludeFiles:
					SetEnabled(kStartDirComboboxID, false);
					SetEnabled(kBrowseStartDirButtonID, false);
					SetEnabled(kRecursiveCheckboxID, true);
					SetEnabled(kEnableFilterCheckboxID, true);
					SetEnabled(kNameFilterEditboxID, IsChecked(kEnableFilterCheckboxID));
					SetEnabled(kTextFilesOnlyCheckboxID, true);
					break;
				
				case kMethodOpenWindows:
					SetEnabled(kStartDirComboboxID, false);
					SetEnabled(kBrowseStartDirButtonID, false);
					SetEnabled(kRecursiveCheckboxID, false);
					SetEnabled(kEnableFilterCheckboxID, false);
					SetEnabled(kNameFilterEditboxID, false);
					SetEnabled(kTextFilesOnlyCheckboxID, false);
					break;
			}
			break;
		}
		
		case kEnableFilterCheckboxID:
			SetEnabled(kNameFilterEditboxID, IsChecked(kEnableFilterCheckboxID));
			break;
		
		default:
			MDialog::ValueChanged(inButonID);
			break;
	}
}

bool MFindDialog::GetInSelection() const
{
	return IsChecked(kInSelectionCheckboxID);
}

bool MFindDialog::GetWrap() const
{
	return IsChecked(kWrapCheckboxID);
}

bool MFindDialog::GetIgnoreCase() const
{
	return IsChecked(kIgnoreCaseCheckboxID);
}

bool MFindDialog::GetRegex() const
{
	return IsChecked(kRegexCheckboxID);
}

void MFindDialog::StoreComboText(
	uint32			inID,
	const string&	inActiveString,
	StringArray&	inArray)
{
	inArray.erase(remove(inArray.begin(), inArray.end(), inActiveString), inArray.end());
	inArray.insert(inArray.begin(), inActiveString);
	
	if (inArray.size() > kMaxComboListSize)
		inArray.erase(inArray.begin() + kMaxComboListSize, inArray.end());
	
	SetValues(inID, inArray);
}

void MFindDialog::FindNext()
{
	if (mMultiFiles.size() == 0)
	{
		PlaySound("warning");
		return;
	}
	
	bool found = false;
	
	while (not found and mMultiFiles.size() > 0)
	{
		try
		{
			MUrl file(mMultiFiles.front());
			mMultiFiles.pop_front();
		
			MTextDocument* doc = dynamic_cast<MTextDocument*>(
				MDocument::GetDocumentForURL(file));
			
			if (doc != nil)
				found = doc->DoFindFirst();
			else
			{
				auto_ptr<MTextDocument> newDoc(new MTextDocument(&file));

				if (newDoc->DoFindNext(kDirectionForward))
				{
					doc = newDoc.release();
					found = true;
				}
			}
			
			if (found and doc != nil)
			{
				MSelection s = doc->GetSelection();
				
				gApp->DisplayDocument(doc);
				
				// center the found and selected text
				doc->Select(s.GetAnchor(), s.GetCaret(),
					kScrollCenterSelection);
			}
		}
		catch (...)
		{
			found = false;
		}		
	}
	
	if (not found)
		PlaySound("warning");
}

void MFindDialog::ReplaceAll(
	bool				inSaveToDisk)
{
	mStopFindAll = false;
	
	try
	{
		while (not mStopFindAll and mMultiFiles.size() > 0)
		{
			MUrl file(mMultiFiles.front());
			mMultiFiles.pop_front();
		
			SetStatusString(file.str());
			
			bool found = false;
			
			MTextDocument* doc = dynamic_cast<MTextDocument*>(
				MDocument::GetDocumentForURL(file));
			
			if (doc != nil)
				found = doc->DoFindFirst();
			else
			{
				auto_ptr<MTextDocument> newDoc(new MTextDocument(&file));

				if (newDoc->DoFindFirst())
				{
					doc = newDoc.release();
					found = true;
				}
			}
			
			if (found and doc != nil)
			{
				doc->DoReplaceAll();
				if (inSaveToDisk and doc->DoSave())
				{
					MController* controller = doc->GetFirstController();
					if (controller != nil)
						controller->ProcessCommand(cmd_Close, nil, 0, 0);
					else
						delete doc;
				}
				else
					gApp->DisplayDocument(doc);
			}
		}
	}
	catch (exception& e)
	{
		DisplayError(e);
	}	
}

void MFindDialog::FindAll(
	const string&	inWhat,
	bool			inIgnoreCase,
	bool			inRegex,
	MMultiMethod	inMethod,
	fs::path			inDirectory,
	bool			inRecursive,
	bool			inTextFilesOnly,
	const string&	inFileNameFilter)
{
	mStopFindAll = false;
	
	try
	{
		FileSet files;
		auto_ptr<MMessageList> list(new MMessageList);
		
		GetFilesForFindAll(inMethod, inDirectory,
			inRecursive, inTextFilesOnly, inFileNameFilter, files);
		
		for (FileSet::iterator file = files.begin(); file != files.end(); ++file)
		{
			if (mStopFindAll)
				break;
			
			SetStatusString(file->string());
			MUrl url(*file);
			
			bool searched = false;
			
			gdk_threads_enter();
			MTextDocument* doc = dynamic_cast<MTextDocument*>(MDocument::GetDocumentForURL(url));

			if (doc != nil)
			{
				doc->FindAll(inWhat, inIgnoreCase, inRegex, false, *list.get());
				searched = true;
			}
			gdk_threads_leave();
			
			if (not searched)
				MTextDocument::FindAll(*file, inWhat, inIgnoreCase, inRegex, false, *list.get());
		}
		
		mFindAllResult = list.release();
	}
	catch (exception& e)
	{
		mFindAllResult = new MMessageList;	// flag failure... sucks.. I know
		mFindAllResult->AddMessage(kMsgKindError, fs::path(), 0, 0, 0, "Error in find all, sorry");
		mFindAllResult->AddMessage(kMsgKindError, fs::path(), 0, 0, 0, e.what());
	}	
	catch (...)
	{
		mFindAllResult = new MMessageList;	// flag failure... sucks.. I know
		mFindAllResult->AddMessage(kMsgKindError, fs::path(), 0, 0, 0, "Error in find all, sorry");
	}	
}

void MFindDialog::GetFilesForFindAll(
	MMultiMethod	inMethod,
	const fs::path&	inDirectory,
	bool			inRecursive,
	bool			inTextFilesOnly,
	const string&	inFileNameFilter,
	FileSet&		outFiles)
{
	SetStatusString(inDirectory.string());
	
	switch (inMethod)
	{
		case eMMDirectory:
		{
			uint32 flags = 0;
			
			if (inRecursive)
				flags |= kFileIter_Deep;
			
			if (inTextFilesOnly)
				flags |= kFileIter_TEXTFilesOnly;
			
			MFileIterator iter(inDirectory, flags);
		
			if (inFileNameFilter.c_str())
				iter.SetFilter(inFileNameFilter);
		
			fs::path file;
			while (iter.Next(file))
				outFiles.insert(file);
			break;
		}
		
		case eMMOpenWindows:
		{
			MDocument* doc = MDocument::GetFirstDocument();
			while (doc != nil)
			{
				fs::path file = doc->GetURL().GetPath();
				if (exists(file))
					outFiles.insert(file);
				doc = doc->GetNextDocument();
			}
			break;
		}
		
		case eMMIncludes:
		{
			MProject* project = MProject::Instance();
			if (project != nil)
			{
				vector<fs::path> includePaths;

				project->GetIncludePaths(includePaths);

				sort(includePaths.begin(), includePaths.end());
				includePaths.erase(unique(includePaths.begin(), includePaths.end()), includePaths.end());

				for (vector<fs::path>::iterator p = includePaths.begin(); p != includePaths.end(); ++p)
					GetFilesForFindAll(eMMDirectory, *p, inRecursive, inTextFilesOnly, inFileNameFilter, outFiles);
			}
			break;
		}
	}
}

void MFindDialog::SetStatusString(
	const string&		inMessage)
{
	if (mFindAllThread != nil)
	{
		boost::mutex::scoped_lock lock(mFindDialogMutex);
		mCurrentMultiFile = inMessage;
	}
	else
		SetText(kStatusPanelID, mCurrentMultiFile);
}

void MFindDialog::SelectSearchDir()
{
	fs::path dir;
	if (ChooseDirectory(dir))
		StoreComboText(kStartDirComboboxID, dir.string(), mStartDirectories);
}

void MFindDialog::Idle(
	double	inSystemTime)
{
	if (mFindAllThread != nil)
	{
		boost::mutex::scoped_lock lock(mFindDialogMutex);
	
		if (mFindAllResult != nil)
		{
			auto_ptr<MMessageList> list(mFindAllResult);
			mFindAllResult = nil;

//			SetVisible(kChasingArrowsID, false);
			SetVisible(kStatusPanelID, false);

			mFindAllThread->join();
			delete mFindAllThread;
			
			mFindAllThread = nil;
			
			if (list->GetCount() > 0)
			{
				MMessageWindow* w = new MMessageWindow("");
				w->SetMessages(
					FormatString("Found ^0 hits for ^1",
						list->GetCount(), mFindStrings.front()),
					*list.get());
			}
			else
				PlaySound("warning");
		}
		else
		{
//			SetVisible(kChasingArrowsID, true);
			SetVisible(kStatusPanelID, true);

			SetText(kStatusPanelID, mCurrentMultiFile);
		}
	}
}

bool MFindDialog::Stop()
{
	bool result = false;
	
	if (mFindAllThread != nil)
	{
		mStopFindAll = true;
		result = true;
	}
	
	return result;
}
